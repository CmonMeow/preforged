#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){
 if(!x)throw std::runtime_error("Gale line "+std::to_string(at.line()));
}
int main()try{
 for(bool npc:{false,true})for(unsigned rank:{0u,4u,5u,20u})for(double roll:{.499,.5}){
  warrior_test::Fixture f(322,npc);const bool success=rank>4||roll>=.5;f.box->drain();
  check(f.world.gale(f.source,f.target,rank,roll,1000));
  check(f.health()==1000&&f.world.knocked_down(f.target,1000)==success);
  const auto packets=f.box->drain();const auto body=std::find(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(f.target.actor,f.source.actor,162));
  const auto down=std::find(packets.begin(),packets.end(),combat::knockdown_2022(f.target.actor,2.f));
  if(success)check(body!=packets.end()&&down!=packets.end()&&body<down);else check(body==packets.end()&&down==packets.end());
  if(success){check(f.world.gale(f.source,f.target,rank,roll,1500));check(f.world.knocked_down(f.target,2999)&&!f.world.knocked_down(f.target,3000));}
 }
 for(unsigned mode=0;mode<3;++mode){
  warrior_test::Fixture f(322,true,1000,mode==2?std::optional<bool>{}:std::optional<bool>{false});
  auto stale=f.target;++stale.generation;check(!f.world.gale(f.source,stale,12,{},1000));
  check(f.world.gale(f.source,f.target,12,{},1000)==(mode!=2));check(!f.world.knocked_down(f.target,1000));
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){
  using namespace skills;warrior_test::Fixture f(322,npc);
  Slots bar{};bar[0]={162,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[162]=1;
  attributes::Ranks ranks{};ranks[8]=mode==1?4:5;
  casting::Hooks h;unsigned commits=0,paid=0;std::string phase;
  h.resources=[&](unsigned id){return f.world.resources(id);};h.events=[](const auto&){};
  h.consume_overcast_energy=[&](unsigned skill,unsigned cost,auto event,auto at){++paid;check(cost==10);return f.world.activate_overcast_energy(f.source,skill,cost,event,at);};
  h.target_scope=[&](unsigned id){return f.world.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float){return id==f.target.actor;};
  h.skill_roll=[](){return std::optional<double>{.4};};
  h.gale=[&](auto target,unsigned rank,auto roll,auto at){++commits;return f.world.gale(f.source,target,rank,roll,at);};
  h.diagnostic=[&](unsigned,const char* value){phase=value;};
  if(mode==2)h.gale={};
  casting::Runtime cast(1);const auto admitted=cast.start(1000,{162,0,f.target.actor},bar,policy,ranks,true,h);
  check(admitted==(mode==2?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==2){check(!paid);continue;}
  check(paid==1&&f.world.overcast_energy_snapshot(f.source));
  if(mode==3)cast.cancel(h);if(mode==4)cast.interrupt(1200,h);
  if(mode==5){auto changed=f.target;++changed.generation;h.target_scope=[&,changed](unsigned id)->std::optional<effects::Scope>{return id==changed.actor?std::optional{changed}:f.world.effect_scope(id);};}
  cast.update(1999,h);check(!commits);cast.update(2000,h);check(!cast.active());
  check(commits==unsigned(mode<2));check(f.world.knocked_down(f.target,2000)==(mode==0));check(f.health()==1000);
  check(f.world.overcast_energy_snapshot(f.source)->curve.burden_units()>0);
 }
 std::cout<<"PASS Gale player/NPC2s control,low-Air failure threshold,immune/unknown/stale targets and atomic Overcast cast lifecycle\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
