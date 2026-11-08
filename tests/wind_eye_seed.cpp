#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
namespace effects=skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Wind/Eye Seed line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;unsigned skill,mode;effects::Scope source,owner,side,monk;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();bool action=false;
 Fixture(unsigned id,unsigned hp,unsigned rank,unsigned behavior,bool unknown=false,float eye_distance=220.f):skill(id),mode(behavior){
  const float x=skill==163?220.f:60.f+eye_distance;
  if(unknown){world::Instance::Npc n;n.id=n.definition.id=100;n.x=x+50;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};d.damageable=true;n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned actor:{1u,2u,3u,4u}){w.join(actor,"Actor",actor==1?0.f:actor==2?60.f:actor==3?x:x+150,0,actor==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,actor==1?1:2);w.set_vitals(actor,{actor==1?1000u:actor==3?hp:500u,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(actor),60,60));}
  source=*w.effect_scope(1);owner=*w.effect_scope(2);side=*w.effect_scope(3);monk=*w.effect_scope(4);
  check(w.party_action(3,158,4,100)&&w.party_action(4,154,3,100));check(w.complete_healing_seed(monk,side,*effects::healing_seed(rank),0,100));
  if(skill==163){if(mode)check(w.activate_overcast_energy(source,219,0,1,100));check(w.attack_activity(side,1000,150));}
  else{if(mode)check(w.status_mutation(owner,mode==1?16:17,950,12,100,10100,true));check(w.apply_wandering_eye(source,owner,12,100));}
  box->drain();
 }
 bool side_hit()const{return skill!=163||mode;}
 bool shares_heal()const{return skill==163;}
 double damage()const{return skill==163?effects::elemental(163,12)->damage:effects::wandering_eye(12)->damage;}
 void rr(int rank){if(rank>=0)check(w.status_mutation(side,307,900,rank,150,8150,true));box->drain();}
 void source_state(unsigned state){if(state==1)check(w.kill(source,150));if(state>=2)w.leave(1);if(state==3){w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},150,2);w.set_vitals(1,{1000,1000,100,100},100);}box->drain();}
 bool cast(){if(skill==163)return w.elemental(source,source,*effects::elemental(163,12),20,200);action=w.attack_activity(owner,1000,200);const auto status=*w.status_snapshot(owner,200);return std::none_of(status.begin(),status.end(),[](const auto& e){return e.skill==2056;});}
 unsigned hp()const{return w.resources(3)->health;}unsigned ally()const{return w.resources(4)->health;}
};
static unsigned radius_cases(){
 unsigned cases=0;
 for(float distance:{251.99f,252.f,252.01f,280.f})for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u}){
  Fixture f(2056,500,rank,1,false,distance);f.rr(rr);check(f.cast());const auto raw=distance<=252.f?f.damage():0.,converted=rr<0?0.:std::min(raw,double(effects::reversal_fortune(rr)->maximum_healing));const bool trigger=raw>converted;const auto healing=effects::healing_seed(rank)->healing;
  check(f.hp()==500+converted-(raw-converted)+(trigger?healing:0)&&f.ally()==500+(trigger?healing:0));++cases;
 }
 for(float distance:{251.99f,252.f,252.01f,280.f})for(int rr:{-1,0,12,20}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60.f+distance;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={500,1000};d.damageable=true;d.healing_allowed=true;d.healing_evidence="synthetic radius fixture";n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0.f:60.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,1000,100,100},100);}
  const auto source=*w.effect_scope(1),owner=*w.effect_scope(2),side=*w.effect_scope(100);if(rr>=0)check(w.stage_npc_reversal(side,rr,100));check(w.apply_wandering_eye(source,owner,12,100));check(!w.attack_activity(owner,1000,200));
  const double raw=distance<=252.f?94.:0.,converted=rr<0?0.:std::min(raw,double(effects::reversal_fortune(rr)->maximum_healing));check(w.npc_combat_snapshot(side)->health->current==500+converted-(raw-converted));++cases;
 }
 check(cases==64);return cases;
}
int main()try{
 unsigned cases=0;
 for(unsigned skill:{163u,2056u})for(unsigned mode=0;mode<(skill==163?2u:3u);++mode)for(int rr:{-1,0,12,20})for(unsigned hp:{1u,10u,500u,1000u})for(unsigned rank:{0u,12u,20u})for(unsigned state=0;state<(skill==163?1u:4u);++state){
  Fixture f(skill,hp,rank,mode);f.rr(rr);f.source_state(state);check(f.cast());const auto raw=f.side_hit()?f.damage():0.,converted=rr<0?0.:std::min(raw,double(effects::reversal_fortune(rr)->maximum_healing));
  const auto healed=std::min(1000.,hp+converted),loss=std::min(healed,raw-converted);const bool triggers=loss>0;const auto heal=effects::healing_seed(rank)->healing;
  const auto wanted=healed-loss>0?std::min(1000.,healed-loss+(triggers?heal:0)):0.;check(std::abs(double(f.hp())-wanted)<1.01);
  check(f.w.resources(2)->health==500-f.damage()+(triggers&&f.shares_heal()?heal:0)&&f.ally()==500+(triggers?heal:0));
  if(skill==163)check(f.w.knocked_down(f.side,200)==bool(f.side_hit()&&wanted>0));else check(f.action==bool(mode));
  if(!wanted)check(f.w.player_death_times({3,4})[0]==200);f.w.tick(201);check(f.ally()==500+(triggers?heal:0));
  if(skill==2056){const auto before=f.hp();check(f.w.attack_activity(f.owner,1000,300));check(f.hp()==before);if(state==3)check(f.w.resources(1)->health==1000-f.damage());}++cases;
 }
 for(unsigned skill:{163u,2056u})for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u}){
  Fixture f(skill,500,rank,1);f.rr(rr);check(f.w.complete_healing_seed(f.monk,f.owner,*effects::healing_seed(rank),0,150));check(f.cast());
  const auto raw=f.damage(),converted=rr<0?0.:std::min(raw,double(effects::reversal_fortune(rr)->maximum_healing));const bool second=raw>converted;const auto heal=effects::healing_seed(rank)->healing;
  check(f.w.resources(2)->health==500-raw+heal+(second&&f.shares_heal()?heal:0));check(f.hp()==500+converted-(raw-converted)+(f.shares_heal()?heal:0)+(second?heal:0));check(f.ally()==500+(second?heal:0));++cases;
 }
 for(unsigned skill:{163u,2056u})for(int rr:{-1,0})for(unsigned rank:{0u,12u,20u}){
  Fixture f(skill,500,rank,1,true);f.rr(rr);check(!f.cast());check(f.hp()==500&&f.w.resources(2)->health==500&&f.ally()==500&&f.box->drain().empty());
  if(skill==2056){const auto status=*f.w.status_snapshot(f.owner,200);check(std::any_of(status.begin(),status.end(),[](const auto& e){return e.skill==2056;})&&std::any_of(status.begin(),status.end(),[](const auto& e){return e.skill==16;}));}
  check(f.w.status_mutation(f.side,307,901,20,200,8200,true));check(f.cast());check(f.hp()==500+f.damage()&&f.w.resources(2)->health==500-f.damage()&&f.ally()==500);++cases;
 }
 for(unsigned skill:{163u,2056u})for(int rr:{0,12,20})for(unsigned rank:{0u,12u,20u}){
  Fixture f(skill,10,rank,1);f.rr(rr);check(f.w.apply_condition(f.source,f.side,combat::conditions::Kind::deep_wound,150,10000));const auto epoch=*f.w.control_epoch(f.side);check(f.cast());check(f.hp()==0&&f.ally()==500&&*f.w.control_epoch(f.side)==epoch+1);f.w.tick(201);check(*f.w.control_epoch(f.side)==epoch+1);++cases;
 }
 for(unsigned skill:{163u,2056u})for(unsigned rank:{0u,12u,20u}){
  Fixture f(skill,500,rank,1);f.w.set_vitals(4,{10,1000,100,100},100);check(f.w.apply_condition(f.source,f.monk,combat::conditions::Kind::deep_wound,150,10000));const auto epoch=*f.w.control_epoch(f.monk);check(f.cast());check(f.ally()==0&&*f.w.control_epoch(f.monk)==epoch+1&&f.w.player_death_times({4,3})[0]==200);f.w.tick(201);check(*f.w.control_epoch(f.monk)==epoch+1);++cases;
 }
 cases+=radius_cases();check(cases==796);std::cout<<"PASS "<<cases<<" Whirlwind/Wandering Eye Seed cases: Overcast,interrupt protection,one-shot,source generation,conversion,range,preflight and death cleanup\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
