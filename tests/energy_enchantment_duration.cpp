#include "gw_world_simulation.h"
#include "gw_enchantment_duration.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Energy enchantment duration line "+std::to_string(at.line()));
}
int main()try{
 inventory::Profile inventory;inventory.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item item;
 item.definition.id=50;item.definition.type=26;item.definition.quantity=1;item.definition.modifiers={0x22b80014};item.bag=1;
 inventory.items.emplace(50,item);inventory.weapons.sets[0].weapon=50;
 const auto equipped=*items::weapon_passives::equipped_enchanting(inventory,7);
 for(unsigned skill:{119u,157u})for(bool npc:{false,true})for(bool dense:{false,true})for(bool renew:{false,true}){
  world::Instance w;
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;
   d.spawn_health=combat::Health{1000,1000};d.spawn_energy=combat::npcs::Energy{0,100};d.energy_pips=2;
   d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});}
  auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},1000,1);w.set_vitals(1,{1000,1000,100,100},100);
  if(!npc){w.join(2,"Ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},1000,1);w.set_vitals(2,{1000,1000,0,100},100);}
  w.tick(1000);const auto caster=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
  check(w.status_mutation(caster,256,777,0,1000,100000,true));
  const auto active=*w.status_snapshot(caster,1000);const auto plan=*energy_enchantment(skill,12);
  const auto quote=enchanting_duration(skill,plan.duration,caster,equipped,active,1000);check(bool(quote));
  const auto duration=*quote->for_enchantment(skill,plan.duration,caster,1000);
  check(duration==(skill==119?12000u:16000u));std::optional<skills::sacrifice::Loss> paid;
  check(!w.energy_enchantment(caster,target,plan,1001,&paid,nullptr,&*quote));check(!paid&&w.resources(1)->health==1000);
  box->drain();check(w.energy_enchantment(caster,target,plan,1000,&paid,nullptr,&*quote));
  check(paid&&paid->lost==(skill==119?330:170)&&w.resources(1)->health==(skill==119?670:830));
  if(!npc){const auto entries=*w.status_snapshot(target,1000);check(entries.size()==1&&entries.front().expires==1000+duration);}
  std::uint64_t started=1000,actual_duration=duration,prior=0;
  if(renew){w.tick(5000);auto changed=inventory;changed.items.at(50).definition.modifiers={0x22b8000a};
   const auto next_equipment=*items::weapon_passives::equipped_enchanting(changed,8);
   const auto next_quote=*enchanting_duration(skill,plan.duration,caster,next_equipment,*w.status_snapshot(caster,5000),5000);
   check(w.energy_enchantment(caster,target,plan,5000,&paid,nullptr,&next_quote));
   check(paid&&paid->lost==(skill==119?330:170));
   started=5000;prior=4000;actual_duration=*next_quote.for_enchantment(skill,plan.duration,caster,5000);
   check(actual_duration==(skill==119?11000u:14000u));
  }
  if(dense)for(auto now=started+100;now<started+plan.duration;now+=100)w.tick(now);
  w.tick(started+plan.duration);
  if(!npc)check(w.overcast_energy_snapshot(target)->curve.effective_energy_pips(2,started+plan.duration)==2+int(plan.pips));
  else check(w.npc_combat_snapshot(target)->effects&0x80);
  if(dense)for(auto now=started+plan.duration+100;now<started+actual_duration;now+=100)w.tick(now);
  w.tick(started+actual_duration);
  const auto expected_units=std::int64_t(2+plan.pips)*std::int64_t(prior+actual_duration)*100;
  if(npc){check(w.npc_precise_energy_snapshot(target)==expected_units);check(!(w.npc_combat_snapshot(target)->effects&0x80));
   w.tick(started+actual_duration+1000);check(w.npc_precise_energy_snapshot(target)==expected_units+200000);
  }else{check(w.resources(2)->energy==unsigned(expected_units/300000));check(w.status_snapshot(target,started+actual_duration)->empty());
   check(w.overcast_energy_snapshot(target)->curve.effective_energy_pips(2,started+actual_duration)==2);}
 }
 std::cout<<"PASS16Energy-enchantment duration cases:player/NPC,dense/coarse,renewed weapon bonus,registry/curve expiry,sacrifice and Monk-only Aura scope\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
