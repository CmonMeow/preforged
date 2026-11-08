#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Gaze Reversal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source,primary,near,player;
 Fixture(unsigned hp,std::optional<unsigned> energy=20){std::vector<world::Instance::Npc> population;
  for(unsigned id:{100u,101u}){world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?60:140;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=474;d.binding.level=20;d.binding.actor=d.binding.definition=id;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health={double(id==100?hp:500u),1000};d.damageable=d.conditions_allowed=true;d.general_armor=id==100?60:80;d.general_armor_evidence="synthetic";
   if(energy){d.spawn_energy=combat::npcs::Energy{*energy,40};d.energy_pips=0;}d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;population.push_back(n);
  }w.configure_npcs(population);w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Extra foe",220,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,1000,100,100},100);
  source=*w.effect_scope(1);primary=*w.effect_scope(100);near=*w.effect_scope(101);player=*w.effect_scope(2);check(w.holy_armor_locations(player,std::array<int,5>{80,60,60,60,60}));w.configure_armor_location_draws([]{return 0u;});
 }
 double hp(){return w.npc_combat_snapshot(primary)->health->current;}
};
int main()try{unsigned cases=0;
 for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u})for(unsigned protection:{0u,16u,17u}){
  ++cases;Fixture f(hp);check(f.w.stage_npc_reversal(f.primary,rr,100)&&f.w.stage_npc_reversal(f.near,12,100)&&f.w.status_mutation(f.player,307,50,0,100,8100,true));
  if(protection)check(f.w.npc_interrupt_stance(f.primary,protection,12,10000,900));
  const auto action=f.w.begin_npc_action(f.primary,combat::npcs::control::Action::spell,950,3000,skills::Slot{194,0});check(action.has_value());
  const auto side=f.w.begin_npc_action(f.near,combat::npcs::control::Action::spell,950,3000,skills::Slot{194,0});check(side.has_value());
  f.box->drain();std::function<void()> publication;const auto selected=f.w.stage_lightbringers_gaze_world(f.source,f.primary,*skills::effects::lightbringers_gaze(2),20,1000,&publication);
  check(selected&&selected->size()==3&&publication&&f.box->drain().empty());
  const double converted=std::min(100.,double(skills::effects::reversal_fortune(rr)->maximum_healing)),expected=std::max(0.,std::min(1000.,hp+converted)-(100-converted));
  check(std::abs(f.hp()-expected)<1e-7);
  const double raw=100/std::sqrt(2.);check(std::abs(f.w.npc_combat_snapshot(f.near)->health->current-(500+67-(raw-67)))<1e-6);
  check(std::abs(double(f.w.resources(2)->health)-(500+15-(raw-15)))<1.1);
  publication();const auto packets=f.box->drain();world::Packet stop;world::u16(stop,158);world::u32(stop,59);world::u32(stop,100);world::u32(stop,0);
  check(std::count(packets.begin(),packets.end(),stop)==unsigned(expected>0&&!protection));
  for(unsigned id:{100u,101u})check(std::any_of(packets.begin(),packets.end(),[&](const auto& p){if(p.size()!=10||p[0]!=240)return false;unsigned actor=0,flags=0;for(unsigned n=0;n<4;++n){actor|=unsigned(p[2+n])<<(8*n);flags|=unsigned(p[6+n])<<(8*n);}return actor==id&&!(flags&0x80);}));
  publication();check(f.box->drain().empty());
  check(f.w.take_npc_deaths(f.source).size()==unsigned(expected==0));
  check(f.w.complete_npc_action(*action,3000)==(expected>0&&protection));check(f.w.complete_npc_action(*side,3000));
  if(expected>0&&protection==17)check(f.w.npc_energy_snapshot(f.primary)->current==15);
 }
 check(cases==27);
 {Fixture f(10);check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000)&&f.w.condition_transfer(f.source,f.primary,*skills::effects::condition_transfer(132,12),100));check(f.w.stage_npc_reversal(f.primary,12,100));
  const auto action=f.w.begin_npc_action(f.primary,combat::npcs::control::Action::spell,950,3000,skills::Slot{194,0});check(action.has_value());
  const auto result=f.w.stage_lightbringers_gaze_world(f.source,f.primary,*skills::effects::lightbringers_gaze(0),20,1000);check(result.has_value());const auto deaths=f.w.take_npc_deaths(f.source);const auto completed=f.w.complete_npc_action(*action,3000);if(f.hp()!=0||deaths.size()!=1||completed)throw std::runtime_error("protection Energy reaction: hp="+std::to_string(f.hp())+" deaths="+std::to_string(deaths.size())+" completed="+std::to_string(completed));
 }
 {Fixture f(500);check(f.w.stage_npc_watch_yourself_recipient(f.primary,f.primary,*skills::effects::watch_yourself(0,true),100));check(f.w.stage_npc_reversal(f.primary,20,100));
  check(f.w.stage_lightbringers_gaze_world(f.source,f.primary,*skills::effects::lightbringers_gaze(0),20,1000).has_value());const auto entries=*f.w.status_snapshot(f.primary,1000);check(std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==2858;}));
 }
 {Fixture f(40,5);check(f.w.stage_npc_reversal(f.primary,12,100)&&f.w.apply_mind_wrack(f.source,f.primary,12,false,100));
  check(f.w.npc_interrupt_stance(f.primary,17,0,10000,900));const auto action=f.w.begin_npc_action(f.primary,combat::npcs::control::Action::spell,950,3000,skills::Slot{194,0});check(action.has_value());
  const auto result=f.w.stage_lightbringers_gaze_world(f.source,f.primary,*skills::effects::lightbringers_gaze(0),20,1000);check(result.has_value());const auto deaths=f.w.take_npc_deaths(f.source);const auto completed=f.w.complete_npc_action(*action,3000);if(f.hp()!=0||deaths.size()!=1||completed)throw std::runtime_error("protection Energy reaction: hp="+std::to_string(f.hp())+" deaths="+std::to_string(deaths.size())+" completed="+std::to_string(completed));
 }
 for(unsigned rr:{0u,12u}){Fixture f(10,std::nullopt);check(f.w.stage_npc_reversal(f.primary,rr,100)&&f.w.npc_interrupt_stance(f.primary,17,12,10000,900));
  const auto action=f.w.begin_npc_action(f.primary,combat::npcs::control::Action::spell,950,3000,skills::Slot{194,0});check(action.has_value());f.box->drain();
  const auto result=f.w.stage_lightbringers_gaze_world(f.source,f.primary,*skills::effects::lightbringers_gaze(0),20,1000);check(result.has_value()==(rr==0));
  if(rr)check(f.hp()==10&&f.box->drain().empty());else check(f.hp()==0&&f.w.take_npc_deaths(f.source).size()==1);
 }
 for(int rr:{-1,0,12}){Fixture f(10);check(f.w.elemental_armor(f.player,60,60));
  if(rr>=0)check(f.w.stage_npc_reversal(f.primary,rr,100));
  check(f.w.stage_npc_bonds_of_torment(f.primary,f.player,*skills::effects::bonds_of_torment_npc(),100));
  check(f.w.stage_lightbringers_gaze_world(f.source,f.primary,*skills::effects::lightbringers_gaze(0),20,1000).has_value());
  const double heal=rr<0?0:std::min(100.,double(skills::effects::reversal_fortune(rr)->maximum_healing));
  const double before_loss=10+heal,loss=std::min(before_loss,100-heal);
  check(f.hp()==before_loss-loss&&f.w.resources(2)->health==500-loss);
  check(f.w.take_npc_deaths(f.source).size()==unsigned(before_loss==loss));
 }
 std::cout<<"PASS35 Gaze mixed conversions/interrupt protections,healing death,zero remaining armor retention,protection Energy reaction death,unknown Energy survival gates,lethal Bonds ordering;307 stays unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
