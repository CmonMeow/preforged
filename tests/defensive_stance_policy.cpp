#include "gw_defensive_stance.h"
#include "gw_adrenaline.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Defensive Stance policy line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=defensive_stance(rank);check(p&&p->duration==1000ull*interpolate(1,5,rank)&&p->maximum_strikes==interpolate(0,4,rank));}
 check(!defensive_stance(21));
 Slots bar{{{322,0},{325,0},{340,0},{331,0},{383,0},{399,0},{1604,0},{281,0}}};
 check(defensive_melee_skills(bar)==5);
 auto copied=bar;copied[7]={340,1};check(defensive_melee_skills(copied)==6);
 copied[7]={340,0};check(!defensive_melee_skills(copied));copied[7]={0,1};check(!defensive_melee_skills(copied));copied[7]={999999,0};check(!defensive_melee_skills(copied));
 const Scope owner{1,25,1};auto stale=owner;++stale.generation;
 for(unsigned rank=0;rank<=20;++rank){
  Registry registry(owner);const auto p=*defensive_stance(rank);const Entry e{123,25,345,rank,Category::stance,100,100+p.duration};
  check(registry.apply(owner,e,Replace::category).accepted);CooldownBarAuthority authority(owner);check(authority.publish(owner,bar,100).has_value());
  check(!prepare_defensive_end(registry,owner,123,*e.expires-1,DefensiveEnd::expiration,authority.snapshot(owner),true));
  for(auto reason:{DefensiveEnd::expiration,DefensiveEnd::replacement,DefensiveEnd::removal,DefensiveEnd::death,DefensiveEnd::retirement}){
   const auto at=reason==DefensiveEnd::expiration?*e.expires:200;
   const auto proposal=prepare_defensive_end(registry,owner,123,at,reason,authority.snapshot(owner),true);check(proposal.has_value());
   check(proposal->strikes==unsigned(reason==DefensiveEnd::death||reason==DefensiveEnd::retirement?0:std::min(5u,p.maximum_strikes)));
   check(registry.lookup(owner,123).has_value()&&!proposal->registry.lookup(owner,123));
   check(!prepare_defensive_end(proposal->registry,owner,123,at,reason,authority.snapshot(owner),true));
  }
  Registry forged(owner);auto bad=e;++*bad.expires;check(forged.apply(owner,bad,Replace::category).accepted);
  check(!prepare_defensive_end(forged,owner,123,*bad.expires,DefensiveEnd::expiration,authority.snapshot(owner),true));
  const auto delayed=prepare_defensive_end(registry,owner,123,*e.expires+1000,DefensiveEnd::expiration,authority.snapshot(owner),true);check(delayed&&delayed->at==*e.expires);
  check(!prepare_defensive_end(registry,stale,123,*e.expires,DefensiveEnd::expiration,authority.snapshot(owner),true));
  check(!prepare_defensive_end(registry,owner,123,*e.expires,DefensiveEnd::expiration,{},true));
  check(prepare_defensive_end(registry,owner,123,*e.expires,DefensiveEnd::death,{},false)->strikes==0);
  check(authority.publish(owner,bar,*e.expires+1).has_value());
  check(!prepare_defensive_end(registry,owner,123,*e.expires+100,DefensiveEnd::expiration,authority.snapshot(owner),true));
 }
 // Retain the last authoritative bar strictly before expiry; no full history
 // and no substitution of a later bar when a tick observes the ending late.
 {CooldownBarAuthority authority(owner);check(authority.publish(owner,bar,100).has_value());
  check(!authority.observe_bar_until(stale,123,100,1000));
  check(!authority.observe_bar_until(owner,0,100,1000));
  check(!authority.observe_bar_until(owner,123,99,1000));
  check(authority.observe_bar_until(owner,123,100,1000));
  check(!authority.observe_bar_until(owner,124,100,1000));
  Slots fewer{};fewer[0]={340,0};
  check(authority.publish(owner,fewer,500).has_value());
  check(authority.observed_bar(owner,123,1000)->bar==fewer);
  auto speculative=authority;check(speculative.publish(owner,bar,900).has_value());
  check(speculative.observed_bar(owner,123,1000)->bar==bar);
  check(authority.observed_bar(owner,123,1000)->bar==fewer);
  check(authority.publish(owner,bar,999).has_value());
  check(authority.publish(owner,fewer,1000).has_value());
  check(authority.publish(owner,Slots{},1500).has_value());
  const auto observed=authority.observed_bar(owner,123,1000);check(observed&&observed->bar==bar);
  check(!authority.observed_bar(stale,123,1000)&&!authority.observed_bar(owner,124,1000)&&!authority.observed_bar(owner,123,1001));
  Registry registry(owner);check(registry.apply(owner,{123,25,345,0,Category::stance,0,1000},Replace::category).accepted);
  check(prepare_defensive_end(registry,owner,123,1500,DefensiveEnd::expiration,observed,true).has_value());
  check(!prepare_defensive_end(registry,owner,123,1500,DefensiveEnd::expiration,authority.snapshot(owner),true));
  check(!authority.release_bar_observation(stale,123)&&!authority.release_bar_observation(owner,124));
  check(authority.release_bar_observation(owner,123));check(!authority.release_bar_observation(owner,123));
  check(!authority.observed_bar(owner,123,1000));
  check(authority.observe_bar_until(owner,124,1500,2000));
 }
 for(bool forbidden:{false,true}){
  Registry registry(owner);check(registry.apply(owner,{123,25,345,15,Category::stance,100,5100},Replace::category).accepted);
  if(forbidden)check(registry.apply(owner,{124,99,56,0,Category::hex,100,6100},Replace::same_skill).accepted);
  Slots bar{};bar[0]={345,0};bar[1]={340,0};bar[2]={331,0};bar[3]={383,0};
  CooldownBarAuthority authority(owner);check(authority.publish(owner,bar,100).has_value());check(authority.observe_bar_until(owner,123,100,5100));
  combat::adrenaline::State pools;check(pools.configure(bar,100));
  check(pools.disable({331,0},6000,100));
  const auto reward=prepare_defensive_reward(registry,pools,authority,owner,123,6000,DefensiveEnd::expiration,true);
  check(reward&&reward->at==5100&&reward->strikes==3&&reward->forbidden==forbidden);
  for(const auto& pool:reward->adrenaline.pools())check(pool.units==unsigned(forbidden||pool.slot.skill==331?0:75));
  check(registry.lookup(owner,123).has_value()&&authority.observed_bar(owner,123,5100).has_value());
  check(!reward->registry.lookup(owner,123)&&!reward->authority.observed_bar(owner,123,5100));
  check(!prepare_defensive_reward(reward->registry,reward->adrenaline,reward->authority,owner,123,6000,DefensiveEnd::expiration,true));
  auto newer=pools;newer.tick(5200);check(!prepare_defensive_reward(registry,newer,authority,owner,123,6000,DefensiveEnd::expiration,true));
  auto edited=authority;check(edited.publish(owner,Slots{},5100).has_value());check(!prepare_defensive_reward(registry,pools,edited,owner,123,6000,DefensiveEnd::expiration,true));
  const auto dead=prepare_defensive_reward(registry,pools,edited,owner,123,6000,DefensiveEnd::death,false);check(dead&&dead->strikes==0);
 }
 // The eventual World commit uses existing capped/disabled pool semantics.
 combat::adrenaline::State pools;Slots charges{};charges[0]={345,0};charges[1]={340,0};check(pools.configure(charges,100));
 check(pools.disable({340,0},2000,100));check(pools.gain(25*6,200));check(pools.pools()[0].units==120&&pools.pools()[1].units==0);
 std::cout<<"PASS Defensive Stance native policy,melee bar classification,rank caps,atomic-copy ending proposals,exactly-once removal,death/stale/future-bar rejection and capped/disabled adrenaline pools\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
