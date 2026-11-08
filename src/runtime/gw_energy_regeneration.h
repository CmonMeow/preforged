#pragma once
#include "gw_vitals.h"
#include <algorithm>
#include <cstdint>
#include <limits>

namespace fw::gw::energy {
// Local May2021 wiki evidence, read without a live/official session:
// Energy.html SHA256bc13f6ddd1c100f0b38dfcbdfc40411d090f93aa5067df9a4fec813ff19930d3
// Energy_Storage.html SHA256d4bd04543891d18bcf616b930d5185d0be33527d3f526f35495827c86b9600cb
// Every profession: innate20energy/2pips;1pip produces1energy each3seconds.
// EnergyStorage gives3maximumenergy/rank, without changing regeneration.
inline void valid_primary(unsigned primary){if(!primary||primary>10)throw std::invalid_argument("energy primary outside1..10");}
inline constexpr unsigned innate_capacity=20;
inline int innate_pips(unsigned primary){valid_primary(primary);return 2;}
struct ArmorBonus {unsigned capacity=0;int pips=0;};
// Wiki Energy.html describes these as BASIC ARMOR bonuses, not innate
// profession benefits. Only apply after actual modeled armor is established.
inline ArmorBonus basic_armor(unsigned primary){
    valid_primary(primary);
    switch(primary){case 1:return {0,0};case 2:return {5,1};case 7:case 10:return {5,2};case 9:return {10,0};default:return {10,2};}
}
inline int regeneration_pips(unsigned primary,int armor_bonus=0,int effects_bonus=0){
    valid_primary(primary);
    if(armor_bonus<-100||armor_bonus>100||effects_bonus<-100||effects_bonus>100)
        throw std::invalid_argument("energy pip modifier outside bound");
    return std::clamp(innate_pips(primary)+armor_bonus+effects_bonus,-10,10);
}
inline unsigned storage_bonus(unsigned primary,unsigned effective_rank){
    valid_primary(primary);if(effective_rank>20)throw std::invalid_argument("energy storage effective rank outside0..20");
    return primary==6?3*effective_rank:0;
}
inline unsigned maximum_energy(unsigned primary,unsigned effective_storage_rank,unsigned capacity_without_storage){
    const auto bonus=storage_bonus(primary,effective_storage_rank);
    if(capacity_without_storage>1000000||capacity_without_storage>UINT32_MAX-bonus)
        throw std::invalid_argument("energy capacity outside bound");
    return capacity_without_storage+bonus;
}
struct Step {bool changed=false;unsigned delta=0;std::uint64_t used_ms=0,discarded_ms=0;};
// Current energy changes by the same amount as its capacity. Keep negative
// energy as authoritative debt while the unsigned UI view remains zero.
inline void resize(vitals::Resources& resources,unsigned capacity,std::uint32_t& debt){
    if(!capacity||capacity>1000000||resources.energy>resources.max_energy||debt>1000000||(debt&&resources.energy))
        throw std::invalid_argument("invalid energy capacity/debt state");
    const auto current=std::int64_t(resources.energy)-debt+std::int64_t(capacity)-resources.max_energy;
    if(current< -1000000||current>capacity)throw std::invalid_argument("energy resize outside bound");
    resources.max_energy=capacity;
    debt=current<0?static_cast<std::uint32_t>(-current):0;
    resources.energy=current>0?static_cast<std::uint32_t>(current):0;
}
struct Gain {
    unsigned repaid=0,restored=0;
    unsigned total()const{return repaid+restored;}
};
// Current Energy is signed through debt; a ceiling may be below zero while
// max_energy remains the positive nominal capacity used by the client view.
// Gaining Energy never lowers current Energy that already exceeds a ceiling.
inline Gain gain(vitals::Resources& resources,std::uint32_t& debt,unsigned amount,std::int64_t ceiling){
    if(resources.max_energy>1000000||resources.energy>resources.max_energy||debt>1000000||
       (debt&&resources.energy)||ceiling< -1000000||ceiling>resources.max_energy)
        throw std::invalid_argument("invalid Energy gain/ceiling state");
    const auto current=std::int64_t(resources.energy)-debt;
    const auto available=static_cast<std::uint64_t>(std::max<std::int64_t>(0,ceiling-current));
    const auto accepted=static_cast<unsigned>(std::min<std::uint64_t>(amount,available));
    const auto repaid=std::min(accepted,debt);debt-=repaid;
    const auto restored=accepted-repaid;resources.energy+=restored;
    return {repaid,restored};
}
inline Gain gain(vitals::Resources& resources,std::uint32_t& debt,unsigned amount){
    return gain(resources,debt,amount,resources.max_energy);
}
class Regeneration {
    // One energy is3000pip-ms. Remainder is a fraction of one current energy,
    // not time credit. It is never rounded away on ordinary simulation ticks.
    std::uint64_t remainder_=0;
public:
    static constexpr std::uint64_t maximum_elapsed_ms=30000;
    std::uint64_t remainder()const{return remainder_;}
    void restore_fraction(std::uint64_t fraction){
        if(fraction>=3000)throw std::invalid_argument("invalid fractional Energy remainder");
        remainder_=fraction;
    }
    void reset(){remainder_=0;}
    Step advance(vitals::Resources& resources,std::uint64_t elapsed_ms,int pips){
        std::uint32_t debt=0;return advance(resources,elapsed_ms,pips,debt);
    }
    Step advance(vitals::Resources& resources,std::uint64_t elapsed_ms,int pips,std::uint32_t& debt){
        return advance(resources,elapsed_ms,pips,debt,resources.max_energy);
    }
    Step advance(vitals::Resources& resources,std::uint64_t elapsed_ms,int pips,std::uint32_t& debt,std::int64_t ceiling){
        if(pips< -10||pips>10)throw std::invalid_argument("invalid Energy regeneration pips");
        if(resources.max_energy>1000000||resources.energy>resources.max_energy||debt>1000000||(debt&&resources.energy)||
           ceiling< -1000000||ceiling>resources.max_energy)throw std::invalid_argument("invalid energy/debt/ceiling state");
        const auto used=std::min(elapsed_ms,maximum_elapsed_ms);
        Step result{false,0,used,elapsed_ms-used};
        if(!resources.health||!resources.max_energy){reset();return result;}
        if(!pips||!used)return result;
        if(pips<0){
            // Degeneration spends fractional Energy before whole points and
            // stops at zero; it cannot deepen capacity-change debt.
            if(debt||(!resources.energy&&!remainder_)){reset();return result;}
            const auto precise=std::uint64_t(resources.energy)*3000+remainder_;
            const auto loss=std::min(precise,used*static_cast<unsigned>(-pips));
            const auto remaining=precise-loss;
            resources.energy=static_cast<unsigned>(remaining/3000);
            remainder_=remaining%3000;
            result.changed=loss!=0;
            return result;
        }
        if(std::int64_t(resources.energy)-debt>=ceiling){reset();return result;}
        const auto accumulated=remainder_+used*static_cast<unsigned>(pips);
        const auto gained=energy::gain(resources,debt,static_cast<unsigned>(accumulated/3000),ceiling);
        remainder_=accumulated%3000;
        result.delta=gained.total();result.changed=result.delta!=0;
        if(std::int64_t(resources.energy)-debt>=ceiling)reset();return result;
    }
};
}
