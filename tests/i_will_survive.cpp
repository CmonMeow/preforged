#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Survival line "+std::to_string(at.line()));}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope owner;
 Fixture(){w.join(1,"Warrior",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);owner=*w.effect_scope(1);}
 void condition(combat::conditions::Kind k,unsigned at,unsigned duration){check(w.apply_condition(owner,owner,k,at,duration));}
};
int main()try{
 check(i_will_survive(0)->duration==5000&&i_will_survive(12)->duration==10000&&i_will_survive(15)->duration==11000&&!i_will_survive(21));
 for(bool delayed:{false,true}){Fixture f;using enum combat::conditions::Kind;
  f.condition(blind,100,2000);f.condition(bleeding,100,1000);check(f.w.i_will_survive(f.owner,*i_will_survive(0),100));
  f.w.tick(1100);check(f.w.resources(1)->health==506);f.condition(poison,1100,2000);
  if(!delayed){f.w.tick(2100);check(f.w.resources(1)->health==510);f.w.tick(3100);check(f.w.resources(1)->health==508);}
  f.w.tick(5100);check(f.w.resources(1)->health==508&&f.w.status_snapshot(f.owner,5100)->empty());
 }
 {Fixture f;using enum combat::conditions::Kind;check(f.w.i_will_survive(f.owner,*i_will_survive(0),100));f.w.tick(1100);check(f.w.resources(1)->health==500);
  f.condition(blind,1100,20000);f.w.tick(2100);check(f.w.resources(1)->health==506);
  check(f.w.i_will_survive(f.owner,*i_will_survive(0),2100));f.w.tick(3100);check(f.w.resources(1)->health==512);
  f.w.tick(7100);check(f.w.resources(1)->health==536);f.w.tick(8100);check(f.w.resources(1)->health==536);
 }
 {Fixture f;using enum combat::conditions::Kind;for(auto k:{blind,crippled,dazed,weakness})f.condition(k,100,10000);
  check(f.w.i_will_survive(f.owner,*i_will_survive(0),100));f.w.tick(1100);check(f.w.resources(1)->health==520); // joint cap10
 }
 {Fixture f;f.condition(combat::conditions::Kind::burning,100,10000);
  check(f.w.i_will_survive(f.owner,*i_will_survive(0),100));f.w.tick(1100);check(f.w.resources(1)->health==492);
 }
 for(unsigned mode=0;mode<5;++mode){Fixture f;casting::Runtime cast(1);casting::Hooks h;unsigned effects=0;std::vector<Packet> wire;
  h.events=[&](const auto& batch){wire.insert(wire.end(),batch.begin(),batch.end());};h.resources=[&](auto id){return f.w.resources(id);};
  h.consume_energy_at=[&](auto cost,auto now){return f.w.consume_energy(1,cost,f.owner,now);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.target_valid=[](auto id,float,bool){return id==1;};
  h.i_will_survive=[&](auto owner,const auto& plan,auto now){++effects;return mode!=2&&f.w.i_will_survive(owner,plan,now);};
  Policy p;p.primary=1;p.campaigns=255;p.learned[368]=1;Slots bar{};bar[0]={368,0};attributes::Ranks rank{};rank[17]=12;
  if(mode==1)h.i_will_survive={};
  if(mode==3){auto stale=f.owner;++stale.generation;h.target_scope=[stale](auto)->std::optional<Scope>{return stale;};}
  check(cast.start(100,{368,0,99},bar,p,rank,true,h)==(mode==1?casting::Reason::unsupported:casting::Reason::accepted));
  check(!cast.active());check(effects==(mode==1?0u:1u));
  check(f.w.resources(1)->energy==(mode==1?100u:95u));
  if(mode==0||mode==4){
   check(std::count(wire.begin(),wire.end(),casting::generic(48,1,368))==1);
   check(std::count(wire.begin(),wire.end(),casting::terminal(58,1))==0);
   check(f.w.status_snapshot(f.owner,100)->size()==1);
   check(cast.start(101,{368,0,1},bar,p,rank,true,h)==casting::Reason::recharge);
   const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(21,1,627))==1);
  }else check(f.w.status_snapshot(f.owner,100)->empty());
 }
 {Fixture f;casting::Runtime cast(1);casting::Hooks h;
  h.events=[](const auto&){};h.resources=[&](auto id){return f.w.resources(id);};
  h.consume_energy_at=[&](auto cost,auto now){return f.w.consume_energy(1,cost,f.owner,now);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.target_valid=[](auto id,float,bool){return id==1;};
  h.heal=[](auto,auto,float,bool){return true;};h.i_will_survive=[&](auto owner,const auto& plan,auto now){return f.w.i_will_survive(owner,plan,now);};
  Policy p;p.primary=1;p.secondary=3;p.campaigns=255;p.learned[368]=p.learned[281]=1;Slots bar{};bar[0]={368,0};bar[1]={281,0};attributes::Ranks ranks{};
  check(cast.start(100,{281,0,1},bar,p,ranks,true,h)==casting::Reason::accepted&&cast.active());
  check(cast.start(200,{368,0,1},bar,p,ranks,true,h)==casting::Reason::accepted&&cast.active());
  cast.update(1100,h);check(!cast.active()&&f.w.resources(1)->energy==90);
 }
 {Fixture f;f.w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(2,{1000,1000,100,100},100);
  check(f.w.gale(*f.w.effect_scope(2),f.owner,12,{},100));check(f.w.knocked_down(f.owner,100));
  check(!casting::Runtime::knockdown_restricted(368)&&f.w.i_will_survive(f.owner,*i_will_survive(0),100));
 }
 std::cout<<"PASS I Will Survive dynamic condition gain/expiry,delayed ticks,joint cap,renewal,negative pips and instant lifecycle\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
