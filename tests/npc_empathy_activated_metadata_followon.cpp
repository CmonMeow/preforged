// NEW unregistered typed immediate NPC hex metadata contract.
#include "gw_npc_activated_skill.h"
#include <stdexcept>
#include <iostream>
using namespace fw::gw;
void check(bool ok){if(!ok)throw std::runtime_error("NPC typed Empathy metadata");}
int main(){using combat::ai::activated::Reviewed;
 for(unsigned skill:{26u,3151u}){const auto entry=Reviewed::empathy_spell({static_cast<std::uint16_t>(skill),0},0,"authored NPC Empathy rank");check(entry&&entry->valid()&&entry->empathy_effect&&entry->effect_range()==1248&&!entry->effect.skill);auto bad=*entry;++bad.empathy_effect->damage;check(!bad.valid());bad=*entry;bad.effect=*skills::effects::shadow_damage(155,0);check(!bad.valid());}
 Reviewed touch{{155,0},*skills::effects::shadow_damage(155,0),"legacy typed damage"};check(touch.valid()&&!touch.empathy_effect&&touch.effect_range()==144);
 check(!Reviewed::empathy_spell({155,0},0,"wrong family"));std::cout<<"PASS typed NPC Empathy distinct from immediate damage, legacy preserved\n";
}
