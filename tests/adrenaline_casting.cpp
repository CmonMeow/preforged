#include "../src/runtime/gw_skill_casting.h"
#include "../src/runtime/gw_adrenaline.h"
#include <stdexcept>
#include <source_location>
#include <iostream>
using namespace fw::gw;
static void check(bool v,const std::source_location where=std::source_location::current()){if(!v)throw std::runtime_error("adrenaline casting regression at line "+std::to_string(where.line()));}
static unsigned count(const std::vector<skills::Packet>& ps,unsigned op,unsigned kind){return std::count_if(ps.begin(),ps.end(),[&](const auto&p){return p.size()>=6&&(p[0]|(p[1]<<8))==op&&(std::uint32_t(p[2])|(std::uint32_t(p[3])<<8)|(std::uint32_t(p[4])<<16)|(std::uint32_t(p[5])<<24))==kind;});}
int main()try{
 for(bool npc:{false,true})for(unsigned id:{335u,336u,337u,338u,339u,1136u,351u,382u,383u,384u,385u,1144u}){
  skills::Slots bar{};bar[0]={id,0};skills::Policy policy{1,0,255,true,{{id,1}}};attributes::Ranks ranks{};ranks[17]=5;ranks[18]=12;ranks[19]=12;ranks[20]=12;
  combat::adrenaline::State pools;check(pools.configure(bar,0));skills::casting::Hooks h;std::vector<skills::Packet> packets;unsigned hits=0,cost=99,reconciles=0;
  combat::weapon::Owned weapon{10,1,{10,10,2},12,false,false,id==351?15u:(id==335||id==336||id==337||id==338||id==339||id==1136)?2u:27u};
  h.events=[&](const auto&p){packets.insert(packets.end(),p.begin(),p.end());};h.consume_energy=[&](unsigned n){cost=n;return true;};
  h.resources=[&](unsigned actor)->std::optional<vitals::Resources>{if(npc&&actor==43)return {};return vitals::Resources{480,480,20,20};};h.weapon_snapshot=[&](){return std::optional(weapon);};h.melee_target=[](unsigned target,auto){return target==43;};
    h.weapon_attack_timing=[&](auto){return std::optional<combat::weapon::Timing>{{500,weapon.item_type==15?1750u:1330u,1200}};};
  h.target_scope=[](unsigned target){return std::optional(skills::effects::Scope{1,target,1});};h.attack_activity=[](auto){};
  h.melee_attack=[&](auto,const auto&,const auto& plan,auto now){check(plan.strength==5&&plan.rank==12);++hits;pools.hit(now);return true;};
  h.consume_adrenaline=[&](auto slot,auto now){return id==385?pools.spend_all(slot,now):pools.spend(slot,now);};
  h.reconcile_adrenaline=[&](auto){++reconciles;for(const auto&p:pools.pools())packets.push_back(combat::adrenaline::snapshot_2022(42,p));};
  skills::casting::Runtime runtime(42);check(runtime.start(0,{id,0,43},bar,policy,ranks,true,h)==skills::casting::Reason::adrenaline);check(!runtime.active());
  pools.gain(400,1);check(runtime.start(1,{id,0,43},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);check(cost==0&&pools.pools()[0].units==0);
  check(packets.back()==skills::casting::activation(42,43,id,true));check(count(packets,159,4)==0&&count(packets,159,50)==1);
  runtime.update(500,h);check(hits==0);runtime.update(501,h);check(hits==1&&reconciles==1&&pools.pools()[0].units==25);
  check(count(packets,158,1)==1&&count(packets,158,3)==0);
  check(count(packets,158,46)==1&&count(packets,158,58)==0);
  check(packets.back()[0]==208); // Publication follows native228, whose callback clears the used slot.
  pools.gain(400,1000);skills::casting::Runtime cancelled(42);check(cancelled.start(1000,{id,0,43},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);cancelled.cancel(h);cancelled.update(1500,h);check(hits==1&&pools.pools()[0].units==0);
  check(count(packets,158,3)==1&&count(packets,158,1)==1);cancelled.cancel(h);check(count(packets,158,3)==1);
  check(count(packets,158,49)==1&&count(packets,158,59)==0);
  pools.gain(400,2000);weapon.item_type=26;skills::casting::Runtime wrong(42);check(wrong.start(2000,{id,0,43},bar,policy,ranks,true,h)==skills::casting::Reason::weapon);check(pools.ready({id,0},2000));
 }
 // Mastery does not become Strength penetration. Bonus damage remains separate.
 combat::weapon::Strike strike;strike.level=20;strike.mastery=12;strike.minimum=10;strike.maximum=10;strike.location_armor=100;strike.base_roll=.5;strike.miss_roll=.5;
 auto zero=*combat::weapon::attack_skill(335,12,0),strength=*combat::weapon::attack_skill(335,12,12);
 auto first=combat::weapon::resolve_attack_skill(strike,zero,false),second=combat::weapon::resolve_attack_skill(strike,strength,false);check(first&&second&&second->damage>first->damage&&zero.bonus==26);
 const auto sever=*combat::weapon::attack_skill(382,12),gash=*combat::weapon::attack_skill(384,12);
 check(sever.required_type==27&&gash.required_type==27&&sever.bonus==0&&gash.bonus==17);
 check(combat::weapon::condition_duration(sever)==21000&&combat::weapon::condition_duration(gash)==17000);
 const auto plain=combat::weapon::resolve_attack_skill(strike,gash,false,false,false),bleeding=combat::weapon::resolve_attack_skill(strike,gash,false,false,true);
 check(plain&&bleeding&&bleeding->damage==plain->damage+17);
 for(unsigned skill:{339u,1136u})for(unsigned strength_rank:{0u,12u,20u}){
  const auto plan=*combat::weapon::attack_skill(skill,12,strength_rank);
  check(plan.bonus==17&&plan.required_type==2&&plan.armor_penetration==20);
  combat::weapon::Strike axe;axe.level=20;axe.mastery=12;axe.minimum=axe.maximum=20;axe.location_armor=100;axe.base_roll=axe.miss_roll=.5;
  const auto hit=combat::weapon::resolve_attack_skill(axe,plan,false);
  check(hit&&std::abs(hit->damage-(10*std::sqrt(2.)+17))<1e-9); // Armor100 ->80, not68 at Strength12.
  axe.weakness=true;const auto weak=combat::weapon::resolve_attack_skill(axe,plan,false);
  check(weak&&std::abs(weak->damage-(10*std::sqrt(2.)*.34+17))<1e-9);
  axe.blind=true;check(combat::weapon::resolve_attack_skill(axe,plan,false)->missed);
  auto forged=plan;forged.armor_penetration=40;check(!combat::weapon::resolve_attack_skill(axe,forged,false));
 }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
