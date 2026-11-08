#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
static void check(bool b,std::source_location at=std::source_location::current()){
 if(!b)throw std::runtime_error("Signet of Corruption world line "+std::to_string(at.line()));
}
int main(int argc,char** argv)try{
 check(argc==2);std::ofstream fixture(argv[1],std::ios::binary);
 auto u32=[&](std::uint32_t n){for(unsigned i=0;i<4;++i)fixture.put(char(n>>(8*i)));};
 auto f32=[&](float n){u32(std::bit_cast<std::uint32_t>(n));};
 fixture.write("FWNAV001",8);for(auto n:{1u,42u,0xeefe704cu,12u,1u,1u})u32(n);
 f32(-500);f32(-500);f32(500);f32(500);u32(2);
 for(unsigned plane=0;plane<2;++plane){u32(1);for(unsigned i=0;i<5;++i)u32(0xffffffff);f32(-500);f32(500);f32(-500);f32(500);f32(-500);f32(500);}fixture.close();
 const auto mesh=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(argv[1]));
 world::Instance w;const std::string hash(64,'a');
 std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
 auto pop=world::load_npc_catalog(appearance);
 std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/signet_of_corruption_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test 60 test\n");
 const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(pop,catalog,hash);
 w.configure_navigation(mesh);w.configure_npcs(pop.for_map(146));
 const auto box=std::make_shared<world::Outbox>();
 w.join(1,"Caster",0,0,box,0,{},0,100,1);
 w.join(2,"Target",60,0,std::make_shared<world::Outbox>(),0,{},0,100,2);
 w.join(3,"Nearby",100,0,std::make_shared<world::Outbox>(),0,{},0,100,2);
 w.join(4,"Far",400,0,std::make_shared<world::Outbox>(),0,{},0,100,2);
 w.set_vitals(1,{500,500,0,100},100);for(unsigned id=2;id<=4;++id)w.set_vitals(id,{500,500,100,100},100);
 const auto caster=*w.effect_scope(1),target=*w.effect_scope(2),npc=*w.effect_scope(100);
 check(w.degeneration_hex(caster,target,*skills::effects::degeneration_hex(101,0),100));
 check(w.degeneration_hex(caster,npc,*skills::effects::degeneration_hex(101,0),100));
 box->drain();const auto plan=*skills::effects::signet_of_corruption(2093,0);
 check(w.signet_of_corruption(caster,target,plan,20,400));
 check(w.resources(1)->energy==4);
 check(w.resources(2)->health==480&&w.resources(3)->health==480&&w.resources(4)->health==500);
 check(w.npc_combat_snapshot(npc)->health->current==480);
 const auto packets=box->drain();check(!packets.empty());
 for(unsigned skill:{1950u,2093u})for(bool npc_primary:{false,true})for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u}){
  world::Instance world;std::vector<world::Instance::Npc> population;
  for(unsigned id:{100u,101u}){world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?100:352;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health={double(id==100?hp:1000u),1000};d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;population.push_back(n);
  }world.configure_npcs(population);
  for(unsigned id:{1u,2u,3u,4u,5u}){world.join(id,"Actor",id==1?0:id==2||id==5?100:id==3?352:353,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);world.set_vitals(id,{id==2?hp:1000u,1000,id==1?0u:100u,100},100);}
  const auto source=*world.effect_scope(1),target=*world.effect_scope(npc_primary?100:2);
  check(world.apply_condition(source,*world.effect_scope(2),combat::conditions::Kind::blind,100,10000));check(world.blindness(source,*world.effect_scope(100),*skills::effects::blindness(220,0),100));
  for(unsigned id:{2u,3u,100u,101u}){const auto scope=*world.effect_scope(id);const auto rank=id==target.actor?rr:id==101?0u:12u;
   if(id<100)check(world.status_mutation(scope,307,id+10,rank,100,8100,true));else check(world.stage_npc_reversal(scope,rank,100));}
  const auto p=*skills::effects::signet_of_corruption(skill,6);check(world.signet_of_corruption(source,target,p,20,1000));
  for(unsigned id:{2u,3u,100u,101u}){const double before=id==2||id==100?hp:1000;const unsigned rank=id==target.actor?rr:id==101?0u:12u;
   const double converted=std::min(double(p.damage),double(skills::effects::reversal_fortune(rank)->maximum_healing));const double expected=std::max(0.,std::min(1000.,before+converted)-(p.damage-converted));
   const double actual=id<100?world.resources(id)->health:world.npc_combat_snapshot(*world.effect_scope(id))->health->current;check(std::abs(actual-expected)<1.1);
  }
  check(world.resources(1)->energy==4&&world.resources(4)->health==1000&&world.resources(5)->health==1000);check(world.take_npc_deaths(source).empty());
 }
 std::cout<<"PASS Signet of Corruption player/NPC nearby damage, conditional capped Energy and out-of-range exclusion\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
