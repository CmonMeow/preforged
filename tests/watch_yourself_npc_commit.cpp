#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Watch NPC commit line "+std::to_string(at.line()));}
struct Fixture{world::Instance w;Scope caster,target,player;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(unsigned health=10000,bool unknown_knockdown=false){std::vector<world::Instance::Npc> bodies;for(unsigned id:{100u,101u}){world::Instance::Npc n;n.id=n.definition.id=id;n.x=float(id-100)*60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{id==101?double(health):10000.,10000};d.spawn_energy=combat::npcs::Energy{10,100};d.energy_pips=0;if(!unknown_knockdown||id==100){d.knockdown_allowed=true;d.knockdown_evidence="fixture";}d.damageable=d.conditions_allowed=true;d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=d.physical_armor=60;n.combat_definition=d;bodies.push_back(n);}
  w.configure_npcs(bodies);w.join(1,"Player",120,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);caster=*w.effect_scope(100);target=*w.effect_scope(101);player=*w.effect_scope(1);}
};
int main()try{
 for(bool pvp:{false,true})for(unsigned rank=0;rank<=20;++rank){Fixture f;const auto plan=*watch_yourself(rank,pvp);
  check(f.w.stage_npc_watch_yourself_recipient(f.caster,f.target,plan,100));const auto entry=f.w.status_snapshot(f.target,100)->front();f.box->drain();
  check(f.w.damage_npc(f.player,f.target,{combat::Kind::armor_ignoring,10,20,133},1248,100).reason==combat::Reason::accepted);
  combat::Damage bypass{combat::Kind::fire,10,20,191};bypass.ignores_armor=true;check(f.w.damage_npc(f.player,f.target,bypass,1248,100).reason==combat::Reason::accepted);
  check(f.w.watch_yourself_armor(f.target,100)==plan.armor);
  for(unsigned n=0;n<plan.charges;++n){const double before=f.w.npc_combat_snapshot(f.target)->health->current;
   check(f.w.lightning(f.player,f.target,*lightning(224,12),20,100));const double loss=before-f.w.npc_combat_snapshot(f.target)->health->current;
   check(std::abs(loss-45*std::exp2((60-(60.+plan.armor)*.75)/40))<1e-7);
   check(f.w.watch_yourself_armor(f.target,100)==(n+1==plan.charges?0u:plan.armor));}
  world::Packet end{67,0};combat::detail::put(end,101);combat::detail::put(end,entry.instance);const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),end)==1);
  check(f.w.status_snapshot(f.target,100)->empty());
 }
 {Fixture f;auto stale=f.caster;++stale.generation;check(!f.w.stage_npc_watch_yourself_recipient(stale,f.target,*watch_yourself(12),100));check(!f.w.stage_npc_watch_yourself_recipient(f.player,f.target,*watch_yourself(12),100));
  check(f.w.stage_npc_watch_yourself_recipient(f.caster,f.target,*watch_yourself(12),100));const auto entry=f.w.status_snapshot(f.target,100)->front();f.box->drain();
  check(f.w.damage_npc(f.player,f.target,{combat::Kind::armor_ignoring,20000,20,133},1248,100).killed);check(f.w.watch_yourself_armor(f.target,100)==0);
  world::Packet end{67,0};combat::detail::put(end,101);combat::detail::put(end,entry.instance);const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),end)==1);
 }
 for(unsigned mode=0;mode<6;++mode){Fixture f;check(f.w.stage_npc_watch_yourself_recipient(f.caster,f.target,*watch_yourself(0,true),100));
  combat::weapon::Owned weapon=mode==1?combat::weapon::Owned{12,1,{20,20,4},12,false,true,26}:mode==2||mode==3?combat::weapon::Owned{8,1,{500,500,mode==3?4u:1u},0,true,false,5}:mode==5?combat::weapon::Owned{1,1,{20,20,1},12,false,false,2}:combat::weapon::Owned{12,1,{20,20,2},12,false,false,27};
  check(f.w.ordinary_weapon(f.player,weapon,20)&&f.w.configure_projectiles({10000,2}));
  if(mode==2||mode==3)check(f.w.melee_attack(f.player,f.target,weapon,*combat::weapon::attack_skill(394,12),100));
  else if(mode==4)check(f.w.melee_attack(f.player,f.target,weapon,*combat::weapon::attack_skill(328,12),100));
  else if(mode==5)check(f.w.melee_attack(f.player,f.player,weapon,*combat::weapon::attack_skill(330,12),100));
  else check(f.w.ordinary_attack(f.player,101,100));
  f.w.tick(600);f.w.tick(1000);check(f.w.npc_combat_snapshot(f.target)->health->current<10000);
  check(f.w.watch_yourself_armor(f.target,1000)==(mode==2||mode==3?5u:0u));
 }
 for(unsigned skill:{185u,209u,2803u,226u,2804u}){Fixture f;check(f.w.stage_npc_watch_yourself_recipient(f.caster,f.target,*watch_yourself(0,true),100));const double before=f.w.npc_combat_snapshot(f.target)->health->current;
  double damage=0,penetration=0;bool accepted=false;
  if(skill==185){const auto p=*mind_burn(skill,12);damage=p.damage;accepted=f.w.mind_burn(f.player,f.target,p,20,100);}
  else if(skill==209||skill==2803){const auto p=*mind_freeze(skill,12);damage=p.damage;accepted=f.w.mind_freeze(f.player,f.target,p,20,100);}
  else{const auto p=*mind_shock(skill,12);damage=p.damage;penetration=p.penetration;accepted=f.w.mind_shock(f.player,f.target,p,20,100);}
  const double expected=damage*std::exp2((60-65*(1-penetration/100))/40)+damage*std::exp2((60-60*(1-penetration/100))/40);
  check(accepted&&std::abs(before-f.w.npc_combat_snapshot(f.target)->health->current-expected)<1e-7&&f.w.watch_yourself_armor(f.target,100)==0);
 }
 {Fixture f(104,true);check(f.w.stage_npc_watch_yourself_recipient(f.caster,f.target,*watch_yourself(0,true),100));
  check(f.w.mind_shock(f.player,f.target,*mind_shock(226,12),20,100));check(f.w.npc_combat_snapshot(f.target)->health->current==0);}
 for(unsigned skill:{937u,174u}){Fixture f;check(f.w.stage_npc_watch_yourself_recipient(f.caster,f.target,*watch_yourself(0,true),100));
  if(skill==174)check(f.w.gale(f.player,f.target,12,{},100));
  const auto entry=f.w.status_snapshot(f.target,100)->front();f.box->drain();
  const double before=f.w.npc_combat_snapshot(f.target)->health->current;const auto plan=*caster_earth_aoe(skill,12);
  check(f.w.shockwave(f.player,plan,20,100));
  const double first=plan.strikes[0].earth_damage*std::exp2(-5./40.);
  const double later=skill==937?plan.strikes[1].earth_damage+plan.strikes[2].earth_damage:plan.strikes[0].knocked_down_bonus;
  check(std::abs(before-f.w.npc_combat_snapshot(f.target)->health->current-first-later)<1e-7);
  check(f.w.watch_yourself_armor(f.target,100)==0);
  world::Packet end{67,0};combat::detail::put(end,101);combat::detail::put(end,entry.instance);const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),end)==1);
 }
 // Direct, caster-area, projectile-area, burn-only, and Earthquake routes.
 for(unsigned skill:{191u,89u,884u,183u,186u,170u}){Fixture f;const auto watch=*watch_yourself(0,skill!=89);
  check(f.w.stage_npc_watch_yourself_recipient(f.caster,f.target,watch,100));
  if(skill==170)check(f.w.earthquake(f.player,f.target,*earthquake(skill,12),20,100));
  else{const auto plan=*elemental(skill,12);check(f.w.elemental(f.player,plan.caster_center?f.player:f.target,plan,20,100));
   if(skill==884){check(f.w.watch_yourself_armor(f.target,100)==watch.armor);check(f.w.elemental(f.player,f.target,plan,20,100));}
   if(skill==186)f.w.tick(1000);
  }
  if(skill==89){ // One cold hit plus one armor-ignoring shadow hit spends only one charge.
   for(unsigned n=1;n<watch.charges;++n){check(f.w.watch_yourself_armor(f.target,100)==watch.armor);check(f.w.lightning(f.player,f.target,*lightning(224,12),20,100));}
  }
  check(f.w.watch_yourself_armor(f.target,skill==186?1000:100)==0);
 }
 for(bool blind:{false,true}){Fixture f;const auto watch=*watch_yourself(0,true);
  check(f.w.stage_npc_watch_yourself_recipient(f.caster,f.target,watch,100));const auto entry=f.w.status_snapshot(f.target,100)->front();f.box->drain();
  combat::weapon::Owned bow{7,1,{20,20,1},12,false,false,5};
  check(f.w.ordinary_weapon(f.player,bow,20)&&f.w.status_mutation(f.player,431,1,12,100,24100,true));
  f.w.configure_attack_miss_draws([]{return .5;});
  if(blind)check(f.w.apply_condition(f.player,f.player,combat::conditions::Kind::blind,100,2000));
  check(f.w.ordinary_attack(f.player,101,100));f.w.tick(600);f.w.tick(1000);
  const double expected=blind?15*std::exp2(-5./40.):20*std::exp2(-5./40.)+15;
  check(std::abs(10000-f.w.npc_combat_snapshot(f.target)->health->current-expected)<1e-7);
  check(f.w.watch_yourself_armor(f.target,1000)==0);
  world::Packet end{67,0};combat::detail::put(end,101);combat::detail::put(end,entry.instance);const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),end)==1);
 }
 std::cout<<"PASS42real World NPC spell-hit charge lifecycles,final-hit armor,one end,armor-ignoring/bypass exclusions and death cleanup;NPC shout payment/party remains staged\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


