#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace fw::gw {
// In-memory representation consumed by the researched 2026 portal callback.
// This is not yet proof of the downstream auth packet's wire representation.
using GameToken = std::array<std::uint8_t, 16>;
inline constexpr std::array<unsigned, 16> token_order{3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15};

inline std::string format_token(const GameToken& token) {
    constexpr char hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(36);
    for (unsigned i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) result += '-';
        auto byte = token[token_order[i]];
        result += hex[byte >> 4];
        result += hex[byte & 15];
    }
    return result;
}

// Accept only a complete canonical layout, unlike the permissive client helper.
inline std::optional<GameToken> parse_token(std::string_view value) {
    if (value.size() != 36) return std::nullopt;
    auto digit = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    GameToken result{};
    unsigned position = 0;
    for (unsigned i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10)
            if (value[position++] != '-') return std::nullopt;
        const int hi = digit(value[position++]), lo = digit(value[position++]);
        if (hi < 0 || lo < 0) return std::nullopt;
        result[token_order[i]] = static_cast<std::uint8_t>((hi << 4) | lo);
    }
    return result;
}
}
