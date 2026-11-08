#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
static void check(bool good,std::source_location at=std::source_location::current()){
 if(!good)throw std::runtime_error("Unstoppable World line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance world;std::shared_ptr<world::Outbox> out=std::make_shared<world::Outbox>();Scope owner,foe;
 Fixture(){
  world.join(1,"Norn",0,0,out,0,{}, {},100,1);world.set_vitals(1,{1000,1000,100,100},100);
  world.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);world.set_vitals(2,{1000,1000,100,100},100);
  owner=*world.effect_scope(1);foe=*world.effect_scope(2);
 }
};
int main()try{
 {Fixture f;const auto plan=*i_am_unstoppable(10);
  check(f.world.apply_condition(f.foe,f.owner,combat::conditions::Kind::crippled,100,10000));
  check(f.world.has_condition(f.owner,combat::conditions::Kind::crippled,100));
  check(f.world.i_am_unstoppable(f.owner,plan,100));
  check(!f.world.has_condition(f.owner,combat::conditions::Kind::crippled,100));
  check(f.world.status_snapshot(f.owner,100)->size()==1);
  check(f.world.apply_condition(f.foe,f.owner,combat::conditions::Kind::crippled,101,10000));
  check(!f.world.has_condition(f.owner,combat::conditions::Kind::crippled,101));
  check(f.world.gale(f.foe,f.owner,12,{},102)&&!f.world.knocked_down(f.owner,102));
  f.world.tick(20100);check(!f.world.status_snapshot(f.owner,20100)->size());
  check(f.world.apply_condition(f.foe,f.owner,combat::conditions::Kind::crippled,20100,1000));
  check(f.world.has_condition(f.owner,combat::conditions::Kind::crippled,20100));
  const auto packets=f.out->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(21,1,1908))==1);
 }
 {Fixture f;casting::Runtime runtime(1);casting::Hooks h;std::vector<Packet> wire;
  h.events=[&](const auto& packets){wire.insert(wire.end(),packets.begin(),packets.end());};
  h.resources=[&](auto actor){return f.world.resources(actor);};
  h.consume_energy_at=[&](auto energy,auto now){return f.world.consume_energy(1,energy,f.owner,now);};
  h.target_scope=[&](auto actor){return f.world.effect_scope(actor);};
  h.target_valid=[](auto actor,float,bool){return actor==1;};
  h.i_am_unstoppable=[&](auto owner,const auto& plan,auto now){return f.world.i_am_unstoppable(owner,plan,now);};
  Slots bar{};bar[0]={2356,0};Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[2356]=1;attributes::Ranks ranks{};
  check(runtime.start(100,{2356,0,0},bar,policy,ranks,true,h)==casting::Reason::unsupported);
  h.norn_rank=[]()->std::optional<unsigned>{return 11;};
  check(runtime.start(100,{2356,0,0},bar,policy,ranks,true,h)==casting::Reason::unsupported);
  h.norn_rank=[]()->std::optional<unsigned>{return 10;};
  check(runtime.start(100,{2356,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);
  check(f.world.resources(1)->energy==95);
  check(std::count(wire.begin(),wire.end(),casting::generic(48,1,2356))==1);
  check(runtime.start(101,{2356,0,1},bar,policy,ranks,true,h)==casting::Reason::recharge);
  const auto status=f.world.status_snapshot(f.owner,100);check(status&&status->size()==1&&status->front().rank==10&&status->front().expires==20100);
 }
 for(bool protected_owner:{false,true}){
  world::Instance world;world::Instance::Npc guardian;guardian.id=guardian.definition.id=100;
  guardian.definition.level=20;guardian.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition definition;definition.binding.map=146;definition.binding.actor=definition.binding.definition=100;
  definition.binding.level=20;definition.relation=combat::npcs::Relation::neutral;
  definition.spawn_health=combat::Health{500,1000};definition.damageable=definition.conditions_allowed=true;
  definition.physical_armor=60;guardian.combat_definition=definition;world.configure_npcs({guardian});
  world.join(1,"Owner",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  world.set_vitals(1,{1000,1000,100,100},100);const auto owner=*world.effect_scope(1),guard=*world.effect_scope(100);
  auto enemy=guardian;enemy.combat_definition->relation=combat::npcs::Relation::hostile_to_players;
  check(world.accept_quest_waves(owner,guard,73,1,{{enemy}},[]{return true;}));
  check(world.physical_armor(owner,60));if(protected_owner)check(world.i_am_unstoppable(owner,*i_am_unstoppable(10),100));
  combat::weapon::Strike incoming;incoming.level=20;incoming.minimum=incoming.maximum=20;incoming.base_roll=incoming.miss_roll=.5;
  check(world.damage_quest_player(*world.effect_scope(60000),owner,128,1000,incoming,1000,combat::weapon::AttackDelivery::melee));
  incoming.location_armor=60;incoming.post_penetration_armor=protected_owner?24:0;
  check(world.resources(1)->health==static_cast<unsigned>(std::ceil(1000-combat::weapon::resolve(incoming)->damage)));
 }
 combat::weapon::Strike strike;strike.level=20;strike.mastery=12;strike.minimum=strike.maximum=20;strike.location_armor=60;strike.base_roll=strike.miss_roll=.5;
 const auto normal=combat::weapon::resolve(strike);check(normal&&!normal->missed);strike.post_penetration_armor=24;
 const auto protected_hit=combat::weapon::resolve(strike);check(protected_hit&&!protected_hit->missed);
 check(std::abs(protected_hit->damage/normal->damage-std::exp2(-24.0/40.0))<1e-12);
 std::cout<<"PASS Unstoppable normal cast,cripple cleanse/immunity,knockdown immunity,20s expiry,post-cap weapon armor\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
