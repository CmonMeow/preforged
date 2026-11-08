#include "gw_auth_wire.h"
#include "gw_auth_responses.h"
#include <fstream>
#include <iostream>
#include <map>
#include <string>

int main(int argc, char** argv) try {
    using namespace fw::gw::auth;
    if (argc != 2) throw std::runtime_error("auth-data retail fixture path required");
    std::ifstream input(argv[1]);
    if (!input) throw std::runtime_error("auth-data fixtures missing");
    std::map<std::string, std::vector<std::uint8_t>> fixtures;
    std::string name, hex;
    while (input >> name >> hex) {
        if (hex.size() % 2) throw std::runtime_error("invalid auth-data fixture");
        std::vector<std::uint8_t> bytes;
        for (std::size_t i = 0; i < hex.size(); i += 2)
            bytes.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
        if (!fixtures.emplace(name, std::move(bytes)).second)
            throw std::runtime_error("duplicate auth-data fixture");
    }
    if (fixtures.size() != 4) throw std::runtime_error("four auth-data fixtures required");
    for (const auto& phase : {std::string("initial"), std::string("reconnect")}) {
        const auto& frame = fixtures.at(phase + "_login");
        Login login; login.transaction = 99; login.login = u"unchanged";
        for (std::size_t size = 0; size < frame.size(); ++size) {
            if (parse_login(std::span(frame).first(size), login) != Parse::incomplete ||
                login.transaction != 99 || login.login != u"unchanged")
                throw std::runtime_error("fragmented login changed request correlation");
        }
        auto coalesced = frame;
        coalesced.insert(coalesced.end(), frame.begin(), frame.end());
        if (parse_login(coalesced, login) != Parse::complete || login.consumed != frame.size() ||
            login.character != u"Hero" || login.login != u"Private" ||
            login.transaction != (phase == "initial" ? 1U : 5U))
            throw std::runtime_error("retail login request correlation/framing mismatch");
        AuthDataLookupResult response;
        response.transaction = login.transaction;
        response.data = {0x83, 4, 7, 0, 0xf1};
        auto encoded = response.encode(false); encoded[1] |= 0x80;
        if (encoded != fixtures.at(phase + "_result"))
            throw std::runtime_error("2022 opaque settings response must echo pending login ID");
        if (phase == "reconnect") {
            response.transaction = 1;
            auto stale = response.encode(false); stale[1] |= 0x80;
            if (stale == encoded) throw std::runtime_error("stale first-login response accepted");
        }
    }
    AuthDataLookupResult response;
    response.transaction = 5; response.data.resize(2048, 0x5a);
    auto maximum = response.encode(false);
    if (maximum.size() != 2056 || maximum[6] != 0 || maximum[7] != 8)
        throw std::runtime_error("2022 settings maximum ARRAY8 bound mismatch");
    response.data.push_back(0);
    bool rejected = false;
    try { (void)response.encode(false); } catch (const std::invalid_argument&) { rejected = true; }
    if (!rejected) throw std::runtime_error("oversized settings accepted");
    response.data.clear();
    auto empty = response.encode(false);
    if (empty != std::vector<std::uint8_t>{22, 0, 5, 0, 0, 0, 0, 0})
        throw std::runtime_error("2022 settings must not invent status prefix");
    std::cout << "PASS retail auth-data initial/reconnect request correlation and bounds\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n'; return 1;
}
