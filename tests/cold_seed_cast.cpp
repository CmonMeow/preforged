// Detached paid cold-skill publication regression; no live-client dependency.
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Cold Seed cast line "+std::to_string(at.line()));
}
unsigned word(const world::Packet& packet,unsigned at){return unsigned(packet[at])|(unsigned(packet[at+1])<<8)|(unsigned(packet[at+2])<<16)|(unsigned(packet[at+3])<<24);}
int main()try{
 const auto seed=*effects::healing_seed(12);
 for(unsigned skill:{209u,2803u,1382u})for(bool npc_target:{false,true})
 for(int eligibility:{-1,0,1})for(bool damaging:{false,true}){
  if((skill!=1382&&!damaging)||(npc_target&&eligibility!=-1))continue;
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  std::vector<world::Instance::Npc> population;
  if(npc_target){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.level=20;
   n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
   d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={500,1000};d.damageable=true;d.conditions_allowed=true;
   d.cold_armor=60;d.cold_armor_evidence="synthetic";d.spawn_energy=combat::npcs::Energy{20,100};
   d.energy_pips=0;d.energy_evidence=d.energy_pips_evidence="synthetic";n.combat_definition=d;population.push_back(n);}
  {world::Instance::Npc n;n.id=n.definition.id=101;n.x=110;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=101;
   d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};d.damageable=true;
   d.healing_allowed=eligibility<0?std::optional<bool>{}:std::optional(bool(eligibility));
   if(eligibility>=0)d.healing_evidence="synthetic";n.combat_definition=d;population.push_back(n);}
  w.configure_npcs(population);
  for(unsigned id:{1u,2u,3u}){
   w.join(id,"Actor",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==1?1000u:500u,1000,id==1?80u:20u,100},100);
   check(w.energy_capacity(id,6,0,100,{80,-2}));check(w.elemental_armor(*w.effect_scope(id),{},60));
  }
  const auto source=*w.effect_scope(1),target=*w.effect_scope(npc_target?100:2),monk=*w.effect_scope(3),ally=*w.effect_scope(101);
  if(!npc_target){check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));
   check(w.complete_healing_seed(monk,target,seed,0,100));}
  if(skill==1382&&damaging)check(w.movement_hex(source,target,*effects::movement_hex(210,12),500));
  const auto* row=find_skill(skill);check(row);
  casting::Hooks hooks;unsigned pays=0,commits=0;std::function<void()> copied;
  hooks.resources=[&](auto id){return w.resources(id);};hooks.events=[&](const auto& packets){box->enqueue(packets);};
  hooks.target_scope=[&](auto id){return w.effect_scope(id);};
  hooks.projectile_target_valid=[&](auto id,float range){return id==target.actor&&range==1248;};
  hooks.target_valid=[](auto id,float,bool self){return self&&id==1;};
  hooks.consume_energy_at=[&](unsigned amount,auto at){++pays;check(amount==row->energy_cost());return w.consume_energy(1,amount,source,at);};
  hooks.consume_overcast_energy=[&](auto id,unsigned amount,auto event,auto at){++pays;check(id==skill&&amount==row->energy_cost());return w.activate_overcast_energy(source,id,amount,event,at);};
  hooks.movement_hex=[&](auto selected,const auto& plan,auto at)->casting::EffectCompletion{
   ++commits;std::vector<world::Packet> visuals;std::function<void()> publication;bool accepted=false;
   if(plan.skill==1382)accepted=w.freezing_gust(source,selected,plan,20,at,&visuals,&publication);
   else{const auto freeze=effects::mind_freeze(plan.skill,plan.rank);check(bool(freeze));
    accepted=w.mind_freeze(source,selected,*freeze,20,at,&visuals,&publication);}
   copied=publication;return {accepted,std::move(visuals),{},std::move(publication),true};
  };
  Slots bar{};bar[0]={skill,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[skill]=1;
  attributes::Ranks ranks{};ranks[11]=12;casting::Runtime runtime(1);box->drain();
  check(runtime.start(1000,{skill,0,target.actor},bar,policy,ranks,true,hooks)==casting::Reason::accepted&&pays==1);
  const unsigned completion=1000+unsigned(row->activation*1000);
  runtime.update(completion-1,hooks);check(commits==0);runtime.update(completion,hooks);check(commits==1);
  const bool accepted=npc_target||!damaging||eligibility>=0;
  const unsigned hits=damaging?(skill==1382?1:2):0;
  const double amount=skill==1382?effects::freezing_gust(12)->damage:effects::mind_freeze(skill,12)->damage;
  const unsigned actual_hits=accepted?hits:0;
  const double hp=npc_target?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);
  check(std::abs(hp-(500-actual_hits*amount+(npc_target?0:actual_hits*seed.healing)))<1.01);
  check(w.resources(3)->health==500+(!npc_target?actual_hits*seed.healing:0));
  check(w.npc_combat_snapshot(ally)->health->current==500+(!npc_target&&eligibility==1?actual_hits*seed.healing:0));
  const auto packets=box->drain();std::vector<unsigned> order;
  const auto finish=casting::terminal(58,1),body=*particles::targeted_body_2022(target.actor,1,skill);
  const auto damage=combat::detail::integer(10,1,skill),healing=particles::health_change_2022(3,3,seed.healing,1000);
  for(const auto& packet:packets){
   if(packet==finish)order.push_back(1);else if(packet==body)order.push_back(2);
   else if(packet==damage)order.push_back(3);else if(packet==healing)order.push_back(4);
   else if(!npc_target&&packet.size()==20&&packet[0]==65&&word(packet,2)==target.actor&&(unsigned(packet[6])|(unsigned(packet[7])<<8))==skill)order.push_back(5);
   else if(npc_target&&packet.size()==10&&packet[0]==240&&word(packet,2)==target.actor&&(word(packet,6)&0x800))order.push_back(5);
  }
  std::vector<unsigned> expected;
  if(accepted){expected={1,2};for(unsigned hit=0;hit<hits;++hit){expected.push_back(3);if(!npc_target)expected.push_back(4);}
   if(skill!=1382||!damaging)expected.push_back(5);}
  if(npc_target&&accepted&&(skill!=1382||!damaging))check(w.npc_combat_snapshot(target)->effects&0x800);
  check(order==expected);
  if(accepted)check(bool(copied));auto second=copied;if(copied)copied();if(second)second();
  runtime.update(completion,hooks);check(commits==1&&box->drain().empty());
 }
 std::cout<<"PASS paid cold Seed damage/snare publication, NPC feedback, refusal and copied callback guards\n";
}catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}
