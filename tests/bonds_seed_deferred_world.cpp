// Detached: build at the next coordinated source save point.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Deferred Bonds Seed line "+std::to_string(at.line()));
}
int main()try{
 for(int reversal:{-1,0,20}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.level=20;
  n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
  d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={1000,1000};d.damageable=true;
  d.lightning_armor=60;d.lightning_armor_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
  auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u,4u}){
   w.join(id,"Actor",id==1?0:id==2?700:id==3?800:750,0,box,0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{500,1000,100,100},100);
  }
  const auto source=*w.effect_scope(1),npc=*w.effect_scope(100),a=*w.effect_scope(2),b=*w.effect_scope(3),monk=*w.effect_scope(4);
  for(auto target:{a,b}){
   check(w.elemental_armor(target,60,60));
   check(w.stage_npc_bonds_of_torment(npc,target,*effects::bonds_of_torment_npc(),100));
   check(w.complete_healing_seed(monk,target,*effects::healing_seed(12),0,100));
  }
  if(reversal>=0)check(w.status_mutation(a,307,900,reversal,100,8100,true));
  box->drain();std::vector<world::Packet> completion;
  check(w.lightning(source,npc,*effects::lightning(228,12,0),20,200,&completion));
  const double raw=1000-w.npc_combat_snapshot(npc)->health->current;
  const double converted=reversal<0?0:std::min(raw,double(effects::reversal_fortune(reversal)->maximum_healing));
  const unsigned count=(raw>converted)+(raw>0),heal=count*effects::healing_seed(12)->healing;
  check(std::abs(double(w.resources(2)->health)-(500+converted-(raw-converted)+heal))<1.01);
  check(std::abs(double(w.resources(3)->health)-(500-raw+heal))<1.01);
  check(w.resources(4)->health==500+heal);
  const auto loss=[](const world::Packet& packet){
   if(packet.size()!=18||packet[0]!=162||packet[1]!=0||packet[2]!=16)return false;
   const auto target=unsigned(packet[6])|(unsigned(packet[7])<<8)|(unsigned(packet[8])<<16)|(unsigned(packet[9])<<24);
   return (target==2||target==3)&&std::bit_cast<float>(std::uint32_t(packet[14])|(std::uint32_t(packet[15])<<8)|
       (std::uint32_t(packet[16])<<16)|(std::uint32_t(packet[17])<<24))<0;
  };
  check(std::count_if(completion.begin(),completion.end(),loss)==int((raw>converted)+(raw>0)));
  const auto immediate=box->drain();check(std::none_of(immediate.begin(),immediate.end(),loss));
 }
 std::cout<<"3 deferred Bonds Seed cases: sequential recipient healing and deferred cold modifiers\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
