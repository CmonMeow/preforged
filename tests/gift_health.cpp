#include "gw_gift_health.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw::skills;using namespace effects;
void check(bool b,std::source_location l=std::source_location::current()){if(!b)throw std::runtime_error("Gift Health line "+std::to_string(l.line()));}
int main()try{
 const Scope owner{1,25,1};Slots slots{};slots[0]={1121,0};slots[1]={1119,0};slots[2]={1120,0};slots[3]={281,1};slots[4]={281,2};slots[5]={74,0};slots[6]={293,0};slots[7]={1121,1};
 CooldownBarAuthority authority(owner);const auto bar=*authority.publish(owner,slots,100);GiftHealthDisables original(owner);
 for(unsigned rank=0;rank<=20;++rank){
  const auto p=gift_health(rank);check(p&&p->healing==15+9*rank&&gift_health_amount(*p)==p->healing);
  const auto proposal=original.propose(owner,*p,bar,1,200);check(proposal.has_value());
  for(unsigned n:{1u,2u,3u,4u}){check(original.disabled_until(owner,slots[n],200)==0);check(proposal->disabled_until(owner,slots[n],200)==200+p->disable);check(proposal->disabled_until(owner,slots[n],199+p->disable)==200+p->disable);check(proposal->disabled_until(owner,slots[n],200+p->disable)==0);}
  for(unsigned n:{0u,5u,6u,7u})check(proposal->disabled_until(owner,slots[n],200)==0);
  check(proposal->disabled_until(owner,{281,0},200)==0); // Exact copy identity.
  auto forged=*p;++forged.disable;check(!original.propose(owner,forged,bar,1,200));
  ++forged.healing;check(!gift_health_amount(forged));
 }
 check(gift_health(0)->disable==10000&&gift_health(15)->disable==5000&&gift_health(20)->disable==3000&&!gift_health(21));
 const auto first=*original.propose(owner,*gift_health(0),bar,1,200);
 check(!first.propose(owner,*gift_health(0),bar,1,300));
 const auto again=*first.propose(owner,*gift_health(20),bar,2,300);check(again.disabled_until(owner,slots[1],300)==10200); // Cannot shorten old disable.
 const auto reset=*again.propose_reset_slots(owner,{slots[1],slots[3]},400);check(reset.disabled_until(owner,slots[1],400)==0&&reset.disabled_until(owner,slots[2],400)==10200&&reset.disabled_until(owner,slots[3],400)==0&&reset.disabled_until(owner,slots[4],400)==10200);
 check(!reset.propose(owner,*gift_health(0),bar,2,500));
 auto corrupt=bar;corrupt.bar[7]=corrupt.bar[3];check(!original.propose(owner,*gift_health(0),corrupt,1,200));
 corrupt=bar;corrupt.bar[7]={0,1};check(!original.propose(owner,*gift_health(0),corrupt,1,200));
 corrupt=bar;corrupt.published.at=201;check(!original.propose(owner,*gift_health(0),corrupt,1,200));
 check(!original.propose({1,25,2},*gift_health(0),bar,1,200));check(!first.disabled_until({1,25,2},slots[1],200));
 check(!original.propose(owner,*gift_health(0),bar,0,200));check(!first.propose(owner,*gift_health(0),bar,2,199));check(!original.propose(owner,*gift_health(0),bar,1,UINT64_MAX));
 auto retired=first;retired.retire();check(!retired.disabled_until(owner,slots[1],200));
 Slots empty{};CooldownBarAuthority blank(owner);const auto emptybar=*blank.publish(owner,empty,100);check(original.propose(owner,*gift_health(0),emptybar,1,200).has_value());
 // Healing Prayers signets are skills too: do not filter only native spells.
 Slots signet{};signet[0]={887,0};CooldownBarAuthority signet_authority(owner);const auto signetbar=*signet_authority.publish(owner,signet,100);
 const auto disabled_signet=*original.propose(owner,*gift_health(0),signetbar,1,200);check(disabled_signet.disabled_until(owner,{887,0},200)==10200);
 std::cout<<"PASS21 Gift of Health plans; exact-copy attribute disable consumer, exclusions, expiry, reset/replay, clock/owner/overflow guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
