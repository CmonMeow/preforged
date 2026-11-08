#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Watch sequence line "+std::to_string(at.line()));}
int main()try{
 for(bool stacked:{false,true})for(unsigned skill:{937u,174u})for(unsigned rank:{0u,12u,20u}){world::Instance w;
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{10000,10000,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto source=*w.effect_scope(1),target=*w.effect_scope(2),foe=*w.effect_scope(3);
  check(w.combat_armor(source,80)&&w.combat_armor(target,80)&&w.lightning_armor(target,80));
  if(stacked){check(w.status_mutation(target,100,1001,0,100,30000,true));check(w.complete_convert_hexes(source,target,*convert_hexes(12),0,100));}
  const auto watch=*watch_yourself(rank,true);check(w.stage_watch_yourself_recipient(source,target,watch,100));
  for(unsigned n=1;n<watch.charges;++n)check(w.lightning(foe,target,*lightning(224,12),20,100));
  if(skill==174)check(w.gale(foe,target,12,{},100));
  const auto before=w.resources(2)->health;const auto plan=*caster_earth_aoe(skill,12);
  check(w.shockwave(foe,plan,20,100));const auto loss=before-w.resources(2)->health;
  const unsigned other=stacked?10:0,first_bonus=stacked?(rank==0?15:rank==12?25:32):watch.armor;
  const double first=plan.strikes[0].earth_damage*std::exp2(-(20.+first_bonus)/40.);
  const double later=skill==937?plan.strikes[1].earth_damage*std::exp2(-(20.+other)/40.)+plan.strikes[2].earth_damage*std::exp2(-double(other)/40.):plan.strikes[0].knocked_down_bonus*std::exp2(-(20.+other)/40.);
  if(std::abs(double(loss)-first-later)>=1.1)std::cerr<<"skill "<<skill<<" rank "<<rank<<" loss "<<loss<<" expected "<<first+later<<'\n';
  check(std::abs(double(loss)-first-later)<1.1&&w.watch_yourself_armor(target,100)==0);
 }
 {world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=90;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{10000,10000};d.damageable=true;n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{10000,10000,100,100},100);}
  const auto target=*w.effect_scope(1),foe=*w.effect_scope(2);check(w.combat_armor(target,80)&&w.lightning_armor(target,80));
  check(w.stage_watch_yourself_recipient(target,target,*watch_yourself(0,true),100));
  check(!w.shockwave(foe,*caster_earth_aoe(937,12),20,100)); // Unreviewed NPC Earth armor rejects after player preview.
  check(w.resources(1)->health==10000&&w.watch_yourself_armor(target,100)==5);
  check(w.lightning(foe,target,*lightning(224,12),20,100)&&w.watch_yourself_armor(target,100)==0);
 }
 std::cout<<"PASS Shockwave/Aftershock next-hit armor changes after last Watch Yourself charge at three ranks\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


