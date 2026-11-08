#pragma once
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <framework/module.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace fw {
inline uint64_t number(std::string_view s, uint64_t max = UINT64_MAX) {
    uint64_t n = 0;
    auto r = std::from_chars(s.data(), s.data() + s.size(), n);
    if (s.empty() || r.ec != std::errc{} || r.ptr != s.data() + s.size() || n > max)
        throw std::runtime_error("invalid unsigned integer");
    return n;
}
inline bool identifier(std::string_view s) {
    return !s.empty() && s.size() <= 48 && std::all_of(s.begin(), s.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' ||
               c == '_';
    });
}
inline bool account_name_valid(std::string_view s) {
    if (identifier(s)) return true;
    if (s.size() < 6 || s.size() > 48) return false;
    auto at = s.find('@');
    if (at == 0 || at == s.npos || at + 1 >= s.size() || s.find('@', at + 1) != s.npos) return false;
    auto local = s.substr(0, at), domain = s.substr(at + 1);
    if (local.size() > 32 || local.front() == '.' || local.back() == '.') return false;
    for (size_t i = 0; i < local.size(); ++i) {
        char c = local[i];
        if (c == '.' && (i == 0 || local[i - 1] == '.')) return false;
        if (c == '.') continue;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '+' || c == '-')) return false;
    }
    if (domain.size() < 3 || domain.front() == '.' || domain.back() == '.') return false;
    bool domain_dot = false;
    for (char c : domain) {
        if (c == '.') { domain_dot = true; continue; }
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-')) return false;
    }
    return domain_dot;
}
inline std::vector<std::string> split(std::string_view s, char sep = '\t') {
    std::vector<std::string> out;
    size_t pos = 0;
    for (;;) {
        auto end = s.find(sep, pos);
        out.emplace_back(s.substr(pos, end == s.npos ? s.size() - pos : end - pos));
        if (end == s.npos)
            break;
        pos = end + 1;
    }
    return out;
}
inline fw_bytes bytes(std::string_view s) {
    return {reinterpret_cast<const uint8_t *>(s.data()), static_cast<uint32_t>(s.size())};
}
inline std::string text(fw_bytes b) {
    return b.size ? std::string(reinterpret_cast<const char *>(b.data), b.size) : std::string{};
}
inline std::string json(std::string_view s) {
    std::string r = "\"";
    constexpr char hex[] = "0123456789abcdef";
    for (unsigned char c : s) {
        if (c == '"' || c == '\\') {
            r += '\\';
            r += char(c);
        } else if (c < 32) {
            r += "\\u00";
            r += hex[c >> 4];
            r += hex[c & 15];
        } else
            r += char(c);
    }
    return r + '"';
}
struct Writer {
    std::vector<uint8_t> data;
    void u(uint64_t n, unsigned size) {
        for (unsigned i = 0; i < size; ++i)
            data.push_back(uint8_t(n >> (i * 8)));
    }
    void str(std::string_view s) {
        if (s.size() > FW_MAX_PAYLOAD)
            throw std::runtime_error("field too large");
        u(s.size(), 2);
        data.insert(data.end(), s.begin(), s.end());
    }
};
struct Reader {
    const std::vector<uint8_t> &data;
    size_t pos = 0;
    uint64_t u(unsigned size) {
        if (size > data.size() - pos)
            throw std::runtime_error("truncated message");
        uint64_t n = 0;
        for (unsigned i = 0; i < size; ++i)
            n |= uint64_t(data[pos++]) << (i * 8);
        return n;
    }
    std::string str() {
        auto n = u(2);
        if (n > FW_MAX_PAYLOAD || n > data.size() - pos)
            throw std::runtime_error("invalid field length");
        std::string s(reinterpret_cast<const char *>(data.data() + pos), size_t(n));
        pos += size_t(n);
        return s;
    }
};
struct Message {
    bool response = false;
    uint64_t id = 0, deadline = 0, target_incarnation = 0;
    uint32_t status = 0, version = 1;
    std::string service, instance, operation, payload;
};
inline std::vector<uint8_t> encode(const Message &m) {
    Writer w;
    w.u(0x31574653, 4);
    w.u(m.response ? 2 : 1, 1);
    w.u(m.id, 8);
    w.u(m.deadline, 8);
    w.u(m.target_incarnation, 8);
    w.u(m.status, 4);
    w.u(m.version, 4);
    w.str(m.service);
    w.str(m.instance);
    w.str(m.operation);
    w.str(m.payload);
    if (w.data.size() > 1100)
        throw std::runtime_error("message exceeds datagram contract");
    return w.data;
}
inline Message decode(const std::vector<uint8_t> &data) {
    Reader r{data};
    if (r.u(4) != 0x31574653)
        throw std::runtime_error("bad magic");
    auto kind = r.u(1);
    if (kind != 1 && kind != 2)
        throw std::runtime_error("bad kind");
    Message m;
    m.response = kind == 2;
    m.id = r.u(8);
    m.deadline = r.u(8);
    m.target_incarnation = r.u(8);
    m.status = uint32_t(r.u(4));
    m.version = uint32_t(r.u(4));
    m.service = r.str();
    m.instance = r.str();
    m.operation = r.str();
    m.payload = r.str();
    if (r.pos != data.size() || !m.id || !identifier(m.service) ||
        (!m.instance.empty() && !identifier(m.instance)) || !identifier(m.operation))
        throw std::runtime_error("invalid envelope");
    return m;
}
} // namespace fw
