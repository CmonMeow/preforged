#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
std::string context;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Burning Renewal line "+std::to_string(at.line())+" "+context);}
unsigned word(const world::Packet& p,unsigned i){return unsigned(p[i])|(unsigned(p[i+1])<<8)|(unsigned(p[i+2])<<16)|(unsigned(p[i+3])<<24);}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source,foe;inventory::Profile gear;
 Fixture(unsigned version,bool extended=false){w.join(1,"Burn caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{100,1000,50,100},100);source=*w.effect_scope(1);
  w.join(2,"Foe",50,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,50,100},100);foe=*w.effect_scope(2);
  if(version){const auto p=*skills::effects::ether_renewal(version,0);check(w.status_mutation(source,version,80,0,100,100+p.duration,true));}
  gear.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item weapon;weapon.definition.id=100;weapon.definition.type=26;weapon.definition.quantity=1;if(extended)weapon.definition.modifiers={0x22b80014};weapon.bag=1;gear.items.emplace(100,weapon);gear.weapons.sets[0].weapon=100;box->drain();
 }
 auto quote(std::uint64_t now,bool wrong=false){const auto equip=items::weapon_passives::equipped_enchanting(gear,1);const auto active=w.status_snapshot(source,now);auto scope=source;if(wrong)++scope.generation;
  return equip&&active?skills::effects::enchanting_duration(823,7000,scope,*equip,*active,now):std::optional<skills::effects::EnchantmentDuration>{};}
 bool complete(skills::effects::Scope scope,unsigned effect,unsigned rank,std::uint64_t event,std::uint64_t now,
    const skills::effects::EnchantmentDuration& duration,std::uint64_t expires,std::function<void()>* publication=nullptr,std::uint64_t* actual=nullptr){
  return w.status_mutation(scope,823,effect,rank,now,expires,true,{},&duration,publication,event,actual);
 }
};
int main()try{unsigned manual=0,paid=0,expiry_checks=0;
 for(unsigned version:{0u,181u,2860u})for(bool extended:{false,true})for(unsigned mode=0;mode<8;++mode){
  context="manual version="+std::to_string(version)+" extended="+std::to_string(extended)+" mode="+std::to_string(mode);Fixture f(version,extended);
  if(mode==7){auto old=f.quote(100);check(old&&f.w.status_mutation(f.source,823,10,0,100,100+*old->for_enchantment(823,7000,f.source,100),true,{},&*old));}
  const auto now=mode==3&&version?100+skills::effects::ether_renewal(version,0)->duration:200;f.w.tick(now);check(f.w.before_spell_effect(f.source,823,1,now));
  auto duration=f.quote(now,mode==4);check(bool(duration));const auto length=mode==4?7000u:*duration->for_enchantment(823,7000,f.source,now);
  if(mode==5)f.w.tick(now+300);f.box->drain();const auto before=*f.w.resources(1);auto source=f.source;if(mode==2)++source.generation;
  std::function<void()> publication;std::uint64_t actual=0;const auto expires=now+length+(mode==6?1:0);
  auto ok=f.complete(source,20,0,mode==1?2:1,now,*duration,expires,&publication,&actual);
  if(mode==1||mode==2||mode==4||mode==6){check(!ok&&!publication&&f.w.resources(1)->health==before.health&&f.w.resources(1)->energy==before.energy);duration=f.quote(now);check(bool(duration));ok=f.complete(f.source,20,0,1,now,*duration,now+*duration->for_enchantment(823,7000,f.source,now),&publication,&actual);}
  check(ok&&publication&&actual==(mode==5?now+300:now));const unsigned count=mode==7?2:1;const bool reward=version&&mode!=3;
  check(f.w.resources(1)->health==before.health+(reward?5*count:0)&&f.w.resources(1)->energy==before.energy+(reward?count:0));
  const auto active=*f.w.status_snapshot(f.source,actual);check(std::count_if(active.begin(),active.end(),[&](const auto& e){return e.skill==823&&e.instance==20&&e.applied==actual&&e.expires==actual+(extended?8000:7000);})==1);
  check(f.w.has_condition(f.source,combat::conditions::Kind::burning,actual));check(f.w.has_condition(f.foe,combat::conditions::Kind::burning,actual)==(mode==7));
  publication();f.box->drain();publication();check(f.box->drain().empty());auto retry=f.quote(actual);check(retry&&!f.complete(f.source,21,0,1,actual,*retry,actual+*retry->for_enchantment(823,7000,f.source,actual)));
  f.w.move(1,10000,0,actual);f.w.tick(actual+1000);check(std::abs(f.w.position(1,actual+1000).first-374.4)<.4);++manual;
 }
 for(unsigned version:{0u,181u,2860u})for(bool extended:{false,true})for(unsigned mode=0;mode<10;++mode){
  context="paid version="+std::to_string(version)+" extended="+std::to_string(extended)+" mode="+std::to_string(mode);Fixture f(version,extended);skills::casting::Runtime cast(1);skills::casting::Hooks h;unsigned calls=0,before_health=0,before_energy=0;std::uint64_t admitted=0,actual=0;
  h.events=[&](const auto& p){f.box->enqueue(p);};h.resources=[&](auto id){return f.w.resources(id);};h.target_valid=[](auto id,float,bool){return id==1;};h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.consume_energy_at=[&](unsigned cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};h.effect_snapshot=[&](auto at){return f.w.status_snapshot(f.source,at);};
  h.local_enchantment_duration=[&](unsigned skill,auto base,auto at)->std::optional<std::uint64_t>{check(skill==823&&base==7000);auto duration=f.quote(at);return duration?duration->for_enchantment(skill,base,f.source,at):std::nullopt;};
  h.before_spell_effect=[&](auto skill,auto event,auto at){f.w.tick(at);before_health=f.w.resources(1)->health;before_energy=f.w.resources(1)->energy;admitted=event;return f.w.before_spell_effect(f.source,skill,event,at);};
  h.burning_speed=[](unsigned,unsigned,auto,auto)->skills::casting::EffectCompletion{throw std::runtime_error("Burning completion silently downgraded");};
  h.burning_speed_completed=[&](unsigned rank,unsigned effect,auto event,auto at,auto expires){check(event==admitted&&event);++calls;auto duration=f.quote(at);if(mode==7)f.w.tick(at+300);skills::casting::EffectCompletion result;result.accepted=duration&&f.complete(f.source,effect,rank,event,at,*duration,expires,&result.publication,&actual);if(result.accepted)result.effect_snapshot_at=actual;return result;};
  skills::Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[823]=1;skills::Slots bar{};bar[0]={823,0};attributes::Ranks ranks{};
  if(mode==8||mode==9){if(mode==8)h.before_spell_effect={};else h.local_enchantment_duration={};check(cast.start(100,{823,0,1},bar,policy,ranks,true,h)==skills::casting::Reason::unsupported);check(f.w.resources(1)->energy==50&&calls==0);++paid;continue;}
  check(cast.start(100,{823,0,1},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(200,h);if(mode==3)h.burning_speed_completed={};if(mode==4)h.before_spell_effect={};if(mode==5)h.local_enchantment_duration={};if(mode==6){f.w.leave(1);f.w.join(1,"Replacement",0,0,f.box,0,{}, {},200,1);f.w.set_vitals(1,{100,1000,40,100},100);}
  f.box->drain();cast.update(350,h);const auto packets=f.box->drain();cast.update(351,h);const bool applied=mode==0||mode==7;check(calls==(applied?1u:0u));
  if(applied){check(f.w.resources(1)->health==before_health+(version?5:0)&&f.w.resources(1)->energy==before_energy+(version?1:0));const auto finish=std::find(packets.begin(),packets.end(),skills::casting::terminal(58,1));const auto body=std::find(packets.begin(),packets.end(),skills::casting::generic(21,1,339));
   const auto status=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==20&&p[0]==65&&p[6]==(823&255)&&p[7]==(823>>8);});check(finish!=packets.end()&&body!=packets.end()&&status!=packets.end()&&finish<body&&body<status);check(std::count(packets.begin(),packets.end(),skills::casting::generic(21,1,339))==1);
   if(version){const auto heal=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=6&&p[0]==162&&p[2]==55;});check(heal<finish);}
   const auto active=*f.w.status_snapshot(f.source,actual);check(std::any_of(active.begin(),active.end(),[&](const auto& e){return e.skill==823&&e.applied==actual&&e.expires==actual+(extended?8000:7000);}));
   const auto entry=std::find_if(active.begin(),active.end(),[](const auto& e){return e.skill==823;});check(entry!=active.end());const auto effect=entry->instance;
   f.box->drain();f.w.tick(actual+(extended?8000:7000));cast.update(actual+(extended?8000:7000),h);const auto ended=f.box->drain();
   check(std::count_if(ended.begin(),ended.end(),[&](const auto& p){return p.size()==10&&p[0]==67&&word(p,2)==1&&word(p,6)==effect;})==1);++expiry_checks;
  }else {const auto active=*f.w.status_snapshot(*f.w.effect_scope(1),350);check(std::none_of(active.begin(),active.end(),[](const auto& e){return e.skill==823;}));}
  ++paid;
 }
 // A replacement must not consume its reward if the old end cohort has a newer clock.
 {Fixture f(181);check(f.w.status_mutation(f.source,823,10,0,100,7100,true));f.w.tick(200);check(f.w.before_spell_effect(f.source,823,1,200));check(f.w.health_capacity(f.foe,1001,500));const auto before=*f.w.resources(1);auto duration=f.quote(200);std::function<void()> publication;
  check(duration&&!f.complete(f.source,20,0,1,200,*duration,7200,&publication));check(!publication&&f.w.resources(1)->health==before.health&&f.w.resources(1)->energy==before.energy);
  check(!f.w.has_condition(f.foe,combat::conditions::Kind::burning,500));f.w.tick(500);duration=f.quote(500);const auto next=*f.w.resources(1);check(duration&&f.complete(f.source,20,0,1,500,*duration,7500));check(f.w.resources(1)->health==next.health+10&&f.w.resources(1)->energy==next.energy+2&&f.w.has_condition(f.foe,combat::conditions::Kind::burning,500));}
 // Frigid Armor suppresses the self condition, not movement or the later burst.
 {Fixture f(181);check(f.w.frigid_armor_enchantment(f.source,0,100));check(f.w.before_spell_effect(f.source,823,1,200));auto duration=f.quote(200);check(duration&&f.complete(f.source,20,0,1,200,*duration,7200));check(!f.w.has_condition(f.source,combat::conditions::Kind::burning,200));f.w.tick(7200);check(f.w.has_condition(f.foe,combat::conditions::Kind::burning,7200));}
 // The old burst may kill its owner through a foe's Fragility/Bile reactions.
 {Fixture f(181);f.w.join(3,"Bile caster",120,0,std::make_shared<world::Outbox>(),0,{}, {},100,3);f.w.set_vitals(3,{1000,1000,100,100},100);const auto caster=*f.w.effect_scope(3);
  check(f.w.status_mutation(f.source,823,10,0,100,7100,true));f.w.set_vitals(1,{10,1000,50,100},100);f.w.set_vitals(2,{10,1000,50,100},100);
  check(f.w.degeneration_hex(caster,f.foe,*skills::effects::degeneration_hex(2998,12),100,20)&&f.w.degeneration_hex(caster,f.foe,*skills::effects::degeneration_hex(2058,0),100,20));
  check(f.w.before_spell_effect(f.source,823,1,200));auto duration=f.quote(200);f.box->drain();check(duration&&f.complete(f.source,20,0,1,200,*duration,7200));f.w.tick(200);const auto packets=f.box->drain();check(f.w.resources(1)->health==0);
  check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==67&&word(p,2)==1&&word(p,6)==10;})==1);
  const auto active=*f.w.status_snapshot(f.source,200);check(std::none_of(active.begin(),active.end(),[](const auto& e){return e.skill==823;}));}
 std::cout<<manual<<" Burning Speed reward/receipt/replay/replacement/movement cases and "<<paid<<" paid completion/clock/cancel/hook-loss cases, "<<expiry_checks<<" one-shot expiry checks plus end-cohort retry, Frigid and nested-death retirement pass\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
