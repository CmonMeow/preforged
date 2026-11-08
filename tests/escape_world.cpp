#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Escape line "+std::to_string(at.line()));}
int main()try{
 for(unsigned id:{448u,3060u})for(unsigned rank=0;rank<=20;++rank)check(escape(id,rank)==1000ull*interpolate(1,8,rank));
 for(unsigned id:{448u,3060u})for(bool bow:{false,true})for(bool ordinary:{false,true}){
 warrior_test::Fixture f(383,false);auto& w=f.world;
 if(bow){auto weapon=f.weapon;weapon.item_type=5;weapon.damage.raw_damage_type=1;check(w.ordinary_weapon(f.source,weapon,20));}
 check(w.status_mutation(f.source,id,123,15,100,8100,true));
 if(ordinary)check(w.ordinary_attack(f.source,2,200));else check(w.attack_activity(f.source,700,200));
 const auto entries=*w.status_snapshot(f.source,200);
 check(escape_active(entries,200)==(id==448||bow));
 }
 for(unsigned type:{32u,35u,36u}){warrior_test::Fixture f(383,false);auto weapon=f.weapon;weapon.item_type=type;check(f.world.ordinary_weapon(f.source,weapon,20));check(f.world.status_mutation(f.source,3060,123,15,100,8100,true));check(f.world.attack_activity(f.source,700,200));check(escape_active(*f.world.status_snapshot(f.source,200),200)==(type==36));}
 for(unsigned mode=0;mode<5;++mode){warrior_test::Fixture f(383,false);auto& w=f.world;
 check(w.status_mutation(f.target,448,123,15,1000,9000,true));
 w.move(2,10000,0,1000);w.tick(2000);check(std::abs(w.position(2,2000).first-(60+288*1.33))<.3);
 if(mode==1)check(w.status_mutation(f.target,371,124,0,2000,10000,true));
 if(mode==2)check(w.status_mutation(f.target,448,123,15,2000,{},false));
 if(mode==3)check(w.kill(f.target,2000));
 if(mode==1||mode==2){w.tick(3000);check(std::abs(w.position(2,3000).first-(60+288*1.33+288))<.3);}
 if(mode==3)check(w.status_snapshot(f.target,2000)->empty());
 if(mode==4){w.tick(9000);check(!escape_active(*w.status_snapshot(f.target,9000),9000));}
 }
 for(bool pvp:{false,true}){warrior_test::Fixture f(383,false);casting::Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return f.world.resources(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};h.consume_energy=[&](unsigned n){check(n==5);return f.world.consume_energy(1,n,f.source);};h.effect_snapshot=[&](auto now){return *f.world.status_snapshot(f.source,now);};h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool active){return f.world.status_mutation(f.source,skill,effect,rank,now,until,active);};Policy p;p.primary=2;p.campaigns=255;p.pvp_match=pvp;p.learned[448]=1;Slots bar{};bar[0]={448,0};attributes::Ranks ranks{};casting::Runtime runtime(1);
 check(runtime.start(1000,{448,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);
 const auto entries=*f.world.status_snapshot(f.source,1000);check(entries.size()==1&&entries.front().skill==(pvp?3060:448));
 check(f.world.attack_activity(f.source,1500,1100));
 check(escape_active(*f.world.status_snapshot(f.source,1100),1100)==!pvp);
 runtime.update(1100,h);
 }
 {warrior_test::Fixture f(383,false);check(f.world.status_mutation(f.target,448,123,15,100,8100,true));unsigned draws=0;f.world.configure_attack_block_draws([&]{++draws;return .74;});check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(383,12),200));check(f.world.take_attack_outcomes(f.source).front().outcome==combat::weapon::AttackOutcome::block&&draws==1);}
 std::cout<<"PASS Escape timed speed,blocking,cleanup and PvP-only melee-start termination with bow preservation and Runtime mode ownership\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
