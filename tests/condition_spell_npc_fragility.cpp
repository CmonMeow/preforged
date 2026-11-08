#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Condition NPC Fragility line "+std::to_string(at.line()));
}
unsigned word(const world::Packet& p,unsigned at){return unsigned(p[at])|(unsigned(p[at+1])<<8)|(unsigned(p[at+2])<<16)|(unsigned(p[at+3])<<24);}
unsigned hits(const std::vector<world::Packet>& packets){return unsigned(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=18&&p[0]==162&&p[1]==0&&word(p,2)==55;}));}
int main()try{
 for(unsigned mode=0;mode<3;++mode){
  world::Instance w;std::vector<world::Instance::Npc> npcs;
  for(unsigned id:{100u,101u}){
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?100:150;n.definition.level=1;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=1;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={mode==1?1.:1000.,1000};d.damageable=true;
   d.conditions_allowed=!(mode==2&&id==101);n.combat_definition=d;npcs.push_back(n);
  }
  w.configure_npcs(npcs);auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),a=*w.effect_scope(100),b=*w.effect_scope(101);
  for(auto target:{a,b})check(w.degeneration_hex(source,target,*effects::degeneration_hex(2998,12),100,20));
  box->drain();const auto accepted=w.condition_spell(source,a,*effects::condition_spell(118,12),200);
  const auto packets=box->drain();
  if(mode==2){check(!accepted&&hits(packets)==0);for(auto target:{a,b})check(w.npc_combat_snapshot(target)->health->current==1000);continue;}
  check(accepted);const auto loss=effects::fragility(2998,12)->damage;
  for(auto target:{a,b})check(w.npc_combat_snapshot(target)->health->current==(mode==1?0.:1000.-loss));
  check(hits(packets)==2); // Publish both immediate reactions at the cast boundary.
  w.tick(200);check(hits(box->drain())==0); // A later refresh cannot replay them.
 }
 std::cout<<"NPC condition spell Fragility immediate publication, lethal recipients and cohort refusal pass\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
