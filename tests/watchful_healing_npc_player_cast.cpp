#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Watchful player-to-NPC cast line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<12;++mode){
  const bool secondary=mode==10,zero=secondary||mode==11;
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=mode==2?2000:100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=mode==1?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
  d.spawn_health=combat::Health{100,500};d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="authored fixture";
  if(mode==8||zero)d.healing_allowed.reset();if(mode==9)d.healing_allowed=false;
  n.combat_definition=d;w.configure_npcs({n});auto box=std::make_shared<world::Outbox>();
  w.join(1,"Monk",0,0,box,0,{world::Packet{165,0,1,0,0,0,static_cast<std::uint8_t>(secondary?6:3),0}}, {},100,1);w.set_vitals(1,{500,500,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);bool valid=true;unsigned legacy=0,attempts=0;
  check(!w.resources(100));casting::Hooks h;
  h.resources=[&](auto id){return w.resources(id);};h.events=[&](const auto& packets){box->enqueue(packets);};
  h.target_scope=[&](auto id){auto scope=w.effect_scope(id);if(mode==3&&id==100&&scope)++scope->generation;return scope;};
  h.target_valid=[&](auto,float,bool){++legacy;return false;}; // The player-only guard must never admit this cast.
  h.ally_enchantment_target_valid=[&](auto scope,unsigned divine,auto at){check(divine==(zero?0u:12u));return valid&&w.valid_watchful_target(source,scope,divine,at);};
  h.consume_energy_at=[&](auto amount,auto at){return w.consume_energy(1,amount,source,at);};
  h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(source,skill,event,at);};
  h.watchful_healing=[&](auto scope,const auto& plan,unsigned divine,auto at)->casting::EffectCompletion{
   ++attempts;std::vector<world::Packet> visuals;std::function<void()> publish;
   const auto ok=w.stage_watchful_healing(source,scope,plan,divine,at,&visuals,&publish);
   return {ok,std::move(visuals),{},std::move(publish),true};
  };
  if(mode==4)h.ally_enchantment_target_valid={};
  Policy policy;policy.primary=secondary?6:3;policy.secondary=secondary?3:0;policy.campaigns=255;policy.learned[1392]=1;
  Slots bar{};bar[0]={1392,0};attributes::Ranks ranks{};ranks[16]=zero?0:12;casting::Runtime cast(1);box->drain();
  check(!w.valid_watchful_target(source,target,21,1000));
  if(secondary)check(!w.valid_watchful_target(source,target,1,1000));
  const auto started=cast.start(1000,{1392,0,100},bar,policy,ranks,true,h);
  check(started==(mode==1||mode==2||mode==3||mode==8?casting::Reason::target:mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  check(legacy==0);
  if((mode>=1&&mode<=4)||mode==8){check(w.resources(1)->energy==100&&attempts==0);continue;}
  check(w.resources(1)->energy==95);if(mode==5)h.ally_enchantment_target_valid={};if(mode==6)valid=false;if(mode==7)cast.cancel(h);
  const bool applied=mode==0||mode>=9;
  box->drain();cast.update(2000,h);const auto packets=box->drain();check(!cast.active()&&legacy==0&&attempts==unsigned(applied));
  check(w.npc_combat_snapshot(target)->health->current==(mode==0?138:100));check(!w.resources(100));
  if(applied){const auto visual=*particles::targeted_body_2022(100,1,1392);
   const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
   const auto body=std::find(packets.begin(),packets.end(),visual);
   const auto status=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>2&&p[0]==65;});
   check(finish!=packets.end()&&body!=packets.end()&&status!=packets.end()&&finish<body&&body<status);
   if(mode>=9){check(std::none_of(packets.begin(),packets.end(),[](const auto& p){return p.size()>6&&p[0]==162&&p[2]==55;}));
    w.tick(3000);check(w.npc_combat_snapshot(target)->health->current==(zero?102:106));}
  }
 }
 std::cout<<"PASS Watchful actual player-to-NPC casting without fabricated Resources,authority guards,cost and completion publication\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
