#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Mark of Pain NPC regression line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<3;++mode){const bool aoe=mode==1;
  world::Instance w;const std::string hash(64,'a');
  std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 60 0 0 - -\nnpc 146 101 2 1235 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
  auto pop=world::load_npc_catalog(appearance);
  const auto row=[&](unsigned id,unsigned model,unsigned health){return "\nnpc 146 "+std::to_string(id)+" "+std::to_string(id-99)+" "+std::to_string(model)+" 0 0 8 1 hostile-to-players - 500 "+std::to_string(health)+" 60 1 1 test test test - test 60 60 test test 60 test\n";};
  auto rows=row(100,1234,aoe?500:20)+row(101,1235,mode==2?30:500);
  std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/mark_of_pain_npc.cpp"+rows);
  const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(pop,catalog,hash);w.configure_npcs(pop.for_map(146));
  for(unsigned id=1;id<=3;++id){w.join(id,"Actor",id==2?100.f:0.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);w.set_vitals(id,{500,500,100,100},100);}
  const auto caster=*w.effect_scope(1),attacker=*w.effect_scope(3),first=*w.effect_scope(100),second=*w.effect_scope(101);
  check(w.physical_armor(*w.effect_scope(2),60));
  check(w.degeneration_hex(caster,first,*skills::effects::degeneration_hex(150,12),100));
  if(aoe)check(w.degeneration_hex(caster,second,*skills::effects::degeneration_hex(150,12),100));
  combat::weapon::Owned weapon{3,1,{20,20,2},12,false,false,aoe?2u:27u};check(w.ordinary_weapon(attacker,weapon,20));
  check(w.melee_attack(attacker,aoe?attacker:first,weapon,*combat::weapon::attack_skill(aoe?330:322,aoe?12:0),200));
  check(w.npc_combat_snapshot(first)->health->current==(aoe?436:0));
  check(w.npc_combat_snapshot(second)->health->current==(aoe?436:mode==2?0:466));
  check(w.resources(2)->health==(aoe?402u:466u));
  const auto deaths=w.take_npc_deaths(attacker);check(deaths.size()==(aoe?0u:1u));if(!aoe)check(deaths[0].target==first);
  const auto credited=w.take_npc_deaths(caster);check(credited.size()==(mode==2?1u:0u));if(mode==2)check(credited[0].target==second);
  w.tick(300);check(w.npc_combat_snapshot(second)->health->current==(aoe?436:mode==2?0:466));check(w.take_npc_deaths(caster).empty());
 }
 std::cout<<"PASS Mark of Pain lethal NPC primary splash and mixed Cyclone reactions without staged Health overwrite\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
