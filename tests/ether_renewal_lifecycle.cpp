#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Ether Renewal lifecycle line "+std::to_string(at.line())+" "+context);
}
combat::Packet end_packet(unsigned effect){combat::Packet p{67,0};combat::detail::put(p,1);combat::detail::put(p,effect);return p;}
struct Fixture {
 world::Instance w;Scope owner,foe;inventory::Profile gear;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 casting::Runtime runtime{1};casting::Hooks h;
 Fixture(bool extended){
  for(unsigned id:{1u,2u}){w.join(id,"Actor",float(id*10),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,1000,50,100},100);}
  owner=*w.effect_scope(1);foe=*w.effect_scope(2);
  gear.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item item;item.definition.id=50;item.definition.type=26;
  item.definition.quantity=1;item.bag=1;if(extended)item.definition.modifiers={0x22b80014};gear.items.emplace(50,item);gear.weapons.sets[0].weapon=50;
  h.resources=[&](auto id){return w.resources(id);};h.target_scope=[&](auto id){return w.effect_scope(id);};h.events=[&](const auto& p){box->enqueue(p);};
  h.effect_snapshot=[&](auto at){return w.status_snapshot(owner,at);};
  h.status_mutation=[&](auto skill,auto effect,auto rank,auto at,auto expires,bool active){return w.status_mutation(owner,skill,effect,rank,at,expires,active);};
  box->drain();
 }
 EnchantmentDuration quote(unsigned skill,unsigned rank,std::uint64_t now){
  const auto equipment=items::weapon_passives::equipped_enchanting(gear,1);const auto active=w.status_snapshot(owner,now);
  const auto plan=ether_renewal(skill,rank);check(equipment&&active&&plan);
  const auto result=enchanting_duration(skill,plan->duration,owner,*equipment,*active,now);check(bool(result));return *result;
 }
 std::uint64_t apply(unsigned skill,unsigned rank,unsigned effect,std::uint64_t now){
  const auto plan=*ether_renewal(skill,rank);const auto receipt=quote(skill,rank,now);
  const auto duration=*receipt.for_enchantment(skill,plan.duration,owner,now);
  check(w.status_mutation(owner,skill,effect,rank,now,now+duration,true,{},&receipt));return now+duration;
 }
 void advance(std::uint64_t now){w.tick(now);runtime.update(now,h);}
};
int main()try{unsigned cases=0;
 for(unsigned skill:{181u,2860u})for(unsigned rank=0;rank<=20;++rank)for(bool extended:{false,true})for(unsigned mode=0;mode<8;++mode){
  context="skill="+std::to_string(skill)+" rank="+std::to_string(rank)+" extended="+std::to_string(extended)+" mode="+std::to_string(mode);
  Fixture f(extended);const auto ends=f.apply(skill,rank,80,100);auto last=ends;f.box->drain();std::vector<combat::Packet> observed;
  if(mode==0||mode==1){
   if(mode==1)for(std::uint64_t at=200;at<ends;at+=100)f.advance(at);
   else f.advance(ends-100);
   check(f.w.status_snapshot(f.owner,ends-100)->size()==1);f.advance(ends);
  }else if(mode==2){check(f.w.before_spell_effect(f.owner,174,1,100));check(f.w.kill(f.owner,200));
   check(!f.w.ether_renewal_completed(f.owner,174,1,200));check(f.w.resources(1)->health==0);
   f.runtime.update(200,f.h);check(f.w.status_snapshot(f.owner,200)->empty());f.advance(ends);}
  else if(mode==3){check(f.w.status_mutation(f.owner,skill,80,0,200,{},false));check(f.w.status_mutation(f.owner,skill,80,0,200,{},false));check(f.w.status_snapshot(f.owner,200)->empty());f.advance(ends);}
  else if(mode==4){check(f.w.enchantment_removal(f.foe,f.owner,*enchantment_removal(69,0),20,200));check(f.w.status_snapshot(f.owner,200)->empty());f.advance(ends);}
  else if(mode==5||mode==6){
   const auto next_skill=mode==5?skill:skill==181?2860u:181u;const auto next_ends=f.apply(next_skill,rank,81,200);
   last=std::max(ends,next_ends);
   const auto active=*f.w.status_snapshot(f.owner,200);check(active.size()==1&&active.front().skill==next_skill&&active.front().instance==81);
   observed=f.box->drain();check(std::count(observed.begin(),observed.end(),end_packet(80))==1);
   for(auto at:std::set<std::uint64_t>{ends,next_ends}){f.advance(at);const auto current=*f.w.status_snapshot(f.owner,at);check(current.size()==unsigned(at<next_ends));}
  }else{
   check(!f.w.status_mutation(f.owner,skill==181?2860:181,80,0,200,{},false));
   auto stale=f.owner;++stale.generation;
   check(!f.w.status_mutation(stale,skill,81,rank,200,200+ether_renewal(skill,rank)->duration,true));
   const auto receipt=f.quote(skill,rank,200);const auto duration=*receipt.for_enchantment(skill,ether_renewal(skill,rank)->duration,f.owner,200);
   check(!f.w.status_mutation(f.owner,skill,80,rank,200,200+duration,true,{},&receipt));
   check(!f.w.status_mutation(f.owner,skill,81,rank,201,201+duration,true,{},&receipt));
   check(f.w.status_snapshot(f.owner,200)->front().expires==ends);f.advance(ends);
  }
  check(f.w.status_snapshot(f.owner,last)->empty());
  // A later update, late idempotent strip and empty Runtime must not emit67again.
  f.advance(last+100);check(f.w.status_mutation(f.owner,skill,80,0,last+100,{},false));
  const auto tail=f.box->drain();observed.insert(observed.end(),tail.begin(),tail.end());check(std::count(observed.begin(),observed.end(),end_packet(80))==1);
  if(mode==5||mode==6)check(std::count(observed.begin(),observed.end(),end_packet(81))==1);
  if(mode!=2){check(f.w.before_spell_effect(f.owner,174,1,last+100));const auto before=*f.w.resources(1);check(!f.w.ether_renewal_completed(f.owner,174,1,last+100));check(f.w.resources(1)->health==before.health&&f.w.resources(1)->energy==before.energy);}
  ++cases;
 }
 unsigned resource_cases=0;
 for(unsigned skill:{181u,2860u})for(unsigned rank:{0u,12u,20u})for(bool extended:{false,true})for(bool dense:{false,true}){
  context="resource neutrality skill="+std::to_string(skill)+" rank="+std::to_string(rank)+" dense="+std::to_string(dense);
  Fixture f(extended),reference(extended);const auto ends=f.apply(skill,rank,80,100);
  if(dense)for(std::uint64_t at=200;at<ends+100;at+=100){f.advance(at);reference.advance(at);}
  f.advance(ends+100);reference.advance(ends+100);
  const auto actual=*f.w.resources(1),expected=*reference.w.resources(1);
  check(actual.health==expected.health&&actual.max_health==expected.max_health&&actual.energy==expected.energy&&actual.max_energy==expected.max_energy);
  ++resource_cases;
 }
 for(unsigned skill:{181u,2860u}){
  context="post-resurrection admission skill="+std::to_string(skill);Fixture f(false);
  f.apply(skill,0,80,100);check(f.w.before_spell_effect(f.owner,174,1,100));
  check(f.w.kill(f.owner,200)&&f.w.resurrect(f.owner,300));f.w.set_vitals(1,{100,1000,50,100},100);
  f.apply(skill,0,81,300);const auto before=*f.w.resources(1);
  check(!f.w.ether_renewal_completed(f.owner,174,1,300));
  check(f.w.resources(1)->health==before.health&&f.w.resources(1)->energy==before.energy);
  check(!f.w.before_spell_effect(f.owner,174,1,300));check(f.w.before_spell_effect(f.owner,174,2,300));
  check(f.w.ether_renewal_completed(f.owner,174,2,300));
  check(f.w.resources(1)->health==before.health+5&&f.w.resources(1)->energy==before.energy+1);
 }
 for(unsigned skill:{181u,2860u}){
  Fixture f(false);Policy p;p.primary=6;p.campaigns=255;p.learned[skill]=1;Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};attributes::Ranks ranks{};
  check(f.runtime.start(100,{skill,0,1},bar,p,ranks,true,f.h)==casting::Reason::unsupported);check(f.w.resources(1)->energy==50);
  check(!f.w.status_mutation(f.owner,skill,0,0,100,100+ether_renewal(skill,0)->duration,true));
  check(!f.w.status_mutation(f.owner,skill,0x80000000u,0,100,100+ether_renewal(skill,0)->duration,true));
 }
 std::cout<<"PASS "<<cases<<" Ether Renewal owned timer/death/strip/replacement/expiry/dedup cases, "<<resource_cases<<" zero-pip resource comparisons and2 post-resurrection admission cases;both normal cast bindings remain gated\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
