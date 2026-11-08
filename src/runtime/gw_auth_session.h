#pragma once
#include "gw_auth_channel.h"
#include "gw_auth_wire.h"
#include "gw_auth_responses.h"
#include "gw_client_profile.h"
#include "gw_character_change.h"
#include "gw_social_wire.h"
#include "gw_session_lifetime.h"
#include <vector>
#include <stdexcept>

namespace fw::gw::auth {
// Bounded pre-authentication stream. A decoded login is not an authenticated user.
// The socket owner enforces a wall-clock deadline and consumes the ticket via Accounts.
class Session {
public:
    enum class State { preface, seed, client_info, client_hash, hello, login, ready, failed };
    struct PostLoginCommand {
        std::uint16_t opcode=0;
        std::uint32_t transaction=0, map_type=0, map_id=0, region=0, district=0, language=0;
        std::uint32_t field0=0, field1=0, field2=0, field3=0;
        std::uint32_t value=0;
        std::size_t wire_size=0;
        std::vector<std::uint8_t> payload;
        std::u16string character_name;
    };
private:
    DiffieHellman& server;
    State phase = State::preface;
    std::optional<Channel> channel;
    std::vector<std::uint8_t> pending, response;
    Hello greeting{};
    Login request{};
    std::optional<InstanceRequest> unauthenticated_instance_request;
    bool optional_seen = false;
    bool connection_hint_seen = false;
    bool prelogin_response_seen = false;
    unsigned keepalives = 0;
    std::uint32_t session_id;
    const std::uint32_t expected_build;
    const char* failure_reason = "none";
    std::uint16_t observed_opcode = 0xffff;
    std::size_t observed_packet_size = 0;
    std::size_t last_authenticated_input_size = 0;
    std::vector<PostLoginCommand> post_login_commands;
    std::vector<std::uint8_t> rejected_authenticated_bytes;
    std::array<std::uint8_t, 16> observed_preface{};
    std::size_t observed_preface_size = 0;
    void send(std::vector<std::uint8_t> packet) {
        channel->encrypt(packet);
        response.insert(response.end(), packet.begin(), packet.end());
    }
    void consume(std::size_t count) { pending.erase(pending.begin(), pending.begin() + count); }
    void reject() { phase = State::failed; pending.clear(); response.clear(); channel.reset(); }
public:
    explicit Session(DiffieHellman& key, std::uint32_t id = 1,
            std::uint32_t build = client_protocol_build)
        : server(key), session_id(id), expected_build(build) {
        if (!fw::gw::client_profile(build)) throw std::invalid_argument("unsupported GW client build");
        pending.reserve(512);
    }
    State state() const { return phase; }
    const char* failure() const { return failure_reason; }
    std::uint16_t observed() const { return observed_opcode; }
    std::size_t observed_size_total() const { return observed_packet_size; }
    const std::optional<InstanceRequest>& early_instance_request() const { return unauthenticated_instance_request; }
    std::span<const std::uint8_t> rejected_authenticated_input() const { return rejected_authenticated_bytes; }
    std::size_t authenticated_buffered_size() const { return pending.size(); }
    std::uint16_t authenticated_pending_opcode() const {
        if (pending.size() < 2) return 0xffff;
        return static_cast<std::uint16_t>((unsigned(pending[0]) | (unsigned(pending[1]) << 8)) & 0x7fff);
    }
    std::span<const std::uint8_t> preface_bytes() const { return std::span(observed_preface).first(observed_preface_size); }
    unsigned keepalive_count() const { return keepalives; }
    bool has_connection_hint() const { return connection_hint_seen; }
    const Login& login() const {
        if (phase != State::ready) throw std::logic_error("GW login is not decoded");
        return request;
    }
    std::vector<std::uint8_t> take_reply() { auto result = std::move(response); response.clear(); return result; }
    std::vector<PostLoginCommand> take_post_login_commands() {
        auto result = std::move(post_login_commands); post_login_commands.clear(); return result;
    }
    void queue_response(std::vector<std::uint8_t> packet) {
        if (phase != State::ready || !channel) throw std::logic_error("GW auth response before ticket authentication");
        send(std::move(packet));
    }
    void observe_authenticated(std::span<const std::uint8_t> bytes) {
        if (phase != State::ready || !channel || bytes.empty()) return;
        // Authenticated settings frames are larger than the login preface
        // bound; allow their bounded frame plus a socket read of lookahead.
        if (pending.size()>4096 || bytes.size() > 4096 - pending.size()) {
            failure_reason="authenticated stream bound";reject();return;
        }
        last_authenticated_input_size = bytes.size();
        auto start = pending.size();
        pending.insert(pending.end(), bytes.begin(), bytes.end());
        channel->decrypt(std::span(pending).subspan(start));
        for (;;) {
            if (pending.size() < 2) return;
            const auto raw_header = unsigned(pending[0]) | (unsigned(pending[1]) << 8);
            const auto opcode = static_cast<std::uint16_t>(raw_header & 0x7fff);
            std::size_t packet_size = 0;
            std::uint32_t transaction = 0;
            PostLoginCommand command;
            if(expected_build==client_2022_protocol_build&&(opcode==26||opcode==27)){
                fw::gw::social::Request request;
                const auto result=fw::gw::social::parse(pending,request);
                if(result==fw::gw::social::Request::Result::incomplete)return;
                if(result!=fw::gw::social::Request::Result::complete){failure_reason="invalid social request";reject();return;}
                packet_size=request.consumed;transaction=request.transaction;
                command.payload.assign(pending.begin(),pending.begin()+packet_size);
            } else if (opcode == 0 || opcode == 0x23) {
                packet_size = 6;
            } else if (opcode == 0x0e) {
                // AUTH_CMSG_SET_PLAYER_STATUS is a DWORD presence value.
                packet_size = 6;
                if (pending.size() >= packet_size)
                    for (unsigned i=0;i<4;++i) command.value |= std::uint32_t(pending[2+i]) << (8*i);
            } else if (opcode == 0x0f) {
                if (pending.size() < 4) return;
                const auto count = unsigned(pending[2]) | (unsigned(pending[3]) << 8);
                if (count > 92) { failure_reason = "hardware-info bound"; reject(); return; }
                packet_size = 4 + count + 16;
            } else if (opcode == 0x35) {
                packet_size = 6;
                if (pending.size() >= packet_size)
                    for (unsigned i=0;i<4;++i) transaction |= std::uint32_t(pending[2+i]) << (8*i);
            } else if (opcode == 0x29) {
                packet_size = 26;
                if (pending.size() >= packet_size) {
                    auto read32 = [&](std::size_t offset) {
                        return std::uint32_t(pending[offset]) | (std::uint32_t(pending[offset+1]) << 8) |
                               (std::uint32_t(pending[offset+2]) << 16) | (std::uint32_t(pending[offset+3]) << 24);
                    };
                    transaction=read32(2); command.map_type=read32(6); command.map_id=read32(10);
                    command.region=read32(14); command.district=read32(18); command.language=read32(22);
                }
            } else if (opcode == 9 && expected_build == client_2022_protocol_build) {
                if(pending.size()<8)return;
                const auto count=std::uint16_t(pending[6])|(std::uint16_t(pending[7])<<8);
                if(!count||count>19){failure_reason="legacy character name bounds";reject();return;}
                const auto summary_offset=8+count*2;
                if(pending.size()<summary_offset+2)return;
                const auto summary_count=std::uint16_t(pending[summary_offset])|
                    (std::uint16_t(pending[summary_offset+1])<<8);
                if(summary_count>64){failure_reason="legacy character summary bounds";reject();return;}
                packet_size=summary_offset+2+summary_count;
                if(pending.size()>=packet_size){
                    for(unsigned i=0;i<4;++i)transaction|=std::uint32_t(pending[2+i])<<(8*i);
                    for(unsigned i=0;i<count;++i){
                        auto unit=std::uint16_t(pending[8+i*2])|(std::uint16_t(pending[9+i*2])<<8);
                        if(!unit||unit>=0xd800){failure_reason="legacy character name encoding";reject();return;}
                        command.character_name.push_back(static_cast<char16_t>(unit));
                    }
                    if(!transaction){failure_reason="legacy character transaction";reject();return;}
                    command.payload.assign(pending.begin()+summary_offset+2,pending.begin()+packet_size);
                }
            } else if ((opcode == 0x0a||opcode==7) && expected_build == client_2022_protocol_build) {
                CharacterChange2022 parsed;
                const auto status=parse_character_change_2022(pending,parsed,opcode);
                if(status==CharacterChange2022::State::incomplete)return;
                if(status==CharacterChange2022::State::invalid){
                    observed_opcode=opcode;observed_packet_size=pending.size();
                    failure_reason="2022 character change fields";reject();return;
                }
                transaction=parsed.transaction;command.character_name=std::move(parsed.name);
                packet_size=parsed.consumed;
            } else if (opcode == 0x0a) {
                packet_size=46;
                if(pending.size()>=packet_size) {
                    for(unsigned i=0;i<4;++i) transaction|=std::uint32_t(pending[2+i])<<(8*i);
                    bool terminated=false;
                    for(unsigned i=0;i<20;++i) {
                        auto unit=std::uint16_t(pending[6+i*2])|(std::uint16_t(pending[7+i*2])<<8);
                        if(terminated) { if(unit) { failure_reason="character name padding"; reject(); return; } continue; }
                        if(!unit) { terminated=true; continue; }
                        if(unit>=0xdc00 && unit<=0xdfff) { failure_reason="character name encoding"; reject(); return; }
                        command.character_name.push_back(static_cast<char16_t>(unit));
                        if(unit>=0xd800 && unit<=0xdbff) {
                            if(++i>=20) { failure_reason="character name encoding"; reject(); return; }
                            auto low=std::uint16_t(pending[6+i*2])|(std::uint16_t(pending[7+i*2])<<8);
                            if(low<0xdc00 || low>0xdfff) { failure_reason="character name encoding"; reject(); return; }
                            command.character_name.push_back(static_cast<char16_t>(low));
                        }
                    }
                    if(!terminated || !transaction || command.character_name.empty()) {
                        failure_reason="character change fields"; reject(); return;
                    }
                }
            } else if (opcode == 0x26) {
                // AUTH_CLIENT_0038 is a 16-bit header plus one byte.
                packet_size=3;
                if (pending.size() >= packet_size) command.value=pending[2];
            } else if (opcode == 0x25) {
                // AUTH_CMSG_REQUEST_GUILD_HALL has a fixed frame in the
                // supplied client packet table: transaction, four DWORDs,
                // then a 16-byte opaque guild/account token.
                packet_size=38;
                if (pending.size() >= packet_size) {
                    for (unsigned i=0;i<4;++i)
                        transaction |= std::uint32_t(pending[2+i]) << (8*i);
                    auto read32=[&](std::size_t offset) {
                        return std::uint32_t(pending[offset]) | (std::uint32_t(pending[offset+1]) << 8) |
                               (std::uint32_t(pending[offset+2]) << 16) | (std::uint32_t(pending[offset+3]) << 24);
                    };
                    command.field0=read32(6); command.field1=read32(10);
                    command.field2=read32(14); command.field3=read32(18);
                    command.payload.assign(pending.begin()+22,pending.begin()+38);
                }
            } else if (opcode == 0x0d) {
                // AUTH_CMSG_DISCONNECT is a 16-bit header plus one DWORD.
                packet_size=6;
                if (pending.size() >= packet_size)
                    for (unsigned i=0;i<4;++i) command.value |= std::uint32_t(pending[2+i]) << (8*i);
            } else if (opcode == 0x21) {
                // AUTH_CLIENT_0033 is a fixed ten-byte request in the supplied
                // reference packet table: header followed by two DWORDs.
                packet_size=10;
                if(pending.size()>=packet_size) {
                    auto read32=[&](std::size_t offset) {
                        return std::uint32_t(pending[offset]) | (std::uint32_t(pending[offset+1])<<8) |
                               (std::uint32_t(pending[offset+2])<<16) | (std::uint32_t(pending[offset+3])<<24);
                    };
                    command.field0=read32(2); command.field1=read32(6);
                }
            } else if (opcode == 0x20) {
                // Donor AUTH_CLIENT_0032 describes transaction + ARRAY_8.
                // The captured current-client request uses this variable-length
                // frame; retain its payload for response research.
                if(pending.size()<8) return;
                for(unsigned i=0;i<4;++i) transaction|=std::uint32_t(pending[2+i])<<(8*i);
                const auto count=unsigned(pending[6])|(unsigned(pending[7])<<8);
                // Pinned 2022 FUN_0048b900 splits uploads into 512-byte chunks.
                if(count>512) { failure_reason="auth command payload bound"; reject(); return; }
                packet_size=8+count;
                if(pending.size()>=packet_size)
                    command.payload.assign(pending.begin()+8,pending.begin()+static_cast<std::ptrdiff_t>(packet_size));
            } else {
                observed_opcode = opcode;
                observed_packet_size = pending.size();
                rejected_authenticated_bytes.assign(pending.begin(), pending.end());
                failure_reason = "unknown post-login auth opcode"; reject(); return;
            }
            if (pending.size() < packet_size) return;
            observed_opcode = opcode;
            observed_packet_size = packet_size;
            command.opcode=opcode; command.transaction=transaction; command.wire_size=packet_size;
            post_login_commands.push_back(command);
            pending.erase(pending.begin(), pending.begin() + packet_size);
        }
    }
    void feed(std::span<const std::uint8_t> bytes) {
        if (phase == State::failed) return;
        if (bytes.empty()) return;
        if (phase == State::ready || bytes.size() > 512 - pending.size()) { reject(); return; }
        auto start = pending.size();
        pending.insert(pending.end(), bytes.begin(), bytes.end());
        if (channel) channel->decrypt(std::span(pending).subspan(start));
        try {
            for (;;) {
                if (phase == State::preface) {
                    auto status = parse_preface(pending,expected_build);
                    observed_preface_size = std::min(pending.size(), observed_preface.size());
                    std::copy_n(pending.begin(), observed_preface_size, observed_preface.begin());
                    if (status == Parse::invalid) { failure_reason = "version preface mismatch"; reject(); return; }
                    if (status == Parse::incomplete) return;
                    consume(16); phase = State::seed;
                } else if (phase == State::seed) {
                    if (pending.empty()) return;
                    if (pending[0] != 0) { reject(); return; }
                    if (pending.size() < 2) return;
                    auto size = pending[1];
                    if (size < 3 || size > 66) { reject(); return; }
                    if (pending.size() < size) return;
                    channel.emplace(server, std::span(pending).first(size));
                    response.assign(channel->reply().begin(), channel->reply().end());
                    consume(size);
                    channel->decrypt(pending);
                    phase = State::client_info;
                } else if (phase == State::client_info) {
                    if (pending.size() < 2) return;
                    auto header = unsigned(pending[0]) | (unsigned(pending[1]) << 8);
                    if ((header & 0x7fff) != 0x0001) { failure_reason = "computer-info opcode mismatch"; reject(); return; }
                    std::size_t position = 2;
                    auto counted_utf16 = [&]() -> Parse {
                        if (pending.size() - position < 2) return Parse::incomplete;
                        auto count = unsigned(pending[position]) | (unsigned(pending[position + 1]) << 8);
                        position += 2;
                        if (count >= 32) return Parse::invalid;
                        if (pending.size() - position < count * 2) return Parse::incomplete;
                        for (unsigned i = 0; i < count; ++i) {
                            auto unit = unsigned(pending[position]) | (unsigned(pending[position + 1]) << 8);
                            position += 2;
                            if (!unit || (unit >= 0xdc00 && unit <= 0xdfff)) return Parse::invalid;
                            if (unit >= 0xd800 && unit <= 0xdbff) {
                                if (++i >= count || pending.size() - position < 2) return Parse::invalid;
                                unit = unsigned(pending[position]) | (unsigned(pending[position + 1]) << 8);
                                position += 2;
                                if (unit < 0xdc00 || unit > 0xdfff) return Parse::invalid;
                            }
                        }
                        return Parse::complete;
                    };
                    auto first = counted_utf16();
                    if (first == Parse::incomplete) return;
                    if (first == Parse::invalid) { failure_reason = "invalid computer-info field"; reject(); return; }
                    auto second = counted_utf16();
                    if (second == Parse::incomplete) return;
                    if (second == Parse::invalid) { failure_reason = "invalid computer-info field"; reject(); return; }
                    consume(position); phase = State::client_hash;
                } else if (phase == State::client_hash) {
                    if (pending.size() < 2) return;
                    auto header = unsigned(pending[0]) | (unsigned(pending[1]) << 8);
                    if ((header & 0x7fff) != 0x0002) { failure_reason = "computer-hash opcode mismatch"; reject(); return; }
                    if (pending.size() < 22) return;
                    consume(22); phase = State::hello;
                    // The researched client waits for session-info after its computer info/hash handshake.
                    SessionInfo session_info; session_info.server_salt=session_id;
                    send(session_info.encode(expected_build));
                } else if (phase == State::hello) {
                    if (pending.size() >= 2) {
                        auto opcode = unsigned(pending[0]) | ((unsigned(pending[1]) & 0x7f) << 8);
                        // The 2022 reconnect asks for a server response before login.
                        // This acknowledges only the control transaction; it cannot
                        // advance the authentication state or access account data.
                        if (expected_build == client_2022_protocol_build && opcode == 0x35) {
                            if (pending.size() < 6) return;
                            if (prelogin_response_seen) { failure_reason = "duplicate pre-login response request"; reject(); return; }
                            std::uint32_t transaction=0;
                            for(unsigned i=0;i<4;++i)transaction|=std::uint32_t(pending[2+i])<<(8*i);
                            ServerResponse reply;reply.transaction=transaction;send(reply.encode());
                            ErrorMessage result;result.transaction=transaction;send(result.encode());
                            prelogin_response_seen=true;consume(6);continue;
                        }
                        if (opcode == 0x29) {
                            InstanceRequest instance;
                            auto status = parse_instance_request(pending, instance);
                            if (status == Parse::incomplete) return;
                            if (status == Parse::complete) {
                                observed_opcode = 0x29;
                                observed_packet_size = 26;
                                unauthenticated_instance_request = instance;
                                failure_reason = "instance request before authenticated login";
                                reject(); return;
                            }
                        }
                        if (opcode == 0 || opcode == 0x23) {
                            if (pending.size() < 6) return;
                            if (opcode == 0) {
                                if(keepalives<std::numeric_limits<unsigned>::max())++keepalives;
                                std::vector<std::uint8_t> pong(pending.begin(), pending.begin() + 6);
                                pong[1] = 0x00; send(std::move(pong));
                            } else {
                                if (connection_hint_seen) { failure_reason = "duplicate connection hint"; reject(); return; }
                                connection_hint_seen = true;
                            }
                            consume(6); continue;
                        }
                        if (opcode == 0x16) {
                            if (pending.size() < 6) return;
                            if (optional_seen) { failure_reason = "duplicate optional hint"; reject(); return; }
                            consume(6); optional_seen = true; continue;
                        }
                        // The observed stock client proceeds from session-info directly to portal login;
                        // opcode 2 is an optional compatibility greeting, not a required gate.
                        if (opcode == 0x38) {
                            auto status = parse_login(pending, request);
                            if (status == Parse::invalid) { failure_reason = "portal login malformed"; reject(); return; }
                            if (status == Parse::incomplete) return;
                            consume(request.consumed);
                            if (!pending.empty()) { failure_reason = "trailing pre-auth command"; reject(); return; }
                            phase = State::ready; return;
                        }
                    }
                    auto status = parse_hello(pending, greeting,expected_build);
                    if (status == Parse::invalid) {
                        observed_packet_size = pending.size();
                        if (pending.size() >= 2)
                            observed_opcode = static_cast<std::uint16_t>(unsigned(pending[0]) | ((unsigned(pending[1]) & 0x7f) << 8));
                        failure_reason = pending.size() >= 2 &&
                            (unsigned(pending[0]) | ((unsigned(pending[1]) & 0x7f) << 8)) == 2
                            ? "hello build mismatch" : "hello opcode mismatch";
                        reject(); return;
                    }
                    if (status == Parse::incomplete) return;
                    consume(22); phase = State::login;
                } else if (phase == State::login) {
                    if (pending.size() < 2) return;
                    auto opcode = unsigned(pending[0]) | ((unsigned(pending[1]) & 0x7f) << 8);
                    if (opcode == 0 || opcode == 0x23) {
                        if (pending.size() < 6) return;
                        if (opcode == 0) {
                            if (++keepalives > 8) { reject(); return; }
                            std::vector<std::uint8_t> pong(pending.begin(), pending.begin() + 6);
                            pong[1] = 0x00; send(std::move(pong));
                        } else {
                            if (connection_hint_seen) { reject(); return; }
                            connection_hint_seen = true;
                        }
                        consume(6); continue;
                    }
                    if (opcode == 0x16) {
                        if (optional_seen) { reject(); return; }
                        if (pending.size() < 6) return;
                        // Opaque client hint; never interpreted as identity or authority.
                        consume(6); optional_seen = true; continue;
                    }
                    auto status = parse_login(pending, request);
                    if (status == Parse::invalid) { reject(); return; }
                    if (status == Parse::incomplete) return;
                    consume(request.consumed);
                    // No post-login commands are allowed before ticket authentication.
                    if (!pending.empty()) { reject(); return; }
                    phase = State::ready; return;
                } else return;
            }
        } catch (...) { reject(); throw; }
    }
};
// Renew only for complete, validated control frames or handshake progress.
// Receiving partial bytes, duplicate hints or rejected traffic is not activity.
class PendingLifetime {
    fw::gw::IdleLifetime idle_;
public:
    explicit PendingLifetime(std::uint64_t now):idle_(now,60000){}
    std::uint64_t deadline()const{return idle_.deadline();}
    void observed(std::uint64_t now,Session::State before,Session::State after,
            unsigned previous_keepalives,unsigned current_keepalives){
        if(after==Session::State::failed)return;
        if(before!=after||(after==Session::State::hello&&current_keepalives>previous_keepalives))idle_.activity(now);
    }
};
}
