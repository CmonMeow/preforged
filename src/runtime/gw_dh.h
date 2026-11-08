#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <memory>

namespace fw::gw::auth {
using PublicValue = std::array<std::uint8_t, 64>;
class DiffieHellman {
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    // CNG DH private blob: header plus four 64-byte big-endian values.
    explicit DiffieHellman(std::span<const std::uint8_t> private_blob);
    ~DiffieHellman();
    DiffieHellman(const DiffieHellman&) = delete;
    DiffieHellman& operator=(const DiffieHellman&) = delete;
    // GW sends and retains little-endian integer byte arrays.
    PublicValue shared(const PublicValue& peer_little_endian);
};
}
