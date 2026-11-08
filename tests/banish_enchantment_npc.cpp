#include "gw_banish_enchantment_npc.h"
#include <cstdlib>

using namespace fw::gw::skills::effects;
static void check(bool ok){if(!ok)std::abort();}

int main(){
    const Scope caster{1,134,1},target{1,14,1};
    Registry effects(target);
    check(fw::gw::skills::find_skill(245)&&fw::gw::skills::find_skill(245)->type==6);
    check(fw::gw::skills::find_skill(254)&&fw::gw::skills::find_skill(254)->type==6);
    check(fw::gw::skills::find_skill(31)&&fw::gw::skills::find_skill(31)->type==4);
    check(effects.apply(target,{8,14,245,0,Category::enchantment,100,5000},Replace::independent).accepted);
    check(effects.apply(target,{13,14,254,0,Category::enchantment,200,5000},Replace::independent).accepted);
    check(effects.apply(target,{22,14,31,0,Category::hex,200,5000},Replace::independent).accepted);
    auto offer=banish_enchantment_offer(caster,target,effects,true,300);
    check(offer&&offer->removed_newest_first.size()==2&&
          offer->removed_newest_first[0].instance==13&&
          offer->removed_newest_first[1].instance==8&&
          offer->disable_events_per_foe==2&&offer->disable_until==6300);
    check(effects.count(target,Category::enchantment,300)==2&&
          effects.count(target,Category::hex,300)==1); // preflight is read-only
    check(!banish_enchantment_offer(caster,target,effects,false,300));
    check(!banish_enchantment_offer(caster,{2,14,1},effects,true,300));
    check(!banish_enchantment_offer(caster,{1,14,2},effects,true,300));
    check(!banish_enchantment_offer(caster,caster,effects,true,300));
    check(!banish_enchantment_offer(caster,target,effects,true,UINT64_MAX-5999));
    Registry unenchanted(target);
    check(unenchanted.apply(target,{22,14,31,0,Category::hex,200,5000},Replace::independent).accepted);
    const auto empty=banish_enchantment_offer(caster,target,unenchanted,true,300);
    check(empty&&empty->removed_newest_first.empty()&&empty->disable_events_per_foe==0);
}
