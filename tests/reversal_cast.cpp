#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
unsigned tested_context=0,tested_mode=0;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Reversal paid cast line "+std::to_string(at.line())+" context "+std::to_string(tested_context)+" mode "+std::to_string(tested_mode));
}
int main()try{
 const auto plan=*effects::reversal_fortune(12);unsigned cases=0;
 // All casts pass through Runtime and World, not trusted effect insertion.
 // Unknown/immune NPCs and foes fail admission without paying. Self is valid.
 for(unsigned context=0;context<8;++context)for(unsigned mode=0;mode<20;++mode){
  ++cases;tested_context=context;tested_mode=mode;const bool npc=context>=4,secondary=context==1||context==5;
  const unsigned target=context==2?1:context==3?3:npc?100:2,divine=secondary?0:12;
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  if(npc){
   world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{500,1000};
   d.healing_allowed=context==7?std::optional<bool>{}:std::optional(context!=6);
   d.healing_evidence=d.healing_allowed?"authored fixture":"-";
   n.combat_definition=d;w.configure_npcs({n});
  }
  w.join(1,"Caster",0,0,box,0,{world::Packet{165,0,1,0,0,0,static_cast<std::uint8_t>(secondary?6:3),0}}, {},100,1);
  w.set_vitals(1,{500,1000,100,100},100);
  w.join(2,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{500,1000,100,100},100);
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  w.join(3,"Foe",150,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),destination=*w.effect_scope(target);
  inventory::Profile inventory=inventory::baseline();
  inventory::Item weapon;weapon.definition.id=100;weapon.definition.file=1;weapon.definition.type=26;weapon.definition.quantity=1;
  weapon.definition.modifiers={0xa7a8160b,0x24b80b00,0x27980909,0x22b80014};weapon.bag=2;weapon.slot=0;
  weapon.profession=0;weapon.customized=u"";inventory.items.emplace(100,weapon);inventory.weapons.sets[0].weapon=100;
  combat::weapon::Policy weapon_policy;weapon_policy.primary=secondary?6:3;weapon_policy.character="Caster";weapon_policy.caster_weapon=true;weapon_policy.effective[9]=9;
  const auto owned_weapon=combat::weapon::owned_weapon(inventory,1,weapon_policy);
  check(owned_weapon&&owned_weapon->item==100&&owned_weapon->damage.maximum==22);
  const auto quote=[&](auto at)->std::optional<effects::EnchantmentDuration>{
   auto equipment=items::weapon_passives::equipped_enchanting(inventory,1);auto active=w.status_snapshot(source,at);
   return equipment&&active?effects::enchanting_duration(307,plan.duration,source,*equipment,*active,at):std::nullopt;
  };
  casting::Hooks h;h.resources=[&](auto id){return w.resources(id);};h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.target_valid=[](auto,float,bool){return false;};
  h.reversal_target_valid=[&](auto selected,unsigned df,auto at){return mode!=4&&!(mode==19&&at>=1250)&&w.valid_reversal_target(source,selected,df,at);};
  h.consume_energy_at=[&](auto cost,auto at){check(cost==5);return w.consume_energy(1,cost,source,at);};
  h.events=[&](const auto& packets){box->enqueue(packets);};
  h.before_spell_effect=[&](auto used,auto event,auto at){return mode!=10&&w.before_spell_effect(source,used,event,at);};
  h.enchantment_duration_ready=[&](unsigned skill,auto base,auto at){check(skill==307&&base==8000);return bool(quote(at));};
  unsigned attempts=0;h.reversal_fortune_completed=[&](auto selected,const auto& p,unsigned df,auto event,auto at)->casting::EffectCompletion{
   ++attempts;check(selected==destination&&p==plan&&df==divine&&event);auto duration=quote(at);
   const bool ok=duration&&w.complete_reversal_fortune(source,selected,p,df,at,&*duration,event);
   return {ok,{},{},{},false,true};
  };
  if(mode==1)h.reversal_fortune_completed={};if(mode==5)inventory.items.at(100).definition.modifiers.back()=0x22b800ff;
  if(mode==12)h.reversal_target_valid={};if(mode==14)h.before_spell_effect={};if(mode==16)h.target_scope={};if(mode==18)h.enchantment_duration_ready={};
  Policy policy;policy.primary=secondary?6:3;policy.secondary=secondary?3:0;policy.campaigns=255;policy.learned[307]=1;
  Slots bar{};bar[0]={307,0};attributes::Ranks ranks{};ranks[15]=12;ranks[16]=divine;casting::Runtime cast(1);
  check(!casting::Runtime::effect_supported(307));
  const bool unsupported=mode==1||mode==5||mode==12||mode==14||mode==16||mode==18;
  const bool denied=unsupported||mode==4||context==3||context>=6;
  const auto admission=cast.start(1000,{307,0,target},bar,policy,ranks,true,h);
  check(admission==(unsupported?casting::Reason::unsupported:denied?casting::Reason::target:casting::Reason::accepted));
  if(denied){check(!cast.active()&&w.resources(1)->energy==100&&attempts==0);continue;}
  check(w.resources(1)->energy==95);cast.update(1249,h);check(attempts==0);
  if(mode==2)h.reversal_fortune_completed={};
  if(mode==3)h.target_scope=[&](auto id)->std::optional<effects::Scope>{auto scope=w.effect_scope(id);if(scope&&id==target)++scope->generation;return scope;};
  if(mode==6)inventory.items.at(100).definition.modifiers.back()=0x22b800ff;if(mode==7)inventory.items.at(100).definition.modifiers.back()=0x22b8000a;
  if(mode==8)cast.cancel(h);if(mode==9)cast.interrupt(1249,h);if(mode==13)h.reversal_target_valid={};
  if(mode==15)h.before_spell_effect={};if(mode==17)h.enchantment_duration_ready={};
  box->drain();cast.update(1250,h);check(!cast.active());
  const bool applied=mode==0||mode==7||mode==11;check(attempts==unsigned(applied));
  const auto packets=box->drain();cast.update(1250,h);check(attempts==unsigned(applied)&&box->drain().empty());
  if(!applied){check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))==packets.end());continue;}
  const auto body=target==1?*particles::target_body_2022(1,307):*particles::targeted_body_2022(target,1,307);
  const auto finished=std::find(packets.begin(),packets.end(),casting::terminal(58,1)),visual=std::find(packets.begin(),packets.end(),body);
  const auto added=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==20&&p[0]==65&&(unsigned(p[6])|(unsigned(p[7])<<8))==307;});
  check(finished!=packets.end()&&visual!=packets.end()&&added!=packets.end()&&finished<visual&&visual<added);
  check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==1&&std::count(packets.begin(),packets.end(),body)==1);
  const auto health=npc?w.npc_combat_snapshot(destination)->health->current:w.resources(target)->health;
  check(health==(secondary?500:538));
  const unsigned duration=mode==7?9000:10000;
  const unsigned effect=unsigned((*added)[12])|(unsigned((*added)[13])<<8)|(unsigned((*added)[14])<<16)|(unsigned((*added)[15])<<24);
  check(*added==casting::effect_add(target,307,12,effect,float(duration)/1000.f));
  check(cast.start(1251,{307,0,target},bar,policy,ranks,true,h)==casting::Reason::busy);
  check(cast.start(2100,{307,0,target},bar,policy,ranks,true,h)==casting::Reason::recharge);
  if(mode==11&&!npc){
   const auto enemy=*w.effect_scope(3);const auto burn=*effects::energy_drain(42,12);const double before=health;
   check(w.before_spell_effect(enemy,42,1,2200));check(w.energy_drain(enemy,destination,burn,20,2200,nullptr,1));
   const auto after=npc?w.npc_combat_snapshot(destination)->health->current:w.resources(target)->health;
   const double raw=burn.amount*burn.damage_per_energy,converted=std::min(raw,double(plan.maximum_healing));
   check(after==before+converted-(raw-converted));
  }else{
   w.tick(1250+duration);const auto ended=box->drain();check(std::count(ended.begin(),ended.end(),casting::effect_remove(target,effect))==1);
  }
 }
 std::cout<<"PASS "<<cases<<" staged paid Reversal cases: self/player/NPC, cost/timing, duration, lifetime, cancel/interrupt, conversion, expiry, disabled production gate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
