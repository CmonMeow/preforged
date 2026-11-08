#pragma once
#include "gw_skill_catalog_generated.h"
#include <cmath>
#include <stdexcept>
namespace fw::gw::skills {
// Local Expertise/Touch_skill wiki: primary Ranger only;4%/rank for attacks,
// rituals, touch skills (including touch spells), and all Ranger skills.
inline unsigned expertise_energy_cost(const SkillRecord& skill,unsigned primary,unsigned rank){
    if(rank>20)throw std::invalid_argument("expertise rank outside0..20");
    const auto base=skill.energy_cost();
    if(primary!=2||!(skill.profession==2||(skill.special&2)||skill.type==14||skill.type==20||skill.type==22))return base;
    // Nearest resource integer; the wiki's base1/rank13 entry confirms a0 cost
    // is allowed rather than imposing a guessed minimum1.
    return static_cast<unsigned>(std::nearbyint(double(base)*(100-4*rank)/100));
}
}
