#include "gw_ending_hex_context.h"
#include "gw_player_hex_end.h"
#include "gw_player_damage_projection.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool v){if(!v)throw std::runtime_error("ending hex context");}
int main()try{
 const Scope owner{1,2,3};Registry registry(owner);
 check(registry.apply(owner,{7,1,205,0,Category::hex,100,3100},Replace::same_skill).accepted);
 check(!EndingHexContext::capture(registry,owner,7,3099));
 check(!EndingHexContext::capture(registry,Scope{1,2,4},7,3100));
 const auto context=EndingHexContext::capture(registry,owner,7,3100);check(bool(context));
 registry.remove(owner,7);
 check(context->qualifies(owner,3100)&&!context->qualifies(owner,3101)&&!context->qualifies(Scope{1,2,4},3100));
 check(!EndingHexContext::capture(registry,owner,7,3100));
 check(registry.apply(owner,{8,1,258,0,Category::enchantment,100,3100},Replace::same_skill).accepted);
 check(!EndingHexContext::capture(registry,owner,8,3100));
 using namespace fw::gw;
 for(unsigned skill:{44u,2054u})for(unsigned rank=0;rank<=20;++rank)for(double hp:{0.,50.,500.}){
  Registry current(owner);const Entry ending{20,1,skill,rank,Category::hex,100,10100};
  check(current.apply(owner,ending,Replace::same_skill).accepted);
  check(current.apply(owner,{21,4,1399,12,Category::enchantment,100,6100},Replace::same_skill).accepted);
  check(current.apply(owner,{22,4,19,12,Category::hex,100,10100},Replace::same_skill).accepted);
  TemporaryHealthSnapshot health{owner,100,hp,{}};health.capacity.reset(1000);
  combat::conditions::State conditions;
  const auto lease=capture_player_hex_removal(current,owner,20,100);check(bool(lease));
  const auto view=prepare_player_hex_end(current,*lease,100,conditions,health);check(bool(view));
  check(!view->registry.lookup(owner,20)&&view->registry.lookup(owner,21)&&view->registry.lookup(owner,22));
  check(current.lookup(owner,20)&&health.current==hp&&conditions.count(100)==0);
  check(view->world_end_required);
  if(!hp){check(view->health.current==0&&view->transitions.empty()&&view->fragility.empty());continue;}
  const auto kind=skill==44?combat::conditions::Kind::deep_wound:combat::conditions::Kind::cracked_armor;
  check(view->conditions.has(kind,100)&&view->transitions.size()==1&&view->fragility.size()==1);
  check(view->fragility.front().transition.kind==kind&&view->fragility.front().cause.instance==22);
  if(skill==44){check(view->health.capacity.maximum()==900);
   check(view->health.current-view->health.capacity.debt==hp-100);}
  else check(view->health.current==hp&&view->health.capacity.maximum()==1000);
 }
 {
  Registry current(owner);check(current.apply(owner,{30,1,44,12,Category::hex,100,10100},Replace::same_skill).accepted);
  combat::conditions::State conditions;check(conditions.apply(combat::conditions::Kind::deep_wound,9,100,50000));
  TemporaryHealthSnapshot health{owner,100,500,{}};health.capacity.reset(1000);health.capacity.capacity(health.current,1000,true);
  const auto lease=*capture_player_hex_removal(current,owner,30,100);
  const auto view=prepare_player_hex_end(current,lease,100,conditions,health);check(bool(view));
  check(view->conditions.lookup(combat::conditions::Kind::deep_wound)->cause==9);
  check(view->conditions.lookup(combat::conditions::Kind::deep_wound)->expires==50100);
  check(view->health.current==health.current&&view->transitions.empty()&&view->fragility.empty());
  check(current.apply(owner,{30,1,44,12,Category::hex,100,10100},Replace::same_skill).accepted);
  check(!prepare_player_hex_end(current,lease,100,conditions,health));
  auto fresh=*capture_player_hex_removal(current,owner,30,100);++fresh.entry.rank;
  check(!prepare_player_hex_end(current,fresh,100,conditions,health));
  auto stale=lease;++stale.target.generation;check(!prepare_player_hex_end(current,stale,100,conditions,health));
  check(!capture_player_hex_removal(current,owner,30,10100));
 }
 {
  Registry current(owner);check(current.apply(owner,{40,1,44,12,Category::hex,100,10100},Replace::same_skill).accepted);
  TemporaryHealth ledger(owner,500,100);TemporaryHealthSnapshot health{owner,100,250,{}};health.capacity.reset(500);
  const auto gain=prepare_endure_pain_health(ledger,health,*endure_pain(12),41,false);check(bool(gain));
  const auto lease=*capture_player_hex_removal(current,owner,40,100);combat::conditions::State conditions;
  const auto view=prepare_player_hex_end(current,lease,100,conditions,gain->health,gain->ledger);check(bool(view));
  check(view->temporary&&view->temporary->capacity()==gain->ledger.capacity());
  check(view->health.capacity.maximum()==gain->ledger.capacity()-100);
  check(view->health.current-view->health.capacity.debt==gain->health.current-100);
  auto wrong=gain->health;++wrong.at;check(!prepare_player_hex_end(current,lease,100,conditions,wrong,gain->ledger));
 }
 {
  Registry current(owner);check(current.apply(owner,{50,1,31,12,Category::hex,100,10100},Replace::same_skill).accepted);
  check(current.apply(owner,{51,1,1399,12,Category::enchantment,100,6100},Replace::same_skill).accepted);
  TemporaryHealthSnapshot health{owner,100,500,{}};health.capacity.reset(1000);combat::conditions::State conditions;
  const auto lease=*capture_player_hex_removal(current,owner,50,100);
  const auto view=prepare_player_hex_end(current,lease,100,conditions,health);check(bool(view));
  check(view->health.current==500&&view->health.capacity.maximum()==1000&&view->transitions.empty()&&view->fragility.empty()&&view->world_end_required);
  check(!capture_player_hex_removal(current,owner,51,100));
  check(!capture_player_hex_removal(current,owner,50,99));
  auto wrong=health;wrong.capacity.active=true;check(!prepare_player_hex_end(current,lease,100,conditions,wrong));
  wrong=health;wrong.capacity.debt=-1;check(!prepare_player_hex_end(current,lease,100,conditions,wrong));
  wrong=health;wrong.current=1001;check(!prepare_player_hex_end(current,lease,100,conditions,wrong));
  wrong=health;++wrong.owner.generation;check(!prepare_player_hex_end(current,lease,100,conditions,wrong));
  check(current.lookup(owner,50)&&current.lookup(owner,51)&&health.current==500&&conditions.count(100)==0);
 }
 // Fragility's first contact advances1399; the skill's next contact observes
 // the next reduction.307 converts that first contact only, before real loss.
 for(unsigned ending:{44u,2054u})for(bool shield:{false,true})for(bool reversal:{false,true}){
  Registry current(owner);check(current.apply(owner,{60,1,ending,12,Category::hex,100,10100},Replace::same_skill).accepted);
  check(current.apply(owner,{61,4,19,12,Category::hex,100,10100},Replace::same_skill).accepted);
  if(shield)check(current.apply(owner,{62,4,1399,12,Category::enchantment,100,6100},Replace::same_skill).accepted);
  if(reversal)check(current.apply(owner,{63,4,307,12,Category::enchantment,100,8100},Replace::same_skill).accepted);
  TemporaryHealthSnapshot health{owner,100,500,{}};health.capacity.reset(1000);combat::conditions::State conditions;
  const auto lease=*capture_player_hex_removal(current,owner,60,100);
  const auto view=prepare_player_hex_end(current,lease,100,conditions,health);check(view&&view->fragility.size()==1);
  PlayerDamageAdmission state{view->registry,view->health,ShieldAbsorption(owner)};
  const auto quoted=[&](const PlayerDamageAdmission& authority,unsigned skill,double amount){
   combat::Actor actor{owner.actor,owner.generation,combat::Health{authority.health.current,authority.health.capacity.maximum()},{},0,true};
   if(authority.registry.newest_skill(owner,1399,100)){
    const auto quote=authority.absorption.prepare(authority.registry,owner,authority.absorption.last_event()+1,100);check(bool(quote));
    actor.incoming_reduction=quote->reduction;actor.cumulative_reduction=ShieldAbsorption::receipt(*quote);
   }
   return combat::apply(actor,{combat::Kind::armor_ignoring,amount,20,skill},true);
  };
  const auto amount=double(view->fragility.front().plan.damage)-(shield?5:0);
  const auto first=project_player_protected_hit(state,quoted(state,19,view->fragility.front().plan.damage),100);check(bool(first));
  double expected=view->health.current;
  auto capacity=view->health.capacity;
  if(reversal)capacity.event(expected,amount,true);
  else capacity.event(expected,-amount);
  check(first->state.health.current==expected&&bool(first->reversal)==reversal);
  check(!first->state.registry.newest_skill(owner,307,100));
  const auto second=project_player_protected_hit(first->state,quoted(first->state,27,50),100);check(bool(second));
  capacity.event(expected,-(50-(shield?10:0)));
  check(second->state.health.current==expected&&!second->reversal);
  check(state.health.current==view->health.current&&current.lookup(owner,60));
  if(shield)check(state.absorption.last_event()==0&&first->state.absorption.last_event()==1&&second->state.absorption.last_event()==2);
  auto stale=quoted(first->state,27,50);stale.before.current+=1;check(!project_player_protected_hit(first->state,stale,100));
  if(shield){auto forged=quoted(first->state,27,50);++forged.cumulative_reduction->prior_hits;check(!project_player_protected_hit(first->state,forged,100));}
 }
 {
  const auto quote=[&](const PlayerDamageAdmission& state,double amount){
   combat::Actor actor{owner.actor,owner.generation,combat::Health{state.health.current,state.health.capacity.maximum()},{},0,true};
   if(state.registry.newest_skill(owner,1399,100)){
    const auto protection=state.absorption.prepare(state.registry,owner,state.absorption.last_event()+1,100);check(bool(protection));
    actor.incoming_reduction=protection->reduction;actor.cumulative_reduction=ShieldAbsorption::receipt(*protection);
   }
   return combat::apply(actor,{combat::Kind::armor_ignoring,amount,20,19},true);
  };
  const auto initial=[&](double hp){Registry registry(owner);TemporaryHealthSnapshot health{owner,100,hp,{}};health.capacity.reset(1000);
   return PlayerDamageAdmission{registry,health,ShieldAbsorption(owner)};};
  auto state=initial(500);
  check(state.registry.apply(owner,{70,1,1399,12,Category::enchantment,100,6100},Replace::same_skill).accepted);
  check(state.registry.apply(owner,{71,1,307,0,Category::enchantment,100,8100},Replace::same_skill).accepted);
  const auto zero_hit=quote(state,3);check(zero_hit.damage==0);
  const auto zero=project_player_protected_hit(state,zero_hit,100);check(zero&&zero->actual&&zero->actual->damage==0&&!zero->reversal);
  check(zero->state.registry.newest_skill(owner,307,100)&&zero->state.absorption.last_event()==1);
  check(!project_player_protected_hit(zero->state,zero_hit,100)); // Same event cannot replay at unchanged Health.
  const auto partial=project_player_protected_hit(zero->state,quote(zero->state,100),100);check(partial&&partial->reversal);
  check(partial->state.health.current==440&&partial->actual->damage==75&&partial->state.absorption.last_event()==2);
  auto renewal=partial->state;
  check(renewal.registry.apply(owner,{70,1,1399,12,Category::enchantment,100,6100},Replace::same_skill).accepted);
  const auto reset_hit=quote(renewal,20);check(reset_hit.damage==15);
  const auto reset=project_player_protected_hit(renewal,reset_hit,100);check(reset&&reset->state.health.current==425);
  for(bool wounded:{false,true}){
   auto reduced=initial(500);
   if(wounded)reduced.health.capacity.capacity(reduced.health.current,1000,true);
   check(reduced.registry.apply(owner,{72,1,129,12,Category::hex,100,10100},Replace::same_skill).accepted);
   check(reduced.registry.apply(owner,{73,1,307,0,Category::enchantment,100,8100},Replace::same_skill).accepted);
   const auto result=project_player_protected_hit(reduced,quote(reduced,50),100);check(result&&result->reversal);
   check(std::abs(result->state.health.current-(wounded?374.:474.9))<1e-9);
  }
  auto lethal=initial(10);check(lethal.registry.apply(owner,{74,1,307,12,Category::enchantment,100,8100},Replace::same_skill).accepted);
  const auto raw=quote(lethal,50);check(raw.killed);
  const auto saved=project_player_protected_hit(lethal,raw,100);check(saved&&saved->state.health.current==60&&saved->actual&&!saved->actual->killed&&saved->actual->damage==0);
  auto ordinary=initial(10);const auto killed=project_player_protected_hit(ordinary,quote(ordinary,50),100);
  check(killed&&killed->actual&&killed->actual->applied==10&&killed->actual->killed&&killed->state.health.current==0);
  auto debt=initial(50);debt.health.capacity.capacity(debt.health.current,1000,true);check(debt.health.current==1&&debt.health.capacity.debt==51);
  check(debt.registry.apply(owner,{75,1,307,0,Category::enchantment,100,8100},Replace::same_skill).accepted);
  const auto conversion_death=project_player_protected_hit(debt,quote(debt,50),100);
  check(conversion_death&&conversion_death->reversal&&!conversion_death->actual&&conversion_death->state.health.current==0&&conversion_death->state.health.capacity.debt==0);
  auto fractional=initial(50);fractional.health.capacity.capacity(fractional.health.current,1000,true);
  const auto settled=project_player_protected_hit(fractional,quote(fractional,.5),100);
  // The reviewed damage receipt remains packet-valid; latent capacity debt
  // belongs to the separate settled Health outcome, not invented damage.
  check(settled&&settled->state.health.current==0&&settled->actual&&!settled->actual->killed);
  check(settled->actual->before.current==1&&settled->actual->after.current==.5&&
        settled->actual->damage==.5&&settled->actual->applied==.5);
  check((settled->actual->effects&0x10u)==0);
  check(!fw::gw::combat::damage_packets_2022(1,*settled->actual).empty());
  auto invalid=initial(500);check(invalid.registry.apply(owner,{76,1,1399,12,Category::enchantment,100,6100},Replace::same_skill).accepted);
  check(invalid.registry.apply(owner,{77,1,307,0,Category::enchantment,100,{}},Replace::same_skill).accepted);
  check(!project_player_protected_hit(invalid,quote(invalid,50),100));
  check(invalid.health.current==500&&invalid.absorption.last_event()==0&&invalid.registry.lookup(owner,77));
 }
 std::cout<<"ending hex boundary context passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
