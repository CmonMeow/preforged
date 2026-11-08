#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Shock Arrow Energy line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;skills::effects::Scope source,target;bool npc;
 Fixture(bool is_npc,unsigned current=50,unsigned health=1000):npc(is_npc){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=200;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(health),1000};d.damageable=d.conditions_allowed=true;
  d.lightning_armor=80;d.lightning_armor_evidence="fixture";n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,current,100},100);
  // All professions have2innate pips here. Armor-2makes the source clock flat.
  check(w.energy_capacity(1,6,0,100,{80,-2}));check(w.resources(1)->energy==current&&w.resources(1)->max_energy==100);
  if(!npc){w.join(2,"Foe",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{health,1000,100,100},100);
   check(w.lightning_armor(*w.effect_scope(2),80));}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);check(w.configure_projectiles({100,32}));
 }
 void crack(){check(w.lightning(source,target,*skills::effects::lightning(865,0),20,1900));}
 void launch(){check(w.lightning(source,target,*skills::effects::lightning(1082,12,13),20,2000));}
 void impact(){w.tick(2084);}
 overcast::Settlement energy(){auto state=w.overcast_energy_snapshot(source);check(bool(state));return *state;}
 double health(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(target.actor)->health);}
};
int main()try{
 for(bool npc:{false,true}){
  // Ordinary equipment capacity preserves the deficit. Overcast application
  // clips to its ceiling;it is not an unconditional Energy subtraction.
  Fixture f(npc,20);f.crack();f.launch();
  check(f.w.activate_overcast_energy(f.source,231,0,1,2050));
  check(f.w.energy_capacity(1,6,0,2050,{50,-2})); //100->70nominal,20->-10current.
  const auto debt=f.energy();check(debt.debt==10&&overcast::precise_current(debt)==-10*std::int64_t(overcast::energy_unit));
  f.impact();const auto after=f.energy();check(after.debt==0&&overcast::precise_current(after)==std::int64_t(overcast::energy_unit));
  f.w.tick(2200);check(overcast::precise_current(f.energy())==std::int64_t(overcast::energy_unit));
 }
 for(bool npc:{false,true}){
  // An actual negative Overcast ceiling admits only its newly recovered
  // fraction,even though the conditional skill reward requests11Energy.
  Fixture f(npc);f.crack();f.launch();
  for(unsigned event=1;event<=11;++event)check(f.w.activate_overcast_energy(f.source,231,0,event,2050));
  const auto before=f.energy();check(overcast::precise_current(before)==-10*std::int64_t(overcast::energy_unit));
  f.impact();const auto after=f.energy();
  const auto ceiling=std::int64_t(after.resources.max_energy)*overcast::energy_unit-std::int64_t(after.curve.burden_units())*3;
  check(overcast::precise_current(after)==ceiling&&ceiling<0);
  check(overcast::precise_current(after)>overcast::precise_current(before)&&
        overcast::precise_current(after)-overcast::precise_current(before)<std::int64_t(overcast::energy_unit));
 }
 for(bool npc:{false,true}){
  Fixture reward(npc),control(npc);
  for(auto* f:{&reward,&control}){
   check(f->w.energy_capacity(1,6,0,100,{80,-1})); //1Energy pip,explicitly timed.
   check(f->w.activate_overcast_energy(f->source,231,0,1,1000));
  }
  reward.crack();reward.launch();control.launch();reward.impact();control.impact();
  const auto a=reward.energy(),b=control.energy();
  check(overcast::precise_current(a)-overcast::precise_current(b)==11*std::int64_t(overcast::energy_unit));
  check(a.regeneration.remainder()==b.regeneration.remainder()&&a.fractional_tail==b.fractional_tail);
  check(a.regeneration.remainder()!=0||a.fractional_tail!=0);
 }
 for(bool npc:{false,true}){
  Fixture f(npc,100);f.crack();f.launch();f.impact();check(f.w.resources(1)->energy==100);
  // Ceiling recovery during flight admits only the fractional newly available cap.
  Fixture capped(npc,100);capped.crack();capped.launch();check(capped.w.activate_overcast_energy(capped.source,231,0,1,2050));
  const auto before=capped.energy();capped.impact();const auto after=capped.energy();
  const auto ceiling=std::int64_t(after.resources.max_energy)*overcast::energy_unit-std::int64_t(after.curve.burden_units())*3;
  check(overcast::precise_current(after)==ceiling&&overcast::precise_current(after)>overcast::precise_current(before));
  check(overcast::precise_current(after)-overcast::precise_current(before)<std::int64_t(overcast::energy_unit));
 }
 for(bool npc:{false,true})for(bool retire:{false,true}){
  Fixture f(npc);f.crack();f.launch();
  if(retire){const auto old=f.source;f.w.leave(1);f.w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},2050,1);
   f.w.set_vitals(1,{1000,1000,50,100},100);check(f.w.energy_capacity(1,6,0,2050,{80,-2}));
   check(*f.w.effect_scope(1)!=old);f.impact();check(f.w.resources(1)->energy==50);
  }else{check(f.w.kill(f.source,2050));const auto dead=f.w.resources(1)->energy;
   f.impact();check(f.w.resources(1)->health==0&&f.w.resources(1)->energy==dead);}
 }
 for(bool npc:{false,true}){
  // Hammer sets Cracked Armor while leaving30HP;the following Shock is lethal.
  Fixture f(npc,50,40);f.crack();check(f.health()>0);f.launch();f.impact();
  check(f.health()==0&&f.w.resources(1)->energy==61);
 }
 std::cout<<"PASS Shock Arrow real player/NPC conditional Energy debt,fractional preservation,full/Overcast caps,source death/retirement and lethal target contact\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
