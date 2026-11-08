// Detached diagnostic: conserve ordinary (non-Overcast) Energy earned before
// actual Patient Spirit expiry death. No remote maintained link masks frontier.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Patient Energy frontier setup line "+std::to_string(at.line()));
}
struct Sample {unsigned energy;std::optional<std::uint64_t> death;std::uint64_t epoch;};
static Sample run(bool lethal,bool dense,bool direct){
 auto w=std::make_unique<world::Instance>();
 for(unsigned id:{1u,2u,3u}){
  w->join(id,"Actor",id==1?500.f:id==2?60.f:0.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);
  w->set_vitals(id,{id==2&&lethal?10u:500u,1000,0,100},100);
 }
 const auto caster=*w->effect_scope(1),target=*w->effect_scope(2),foe=*w->effect_scope(3);
 // Monk innate20 + authored equipment80 =100 maximum; innate2 + armor2=4pips.
 check(w->energy_capacity(2,3,0,100,energy::ArmorBonus{80,2}));
 check(w->resources(2)->energy==0&&w->resources(2)->max_energy==100);
 check(!w->overcast_energy_snapshot(target));
 check(w->protective_enchantment(caster,target,*skills::regeneration::reviewed(2061,0),0,100));
 if(lethal)check(w->apply_condition(foe,target,combat::conditions::Kind::deep_wound,100,10000));
 const auto epoch=*w->control_epoch(target);
 auto advance=[&](std::uint64_t at){if(direct)check(w->settle_resources(target,at));else w->tick(at);};
 if(dense)for(std::uint64_t at=200;at<=2000;at+=100)advance(at);
 advance(5000);
 Sample result{w->resources(2)->energy,w->player_death_times({2,1})[0],*w->control_epoch(target)-epoch};
 std::cout<<"{\"event\":\"patient_energy_frontier\",\"lethal\":"<<lethal<<",\"dense\":"<<dense
  <<",\"direct_resource\":"<<direct<<",\"energy\":"<<result.energy<<",\"death_at\":"
  <<(result.death?std::to_string(*result.death):"null")<<",\"epoch_delta\":"<<result.epoch<<"}\n";
 return result;
}
int main()try{
 bool good=true;
 for(bool direct:{false,true})for(bool lethal:{false,true}){
  const auto dense=run(lethal,true,direct),coarse=run(lethal,false,direct);
  // Exact pip-ms: living100..2100 earns8000/3000 Energy; whole view floor=2.
  // Living control100..5000 earns19600/3000; whole view floor=6.
  // Corpses clear fractional remainder, so do not fabricate private precision.
  const auto expected=lethal?2u:6u;
  good&=dense.energy==expected&&coarse.energy==expected&&dense.energy==coarse.energy;
  if(lethal)good&=dense.death==2100&&coarse.death==2100&&dense.epoch==1&&coarse.epoch==1;
  else good&=!dense.death&&!coarse.death&&dense.epoch==0&&coarse.epoch==0;
 }
 if(!good){std::cerr<<"FAIL Patient expiry ordinary Energy/frontier conservation\n";return 1;}
 std::cout<<"PASS Patient expiry ordinary Energy/frontier conservation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
