#include "gw_skull_crack.h"
#include "gw_adrenaline.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;using namespace combat::weapon;
void check(bool b){if(!b)throw std::runtime_error("Skull Crack policy");}
int main()try{
 skills::Slots bar{};bar[0]={329,0};bar[1]={340,0};combat::adrenaline::State pools;
 check(pools.configure(bar,100)&&pools.gain(219,101));check(!pools.ready({329,0},101));
 check(pools.gain(1,102)&&pools.ready({329,0},102));check(pools.spend({329,0},103));check(pools.pools()[0].units==0&&pools.pools()[1].units==125);
 const auto p=skull_crack();check(p&&p->adrenaline==220&&p->windup==500&&p->dazed_ms==15000&&p->target_body==576);
 for(unsigned spell:{281u,187u,192u}){const AttackActionFacts a{AttackTargetAction::skill,skills::Slot{static_cast<std::uint16_t>(spell),0}};
  const auto hit=skull_crack_contact(*p,a,AttackOutcome::hit,100,true);check(hit&&hit->attempt_interrupt&&hit->apply_dazed&&hit->duration==15000);
  const auto immune=skull_crack_contact(*p,a,AttackOutcome::hit,100,false);check(immune&&immune->attempt_interrupt&&!immune->apply_dazed&&!immune->duration);
 }
 for(unsigned skill:{1u,361u,362u}){const auto hit=skull_crack_contact(*p,{AttackTargetAction::skill,skills::Slot{static_cast<std::uint16_t>(skill),0}},AttackOutcome::hit,100,true);check(hit&&hit->attempt_interrupt&&!hit->apply_dazed);}
 check(skull_crack_contact(*p,{AttackTargetAction::attack,{}},AttackOutcome::hit,100,true)->attempt_interrupt);
 const auto idle=skull_crack_contact(*p,{},AttackOutcome::hit,100,true);check(idle&&!idle->attempt_interrupt&&!idle->apply_dazed);
 for(auto outcome:{AttackOutcome::miss,AttackOutcome::block,AttackOutcome::dodge,AttackOutcome::obstruction,AttackOutcome::cancelled,AttackOutcome::replaced,AttackOutcome::target_retired}){const auto hit=skull_crack_contact(*p,{AttackTargetAction::skill,{}},outcome,100,true);check(hit&&!hit->attempt_interrupt&&!hit->apply_dazed);}
 check(!skull_crack_contact(*p,{AttackTargetAction::skill,{}},AttackOutcome::hit,100,true));
 check(!skull_crack_contact(*p,{AttackTargetAction::skill,skills::Slot{281,0}},AttackOutcome::hit,UINT64_MAX-14999,true));
 auto forged=*p;forged.dazed_ms=10000;check(!skull_crack_contact(forged,{},AttackOutcome::hit,100,true));
 std::cout<<"PASS staged Skull Crack native220adrenaline/500ms,15s frozen condition duration,spell/nonspell/idle/miss/block/unknown/overflow and independent condition eligibility;World/Runtime pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
