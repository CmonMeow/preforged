#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Shield Regeneration line "+std::to_string(at.line()));}
combat::npcs::LiveActor make_npc(){combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{100,2000};d.damageable=d.conditions_allowed=true;d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=d.physical_armor=d.general_armor=60;return {d,{1,1,1,146,100}};}
int main()try{const Scope caster{1,1,1};
 for(unsigned rank=0;rank<=20;++rank)for(unsigned mode=0;mode<3;++mode){const auto plan=*shield_regeneration(rank);auto n=make_npc();const auto life=n.lifetime();
  check(plan.armor==40&&plan.pips==interpolate(3,10,rank)&&plan.duration==1000*interpolate(5,13,rank));auto bad=plan;++bad.pips;check(!n.apply_shield_regeneration(life,caster,bad,1,1000));
  check(n.apply_shield_regeneration(life,caster,plan,1,1000));check(!n.apply_shield_regeneration(life,caster,plan,1,1000));
  check(n.final_earth_armor(life,1000)==100&&n.final_physical_armor(life,1000)==100&&n.final_caster_armor(life,3,1000)==100);
  auto hit=n.apply_damage(life,{combat::Kind::cold,100,20,144},1000);check(hit.reason==combat::Reason::accepted&&hit.damage==50);
  const auto until=1000+plan.duration;
  if(mode==0)for(unsigned at=1100;at<=until;at+=100)check(bool(n.advance_conditions(life,at)));
  else if(mode==1)n.refresh_hexes(life,until);
  else {check(n.remove_reviewed_enchantment(life,1,2000));const auto ends=n.take_enchantment_ends(life);check(ends.size()==1&&ends[0].entry.skill==261&&ends[0].gained==0);check(bool(n.advance_conditions(life,until)));}
  const auto gained=2.*std::min(10u,plan.pips)*(mode==2?1000:plan.duration)/1000.;check(std::abs(n.actor(life)->health->current-(50+gained))<1e-7);
  check(n.final_physical_armor(life,until)==60);const auto before=n.actor(life)->health->current;check(bool(n.advance_conditions(life,until+1000)));check(n.actor(life)->health->current==before);
 }
 // Combined regeneration caps at10pips;strong single-source40armor is
 // retained rather than summing with Deflection's27or truncating to25.
 {auto n=make_npc();const auto life=n.lifetime();check(n.apply_shield_regeneration(life,caster,*shield_regeneration(20),1,1000));check(n.apply_shield_deflection(life,caster,*shield_deflection(12),2,1000));check(n.apply_watchful_healing(life,*watchful_healing(12),1,3,1000));check(n.final_physical_armor(life,1000)==100);check(bool(n.advance_conditions(life,2000)));check(n.actor(life)->health->current==120);}
 for(bool npc:{false,true})for(bool dense:{false,true}){world::Instance w;auto box=std::make_shared<world::Outbox>();
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{100,2000};n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{100,2000,100,100},100);const auto source=*w.effect_scope(1),target=npc?*w.effect_scope(100):source;box->drain();
  check(w.complete_shield_regeneration(source,target,*shield_regeneration(12),0,1000));const auto packets=box->drain();const auto added=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==20&&p[0]==65&&p[6]==5&&p[7]==1;});check(added!=packets.end());
  const unsigned effect=unsigned((*added)[12])|(unsigned((*added)[13])<<8)|(unsigned((*added)[14])<<16)|(unsigned((*added)[15])<<24);
  if(dense)for(unsigned at=1100;at<=12000;at+=100)w.tick(at);else w.tick(12000);
  const auto health=npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(1)->health);check(std::abs(health-298)<1e-7);
  const auto ended=box->drain();check(std::count(ended.begin(),ended.end(),skills::casting::effect_remove(target.actor,effect))==1);w.tick(13000);check(std::abs((npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(1)->health))-298)<1e-7);
 }
 check(!shield_regeneration(21));std::cout<<"PASS63 rank/NPC armor,regeneration/removal cadence cases,combined pips/armor and4World player/NPC expiry cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
