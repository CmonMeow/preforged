#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Justice line "+std::to_string(at.line()));}
unsigned charge(world::Instance& w,Scope owner,unsigned skill=340){const auto pools=w.adrenaline(owner);check(pools.has_value());for(const auto& p:*pools)if(p.slot.skill==skill)return p.units;throw std::runtime_error("pool");}
int main()try{
 check(for_great_justice()->duration==20000&&for_great_justice(343,true)->skill==2883&&for_great_justice(2883)->duration==8000&&!for_great_justice(300));
 for(unsigned mode=0;mode<7;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={340,0};check(w.configure_adrenaline(f.source,bar,100));
  const bool pvp=mode==1||mode==3;check(w.for_great_justice(f.source,*for_great_justice(343,pvp),100));
  if(mode==2||mode==3)check(w.status_mutation(f.source,370,123,0,100,5100,true));
  if(mode==4){w.tick(20100);}
  if(mode==5)check(w.status_mutation(f.source,56,124,0,100,8100,true));
  if(mode==6)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,10000));
  const auto at=mode==4?20200u:200u;check(w.ordinary_attack(f.source,2,at));w.tick(at+500);
  check(charge(w,f.source)==(mode==3?75u:mode==4?25u:mode>=5?0u:50u));
 }
 for(unsigned mode=0;mode<3;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={340,0};check(w.configure_adrenaline(f.target,bar,100));
  check(w.for_great_justice(f.target,*for_great_justice(343,mode!=0),100));
  if(mode==2)check(w.status_mutation(f.target,370,123,0,100,5100,true));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),200));check(charge(w,f.target)==(mode==0?4u:mode==1?2u:3u));
 }
 for(bool pvp:{false,true}){warrior_test::Fixture f(340,false,500);auto& w=f.world;Slots bar{};bar[0]={329,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.for_great_justice(f.source,*for_great_justice(343,pvp),100));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(324,12),200));
  check(charge(w,f.source,329)==(pvp?100u:150u)); // base hit plus two-strike skill reward
 }
 for(bool pvp:{false,true}){warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={345,0};bar[1]={340,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.for_great_justice(f.source,*for_great_justice(343,pvp),100));
  check(w.status_mutation(f.source,345,123,15,100,5100,true));w.tick(6100);
  check(charge(w,f.source,345)==(pvp?25u:50u));w.tick(6200);check(charge(w,f.source,345)==(pvp?25u:50u));
 }
 for(bool pvp:{false,true}){warrior_test::Fixture f(340,false);auto& w=f.world;casting::Runtime cast(1);casting::Hooks h;std::vector<Packet> wire;
  h.events=[&](const auto& packets){wire.insert(wire.end(),packets.begin(),packets.end());};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto cost,auto now){return w.consume_energy(1,cost,f.source,now);};h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.target_valid=[](auto id,float,bool){return id==1;};h.for_great_justice=[&](auto owner,const auto& plan,auto now){return w.for_great_justice(owner,plan,now);};
  Policy p;p.primary=1;p.campaigns=255;p.pvp_match=pvp;p.learned[343]=1;Slots bar{};bar[0]={343,0};attributes::Ranks ranks{};
  check(cast.start(100,{343,0,2},bar,p,ranks,true,h)==casting::Reason::accepted&&!cast.active()&&w.resources(1)->energy==25);
  const auto effects=*w.status_snapshot(f.source,100);check(effects.size()==1&&effects[0].skill==(pvp?2883u:343u)&&effects[0].expires==(pvp?8100u:20100u));
  check(std::count(wire.begin(),wire.end(),casting::generic(48,1,343))==1);
  check(std::count(wire.begin(),wire.end(),casting::slot_event(228,1,{343,0},45))==1);
  check(cast.start(101,{343,0,1},bar,p,ranks,true,h)==casting::Reason::recharge);
 }
 std::cout<<"PASS FGJ PvE/PvP hit/damage/skill/end rewards,cap with Berserker,miss/forbidden/expiry,Runtime cost/mode/recharge\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
