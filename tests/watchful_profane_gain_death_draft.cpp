// DETACHED actual ground-field strip -> Watchful Health gain -> maintained
// caster death retirement. No synthetic Health delta or direct kill injection.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Watchful Profane gain death draft line "+std::to_string(at.line()));
}
static bool verified=true;
static void expect(bool ok){verified=verified&&ok;}
int main()try{
 for(bool lethal:{false,true}){
  auto w=std::make_unique<world::Instance>();auto box=std::make_shared<world::Outbox>(),victim_box=std::make_shared<world::Outbox>();
  world::Instance::Npc corpse;corpse.id=corpse.definition.id=100;corpse.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health={100,100};d.damageable=true;d.fleshy=true;d.fleshy_evidence="authored well corpse";corpse.combat_definition=d;w->configure_npcs({corpse});
  w->join(1,"Watchful caster",500,0,box,0,{}, {},100,1);w->set_vitals(1,{500,1000,100,100},100);
  w->join(2,"Maintainer",60,0,victim_box,0,{}, {},100,1);w->set_vitals(2,{lethal?10u:100u,1000,100,100},100);
  w->join(3,"Profane foe",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w->set_vitals(3,{500,1000,100,100},100);
  const auto caster=*w->effect_scope(1),victim=*w->effect_scope(2),foe=*w->effect_scope(3);
  // Remote lease is outside the field; only the maintainer's actual death
  // can retire its upkeep/recipient effect.
  check(w->stage_retribution(victim,caster,*retribution(0),100));
  const auto remote=w->status_snapshot(caster,100)->front();const auto ended=mending_end_wire(victim,caster,remote);check(bool(ended));
  check(w->stage_watchful_healing(caster,victim,*watchful_healing(0),0,100));
  const auto watchful=w->status_snapshot(victim,100)->front();check(watchful.skill==1392);
  world::Packet removed{67,0};world::u32(removed,2);world::u32(removed,watchful.instance);
  check(w->degeneration_hex(foe,victim,*degeneration_hex(129,12),100));
  if(lethal)check(w->apply_condition(foe,victim,combat::conditions::Kind::deep_wound,100,10000));
  const auto epoch=*w->control_epoch(victim);
  check(w->damage_npc(foe,*w->effect_scope(100),{combat::Kind::armor_ignoring,1000,20,194},1248,100).killed);
  box->drain();victim_box->drain();
  check(w->claim_well_corpse(foe,*well_cast_plan(94,12),1,100,252,[](const auto&){return true;}));
  const auto death=w->player_death_times({2,1})[0];
  const auto owner_packets=victim_box->drain();const auto upkeep_ends=std::count(owner_packets.begin(),owner_packets.end(),ended->upkeep);
  const auto watchful_ends=std::count(owner_packets.begin(),owner_packets.end(),removed);
  std::cout<<"{\"event\":\"watchful_profane_probe\",\"lethal\":"<<lethal<<",\"health\":"<<w->resources(2)->health
   <<",\"death_at\":"<<(death?std::to_string(*death):"null")<<",\"epoch_delta\":"<<(*w->control_epoch(victim)-epoch)
   <<",\"watchful_end_count\":"<<watchful_ends<<",\"upkeep_end_count\":"<<upkeep_ends<<"}\n";
  expect(w->resources(2)->health==(lethal?0u:130u));
  expect(*w->control_epoch(victim)==epoch+unsigned(lethal));
  expect(w->maintained_enchantment_count(victim,100)==unsigned(!lethal));
  const auto active=w->status_snapshot(caster,100);expect(active&&active->size()==unsigned(!lethal));
  expect(upkeep_ends==unsigned(lethal)&&watchful_ends==1);
  if(lethal)expect(death==100);else expect(!death);
  w->tick(100);expect(*w->control_epoch(victim)==epoch+unsigned(lethal));
  expect(w->maintained_enchantment_count(victim,100)==unsigned(!lethal));
  const auto repeated=victim_box->drain();expect(std::count(repeated.begin(),repeated.end(),ended->upkeep)==0);
  expect(std::count(repeated.begin(),repeated.end(),removed)==0);
 }
 if(!verified){std::cerr<<"FAIL Watchful Profane probe expectations\n";return 1;}
 std::cout<<"PASS detached actual Profane Watchful gain, Defile bypass, lethal debt and remote upkeep death-once\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
