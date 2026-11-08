#include "gw_blood_ritual.h"
#include "gw_blood_is_power.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Blood Ritual World line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=blood_ritual(rank);check(p&&p->skill==157&&p->rank==rank&&p->pips==3&&p->range==144);}
 check(!blood_ritual(21)&&!energy_enchantment(118,12));
 check(blood_ritual(0)->duration==8000&&blood_ritual(12)->duration==13000&&blood_ritual(20)->duration==16000);
 for(unsigned mode=0;mode<4;++mode){
  world::Instance w;auto out=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(2,"Ally",mode==1?144.01f:144.f,0,out,0,{}, {},100,mode==2?2:1);
  w.set_vitals(1,{1000,1000,100,100},100);w.set_vitals(2,{1000,1000,0,100},100);
  const auto a=*w.effect_scope(1),b=*w.effect_scope(2);auto target=mode==3?a:b;
  std::optional<skills::sacrifice::Loss> paid;std::vector<world::Packet> visuals;
  check(w.valid_energy_enchantment_target(a,target.actor,*blood_ritual(12),100)==(mode==0));
  check(w.energy_enchantment(a,target,*blood_ritual(12),100,&paid,&visuals)==(mode==0));
  if(mode){check(!paid&&w.resources(1)->health==1000);continue;}
  check(paid&&paid->lost==170&&w.resources(1)->health==830&&visuals.empty());
  // Both independently owned enchantments must survive the other's renewal/removal.
  check(w.energy_enchantment(a,b,*blood_is_power(20),100,&paid));
  auto state=w.overcast_energy_snapshot(b);check(state&&state->curve.effective_energy_pips(2,100)==10);
  check(w.energy_enchantment(a,b,*blood_ritual(0),1100,&paid));
  auto entries=w.status_snapshot(b,1100);check(entries&&entries->size()==2);
  unsigned ritual=0,power=0;for(const auto& e:*entries){if(e.skill==157)ritual=e.instance;if(e.skill==119)power=e.instance;}
  check(ritual&&power);check(!w.status_mutation(b,157,999,12,1100,14100,true));
  check(w.status_mutation(b,157,ritual,0,2100,std::nullopt,false));
  state=w.overcast_energy_snapshot(b);check(state&&state->curve.effective_energy_pips(2,2100)==9);
  entries=w.status_snapshot(b,2100);check(entries&&entries->size()==1&&entries->front().skill==119);
  check(w.energy_enchantment(a,b,*blood_ritual(0),2100,&paid));
  w.tick(10100);state=w.overcast_energy_snapshot(b);check(state&&state->curve.effective_energy_pips(2,10100)==2);
  entries=w.status_snapshot(b,10100);check(entries&&entries->empty());
 }
 std::cout<<"PASS Blood Ritual native ranks,touch boundary,ally gates,sacrifice receipt,independent stacking/renewal/removal/expiry; rendering not verified\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
