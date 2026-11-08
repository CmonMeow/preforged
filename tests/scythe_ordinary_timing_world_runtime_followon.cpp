// NEW unregistered shared period/deadline and actual Runtime timing53 trace.
// Scythe collateral geometry is not enabled;500ms basewindup is existing policy.
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("shared Scythe timing World/Runtime line "+std::to_string(at.line()));}
struct TracePacket{std::string origin;world::Packet packet;};std::vector<TracePacket> trace;
std::uint32_t word(const world::Packet& p,unsigned at){check(p.size()>=at+4);return std::uint32_t(p[at])|(std::uint32_t(p[at+1])<<8)|(std::uint32_t(p[at+2])<<16)|(std::uint32_t(p[at+3])<<24);}
unsigned capture(const std::vector<world::Packet>& packets,const std::string& origin,double period_ms=0){unsigned count=0;for(const auto& p:packets)if(p.size()==14&&p[0]==53&&p[1]==0){
  const float interval=std::bit_cast<float>(word(p,6)),factor=std::bit_cast<float>(word(p,10));check(interval>0&&factor>0);if(period_ms)check(std::abs(double(interval)*factor*1000-period_ms)<.01);trace.push_back({origin,p});++count;}return count;}
struct Fixture{
 world::Instance w;Scope actor,target;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();combat::weapon::Owned weapon;
 Fixture(unsigned type=35,unsigned mask=0):weapon{12,1,{20,20,type==5?1u:2u},12,false,false,type,0.}{
  w.join(1,"Timing owner",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);actor=*w.effect_scope(1);
  w.join(2,"Timing target",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);target=*w.effect_scope(2);
  check(w.physical_armor(target,60)&&w.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{3,0,20}));check(w.ordinary_weapon(actor,weapon,20,100,combat::weapon::CriticalProfile{1,0,20}));
  if(mask&1)check(w.status_mutation(actor,346,123,0,100,8100,true));if(mask&2)check(w.faintheartedness(target,actor,*faintheartedness(12),100));
  w.configure_attack_miss_draws([]{return .5;});w.configure_attack_critical_draws([]{return .999999;});box->drain();
 }
};
int main(int argc,char** argv)try{
 check(argc==2);
 for(unsigned type:{27u,35u})for(unsigned mask=0;mask<4;++mask){
  Fixture f(type,mask);const auto timing=f.w.weapon_attack_timing(f.actor,1000);check(bool(timing));
  const auto base=type==35?1500u:1330u;const auto multiplier=(mask&1?67u:100u)*(mask&2?150u:100u);
  const auto period=(base*multiplier+5000)/10000,windup=(500*multiplier+5000)/10000;
  check(timing->period_ms==period&&timing->windup_ms==windup&&timing->range==128);
  const std::string label="world-type"+std::to_string(type)+"-mask"+std::to_string(mask);
  check(f.w.ordinary_attack(f.actor,f.target.actor,1000));check(capture(f.box->drain(),label,period)==1);
  const auto first=1000+windup;f.w.tick(first-1);check(f.w.resources(2)->health==1000&&f.w.take_attack_outcomes(f.actor).empty());capture(f.box->drain(),label,period);
  f.w.tick(first);const auto first_events=f.w.take_attack_outcomes(f.actor);check(first_events.size()==1&&first_events[0].at==first&&first_events[0].outcome==combat::weapon::AttackOutcome::hit);const auto first_hp=f.w.resources(2)->health;check(first_hp<1000);capture(f.box->drain(),label,period);
  const auto second=first+period,animation=second-windup;
  f.w.tick(animation-1);check(capture(f.box->drain(),label,period)==0);check(f.w.resources(2)->health==first_hp);
  f.w.tick(animation);check(capture(f.box->drain(),label,period)==1);check(f.w.resources(2)->health==first_hp);
  f.w.tick(second-1);check(f.w.take_attack_outcomes(f.actor).empty()&&f.w.resources(2)->health==first_hp);capture(f.box->drain(),label,period);
  f.w.tick(second);const auto second_events=f.w.take_attack_outcomes(f.actor);check(second_events.size()==1&&second_events[0].at==second&&second_events[0].outcome==combat::weapon::AttackOutcome::hit);check(f.w.resources(2)->health<first_hp);f.w.cancel_ordinary_attack(f.actor);capture(f.box->drain(),label,period);
  const auto hp=f.w.resources(2)->health;f.w.tick(second);check(f.w.take_attack_outcomes(f.actor).empty()&&f.w.resources(2)->health==hp);
 }
 // Cancelled ordinary intent leaves the actual released arrow owned by the
 // existing queue. Only its canonical projectile frontier can bypass cadence.
 {Fixture f(5);check(f.w.ordinary_attack(f.actor,f.target.actor,1000));capture(f.box->drain(),"world-bow",2475);
  f.w.tick(1499);check(f.w.resources(2)->health==1000);f.w.tick(1500);check(f.w.resources(2)->health==1000);f.w.cancel_ordinary_attack(f.actor);
  const auto flight=projectiles::intercept({2500,32},0,0,80,0,0,0);check(bool(flight));const auto arrives=1500+flight->flight_ms;
  f.w.tick(arrives-1);check(f.w.resources(2)->health==1000&&f.w.take_attack_outcomes(f.actor).empty());
  f.w.tick(arrives);const auto out=f.w.take_attack_outcomes(f.actor);check(out.size()==1&&out[0].at==arrives&&out[0].outcome==combat::weapon::AttackOutcome::hit&&f.w.resources(2)->health<1000);capture(f.box->drain(),"world-bow",2475);
 }
 // Actual Runtime packets consume the real World timing hook. Completion is a
 // trusted timing sink: this does NOT claim PowerAttack Scythe effect support.
 for(unsigned mask=0;mask<4;++mask){Fixture f(35,mask);skills::casting::Hooks h;std::vector<world::Packet> events;unsigned commits=0;
  const auto timing=f.w.weapon_attack_timing(f.actor,1000,true);check(bool(timing));
  h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& batch){events.insert(events.end(),batch.begin(),batch.end());};
  h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.actor,at);};h.weapon_snapshot=[&](){return f.w.attack_weapon(f.actor);};
  h.weapon_attack_timing=[&](auto at){return f.w.weapon_attack_timing(f.actor,at,true);};h.attack_duration=[&](auto base,auto at){return f.w.attack_duration(f.actor,base,at);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.melee_target=[&](auto id,auto at){return f.w.melee_target(f.actor,id,at,322);};h.attack_skill_target=[&](auto id,auto skill,auto at){return f.w.melee_target(f.actor,id,at,skill);};
  h.attack_activity=[&](auto until){f.w.attack_activity(f.actor,until,1000);};h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto at){check(target==f.target&&combat::weapon::same_weapon(weapon,f.weapon)&&plan.skill==322&&at==1000+timing->windup_ms);++commits;return true;};
  skills::Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[322]=1;skills::Slots bar{};bar[0]={322,0};attributes::Ranks ranks{};ranks[17]=12;skills::casting::Runtime runtime(1);
  check(runtime.start(1000,{322,0,2},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);check(capture(events,"runtime-322-mask"+std::to_string(mask),timing->period_ms)==1);events.clear();
  runtime.update(1000+timing->windup_ms-1,h);check(commits==0);runtime.update(1000+timing->windup_ms,h);check(commits==1);runtime.update(1000+timing->windup_ms,h);check(commits==1&&f.w.resources(2)->health==1000);
 }
 const std::filesystem::path path=argv[1];if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());std::ofstream out(path,std::ios::binary);check(bool(out));out<<"{\"timing_packets\":[";bool comma=false;const char digits[]="0123456789abcdef";for(const auto& row:trace){if(comma)out<<',';comma=true;out<<"{\"origin\":\""<<row.origin<<"\",\"hex\":\"";for(auto byte:row.packet)out<<digits[byte>>4]<<digits[byte&15];out<<"\"}";}out<<"]}\n";out.close();check(bool(out));
 std::cout<<"PASS sharedSword/Scythe exact windup/repeat +/-1, rangedarrivaltimer, IAS/slow, actualWorld+Runtime53 trace; windupretailunproved\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

