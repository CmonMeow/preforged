#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Watchful NPC World line "+std::to_string(at.line()));
}
int main()try{
 const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 80 0 0 - -\n");
 auto population=world::load_npc_catalog(appearance);
 std::istringstream metadata("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/watchful_healing_npc_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 100 - 1 0 test test - - - - - - -\n");
 world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
 for(unsigned mode=0;mode<10;++mode){
  auto defs=population.for_map(146);auto& d=*defs.front().combat_definition;
  d.relation=mode==1?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
  d.healing_allowed=true;d.healing_evidence="authored fixture";
  if(mode==2)defs.front().x=2000;
  if(mode==3)d.healing_allowed.reset();
  if(mode==4)d.healing_allowed=false;
  world::Instance w;w.configure_npcs(defs);auto box=std::make_shared<world::Outbox>();
  w.join(1,"Monk",0,0,box,0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);
  w.set_vitals(1,{500,500,100,100},100);
  auto source=*w.effect_scope(1),target=*w.effect_scope(100);
  if(mode==5)++target.generation;
  if(mode==6)++source.generation;
  auto plan=*skills::effects::watchful_healing(12);
  if(mode==7)++plan.pips;
  box->drain();std::vector<world::Packet> visuals;std::function<void()> publish;
  check(!w.apply_health_regeneration(source,999,*skills::regeneration::reviewed(1392,12),100,10100,1248,true,target,0));
  const bool accepted=w.stage_watchful_healing(source,target,plan,12,100,&visuals,&publish);
  const bool expected=mode==0||mode==4||mode==8||mode==9;
  check(accepted==expected);
  if(!accepted){check(visuals.empty()&&!publish);check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==100);continue;}
  check(visuals.size()==1&&visuals.front()==*skills::particles::targeted_body_2022(100,1,1392));
  check(box->drain().empty()&&bool(publish));publish();const auto packets=box->drain();publish();check(box->drain().empty());
  check(w.npc_combat_snapshot(target)->health->current==(mode==4?100:138));
  check(std::any_of(packets.begin(),packets.end(),[](const auto& p){return p.size()>2&&p[0]==65;}));
  if(mode!=4)check(std::count(packets.begin(),packets.end(),skills::particles::health_change_2022(100,1,38,500))==1);
  if(mode==8){w.tick(10100);check(w.npc_combat_snapshot(target)->health->current==198);
   const auto expired=box->drain();check(std::count_if(expired.begin(),expired.end(),[](const auto& p){return p.size()==10&&p[0]==67;})==1);
   w.tick(11100);const auto again=box->drain();check(std::none_of(again.begin(),again.end(),[](const auto& p){return p.size()==10&&p[0]==67;}));}
  if(mode==9){check(w.stage_watchful_healing(source,target,plan,0,1100));
   check(w.npc_combat_snapshot(target)->health->current==144);w.tick(11100);check(w.npc_combat_snapshot(target)->health->current==204);}
 }
 std::cout<<"PASS staged Watchful NPC World scope,allegiance,range,healing policy,publication,expiry,renewal\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
