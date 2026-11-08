#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Hex Breaker Reversal line "+std::to_string(at.line()));}
struct Fixture{
 world::Instance w;effects::Scope caster,owner;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(unsigned hp=500){for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,box,0,{}, {},100,id);w.set_vitals(id,{id==1?hp:1000u,1000,100,100},100);}caster=*w.effect_scope(1);owner=*w.effect_scope(2);}
 void rr(unsigned rank){check(w.status_mutation(caster,307,900,rank,100,8100,true));}
 bool has(unsigned id,unsigned skill,unsigned now=200){const auto s=*w.status_snapshot(*w.effect_scope(id),now);return std::any_of(s.begin(),s.end(),[&](const auto& e){return e.skill==skill;});}
 void stance(unsigned rank){check(w.hex_breaker(owner,*effects::hex_breaker(rank),20,100));}
 bool hex(unsigned skill,unsigned now=200){if(skill==28)return w.backfire(caster,owner,*effects::backfire(28,12),20,now);if(skill==100)return w.soul_barbs(caster,owner,*effects::soul_barbs(12),20,now);if(skill==76)return w.movement_hex(caster,owner,*effects::movement_hex(76,12),now);if(skill==99)return w.apply_parasitic_bond(caster,owner,12,now);if(skill==44)return w.wound_spell(caster,owner,*effects::wound_spell(44,12),20,now);if(skill==135)return w.faintheartedness(caster,owner,*effects::faintheartedness(12),now);return w.degeneration_hex(caster,owner,*effects::degeneration_hex(skill,12),now);}
};
double expected(unsigned hp,unsigned amount,int rank){const auto heal=rank<0?0u:std::min(amount,effects::reversal_fortune(rank)->maximum_healing);return std::max(0.,std::min(1000.,hp+double(heal))-(amount-heal));}
unsigned word(const world::Packet& p,unsigned at){return unsigned(p[at])|(unsigned(p[at+1])<<8)|(unsigned(p[at+2])<<16)|(unsigned(p[at+3])<<24);}
int main()try{
 unsigned cases=0;
 for(unsigned skill:{28u,100u,76u,137u,101u,150u,31u,53u,129u,2054u,99u,44u,135u})for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u,1000u}){
  ++cases;Fixture f(hp);f.stance(rank);if(rr>=0)f.rr(rr);check(f.hex(skill));const auto wanted=expected(hp,effects::hex_breaker(rank)->damage,rr);check(std::abs(double(f.w.resources(1)->health)-wanted)<1.01&&!f.has(1,307)&&!f.has(2,10)&&!f.has(2,skill));if(wanted>0){check(f.hex(skill,300)&&f.has(2,skill,300));check(std::abs(double(f.w.resources(1)->health)-wanted)<1.01);}
 }
 for(unsigned skill:{28u,100u,76u,137u,101u,150u,31u,53u,129u,2054u,99u,44u,135u})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f;check(f.hex(skill,100));f.stance(12);f.rr(rr);check(f.hex(skill));const auto s=*f.w.status_snapshot(f.owner,200);const auto old=std::find_if(s.begin(),s.end(),[&](const auto& e){return e.skill==skill;});check(old!=s.end()&&old->applied==100&&!f.has(2,10)&&!f.has(1,307));
 }
 for(unsigned skill:{28u,100u,76u,137u,101u,150u,31u,53u,129u,2054u,99u,44u,135u})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f;f.stance(0);f.rr(rr);check(f.hex(skill,5100)&&f.w.resources(1)->health==500&&f.has(1,307,5100)&&f.has(2,skill,5100));
 }
 for(unsigned skill:{28u,100u,76u,137u,101u,150u,31u,53u,129u,2054u,99u,44u,135u})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(10);check(f.w.apply_condition(f.owner,f.caster,combat::conditions::Kind::deep_wound,100,10000));f.stance(0);f.rr(rr);check(f.hex(skill));check(f.w.resources(1)->health==0&&!f.has(1,307)&&!f.has(2,10)&&!f.has(2,skill));check(f.w.player_death_times({1,2})[0]==200);
 }
 for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})for(unsigned mode=0;mode<4;++mode){
  ++cases;Fixture f;f.stance(rank);if(rr>=0)f.rr(rr);if(mode&1)check(f.w.status_mutation(f.caster,346,901,0,100,10100,true));if(mode&2)check(f.w.protective_enchantment(f.caster,f.caster,*skills::regeneration::reviewed(299,12),0,100));check(f.hex(28));const auto amount=unsigned(std::max(0,int(effects::hex_breaker(rank)->damage*((mode&1)?2:1))-((mode&2)?15:0)));check(std::abs(double(f.w.resources(1)->health)-expected(500,amount,rr))<1.01);check(f.has(1,307)==(rr>=0&&!amount)&&!f.has(2,10)&&!f.has(2,28));
 }
 for(unsigned rank:{0u,12u,20u})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f;f.stance(rank);const auto stance=f.w.status_snapshot(f.owner,100)->front().instance;f.rr(rr);f.box->drain();check(f.hex(28));const auto packets=f.box->drain();unsigned owner=unsigned(packets.size()),recipient=owner,removals=0;for(unsigned i=0;i<packets.size();++i){const auto& p=packets[i];if(p.size()==10&&p[0]==67){if(word(p,2)==2&&word(p,6)==stance)owner=std::min(owner,i);if(word(p,2)==1&&word(p,6)==900){recipient=std::min(recipient,i);++removals;}}}check(owner<recipient&&recipient<packets.size()&&removals==2); // Same shared observer box receives one event per peer.
 }
 check(cases==642);std::cout<<"PASS "<<cases<<" Hex Breaker conversion,prevention,renewal,expiry,healing death,protection and retirement-order cases;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
