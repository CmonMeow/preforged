#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("You Will Die line "+std::to_string(at.line()));}
unsigned charge(world::Instance& w,Scope owner){const auto p=w.adrenaline(owner);check(p.has_value());for(const auto& pool:*p)if(pool.slot.skill==340)return pool.units;throw std::runtime_error("missing pool");}
void knockdown_source(warrior_test::Fixture& f){
 f.world.join(3,"Knockdown cause",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
 f.world.set_vitals(3,{1000,1000,30,30},100);
 check(f.world.gale(*f.world.effect_scope(3),f.source,12,{},100));
 check(f.world.knocked_down(f.source,100));
}
int main()try{
 constexpr unsigned wiki_strikes[]{1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,4,4};
 for(unsigned rank=0;rank<=20;++rank){const auto p=resource_gain(1141,rank);check(p&&p->adrenaline==wiki_strikes[rank]&&p->health==0&&p->energy==0&&gain_is_shout(*p));}check(!resource_gain(1141,21));check(timing::instant_action(*find_skill(1141)));
 for(bool npc:{false,true})for(unsigned health:{499u,500u,501u})for(unsigned mode=0;mode<5;++mode){
  warrior_test::Fixture f(340,npc,health);auto& w=f.world;Slots bar{};bar[0]={340,0};check(w.configure_adrenaline(f.source,bar,100));
  if(mode==1||mode==2)check(w.for_great_justice(f.source,*for_great_justice(343,mode==2),100));
  if(mode==2)check(w.status_mutation(f.source,370,124,0,100,5100,true));
  if(mode==3)check(w.status_mutation(f.source,56,123,0,100,8100,true));
  if(mode==4)knockdown_source(f);
  auto forged=*resource_gain(1141,12);++forged.adrenaline;check(!w.resource_gain(f.source,f.target,forged,100));
  auto stale=f.target;++stale.generation;check(!w.resource_gain(f.source,stale,*resource_gain(1141,12),100));
  f.box->drain();const auto before=w.resources(1)->energy;const auto result=w.resource_gain(f.source,f.target,*resource_gain(1141,12),100);
  check(result&&result->condition_met==(health<500)&&!result->energy_gained&&!result->health_delta);
  const auto expected=health>=500||mode==3?0u:mode==1?150u:mode==2?112u:75u;
  check(charge(w,f.source)==expected&&w.resources(1)->energy==before);
  const auto pools=*w.adrenaline(f.source);check(pools[0].half==unsigned(health<500&&mode==2));
  const auto published=f.box->drain();check(std::count(published.begin(),published.end(),combat::adrenaline::snapshot_2022(1,pools[0]))==unsigned(expected!=0));
  for(unsigned i=0;i<10;++i)check(w.resource_gain(f.source,f.target,*resource_gain(1141,20),100).has_value());
  if(health<500&&mode!=3)check(charge(w,f.source)==find_skill(340)->adrenaline);
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<5;++mode){
  warrior_test::Fixture f(340,npc,499);auto& w=f.world;Slots bar{};bar[0]={1141,0};bar[1]={340,0};check(w.configure_adrenaline(f.source,bar,100));
  casting::Runtime cast(1);casting::Hooks h;std::vector<Packet> wire;unsigned validations=0;
  h.events=[&](const auto& packets){wire.insert(wire.end(),packets.begin(),packets.end());};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto cost,auto now){return w.consume_energy(1,cost,f.source,now);};h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.shout_target_valid=[&](auto id,float range){if(mode==4&&++validations==2)h.resource_gain={};return w.shout_target_valid(f.source,id,range,100);};
  h.resource_gain=[&](auto target,const auto& plan,auto now){return w.resource_gain(f.source,target,plan,now);};
  Policy p;p.primary=1;p.campaigns=255;p.learned[1141]=p.learned[340]=1;attributes::Ranks ranks{};ranks[17]=12;
  if(mode==1)h.resource_gain={};if(mode==2)h.shout_target_valid={};if(mode==3)knockdown_source(f);
  const auto reason=cast.start(100,{1141,0,f.target.actor},bar,p,ranks,true,h);
  if(mode==1||mode==2){check(reason==casting::Reason::unsupported&&w.resources(1)->energy==30);continue;}
  if(reason!=casting::Reason::accepted||cast.active()||w.resources(1)->energy!=25)
   throw std::runtime_error("You Will Die Runtime npc="+std::to_string(npc)+" mode="+std::to_string(mode)+" reason="+std::to_string(unsigned(reason))+" active="+std::to_string(cast.active())+" energy="+std::to_string(w.resources(1)->energy));
  check(charge(w,f.source)==(mode==4?0u:75u));
  check(std::count(wire.begin(),wire.end(),casting::generic(48,1,1141))==1);
  check(std::count(wire.begin(),wire.end(),combat::detail::integer(21,1,1033))==(mode==4?0:1));
  check(std::count(wire.begin(),wire.end(),casting::slot_event(228,1,{1141,0},15))==(mode==4?0:1));
  check(cast.start(101,{1141,0,f.target.actor},bar,p,ranks,true,h)==
        (mode==4?casting::Reason::unsupported:casting::Reason::recharge));
  check(w.resources(1)->energy==25);
 }
 {warrior_test::Fixture f(340,false,499);auto& w=f.world;casting::Runtime cast(1);casting::Hooks h;
  h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.consume_energy_at=[&](auto cost,auto now){return w.consume_energy(1,cost,f.source,now);};h.target_valid=[](auto id,float,bool){return id==1;};h.heal=[](auto,auto,float,bool){return true;};
  h.shout_target_valid=[&](auto id,float range){return w.shout_target_valid(f.source,id,range,200);};h.resource_gain=[&](auto target,const auto& plan,auto now){return w.resource_gain(f.source,target,plan,now);};
  Policy p;p.primary=1;p.secondary=3;p.campaigns=255;p.learned[281]=p.learned[1141]=p.learned[340]=1;Slots bar{};bar[0]={1141,0};bar[1]={281,0};bar[2]={340,0};check(w.configure_adrenaline(f.source,bar,100));attributes::Ranks ranks{};ranks[17]=12;
  check(cast.start(100,{281,0,1},bar,p,ranks,true,h)==casting::Reason::accepted&&cast.active());
  check(cast.start(200,{1141,0,2},bar,p,ranks,true,h)==casting::Reason::accepted&&cast.active());check(charge(w,f.source)==75);
  cast.update(1100,h);check(!cast.active());
 }
 {warrior_test::Fixture f(340,true,499);auto& w=f.world;Slots bar{};bar[0]={340,0};check(w.configure_adrenaline(f.source,bar,100));
  w.join(3,"Cause",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(3,{1000,1000,30,30},100);const auto cause=*w.effect_scope(3);
  check(w.degeneration_hex(cause,f.target,*degeneration_hex(31,0),3000));const auto energy=w.resources(1)->energy;
  check(!w.resource_gain(f.source,f.target,*resource_gain(1141,12),2000));check(w.resources(1)->energy==energy&&charge(w,f.source)==0);
  check(w.settle_resources(f.source,1000)); // rejected future target did not advance the caster clock
 }
 {warrior_test::Fixture f(340,false,499);auto& w=f.world;Slots bar{};bar[0]={340,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.settle_resources(f.target,3000));const auto before=w.resources(1)->energy;
  check(!w.resource_gain(f.source,f.target,*resource_gain(1141,12),2000));check(w.resources(1)->energy==before&&charge(w,f.source)==0);
  check(w.settle_resources(f.source,1000));
 }
 std::cout<<"PASS You Will Die strict threshold/playerNPC/shared adrenaline modifiers/cap/forbidden/knockdown/instant/missing and lost hooks/future target\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
