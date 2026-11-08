#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Recall World line "+std::to_string(at.line()));}
int main()try{using namespace skills::effects;
 for(unsigned mode=0;mode<9;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{500,500,50,100},100);w.set_vitals(2,{500,500,100,100},100);const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);
  check(w.energy_capacity(1,5,0,100,{80,-2}));
  check(w.mantra_recall(owner,12,100));check(w.resources(1)->energy==50);
  const auto initial=*w.status_snapshot(owner,100);check(initial.size()==1&&initial.front().expires==20100);
  if(mode==1){check(w.mantra_recall(owner,12,500));check(w.resources(1)->energy==50);check(w.settle_resources(owner,20100));check(w.resources(1)->energy==50);check(w.settle_resources(owner,20500));}
  else if(mode==2)check(w.enchantment_removal(foe,owner,*enchantment_removal(68,0),20,500));
  else if(mode==3){check(w.kill(owner,500));check(w.settle_resources(owner,20100));}
  else if(mode==4){auto stale=owner;++stale.generation;check(!w.mantra_recall(stale,12,500));check(w.settle_resources(owner,20100));}
  else if(mode==5)check(w.consume_energy(1,65,owner,20100));
  else if(mode==7)check(w.status_mutation(owner,82,initial.front().instance,12,500,{},false));
  else if(mode==8){check(!w.status_mutation(owner,82,123,12,500,20500,true));check(w.settle_resources(owner,20100));}
  else if(mode==6){check(w.degeneration_hex(owner,foe,*degeneration_hex(36,0),100));check(w.settle_resources(owner,25100));}
  else {check(w.settle_resources(owner,20099));check(w.resources(1)->energy==50);check(w.settle_resources(owner,20100));}
  std::cout<<"mode="<<mode<<" energy="<<w.resources(1)->energy<<"\n";
  check(w.resources(1)->energy==(mode==3?50u:mode==5?7u:mode==6?72u+arcane_conundrum(0)->energy:72u));
  check(w.settle_resources(owner,26000));check(w.resources(1)->energy==(mode==3?50u:mode==5?7u:mode==6?72u+arcane_conundrum(0)->energy:72u));
 }
 for(unsigned initial:{60u,75u,98u,100u}){world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,initial,100},100);const auto owner=*w.effect_scope(1);
  check(w.energy_capacity(1,5,0,100,{80,-2}));
  check(w.mantra_recall(owner,12,100));check(w.energy_capacity(1,5,0,100,{0,-2}));check(w.settle_resources(owner,20100));
  check(w.resources(1)->energy==(initial==60?2u:initial==75?17u:20u));check(w.energy_capacity(1,5,0,20100,{80,-2}));
  check(w.resources(1)->energy==(initial==60?82u:initial==75?97u:100u));
 }
 for(bool frequent:{false,true}){world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,50,100},100);const auto owner=*w.effect_scope(1);
  check(w.energy_capacity(1,5,0,100,{80,0})&&w.mantra_recall(owner,12,100));
  if(frequent)for(std::uint64_t at=1100;at<25100;at+=1000)check(w.settle_resources(owner,at));
  check(w.settle_resources(owner,25100)&&w.resources(1)->energy==88);
  check(w.settle_resources(owner,26100)&&w.resources(1)->energy==89);
 }
 for(bool negative:{false,true})for(bool frequent:{false,true})for(bool removal:{false,true}){
  world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.set_vitals(1,{500,500,5,30},100);const auto owner=*w.effect_scope(1);
  check(w.energy_capacity(1,6,0,100,{10,-2}));
  check(w.activate_overcast_energy(owner,231,0,1,1000));
  if(negative)for(unsigned event=2;event<=4;++event)check(w.activate_overcast_energy(owner,231,0,event,1000));
  check(w.mantra_recall(owner,12,1000));
  const std::uint64_t deadline=removal?1500:21000;
  if(frequent)for(std::uint64_t at=1100;at<deadline;at+=100)check(w.settle_resources(owner,at));
  if(removal){const auto effects=*w.status_snapshot(owner,1400);check(effects.size()==1);check(w.status_mutation(owner,82,effects.front().instance,12,deadline,{},false));}
  check(w.settle_resources(owner,deadline));const auto state=*w.overcast_energy_snapshot(owner);
  check(state.curve.cursor()==deadline&&state.curve.burden_units()==(negative?4000000u:1000000u)-33*(deadline-1000));
  check(overcast::precise_current(state)==(removal?(negative?-2950500:6049500):(negative?-1020000:7980000)));
  check(w.settle_resources(owner,deadline+1));const auto after=*w.overcast_energy_snapshot(owner);
  check(overcast::precise_current(after)==overcast::precise_current(state)); // No duplicate end reward, pips0.
 }
 std::cout<<"PASS Recall owned expiry/removal/renewal, debt/capacity, same-event cost, deferred Arcane ordering, scope/death cleanup and no duplicate reward\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


