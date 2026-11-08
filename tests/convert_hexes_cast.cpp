#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
unsigned context=0;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Convert cast line "+std::to_string(at.line())+" mode "+std::to_string(context));
}
int main()try{
 for(unsigned rank=0;rank<=20;++rank)for(unsigned mode=0;mode<11;++mode){context=mode;
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u}){w.join(id,"Actor",float(id*30),0,id==1?box:std::make_shared<world::Outbox>(),0,id==1?std::vector<world::Packet>{{165,0,1,0,0,0,3,0}}:std::vector<world::Packet>{},{},100,1);w.set_vitals(id,{500,1000,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.status_mutation(target,100,123,0,100,30000,true));
  casting::Runtime cast(1);casting::Hooks h;unsigned payments=0,completed=0;
  h.resources=[&](auto id){return w.resources(id);};h.events=[&](const auto& p){box->enqueue(p);};
  h.consume_energy_at=[&](auto cost,auto at){++payments;return mode!=2&&w.consume_energy(1,cost,source,at);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[](auto,float,bool){return true;};
  h.ally_enchantment_target_valid=[&](auto to,auto divine,auto at){return w.valid_ally_enchantment_target(source,to,divine,at);};
  h.before_spell_effect=[](auto,auto,auto){return true;};
  h.convert_hexes=[&](auto from,auto to,const auto& plan,auto divine,auto at){++completed;return w.complete_convert_hexes(from,to,plan,divine,at);};
  if(mode==1)h.convert_hexes={};if(mode==3)h.before_spell_effect={};
  Policy p;p.primary=3;p.campaigns=255;p.learned[303]=1;Slots bar{};bar[0]={303,0};attributes::Ranks ranks{};ranks[15]=mode==10?21:rank;
  const auto admission=cast.start(100,{303,0,mode==4?1u:2u},bar,p,ranks,true,h);
  if(mode==1||mode==3||mode==10){check(admission==casting::Reason::unsupported&&payments==0);continue;}
  if(mode==4){check(admission==casting::Reason::target&&payments==0);continue;}
  if(mode==2){check(admission==casting::Reason::energy&&payments==1);continue;}
  check(admission==casting::Reason::accepted&&payments==1&&w.resources(1)->energy==85);cast.update(1099,h);check(completed==0);
  if(mode==5)cast.cancel(h);if(mode==6)cast.interrupt(1099,h);
  if(mode==7)h.target_scope=[&](auto id){auto scope=w.effect_scope(id);if(scope&&id==2)++scope->generation;return scope;};
  if(mode==8)h.convert_hexes={};if(mode==9)h.ally_enchantment_target_valid={};box->drain();cast.update(1100,h);check(!cast.active());
  if(mode>=5){check(completed==0&&w.convert_hexes_armor(target,1100)==0);continue;}
  check(completed==1&&w.convert_hexes_armor(target,1100)==10);const auto packets=box->drain();
  check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==1);
  check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,303))==1);
  cast.update(1200,h);check(completed==1);
  check(cast.start(2100,{303,0,2},bar,p,ranks,true,h)==casting::Reason::recharge);
 }
 for(unsigned rank=0;rank<=20;++rank){
  world::Instance w;std::vector<world::Instance::Npc> npcs;
  for(unsigned id:{100u,101u}){world::Instance::Npc n;n.id=n.definition.id=id;n.x=float(id-100)*100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;d.relation=id==100?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
   d.spawn_health=combat::Health{500,1000};d.damageable=d.conditions_allowed=true;d.spawn_energy=combat::npcs::Energy{50,50};d.energy_pips=0;d.healing_allowed=true;d.healing_evidence="authored cast fixture";n.combat_definition=d;npcs.push_back(n);}
  w.configure_npcs(npcs);auto box=std::make_shared<world::Outbox>();w.join(1,"Monk",50,0,box,0,{{165,0,1,0,0,0,3,0}},{},100,1);w.set_vitals(1,{500,1000,100,100},100);
  const auto source=*w.effect_scope(1),foe=*w.effect_scope(100),target=*w.effect_scope(101);
  check(w.configure_npc_spellbook(foe,{{{31,0},1248,"authored hostile cast"}}));check(w.begin_npc_phantasm(foe,target,12,100).has_value());w.tick(1100);
  check(w.status_snapshot(target,1100)->size()==1);
  casting::Runtime cast(1);casting::Hooks h;
  h.resources=[&](auto id){return w.resources(id);};h.events=[&](const auto& packets){box->enqueue(packets);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[](auto,float,bool){return true;};
  h.consume_energy_at=[&](auto cost,auto at){return w.consume_energy(1,cost,source,at);};h.before_spell_effect=[](auto,auto,auto){return true;};
  h.ally_enchantment_target_valid=[&](auto to,auto divine,auto at){return w.valid_ally_enchantment_target(source,to,divine,at);};
  h.convert_hexes=[&](auto from,auto to,const auto& plan,auto divine,auto at){return w.complete_convert_hexes(from,to,plan,divine,at);};
  Policy p;p.primary=3;p.campaigns=255;p.learned[303]=1;Slots bar{};bar[0]={303,0};attributes::Ranks ranks{};ranks[15]=rank;ranks[16]=12;
  check(cast.start(1100,{303,0,101},bar,p,ranks,true,h)==casting::Reason::accepted);box->drain();cast.update(2100,h);
  check(w.status_snapshot(target,2100)->empty()&&w.convert_hexes_armor(target,2100)==0);
  check(std::abs(w.npc_combat_snapshot(target)->health->current-528)<1e-7); //10degeneration,38Divine Favor.
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==1);
 }
 std::cout<<"PASS paid Convert Hexes all ranks,hook guards,identity,cancel,interrupt,one completion,pinned recharge and real NPC hex removal/Divine Favor\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
