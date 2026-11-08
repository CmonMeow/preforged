#include "gw_auth_channel.h"
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <stdexcept>

namespace fw::gw::auth {
Channel::Channel(DiffieHellman& server, std::span<const std::uint8_t> message) {
    if (message.size() < 3 || message.size() > 66 || message[0] != 0 || message[1] != message.size())
        throw std::invalid_argument("GW client seed framing");
    PublicValue peer{};
    std::copy(message.begin() + 2, message.end(), peer.begin());
    auto shared = server.shared(peer);
    response[0] = 1; response[1] = 22;
    if (BCryptGenRandom(nullptr, response.data() + 2, 20, BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) {
        SecureZeroMemory(shared.data(), shared.size());
        throw std::runtime_error("GW server seed generation failed");
    }
    Seed combined{};
    for (unsigned i = 0; i < 20; ++i) combined[i] = shared[i] ^ response[i + 2];
    incoming.emplace(combined);
    outgoing.emplace(combined);
    SecureZeroMemory(combined.data(), combined.size());
    SecureZeroMemory(shared.data(), shared.size());
}
}
