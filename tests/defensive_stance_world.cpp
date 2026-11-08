#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <string>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Defensive Stance World line "+std::to_string(at.line()));}
unsigned charge(world::Instance& w,Scope owner,unsigned skill){const auto pools=w.adrenaline(owner);for(const auto& p:*pools)if(p.slot.skill==skill)return p.units;throw std::runtime_error("missing pool");}
int main()try{
 for(unsigned mode=0;mode<11;++mode){
  warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={345,0};bar[1]={340,0};
  check(w.configure_adrenaline(f.source,bar,100));
  for(unsigned i=0;i<5;++i)check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),200+i));
  check(charge(w,f.source,345)==120);
  casting::Runtime runtime(1);casting::Hooks h;std::vector<Packet> packets;
  h.events=[&](const auto& b){packets.insert(packets.end(),b.begin(),b.end());};h.resources=[&](auto id){return w.resources(id);};
  h.target_valid=[](auto id,float,bool self){return id==1&&self;};h.consume_energy=[&](auto n){check(n==0);return w.consume_energy(1,n,f.source);};
  h.consume_adrenaline=[&](auto slot,auto at){return w.consume_adrenaline(f.source,slot,at);};h.reconcile_adrenaline=[&](auto at){check(w.reconcile_adrenaline(f.source,at));};
  h.effect_snapshot=[&](auto at){return *w.status_snapshot(f.source,at);};h.status_mutation=[&](auto s,auto e,auto r,auto at,auto until,bool active){return w.status_mutation(f.source,s,e,r,at,until,active);};
  Policy p;p.primary=1;p.campaigns=255;p.learned[345]=1;p.learned[340]=1;attributes::Ranks ranks{};ranks[21]=12;
  check(runtime.start(1000,{345,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);
  check(charge(w,f.source,345)==0&&w.resources(1)->energy==30);
  check(std::count(packets.begin(),packets.end(),casting::generic(21,1,618))==1);
  if(mode==1)check(w.status_mutation(f.source,371,124,0,2000,10000,true));
  if(mode==2)check(w.status_mutation(f.source,345,1,15,2000,{},false));
  if(mode==3||mode==10)check(w.status_mutation(f.source,56,125,0,2000,12000,true));
  if(mode==4)check(w.kill(f.source,2000));
  if(mode==5){Slots changed=bar;changed[2]={331,0};check(w.configure_adrenaline(f.source,changed,7000));}
  if(mode==6)check(w.reconcile_adrenaline(f.source,7000));
  if(mode==7){auto changed=bar;changed[2]={331,0};check(w.publish_skillbar_transition(f.source,{bar,changed},7000));}
  if(mode==8)check(w.status_mutation(f.source,345,126,12,2000,6000,true));
  if(mode==9)check(!w.status_mutation(f.source,371,127,21,2000,10000,true));
  if(mode==10)check(w.status_mutation(f.source,56,125,0,7000,{},false));
  w.tick(7000);
  check(charge(w,f.source,345)==unsigned(mode==3||mode==4||mode==10?0:mode==8?50:25));
  runtime.update(7000,h);w.tick(7100);check(charge(w,f.source,345)==unsigned(mode==3||mode==4||mode==10?0:mode==8?50:25));
 }
 for(bool blocked:{false,true}){
  warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={345,0};bar[1]={340,0};check(w.configure_adrenaline(f.target,bar,100));
  check(w.status_mutation(f.target,345,123,15,100,5100,true));w.configure_attack_block_draws([&]{return blocked?.74:.75;});
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),200));
  check(f.health()==unsigned(blocked?1000:980));
 }
 std::cout<<"PASS Defensive Stance Runtime120-unit spend/no Energy,body618,World timed/replaced/removed/forbidden/death rewards,bar-change ordering,duplicate suppression and block threshold\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
