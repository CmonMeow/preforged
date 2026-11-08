#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <fstream>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Reversal caster ownership line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 effects::Scope attacker,target,healer;
 Fixture(unsigned hp,unsigned state){
  std::vector<world::Instance::Npc> actors;
  for(unsigned id:{100u,101u}){
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding={146,id,id,0,0,0,0,20};d.spawn_health={id==100?double(hp):1000.,1000};
   d.damageable=d.conditions_allowed=true;d.relation=combat::npcs::Relation::hostile_to_players;
   d.healing_allowed=d.fleshy=true;d.healing_evidence=d.fleshy_evidence="synthetic living recipient";d.earth_armor=60;
   n.combat_definition=d;actors.push_back(n);
  }
  w.configure_npcs(actors);w.join(1,"Attacker",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Healer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{1000,1000,100,100},100);
  attacker=*w.effect_scope(1);target=*w.effect_scope(100);healer=*w.effect_scope(state==0?100:state>=5?101:2);
  check(w.configure_projectiles({1000,32}));box->drain();
 }
};
int main(int argc,char** argv)try{
 std::ofstream trace;if(argc==2){trace.open(argv[1]);check(bool(trace));}else check(argc==1);
 unsigned cases=0;
 for(unsigned state=0;state<7;++state)for(unsigned rank:{0u,12u,20u})for(unsigned hp:{1u,500u,1000u})for(unsigned route=0;route<4;++route){
  Fixture f(hp,state);check(f.w.stage_npc_reversal_from(f.healer,f.target,rank,100));
  const auto status=*f.w.status_snapshot(f.target,100);
  check(status.size()==1&&status[0].cause==f.healer.actor);
  if(state==2)check(f.w.kill(f.healer,150));
  if(state==3||state==4)f.w.leave(2);
  if(state==4){f.w.join(2,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},150,1);f.w.set_vitals(2,{1000,1000,100,100},100);check(*f.w.effect_scope(2)!=f.healer);}
  if(state==6)check(f.w.damage_npc(f.attacker,f.healer,{combat::Kind::armor_ignoring,2000,20,194},1248,150).killed);
  f.w.take_npc_deaths(f.attacker);f.box->drain();double amount=72;
  if(route==0)check(f.w.npc_reversal_damage(f.attacker,f.target,{combat::Kind::armor_ignoring,amount,20,42},200));
  if(route==1)check(f.w.npc_reversal_life_steal_recipient(f.attacker,f.target,amount,200));
  if(route==2){const auto plan=*effects::caster_earth_aoe(174,12);check(plan.count==1);amount=plan.strikes[0].earth_damage;check(f.w.shockwave(f.attacker,plan,20,200));}
  if(route==3){const auto plan=*effects::disrupting_dagger(12);amount=plan.earth_damage;check(f.w.launch_disrupting_dagger(f.attacker,f.target,plan,20,200));f.w.tick(300);}
  const double converted=std::min(amount,double(effects::reversal_fortune(rank)->maximum_healing));
  const double healed=std::min(1000.,hp+converted),expected=std::max(0.,healed-(amount-converted));
  if(f.w.npc_combat_snapshot(f.target)->health->current!=expected)std::cerr<<"state="<<state<<" rank="<<rank<<" hp="<<hp<<" route="<<route<<'\n';
  check(f.w.npc_combat_snapshot(f.target)->health->current==expected);
  const auto packets=f.box->drain();
  if(trace&&rank==12&&hp==500){
   trace<<"{\"state\":"<<state<<",\"route\":"<<route<<",\"healer\":"<<f.healer.actor
        <<",\"gained\":"<<healed-hp<<",\"final_health\":"<<expected<<",\"packets\":[";
   bool first=true;for(const auto& packet:packets){if(!first)trace<<',';first=false;trace<<'\"';
    constexpr char hex[]="0123456789abcdef";for(const auto byte:packet)trace<<hex[byte>>4]<<hex[byte&15];trace<<'\"';}
   trace<<"]}\n";
  }
  const auto heal=skills::particles::health_change_2022(f.target.actor,f.healer.actor,healed-hp,1000);
  check(std::count(packets.begin(),packets.end(),heal)==unsigned(healed>hp&&state!=3&&state!=4));
  check(f.w.take_npc_deaths(f.attacker).size()==unsigned(expected<=0));
  const auto after=*f.w.status_snapshot(f.target,300);
  check(std::none_of(after.begin(),after.end(),[](const auto& e){return e.skill==307;}));
  ++cases;
 }
 // Replacement changes credit atomically; stale/dead sources cannot apply it.
 for(unsigned state:{1u,5u}){
  Fixture f(500,state);auto stale=f.healer;++stale.generation;
  check(!f.w.stage_npc_reversal_from(stale,f.target,12,100));
  check(f.w.stage_npc_reversal_from(f.healer,f.target,12,100));
  check(f.w.stage_npc_reversal(f.target,12,150));f.box->drain();
  check(f.w.npc_reversal_damage(f.attacker,f.target,{combat::Kind::armor_ignoring,72,20,42},200));
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),skills::particles::health_change_2022(100,100,67,1000))==1);++cases;
 }
 std::cout<<cases<<" external/self NPC Reversal caster cases: direct, life-steal, elemental and delayed impact; death, departure, reused IDs, stale source and replacement passed\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
