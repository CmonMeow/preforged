#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace fw::gw;

static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){
  const auto pve=combat::weapon::attack_skill(1191,rank);
  const auto pvp=combat::weapon::attack_skill(2864,rank);
  if(!pve||!pvp)std::cerr<<"missing plan rank="<<rank<<" pve="<<bool(pve)<<" pvp="<<bool(pvp)<<'\n';
  check(pve&&pvp,"catalog plans");
  check(pve->bonus==static_cast<unsigned>(std::lround(5.+20.*rank/15.)),"PvE bonus");
  check(pvp->bonus==static_cast<unsigned>(std::lround(3.+7.*rank/15.)),"PvP bonus");
  check(pve->armor_penetration==10&&pvp->armor_penetration==10,"armor penetration");
  check(pve->required_type==5&&pvp->required_type==5,"bow required");
  check(pve->windup_ms==750&&pvp->windup_ms==1000,"mode activation");
  check(combat::weapon::valid(*pve)&&combat::weapon::valid(*pvp),"canonical plans");
 }
 check(skills::casting::reviewed_cast_record(1191,false)->id==1191,"PvE row");
 check(skills::casting::reviewed_cast_record(1191,true)->id==2864,"PvP row");
 combat::weapon::Strike strike;strike.level=20;strike.mastery=12;strike.minimum=20;strike.maximum=20;
 strike.location_armor=60;strike.base_roll=.5;strike.miss_roll=.5;
 const auto ordinary=combat::weapon::resolve(strike);
 const auto pve=combat::weapon::resolve_attack_skill(strike,*combat::weapon::attack_skill(1191,12),false);
 const auto pvp=combat::weapon::resolve_attack_skill(strike,*combat::weapon::attack_skill(2864,12),false);
 check(ordinary&&pve&&pvp&&!pve->missed&&!pvp->missed,"resolved hits");
 const auto weapon_with_ap=ordinary->damage*std::exp2(60*.10/40.);
 check(std::abs(pve->damage-(weapon_with_ap+21))<1e-9,"PvE armor and bonus");
 check(std::abs(pvp->damage-(weapon_with_ap+9))<1e-9,"PvP armor and bonus");
 auto forged=*combat::weapon::attack_skill(2864,12);forged.armor_penetration=20;
 check(!combat::weapon::resolve_attack_skill(strike,forged,false),"forged penetration rejected");
 strike.blind=true;strike.miss_roll=0;
 const auto miss=combat::weapon::resolve_attack_skill(strike,*combat::weapon::attack_skill(2864,12),false);
 check(miss&&miss->missed&&miss->damage==0,"miss grants no bonus");
 for(bool pvp:{false,true}){
  world::Instance world;auto box=std::make_shared<world::Outbox>();
  world.join(1,"Ranger",0,0,box,0,{}, {},100,1);
  world.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  world.set_vitals(1,{1000,1000,30,30},100);
  world.set_vitals(2,{1000,1000,30,30},100);
  const auto source=*world.effect_scope(1),target=*world.effect_scope(2);
  check(world.physical_armor(target,60),"target armor");
  combat::weapon::Owned bow{12,1,{20,20,1},12,false,false,5,0.};
  check(world.ordinary_weapon(source,bow,20,100),"equipped bow");
  world.configure_attack_miss_draws([]{return .5;});
  skills::casting::Runtime runtime(1);skills::casting::Hooks hooks;unsigned commits=0;
  hooks.events=[](const auto&){};
  hooks.resources=[&](auto id){return world.resources(id);};
  hooks.target_scope=[&](auto id){return world.effect_scope(id);};
  hooks.consume_energy_at=[&](auto amount,auto at){return world.consume_energy(1,amount,source,at);};
  hooks.weapon_snapshot=[&](){return std::optional(bow);};
  hooks.melee_target=[&](auto id,auto at){return world.melee_target(source,id,at,1191);};
  hooks.melee_attack=[&](auto dst,const auto& owned,const auto& plan,auto at){++commits;check(plan.skill==(pvp?2864u:1191u),"mode plan at commit");return world.melee_attack(source,dst,owned,plan,at);};
  hooks.attack_activity=[&](auto until){world.attack_activity(source,until,1000);};
  hooks.weapon_attack_timing=[&](auto at){return world.weapon_attack_timing(source,at);};
  skills::Policy policy;policy.primary=2;policy.campaigns=255;policy.learned[1191]=1;policy.pvp_match=pvp;
  skills::Slots bar{};bar[0]={1191,0};attributes::Ranks ranks{};ranks[25]=12;
  check(runtime.start(1000,{1191,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::accepted,"Runtime start");
  check(world.resources(1)->energy==20,"10 Energy paid");
  const auto release=pvp?2000u:1750u;
  runtime.update(release-1,hooks);check(commits==0,"windup pending");
  runtime.update(release,hooks);check(commits==1,"single release");
  world.tick(release+200);
  check(world.resources(2)->health<1000,"World contact damage");
  check(runtime.start(release+3999,{1191,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::recharge,"4s recharge");
 }
 std::cout<<"sundering attack passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
