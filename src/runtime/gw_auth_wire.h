#pragma once
#include "gw_token.h"
#include <span>
#include <algorithm>

namespace fw::gw::auth {
enum class Parse { incomplete, invalid, complete };
// Exact 16-byte auth preface captured from the current supplied build 38888.
inline constexpr std::array<std::uint8_t, 16> version_preface{
    0x00, 0x04, 0x0c, 0x00, 0xe8, 0x97, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00};
inline constexpr std::uint32_t client_protocol_build = 38888;
inline constexpr std::uint32_t client_2022_protocol_build = 37130;
// Admission stays pinned to the current client unless an explicit profile is
// supplied. Older auth framing alone does not establish world compatibility.
inline Parse parse_preface(std::span<const std::uint8_t> bytes,
                           std::uint32_t expected_build = client_protocol_build) {
    auto expected = version_preface;
    for(unsigned i=0;i<4;++i)expected[4+i]=static_cast<std::uint8_t>(expected_build>>(8*i));
    auto count = std::min(bytes.size(), version_preface.size());
    if (!std::equal(bytes.begin(), bytes.begin() + count, expected.begin())) return Parse::invalid;
    return bytes.size() < version_preface.size() ? Parse::incomplete : Parse::complete;
}
struct Login {
    std::uint32_t transaction = 0;
    GameToken user{}, ticket{};
    std::u16string character, login;
    std::size_t consumed = 0;
};
struct Hello {
    std::uint32_t build = 0;
    GameToken client_id{};
};
// The pinned 44fbd687 current-client sender/serializer produces a 16-bit opcode
// followed by six DWORDs (gw_current_instance_probe.py). Keep decoding independent of
// admission policy: an instance request is meaningful only after its account
// and selected character have been authenticated.
struct InstanceRequest {
    std::uint32_t transaction = 0;
    std::uint32_t map_type = 0;
    std::uint32_t map_id = 0;
    std::uint32_t region = 0;
    std::uint32_t district = 0;
    std::uint32_t language = 0;
};
inline Parse parse_instance_request(std::span<const std::uint8_t> bytes, InstanceRequest& output) {
    auto read16 = [&](std::size_t at) { return unsigned(bytes[at]) | (unsigned(bytes[at + 1]) << 8); };
    if (bytes.size() < 2) return Parse::incomplete;
    if ((read16(0) & 0x7fff) != 0x29) return Parse::invalid;
    if (bytes.size() < 26) return Parse::incomplete;
    auto read32 = [&](std::size_t at) {
        return std::uint32_t(bytes[at]) | (std::uint32_t(bytes[at + 1]) << 8) |
               (std::uint32_t(bytes[at + 2]) << 16) | (std::uint32_t(bytes[at + 3]) << 24);
    };
    InstanceRequest value;
    value.transaction = read32(2);
    value.map_type = read32(6);
    value.map_id = read32(10);
    value.region = read32(14);
    value.district = read32(18);
    value.language = read32(22);
    output = value;
    return Parse::complete;
}
// Opcode 2 is sent by the handshake-success callback before portal login.
inline Parse parse_hello(std::span<const std::uint8_t> bytes, Hello& output,
                        std::uint32_t expected_build = client_protocol_build) {
    if (bytes.size() < 2) return Parse::incomplete;
    if ((unsigned(bytes[0]) | ((unsigned(bytes[1]) & 0x7f) << 8)) != 2) return Parse::invalid;
    if (bytes.size() < 22) return Parse::incomplete;
    Hello value;
    for (unsigned i = 0; i < 4; ++i) value.build |= std::uint32_t(bytes[2 + i]) << (8 * i);
    if (value.build != expected_build) return Parse::invalid;
    std::copy_n(bytes.begin() + 6, 16, value.client_id.begin());
    output = value;
    return Parse::complete;
}

// Decoded plaintext stream, after the version-specific encryption handshake.
// Descriptor: 38,404,1005,1005,1417,2017 in the researched 2026 executable.
inline Parse parse_login(std::span<const std::uint8_t> bytes, Login& output) {
    auto word = [&](std::size_t at) { return unsigned(bytes[at]) | (unsigned(bytes[at + 1]) << 8); };
    if (bytes.size() < 2) return Parse::incomplete;
    if ((word(0) & 0x7fff) != 0x38) return Parse::invalid;
    if (bytes.size() < 38) return Parse::incomplete;
    Login value;
    for (unsigned i = 0; i < 4; ++i) value.transaction |= std::uint32_t(bytes[2 + i]) << (8 * i);
    std::copy_n(bytes.begin() + 6, 16, value.user.begin());
    std::copy_n(bytes.begin() + 22, 16, value.ticket.begin());
    std::size_t position = 38;
    auto string = [&](std::u16string& text, unsigned capacity) {
        if (bytes.size() - position < 2) return Parse::incomplete;
        auto count = word(position); position += 2;
        if (count >= capacity) return Parse::invalid;
        if (bytes.size() - position < count * 2) return Parse::incomplete;
        for (unsigned i = 0; i < count; ++i) {
            auto unit = word(position); position += 2;
            if (!unit || (unit >= 0xdc00 && unit <= 0xdfff)) return Parse::invalid;
            text += static_cast<char16_t>(unit);
            if (unit >= 0xd800 && unit <= 0xdbff) {
                if (++i >= count) return Parse::invalid;
                unit = word(position); position += 2;
                if (unit < 0xdc00 || unit > 0xdfff) return Parse::invalid;
                text += static_cast<char16_t>(unit);
            }
        }
        return Parse::complete;
    };
    auto status = string(value.character, 20); if (status != Parse::complete) return status;
    status = string(value.login, 32); if (status != Parse::complete) return status;
    value.consumed = position; output = std::move(value); return Parse::complete;
}
}
