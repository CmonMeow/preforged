#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Burning Speed cast line "+std::to_string(at.line()));
}
int main()try{
 using namespace skills;
 for(unsigned mode=0;mode<7;++mode){
  world::Instance w;auto caster_box=std::make_shared<world::Outbox>();
  w.join(1,"Elementalist",0,0,caster_box,0,{}, {},100,1);
  w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(2,{1000,1000,100,100},100);
  caster_box->drain();
  const auto scope=*w.effect_scope(1),foe=*w.effect_scope(2);
  inventory::Profile profile;
  if(mode==3){
   profile.bags.emplace(1,inventory::Bag{1,1,0,20});
   inventory::Item weapon;weapon.definition.id=100;weapon.definition.type=26;
   weapon.definition.quantity=1;weapon.definition.interaction=0x200000;
   weapon.definition.modifiers={0x22b80014u};weapon.bag=1;weapon.slot=0;
   profile.items.emplace(100,weapon);profile.weapons.sets[0]={100,0};
  }
  const auto duration=[&](unsigned skill,std::uint64_t base,std::uint64_t at)->std::optional<std::uint64_t>{
   const auto equipment=items::weapon_passives::equipped_enchanting(profile,7);
   const auto active=w.status_snapshot(scope,at);
   if(!equipment||!active)return {};
   const auto quote=skills::effects::enchanting_duration(skill,base,scope,*equipment,*active,at);
   return quote?quote->for_enchantment(skill,base,scope,at):std::nullopt;
  };
  std::vector<world::Packet> sent;unsigned paid=0;
  casting::Hooks h;
  h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());
   w.skill_event_live(1,packets,[]{return std::uint64_t{1250};});};
  h.resources=[&](auto id){return w.resources(id);};
  h.target_valid=[](auto id,float,bool self){return id==1&&self;};
  h.consume_energy_at=[&](auto energy,auto at){paid+=energy;return w.consume_energy(1,energy,scope,at);};
  h.effect_snapshot=[&](auto at){return w.status_snapshot(scope,at);};
  h.local_enchantment_duration=duration;
  h.status_mutation=[&](auto skill,auto effect,auto rank,auto at,auto until,bool active){
   if(active&&skill==823){
    const auto equipment=items::weapon_passives::equipped_enchanting(profile,7);
    const auto effects=w.status_snapshot(scope,at);
    if(!equipment||!effects)return false;
    const auto quote=skills::effects::enchanting_duration(823,7000,scope,*equipment,*effects,at);
    return quote&&w.status_mutation(scope,skill,effect,rank,at,until,true,{},&*quote);
   }
   return w.status_mutation(scope,skill,effect,rank,at,until,active);
  };
  h.burning_speed=[&](auto rank,auto effect,auto at,auto until){
   const auto equipment=items::weapon_passives::equipped_enchanting(profile,7);
   const auto effects=w.status_snapshot(scope,at);
   if(!equipment||!effects)return casting::EffectCompletion{};
   const auto quote=skills::effects::enchanting_duration(823,7000,scope,*equipment,*effects,at);
   if(!quote)return casting::EffectCompletion{};
   std::function<void()> publication;
   const bool accepted=w.status_mutation(scope,823,effect,rank,at,until,true,{},&*quote,&publication);
   return casting::EffectCompletion{accepted,{}, {},std::move(publication)};
  };
  if(mode==1)h.local_enchantment_duration={};
  if(mode==2)h.effect_snapshot={};
  if(mode==6)h.burning_speed={};
  Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[823]=1;
  Slots bar{};bar[0]={823,0};attributes::Ranks ranks{};ranks[10]=15;
  casting::Runtime runtime(1);
  const auto result=runtime.start(1000,{823,0,0},bar,policy,ranks,true,h);
  if(mode==1||mode==2||mode==6){check(result==casting::Reason::unsupported&&paid==0);continue;}
  check(result==casting::Reason::accepted&&paid==10);
  runtime.update(1250,h);
  const auto active=w.status_snapshot(scope,1250);
  check(active&&active->size()==1&&active->front().skill==823);
  const auto expiry=mode==3?9250u:8250u;
  check(active->front().expires==expiry);
  check(std::count(sent.begin(),sent.end(),casting::generic(21,1,339))==1);
  check(std::count(sent.begin(),sent.end(),casting::effect_add(1,823,15,active->front().instance,float(expiry-1250)/1000.f))==1);
  const auto wire=caster_box->drain();
  const auto finish=std::find(wire.begin(),wire.end(),casting::terminal(58,1));
  const auto body=std::find(wire.begin(),wire.end(),casting::generic(21,1,339));
  const auto owned=std::find(wire.begin(),wire.end(),casting::effect_add(1,823,15,active->front().instance,float(expiry-1250)/1000.f));
  const auto burning=std::find_if(wire.begin(),wire.end(),[](const auto& packet){
   return packet.size()==20&&(packet[0]|unsigned(packet[1])<<8)==65&&
      (packet[6]|unsigned(packet[7])<<8)==480;
  });
  check(finish!=wire.end()&&body!=wire.end()&&owned!=wire.end()&&burning!=wire.end()&&
      finish<body&&body<owned&&owned<burning);
  if(mode==4){
   check(w.enchantment_removal(foe,scope,*skills::effects::enchantment_removal(68,0),20,2000));
   const auto stripped=caster_box->drain();
   const auto removed=casting::effect_remove(1,active->front().instance);
   check(std::count(stripped.begin(),stripped.end(),removed)==1);
   runtime.update(2000,h);
   check(w.status_snapshot(scope,2000)->empty()&&w.has_condition(foe,combat::conditions::Kind::burning,2000));
   const auto after=caster_box->drain();check(std::count(after.begin(),after.end(),removed)==0);
   continue;
  }
  if(mode==5){
   check(runtime.start(2300,{823,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);
   runtime.update(2550,h);
   const auto renewed=w.status_snapshot(scope,2550);
   check(renewed&&renewed->size()==1&&renewed->front().instance!=active->front().instance);
   const auto changed=caster_box->drain();
   check(std::count(changed.begin(),changed.end(),casting::effect_remove(1,active->front().instance))==1);
   check(w.has_condition(foe,combat::conditions::Kind::burning,2550));
   w.tick(8250);check(w.status_snapshot(scope,8250)->size()==1);
   continue;
  }
  runtime.update(expiry,h);
  check(w.status_snapshot(scope,expiry)->empty());
  check(w.has_condition(foe,combat::conditions::Kind::burning,expiry));
 }
 std::cout<<"PASS Burning Speed paid cast, body339, effect duration, expiry and missing-hook admission\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
