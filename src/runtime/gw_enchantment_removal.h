#pragma once
#include "gw_skill_effect_plan.h"
#include <compare>
namespace fw::gw::skills::effects {
struct EnchantmentRemovalPlan {
    unsigned skill=0,rank=0,damage=0,energy=0,health=0;float range=1248;
    unsigned count=1,steal=0,monk_loss=0;bool lose_all_energy=false;unsigned minimum_health_percent=0;bool compare_enchantments=false;std::uint64_t bleeding_ms=0,poison_per_remaining_ms=0;unsigned hexed_recharge_percent=0,target_energy_loss=0;
    auto operator<=>(const EnchantmentRemovalPlan&)const=default;
};
struct EnchantmentRemovalOutcome {bool target_hexed=false;};
// The local pre-Reforged wiki distinguishes Drain's Health gain from healing.
// Shatter removes the enchantment before applying its armor-ignoring damage.
inline std::optional<EnchantmentRemovalPlan> enchantment_removal(unsigned skill,unsigned rank=0){
    if(rank>20)return {};
    if(skill==1061){const auto* row=find_skill(skill);
        if(!row||row->type!=5||row->campaign!=2||row->profession!=5||row->attribute!=3||
           row->energy_cost()!=10||row->activation!=2.f||row->aftercast!=.75f||
           row->recharge!=30||row->scale0!=4||row->scale15!=10||
           row->caster_body_resource!=2077||row->target_body_resource!=980)return {};
        EnchantmentRemovalPlan plan{skill,rank};
        plan.target_energy_loss=interpolate(row->scale0,row->scale15,rank);
        return plan;
    }
    if(skill==1347){const auto* row=find_skill(skill);
        if(!row||row->type!=5||row->profession!=5||row->attribute!=3||row->energy_cost()!=10||
           row->activation!=1.f||row->aftercast!=.75f||row->recharge!=15||row->scale0!=20||row->scale15!=50||
           row->caster_body_resource!=1604||row->target_body_resource!=2077)return {};
        return EnchantmentRemovalPlan{1347,rank,0,0,0,1248,1,0,0,false,0,false,0,0,interpolate(20,50,rank)};
    }
    if(skill==936){const auto* row=find_skill(skill);
        if(!row||row->type!=5||row->profession!=4||row->attribute!=7||row->energy_cost()!=5||
           row->activation!=1.f||row->aftercast!=.75f||row->recharge!=20||row->scale0!=3||row->scale15!=10||
           row->caster_body_resource!=1622||row->target_body_resource!=2077)return {};
        return EnchantmentRemovalPlan{936,rank,0,0,0,1248,1,0,0,false,0,false,0,1000ull*interpolate(3,10,rank)};
    }
    if(skill==955){const auto* row=find_skill(skill);
        if(!row||row->type!=5||row->profession!=4||row->attribute!=7||row->energy_cost()!=5||
           row->activation!=1.f||row->aftercast!=.75f||row->recharge!=15||row->scale0!=1||row->scale15!=1||
           row->bonus0!=5||row->bonus15!=25||row->caster_body_resource!=1623||row->target_body_resource!=2077)return {};
        return EnchantmentRemovalPlan{955,rank,0,0,0,1248,1,0,0,false,0,false,1000ull*interpolate(5,25,rank)};
    }
    if(skill==877){const auto* row=find_skill(skill);
        if(!row||row->type!=5||row->profession!=5||row->attribute!=51||row->energy_cost()!=5||
           row->activation!=1.f||row->aftercast!=.75f||row->recharge!=15||
           row->caster_body_resource!=2077||row->target_body_resource!=958)return {};
        return EnchantmentRemovalPlan{877,rank,0,0,0,1248,1,0,0,false,0,true};
    }
    if(skill==766){const auto* row=find_skill(skill);
        if(!row||row->type!=5||row->profession!=4||row->attribute!=51||row->energy_cost()!=10||
           row->activation!=2.f||row->aftercast!=.75f||row->recharge!=15||row->scale0!=50||row->scale15!=50||
           row->caster_body_resource!=2077||row->target_body_resource!=829)return {};
        return EnchantmentRemovalPlan{766,rank,0,0,0,1248,UINT32_MAX,0,0,false,50};
    }
    if(skill==882){const auto* row=find_skill(skill);
        if(!row||row->type!=7||row->profession!=5||row->attribute!=51||row->energy_cost()!=0||
           row->activation!=1.f||row->aftercast!=.75f||row->recharge!=15||row->caster_body_resource!=976||row->target_body_resource!=977)return {};
        return EnchantmentRemovalPlan{882,rank,0,0,0,1248,1,0,0,true};
    }
    if(skill!=68&&skill!=69&&skill!=141&&skill!=143)return {};
    const auto* row=find_skill(skill);const unsigned profession=skill==141||skill==143?4:5;
    const unsigned attribute=skill==141?7:skill==143?4:skill==68?3:2;
    if(!row||row->type!=5||row->profession!=profession||row->attribute!=attribute)return {};
    if(skill==141)return EnchantmentRemovalPlan{skill,rank,0,0,0,1248,interpolate(row->bonus0,row->bonus15,rank),0,interpolate(row->scale0,row->scale15,rank)};
    if(skill==143)return EnchantmentRemovalPlan{skill,rank,0,0,0,1248,interpolate(row->scale0,row->scale15,rank),interpolate(row->bonus0,row->bonus15,rank)};
    if(skill==69)return EnchantmentRemovalPlan{skill,rank,interpolate(row->scale0,row->scale15,rank)};
    return EnchantmentRemovalPlan{skill,rank,0,interpolate(row->scale0,row->scale15,rank),interpolate(row->bonus0,row->bonus15,rank)};
}
inline bool valid_enchantment_removal(const EnchantmentRemovalPlan& plan){const auto expected=enchantment_removal(plan.skill,plan.rank);return expected&&*expected==plan;}
}
