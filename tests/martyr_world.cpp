#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;using combat::conditions::Kind;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Martyr World line "+std::to_string(at.line()));}
struct Fixture {
 world::Instance w;Scope source,ally,foe,far;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(){for(unsigned id=1;id<=4;++id){w.join(id,"Actor",id==4?5031:float(id*30),0,id==1?box:std::make_shared<world::Outbox>(),0,id==1?std::vector<world::Packet>{{165,0,1,0,0,0,3,0}}:std::vector<world::Packet>{},{},100,id==3?2:1);w.set_vitals(id,{500,1000,100,100},100);}source=*w.effect_scope(1);ally=*w.effect_scope(2);foe=*w.effect_scope(3);far=*w.effect_scope(4);}
};
int main()try{
 for(unsigned divine:{0u,12u,20u}){Fixture f;
  for(unsigned i=0;i<10;++i){check(f.w.apply_condition(f.foe,f.ally,Kind(i),100,1000+i));check(f.w.apply_condition(f.foe,f.foe,Kind(i),100,1000+i));}
  check(f.w.apply_condition(f.foe,f.far,Kind::blind,100,1000));
  check(f.w.complete_martyr(f.source,*martyr(),divine,100));
  for(unsigned i=0;i<10;++i){check(f.w.has_condition(f.source,Kind(i),100)&&!f.w.has_condition(f.ally,Kind(i),100)&&f.w.has_condition(f.foe,Kind(i),100));}
  check(f.w.has_condition(f.far,Kind::blind,100));check(f.w.resources(1)->max_health==900&&f.w.resources(2)->max_health==1000);
  check(f.w.resources(1)->health==unsigned(std::ceil(400.+.8*std::lround(3.2*divine))));
  f.w.tick(1200);for(unsigned i=0;i<10;++i)check(!f.w.has_condition(f.source,Kind(i),1200));check(f.w.resources(1)->max_health==1000);
 }
 {Fixture f;check(f.w.apply_condition(f.foe,f.ally,Kind::blind,100,1000));auto stale=f.source;++stale.generation;auto forged=*martyr();++forged.range;
  check(!f.w.complete_martyr(stale,*martyr(),0,100)&&!f.w.complete_martyr(f.source,forged,0,100)&&!f.w.complete_martyr(f.source,*martyr(),21,100));check(f.w.has_condition(f.ally,Kind::blind,100));}
 // Lethal Fragility on the caster must stop later condition applications and
 // preserve donors not yet processed;no stale final-state assignment resurrects.
 {Fixture f;check(f.w.degeneration_hex(f.foe,f.source,*degeneration_hex(19,12),100));f.w.set_vitals(1,{10,1000,100,100},100);
  check(f.w.apply_condition(f.foe,f.ally,Kind::blind,100,1000)&&f.w.apply_condition(f.foe,f.ally,Kind::poison,100,1000));
  check(f.w.complete_martyr(f.source,*martyr(),12,100));check(f.w.resources(1)->health==0);
  check(f.w.has_condition(f.ally,Kind::blind,100)&&!f.w.has_condition(f.ally,Kind::poison,100));
  f.w.tick(200);check(f.w.resources(1)->health==0);
 }
 {world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{500,1000};d.damageable=d.conditions_allowed=true;d.fleshy=true;d.fleshy_evidence="fixture";d.creature_type="human";d.creature_type_evidence="fixture";d.disease_immune=false;d.disease_immunity_evidence="fixture";n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?1000.f:0.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,1000,100,100},100);}
  const auto source=*w.effect_scope(1),infected=*w.effect_scope(2),ally=*w.effect_scope(100);
  check(w.apply_condition(source,infected,Kind::disease,100,1000));w.tick(200);check(w.has_condition(ally,Kind::disease,200));
  check(w.complete_martyr(source,*martyr(),0,200));check(!w.has_condition(ally,Kind::disease,200)&&w.has_condition(source,Kind::disease,200));
  check(!w.has_condition(source,Kind::disease,1100));
 }
 std::cout<<"PASS Martyr World ally/foe/range filters,all conditions,DeepWound/expiry,DivineFavor,identity and lethal reaction ordering\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}



