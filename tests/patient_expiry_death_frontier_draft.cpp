// DETACHED: actual Patient Spirit natural-end healing must retire a lethal
// maintainer at the expiry frontier, including direct delayed resource events.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Patient expiry death frontier draft line "+std::to_string(at.line()));
}
static bool verified=true;
static void expect(bool ok){verified=verified&&ok;}
int main()try{
 for(bool remote_lease:{false,true})for(bool direct_resource:{false,true}){
  auto w=std::make_unique<world::Instance>();auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u}){w->join(id,"Actor",id==1?500.f:id==2?60.f:0.f,0,id==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w->set_vitals(id,{id==2?10u:500u,1000,100,100},100);}
  const auto caster=*w->effect_scope(1),maintainer=*w->effect_scope(2),foe=*w->effect_scope(3);
  check(w->party_action(1,158,2,100)&&w->party_action(2,154,1,100));
  std::optional<MendingWire> end;
  if(remote_lease){check(w->stage_retribution(maintainer,caster,*retribution(0),100));
   const auto remote=w->status_snapshot(caster,100)->front();check(remote.skill==248);
   end=mending_end_wire(maintainer,caster,remote);check(bool(end));}
  check(w->protective_enchantment(caster,maintainer,*skills::regeneration::reviewed(2061,0),0,100));
  const auto patient=w->status_snapshot(maintainer,100)->front();check(patient.skill==2061);
  world::Packet expired{67,0};world::u32(expired,2);world::u32(expired,patient.instance);
  check(w->apply_condition(foe,maintainer,combat::conditions::Kind::deep_wound,100,10000));
  const auto epoch=*w->control_epoch(maintainer);box->drain();
  if(direct_resource)check(w->settle_resources(maintainer,5000));else w->tick(5000);
  const auto death=w->player_death_times({2,1})[0];
  const auto packets=box->drain();
  const auto upkeep_ends=end?std::count(packets.begin(),packets.end(),end->upkeep):0;
  const auto effect_ends=std::count(packets.begin(),packets.end(),expired);
  std::cout<<"{\"event\":\"patient_expiry_probe\",\"remote_lease\":"<<remote_lease
   <<",\"direct_resource\":"<<direct_resource<<",\"health\":"<<w->resources(2)->health
   <<",\"death_at\":"<<(death?std::to_string(*death):"null")<<",\"epoch_delta\":"<<(*w->control_epoch(maintainer)-epoch)
   <<",\"patient_end_count\":"<<effect_ends<<",\"upkeep_end_count\":"<<upkeep_ends<<"}\n";
  expect(w->resources(2)->health==0&&*w->control_epoch(maintainer)==epoch+1);
  expect(death==2100&&effect_ends==1&&upkeep_ends==unsigned(remote_lease));
  expect(w->maintained_enchantment_count(maintainer,5000)==0);
  expect(w->status_snapshot(caster,5000)->empty());
  w->tick(5000);expect(*w->control_epoch(maintainer)==epoch+1&&w->player_death_times({2,1})[0]==2100);
  const auto repeated=box->drain();expect(std::count(repeated.begin(),repeated.end(),expired)==0);
  if(end)expect(std::count(repeated.begin(),repeated.end(),end->upkeep)==0);
 }
 if(!verified){std::cerr<<"FAIL Patient expiry probe expectations\n";return 1;}
 std::cout<<"PASS detached Patient natural-end lethal debt exact frontier and remote upkeep retirement\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
