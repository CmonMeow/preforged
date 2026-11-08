#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Attunement completion line "+std::to_string(at.line())+" "+context);}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope source;inventory::Profile gear;
 Fixture(unsigned version,bool extended=false){w.join(1,"Self caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{100,1000,50,100},100);source=*w.effect_scope(1);
  if(version){const auto p=*effects::ether_renewal(version,0);check(w.status_mutation(source,version,80,0,100,100+p.duration,true));}
  gear.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item weapon;weapon.definition.id=100;weapon.definition.type=26;weapon.definition.quantity=1;if(extended)weapon.definition.modifiers={0x22b80014};weapon.bag=1;gear.items.emplace(100,weapon);gear.weapons.sets[0].weapon=100;box->drain();
 }
 std::optional<effects::EnchantmentDuration> quote(unsigned skill,unsigned rank,std::uint64_t now,bool wrong=false){const auto base=casting::owned_self_enchantment_base(skill,rank);const auto equip=items::weapon_passives::equipped_enchanting(gear,1);const auto active=w.status_snapshot(source,now);auto owner=source;if(wrong)++owner.generation;return equip&&active?effects::enchanting_duration(skill,base,owner,*equip,*active,now):std::nullopt;}
 bool complete(effects::Scope owner,unsigned skill,unsigned rank,std::uint64_t event,std::uint64_t now,const effects::EnchantmentDuration* duration,std::uint64_t* completed_at=nullptr){const auto plan=effects::attunement(skill,rank);return plan&&w.attunement_enchantment(owner,*plan,now,event,duration,completed_at);}

};
int main()try{unsigned manual=0,paid=0;
 for(unsigned skill:{164u,169u,184u,208u,225u})for(unsigned version:{0u,181u,2860u}){
  for(unsigned mode=0;mode<10;++mode){context="manual skill="+std::to_string(skill)+" version="+std::to_string(version)+" mode="+std::to_string(mode);Fixture f(version,mode==5);
   const auto now=mode==3&&version?100+effects::ether_renewal(version,0)->duration:mode==8&&version?effects::ether_renewal(version,0)->duration:200;f.w.tick(now);check(f.w.before_spell_effect(f.source,skill,1,now));f.box->drain();const auto before=*f.w.resources(1);auto scope=f.source;if(mode==2)++scope.generation;
   auto duration=f.quote(skill,0,mode==9?now-1:now,mode==6);check(bool(duration));if(mode==7||mode==8)f.w.tick(now+300);const auto effective=(mode==7||mode==8)?now+300:now;bool ok=f.complete(scope,skill,0,mode==1?2:1,now,&*duration);
   if(mode==1||mode==2||mode==6||mode==9){check(!ok&&f.box->drain().empty());check(f.w.resources(1)->health==before.health&&f.w.resources(1)->energy==before.energy);duration=f.quote(skill,0,now);check(bool(duration));ok=f.complete(f.source,skill,0,1,now,&*duration);}
   check(ok);const bool reward=version&&mode!=3&&mode!=8;check(f.w.resources(1)->health==before.health+(reward?5:0)&&f.w.resources(1)->energy==before.energy+(reward?1:0));
   const auto statuses=*f.w.status_snapshot(f.source,effective);const auto base=casting::owned_self_enchantment_base(skill,0);const auto expected=mode==5?(base/1000+(base/1000*20+50)/100)*1000:base;
   check(std::count_if(statuses.begin(),statuses.end(),[&](const auto& e){return e.skill==skill&&e.expires==effective+expected;})==1);
   f.box->drain();check(!f.complete(f.source,skill,0,1,now,&*duration)&&f.box->drain().empty());
   if(mode==4){check(f.w.before_spell_effect(f.source,skill,2,now+100));duration=f.quote(skill,0,now+100);const auto prior=*f.w.resources(1);check(f.complete(f.source,skill,0,2,now+100,&*duration));check(f.w.resources(1)->health==prior.health+(version?10:0)&&f.w.resources(1)->energy==prior.energy+(version?2:0));}
   ++manual;
  }
  for(unsigned mode=0;mode<10;++mode){context="paid skill="+std::to_string(skill)+" version="+std::to_string(version)+" mode="+std::to_string(mode);Fixture f(version,true);casting::Runtime cast(1);casting::Hooks h;unsigned calls=0,before_health=0,before_energy=0;std::uint64_t admitted=0;std::optional<effects::EnchantmentDuration> delayed_quote;
   h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& p){f.box->enqueue(p);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.target_valid=[](auto id,float,bool){return id==1;};h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
   h.enchantment_duration_ready=[&](auto used,auto base,auto at){check(used==skill&&base==casting::owned_self_enchantment_base(skill,0));return bool(f.quote(skill,0,at));};
   h.before_spell_effect=[&](auto used,auto event,auto at){check(used==skill);f.w.tick(at);admitted=event;before_health=f.w.resources(1)->health;before_energy=f.w.resources(1)->energy;const bool accepted=f.w.before_spell_effect(f.source,used,event,at);if(mode==9){delayed_quote=f.quote(skill,0,at);f.w.tick(at+300);}return accepted;};
   h.effect_snapshot=[&](auto at){return f.w.status_snapshot(f.source,at);};h.restore_energy=[&](unsigned n){return f.w.restore_energy(1,n,f.source);};
   h.mantra_recall=[](auto,unsigned,auto)->bool{throw std::runtime_error("Recall silently downgraded");};h.conjure_enchantment=[](auto,const auto&,auto)->bool{throw std::runtime_error("Conjure silently downgraded");};
   h.simple_enchantment_completed=[&](auto target,auto used,unsigned rank,unsigned,auto event,auto at)->casting::EffectCompletion{check(target==f.source&&used==skill&&event==admitted&&event);++calls;auto duration=mode==9?delayed_quote:f.quote(skill,rank,at);std::uint64_t completed_at=0;const bool ok=duration&&f.complete(target,used,rank,event,at,&*duration,&completed_at);casting::EffectCompletion result{ok};if(ok)result.effect_snapshot_at=completed_at;return result;};
   h.armor_earth=[](auto,unsigned,auto)->bool{throw std::runtime_error("Armor silently downgraded");};h.armor_earth_completed=[&](auto target,unsigned rank,auto event,auto at){check(target==f.source&&event==admitted&&event);++calls;auto duration=mode==9?delayed_quote:f.quote(skill,rank,at);return duration&&f.complete(target,165,rank,event,at,&*duration);};
   Policy policy;policy.primary=skill==82?5:6;policy.campaigns=255;policy.learned[skill]=1;Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};attributes::Ranks ranks{};
   if(mode==7||mode==8){if(mode==7)h.enchantment_duration_ready={};else f.gear.items.at(100).definition.modifiers={0x22b800ff};check(cast.start(100,{skill,0,1},bar,policy,ranks,true,h)==casting::Reason::unsupported);check(f.w.resources(1)->energy==50&&f.w.resources(1)->health==100&&calls==0);++paid;continue;}
   check(cast.start(100,{skill,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);
   if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(101,h);if(mode==3){if(skill==165)h.armor_earth_completed={};else h.simple_enchantment_completed={};}if(mode==4)h.before_spell_effect={};if(mode==5)h.enchantment_duration_ready={};if(mode==6){f.w.leave(1);f.w.join(1,"Replacement",0,0,f.box,0,{}, {},200,1);f.w.set_vitals(1,{100,1000,40,100},100);}
   const auto now=100+static_cast<std::uint64_t>(find_skill(skill)->activation*1000);f.box->drain();cast.update(now,h);const auto packets=f.box->drain();cast.update(now+10000,h);check(calls==((mode==0||mode==9)?1u:0u));
   if(mode==0||mode==9){check(f.w.resources(1)->health==before_health+(version?5:0)&&f.w.resources(1)->energy==before_energy+(version?1:0)+(skill==164?0:4));const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));check(finish!=packets.end()&&std::count(packets.begin(),packets.end(),casting::terminal(58,1))==1);
    if(version){const auto heal=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=6&&p[0]==162&&p[2]==55;});check(heal<finish);}
    if(const auto body=casting::skill_success_body_effect(1,skill))check(std::count(packets.begin(),packets.end(),*body)==1);
    const auto statuses=*f.w.status_snapshot(f.source,mode==9?now+300:now);check(std::count_if(statuses.begin(),statuses.end(),[&](const auto& e){return e.skill==skill;})==1);

   }
   ++paid;
  }
 }
 for(unsigned skill:{164u,169u,184u,208u,225u}){Fixture f(181,true);casting::Runtime cast(1);casting::Hooks h;unsigned refund=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& p){f.box->enqueue(p);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.target_valid=[](auto id,float,bool){return id==1;};h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.before_spell_effect=[&](auto used,auto event,auto at){return f.w.before_spell_effect(f.source,used,event,at);};
  h.enchantment_duration_ready=[&](auto used,auto,auto at){return bool(f.quote(used,0,at));};
  h.effect_snapshot=[&](auto at){return f.w.status_snapshot(f.source,at);};h.restore_energy=[&](unsigned n){refund+=n;return f.w.restore_energy(1,n,f.source);};
  h.simple_enchantment_completed=[&](auto target,auto used,unsigned rank,unsigned,auto event,auto at)->casting::EffectCompletion{auto duration=f.quote(used,rank,at);return {duration&&f.complete(target,used,rank,event,at,&*duration)};};
  Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[skill]=1;Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};attributes::Ranks ranks{};
  check(cast.start(100,{skill,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);const auto paid_energy=f.w.resources(1)->energy;cast.update(1100,h);
  check(refund==(skill==164?0u:4u)&&f.w.resources(1)->energy==paid_energy+1+refund);
  auto active=*f.w.status_snapshot(f.source,1100);check(effects::elemental_attribute_bonus(active,1100)==(skill==164?1u:0u));
 }
 std::cout<<manual<<" attunement reward/replacement/expiry/duration/scope cases and "<<paid<<" paid completion/order/cancel/interrupt/hook-loss/replacement casts pass\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
