#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){
 if(!x)throw std::runtime_error("Called Shot line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=combat::weapon::attack_skill(403,rank);
  check(p&&p->required_type==5&&p->projectile_speed_percent==300&&!p->bonus&&p->arrows==1&&!p->fixed_damage);
 }
 check(!combat::weapon::attack_skill(403,21));
 for(bool prepared:{false,true}){
  world::Instance w;w.join(1,"Archer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Foe",900,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{480,480,40,40},100);w.set_vitals(2,{480,480,40,40},100);
  const auto a=*w.effect_scope(1),b=*w.effect_scope(2);combat::weapon::Owned bow{7,1,{20,20,1},12,false,false,5};
  check(w.ordinary_weapon(a,bow,20)&&w.physical_armor(b,60));
  if(prepared)check(w.status_mutation(a,432,1,12,100,24100,true));
  auto plan=*combat::weapon::attack_skill(403,0);auto forged=plan;forged.projectile_speed_percent=600;check(!w.melee_attack(a,b,bow,forged,100));
  check(w.melee_attack(a,b,bow,plan,100));
  w.tick(190);check(w.resources(2)->health==480); // A preparation must not turn triple speed into sixfold.
  w.tick(290);check(w.resources(2)->health==(prepared?451u:460u));
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<7;++mode){
  using namespace skills;warrior_test::Fixture f(403,npc);unsigned draws=0,commits=0;bool stale=false;
  if(!npc)check(f.world.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));
  f.world.configure_attack_block_draws([&]{++draws;return 0.;});
  if(mode==1)check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::blind,300,5000));
  Slots bar{};bar[0]={403,0};Policy policy;policy.primary=2;policy.campaigns=255;policy.learned[403]=1;attributes::Ranks ranks{};
  std::vector<world::Packet> packets;casting::Hooks h;
  h.resources=[&](unsigned id){return f.world.resources(id);};h.events=[&](const auto& values){packets.insert(packets.end(),values.begin(),values.end());};
  h.consume_energy_at=[&](unsigned cost,auto at){return f.world.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned id){auto scope=f.world.effect_scope(id);if(stale&&id==f.target.actor)return std::optional<effects::Scope>{};return scope;};
  h.weapon_snapshot=[&](){auto weapon=f.weapon;if(mode==6)weapon.item_type=27;return std::optional{weapon};};
  h.melee_target=[&](unsigned id,auto at){return f.world.melee_target(f.source,id,at,403);};
  if(mode!=5)h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto at){++commits;return f.world.melee_attack(f.source,target,weapon,plan,at);};
  h.attack_activity=[&](auto until){f.world.attack_activity(f.source,until,1000);};
  h.attack_interrupt_epoch=[&](){return f.world.attack_interrupt_epoch(f.source);};
  h.weapon_attack_timing=[&](auto at){return f.world.weapon_attack_timing(f.source,at);};
  casting::Runtime cast(1);const auto reason=cast.start(1000,{403,0,f.target.actor},bar,policy,ranks,true,h);
  check(reason==(mode==5?casting::Reason::unsupported:mode==6?casting::Reason::weapon:casting::Reason::accepted));
  if(mode>=5){check(f.world.resources(1)->energy==30);continue;}
  check(f.world.resources(1)->energy==25);
  check(std::count(packets.begin(),packets.end(),casting::activation(1,f.target.actor,403,true))==1);
  if(mode==2)cast.cancel(h);if(mode==3)cast.interrupt(1200,h);if(mode==4)stale=true;
  cast.update(1499,h);check(!commits);cast.update(1500,h);f.world.tick(1600);
  check(!draws); // Unblockable still misses under Blind; it does not draw block RNG.
  check(f.health()==(mode==0?980:1000));
  check(std::count(packets.begin(),packets.end(),casting::terminal(46,1))==0);
  check(std::count(packets.begin(),packets.end(),combat::weapon::finish_2022(1,true))==unsigned(mode<2));
  const auto outcomes=f.world.take_attack_outcomes(f.source);check(outcomes.size()==unsigned(mode<2));
  if(mode<2)check(outcomes.front().outcome==(mode==1?combat::weapon::AttackOutcome::miss:combat::weapon::AttackOutcome::hit));
 }
 std::cout<<"PASS Called Shot triple-speed flight,nonstacking preparation,player/NPC casts,block bypass,Blind,cancel,interrupt,stale target and weapon policy\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
