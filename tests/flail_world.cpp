#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Flail line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank=0;rank<=20;++rank)check(flail(rank)==1000ull*interpolate(1,15,rank));
 check(!flail(21));
 for(unsigned mode=0;mode<4;++mode){warrior_test::Fixture f(383,false);auto& w=f.world;
 check(!w.status_mutation(f.target,1404,123,15,1000,15999,true));
 check(w.status_mutation(f.target,1404,123,15,1000,16000,true));
 check(w.attack_duration(f.target,1000,1000)==670);
 w.move(2,10000,0,1000);w.tick(2000);
 check(std::abs(w.position(2,2000).first-(60+288*.67))<.3);
 if(mode==0)check(w.status_mutation(f.target,1404,123,15,2000,{},false));
 if(mode==1)check(w.status_mutation(f.target,371,124,0,2000,10000,true));
 if(mode==2)check(w.kill(f.target,2000));
 if(mode<2){w.tick(3000);check(std::abs(w.position(2,3000).first-(60+288*.67+288))<.3);check(w.attack_duration(f.target,1000,3000)==1000);}
 if(mode==2)check(w.status_snapshot(f.target,2000)->empty());
 if(mode==3){w.tick(16000);check(w.attack_duration(f.target,1000,16000)==1000);w.tick(17000);check(std::abs(w.position(2,17000).first-w.position(2,16000).first-288)<.3);}
 }
 for(bool ready:{false,true}){warrior_test::Fixture f(383,false);Slots bar{};bar[0]={1404,0};check(f.world.configure_adrenaline(f.source,bar,100));
 if(ready)for(unsigned n=0;n<4;++n)check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(322,0),101+n));
 casting::Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return f.world.resources(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};h.consume_energy=[](unsigned n){check(n==0);return true;};h.effect_snapshot=[&](auto now){return *f.world.status_snapshot(f.source,now);};h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool active){return f.world.status_mutation(f.source,skill,effect,rank,now,until,active);};h.consume_adrenaline=[&](auto slot,auto now){return f.world.consume_adrenaline(f.source,slot,now);};h.reconcile_adrenaline=[&](auto now){f.world.reconcile_adrenaline(f.source,now);};Policy p;p.primary=1;p.campaigns=255;p.learned[1404]=1;attributes::Ranks ranks{};ranks[17]=12;casting::Runtime runtime(1);
 check(runtime.start(1000,{1404,0,2},bar,p,ranks,true,h)==(ready?casting::Reason::accepted:casting::Reason::adrenaline));
 if(ready){runtime.update(1000,h);check(f.world.adrenaline(f.source)->at(0).units==0);check(f.world.attack_duration(f.source,1000,1000)==670);check(f.world.status_snapshot(f.source,1000)->front().expires==13000);}
 }
 std::cout<<"PASS Flail timed movement,haste,canonical duration,replacement/removal/death/expiry and real four-strike adrenaline Runtime admission\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
