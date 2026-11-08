#include "gw_selfless_target_policy.h"
#include <stdexcept>
#include <iostream>
using namespace fw::gw::skills;
using namespace fw::gw::skills::effects;
static void check(bool value){if(!value)throw std::runtime_error("Selfless target policy regression");}
int main(){
    const auto plan=*selfless_spirit(1952);
    const auto& spell=*find_skill(281);
    const SelflessTargetFacts ally{1,2,SelflessRelation::ally,true};
    const SelflessTargetFacts self{1,1,SelflessRelation::self,true};
    for(auto family:{SelflessCastFamily::direct_ally_heal,SelflessCastFamily::ally_status_heal,
        SelflessCastFamily::ally_enchantment,SelflessCastFamily::ally_weapon_spell,
        SelflessCastFamily::ally_condition_removal,SelflessCastFamily::ally_hex_removal,
        SelflessCastFamily::ally_hex_copy,SelflessCastFamily::ally_condition_transfer,
        SelflessCastFamily::ally_skill_copy,SelflessCastFamily::targeted_ally_resurrection}){
        check(classify_selfless_target(family,ally)==SelflessTarget::other_ally);
        check(quote_selfless_targeted_spell(plan,true,spell,family,ally,5)->after==2);
        check(quote_selfless_targeted_spell(plan,true,spell,family,ally,1)->after==0);
        check(quote_selfless_targeted_spell(plan,false,spell,family,ally,5)->after==5);
        check(quote_selfless_targeted_spell(plan,true,spell,family,self,5)->after==5);
        check(quote_selfless_targeted_spell(plan,true,*find_skill(293),family,ally,5)->after==5);
        for(auto invalid:{SelflessTargetFacts{1,2,SelflessRelation::ally,false},
            SelflessTargetFacts{1,2,SelflessRelation::unknown,true},
            SelflessTargetFacts{1,2,SelflessRelation::neutral,true},
            SelflessTargetFacts{1,2,SelflessRelation::foe,true},
            SelflessTargetFacts{1,1,SelflessRelation::ally,true},
            SelflessTargetFacts{1,2,SelflessRelation::self,true},
            SelflessTargetFacts{0,2,SelflessRelation::ally,true},
            SelflessTargetFacts{1,0,SelflessRelation::ally,true}})
            check(!quote_selfless_targeted_spell(plan,true,spell,family,invalid,5));
    }
    for(auto family:{SelflessCastFamily::self_effect,SelflessCastFamily::party_effect,
        SelflessCastFamily::caster_area,SelflessCastFamily::corpse_effect,
        SelflessCastFamily::ground_effect,SelflessCastFamily::untargeted_effect,
        SelflessCastFamily::nonspell})
        check(quote_selfless_targeted_spell(plan,true,spell,family,ally,5)->after==5);
    check(!quote_selfless_targeted_spell(plan,true,spell,SelflessCastFamily::unsupported,ally,5));
    check(!quote_selfless_targeted_spell(plan,true,spell,SelflessCastFamily::hostile_target,ally,5));
    check(quote_selfless_targeted_spell(plan,true,spell,SelflessCastFamily::hostile_target,
        {1,2,SelflessRelation::foe,true},5)->after==5);
    std::cout<<"Selfless semantic target policy passed; detached\n";
}
