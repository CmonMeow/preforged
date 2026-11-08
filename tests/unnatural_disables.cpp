#include "gw_unnatural_disables.h"
#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Unnatural disables line "+std::to_string(at.line()));}
int main()try{
 const Scope owner{1,2,3};CooldownBarSnapshot bar{{owner,1,100},1,{}};
 bar.bar[0]={3188,0};bar.bar[1]={322,0};bar.bar[2]={322,1};bar.bar[3]={1,0};bar.bar[4]={281,0};bar.bar[5]={26,0};bar.bar[6]={4,0};
 UnnaturalDisables initial(owner);const auto next=initial.propose(owner,bar,1,100);check(next.has_value());const auto before=*next;
 for(auto slot:{Slot{322,0},Slot{322,1},Slot{281,0},Slot{4,0}})check(next->disabled_until(owner,slot,200)==10100);
 for(auto slot:{Slot{3188,0},Slot{1,0},Slot{26,0},Slot{322,2}})check(next->disabled_until(owner,slot,200)==0);
 check(!next->propose(owner,bar,1,200)&&*next==before);
 const auto reset=next->propose_reset_slots(owner,{{322,0}},300);check(reset&&reset->disabled_until(owner,{322,0},300)==0&&reset->disabled_until(owner,{322,1},300)==10100);
 check(!reset->propose(owner,bar,1,400));check(reset->disabled_until(owner,{322,1},10100)==0);
 auto malformed=bar;malformed.bar[7]={322,0};check(!initial.propose(owner,malformed,1,100));
 malformed=bar;malformed.bar[7]={0,1};check(!initial.propose(owner,malformed,1,100));
 malformed=bar;malformed.published.at=200;check(!initial.propose(owner,malformed,1,100));
 auto stale=owner;++stale.generation;check(!initial.propose(stale,bar,1,100)&&!next->disabled_until(stale,{322,0},200));
 check(!initial.propose(owner,bar,1,UINT64_MAX-9999));
 auto changed=bar;changed.bar={};changed.bar[0]={322,2};changed.published={owner,2,400};changed.revision=2;
 const auto again=reset->propose(owner,changed,2,400);check(again&&again->disabled_until(owner,{322,1},400)==10100&&again->disabled_until(owner,{322,2},400)==10400);
 {warrior_test::Fixture f(340,false);auto& w=f.world;Slots equipped{};equipped[0]={389,0};equipped[1]={322,0};equipped[2]={322,1};equipped[3]={1,0};equipped[4]={281,0};equipped[5]={26,0};
  check(w.configure_adrenaline(f.source,equipped,100));const auto old=*w.disable_reset_revision(f.source);check(old.size()==11&&old[5]==0&&old[7]==0&&old[10]==0);
  check(w.apply_unnatural_disables(f.source,1,100));check(!w.reset_skill_disables(f.source,{{322,0}},old,100));
  check(w.skill_disabled_slot(f.source,{322,0},100)==10100&&w.skill_disabled_slot(f.source,{322,1},100)==10100);
  check(w.skill_disabled_slot(f.source,{1,0},100)==0&&w.skill_disabled_slot(f.source,{26,0},100)==0);
  const auto fresh=*w.disable_reset_revision(f.source);check(fresh[5]!=0&&w.reset_skill_disables(f.source,{{322,0}},fresh,200));
  check(w.skill_disabled_until(f.source,322,200)==0&&w.skill_disabled_slot(f.source,{322,1},200)==10100);
  check(!w.apply_unnatural_disables(f.source,1,200));
  w.set_vitals(1,{1000,1000,10,30},100);const auto permit=*w.prepare_flourish_reset(f.source,300);
  FlourishCooldowns snapshot{f.source,1,0,300,true,{}};
  for(const auto slot:equipped)if(slot.skill)snapshot.slots.push_back({slot,0,*w.skill_disabled_slot(f.source,slot,300)});
  const auto proposal=*prepare_flourish_recharge(snapshot,*flourish(12),1);check(proposal.energy==6);
  check(w.commit_flourish_reset(proposal,permit,300).has_value()&&w.resources(1)->energy==16);
  check(w.skill_disabled_slot(f.source,{322,1},300)==0&&w.skill_disabled_slot(f.source,{281,0},300)==10100);
  w.leave(1);w.join(1,"New",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,30,30},100);
  check(w.skill_disabled_slot(*w.effect_scope(1),{322,1},300)==0&&!w.skill_disabled_slot(f.source,{322,1},300));
 }
 std::cout<<"PASS staged Unnatural PvP disable authority,current copies/predicate/replay/expiry,six-source reset/Flourish/lifetime;PvP damage integration validated separately\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
