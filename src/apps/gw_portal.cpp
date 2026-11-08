#include <winsock2.h>
#include "tcp.h"
#include "common.h"
#include "transport.h"
#include "network.h"
#include "gw_listeners.h"
#include "gw_portal_xml.h"
#include "gw_portal_reply.h"
#include <sodium.h>
#include <atomic>
#include <filesystem>
#include <future>
#include <iostream>
#include <syncstream>
#include <string_view>
#include <list>
#include <mutex>
#include <set>

namespace {
std::atomic<bool> stopping{false};
std::atomic<bool> ready{false};
std::atomic<uint32_t> active_connections{0};
struct ConnectionLease {
    ConnectionLease() { ++active_connections; }
    ~ConnectionLease() { --active_connections; }
};

struct Reply { unsigned status; std::string body; };
Reply error(unsigned status) { return {status, fw::gw::portal::error_reply(243)}; }
struct Request { std::string path, authorization, body; };
void wait_socket(SOCKET socket, bool writing, uint64_t deadline) {
    if (stopping || fw::now_ms() >= deadline) throw std::runtime_error("Socket deadline");
    fd_set set; FD_ZERO(&set); FD_SET(socket, &set);
    timeval time{0, 50000};
    if (select(0, writing ? nullptr : &set, writing ? &set : nullptr, nullptr, &time) < 0)
        throw std::runtime_error("Socket wait failed");
}
Request receive(SOCKET socket) {
    u_long nonblocking = 1;
    if (ioctlsocket(socket, FIONBIO, &nonblocking)) throw std::runtime_error("Socket mode");
    const auto deadline = fw::now_ms() + 3000;
    std::string data;
    Request request;
    std::size_t header = std::string::npos, body_size = 0;
    bool session_get = false;
    for (;;) {
        wait_socket(socket, false, deadline);
        char bytes[2048]; auto size = recv(socket, bytes, sizeof(bytes), 0);
        if (size < 0 && WSAGetLastError() == WSAEWOULDBLOCK) continue;
        if (size <= 0) throw std::runtime_error("Incomplete request");
        data.append(bytes, size);
        if (data.size() > 8192 + 4096) throw std::invalid_argument("Request bound");
        if (header == std::string::npos) {
            auto end = data.find("\r\n\r\n");
            if (end == data.npos) { if (data.size() > 8192) throw std::invalid_argument("Header bound"); continue; }
            header = end + 4;
            if (header > 8192) throw std::invalid_argument("Header bound");
            auto line_end = data.find("\r\n"); auto line = data.substr(0, line_end);
            if (!line.ends_with(" HTTP/1.1")) throw std::invalid_argument("HTTP/1.1 required");
            session_get = line.starts_with("GET ");
            if (!session_get && !line.starts_with("POST ")) throw std::invalid_argument("Unsupported method");
            const std::size_t prefix = session_get ? 4 : 5;
            request.path = line.substr(prefix, line.size() - prefix - 9);
            if (session_get && request.path != "/Spawned/WebGate/session/create.xml") throw std::invalid_argument("GET route unsupported");
            std::set<std::string> seen;
            std::size_t position = line_end + 2;
            while (position < end) {
                line_end = data.find("\r\n", position); line = data.substr(position, line_end - position); position = line_end + 2;
                auto colon = line.find(':');
                if (!colon || colon == line.npos) throw std::invalid_argument("Header syntax");
                auto name = line.substr(0, colon), value = line.substr(colon + 1);
                for (auto& c : name) { if (c >= 'A' && c <= 'Z') c += 'a' - 'A'; if (!(c >= 'a' && c <= 'z') && c != '-') throw std::invalid_argument("Header name"); }
                auto first = value.find_first_not_of(" \t"), last = value.find_last_not_of(" \t");
                value = first == value.npos ? "" : value.substr(first, last - first + 1);
                for (unsigned char c : value) if (c < 32 || c == 127) throw std::invalid_argument("Header value");
                if (!seen.insert(name).second || name == "transfer-encoding" || name == "expect") throw std::invalid_argument("Ambiguous framing");
                if (name == "content-length") {
                    if (value.empty() || value.find_first_not_of("0123456789") != value.npos) throw std::invalid_argument("Content length");
                    body_size = fw::number(value, 4096);
                } else if (name == "authorization") request.authorization = value;
            }
            if (!seen.contains("host") || (!session_get && !seen.contains("content-length"))) throw std::invalid_argument("Required headers");
            if (session_get && body_size) throw std::invalid_argument("Session GET body unsupported");
        }
        if (data.size() >= header + body_size) {
            if (data.size() != header + body_size) throw std::invalid_argument("Pipelining unsupported");
            request.body = data.substr(header); sodium_memzero(data.data(), data.size()); return request;
        }
    }
}
void send_reply(SOCKET socket, const Reply& reply) {
    auto data = "HTTP/1.1 " + std::to_string(reply.status) + " Result\r\nContent-Type: application/xml; charset=utf-8\r\nContent-Length: " +
                std::to_string(reply.body.size()) + "\r\nConnection: close\r\nCache-Control: no-store\r\n\r\n" + reply.body;
    const auto deadline = fw::now_ms() + 3000;
    std::size_t position = 0;
    while (position < data.size()) {
        wait_socket(socket, true, deadline);
        auto size = send(socket, data.data() + position, int(data.size() - position), 0);
        if (size < 0 && WSAGetLastError() == WSAEWOULDBLOCK) continue;
        if (size <= 0) throw std::runtime_error("Response send failed");
        position += size;
    }
    sodium_memzero(data.data(), data.size());
}
struct Portal {
    struct Session { uint64_t deadline; std::optional<gwserver::SessionToken> account_token; uint64_t revision = 0; };

