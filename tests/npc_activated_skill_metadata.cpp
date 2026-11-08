#include "gw_npc_activated_skill.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Activated NPC skill metadata line "+std::to_string(at.line()));
}
int main()try{
    using namespace combat::ai::activated;
    // Same immediate damage family must not erase non-spell identity.
    check(kind(155)==Kind::skill);check(kind(133)==Kind::spell);
    Reviewed touch{{155,0},*skills::effects::shadow_damage(155,0),"explicit synthetic rank0"};
    check(touch.valid()&&touch.effect.range==144&&touch.effect.damage==20&&!touch.effect.steal_above_half);
    const auto* touch_row=skills::find_skill(155);const auto* obsidian_row=skills::find_skill(219);
    check(touch_row&&touch_row->overcast==5&&!(touch_row->special&1));
    check(obsidian_row&&obsidian_row->overcast==5&&(obsidian_row->special&1));
    check(!Reviewed{{219,0},*skills::effects::shadow_damage(219,0),"unsupported active Overcast"}.valid());
    auto tampered=touch;tampered.effect.damage=21;check(!tampered.valid());
    tampered=touch;tampered.effect.rank=12;check(!tampered.valid());
    tampered=touch;tampered.evidence.clear();check(!tampered.valid());
    check(!Reviewed{{102,0},*skills::effects::shadow_damage(102,0),"unsupported followup"}.valid());
    check(!Reviewed{{133,0},*skills::effects::shadow_damage(133,0),"unsupported sacrifice"}.valid());
    check(!kind(0));
    std::cout<<"Typed NPC immediate skill metadata preserves non-spell identity and canonical effects\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
