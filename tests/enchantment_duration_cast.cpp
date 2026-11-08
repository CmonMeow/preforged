#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Enchantment duration cast line "+std::to_string(at.line()));
}
inventory::Profile profile(){
 inventory::Profile p;p.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item item;
 item.definition.id=100;item.definition.type=26;item.definition.quantity=1;item.definition.modifiers={0x22b80014};
 item.bag=1;p.items.emplace(100,item);p.weapons.sets[0].weapon=100;return p;
}
int main()try{
 for(unsigned skill:{119u,157u,258u,288u,446u})for(unsigned mode=0;mode<7;++mode){
  const bool energy=skill==119||skill==157,enchant=skill!=446;const unsigned primary=energy?4:enchant?3:2,attribute=find_skill(skill)->attribute;
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);
  if(energy){w.join(2,"Ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{1000,1000,0,100},100);}
  const auto scope=*w.effect_scope(1);auto inventory=profile();std::uint64_t revision=1;
  const auto target=energy?*w.effect_scope(2):scope;
  unsigned checks=0,applied=0,before=0,sacrifices=0;std::vector<world::Packet> sent;
  if(mode==1)inventory.items.at(100).definition.modifiers={0x22b800ff};
  if(mode==5)check(w.status_mutation(scope,256,777,12,100,100000,true));
  const auto quote=[&](const auto& plan,std::uint64_t at)->std::optional<effects::EnchantmentDuration>{
   const auto equipment=items::weapon_passives::equipped_enchanting(inventory,revision);
   const auto active=w.status_snapshot(scope,at);if(!equipment||!active)return {};
   return effects::enchanting_duration(plan.skill,plan.duration,scope,*equipment,*active,at);
  };
  casting::Hooks h;h.resources=[&](auto id){return w.resources(id);};h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[&](auto id,float range,bool self){return w.valid_heal_target(1,id,range,self);};
  h.consume_energy_at=[&](auto amount,auto at){return w.consume_energy(1,amount,scope,at);};
  h.before_spell_effect=[&](auto used,auto event,auto at){++before;return w.before_spell_effect(scope,used,event,at);};
  h.enchantment_duration_ready=[&](unsigned used,auto base,auto at){++checks;
   if(energy){const auto plan=effects::energy_enchantment(used,12);check(plan&&plan->duration==base);return bool(quote(*plan,at));}
   const auto plan=regeneration::reviewed(used,12);check(plan&&plan->duration==base);return bool(quote(*plan,at));};
  h.protective_enchantment=[&](auto target,const auto& plan,unsigned divine,auto at){
   ++applied;const auto duration=quote(plan,at);return duration&&w.protective_enchantment(scope,target,plan,divine,at,&*duration);
  };
  h.apply_health_regeneration=[&](auto target,const auto& plan,auto at,auto expires,float range,bool self){
   ++applied;return w.apply_health_regeneration(scope,target,plan,at,expires,range,self);
  };
  h.energy_enchantment_target_valid=[&](auto id,const auto& plan,auto at){return w.valid_energy_enchantment_target(scope,id,plan,at);};
  h.energy_enchantment=[&](auto destination,const auto& plan,auto at)->casting::EffectCompletion{
   ++applied;std::optional<sacrifice::Loss> paid;std::vector<world::Packet> visuals;const auto duration=quote(plan,at);
   const bool ok=duration&&w.energy_enchantment(scope,destination,plan,at,&paid,&visuals,&*duration);
   return {ok,std::move(visuals),paid};
  };
  h.health_sacrifice=[&](auto,auto)->std::optional<sacrifice::Loss>{++sacrifices;return {};};
  if(mode==6)h.enchantment_duration_ready={}; // Legacy caller did not declare this capability.
  Policy policy;policy.primary=primary;policy.campaigns=255;policy.learned[skill]=1;
  Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[attribute]=12;casting::Runtime cast(1);
  const auto reason=cast.start(1000,{skill,0,target.actor},bar,policy,ranks,true,h);
  const bool rejected=enchant&&(mode==1||(mode==5&&!energy));
  check(reason==(rejected?casting::Reason::unsupported:casting::Reason::accepted));
  check(checks==(enchant&&mode!=6?1u:0u));
  if(rejected){check(w.resources(1)->energy==100&&applied==0&&before==0&&!cast.active());continue;}
  check(w.resources(1)->energy==100-find_skill(skill)->energy_cost());
  if(mode==2)inventory.items.at(100).definition.modifiers={0x22b800ff};
  if(mode==3)h.enchantment_duration_ready={};
  if(mode==4){inventory.items.at(100).definition.modifiers={0x22b8000a};++revision;}
  const auto complete=1000+static_cast<std::uint64_t>(find_skill(skill)->activation*1000);
  cast.update(complete,h);check(!cast.active());
  check(sacrifices==0);
  const bool failed=enchant&&(mode==2||mode==3);
  check(applied==unsigned(!failed));
  if(failed){check(before==0&&std::none_of(sent.begin(),sent.end(),[](const auto& packet){return packet==casting::terminal(58,1);}));continue;}
  const auto active=w.status_snapshot(target,complete);check(bool(active));
  const auto entry=std::find_if(active->begin(),active->end(),[&](const auto& e){return e.skill==skill;});check(entry!=active->end());
  const auto plan=regeneration::reviewed(skill,12);const auto energy_plan=effects::energy_enchantment(skill,12);
  const auto base=energy?energy_plan->duration:plan->duration;auto duration=base;
  if(enchant){const auto receipt=energy?*quote(*energy_plan,complete):*quote(*plan,complete);duration=*receipt.for_enchantment(skill,base,scope,complete);check(receipt.revision()==revision);}
  check(entry->expires==complete+duration);
  if(skill==288)check(duration==(mode==4?17000u:18000u)); // Requote changed equipment at completion.
  if(energy)check(w.resources(1)->health==(skill==119?170u:330u));
  if(!enchant)check(checks==0&&duration==13000);
 }
 std::cout<<"PASS35duration casting cases:prepayment rejection,completion capability/state loss,fresh weapon quote,Energy enchantments,legacy and non-enchantment isolation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