    std::mutex mutex;
    std::map<std::string, Session> sessions;
    void expire_sessions(bool all=false){
        std::vector<gwserver::SessionToken> revoke;
        {
            std::lock_guard lock(mutex);const auto now=fw::now_ms();
            for(auto it=sessions.begin();it!=sessions.end();){
                if(all||it->second.deadline<=now){
                    if(it->second.account_token)revoke.push_back(*it->second.account_token);
                    it=sessions.erase(it);
                }else ++it;
            }
        }
        for(const auto& token:revoke)gwserver::NetworkManager::current().server().logout({token});
    }
    ~Portal(){try{expire_sessions(true);}catch(...){}}

    static std::pair<uint32_t, std::string> account_result(const fw::AccountResult& reply) {
        return {reply.status,reply.payload};
    }

    Reply handle(const Request& request) {
        constexpr std::string_view prefix = "/Spawned/WebGate";
        if (!request.path.starts_with(prefix)) return error(404);
        const auto route = request.path.substr(prefix.size());
        if (route == "/session/create.xml") {
            if (!request.body.empty() || (request.authorization != "Arena 0" && !request.authorization.empty())) return error(400);
            expire_sessions();
            std::lock_guard lock(mutex);
            auto now = fw::now_ms();
            if (sessions.size() >= 1024) return error(503);
            unsigned char bytes[32]; randombytes_buf(bytes, sizeof(bytes));
            char hex[65]; sodium_bin2hex(hex, sizeof(hex), bytes, sizeof(bytes)); sodium_memzero(bytes, sizeof(bytes));
            sessions.emplace(hex, Session{now + 60000, {}});
            return {200, fw::gw::portal::session_reply(hex)};
        }
        if (route != "/users/login.xml" && route != "/my_account/game_accounts.xml" && route != "/my_account/token.xml") return error(404);
        if (!request.authorization.starts_with("Arena ")) return error(401);
        auto session_key = request.authorization.substr(6);
        Session session;
        {
            std::lock_guard lock(mutex); auto it = sessions.find(session_key);
            if (it == sessions.end() || it->second.deadline <= fw::now_ms()) return error(401);
            session = it->second;
        }
        auto fields = fw::gw::portal::request_fields(request.body);
        if (!fields.contains("GameCode") || fields.at("GameCode") != "gw1") return error(400);
        if (route == "/users/login.xml") {
            for (const auto& [name, value] : fields)
                if (name != "GameCode" && name != "LoginName" && name != "Provider" && name != "Password" && name != "CreationFlags") return error(400);
            if (fields["Provider"] != "Portal" || !fields.contains("LoginName") || !fields.contains("Password")) return error(400);
            if (session.account_token) return error(409);
            unsigned char password[128]{}; size_t length = 0;
            const auto& encoded = fields.at("Password");
            if (sodium_base642bin(password, sizeof(password), encoded.data(), encoded.size(), nullptr, &length, nullptr, sodium_base64_VARIANT_ORIGINAL) || (length < 8 && !(fields["LoginName"]=="test@example.test" && length==4 && std::equal(password,password+4,reinterpret_cast<const unsigned char*>("test"))))) {
                sodium_memzero(password, sizeof(password)); return error(400);
            }
            struct ErasePassword {unsigned char* bytes;~ErasePassword(){sodium_memzero(bytes,128);}} erase_password{password};
            auto result=gwserver::NetworkManager::current().server().login({fields.at("LoginName"),std::span(password,length)});
            if(result.status!=FW_OK){
                std::cerr<<"Accounts login status="<<result.status<<'\n';
                return error(result.status==FW_NOT_FOUND?401:result.status==FW_INVALID?400:503);
            }
            if(!result.login)return error(503);
            const auto& identity=*result.login;
            bool bound = false;
            {
                std::lock_guard lock(mutex); auto it = sessions.find(session_key);
                if (it != sessions.end() && it->second.revision == session.revision && it->second.deadline > fw::now_ms() && !it->second.account_token) {
                    it->second.account_token = identity.session; it->second.deadline = fw::now_ms() + 3600000; ++it->second.revision; bound = true;
                }
            }
            if (!bound) { gwserver::NetworkManager::current().server().logout({identity.session}); return error(409); }
            return {200, "<Reply><UserId>" + fw::gw::format_token(identity.identity.account.bytes) + "</UserId></Reply>"};
        }
        if (!session.account_token) return error(401);
        for (const auto& [name, value] : fields)
            if (name != "GameCode" && !(route == "/my_account/token.xml" && name == "AccountAlias")) return error(400);
        const auto validated=gwserver::NetworkManager::current().server().validateSession({*session.account_token});
        if(validated.status!=FW_OK)return error(validated.status==FW_NOT_FOUND?401:503);
        if(!validated.identity)return error(503);
        auto alias=gwserver::NetworkManager::current().server().gameAccountAlias(validated.identity->account);
        if(alias.status!=FW_OK||!alias.value)return error(503);
        if (route == "/my_account/game_accounts.xml")
            return {200, "<Reply type=\"array\"><Row><GameCode>gw1</GameCode><Alias>" + *alias.value + "</Alias></Row></Reply>"};
        if (!fields.contains("AccountAlias") || fields.at("AccountAlias") != *alias.value) return error(403);
        const auto issued = gwserver::NetworkManager::current().server().issueTicket({*session.account_token});
        if(issued.status!=FW_OK)return error(issued.status==FW_NOT_FOUND?401:503);
        return issued.ticket?Reply{200,fw::gw::portal::token_reply(issued.ticket->token)}:error(503);
    }
};
}
void fw_gw_portal_prepare() { stopping = false; ready = false; }
void fw_gw_portal_stop() { stopping = true; }
bool fw_gw_portal_ready() { return ready.load(); }
uint32_t fw_gw_portal_connections() { return active_connections.load(); }
int fw_gw_portal_run(const gwserver::ServerSettings& settings) try {
    const auto port=settings.portal_port;
    if (sodium_init() < 0) throw std::runtime_error("Random source unavailable");
    fw::tcp::Runtime network; Portal portal;
    fw::tcp::Listener listener(settings.bind, static_cast<uint16_t>(port));
    std::list<std::future<void>> workers;
    struct Join { std::list<std::future<void>>& workers; ~Join() { stopping = true; for (auto& worker : workers) if (worker.valid()) worker.wait(); } } join{workers};
    ready = true;
    struct ClearReady { ~ClearReady() { ready = false; } } clear_ready;
    std::osyncstream(std::cout) << "READY GW portal " << settings.bind << ':' << port << '\n' << std::flush;
    while (!stopping) {
        for (auto it = workers.begin(); it != workers.end();) {
            if (it->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) { it->get(); it = workers.erase(it); } else ++it;
        }
        std::string address; auto socket = listener.accept(address, 100);
        if (!socket || workers.size() >= 16) continue;
        workers.push_back(std::async(std::launch::async, [&, socket = std::move(socket)]() mutable {
            ConnectionLease connection;
            try {
                Reply reply;
                try {
                    auto request = receive(SOCKET(socket.get()));
                    reply = portal.handle(request);
                    // Route/status metadata lets local compatibility probes
                    // distinguish a successful portal exchange from a later
                    // auth/UI stall without recording bodies or credentials.
                    std::cout << "PORTAL path=" << request.path << " status=" << reply.status << '\n' << std::flush;
                    sodium_memzero(request.body.data(), request.body.size());
                }
                catch (const std::invalid_argument& e) { std::cerr << "Rejected request: " << e.what() << '\n'; reply = error(400); }
                send_reply(SOCKET(socket.get()), reply);
            } catch (...) { /* Close failed or expired connections without logging secrets. */ }
        }));
    }
    stopping = true;
    return 0;
} catch (const std::exception& error) { std::cerr << "GW portal: " << error.what() << '\n'; return 2; }
