#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Energy enchantment Renewal line "+std::to_string(at.line())+" "+context);
}
struct Fixture {
 world::Instance w;Scope caster,target;inventory::Profile gear;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool npc,unsigned version,unsigned health=500,bool known=true){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{1000,1000};
   d.damageable=d.conditions_allowed=true;d.spawn_energy=combat::npcs::Energy{0,100};if(known)d.energy_pips=2;
   n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id:{1u,2u}){w.join(id,"Actor",float(id*10),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{id==1?health:1000,1000,id==1?50u:0u,100},100);}
  caster=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);
  if(version){const auto p=*ether_renewal(version,0);check(w.status_mutation(caster,version,80,0,100,100+p.duration,true));}
  gear.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item item;item.definition.id=50;item.definition.type=26;
  item.definition.quantity=1;item.definition.modifiers={0x22b80014};item.bag=1;gear.items.emplace(50,item);gear.weapons.sets[0].weapon=50;box->drain();
 }
 std::optional<EnchantmentDuration> quote(unsigned skill,std::uint64_t at,bool wrong=false){
  auto source=caster;if(wrong)++source.generation;const auto equipment=items::weapon_passives::equipped_enchanting(gear,1);
  const auto active=w.status_snapshot(caster,at);const auto plan=energy_enchantment(skill,0);
  return equipment&&active&&plan?enchanting_duration(skill,plan->duration,source,*equipment,*active,at):std::nullopt;
 }
 bool complete(Scope source,const EnergyEnchantmentPlan& plan,std::uint64_t event,std::uint64_t at,
               std::optional<sacrifice::Loss>& paid,const EnchantmentDuration* quote,
               std::vector<combat::Packet>* visuals=nullptr,std::function<void()>* publication=nullptr){
  return w.energy_enchantment(source,target,plan,at,&paid,visuals,quote,event,publication);
 }
};
int main()try{unsigned direct=0,paid_cases=0;
 for(unsigned skill:{119u,157u})for(bool npc:{false,true})for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<8;++mode){
  context="direct skill="+std::to_string(skill)+" npc="+std::to_string(npc)+" version="+std::to_string(version)+" mode="+std::to_string(mode);
  Fixture f(npc,version,mode==6?2:mode==7?800:500);const auto at=mode==5&&version?100+ether_renewal(version,0)->duration:200;
  f.w.tick(at);check(f.w.before_spell_effect(f.caster,skill,1,at));auto quote=f.quote(skill,mode==4?at-1:at,mode==3);check(bool(quote));
  const auto before=*f.w.resources(1);f.box->drain();auto source=f.caster;if(mode==2)++source.generation;
  std::optional<sacrifice::Loss> paid;bool ok=f.complete(source,*energy_enchantment(skill,0),mode==1?2:1,at,paid,&*quote);
  if(mode>=1&&mode<=4){check(!ok&&!paid&&f.box->drain().empty());check(f.w.resources(1)->health==before.health&&f.w.resources(1)->energy==before.energy);
   quote=f.quote(skill,at);ok=f.complete(f.caster,*energy_enchantment(skill,0),1,at,paid,&*quote);}
  check(ok&&paid);const bool reward=version&&mode!=5;const unsigned health_cost=skill==119?330:170;
  check(f.w.resources(1)->health==unsigned(std::max(0,int(before.health)+(reward?5:0)-int(health_cost))));
  check(f.w.resources(1)->energy==before.energy+(reward?1:0));check(paid->killed==(mode==6));
  const auto active=*f.w.status_snapshot(f.target,at);const auto duration=skill==119?12000u:10000u;
  check(active.size()==1&&active.front().skill==skill&&active.front().expires==at+duration);
  f.box->drain();check(!f.complete(f.caster,*energy_enchantment(skill,0),1,at,paid,&*quote)&&!paid&&f.box->drain().empty());
  if(mode==7){f.w.tick(at+100);check(f.w.before_spell_effect(f.caster,skill,2,at+100));quote=f.quote(skill,at+100);const auto previous=*f.w.resources(1);
   check(f.complete(f.caster,*energy_enchantment(skill,0),2,at+100,paid,&*quote)&&paid);
   check(f.w.resources(1)->health==previous.health+(version?5:0)-health_cost);
   check(f.w.status_snapshot(f.target,at+100)->size()==1);}
  if(mode==6){const auto energy=npc?f.w.npc_energy_snapshot(f.target)->current:f.w.resources(2)->energy;
   f.w.tick(at+3000);check((npc?f.w.npc_energy_snapshot(f.target)->current:f.w.resources(2)->energy)==energy+5);}
  ++direct;
 }
 for(unsigned skill:{119u,157u})for(bool npc:{false,true})for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<10;++mode){
  context="paid skill="+std::to_string(skill)+" npc="+std::to_string(npc)+" version="+std::to_string(version)+" mode="+std::to_string(mode);
  Fixture f(npc,version);casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0,before_health=0,before_energy=0,published=0;std::uint64_t admitted=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& packets){f.box->enqueue(packets);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};
  h.consume_energy_at=[&](auto n,auto at){return f.w.consume_energy(1,n,f.caster,at);};
  h.energy_enchantment_target_valid=[&](auto id,const auto& plan,auto at){return f.w.valid_energy_enchantment_target(f.caster,id,plan,at);};
  h.enchantment_duration_ready=[&](auto used,auto base,auto at){check(used==skill&&base==energy_enchantment(skill,0)->duration);return bool(f.quote(skill,at));};
  h.before_spell_effect=[&](auto used,auto event,auto at){check(used==skill);f.w.tick(at);admitted=event;before_health=f.w.resources(1)->health;before_energy=f.w.resources(1)->energy;return f.w.before_spell_effect(f.caster,used,event,at);};
  h.energy_enchantment=[](auto,const auto&,auto)->casting::EffectCompletion{throw std::runtime_error("typed Energy enchantment downgraded");};
  h.health_sacrifice=[](auto,auto)->std::optional<sacrifice::Loss>{throw std::runtime_error("sacrifice charged twice");};
  h.energy_enchantment_completed=[&](auto target,const auto& plan,auto event,auto at)->casting::EffectCompletion{
   check(target==f.target&&event==admitted&&event);++calls;const auto quote=f.quote(skill,at);check(bool(quote));
   std::optional<sacrifice::Loss> paid;std::vector<combat::Packet> visuals;std::function<void()> publication;
   const bool ok=f.complete(f.caster,plan,event,at,paid,&*quote,&visuals,&publication);
   if(publication){auto native=std::move(publication);publication=[&,native=std::move(native)]()mutable{++published;native();};}
   return {ok,std::move(visuals),paid,std::move(publication),true};
  };
  Policy policy;policy.primary=4;policy.secondary=6;policy.campaigns=255;policy.pvp_match=mode%2;policy.learned[skill]=1;
  Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};attributes::Ranks ranks{};
  if(mode==8)h.before_spell_effect={};if(mode==9)h.enchantment_duration_ready={};
  check(runtime.start(100,{skill,0,f.target.actor},bar,policy,ranks,true,h)==(mode>=8?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode>=8){check(calls==0&&f.w.resources(1)->health==500&&f.w.resources(1)->energy==50);++paid_cases;continue;}
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(101,h);if(mode==3)h.energy_enchantment_completed={};
  if(mode==4)h.before_spell_effect={};if(mode==5)h.enchantment_duration_ready={};if(mode==6)h.energy_enchantment_target_valid={};
  if(mode==7){f.w.leave(1);f.w.join(1,"Replacement",0,0,f.box,0,{}, {},200,1);f.w.set_vitals(1,{500,1000,40,100},100);}
  const auto at=100+std::uint64_t(find_skill(skill)->activation*1000);f.box->drain();runtime.update(at,h);const auto packets=f.box->drain();runtime.update(at+100,h);
  check(calls==unsigned(mode==0)&&published==unsigned(mode==0));
  if(mode==0){check(f.w.resources(1)->health==before_health+(version?5:0)-(skill==119?330:170));check(f.w.resources(1)->energy==before_energy+(version?1:0));
   const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
   const auto body=skill==119?*particles::targeted_body_2022(f.target.actor,1,119):combat::detail::integer(21,1,281);
   const auto visual=std::find(packets.begin(),packets.end(),body);check(finish<visual&&visual!=packets.end()&&std::count(packets.begin(),packets.end(),body)==1);
   if(version){const auto heal=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=6&&p[0]==162&&p[2]==55;});check(heal<finish);}
   if(!npc){const auto status=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>1&&p[0]==65&&p[1]==0;});check(visual<status&&status!=packets.end());}
   const auto energy=npc?f.w.npc_energy_snapshot(f.target)->current:f.w.resources(2)->energy;
   f.w.tick(at+3000);check((npc?f.w.npc_energy_snapshot(f.target)->current:f.w.resources(2)->energy)==energy+5);
  }
  ++paid_cases;
 }
 for(unsigned skill:{119u,157u}){
  Fixture f(true,181,500,false);f.w.tick(200);check(f.w.before_spell_effect(f.caster,skill,1,200));const auto quote=f.quote(skill,200);check(bool(quote));
  const auto before=*f.w.resources(1);std::optional<sacrifice::Loss> paid;
  check(!f.complete(f.caster,*energy_enchantment(skill,0),1,200,paid,&*quote)&&!paid);
  check(f.w.resources(1)->health==before.health&&f.w.resources(1)->energy==before.energy);
 }
 for(unsigned skill:{119u,157u})for(bool npc:{false,true}){
  context="future clock skill="+std::to_string(skill)+" npc="+std::to_string(npc);Fixture f(npc,181);
  f.w.tick(200);check(f.w.before_spell_effect(f.caster,skill,1,200));const auto old=f.quote(skill,200);check(bool(old));
  f.w.tick(300);const auto before=*f.w.resources(1);std::optional<sacrifice::Loss> paid;
  check(!f.complete(f.caster,*energy_enchantment(skill,0),1,200,paid,&*old)&&!paid);
  check(f.w.resources(1)->health==before.health&&f.w.resources(1)->energy==before.energy);
  const auto fresh=f.quote(skill,300);check(bool(fresh));check(f.complete(f.caster,*energy_enchantment(skill,0),1,300,paid,&*fresh)&&paid);
  check(f.w.resources(1)->health==before.health+5-(skill==119?330:170)&&f.w.resources(1)->energy==before.energy+1);
 }
 for(bool npc:{false,true}){
  context="zero sacrifice npc="+std::to_string(npc);Fixture f(npc,181);f.w.set_vitals(1,{1,1,50,100},100);
  check(f.w.before_spell_effect(f.caster,119,1,100));const auto quote=f.quote(119,100);check(bool(quote));std::optional<sacrifice::Loss> paid;
  check(f.complete(f.caster,*energy_enchantment(119,0),1,100,paid,&*quote)&&paid&&!paid->killed&&paid->lost==0);
  check(f.w.resources(1)->health==1&&f.w.resources(1)->energy==51&&f.w.status_snapshot(f.target,100)->size()==1);
 }
 std::cout<<"PASS "<<direct<<" Energy enchantment reward/sacrifice/receipt/replay/replacement cases and "<<paid_cases<<" paid completion/order/cancel/hook/lifetime cases;2 unknown-NPC refusals,4 clock retries,2 zero-sacrifice cases and actual recipient Energy gain\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
