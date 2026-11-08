#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include "gw_enchantment_duration.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Local enchantment duration line "+std::to_string(at.line()));}
inventory::Profile equipped_profile(){inventory::Profile p;p.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item item;item.definition.id=100;item.definition.type=26;item.definition.quantity=1;item.definition.modifiers={0x22b80014};item.bag=1;p.items.emplace(100,item);p.weapons.sets[0].weapon=100;return p;}
int main()try{
 unsigned cases=0;
 for(unsigned skill:{38u,169u,184u,208u,225u,164u})for(unsigned mode=0;mode<8;++mode){++cases;
  world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{500,500,100,100},100);w.set_vitals(2,{500,500,100,100},100);const auto scope=*w.effect_scope(1);
  auto inventory=equipped_profile();std::uint64_t revision=1,clock=1000;unsigned checks=0,refunds=0;std::vector<combat::Packet> sent;
  if(mode==1)inventory.items.at(100).definition.modifiers={0x22b800ff};
  const auto quote=[&](unsigned used,std::uint64_t base,std::uint64_t at)->std::optional<std::uint64_t>{++checks;
   const auto owned=items::weapon_passives::equipped_enchanting(inventory,revision);const auto active=w.status_snapshot(scope,at);if(!owned||!active)return {};
   const auto duration=effects::enchanting_duration(used,base,scope,*owned,*active,at);return duration?duration->for_enchantment(used,base,scope,at):std::nullopt;
  };
  casting::Hooks h;h.resources=[&](auto id){return w.resources(id);};h.target_valid=[&](auto id,float,bool){return id==1;};
  h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};
  h.consume_energy_at=[&](auto amount,auto at){return w.consume_energy(1,amount,scope,at);};
  h.restore_energy=[&](unsigned amount){refunds+=amount;return w.restore_energy(1,amount,scope,clock);};
  h.status_mutation=[&](auto used,auto id,auto rank,auto at,auto expires,bool active){return w.status_mutation(scope,used,id,rank,at,expires,active);};
  h.effect_snapshot=[&](auto at){return w.status_snapshot(scope,at);};h.local_enchantment_duration=quote;
  h.prepare_channeling=[&](auto used,auto event,auto at){return w.prepare_channeling(scope,used,event,at);};
  h.complete_channeling=[&](auto event,bool ok,auto at){return w.complete_channeling(scope,event,ok,at);};
  if(mode==7)h.local_enchantment_duration={};
  Policy policy;policy.primary=skill==38?5:6;policy.secondary=3;policy.campaigns=0x1e;policy.learned[skill]=1;Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[find_skill(skill)->attribute]=12;
  casting::Runtime cast(1);const auto native=*casting::local_enchantment_base(skill,12);
  check(cast.start(clock,{skill,0,1},bar,policy,ranks,true,h)==(mode==1?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==1){check(w.resources(1)->energy==100&&!cast.active());continue;}
  if(mode==2)h.local_enchantment_duration={};if(mode==3)inventory.items.at(100).definition.modifiers={0x22b800ff};
  if(mode==4){inventory.items.at(100).definition.modifiers={0x22b8000a};++revision;}
  clock=1000+static_cast<std::uint64_t>(find_skill(skill)->activation*1000);cast.update(clock,h);check(!cast.active());
  if(mode==2||mode==3){check(!w.has_enchantment(scope,1,clock));check(std::find(sent.begin(),sent.end(),casting::terminal(58,1))==sent.end());continue;}
  auto active=*w.status_snapshot(scope,clock);check(active.size()==1&&active[0].skill==skill);
  const auto duration=mode==7?native:*quote(skill,native,clock);const auto expiry=clock+duration,old_native_expiry=clock+native;
  check(active[0].expires==expiry);check(std::find(sent.begin(),sent.end(),casting::effect_add(1,skill,12,active[0].instance,float(duration)/1000.f))!=sent.end());
  if(mode==7){check(checks==0);cast.update(expiry,h);check(!w.has_enchantment(scope,1,expiry));continue;}
  // Already committed duration survives a later weapon change.
  inventory.items.at(100).definition.modifiers={0x22b8000a};++revision;clock=old_native_expiry+1000;cast.update(clock,h);
  check(w.has_enchantment(scope,1,clock));
  if(mode==6){const auto old_id=active[0].instance;check(cast.start(clock,{skill,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);
   clock+=static_cast<std::uint64_t>(find_skill(skill)->activation*1000);cast.update(clock,h);const auto renewed=*w.status_snapshot(scope,clock);
   check(renewed.size()==1&&renewed[0].instance!=old_id&&renewed[0].expires==clock+*quote(skill,native,clock));
   check(std::find(sent.begin(),sent.end(),casting::effect_remove(1,old_id))!=sent.end());continue;
  }
  if(skill==38){check(w.consume_energy(1,30,scope,clock));const auto before=w.resources(1)->energy;
   check(w.prepare_channeling(scope,281,100,clock)&&w.complete_channeling(scope,100,true,clock));check(w.resources(1)->energy==before+1);
  }else{const auto snapshot=*w.status_snapshot(scope,clock);const auto probe=skill==164?174u:skill;
   check(effects::attunement_cast_refund(snapshot,*find_skill(probe),clock)>0);if(skill==164)check(effects::elemental_attribute_bonus(snapshot,clock)==2);
  }
  if(mode==5){check(w.status_mutation(scope,skill,active[0].instance,12,clock,{},false));cast.update(clock,h);}
  clock=expiry;cast.update(clock,h);check(!w.has_enchantment(scope,1,clock));
  const auto snapshot=*w.status_snapshot(scope,clock);check(effects::attunement_cast_refund(snapshot,*find_skill(174),clock)==0&&effects::elemental_attribute_bonus(snapshot,clock)==0);
  if(skill==38){check(w.consume_energy(1,30,scope,clock));const auto before=w.resources(1)->energy;check(w.prepare_channeling(scope,281,101,clock)&&w.complete_channeling(scope,101,true,clock));check(w.resources(1)->energy==before);}
 }
 std::cout<<"PASS "<<cases<<" local-enchantment duration cases: Channeling/five attunements, real World Energy return, registry/wire, renewal/removal, equipment changes and failure guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
