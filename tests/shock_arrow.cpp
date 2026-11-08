#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Shock Arrow World line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source,target;bool npc;
 Fixture(bool is_npc,bool armor=true,unsigned health=1000,float distance=200):npc(is_npc){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=distance;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(health),1000};d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="fixture";
  if(armor){d.lightning_armor=80;d.lightning_armor_evidence="fixture";}n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,50,100},100);
  check(w.energy_capacity(1,6,0,100,{80,-2}));check(w.resources(1)->energy==50&&w.resources(1)->max_energy==100);
  if(!npc){w.join(2,"Foe",distance,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{health,1000,100,100},100);
   check(w.lightning_armor(*w.effect_scope(2),armor?std::optional<int>{80}:std::nullopt));}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);check(w.configure_projectiles({100,32}));box->drain();
 }
 double health(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(target.actor)->health);}
 void crack(std::uint64_t now){check(w.lightning(source,target,*skills::effects::lightning(865,0),20,now));}
};
int main()try{
 using namespace skills;
 // Successful impacts retain conditional Energy even if Reversal converts
 // all damage;healing-debt death is not an invented damaging Hit.
 for(bool cracked:{false,true})for(bool wounded:{false,true}){Fixture f(true,true,10);
  if(cracked)check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::cracked_armor,100,10000));
  if(wounded)check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));
  if(cracked||wounded)check(f.w.condition_transfer(f.source,f.target,*effects::condition_transfer(132,12),100));
  check(f.w.stage_npc_reversal(f.target,12,100));
  check(f.w.lightning(f.source,f.target,*effects::lightning(1082,12,13),20,1000));f.box->drain();f.w.tick(1100);
  const double raw=41*(cracked?std::exp2(15./40.):1.);
  check(std::abs(f.health()-(wounded?0:10+raw))<1e-7&&f.w.resources(1)->energy==(cracked?61:50));
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),particles::shock_arrow_impact_2022(f.target.actor,1))==1);
  check(f.w.take_npc_deaths(f.source).size()==unsigned(wounded));f.w.tick(1200);check(f.w.resources(1)->energy==(cracked?61:50));
 }
 // Cracked Armor is observed at impact,including natural expiry at release.
 for(bool npc:{false,true})for(unsigned mode=0;mode<4;++mode){Fixture f(npc);
  if(mode==1)f.crack(5000);if(mode==2)f.crack(1000);
  const auto before=f.health();check(f.w.lightning(f.source,f.target,*effects::lightning(1082,12,13),20,6000));
  if(mode==3)f.crack(6050);
  const auto before_hit=f.health();f.box->drain();f.w.tick(6100);
  check(f.health()<before_hit&&f.health()<=before);
  if(f.w.resources(1)->energy!=50+(mode==1||mode==3?11u:0u))throw std::runtime_error("Shock Energy npc="+std::to_string(npc)+" mode="+std::to_string(mode)+" actual="+std::to_string(f.w.resources(1)->energy));
  const auto packets=f.box->drain();const auto impact=particles::shock_arrow_impact_2022(f.target.actor,1);
  check(std::count(packets.begin(),packets.end(),impact)==1);f.w.tick(6200);
  const auto later=f.box->drain();check(std::find(later.begin(),later.end(),impact)==later.end());
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<7;++mode){
  const bool miss=!npc&&mode==5;const float distance=miss?600.f:200.f;
  Fixture f(npc,true,mode==6?1:1000,distance);
  casting::Hooks h;unsigned releases=0;h.resources=[&](auto id){return f.w.resources(id);};
  h.events=[&](const auto& p){f.w.skill_event(1,p);};
  h.consume_energy_at=[&](auto amount,auto now){check(amount==5);return f.w.consume_energy(1,amount,f.source,now);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float){return id==f.target.actor;};
  if(mode!=3)h.lightning=[&](auto target,const auto& plan,auto now)->casting::EffectCompletion{
   check(plan.storage==13&&plan.energy_if_cracked==11);std::vector<world::Packet> out;const bool ok=f.w.lightning(f.source,target,plan,20,now,&out);
   if(ok)++releases;return {ok,std::move(out)};};
  Slots bar{};bar[0]={1082,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[1082]=1;
  attributes::Ranks ranks{};ranks[8]=12;ranks[12]=13;casting::Runtime runtime(1);
  check(runtime.start(1000,{1082,0,f.target.actor},bar,policy,ranks,true,h)==(mode==3?casting::Reason::unsupported:casting::Reason::accepted));
  check(f.w.resources(1)->energy==(mode==3?50u:45u));if(mode==3)continue;
  if(mode==1)runtime.cancel(h,1500);if(mode==2)runtime.interrupt(1500,h);
  runtime.update(1999,h);check(!releases);runtime.update(2000,h);const bool released=mode!=1&&mode!=2;check(releases==unsigned(released));
  auto packets=f.box->drain();const auto launch=particles::spell_projectile_launch_2022(1,distance,0,0,miss?.25f:.084f,1082);
  check(std::count(packets.begin(),packets.end(),launch)==unsigned(released));
  if(released)check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))<std::find(packets.begin(),packets.end(),launch));
  if(mode==4){f.w.leave(1);f.w.join(1,"Replacement",0,0,f.box,0,{}, {},2000,1);f.w.set_vitals(1,{1000,1000,50,100},100);}
  // The84ms short flight permits less than32units of normal movement. Use
  // a250ms flight so the target can actually leave the reviewed hit bubble.
  if(miss)f.w.move(2,distance,500,2001);
  f.w.tick(miss?2300:2200);const bool hit=released&&mode!=4&&!miss;
  if((f.health()<(mode==6?1:1000))!=hit)throw std::runtime_error("Shock hit npc="+std::to_string(npc)+" mode="+std::to_string(mode)+" health="+std::to_string(f.health())+" expectedHit="+std::to_string(hit));
  packets=f.box->drain();check(std::count(packets.begin(),packets.end(),particles::shock_arrow_impact_2022(f.target.actor,1))==unsigned(hit));
 }
 for(bool npc:{false,true}){Fixture f(npc,false);check(!f.w.lightning(f.source,f.target,*effects::lightning(1082,12,13),20,1000));check(f.health()==1000);}
 std::cout<<"PASS Shock Arrow real player/NPC impact condition timing,Energy return,Runtime/World release,cancel,interrupt,missing hook,source reuse,miss,lethal,unknown armor and exact1179/1181 publication\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
