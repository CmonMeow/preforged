#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Bile lifetime line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;effects::Scope caster,dead,target;
 Fixture(){for(unsigned id:{1u,2u,3u})join(id,100);caster=*w.effect_scope(1);dead=*w.effect_scope(2);target=*w.effect_scope(3);}
 void join(unsigned id,unsigned at){w.join(id,"Actor",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},at,id==1?1:2);w.set_vitals(id,{500,1000,100,100},100);}
 void bile(unsigned at=100){check(w.degeneration_hex(caster,dead,*effects::degeneration_hex(2058,0),at,20));}
 // Deliberately pre-existing future observations exercise retained-event
 // refusal; the proactive scheduler now prevents creating them AFTER enrollment.
 void pending(){check(w.settle_resources(target,5000));bile();check(w.kill(dead,101));check(w.pending_putrid_deaths()==1&&w.resources(3)->health==500);}
};
int main()try{
 {
  Fixture f;f.pending();const auto old=f.target;f.w.leave(3);f.join(3,5000);const auto replacement=*f.w.effect_scope(3);check(replacement!=old);
  f.w.tick(6000);check(!f.w.pending_putrid_deaths()&&f.w.resources(3)->health==500);
 }
 {
  Fixture f;f.pending();const auto old=f.caster;f.w.leave(1);check(!f.w.pending_putrid_deaths());f.join(1,5000);
  check(*f.w.effect_scope(1)!=old);f.w.tick(6000);check(f.w.resources(3)->health==500&&!f.w.pending_putrid_deaths());
 }
 {
  Fixture f;f.pending();f.w.leave(2);f.w.leave(3);f.join(2,5000);check(*f.w.effect_scope(2)!=f.dead);
  f.join(3,5000);f.w.tick(6000);
  check(!f.w.pending_putrid_deaths()&&f.w.resources(2)->health==500&&f.w.resources(3)->health==500);
 }
 {
  Fixture f;f.bile();check(f.w.kill(f.dead,101));check(f.w.resources(3)->health==475&&!f.w.pending_putrid_deaths());
  check(!f.w.kill(f.dead,102));f.w.tick(201);check(f.w.resources(3)->health==475);
  check(f.w.resurrect(f.dead,301));check(*f.w.effect_scope(2)==f.dead);
  f.bile(301);check(f.w.kill(f.dead,302));check(f.w.resources(3)->health==450&&!f.w.pending_putrid_deaths());
 }
 {
  Fixture f;check(f.w.kill(f.target,5000));f.bile();
  check(f.w.kill(f.dead,101));check(f.w.pending_putrid_deaths()==1);
  check(!f.w.putrid_death_diagnostics().empty());f.w.tick(6000);check(f.w.pending_putrid_deaths()==1);
  f.w.leave(1);check(!f.w.pending_putrid_deaths()&&f.w.putrid_death_diagnostics().empty());
 }
 for(bool npc:{false,true}){
  world::Instance w;std::vector<world::Instance::Npc> population;
  if(npc)for(unsigned id=100;id<165;++id){
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={500,1000};d.damageable=true;
   n.combat_definition=d;population.push_back(n);
  }
  w.configure_npcs(population);
  for(unsigned id=1;id<=(npc?2u:67u);++id){w.join(id,"Actor",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,1000,100,100},100);}
  const auto caster=*w.effect_scope(1),dead=*w.effect_scope(2);
  check(w.degeneration_hex(caster,dead,*effects::degeneration_hex(2058,0),100,20));check(w.kill(dead,101));
  check(!w.pending_putrid_deaths());
  if(npc){for(unsigned id=100;id<165;++id)check(w.npc_combat_snapshot(*w.effect_scope(id))->health->current==475);}
  else for(unsigned id=3;id<=67;++id)check(w.resources(id)->health==475);
 }
 std::cout<<"PASS Bile retained future-clock event, departed/replaced lifetimes and new post-resurrection death\n";
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}
