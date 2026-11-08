#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Reversal Divine Favor death line "+std::to_string(at.line()));
}
unsigned u32(const world::Packet& p,unsigned at){unsigned n=0;for(unsigned i=0;i<4;++i)n|=unsigned(p.at(at+i))<<(8*i);return n;}
int main()try{
 unsigned cases=0;
 for(auto relation:{combat::npcs::Relation::allied_to_players,combat::npcs::Relation::protected_ally})
 for(unsigned rank:{0u,12u,20u})for(unsigned divine:{0u,1u,12u,20u})
 for(unsigned hp:{10u,100u,500u})for(bool wound:{false,true})for(bool heals:{false,true}){
  if(relation==combat::npcs::Relation::protected_ally&&wound)continue; // Protected definitions reject combat conditions.
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding={146,100,100,0,0,0,0,0};d.relation=relation;
  d.spawn_health={double(hp),1000};d.damageable=d.conditions_allowed=relation!=combat::npcs::Relation::protected_ally;
  d.healing_allowed=heals;d.fleshy=true;d.healing_evidence=d.fleshy_evidence="synthetic Divine Favor recipient";
  n.combat_definition=d;w.configure_npcs({n});
  auto box=std::make_shared<world::Outbox>();
  w.join(1,"Monk",0,0,box,0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);
  w.set_vitals(1,{1000,1000,100,100},100);const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
  if(wound)check(w.apply_condition(source,target,combat::conditions::Kind::deep_wound,100,10000));
  const auto before=w.npc_combat_snapshot(target)->health->current;
  box->drain();
  if(!heals){
   // Existing staged admission requires affirmative healing eligibility.
   // Keep that conservative boundary explicit rather than inventing immune
   // recipient conversion semantics while validating this death path.
   check(!w.complete_reversal_fortune(source,target,*effects::reversal_fortune(rank),divine,100));
   check(w.npc_combat_snapshot(target)->health->current==before&&box->drain().empty());
   check(w.status_snapshot(target,100)->empty());++cases;continue;
  }
  check(w.complete_reversal_fortune(source,target,*effects::reversal_fortune(rank),divine,100));
  const bool death=wound&&hp==10&&heals&&divine>0;
  const auto state=w.npc_combat_snapshot(target);check(state&&bool(state->health->current<=0)==death);
  if(!divine||!heals)check(state->health->current==before);
  const auto status=*w.status_snapshot(target,100);
  check(std::count_if(status.begin(),status.end(),[](const auto& e){return e.skill==307;})==unsigned(!death));
  check(w.take_npc_deaths(source).empty()); // Healing is not a damaging kill.
  check(w.resources(1)->health==1000);
  const auto packets=box->drain();unsigned adds=0,ends=0,damages=0;
  for(const auto& p:packets){
   if(p.size()>=10&&p[0]==65&&u32(p,2)==100)++adds;
   if(p.size()==10&&p[0]==67&&u32(p,2)==100)++ends;
   if(p.size()==18&&p[0]==162&&u32(p,6)==100&&u32(p,2)!=55)++damages;
  }
  if(adds!=1||ends!=unsigned(death)||damages)std::cerr<<"rank="<<rank<<" divine="<<divine<<" hp="<<hp<<" wound="<<wound<<" adds="<<adds<<" ends="<<ends<<" death="<<death<<" damage="<<damages<<'\n';
  check(adds==1&&ends==unsigned(death)&&damages==0);
  if(death){
   check(!w.complete_reversal_fortune(source,target,*effects::reversal_fortune(rank),divine,100));
   w.tick(200);check(w.take_npc_deaths(source).empty());
   const auto later=box->drain();check(std::none_of(later.begin(),later.end(),[](const auto& p){return p.size()==10&&p[0]==67&&u32(p,2)==100;}));
  }
  ++cases;
 }
 std::cout<<cases<<" Reversal Divine Favor NPC healing-death, immediate effect retirement, immunity and living controls passed; paid307 unbound\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
