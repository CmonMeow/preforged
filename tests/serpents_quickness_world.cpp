#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Serpents Quickness World line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;Scope owner,enemy;
 Fixture(unsigned health=500){for(unsigned id:{1u,2u}){
   w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{id==1?health:500u,500,50,50},100);
  }owner=*w.effect_scope(1);enemy=*w.effect_scope(2);
  check(w.energy_capacity(1,2,0,100,{30,-2}));
 }
 void apply(unsigned rank=12,std::uint64_t now=100){check(w.status_mutation(owner,456,123,rank,now,now+serpents_quickness(rank)->duration,true));}
 bool active(std::uint64_t at=100){const auto entries=w.status_snapshot(owner,at);return entries&&std::any_of(entries->begin(),entries->end(),[](const auto& e){return e.skill==456;});}
};
int main()try{
 {Fixture f(250);f.apply();check(f.active());check(f.w.shadow_damage(f.enemy,f.owner,*shadow_damage(133,0),20,100));check(!f.active());check(f.w.heal_target(1,1,100,1248,true,100));check(!f.active());}
 {Fixture f(249);f.apply();check(!f.active());}
 {Fixture f(400);f.apply();check(bool(f.w.health_sacrifice(f.owner,119,100)));check(!f.active());}
 {Fixture f(260);f.apply();check(f.w.life_steal(f.enemy,f.owner,*life_steal(153,0),100));check(!f.active());}
 {Fixture f(255);f.apply();check(f.w.apply_condition(f.enemy,f.owner,combat::conditions::Kind::poison,100,1100));f.w.tick(1100);check(!f.active(1100));check(f.w.heal_target(1,1,200,1248,true,1100));check(!f.active(1100));}
 {Fixture f(400);f.apply();check(f.w.health_capacity(f.owner,100,100));check(!f.active());}
 {Fixture f(260);f.apply();check(f.w.apply_condition(f.enemy,f.owner,combat::conditions::Kind::deep_wound,100,1100));check(!f.active());f.w.tick(1100);check(!f.active(1100));}
 {Fixture f(400);f.w.set_vitals(2,{50,500,50,50},100);f.apply();check(f.w.grenths_balance(f.owner,f.enemy,100));check(f.w.resources(1)->health==225&&!f.active());}
 {Fixture f;f.apply();f.w.set_vitals(1,{249,500,50,50},100);check(!f.active());}
 {Fixture f;f.apply();check(f.w.status_mutation(f.owner,455,124,0,100,8100,true));check(!f.active());}
 {Fixture f;f.apply(0);check(f.active(15099)&&!f.active(15100));}
 {Fixture f;auto bad=f.owner;++bad.generation;check(!f.w.status_mutation(bad,456,123,12,100,27100,true));check(!f.w.status_mutation(f.owner,456,123,12,100,27101,true));check(!f.active());}
 for(bool low:{false,true})for(bool pvp:{false,true}){
  Fixture f(low?200:500);Slots bar{};bar[0]={456,0};bar[1]={155,0};
  check(f.w.configure_adrenaline(f.owner,bar,100));
  Policy policy;policy.primary=2;policy.secondary=4;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[456]=policy.learned[155]=1;
  attributes::Ranks ranks{};ranks[24]=12;
  std::vector<Packet> packets;casting::Hooks h;
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  h.resources=[&](auto id){return f.w.resources(id);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};
  h.target_valid=[](auto id,float,bool self){return self&&id==1;};h.projectile_target_valid=[](auto id,float){return id==2;};
  h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.owner,at);};
  h.effect_snapshot=[&](auto at){return f.w.status_snapshot(f.owner,at);};
  h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto end,bool add){return f.w.status_mutation(f.owner,skill,effect,rank,now,end,add);};
  h.shadow_damage=[&](auto target,const auto& plan,auto now){return f.w.shadow_damage(f.owner,target,plan,20,now);};
  const auto snapshot=h.effect_snapshot;h.effect_snapshot={};casting::Runtime rejected(1);
  check(rejected.start(100,{456,0,0},bar,policy,ranks,true,h)==casting::Reason::unsupported&&f.w.resources(1)->energy==50);h.effect_snapshot=snapshot;
  casting::Runtime cast(1);check(cast.start(100,{456,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);
  check(!cast.active()&&f.w.resources(1)->energy==45&&f.active()==!low);
  check(std::count(packets.begin(),packets.end(),casting::generic(21,1,768))==1);
  check(std::find(packets.begin(),packets.end(),casting::slot_event(228,1,{456,0},45))!=packets.end());
  check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>2&&p[0]==65;})==unsigned(!low));
  check(cast.start(1000,{155,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);
  const auto completes=1000+std::uint64_t(find_skill(155)->activation*1000);cast.update(completes,h);
  check(std::find(packets.begin(),packets.end(),casting::slot_event(228,1,{155,0},low?2:1))!=packets.end());
  check(cast.start(45099,{456,0,0},bar,policy,ranks,true,h)==casting::Reason::recharge);
  check(cast.start(45100,{456,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);
 }
 // The state at completion, not admission, determines the reduction.
 for(unsigned mode=0;mode<4;++mode){
  Fixture f(255);if(mode)f.apply();Slots bar{};bar[0]={155,0};
  check(f.w.configure_adrenaline(f.owner,bar,100));Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[155]=1;
  std::vector<Packet> packets;casting::Hooks h;
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  h.resources=[&](auto id){return f.w.resources(id);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};
  h.target_valid=[](auto,float,bool){return true;};h.projectile_target_valid=[](auto id,float){return id==2;};
  h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.owner,at);};
  h.effect_snapshot=[&](auto at){return f.w.status_snapshot(f.owner,at);};
  h.shadow_damage=[&](auto target,const auto& plan,auto at){return f.w.shadow_damage(f.owner,target,plan,20,at);};
  casting::Runtime cast(1);check(cast.start(1000,{155,0,2},bar,policy,{},true,h)==casting::Reason::accepted);
  if(mode==0)f.apply(12,1100);
  if(mode==1)check(f.w.shadow_damage(f.enemy,f.owner,*shadow_damage(133,0),20,1200));
  if(mode==2)cast.interrupt(1200,h);
  if(mode==3)cast.cancel(h,1200);
  cast.update(1750,h);
  const auto reduced=casting::slot_event(228,1,{155,0},1),normal=casting::slot_event(228,1,{155,0},2);
  check(std::count(packets.begin(),packets.end(),reduced)==unsigned(mode==0));
  check(std::count(packets.begin(),packets.end(),normal)==unsigned(mode==1||mode==2));
  check(f.w.resources(2)->health==(mode<2?480u:500u));
 }
 std::cout<<"PASS Serpents Quickness real World damage/sacrifice/life steal/Poison/capacity/DeepWound/Balance/vitals ends,expiry/replacement,low Health,no resurrection and Runtime PvE/PvP payment/body/recharge,midcast changes/cancel/interrupt\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
