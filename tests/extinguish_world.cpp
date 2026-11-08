#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Extinguish World line "+std::to_string(at.line()));}
int main()try{using namespace skills::effects;using combat::conditions::Kind;
 for(bool covered:{false,true})for(unsigned divine:{0u,12u}){
  world::Instance w;for(unsigned id:{1u,2u,3u,4u}){w.join(id,"Actor",id==3?5021:float(id*20),0,std::make_shared<world::Outbox>(),0,id==1?std::vector<world::Packet>{{165,0,1,0,0,0,3,0}}:std::vector<world::Packet>{}, {},100,1);w.set_vitals(id,{100,500,30,30},100);}
  for(unsigned id:{2u,3u})check(w.party_action(1,158,id,100)&&w.party_action(id,154,1,100));
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2),far=*w.effect_scope(3),outsider=*w.effect_scope(4);
  for(auto actor:{target,far,outsider})check(w.apply_condition(owner,actor,Kind::burning,100,10000));
  if(covered)check(w.apply_condition(owner,target,Kind::weakness,100,10000));
  const auto plan=*extinguish(12,divine);check(w.extinguish(owner,plan,100));
  check(w.resources(1)->health==100+plan.caster_heal&&w.resources(2)->health==(covered?100u:182u));
  check(w.has_condition(target,Kind::burning,100)==covered&&!w.has_condition(target,Kind::weakness,100));
  check(w.has_condition(far,Kind::burning,100)&&w.has_condition(outsider,Kind::burning,100));
  auto stale=owner;++stale.generation;check(!w.extinguish(stale,plan,100));auto forged=plan;++forged.burning_heal;check(!w.extinguish(owner,forged,100));
 }
 for(unsigned health:{16u,18u,100u}){
  world::Instance w;w.join(1,"Monk",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Target",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(3,"Mesmer",120,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  for(unsigned id:{1u,2u,3u})w.set_vitals(id,{id==2?health:500u,500,30,30},100);check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2),foe=*w.effect_scope(3);
  check(w.apply_condition(foe,target,Kind::burning,100,10000)&&w.degeneration_hex(foe,target,*degeneration_hex(19,12),100));
  const auto epoch=*w.control_epoch(target);check(w.extinguish(owner,*extinguish(12,0),100));
  check(w.resources(2)->health==(health<=17?0u:health-17+82));check(*w.control_epoch(target)-epoch==unsigned(health<=17));
  w.tick(200);check(w.resources(2)->health==(health<=17?0u:health-17+82));
 }
 // Accepted area effects keep their frozen party recipients when the
 // caster dies to its own removal reaction. This is server ordering policy.
 for(unsigned caster_id:{1u,2u})for(bool caster_leads:{false,true}){
  world::Instance w;const auto ally_id=caster_id==1?2u:1u;
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*20),0,std::make_shared<world::Outbox>(),0,id==caster_id?std::vector<world::Packet>{{165,0,static_cast<unsigned char>(id),0,0,0,3,0}}:std::vector<world::Packet>{}, {},100,id==3?2:1);w.set_vitals(id,{id==caster_id?16u:100u,500,30,30},100);}
  const auto owner=*w.effect_scope(caster_id),ally=*w.effect_scope(ally_id),foe=*w.effect_scope(3);
  const auto leader=caster_leads?caster_id:ally_id,member=caster_leads?ally_id:caster_id;
  check(w.party_action(leader,158,member,100)&&w.party_action(member,154,leader,100));
  // Fragility reaches adjacent foes: apply both conditions before either hex,
  // so this fixture isolates removal damage rather than application damage.
  for(auto actor:{owner,ally})check(w.apply_condition(foe,actor,Kind::burning,100,10000));
  for(auto actor:{owner,ally})check(w.degeneration_hex(foe,actor,*degeneration_hex(19,12),100));
  const auto epoch=*w.control_epoch(owner);check(w.extinguish(owner,*extinguish(12,12),100));
  check(w.resources(caster_id)->health==0&&*w.control_epoch(owner)-epoch==1&&w.resources(ally_id)->health==165);
  check(!w.has_condition(ally,Kind::burning,100));w.tick(200);check(w.resources(caster_id)->health==0&&w.resources(ally_id)->health==165);
 }
 for(bool dense:{false,true})for(auto at:{1099u,1100u}){
  world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",float(id*20),0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{id==1?14u:100u,500,30,30},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto owner=*w.effect_scope(1),ally=*w.effect_scope(2);
  for(auto actor:{owner,ally})check(w.apply_condition(owner,actor,Kind::burning,100,10000));
  if(dense)for(unsigned time=200;time<at;time+=100)w.tick(time);
  check(w.extinguish(owner,*extinguish(12,0),at)==(at==1099));
  if(at==1100){check(w.resources(1)->health==0&&w.has_condition(ally,Kind::burning,at));check(w.control_event_time(owner,0)==1100);}
  else{check(w.resources(1)->health>=82&&w.resources(1)->health<=83&&!w.has_condition(ally,Kind::burning,at));}
 }
 std::cout<<"PASS Extinguish party/range/outsider selection,covered Burning,caster Divine Favor,scoped plan guards and lethal Fragility no-revival;casting pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
