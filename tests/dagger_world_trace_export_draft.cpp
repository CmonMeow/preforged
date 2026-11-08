// DETACHED trace exporter: stdout JSONL is actual staged World output only.
// No cast/failure packet is invented to fill a missing Runtime producer.
#include "gw_world_simulation.h"
#include "dagger_retained_owner_fixture_draft.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
using namespace fw::gw;
static void check(bool ok){if(!ok)throw std::runtime_error("dagger World trace fixture");}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source,target;
 std::unique_ptr<test::RetainedDaggerOwnerDraft> paid_owner;std::uint64_t contact_at=1000;
 combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,32,0.};bool npc;
 Fixture(bool is_npc):npc(is_npc){
  w.configure_attack_miss_draws([]{return .5;});w.configure_attack_critical_draws([]{return .999999;});
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={1000,1000};d.damageable=d.conditions_allowed=true;
   d.physical_armor=60;d.fleshy=true;d.fleshy_evidence="authored dagger trace recipient";n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Assassin",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,30,30},100);source=*w.effect_scope(1);
  if(!npc){w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,30,30},100);target=*w.effect_scope(2);check(w.physical_armor(target,60));
   check(w.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{1,0,20}));}
  else target=*w.effect_scope(100);
  check(w.ordinary_weapon(source,weapon,20,100,combat::weapon::CriticalProfile{7,0,20}));box->drain();
 }
 bool hit(unsigned skill,std::uint64_t at){const auto plan=combat::weapon::attack_skill(skill,0,0,0,true);check(bool(plan));
  if(combat::weapon::dagger_dual_role(skill)){if(!paid_owner)paid_owner=std::make_unique<test::RetainedDaggerOwnerDraft>(w,source,box);
   const bool accepted=paid_owner->execute(target,skill,0,at);contact_at=paid_owner->final_contact_at();return accepted;}
  contact_at=at;return w.melee_attack(source,target,weapon,*plan,at,{},true);
 }
 void emit(const std::string& name,unsigned skill,bool accepted,std::uint64_t at){
  at=std::max(at,contact_at);const auto packets=box->drain();const double hp=npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);
  std::cout<<"{\"origin\":\"generated_actual_world\",\"case\":\""<<name<<"\",\"skill\":"<<skill<<",\"source\":1,\"target\":"<<target.actor
   <<",\"time\":"<<at<<",\"accepted\":"<<(accepted?"true":"false")<<",\"stage\":"<<w.dagger_stage(source,target,at)<<",\"health\":"<<std::setprecision(15)<<hp<<",\"packets\":[";
  for(std::size_t i=0;i<packets.size();++i){if(i)std::cout<<',';std::ostringstream hex;for(const auto byte:packets[i])hex<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(byte);std::cout<<'"'<<hex.str()<<'"';}
  std::cout<<"]}\n";
 }
};
int main()try{
 for(bool npc:{false,true}){
  const std::string prefix=npc?"npc":"player";
  {Fixture f(npc);for(unsigned skill:{782u,780u,776u}){const bool ok=f.hit(skill,1000);check(ok);f.emit(prefix+"_chain",skill,ok,1000);}f.w.tick(18000);f.emit(prefix+"_chain",776,true,18000);}
  {Fixture f(npc);const bool ok=f.hit(780,1000);check(!ok);f.emit(prefix+"_invalid_offhand",780,ok,1000);}
  {Fixture f(npc);check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));f.box->drain();const bool ok=f.hit(782,1000);check(ok);f.emit(prefix+"_missed_lead",782,ok,1000);}
 }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
