#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Endurance World line "+std::to_string(at.line()));}
int main()try{
 for(bool npc:{false,true})for(unsigned mode=0;mode<7;++mode){warrior_test::Fixture f(383,npc);auto& w=f.world;
 w.set_vitals(1,{1000,1000,mode==4?21u:mode==5?24u:10u,30},100);
check(w.energy_capacity(1,1,0,100,{10,-2}));
 check(w.status_mutation(f.source,374,123,12,100,29100,true));
 if(mode==1)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,30000));
 if(mode==2&&!npc)check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));
 if(mode==3)check(w.status_mutation(f.source,374,123,12,200,{},false));
 w.configure_attack_block_draws([]{return 0.;});
 check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(383,12),mode==6?29100:1000));
 const auto expected=mode==4?22u:mode==5?24u:mode==1||mode==3||mode==6||mode==2&&!npc?10u:13u;
 check(w.resources(1)->energy==expected);
 }
 for(bool pvp:{false,true}){warrior_test::Fixture f(383,false);
check(f.world.status_mutation(f.source,344,123,0,100,5100,true));casting::Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return f.world.resources(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};h.consume_energy=[&](unsigned n){check(n==5);return f.world.consume_energy(1,n,f.source);};h.effect_snapshot=[&](auto now){return *f.world.status_snapshot(f.source,now);};h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool active){return f.world.status_mutation(f.source,skill,effect,rank,now,until,active);};Policy p;p.primary=1;p.campaigns=255;p.pvp_match=pvp;p.learned[374]=1;Slots bar{};bar[0]={374,0};attributes::Ranks ranks{};ranks[17]=12;casting::Runtime runtime(1);
check(runtime.start(1000,{374,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);
auto entries=*f.world.status_snapshot(f.source,1000);
check(entries.size()==(pvp?1u:2u));
check(std::any_of(entries.begin(),entries.end(),[&](auto e){return e.skill==(pvp?3002u:374u);}));
check(f.world.consume_energy(1,15,f.source));
check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(383,12),1100));
check(f.world.resources(1)->energy==13);
check(f.world.status_mutation(f.source,349,124,0,1200,9200,true));
check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(383,12),1300));
check(f.world.resources(1)->energy==(pvp?13u:16u));
check(f.world.kill(f.source,1400));
check(f.world.status_snapshot(f.source,1400)->empty());}
 {warrior_test::Fixture f(330,false);f.world.set_vitals(1,{1000,1000,10,30},100);
check(f.world.energy_capacity(1,1,0,100,{10,-2}));f.world.join(3,"Foe",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.world.set_vitals(3,{1000,1000,30,30},100);
check(f.world.physical_armor(*f.world.effect_scope(3),60));
check(f.world.status_mutation(f.source,374,123,12,100,29100,true));
check(f.world.melee_attack(f.source,f.source,f.weapon,*combat::weapon::attack_skill(330,0),1000));
check(f.world.resources(1)->energy==16);}
 {warrior_test::Fixture f(383,false);f.world.set_vitals(1,{1000,1000,10,30},100);
check(f.world.energy_capacity(1,1,0,100,{10,-2}));
check(f.world.status_mutation(f.source,374,123,12,100,29100,true));auto bow=f.weapon;bow.item_type=5;bow.damage.raw_damage_type=1;
check(f.world.ordinary_weapon(f.source,bow,20));
check(f.world.ordinary_attack(f.source,2,1000));f.world.tick(2000);
check(f.world.ordinary_weapon(f.source,f.weapon,20));f.world.tick(2200);
check(f.world.resources(1)->energy==10&&f.health()<1000);}
 std::cout<<"PASS Endurance player/NPC hits,miss/removal/expiry/caps,mode-specific stance ownership,multi-target rewards,bow release identity and death\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
