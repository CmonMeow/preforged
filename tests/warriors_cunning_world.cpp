#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Cunning line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank=0;rank<=20;++rank)check(warriors_cunning(rank)==1000ull*interpolate(5,11,rank));
 const Entry e{123,1,362,0,Category::other,100,5100};
 for(unsigned type:{2u,5u,15u,22u,26u,27u,32u,35u,36u})check(warriors_cunning_active(std::span<const Entry>(&e,1),type,1000)==(type==2||type==15||type==27||type==32||type==35));
 for(unsigned mode=0;mode<5;++mode){warrior_test::Fixture f(383,false);auto& w=f.world;
 check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));
 check(w.status_mutation(f.source,362,123,0,100,5100,true));
 if(mode==1)check(w.apply_condition(f.target,f.source,combat::conditions::Kind::blind,100,6000));
 if(mode==2)check(w.status_mutation(f.source,362,123,0,200,{},false));
 if(mode==3)check(w.status_mutation(f.source,344,124,0,100,5100,true));
 unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return 0.;});
 check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(322,0),mode==4?5100:1000));
 const auto events=w.take_attack_outcomes(f.source);check(events.size()==1);using Outcome=combat::weapon::AttackOutcome;
 check(events.front().outcome==(mode==1?Outcome::miss:mode==2||mode==4?Outcome::block:Outcome::hit));
 check(draws==(mode==2||mode==4?1u:0u));
 }
 {warrior_test::Fixture f(383,false);auto bow=f.weapon;bow.item_type=5;bow.damage.raw_damage_type=1;check(f.world.ordinary_weapon(f.source,bow,20));check(f.world.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));check(f.world.status_mutation(f.source,362,123,0,100,5100,true));unsigned draws=0;f.world.configure_attack_block_draws([&]{++draws;return 0.;});check(f.world.ordinary_attack(f.source,2,1000));f.world.tick(2000);f.world.tick(2200);const auto outcomes=f.world.take_attack_outcomes(f.source);check(outcomes.size()==1&&outcomes.front().outcome==combat::weapon::AttackOutcome::block&&draws==1);check(f.health()==1000);}
 {warrior_test::Fixture f(330,false);check(f.world.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));check(f.world.status_mutation(f.source,362,123,0,100,5100,true));f.world.configure_attack_block_draws([]()->double{throw std::runtime_error("unexpected Cyclone block draw");});check(f.world.melee_attack(f.source,f.source,f.weapon,*combat::weapon::attack_skill(330,0),1000));check(f.health()<1000);}
 {warrior_test::Fixture f(383,false);check(f.world.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));check(f.world.status_mutation(f.source,362,123,0,100,5100,true));f.world.configure_attack_block_draws([]()->double{throw std::runtime_error("unexpected ordinary block draw");});check(f.world.ordinary_attack(f.source,2,1000));f.world.tick(2000);check(f.health()<1000);}
 {warrior_test::Fixture f(383,false);casting::Hooks h;std::vector<world::Packet> packets;h.events=[&](const auto& p){packets.insert(packets.end(),p.begin(),p.end());};h.resources=[&](auto id){return f.world.resources(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};h.consume_energy=[&](unsigned n){check(n==10);return f.world.consume_energy(1,n,f.source);};h.effect_snapshot=[&](auto now){return *f.world.status_snapshot(f.source,now);};h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool active){return f.world.status_mutation(f.source,skill,effect,rank,now,until,active);};Policy p;p.primary=1;p.campaigns=255;p.learned[362]=1;Slots bar{};bar[0]={362,0};attributes::Ranks ranks{};ranks[17]=12;casting::Runtime runtime(1);
 check(runtime.start(1000,{362,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);
 check(std::count(packets.begin(),packets.end(),casting::generic(48,1,362))==1);
 check(f.world.status_snapshot(f.source,1000)->front().expires==11000);
 check(f.world.kill(f.source,1100));check(f.world.status_snapshot(f.source,1100)->empty());
 }
 std::cout<<"PASS Cunning melee-only bypass,Blind preservation,no block RNG,ordinary/Cyclone contact,stance coexistence,expiry/removal and Runtime instant/self/death\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
