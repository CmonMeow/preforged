#pragma once
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <span>
#include <set>
namespace fw::gw::party {
using Packet=std::vector<std::uint8_t>;
inline void word(Packet& p,std::uint16_t n){p.push_back(n&255);p.push_back(n>>8);}
// 37130 callbacks 7d7d50, 7d77c0, 7d7d90: build, append, publish.
inline std::vector<Packet> roster(std::uint16_t party,std::span<const std::uint16_t> players,
        std::uint16_t recipient){
    if(!party||players.empty()||players.size()>12)throw std::invalid_argument("invalid party roster");
    std::set<std::uint16_t> unique;
    for(auto player:players)if(!player||!unique.insert(player).second)
        throw std::invalid_argument("invalid party member identity");
    if(!unique.contains(recipient))throw std::invalid_argument("party recipient is not a member");
    std::vector<Packet> packets;
    Packet begin;word(begin,465);word(begin,party);
    packets.push_back(std::move(begin));
    for(auto player:players){
        Packet member;word(member,458);word(member,party);word(member,player);member.push_back(1);
        packets.push_back(std::move(member));
    }
    Packet end;word(end,466);word(end,party);
    packets.push_back(std::move(end));
    // 433 -> 7d6930 selects the published party and leader flag.
    Packet selected;word(selected,433);word(selected,party);selected.push_back(recipient==players.front());
    packets.push_back(std::move(selected));
    return packets;
}
inline std::vector<Packet> solo(std::uint16_t party,std::uint16_t player){
    return roster(party,std::span(&player,1),player);
}
inline Packet ready(std::uint16_t party,std::uint16_t player,bool value){
    if(!party||!player)throw std::invalid_argument("invalid party member identity");
    Packet p;word(p,464);word(p,party);word(p,player);p.push_back(value);return p;
}
}
