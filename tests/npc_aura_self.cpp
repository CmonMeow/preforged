#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC Aura self line "+std::to_string(at.line()));
}
int main()try{
    using namespace combat::ai;
    const skills::effects::Scope owner{1,100,1},foe{1,1,1};
    const auto plan=skills::effects::aura_restoration_plan(0);check(plan&&plan->duration==60000);
    check(!skills::effects::aura_restoration_plan(21));
    Spellbook book({{{180,0},0,"Brood wiki Storage0 self Aura",SpellTargetClass::self},
        {{194,0},1248,"reviewed offensive spell"}},owner);
    const SpellTarget self{owner,0,true,false,true,true},enemy{foe,100,true,true,true,true};
    check(book.choose(self,5,1000)==std::optional(skills::Slot{180,0}));
    check(book.choose(enemy,5,1000)==std::optional(skills::Slot{194,0}));
    auto fake=self;fake.scope=foe;check(!book.choose(fake,5,1000));
    fake=self;fake.hostile=true;check(!book.eligible({180,0},fake,5,1000));
    fake=self;fake.distance=1;check(!book.eligible({180,0},fake,5,1000));
    check(!book.choose(self,4,1000));
    const combat::npcs::control::Token token{owner,0,1,combat::npcs::control::Action::spell,1000,1250,skills::Slot{180,0}};
    check(book.begin(token));book.finish(1250,true,true);
    check(!book.choose(enemy,5,1999));check(book.choose(enemy,5,2000).has_value());
    check(!book.choose(self,5,21249));check(book.choose(self,5,21250).has_value());
    const auto* spell=skills::find_skill(194);check(spell!=nullptr);
    check(skills::effects::aura_restoration(*spell,0)->healing==spell->energy_cost()*2);
    check(skills::effects::aura_restoration(*spell,0)->energy==0);
    check(skills::effects::aura_restoration(*spell,8)->energy==1);
    combat::npcs::Definition definition;definition.binding.map=161;
    definition.binding.actor=definition.binding.definition=100;definition.binding.level=4;
    definition.spawn_health=combat::Health{100,100};definition.healing_allowed=true;
    definition.healing_evidence="synthetic owned Aura registry fixture";
    combat::npcs::LiveActor actor(definition,{1,1,1,161,100});const auto life=actor.lifetime();
    check(actor.apply_aura_restoration(life,*plan,7,1000));
    check(actor.enchantment_count(life,1000)==1);check(actor.newest_enchantment(life,1000)->instance==7);
    check((actor.actor(life)->effects&0x80)!=0);
    check(actor.remove_reviewed_enchantment(life,7,1100));
    check(actor.enchantment_count(life,1100)==0&&!actor.newest_enchantment(life,1100));
    check(!actor.aura_restoration_entry(life,1100)&&(actor.actor(life)->effects&0x80)==0);
    check(!actor.remove_reviewed_enchantment(life,7,1100));
    std::cout<<"NPC Aura explicit self admission and shared healing formula passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
