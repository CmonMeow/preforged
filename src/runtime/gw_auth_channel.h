#pragma once
#include "gw_dh.h"
#include "gw_auth_crypto.h"
#include <optional>

namespace fw::gw::auth {
// Construct after the version preface, from one complete client seed message.
// The caller sends reply() unencrypted before sending encrypted application data.
class Channel {
    std::array<std::uint8_t, 22> response{};
    std::optional<Cipher> incoming, outgoing;
public:
    Channel(DiffieHellman& server, std::span<const std::uint8_t> client_seed);
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;
    std::span<const std::uint8_t> reply() const { return response; }
    void decrypt(std::span<std::uint8_t> bytes) { incoming->apply(bytes); }
    void encrypt(std::span<std::uint8_t> bytes) { outgoing->apply(bytes); }
};
}
