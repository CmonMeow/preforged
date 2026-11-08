#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Victory line "+std::to_string(at.line()));}
int main()try{
 check(resource_gain(365,0)->health==10&&resource_gain(365,12)->health==56&&resource_gain(365,15)->energy==7&&!resource_gain(365,21));
 for(bool npc:{false,true})for(unsigned count=0;count<3;++count){
  warrior_test::Fixture f(340,npc);auto& w=f.world;w.set_vitals(1,{500,1000,10,30},100);
  if(count)check(f.condition(combat::conditions::Kind::blind,100,1000));
  if(count==2)check(f.condition(combat::conditions::Kind::crippled,100,1000));
  auto forged=*resource_gain(365,12);++forged.energy;check(!w.resource_gain(f.source,f.target,forged,100));
  auto stale=f.target;++stale.generation;check(!w.resource_gain(f.source,stale,*resource_gain(365,12),100));
  check(!w.resource_gain(f.source,f.source,*resource_gain(365,12),100));
  f.box->drain();const auto result=w.resource_gain(f.source,f.target,*resource_gain(365,12),100);
  check(result&&result->condition_met==(count!=0)&&result->health_delta==56.*count&&result->energy_gained==6*count);
  check(w.resources(1)->health==500+56*count&&w.resources(1)->energy==10+6*count);
  const auto wire=f.box->drain();check(std::count(wire.begin(),wire.end(),combat::detail::integer(21,1,624))==1);
  const auto expired=w.resource_gain(f.source,f.target,*resource_gain(365,12),1100);check(expired&&!expired->condition_met);
 }
 for(unsigned mode=0;mode<5;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;
  w.set_vitals(1,{mode==1?990u:500u,1000,mode==1?29u:10u,30},100);
  check(f.condition(combat::conditions::Kind::blind,100,10000));check(f.condition(combat::conditions::Kind::crippled,100,10000));
  if(mode==0)check(w.degeneration_hex(f.target,f.source,*degeneration_hex(129,0),100)); // gain is not healing
  if(mode==2)check(w.activate_overcast_energy(f.source,231,0,1,1000));
  if(mode==3){check(w.gale(f.target,f.source,12,{},100));check(w.shout_target_valid(f.source,2,1248,100)&&!w.projectile_target_valid(f.source,2,1248,100));}
  if(mode==4){w.set_vitals(1,{500,1000,0,30},100);check(w.energy_capacity(1,1,0,100,{0,-2}));}
  const auto result=w.resource_gain(f.source,f.target,*resource_gain(365,12),mode==2?1000:100);check(result.has_value());
  check(w.resources(1)->health==(mode==1?1000u:612u));
  check(w.resources(1)->energy==(mode==1?30u:mode==2?20u:mode==4?2u:22u));
 }
 for(unsigned mode=0;mode<6;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;w.set_vitals(1,{500,1000,10,30},100);
  if(mode!=5)check(f.condition(combat::conditions::Kind::blind,100,10000));casting::Runtime cast(1);casting::Hooks h;std::vector<Packet> wire;
  h.events=[&](const auto& batch){wire.insert(wire.end(),batch.begin(),batch.end());};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto amount,auto now){return w.consume_energy(1,amount,f.source,now);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.shout_target_valid=[&](auto id,float range){return w.shout_target_valid(f.source,id,range,100);};
  h.resource_gain=[&](auto target,const auto& plan,auto now){return w.resource_gain(f.source,target,plan,now);};
  Policy p;p.primary=1;p.campaigns=255;p.learned[365]=1;Slots bar{};bar[0]={365,0};attributes::Ranks ranks{};ranks[21]=12;
  if(mode==1)h.shout_target_valid={};if(mode==2){w.leave(2);w.join(2,"Far foe",2000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,30,30},100);}
  if(mode==3)check(w.gale(f.target,f.source,12,{},100));
  if(mode==4)h.resource_gain={};
  const auto reason=cast.start(100,{365,0,2},bar,p,ranks,true,h);
  if(mode==1||mode==4){check(reason==casting::Reason::unsupported&&w.resources(1)->energy==10);continue;}
  if(mode==2){check(reason==casting::Reason::target&&w.resources(1)->energy==10);continue;}
  check(reason==casting::Reason::accepted&&!cast.active()&&w.resources(1)->health==(mode==5?500u:556u)&&w.resources(1)->energy==(mode==5?5u:11u));
  check(std::count(wire.begin(),wire.end(),casting::generic(48,1,365))==1);
  check(std::count(wire.begin(),wire.end(),casting::slot_event(228,1,{365,0},15))==1);
  check(std::count(wire.begin(),wire.end(),casting::terminal(58,1))==0);
  check(cast.start(101,{365,0,2},bar,p,ranks,true,h)==casting::Reason::recharge);
 }
 {warrior_test::Fixture f(340,false);f.world.set_vitals(1,{10,2000,0,100},100);
  for(unsigned i=0;i<10;++i)check(f.condition(static_cast<combat::conditions::Kind>(i),100,10000));
  const auto result=f.world.resource_gain(f.source,f.target,*resource_gain(365,20),100);
  check(result&&result->health_delta==870&&result->energy_gained==80&&f.world.resources(1)->health==880);
 }
 std::cout<<"PASS Victory Is Mine player/NPC conditions,expiry,Health gain,Energy cap/Overcast/debt,knockdown and instant lifecycle\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
