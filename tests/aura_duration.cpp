#include "gw_skill_casting.h"
#include "gw_character_skills.h"
#include "gw_enchantment_duration.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Aura duration line "+std::to_string(at.line()));}
int main()try{
 for(unsigned scenario=0;scenario<18;++scenario){
  const auto mode=scenario%6;const bool pvp=scenario>=6;const unsigned aura_skill=scenario>=12?3375u:180u;const unsigned expected_heal=pvp?15u:18u;
  Slots slots{};slots[0]={aura_skill,0};slots[1]={281,0};Policy policy;policy.primary=6;policy.secondary=3;policy.campaigns=0x1e;policy.learned[aura_skill]=policy.learned[281]=1;policy.pvp_match=pvp;
  attributes::Ranks ranks{};ranks[12]=8;vitals::Resources resource{100,480,100,100};
  effects::Scope scope{1,42,1};effects::Registry registry(scope);casting::Hooks h;unsigned heals=0,returns=0,checks=0;std::vector<combat::Packet> sent;
  inventory::Profile inventory;inventory.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item item;
  item.definition.id=100;item.definition.type=26;item.definition.quantity=1;item.definition.modifiers={0x22b80014};item.bag=1;inventory.items.emplace(100,item);inventory.weapons.sets[0].weapon=100;
  if(mode==1)inventory.items.at(100).definition.modifiers={0x22b800ff};
  h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};h.target_valid=[](auto,float,bool){return true;};
  h.resources=[&](auto)->std::optional<vitals::Resources>{return resource;};h.consume_energy=[&](unsigned value){resource.energy-=value;return true;};
  h.heal=[&](auto actor,unsigned amount,float,bool){if(actor==42)heals+=amount;return true;};h.restore_energy=[&](unsigned amount){returns+=amount;return true;};
  h.status_mutation=[&](unsigned skill,unsigned id,unsigned rank,auto now,auto expires,bool active){if(active)return registry.apply(scope,{id,42,skill,rank,effects::category(*find_skill(skill)),now,expires},effects::Replace::same_skill).accepted;registry.remove(scope,id);return true;};
  h.effect_snapshot=[&](auto now)->std::optional<std::vector<effects::Entry>>{return registry.active(scope,now);};
  h.local_enchantment_duration=[&](unsigned skill,auto base,auto now)->std::optional<std::uint64_t>{++checks;
   auto equipment=items::weapon_passives::equipped_enchanting(inventory,1);if(!equipment)return {};
   auto active=registry.active(scope,now);auto quote=effects::enchanting_duration(skill,base,scope,*equipment,active,now);
   return quote?quote->for_enchantment(skill,base,scope,now):std::nullopt;};
  casting::Runtime cast(42);const auto admitted=cast.start(0,{aura_skill,0,42},slots,policy,ranks,true,h);if(admitted!=(mode==1?casting::Reason::unsupported:casting::Reason::accepted))std::cerr<<"mode="<<mode<<" reason="<<unsigned(admitted)<<" checks="<<checks<<"\n";check(admitted==(mode==1?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==1){check(resource.energy==100&&!cast.active());continue;}
  if(mode==2)h.local_enchantment_duration={};
  if(mode==3)inventory.items.at(100).definition.modifiers={0x22b800ff};
  if(mode==4)inventory.items.at(100).definition.modifiers={0x22b8000a};
  cast.update(250,h);
  if(mode==2||mode==3){check(!registry.has(scope,effects::Category::enchantment,250));check(std::find(sent.begin(),sent.end(),casting::terminal(58,42))==sent.end());continue;}
  const auto duration=mode==4?66000u:72000u;auto active=registry.active(scope,250);check(active.size()==1&&active[0].expires==250+duration);
  check(std::find(sent.begin(),sent.end(),casting::effect_add(42,pvp?3375:180,8,active[0].instance,float(duration)/1000.f))!=sent.end());
  // Changing gear afterwards cannot shorten an already committed enchantment.
  inventory.items.at(100).definition.modifiers.clear();
  check(cast.start(61000,{281,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted);cast.update(62000,h);check(heals==expected_heal&&returns==1);
  if(mode==5){registry.remove(scope,active[0].instance);cast.update(63000,h);}
  check(cast.start(250+duration,{281,0,43},slots,policy,ranks,true,h)==casting::Reason::accepted);cast.update(1250+duration,h);check(heals==expected_heal&&returns==1);
 }
 {
  Profile saved;saved.primary=6;saved.secondary=3;saved.learned[180]=1;saved.bar[0]={180,0};
  const auto match=match_profile(saved,true);
  check(match.bar[0].skill==3375&&match.learned.at(3375)==1&&saved.bar[0].skill==180&&!saved.learned.contains(3375));
  check(match_profile(saved,false).bar[0].skill==180);
  Policy policy;policy.primary=6;policy.campaigns=0x1e;policy.learned[3375]=1;
  Slots slots{};slots[0]={3375,0};attributes::Ranks ranks{};ranks[12]=8;
  casting::Hooks h;unsigned payments=0;h.events=[](const auto&){};
  h.resources=[](auto)->std::optional<vitals::Resources>{return vitals::Resources{100,480,100,100};};
  h.consume_energy=[&](unsigned){++payments;return true;};h.heal=[](auto,unsigned,float,bool){return true;};h.restore_energy=[](unsigned){return true;};
  casting::Runtime cast(42);
  check(cast.start(0,{3375,0,42},slots,policy,ranks,true,h)==casting::Reason::unsupported&&payments==0);
 }
 std::cout<<"PASS Aura PvE/PvP/base-alias equipment duration: wire/world/local expiry, fresh completion quote, malformed/lost capability, postcast swap and removal\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
