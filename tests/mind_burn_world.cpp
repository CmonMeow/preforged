#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Mind Burn World line "+std::to_string(at.line()));}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope source,target,npc;
 Fixture(unsigned enemy=20,bool known=true,bool armor=true){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=200;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;
  if(armor){d.fire_armor=60;d.fire_armor_evidence="test";}if(known){d.spawn_energy=combat::npcs::Energy{enemy,100};d.energy_pips=0;d.energy_evidence=d.energy_pips_evidence="test";}
  n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id=1;id<=5;++id){const float x=id==1?0:id==2?100:id==3?250:267;
   w.join(id,"Actor",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);
   w.set_vitals(id,{1000,1000,id==1?80u:id==3?90u:20u,100},100);check(w.energy_capacity(id,6,0,100,{80,-2}));
   check(w.elemental_armor(*w.effect_scope(id),60,{}));}
  source=*w.effect_scope(1);target=*w.effect_scope(2);npc=*w.effect_scope(100);box->drain();
 }
};
int main()try{
 for(unsigned rank:{0u,12u,20u})for(unsigned e:{20u,80u,90u}){
  Fixture f(e);const auto plan=*effects::mind_burn(185,rank);std::vector<world::Packet> out;
  check(f.w.mind_burn(f.source,f.target,plan,20,1000,&out));
  check(f.w.resources(2)->health==1000-plan.damage*2&&f.w.has_condition(f.target,combat::conditions::Kind::burning,1000));
  check(f.w.resources(3)->health==1000-plan.damage&&!f.w.has_condition(*f.w.effect_scope(3),combat::conditions::Kind::burning,1000));
  check(f.w.resources(4)->health==1000&&f.w.resources(5)->health==1000&&f.w.resources(1)->health==1000);
  check(f.w.npc_combat_snapshot(f.npc)->health->current==1000-plan.damage*(e<80?2:1));
  check(f.w.has_condition(f.npc,combat::conditions::Kind::burning,1000)==(e<80));
  check(std::count(out.begin(),out.end(),*particles::targeted_body_2022(2,1,185))==1);
 }
 for(unsigned mode:{0u,1u}){Fixture f(20,mode!=0,mode!=1);check(!f.w.mind_burn(f.source,f.target,*effects::mind_burn(185,12),20,1000));
  check(f.w.resources(2)->health==1000&&f.w.resources(3)->health==1000&&f.w.npc_combat_snapshot(f.npc)->health->current==1000);}
 for(bool npc_target:{false,true})for(unsigned mode=0;mode<7;++mode){Fixture f;const auto target_id=npc_target?100u:2u;casting::Hooks h;bool stale=false;unsigned paid=0,commits=0;
  h.events=[&](const auto& packets){f.w.skill_event(1,packets);};h.resources=[&](auto id){return f.w.resources(id);};
  h.target_scope=[&](auto id){return stale&&id==target_id?std::optional<effects::Scope>{}:f.w.effect_scope(id);};
  h.projectile_target_valid=[](auto id,float range){return (id==2||id==100)&&range==1248;};
  if(mode!=6)h.consume_overcast_energy=[&](auto id,auto amount,auto event,auto at){++paid;check(id==185&&amount==5);return f.w.activate_overcast_energy(f.source,id,amount,event,at);};
  if(mode!=4)h.mind_burn=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{++commits;
   if(mode==5)return {false,{*particles::targeted_body_2022(target_id,1,185)}};
   std::vector<world::Packet> out;std::function<void()> publish;const bool ok=f.w.mind_burn(f.source,target,plan,20,at,&out,&publish);return {ok,std::move(out),{},std::move(publish)};};
  Slots bar{};bar[0]={185,0};Policy p;p.primary=6;p.campaigns=255;p.learned[185]=1;attributes::Ranks ranks{};ranks[10]=12;
  casting::Runtime runtime(1);const bool missing=mode==4||mode==6;
  check(runtime.start(1000,{185,0,target_id},bar,p,ranks,true,h)==(missing?casting::Reason::unsupported:casting::Reason::accepted));
  check(paid==unsigned(!missing)&&f.w.resources(1)->energy==(missing?80u:75u));if(missing)continue;
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1100,h);if(mode==3)stale=true;
  runtime.update(1999,h);check(!commits);runtime.update(2000,h);check(commits==unsigned(mode==0||mode==5));
  const auto packets=f.box->drain();const auto visual=*particles::targeted_body_2022(target_id,1,185);
  check(std::count(packets.begin(),packets.end(),visual)==unsigned(mode==0));
  if(mode==0)check(f.w.resources(2)->health==898&&std::find(packets.begin(),packets.end(),casting::terminal(58,1))<std::find(packets.begin(),packets.end(),visual));
 }
 std::cout<<"PASS Mind Burn mixed adjacent foes,per-recipient Energy,unknown state atomicity and Runtime payment/cancel/visual order\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
