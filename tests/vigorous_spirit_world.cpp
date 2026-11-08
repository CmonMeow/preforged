#include "gw_world_simulation.h"
#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){if(!value)throw std::runtime_error("Vigorous World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank=0;rank<=20;++rank)for(bool ally:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u}){w.join(id,"Actor",float(id*50),0,id==1?box:std::make_shared<world::Outbox>(),0,{world::Packet{165,0,static_cast<std::uint8_t>(id),0,0,0,3,0}}, {},100,1);w.set_vitals(id,{500,1000,100,100},100);}
  if(ally)check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  const auto source=*w.effect_scope(1),target=*w.effect_scope(ally?2:1);const auto plan=*vigorous_spirit(rank);box->drain();
  check(w.complete_vigorous_spirit(source,target,plan,0,1000));
  auto active=*w.status_snapshot(target,1000);check(active.size()==1&&active[0].skill==254&&active[0].cause==1&&active[0].expires==31000);
  const auto wire=box->drain();const auto body=ally?*skills::particles::targeted_body_2022(2,1,254):*skills::particles::target_body_2022(1,254);
  check(std::count(wire.begin(),wire.end(),body)==1&&std::count(wire.begin(),wire.end(),combat::detail::integer(58,1,0))==1);
  VigorousAction attack{target,1,1001};check(w.vigorous_action(attack));check(w.resources(target.actor)->health==500+plan.healing);
  check(w.vigorous_action(attack)&&w.resources(target.actor)->health==500+plan.healing);
  auto forged=attack;forged.skill=1;check(!w.vigorous_action(forged));++forged.actor.generation;check(!w.vigorous_action(forged));
  attack.event=2;check(w.vigorous_action(attack));check(w.resources(target.actor)->health==500+2*plan.healing);
  check(w.vigorous_action({target,1,1001,VigorousActionKind::completed_spell,254}));check(w.resources(target.actor)->health==500+3*plan.healing);
  check(w.vigorous_action({target,1,1001,VigorousActionKind::other_skill,347}));check(w.resources(target.actor)->health==500+3*plan.healing);
  check(w.complete_vigorous_spirit(source,target,plan,0,1002));check(w.vigorous_action(attack));check(w.resources(target.actor)->health==500+3*plan.healing);
  attack.event=3;attack.at=1003;check(w.vigorous_action(attack));check(w.resources(target.actor)->health==500+4*plan.healing);
  auto late=attack;late.event=4;late.at=1002;check(!w.vigorous_action(late));
  w.tick(31002);const auto before=w.resources(target.actor)->health;attack.event=4;attack.at=31002;check(w.vigorous_action(attack)&&w.resources(target.actor)->health==before);
  check(w.kill(target,31003));attack.event=5;attack.at=31004;check(!w.vigorous_action(attack));
 }
 for(unsigned mode=0;mode<5;++mode){
  world::Instance w;w.join(1,"Monk",0,0,std::make_shared<world::Outbox>(),0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);
  const auto original=*w.effect_scope(1);auto source=original,target=original;auto plan=*vigorous_spirit(12);
  if(mode==0)++source.generation;if(mode==1)++target.generation;if(mode==2)++plan.healing;
  if(mode<3)check(!w.complete_vigorous_spirit(source,target,plan,12,1000));
  else{check(w.complete_vigorous_spirit(source,target,plan,12,1000));check(w.resources(1)->health==538);const auto effect=w.status_snapshot(target,1000)->front();
   if(mode==3){w.join(2,"Foe",50,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,1000,100,100},100);
    check(w.enchantment_removal(*w.effect_scope(2),target,*enchantment_removal(69,12),20,1001));check(w.status_snapshot(target,1001)->empty());
    const auto before=w.resources(1)->health;check(w.vigorous_action({target,1,1001})&&w.resources(1)->health==before);
   }else{check(w.kill(target,1001));check(!w.vigorous_action({target,1,1002}));check(w.status_snapshot(target,1002)->empty());}
  }
 }
 {
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Cap",0,0,box,0,{}, {},100,1);w.set_vitals(1,{995,1000,100,100},100);
  const auto owner=*w.effect_scope(1);check(w.complete_vigorous_spirit(owner,owner,*vigorous_spirit(12),0,1000));box->drain();
  check(w.vigorous_action({owner,1,1001}));check(w.resources(1)->health==1000);const auto wire=box->drain();
  check(std::count(wire.begin(),wire.end(),skills::particles::health_change_2022(1,1,5,1000))==1);
  check(w.vigorous_action({owner,2,1001})&&w.resources(1)->health==1000);const auto full=box->drain();
  check(std::none_of(full.begin(),full.end(),[](const auto& p){return p.size()>2&&p[0]==162&&p[1]==0;}));
 }
 {
  world::Instance w;w.join(1,"Duration",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);const auto owner=*w.effect_scope(1);const auto plan=*vigorous_spirit(12);
  inventory::Profile inventory;inventory.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item item;item.definition.id=100;item.definition.type=26;item.definition.quantity=1;item.definition.modifiers={0x22b80014};item.bag=1;inventory.items.emplace(100,item);inventory.weapons.sets[0].weapon=100;
  const auto equipment=items::weapon_passives::equipped_enchanting(inventory,1);check(equipment.has_value());
  const auto quote=enchanting_duration(254,plan.duration,owner,*equipment,*w.status_snapshot(owner,1000),1000);check(quote.has_value());
  check(w.complete_vigorous_spirit(owner,owner,plan,0,1000,&*quote));check(w.status_snapshot(owner,35000)->front().expires==37000);
  check(!w.complete_vigorous_spirit(owner,owner,plan,0,35000,&*quote));check(w.status_snapshot(owner,37000)->empty());
 }
 const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 80 0 0 - -\n");auto population=world::load_npc_catalog(appearance);
 std::istringstream metadata("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/vigorous_spirit_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 100 - 1 0 test test - - - - - - -\n");world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
 for(bool allowed:{false,true}){
  auto defs=population.for_map(146);auto& d=*defs.front().combat_definition;d.relation=combat::npcs::Relation::allied_to_players;d.healing_allowed=allowed;d.healing_evidence="authored fixture";
  world::Instance w;w.configure_npcs(defs);w.join(1,"Monk",0,0,std::make_shared<world::Outbox>(),0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);check(w.complete_vigorous_spirit(source,target,*vigorous_spirit(12),0,1000));
  check(w.vigorous_action({target,1,1001}));check(w.npc_combat_snapshot(target)->health->current==(allowed?117:100));
  check(w.vigorous_action({target,1,1001}));check(w.npc_combat_snapshot(target)->health->current==(allowed?117:100));
  check(!w.vigorous_action({target,2,999}));
  w.tick(31000);const auto before=w.npc_combat_snapshot(target)->health->current;check(w.vigorous_action({target,2,31000}));check(w.npc_combat_snapshot(target)->health->current==before);
 }
 std::cout<<"PASS staged Vigorous World player/NPC lifecycle,DF,healing,receipt replay,expiry/strip/death and clock rejection;normal producers/casting pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
