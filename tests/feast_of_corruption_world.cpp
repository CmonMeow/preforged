#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Feast world regression line "+std::to_string(at.line()));}
int main(int argc,char** argv)try{

 for(bool npc:{false,true})for(bool primed:{false,true})for(bool reversal:{false,true}){
  world::Instance w;
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=d.conditions_allowed=true;
   d.healing_allowed=true;d.healing_evidence="authored Feast conversion fixture";n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{100,500,100,100},100);
  if(!npc){w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
  if(npc)check(w.stage_npc_shield_absorption(target,12,100));else check(w.status_mutation(target,1399,80,12,100,6100,true));
  if(primed){for(unsigned n=0;n<13;++n){
    if(npc)check(w.damage_npc(source,target,{combat::Kind::armor_ignoring,1,20,42},1248,100).reason==combat::Reason::accepted);
    else check(w.wound_spell(source,target,*skills::effects::wound_spell(1052,0),20,100));
   }if(!npc)w.set_vitals(2,{500,500,100,100},100);
  }
  check(w.degeneration_hex(source,target,*skills::effects::degeneration_hex(101,0),100));
  if(reversal){if(npc)check(w.stage_npc_reversal(target,12,100));else check(w.status_mutation(target,307,81,12,100,8100,true));}
  const auto anchor=*skills::effects::acquire_feast_anchor(source,{target,60,0,0,true,100},100);
  const auto plan=*skills::effects::feast_of_corruption(12);
  const auto health=[&](){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);};
  check(w.feast_of_corruption(source,anchor,plan,100));
  const unsigned first=primed?(reversal?500:466):(reversal?466:404);
  check(health()==first&&w.resources(1)->health==(primed&&reversal?100u:134u));
  const auto entries=*w.status_snapshot(target,100);check(std::none_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;}));
  check(w.feast_of_corruption(source,anchor,plan,101));
  check(health()==first-(primed?34:91)&&w.resources(1)->health==(primed&&reversal?134u:168u));
  check(w.feast_late_refusals()==0);
 }
 check(argc==2);std::ofstream fixture(argv[1],std::ios::binary);
 auto u32=[&](std::uint32_t n){for(unsigned i=0;i<4;++i)fixture.put(char(n>>(8*i)));};auto f32=[&](float n){u32(std::bit_cast<std::uint32_t>(n));};
 fixture.write("FWNAV001",8);for(auto n:{1u,42u,0xeefe704cu,12u,1u,1u})u32(n);f32(-500);f32(-500);f32(500);f32(500);u32(2);
 for(unsigned plane=0;plane<2;++plane){u32(1);for(unsigned i=0;i<5;++i)u32(0xffffffff);f32(-500);f32(500);f32(-500);f32(500);f32(-500);f32(500);}fixture.close();
 const auto mesh=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(argv[1]));
 for(unsigned mode=0;mode<10;++mode){
  world::Instance w;const std::string hash(64,'a');
  std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\nnpc 146 101 2 1235 0 1677721600 0 8 1 1 0 227 0 0 - -\n");auto pop=world::load_npc_catalog(appearance);
  const auto row=[&](unsigned id,unsigned model,unsigned health){return "\nnpc 146 "+std::to_string(id)+" "+std::to_string(id-99)+" "+std::to_string(model)+" 0 0 8 1 hostile-to-players - 500 "+std::to_string(health)+" 60 1 1 test test test - test 60 60 test test 60 test\n";};
  std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/feast_of_corruption_world.cpp"+row(100,1234,500)+row(101,1235,mode==8?70:500));
  const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(pop,catalog,hash);w.configure_navigation(mesh);w.configure_npcs(pop.for_map(146));
  const auto box=std::make_shared<world::Outbox>();
  for(unsigned id=1;id<=6;++id){const float x=id==1?0.f:id==2||id==5?60.f:id==3?226.f:id==4?227.f:100.f;
   w.join(id,"Actor",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{},id==6?1:0,100,id==1||id==5?1:2);w.set_vitals(id,{id==1?100u:500u,500,100,100},100);
  }
  const auto caster=*w.effect_scope(1),player=*w.effect_scope(2),npc=*w.effect_scope(100),other_npc=*w.effect_scope(101);
  const auto selected=mode>=6?npc:player;const auto anchor=skills::effects::acquire_feast_anchor(caster,{selected,mode>=6?100.:60.,0,0,true,100},100);check(bool(anchor));
  if(mode==1||mode==4||mode==9){check(w.degeneration_hex(caster,player,*skills::effects::degeneration_hex(101,0),100));check(w.degeneration_hex(caster,npc,*skills::effects::degeneration_hex(101,0),100));}
  // Life stolen is Health gain, not received healing: Defile Flesh must not
  // reduce transfers from either player or NPC recipients into the caster.
  if(mode==9)check(w.degeneration_hex(player,caster,*skills::effects::degeneration_hex(129,12),100));
  if(mode==2)check(w.kill(player,200));
  if(mode==3){w.leave(2);w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{},0,200,2);w.set_vitals(2,{500,500,100,100},100);}
  if(mode==5)check(w.kill(caster,200));
  if(mode==7){combat::weapon::Owned sword{1,1,{500,500,2},12,false,false,27};check(w.ordinary_weapon(caster,sword,20)&&w.melee_attack(caster,npc,sword,*combat::weapon::attack_skill(322,0),200));check(w.npc_combat_snapshot(npc)->health->current==0);w.take_npc_deaths(caster);}
  if(mode==8)check(w.degeneration_hex(caster,other_npc,*skills::effects::degeneration_hex(101,0),100));
  box->drain();const auto at=mode==4?30300u:400u;
  const bool accepted=w.feast_of_corruption(caster,*anchor,*skills::effects::feast_of_corruption(12),at);
  check(accepted==(mode!=3&&mode!=5));
  if(!accepted){check(w.resources(3)->health==500&&w.npc_combat_snapshot(npc)->health->current==500);continue;}
  check(w.resources(2)->health==(mode==2?0u:mode==1||mode==9?399u:433u));
  check(w.resources(3)->health==433&&w.resources(4)->health==(mode>=6?433u:500u)&&w.resources(5)->health==500&&w.resources(6)->health==500);
  check(w.npc_combat_snapshot(npc)->health->current==(mode==7?0:mode==1||mode==9?399:433));
  check(w.npc_combat_snapshot(other_npc)->health->current==(mode==8?0:mode>=6?433:500));
  check(w.resources(1)->health==(mode==1||mode==9?168u:mode==8?104u:100u));
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(selected.actor,1,151))==1);
  if(mode!=2)check(std::find(packets.begin(),packets.end(),skills::particles::health_change_2022(2,1,-67,500))!=packets.end());
  if(mode==8)check(std::find(packets.begin(),packets.end(),combat::detail::effects(other_npc.actor,w.npc_combat_snapshot(other_npc)->effects))!=packets.end());
  const auto deaths=w.take_npc_deaths(caster);check(deaths.size()==(mode==8?1u:0u));if(mode==8)check(deaths[0].target==other_npc);
  w.tick(at+100);check(w.take_npc_deaths(caster).empty());
 }
 std::cout<<"PASS staged Feast mixed player/NPC AoE,hex-only steal,radius/team/layer,player/NPC corpse anchors,replacement rejection,expiry,target273 and kill credit;casting/retail order pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
