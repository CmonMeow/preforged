#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC elemental Reversal line "+std::to_string(at.line()));
}
int main()try{
 unsigned earth_cases=0;
 for(unsigned skill:{174u,937u})for(unsigned rank:{0u,12u})for(unsigned hp:{10u,1000u}){
  ++earth_cases;world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.spawn_health={double(hp),1000};d.damageable=d.conditions_allowed=true;d.relation=combat::npcs::Relation::hostile_to_players;
  d.earth_armor=60;d.healing_allowed=true;d.healing_evidence="authored earth conversion";
  n.combat_definition=d;w.configure_npcs({n});auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
  check(w.stage_npc_shield_absorption(target,12,100)&&w.stage_npc_reversal(target,rank,100));
  const auto plan=*skills::effects::caster_earth_aoe(skill,12);box->drain();
  check(w.shockwave(source,plan,20,100));
  double expected=hp;bool conversion=true;unsigned hit=0;
  for(unsigned slice=0;slice<plan.count&&expected>0;++slice){
   const double raw=std::max(0.,double(plan.strikes[slice].earth_damage)-5*++hit);
   const auto converted=conversion&&raw>0?std::min(raw,double(skills::effects::reversal_fortune(rank)->maximum_healing)):0.;
   if(converted>0){expected=std::min(1000.,expected+converted);conversion=false;}
   expected=std::max(0.,expected-(raw-converted));
  }
  check(w.npc_combat_snapshot(target)->health->current==expected);
  const auto packets=box->drain();check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==67&&p[2]==100;})==1);
  check(w.take_npc_deaths(source).size()==unsigned(expected<=0));
 }
 for(unsigned mode:{0u,1u,2u,3u}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.x=60;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.spawn_health={mode==2?1000.:500.,1000};
  d.damageable=d.conditions_allowed=true;d.relation=combat::npcs::Relation::hostile_to_players;
  d.earth_armor=mode==2?400:60;d.healing_allowed=true;d.healing_evidence="authored earth feedback";
  n.combat_definition=d;w.configure_npcs({n});auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
  if(mode==3){
   w.join(2,"Bonded",700,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,1000,100,100},100);
   check(w.elemental_armor(*w.effect_scope(2),60,60));
   check(w.stage_npc_bonds_of_torment(target,*w.effect_scope(2),*skills::effects::bonds_of_torment_npc(),100));
  }
  if(mode!=1)check(w.degeneration_hex(source,target,*skills::effects::degeneration_hex(19,mode==2?20:12),100));
  else check(w.stage_npc_watch_yourself_recipient(target,target,*skills::effects::watch_yourself(0,true),100));
  if(mode==2)check(w.stage_npc_shield_absorption(target,12,100));
  check(w.stage_npc_reversal(target,12,100));box->drain();
  const auto plan=*skills::effects::caster_earth_aoe(mode!=1?937:174,0);
  check(w.shockwave(source,plan,20,100));
  if(mode==2){
   check(w.npc_combat_snapshot(target)->health->current==995);
   const auto packets=box->drain();
   check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==67&&p[2]==100;})==1);
   const auto next=w.damage_npc(source,target,{combat::Kind::armor_ignoring,100,20,174},1248,100);
   check(next.reason==combat::Reason::accepted&&next.damage==65);continue;
  }
  const double first=plan.strikes[0].earth_damage*std::exp2((60.-(mode!=1?60.:65.))/40.);
  const double expected=mode!=1?500+first-2*first-3*skills::effects::fragility(19,12)->damage:500+first;
  check(std::abs(w.npc_combat_snapshot(target)->health->current-expected)<1e-7);
  const auto packets=box->drain();
  const auto healing=skills::particles::health_change_2022(100,100,first,1000);
  const auto healed=std::find(packets.begin(),packets.end(),healing);check(healed!=packets.end());
  if(mode!=1){
   const auto loss=skills::particles::health_change_2022(100,1,-double(skills::effects::fragility(19,12)->damage),1000);
   check(std::count(packets.begin(),packets.end(),loss)==3);
   check(std::count(packets.begin(),packets.end(),combat::detail::fraction(100,{expected,1000}))>=1);
   const auto attributed=std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,937));
   check(attributed!=packets.end()&&healed<attributed);
   std::vector<unsigned> order;
   for(const auto& packet:packets){if(packet==combat::detail::integer(10,1,19))order.push_back(19);else if(packet==combat::detail::integer(10,1,937))order.push_back(937);}
   check(order==std::vector<unsigned>({19,937,19,937,19}));
   if(mode==3){
    check(w.resources(2)->health==419);
    std::vector<unsigned> callbacks;
    for(const auto& packet:packets){
     if(packet==combat::detail::integer(10,1,19))callbacks.push_back(19);
     else if(packet==combat::detail::integer(10,1,937))callbacks.push_back(937);
     else if(packet==combat::detail::integer(10,100,1881))callbacks.push_back(1881);
    }
    check(callbacks==std::vector<unsigned>({19,1881,937,1881,19,1881,937,1881,19,1881}));
   }
  }else{
   check(w.watch_yourself_armor(target,100)==5);
   check(w.damage_npc(source,target,{combat::Kind::earth,100,20,174},1248,100).reason==combat::Reason::accepted);
   check(w.watch_yourself_armor(target,100)==0);
  }
 }
 {
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.spawn_health={15,1000};
  d.damageable=d.conditions_allowed=true;d.relation=combat::npcs::Relation::hostile_to_players;d.earth_armor=400;
  n.combat_definition=d;w.configure_npcs({n});auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
  check(w.degeneration_hex(source,target,*skills::effects::degeneration_hex(19,12),100));box->drain();
  check(w.shockwave(source,*skills::effects::caster_earth_aoe(937,0),20,100));
  check(w.npc_combat_snapshot(target)->health->current==0);
  check(w.has_condition(target,combat::conditions::Kind::weakness,100));
  check(!w.has_condition(target,combat::conditions::Kind::blind,100));
  check(!w.has_condition(target,combat::conditions::Kind::cracked_armor,100));
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,937))==1);
  check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,19))==1);
  const auto reaction=std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,19));
  const auto death=std::find_if(packets.begin(),packets.end(),[](const auto& packet){return packet.size()==10&&packet[0]==240&&(packet[6]&0x10);});
  check(death!=packets.end()&&reaction<death);
  check(w.take_npc_deaths(source).size()==1);
 }
 unsigned cases=0;
 for(unsigned skill:{194u,163u,170u,1086u,89u,1227u,1068u,866u,145u,3059u,188u,1081u,845u,183u,186u,189u,191u,1379u,884u,2192u,1095u,846u})
 for(unsigned rr:{0u,12u})for(unsigned hp:{10u,500u,1000u})for(bool burning:{false,true}){
  const auto plan=*skills::effects::elemental(skill,12);if(plan.projectile&&burning)continue;++cases;
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.spawn_health={double(hp),1000};d.damageable=true;d.conditions_allowed=true;d.relation=combat::npcs::Relation::hostile_to_players;
  d.fire_armor=d.cold_armor=d.lightning_armor=d.earth_armor=60;d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;
  d.knockdown_allowed=true;d.knockdown_evidence="synthetic";d.healing_allowed=true;d.healing_evidence="synthetic";d.creature_type="human";d.creature_type_evidence="synthetic";
  n.combat_definition=d;w.configure_npcs({n});auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
  if(burning){check(w.apply_condition(source,source,combat::conditions::Kind::burning,100,10000));check(w.condition_transfer(source,target,*skills::effects::condition_transfer(132,12),100));}
  if(skill==163)check(w.begin_npc_action(target,combat::npcs::control::Action::attack,100,2000).has_value());
  check(w.stage_npc_reversal(target,rr,100));box->drain();skills::effects::ElementalOutcome outcome;std::optional<skills::sacrifice::Loss> sacrifice;
  if(!w.elemental(source,plan.caster_center?source:target,plan,20,100,nullptr,&sacrifice,&outcome))throw std::runtime_error("admission skill="+std::to_string(skill));
  if(plan.projectile)w.tick(1000);
  const bool no_damage=plan.burn_or_damage&&!burning;
  const double raw=(plan.damage+(burning?plan.bonus_if_burning:0))*(plan.half_if_enchanted?.5:1.);
  double expected=hp;
  if(!no_damage){const double converted=std::min(raw,rr?67.:15.);expected=std::max(0.,std::min(1000.,expected+converted)-(raw-converted));
   if(expected>0&&hp>500&&plan.shadow_above_half)expected=std::max(0.,expected-plan.shadow_above_half);}
  if(plan.self_bleeding_ms)check(sacrifice.has_value()&&w.resources(1)->health<1000);
  const auto at=plan.projectile?1000:100;
  // Existing corpse condition timers are a separate cleanup policy. Check
  // surviving recipients and prohibit new Burning on a killed recipient.
  if(expected>0||!burning)check(w.has_condition(target,combat::conditions::Kind::burning,at)==(expected>0&&(burning||plan.burning_ms)));
  check(w.has_condition(target,combat::conditions::Kind::blind,at)==(expected>0&&burning&&plan.blind_if_burning_ms));
  if(skill==163||skill==170||skill==1086)check(w.knocked_down(target,at)==(expected>0));
  const auto actor=*w.npc_combat_snapshot(target);
  if(std::abs(actor.health->current-expected)>1e-7)throw std::runtime_error("skill="+std::to_string(skill)+" HP="+std::to_string(actor.health->current)+" expected="+std::to_string(expected));
  const auto packets=box->drain();const auto retirements=std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==67&&p[2]==100;});
  check(retirements==unsigned(!no_damage));check(w.take_npc_deaths(source).size()==unsigned(expected<=0));
 }
 unsigned control_cases=0;
 for(unsigned skill:{163u,170u,1086u})for(unsigned mode=0;mode<3;++mode){
  ++control_cases;world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.spawn_health={10,1000};
  d.damageable=true;d.conditions_allowed=true;d.relation=combat::npcs::Relation::hostile_to_players;
  d.earth_armor=d.cold_armor=60;d.healing_allowed=true;d.healing_evidence="synthetic";
  d.knockdown_allowed=mode?std::optional<bool>{mode==2}:std::nullopt;if(mode)d.knockdown_evidence="synthetic";
  n.combat_definition=d;w.configure_npcs({n});w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);check(w.stage_npc_reversal(target,12,100));
  if(skill==163)check(w.begin_npc_action(target,combat::npcs::control::Action::attack,100,2000).has_value());
  const auto plan=*skills::effects::elemental(skill,12);
  check(w.elemental(source,plan.caster_center?source:target,plan,20,100)==(mode!=0));
  const double converted=std::min(plan.damage,67.);
  check(std::abs(w.npc_combat_snapshot(target)->health->current-(mode?10+converted-(plan.damage-converted):10))<1e-7);
  check(w.knocked_down(target,100)==(mode==2));
  if(!mode)check(w.npc_reversal_damage(source,target,{combat::Kind::armor_ignoring,1,20,42},100));
 }
 check(control_cases==9);
 check(cases==252);std::cout<<"PASS252 World NPC Reversal cases across22 elemental plans;paid307 and other damage families remain pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
