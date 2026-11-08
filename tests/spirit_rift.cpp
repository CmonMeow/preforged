#include "gw_spirit_rift.h"
#include "gw_skill_particle_wire.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Rift line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){
  const auto plan=spirit_rift(rank);check(plan&&plan->delay==3000&&plan->radius==166&&plan->cracked_ms==Time(std::lround(1.+19.*rank/15))*1000);
  combat::Actor target;target.id=2;target.generation=1;target.health=combat::Health{1000,1000};target.lightning_armor=80;target.damageable=true;
  const auto first=spirit_rift_hit(*plan,target,{},20,1,1000,true);check(first&&first->cracked_added);
  check(std::abs(first->hit.applied-plan->damage/std::sqrt(2.))<1e-6);
  auto renewed=target;renewed.health->current=1000;
  const auto next=spirit_rift_hit(*plan,renewed,first->conditions,20,1,1100,true);check(next&&std::abs(next->hit.applied-plan->damage)<1e-6);
  const auto immune=spirit_rift_hit(*plan,target,{},20,1,1000,false);check(immune&&!immune->cracked_added&&immune->hit.applied==first->hit.applied);
  auto unknown=target;unknown.lightning_armor.reset();check(!spirit_rift_hit(*plan,unknown,{},20,1,1000,true));
  auto dead=target;dead.health->current=1;const auto lethal=spirit_rift_hit(*plan,dead,{},20,1,1000,true);check(lethal&&lethal->hit.killed&&!lethal->cracked_added);
  auto bad=*plan;++bad.damage;check(!spirit_rift_hit(bad,target,{},20,1,1000,true));
 }
 SpiritRiftSchedule queue;const Scope owner{1,1,1};auto p=*spirit_rift(15);unsigned hits=0;
 check(queue.add(owner,20,100,200,2,p,1000)&&queue.next_deadline()==4000);
 queue.advance(3999,[&](const auto&){++hits;});check(!hits);
 queue.advance(4000,[&](const auto& event){++hits;check(event.x==100&&event.y==200&&event.plane==2&&event.at==4000);});
 queue.advance(9000,[&](const auto&){++hits;});check(hits==1&&!queue.next_deadline());
 check(!queue.add(owner,20,0,0,0,p,8999));check(queue.add(owner,20,0,0,0,p,9000));queue.retire({1,1,2});check(queue.size()==1);queue.retire(owner);check(!queue.size());
 const auto packet=skills::particles::spirit_rift_ground_2022(42,100,200,2);
 check(packet.size()==20&&packet[0]==160&&packet[1]==128&&packet[10]==2&&packet[12]==42&&packet[16]==79&&packet[17]==4);
 bool rejected=false;try{(void)skills::particles::spirit_rift_ground_2022(0,100,200,2);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 check(!spirit_rift(21));std::cout<<"PASS Rift21rank damage/condition contexts,fixed ground center,3s/single delivery,retirement and clock guards;not a live binding\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

