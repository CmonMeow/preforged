#include "gw_dh.h"
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

namespace fw::gw::auth {
namespace {
void check(NTSTATUS status) {
    if (status < 0) throw std::runtime_error("GW DH cryptographic operation failed: " + std::to_string(static_cast<unsigned long>(status)));
}
struct Key {
    BCRYPT_KEY_HANDLE handle = nullptr;
    ~Key() { if (handle) BCryptDestroyKey(handle); }
};
struct Secret {
    BCRYPT_SECRET_HANDLE handle = nullptr;
    ~Secret() { if (handle) BCryptDestroySecret(handle); }
};
}
struct DiffieHellman::Impl {
    BCRYPT_ALG_HANDLE provider = nullptr;
    Key key;
    std::array<std::uint8_t, 200> public_blob{};
    explicit Impl(std::span<const std::uint8_t> blob) {
        if (blob.size() != 264) throw std::invalid_argument("GW DH private blob size");
        BCRYPT_DH_KEY_BLOB header{}; std::memcpy(&header, blob.data(), sizeof(header));
        if (header.dwMagic != BCRYPT_DH_PRIVATE_MAGIC || header.cbKey != 64)
            throw std::invalid_argument("GW DH private blob header");
        check(BCryptOpenAlgorithmProvider(&provider, BCRYPT_DH_ALGORITHM, nullptr, 0));
        try {
            std::array<std::uint8_t, 264> copy{};
            std::copy(blob.begin(), blob.end(), copy.begin());
            auto status = BCryptImportKeyPair(provider, nullptr, BCRYPT_DH_PRIVATE_BLOB, &key.handle,
                                              copy.data(), ULONG(copy.size()), 0);
            SecureZeroMemory(copy.data(), copy.size());
            check(status);
            std::copy_n(blob.begin(), public_blob.size(), public_blob.begin());
            header.dwMagic = BCRYPT_DH_PUBLIC_MAGIC;
            std::memcpy(public_blob.data(), &header, sizeof(header));
        } catch (...) { BCryptCloseAlgorithmProvider(provider, 0); provider = nullptr; throw; }
    }
    ~Impl() {
        if (key.handle) { BCryptDestroyKey(key.handle); key.handle = nullptr; }
        if (provider) BCryptCloseAlgorithmProvider(provider, 0);
    }
    PublicValue shared(const PublicValue& peer) {
        PublicValue big{}; std::reverse_copy(peer.begin(), peer.end(), big.begin());
        PublicValue one{}; one.back() = 1;
        PublicValue limit{}; std::copy_n(public_blob.begin() + 8, 64, limit.begin());
        for (auto i = limit.size(); i > 0; --i) { if (limit[i - 1]--) break; }
        if (!(one < big && big < limit)) throw std::invalid_argument("GW DH peer outside valid range");
        auto blob = public_blob;
        std::copy(big.begin(), big.end(), blob.begin() + 136);
        Key remote;
        check(BCryptImportKeyPair(provider, nullptr, BCRYPT_DH_PUBLIC_BLOB, &remote.handle, blob.data(), ULONG(blob.size()), 0));
        Secret agreement;
        check(BCryptSecretAgreement(key.handle, remote.handle, &agreement.handle, 0));
        PublicValue result{}; ULONG written = 0;
        auto status = BCryptDeriveKey(agreement.handle, BCRYPT_KDF_RAW_SECRET, nullptr,
                                     result.data(), ULONG(result.size()), &written, 0);
        if (status < 0 || written != result.size()) {
            SecureZeroMemory(result.data(), result.size());
            throw std::runtime_error("GW DH raw secret derivation failed");
        }
        return result;
    }
};
DiffieHellman::DiffieHellman(std::span<const std::uint8_t> blob) : impl(std::make_unique<Impl>(blob)) {}
DiffieHellman::~DiffieHellman() = default;
PublicValue DiffieHellman::shared(const PublicValue& peer) { return impl->shared(peer); }
}
