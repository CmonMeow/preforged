#include "gw_world_simulation.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Overcast World line "+std::to_string(at.line()));
}
int main()try{
 world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Actor",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,500,30,30},100);
 const auto owner=*w.effect_scope(1);check(w.energy_capacity(1,6,0,100,{10,2}));box->drain();
 check(!w.activate_overcast_energy(owner,194,5,1,1000)); // Not an Overcast-producing row.
 check(w.activate_overcast_energy(owner,231,5,1,1000)&&w.resources(1)->energy==20);
 const auto initial=*w.overcast_energy_snapshot(owner);check(initial.curve.burden_units()==1000000&&initial.resources.max_energy==30);
 auto packets=box->drain();check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=2&&p[0]==156&&p[1]==0;})==1);
 check(w.settle_resources(owner,1500));auto settled=*w.overcast_energy_snapshot(owner);
 check(overcast::precise_current(settled)==20*overcast::energy_unit+49500&&settled.curve.burden_units()==983500);
 check(w.consume_energy(1,5,owner,1500)&&w.restore_energy(1,2,owner,1500));
 check(overcast::precise_current(*w.overcast_energy_snapshot(owner))==17*overcast::energy_unit+49500);
 check(w.activate_overcast_energy(owner,231,5,2,1500));
 check(overcast::precise_current(*w.overcast_energy_snapshot(owner))==10*overcast::energy_unit+49500);
 check(!w.activate_overcast_energy(owner,231,5,2,1500));
 check(w.activate_overcast_energy(owner,231,0,3,1500)&&w.activate_overcast_energy(owner,231,0,4,1500));
 const auto negative=*w.overcast_energy_snapshot(owner);check(negative.debt==10&&negative.resources.energy==0&&negative.fractional_tail==0);
 {const auto updates=box->drain();bool signed_current=false;
  for(const auto& packet:updates)if(packet.size()==14&&packet[0]==161&&packet[1]==0&&packet[2]==33){
   float value;std::memcpy(&value,packet.data()+10,4);if(value<0){check(std::abs(value-double(overcast::precise_current(negative))/overcast::energy_unit/negative.resources.max_energy)<1e-6);signed_current=true;}}
  check(signed_current);
 }
 check(!w.consume_energy(1,1,owner,1500)&&w.restore_energy(1,100,owner,1500));
 check(overcast::precise_current(*w.overcast_energy_snapshot(owner))==overcast::precise_current(negative));
 check(w.energy_capacity(1,6,0,1500,{20,2}));
 const auto expanded=*w.overcast_energy_snapshot(owner);
 check(expanded.resources.max_energy==40&&expanded.curve.burden_units()==negative.curve.burden_units());
 check(overcast::precise_current(expanded)==overcast::precise_current(negative)+10*overcast::energy_unit);
 check(w.energy_capacity(1,6,0,1500,{10,2}));
 check(overcast::precise_current(*w.overcast_energy_snapshot(owner))==overcast::precise_current(negative));
 check(w.kill(owner,1500));check(w.settle_resources(owner,4500));
 const auto corpse=*w.overcast_energy_snapshot(owner);
 check(corpse.curve.burden_units()<negative.curve.burden_units()&&overcast::precise_current(corpse)==overcast::precise_current(negative));
 auto stale=owner;++stale.generation;check(!w.activate_overcast_energy(stale,231,0,5,4500));
 check(!w.resurrect(stale,4500));
 check(w.resurrect(owner,4501));const auto revived=*w.overcast_energy_snapshot(owner);
 check(revived.resources.health==500&&revived.curve.cursor()==4501&&revived.curve.burden_units()<corpse.curve.burden_units());
 check(overcast::precise_current(revived)==std::int64_t(revived.resources.max_energy)*overcast::energy_unit-std::int64_t(revived.curve.burden_units())*3);
 check(!w.resurrect(owner,4501));
 check(w.kill(owner,4501)&&w.resurrect(owner,200000));const auto recovered=*w.overcast_energy_snapshot(owner);
 check(!recovered.curve.active()&&recovered.resources.energy==30&&recovered.debt==0&&recovered.fractional_tail==0);
 w.leave(1);check(!w.overcast_energy_snapshot(owner));w.join(1,"Replacement",0,0,box,0,{}, {},5000,1);w.set_vitals(1,{500,500,30,30},100);
 check(!w.overcast_energy_snapshot(*w.effect_scope(1)));
 {world::Instance sim;auto rewards=std::make_shared<world::Outbox>();sim.join(1,"Actor",0,0,rewards,0,{}, {},100,1);sim.set_vitals(1,{500,500,30,30},100);
  const auto who=*sim.effect_scope(1);check(sim.energy_capacity(1,6,0,100,{10,-2})&&sim.activate_overcast_energy(who,231,0,1,1000));
  check(sim.settle_resources(who,1501));const auto before=*sim.overcast_energy_snapshot(who);rewards->drain();
  check(sim.restore_energy(1,1,who,1501));const auto after=*sim.overcast_energy_snapshot(who);
  check(after.resources.energy==before.resources.energy&&overcast::precise_current(after)-overcast::precise_current(before)==49599);
  const auto published=rewards->drain();check(!published.empty());
  check(sim.restore_energy(1,1,who,1501)&&rewards->drain().empty()); // Capped no-op.
 }
 for(bool negative:{false,true}){
  world::Instance sim;for(unsigned id:{1u,2u,3u}){sim.join(id,"Actor",float(id*20),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);sim.set_vitals(id,{500,500,id==1?3u:30u,30},100);}
  const auto caster=*sim.effect_scope(1),ally=*sim.effect_scope(2),foe=*sim.effect_scope(3);
  check(sim.party_action(1,158,2,100)&&sim.party_action(2,154,1,100));
  check(sim.energy_capacity(1,6,0,100,{10,-2})&&sim.activate_overcast_energy(caster,231,0,1,1000));
  if(negative)for(unsigned event=2;event<=4;++event)check(sim.activate_overcast_energy(caster,231,0,event,1000));
  check(sim.energy_capacity(1,6,0,1000,{10,2})&&sim.settle_resources(caster,1500));
  check(sim.apply_mind_wrack(foe,caster,12,false,1500));
  check(sim.apply_condition(foe,ally,combat::conditions::Kind::blind,1500,5000)&&sim.apply_condition(foe,ally,combat::conditions::Kind::crippled,1500,5000));
  const auto before=*sim.overcast_energy_snapshot(caster);
  check(sim.ally_cleansing(caster,ally,*skills::effects::ally_cleansing(295),1500));const auto after=*sim.overcast_energy_snapshot(caster);
  check(overcast::precise_current(after)==(negative?overcast::precise_current(before):0)&&after.resources.health==(negative?500u:417u));
  check(!sim.has_condition(ally,combat::conditions::Kind::blind,1500)&&!sim.has_condition(ally,combat::conditions::Kind::crippled,1500));
  const auto packets=sim.set_vitals(1,{500,500,7,30},100);check(!sim.overcast_energy_snapshot(caster)&&sim.resources(1)->energy==7);
  check(std::count_if(packets.begin(),packets.end(),[](const auto& packet){return packet.size()>=2&&packet[0]==156&&packet[1]==0;})==1);
  check(sim.settle_resources(caster,2250)&&sim.resources(1)->energy==8); // No stale fractional carry.
 }
 for(unsigned mode=0;mode<3;++mode)for(bool below_zero:{false,true}){
  world::Instance sim;for(unsigned id:{1u,2u}){sim.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);sim.set_vitals(id,{500,500,id==1?3u:30u,30},100);}
  const auto victim=*sim.effect_scope(1),foe=*sim.effect_scope(2);
  check(sim.energy_capacity(1,6,0,100,{10,-2})&&sim.activate_overcast_energy(victim,231,0,1,1000));
  if(below_zero)for(unsigned event=2;event<=4;++event)check(sim.activate_overcast_energy(victim,231,0,event,1000));
  check(sim.energy_capacity(1,6,0,1000,{10,2})&&sim.settle_resources(victim,1500));
  const auto before=*sim.overcast_energy_snapshot(victim);
  check(sim.apply_mind_wrack(foe,victim,12,false,1500));
  if(mode==0){check(sim.status_mutation(victim,17,900,12,1500,10100,true)&&sim.cast_activity(victim,82,1500,2500));check(!sim.interrupt_cast(victim,82,1500));}
  else if(mode==1){check(sim.cast_activity(victim,82,1500,2500));check(sim.interrupt_skill(foe,victim,*skills::effects::interrupt_skill(24,12),20,1500));}
  else{check(sim.degeneration_hex(foe,victim,*skills::effects::degeneration_hex(66,12),1500));check(sim.attack_activity(victim,2000,1500));}
  const auto after=*sim.overcast_energy_snapshot(victim);
  check(overcast::precise_current(after)==(below_zero?overcast::precise_current(before):0));
  check(after.curve.burden_units()==before.curve.burden_units());
  check(after.resources.health==(below_zero?500u:417u));
 }
 std::cout<<"PASS World owned Overcast costs, gains, fractional recovery, signed ceiling wire, corpse recovery and lifetime replacement; Runtime gate remains closed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
