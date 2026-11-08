#include "gw_world_simulation.h"
#include "gw_enchantment_duration.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Enchantment duration line "+std::to_string(at.line()));
}
inventory::Profile profile(unsigned percent=20){
 inventory::Profile p;p.bags.emplace(1,inventory::Bag{1,1,0,20});p.bags.emplace(2,inventory::Bag{2,2,21,9});
 for(unsigned id:{100u,101u,102u,103u}){inventory::Item item;item.definition.id=id;item.definition.type=id==103?24:26;
  item.definition.quantity=1;item.definition.modifiers={0x22b80000u|(id==100?percent:10u)};
  item.bag=id==101?2:1;item.slot=id==101?0:static_cast<std::uint8_t>(id-100);
  p.items.emplace(id,item);
 }
 p.items.at(100).definition.interaction=0x200000;p.weapons.sets[0]={100,103};p.weapons.sets[1]={102,0};return p;
}
int main()try{
 auto p=profile();auto equipped=items::weapon_passives::equipped_enchanting(p,7);
 check(equipped&&equipped->item()==100&&equipped->percent()==20&&equipped->revision()==7);
 p.weapons.active=1;check(items::weapon_passives::equipped_enchanting(p,8)->percent()==10);
 p.weapons.active=3;check(items::weapon_passives::equipped_enchanting(p,9)->percent()==0); // Physical slot101 does not grant an active bonus.
 for(unsigned mode=0;mode<7;++mode){auto bad=profile();
  if(mode==0)bad.weapons.sets[0].weapon=999;
  if(mode==1)bad.items.at(100).definition.quantity=2;
  if(mode==2)bad.items.at(100).definition.type=29;
  if(mode==3)bad.items.at(100).definition.modifiers={0x22b80014,0x22b80014};
  if(mode==4)bad.items.at(100).definition.id=999;
  if(mode==5)bad.items.at(100).bag=999;
  if(mode==6)bad.weapons.active=4;
  check(!items::weapon_passives::equipped_enchanting(bad,7));
 }
 const Scope scope{1,1,1};
 for(const auto [base,percent,expected]:std::vector<std::tuple<Time,unsigned,Time>>{{1000,10,2000},{5000,10,6000},{10000,15,12000},{10000,20,12000},{15000,10,17000}}){
  const auto equipment=*items::weapon_passives::equipped_enchanting(profile(percent),7);
  const auto duration=enchanting_duration(1392,base,scope,equipment,{},1000);
  check(duration&&duration->for_enchantment(1392,base,scope,1000)==expected);
  check(!duration->for_enchantment(1392,base,scope,1001));
  check(!duration->for_enchantment(1392,base,{1,1,2},1000));
  check(!duration->for_enchantment(258,base,scope,1000));
 }
 const auto equipment=*items::weapon_passives::equipped_enchanting(profile(),7);
 check(!enchanting_duration(1392,12500,scope,equipment,{},1000));
 check(!enchanting_duration(194,10000,scope,equipment,{},1000));
 check(!enchanting_duration(1392,10000,scope,equipment,{},UINT64_MAX));
 const std::array<Entry,1> aura{{{1,1,256,12,Category::enchantment,100,{}}}};
 check(!enchanting_duration(1392,10000,scope,equipment,aura,1000));
 check(bool(enchanting_duration(119,10000,scope,equipment,aura,1000))); // Aura extends Monk spells,not Necromancer spells.
 for(bool npc:{false,true})for(bool early:{false,true}){
  if(npc&&early)continue; // Hostile-removal authority for an allied NPC is a separate path.
  world::Instance w;
  if(npc){world::Instance::Npc body;body.id=body.definition.id=100;body.x=100;body.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{500,1000};d.damageable=d.conditions_allowed=true;
   body.combat_definition=d;w.configure_npcs({body});}
  auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{id==2?500u:1000u,1000,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  const auto caster=*w.effect_scope(1),target=*w.effect_scope(npc?100:2),foe=*w.effect_scope(3);
  const auto quote=*enchanting_duration(1392,10000,caster,equipment,{},1000);const auto plan=*watchful_healing(12);
  check(!w.stage_watchful_healing(caster,target,plan,0,1001,nullptr,nullptr,&quote));
  box->drain();check(w.stage_watchful_healing(caster,target,plan,0,1000,nullptr,nullptr,&quote));
  const auto packets=box->drain();check(std::any_of(packets.begin(),packets.end(),[](const auto& packet){return packet.size()==20&&packet[0]==65&&packet[18]==0x40&&packet[19]==0x41;}));
  auto changed=profile();changed.weapons.active=3;check(items::weapon_passives::equipped_enchanting(changed,8)->percent()==0);
  w.tick(11000);check(npc?w.npc_combat_snapshot(target)->health->current==560:w.resources(2)->health==560);
  if(early){check(w.enchantment_removal(foe,target,*enchantment_removal(68,0),20,12000));check(w.resources(2)->health==668);}
  w.tick(13000);check(npc?w.npc_combat_snapshot(target)->health->current==572:w.resources(2)->health==(early?668u:572u));
 }
 {world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);const auto plan=*skills::regeneration::reviewed(288,12);
  const auto quote=*enchanting_duration(288,plan.duration,caster,equipment,{},1000);
  check(w.protective_enchantment(caster,target,plan,0,1000,&quote));w.tick(16000);check(w.resources(2)->health==740);
  w.tick(20000);check(w.resources(2)->health==788);
 }
 {world::Instance w;w.join(1,"Actor",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);
  const auto caster=*w.effect_scope(1);const auto plan=*skills::regeneration::reviewed(115,12);
  check(w.status_mutation(caster,256,777,12,100,100000,true));
  const auto active=*w.status_snapshot(caster,1000);const auto quote=*enchanting_duration(115,plan.duration,caster,equipment,active,1000);
  check(w.apply_health_regeneration(caster,1,plan,1000,9000,1248,true,{},0,nullptr,nullptr,&quote));
  w.tick(8000);check(w.resources(1)->health==570);w.tick(9000);check(w.resources(1)->health==740);
 }
 std::cout<<"PASS active owned enchanting source,rounding/minimum,scope/time receipts and World extended regeneration/expiry/early end\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
