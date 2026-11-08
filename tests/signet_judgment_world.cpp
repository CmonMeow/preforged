#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Signet Judgment World line "+std::to_string(at.line()));}
int main()try{unsigned cases=0;
 for(bool pvp:{false,true})for(bool npc_primary:{false,true})for(unsigned mode=0;mode<12;++mode){++cases;world::Instance w;auto box=std::make_shared<world::Outbox>();const auto plan=*skills::effects::signet_judgment(12,pvp);const float center=mode==4?plan.range+1:500;
  std::vector<world::Instance::Npc> npcs;
  for(unsigned id:{100u,101u,102u,103u}){world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?(npc_primary?center:center+80):id==101?center+(mode==8?166.01f:166.f):center+1;n.definition.visual_adjustment={0,0,0,100};n.plane=(id==101&&mode==9)||id==103?1:0;
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.relation=id==102?combat::npcs::Relation::allied_to_players:combat::npcs::Relation::hostile_to_players;d.damageable=d.conditions_allowed=true;d.spawn_health=combat::Health{id==100&&npc_primary&&mode==5?1.:1000.,1000};d.holy_traits.light_sensitive=id==101;d.light_sensitive_evidence="authored";
   if(id==100){d.knockdown_allowed=mode!=6;d.knockdown_evidence="authored";if(npc_primary&&mode==1)d.holy_traits.light_sensitive.reset();}
   if(id==101&&(mode==2||mode==8||mode==9))d.spawn_health.reset();if(id==101&&mode==3)d.holy_traits.light_sensitive.reset();if(id>=102){d.spawn_health.reset();d.holy_traits={};}if(!d.spawn_health)d.damageable=d.conditions_allowed=false;n.combat_definition=d;npcs.push_back(n);
  }w.configure_npcs(npcs);
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);w.join(2,"Foe",npc_primary?center+100:center,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{!npc_primary&&mode==5?1u:1000u,1000,100,100},100);w.join(3,"Ally",center+1,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(3,{1000,1000,100,100},100);
  const auto caster=*w.effect_scope(1),primary=*w.effect_scope(npc_primary?100:2);check(w.player_holy_equipment(*w.effect_scope(2),!npc_primary&&mode==1?std::optional<unsigned>{}:std::optional<unsigned>{mode==10?6u:0u}));
  if(!npc_primary&&mode==6)check(w.status_mutation(primary,361,77,0,100,8100,true));if(npc_primary&&mode==10)check(w.stage_npc_shield_deflection(primary,*skills::effects::shield_deflection(12),100));
  w.configure_attack_block_draws([]()->double{throw std::runtime_error("signet used attack block roll");});box->drain();const bool accepted=mode==11?w.complete_signet_judgment(caster,primary,plan,20,1000):w.stage_signet_judgment(caster,primary,plan,20,1000);
  const bool rejected=mode==1||mode==2||mode==3||mode==4;check(accepted==!rejected);const auto packets=box->drain();
  const auto npc_health=w.npc_combat_snapshot(*w.effect_scope(100))->health->current;const auto player_health=w.resources(2)->health;
  if(rejected){check(player_health==1000&&npc_health==1000);check(!w.knocked_down(primary,1000));continue;}
  const unsigned player_damage=plan.damage+(mode==10?6:0);check(player_health==(!npc_primary&&mode==5?0:1000-player_damage));check(npc_health==(npc_primary&&mode==5?0:1000-plan.damage));
  check(w.knocked_down(primary,1000)==(mode!=5&&mode!=6));check(!w.knocked_down(*w.effect_scope(npc_primary?2:100),1000));
  const auto side=w.npc_combat_snapshot(*w.effect_scope(101));if(mode==8||mode==9)check(!side->health);else check(side->health->current==1000-2*plan.damage);
  check(w.resources(3)->health==1000&&w.resources(1)->energy==100);check(std::count(packets.begin(),packets.end(),combat::detail::integer(58,1,0))==unsigned(mode==11));
  if(mode==11){const auto finish=std::find(packets.begin(),packets.end(),combat::detail::integer(58,1,0)),body=std::find(packets.begin(),packets.end(),combat::detail::integer(21,1,519)),damage=std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,plan.skill)),knock=std::find(packets.begin(),packets.end(),combat::knockdown_2022(primary.actor,2));check(finish<body&&body<damage&&damage<knock&&knock!=packets.end());}
 }
 for(bool pvp:{false,true}){
  world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Combatant",id==1?0.f:500.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==2?10u:id==3?500u:1000u,1000,100,100},100);}
  const auto source=*w.effect_scope(1),primary=*w.effect_scope(2),neighbor=*w.effect_scope(3);
  check(w.player_holy_equipment(primary,0)&&w.player_holy_equipment(neighbor,0));
  check(w.degeneration_hex(neighbor,primary,*skills::effects::degeneration_hex(99,12),500));
  const auto plan=*skills::effects::signet_judgment(12,pvp);check(w.stage_signet_judgment(source,primary,plan,20,500));
  check(w.resources(2)->health==0&&w.resources(3)->health==500+skills::effects::parasitic_bond(12)->healing-plan.damage);
 }

 for(bool pvp:{false,true})for(unsigned mode=0;mode<7;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Foe",500,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.player_holy_equipment(target,0));
  skills::Slots bar{};bar[0]={294,0};skills::Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[294]=1;policy.pvp_match=pvp;attributes::Ranks ranks{};ranks[14]=12;
  skills::casting::Hooks h;unsigned commits=0;bool stale=false,ready=true;h.resources=[&](unsigned id){return w.resources(id);};h.consume_energy_at=[&](unsigned cost,auto now){check(cost==0);return w.consume_energy(1,cost,source,now);};h.events=[&](const auto& packets){w.skill_event(1,packets);};h.target_scope=[&](unsigned id){return stale&&id==2?std::optional<skills::effects::Scope>{}:w.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float range){check(range==(pvp?624:1248));return id==2;};h.signet_judgment_ready=[&](auto){return ready;};
  if(mode!=1)h.signet_judgment=[&](auto selected,const auto& plan,auto now)->skills::casting::EffectCompletion{++commits;check(plan.skill==(pvp?2887u:294u));const bool ok=w.complete_signet_judgment(source,selected,plan,20,now);return {ok,{},{},{},false,true};};
  skills::casting::Runtime runtime(1);check(runtime.start(1000,{294,0,2},bar,policy,ranks,true,h)==(mode==1?skills::casting::Reason::unsupported:skills::casting::Reason::accepted));check(w.resources(1)->energy==100);
  box->drain();if(mode==2)runtime.cancel(h);if(mode==3)runtime.interrupt(1500,h);if(mode==4)stale=true;if(mode==5)ready=false;
  runtime.update(1999,h);check(commits==0);runtime.update(2000,h);runtime.update(2000,h);const bool success=mode==0||mode==6;check(commits==unsigned(success));
  const auto packets=box->drain();check(w.resources(2)->health==1000-(success?skills::effects::signet_judgment(12,pvp)->damage:0));check(std::count(packets.begin(),packets.end(),skills::casting::terminal(58,1))==unsigned(success));
  if(success){
   check(runtime.start(21999,{294,0,2},bar,policy,ranks,true,h)==skills::casting::Reason::recharge);
   check(runtime.start(22000,{294,0,2},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);
   runtime.cancel(h);
  }
 }

 // Real World admission,exact range boundaries,explicit alias policy and
 // replacement actor identity: no permissive target hook in these cases.
 for(bool pvp:{false,true})for(bool alias:{false,true})for(unsigned mode=0;mode<7;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();const auto skill=alias?2887u:294u;const float range=pvp?624.f:1248.f;const float x=mode==1?range+.01f:range;
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);w.join(2,"Foe",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.player_holy_equipment(target,0));check(w.ordinary_weapon(source,{},20,100));
  skills::Slots bar{};bar[0]={skill,0};skills::Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[skill]=1;policy.pvp_match=pvp;attributes::Ranks ranks{};ranks[14]=12;
  skills::casting::Hooks h;unsigned commits=0;std::uint64_t at=1000;bool ready=mode!=2;h.resources=[&](unsigned id){return w.resources(id);};h.events=[&](const auto& packets){w.skill_event(1,packets);};h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.consume_energy_at=[&](unsigned cost,auto now){check(cost==0);return w.consume_energy(1,cost,source,now);};
  h.projectile_target_valid=[&](unsigned id,float limit){return w.projectile_target_valid(source,id,limit,at);};h.signet_judgment_ready=[&](auto now){return ready&&w.shield_judgment_caster_ready(source,now);};
  h.signet_judgment=[&](auto selected,const auto& plan,auto now)->skills::casting::EffectCompletion{++commits;const bool ok=w.complete_signet_judgment(source,selected,plan,20,now);return {ok,{},{},{},false,true};};
  if(mode==6)ranks[14]=21;skills::casting::Runtime runtime(1);const auto result=runtime.start(at,{skill,0,2},bar,policy,ranks,true,h);
  const bool admitted=!(alias&&!pvp)&&mode!=1&&mode!=2&&mode!=6;
  check((result==skills::casting::Reason::accepted)==admitted);check(w.resources(1)->energy==100);
  if(!admitted)continue;
  if(mode==3){w.leave(2);w.join(2,"Replacement",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);check(w.player_holy_equipment(*w.effect_scope(2),0));check(*w.effect_scope(2)!=target);}
  if(mode==4)h.signet_judgment={};if(mode==5)ready=false;box->drain();at=2000;runtime.update(at,h);
  check(commits==unsigned(mode==0));check(w.resources(2)->health==1000-(mode==0?skills::effects::signet_judgment(12,pvp)->damage:0));
 }
 // Each recipient owns its conversion independently; only the primary
 // receives control, including when conversion saves an otherwise lethal hit.
 for(bool pvp:{false,true})for(bool npc_primary:{false,true})for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u}){
  world::Instance w;std::vector<world::Instance::Npc> npcs;
  for(unsigned id:{100u,101u}){world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?(npc_primary?500:580):666;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health={double(hp),1000};d.damageable=d.conditions_allowed=true;d.holy_traits.light_sensitive=id==101;d.light_sensitive_evidence="synthetic";
   d.knockdown_allowed=true;d.knockdown_evidence="synthetic";d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;npcs.push_back(n);
  }w.configure_npcs(npcs);auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Foe",npc_primary?580.f:500.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{hp,1000,100,100},100);
  const auto source=*w.effect_scope(1),player=*w.effect_scope(2),primary=*w.effect_scope(npc_primary?100:2);
  check(w.player_holy_equipment(player,6));check(w.status_mutation(player,307,5,rr,100,8100,true));
  for(unsigned id:{100u,101u})check(w.stage_npc_reversal(*w.effect_scope(id),rr,100));
  box->drain();const auto plan=*skills::effects::signet_judgment(12,pvp);check(w.complete_signet_judgment(source,primary,plan,20,1000));
  const auto expected=[&](double raw){const double converted=std::min(raw,double(skills::effects::reversal_fortune(rr)->maximum_healing));return std::max(0.,std::min(1000.,hp+converted)-(raw-converted));};
  check(std::abs(double(w.resources(2)->health)-expected(plan.damage+6))<1.1);
  for(unsigned id:{100u,101u})check(std::abs(w.npc_combat_snapshot(*w.effect_scope(id))->health->current-expected(plan.damage*(id==101?2:1)))<1e-7);
  check(w.knocked_down(primary,1000)==(expected(plan.damage+(npc_primary?0:6))>0));
  check(!w.knocked_down(*w.effect_scope(npc_primary?2:100),1000)&&!w.knocked_down(*w.effect_scope(101),1000));
  check(w.take_npc_deaths(source).size()==unsigned(expected(plan.damage)<=0)+unsigned(expected(2*plan.damage)<=0));
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(58,1,0))==1);
 }
 // Unknown control is required only when the post-conversion primary survives.
 for(bool pvp:{false,true})for(unsigned rr:{0u,12u}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=500;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health={10,1000};d.damageable=true;d.holy_traits.light_sensitive=false;d.light_sensitive_evidence="synthetic";d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
  auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);check(w.stage_npc_reversal(target,rr,100));box->drain();
  check(w.complete_signet_judgment(source,target,*skills::effects::signet_judgment(12,pvp),20,1000)==(rr==0));
  if(rr){check(w.npc_combat_snapshot(target)->health->current==10&&box->drain().empty());check(w.npc_reversal_damage(source,target,{combat::Kind::armor_ignoring,1,20,42},1000));check(w.npc_combat_snapshot(target)->health->current==11);}
 }
 // Deferred Deep Wound loss can make the conversion heal lethal. No
 // damage/control feedback is valid after that healing death.
 for(bool pvp:{false,true})for(bool npc:{false,true}){
  world::Instance w;if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=500;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health={10,1000};d.damageable=d.conditions_allowed=true;d.holy_traits.light_sensitive=false;d.light_sensitive_evidence="synthetic";d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
  }else{w.join(2,"Foe",500,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{10,1000,100,100},100);}
  auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
  if(npc){check(w.apply_condition(source,source,combat::conditions::Kind::deep_wound,100,10000));check(w.condition_transfer(source,target,*skills::effects::condition_transfer(132,12),100));check(w.stage_npc_reversal(target,12,100));}
  else{check(w.player_holy_equipment(target,0));check(w.apply_condition(source,target,combat::conditions::Kind::deep_wound,100,10000));check(w.status_mutation(target,307,5,12,100,8100,true));}
  box->drain();check(w.complete_signet_judgment(source,target,*skills::effects::signet_judgment(12,pvp),20,100));
  check((npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health)==0&&!w.knocked_down(target,100));
  const auto packets=box->drain();check(std::none_of(packets.begin(),packets.end(),[](const auto& p){return p.size()==18&&p[0]==162;}));
  if(npc)check(w.take_npc_deaths(source).size()==1);
 }
 std::cout<<"PASS48 Signet Judgment staged World PvE/PvP mixed-crowd,range,immunity,unknown-fact,lethal and completion-publication cases,two cross-recipient death-heal regressions,14 normal casts and28 alias/range/lifetime/rank guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}







