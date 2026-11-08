#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Watch Mind sequence line "+std::to_string(at.line()));}
int main()try{
 for(unsigned skill:{185u,209u,2803u,226u,2804u})for(bool advantage:{false,true})for(bool stacked:{false,true})for(unsigned rank:{0u,12u,20u}){
  world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{10000,10000,id==2&&advantage?10u:100u,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto source=*w.effect_scope(1),target=*w.effect_scope(2),foe=*w.effect_scope(3);
  check(w.elemental_armor(source,80,80)&&w.elemental_armor(target,80,80)&&w.lightning_armor(target,80));
  if(stacked){check(w.status_mutation(target,100,1001,0,100,30000,true)&&w.complete_convert_hexes(source,target,*convert_hexes(12),0,100));}
  const auto watch=*watch_yourself(rank,true);check(w.stage_watch_yourself_recipient(source,target,watch,100));
  for(unsigned n=1;n<watch.charges;++n)check(w.lightning(foe,target,*lightning(224,12),20,100));
  const auto before=w.resources(2)->health;bool accepted=false;double damage=0,penetration=0;
  if(skill==185){const auto plan=*mind_burn(skill,12);damage=plan.damage;accepted=w.mind_burn(foe,target,plan,20,100);}
  else if(skill==209||skill==2803){const auto plan=*mind_freeze(skill,12);damage=plan.damage;accepted=w.mind_freeze(foe,target,plan,20,100);}
  else{const auto plan=*mind_shock(skill,12);damage=plan.damage;penetration=plan.penetration;accepted=w.mind_shock(foe,target,plan,20,100);}
  check(accepted);const unsigned other=stacked?10:0,first_bonus=stacked?(rank==0?15:rank==12?25:32):watch.armor;
  const double expected=damage*std::exp2((60-(80.+first_bonus)*(1-penetration/100))/40)+(advantage?damage*std::exp2((60-(80.+other)*(1-penetration/100))/40):0);
  const auto loss=before-w.resources(2)->health;if(std::abs(double(loss)-expected)>=1.1)std::cerr<<"skill "<<skill<<" rank "<<rank<<" advantage "<<advantage<<" stacked "<<stacked<<" loss "<<loss<<" expected "<<expected<<'\n';
  check(std::abs(double(loss)-expected)<1.1&&w.watch_yourself_armor(target,100)==0);
 }
 std::cout<<"PASS60 Mind Burn/Freeze/Shock PvE/PvP energy/stack/rank contexts update armor between damage events\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
