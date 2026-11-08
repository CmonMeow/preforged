// Finite-charge transaction foundation. Production792 consumers are separate.
#include "gw_weapon_spell_charges.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <vector>
#include <string>
using namespace fw::gw;using namespace skills::effects;using combat::weapon::AttackOutcome;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("charged weapon spell line "+std::to_string(at.line()));}
struct Fixture{
 Scope caster{1,1,1},recipient{1,2,2},foe{1,100,3},other{1,101,4};ChargedSpellSnapshot state{recipient};OwnedChargedWeaponSpell owned;
 explicit Fixture(unsigned skill=795,unsigned rank=0){
  const auto plan=*charged_weapon_spell(skill,rank,8,0);owned={caster,recipient,{1,caster.actor,skill,rank,Category::weapon_spell,100,100+plan.duration},plan};
  check(state.registry.apply(recipient,owned.entry,Replace::category).accepted);check(state.ledger.attach(owned,state.registry,100));
  state.health={{recipient,{500,1000,100,1}},{foe,{100,1000,100,1}},{other,{300,1000,100,1}}};
 }
 combat::weapon::ResolvedAttack event(std::uint64_t id=1,std::uint64_t at=200,AttackOutcome outcome=AttackOutcome::hit)const{
  return {recipient,foe,id,150,at,outcome,outcome==AttackOutcome::hit?std::optional(at):std::nullopt,combat::weapon::AttackDelivery::ranged};
 }
 ChargedHealthMutation change(Scope scope,double after,std::uint64_t at=200)const{
  const auto before=state.health.at(scope);return {scope,before,{after,before.maximum,at,before.revision+1}};
 }
 OwnedChargedWeaponSpell replace(unsigned effect=2,std::uint64_t at=300){
  auto replacement=owned;replacement.entry.instance=effect;replacement.entry.applied=at;replacement.entry.expires=at+replacement.plan.duration;
  check(replace_charged_weapon_spell(state,replacement,at));return replacement;
 }
};
int main()try{
 static_assert(!std::is_invocable_v<decltype(&WeaponSpellChargeLedger::retire_receipts),WeaponSpellChargeLedger&,const combat::weapon::AttackCompletionFrontier&>);
 const unsigned charges[]={1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,6,6,6,6};
 const unsigned nightmare[]={10,13,15,18,21,23,26,29,31,34,37,39,42,45,47,50,53,55,58,61,63};
 for(unsigned rank=0;rank<=20;++rank){const auto a=charged_weapon_spell(792,rank,8,12),b=charged_weapon_spell(795,rank,8,12);
  check(a&&b&&a->charges==charges[rank]&&a->amount==5+3*rank&&a->duration==30000&&b->charges==3&&b->amount==nightmare[rank]&&b->duration==18000);}
 const unsigned pvp_damage[]={5,7,8,10,12,13,15,17,18,20,22,23,25,27,28,30,32,33,35,37,38};
 for(unsigned rank=0;rank<=20;++rank){const auto p=charged_weapon_spell(2868,rank,8,0);check(p&&p->charges==charges[rank]&&p->amount==pvp_damage[rank]&&p->duration==20000);}
 {Fixture f(792);const auto old=f.owned;check(!f.state.ledger.detach(f.foe,200)&&!f.state.ledger.detach(f.recipient,99));
  check(f.state.ledger.detach(f.recipient,200)&&!f.state.ledger.owned()&&f.state.ledger.remaining()==0);
  check(!f.state.ledger.attach(old,f.state.registry,200));
  auto replacement=old;replacement.entry.instance=2;replacement.entry.applied=200;replacement.entry.expires=20200;
  check(f.state.registry.apply(f.recipient,replacement.entry,Replace::category).accepted&&f.state.ledger.attach(replacement,f.state.registry,200));
  check(f.state.ledger.remaining()==1&&f.state.ledger.owned()->entry.instance==2);}
 check(!charged_weapon_spell(792,21,8,0)&&!charged_weapon_spell(795,0,0,0)&&!charged_weapon_spell(795,0,2,1)&&!charged_weapon_spell(787,0,8,0));
 {Fixture f;check(!f.state.ledger.attach(f.owned,f.state.registry,100));const auto old=f.state.health;
  std::vector<ChargedHealthMutation>changes{f.change(f.recipient,520),f.change(f.foe,80),f.change(f.other,285)};
  auto proposal=ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::consume,changes);
  check(proposal&&f.state.ledger.remaining()==3&&f.state.health==old);check(proposal->commit(f.state,200));
  check(f.state.ledger.remaining()==2&&f.state.health.at(f.recipient).current==520&&f.state.health.at(f.foe).current==80&&f.state.health.at(f.other).current==285);
  check(!proposal->commit(f.state,200)&&!ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::consume,{}));}
 {Fixture f;const auto old=f.state.health;auto a=f.change(f.recipient,520),b=f.change(f.foe,80);++b.before.revision;
  const ChargedHealthMutation changes[]={a,b};check(!ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::consume,changes));check(f.state.health==old&&f.state.ledger.remaining()==3);}
 {Fixture f;const ChargedHealthMutation changes[]={f.change(f.recipient,520),f.change(f.foe,80)};
  auto proposal=ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::consume,changes);check(bool(proposal));
  f.state.health.at(f.other)={290,1000,200,2};check(!proposal->commit(f.state,200));check(f.state.ledger.remaining()==3&&f.state.health.at(f.recipient).current==500&&f.state.health.at(f.other).current==290);}
 {Fixture f;auto proposal=ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::consume,{});check(bool(proposal));
  const auto replacement=f.replace();check(!proposal->commit(f.state,200));auto event=f.event(1,400);
  check(!ChargeProposal::prepare(f.state,f.owned,event,ChargeConsumption::consume,{}));
  auto next=ChargeProposal::prepare(f.state,replacement,event,ChargeConsumption::consume,{});check(next&&next->commit(f.state,400)&&f.state.ledger.remaining()==2);}
 {Fixture f;auto first=ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::consume,{});check(first&&first->commit(f.state,200));
  const auto replacement=f.replace();check(!ChargeProposal::prepare(f.state,replacement,f.event(1,400),ChargeConsumption::consume,{}));check(f.state.ledger.remaining()==3);}
 {Fixture f;for(unsigned n=0;n<3;++n){auto proposal=ChargeProposal::prepare(f.state,f.owned,f.event(n+1,200+n),ChargeConsumption::consume,{});check(proposal&&proposal->commit(f.state,200+n));}
  check(f.state.ledger.remaining()==0&&!f.state.ledger.owned()&&!f.state.registry.lookup(f.recipient,1));check(!ChargeProposal::prepare(f.state,f.owned,f.event(4,210),ChargeConsumption::consume,{}));}
 for(unsigned skill:{792u,795u})for(auto outcome:{AttackOutcome::hit,AttackOutcome::miss,AttackOutcome::block,AttackOutcome::cancelled}){
  // Deliberately test BOTH explicit choices. These are policy plumbing tests,
  // not evidence that a retail skill consumes on any particular failure.
  for(auto choice:{ChargeConsumption::ignore,ChargeConsumption::consume}){Fixture f(skill,12);const auto count=f.state.ledger.remaining();auto p=ChargeProposal::prepare(f.state,f.owned,f.event(1,200,outcome),choice,{});
   check(p&&p->commit(f.state,200)&&f.state.ledger.remaining()==count-unsigned(choice==ChargeConsumption::consume));check(!ChargeProposal::prepare(f.state,f.owned,f.event(1,201,outcome),choice,{}));}}
 {Fixture f;auto p=ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::ignore,{});check(p&&p->commit(f.state,200));
  check(!ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::consume,{}));const ChargedHealthMutation change[]={f.change(f.recipient,520,210)};
  auto independent=ChargeProposal::prepare(f.state,f.owned,f.event(2,210),ChargeConsumption::ignore,change);check(independent&&independent->commit(f.state,210)&&f.state.ledger.remaining()==3&&f.state.health.at(f.recipient).current==520);}
 {Fixture f;auto high=ChargeProposal::prepare(f.state,f.owned,f.event(9,200),ChargeConsumption::consume,{});check(high&&high->commit(f.state,200));
  auto low=ChargeProposal::prepare(f.state,f.owned,f.event(8,210),ChargeConsumption::consume,{});check(low&&low->commit(f.state,210));check(!ChargeProposal::prepare(f.state,f.owned,f.event(10,205),ChargeConsumption::consume,{}));}
 {Fixture f;auto event=f.event(1,12100);check(!ChargeProposal::prepare(f.state,f.owned,event,ChargeConsumption::consume,{}));
   event=f.event(1,12099);check(bool(ChargeProposal::prepare(f.state,f.owned,event,ChargeConsumption::consume,{})));}
 {Fixture f;auto p=ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::consume,{});check(bool(p));check(f.state.ledger.retire(f.recipient));check(!p->commit(f.state,200));check(!f.state.ledger.attach(f.owned,f.state.registry,200));}
 {Fixture f;auto p=ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::consume,{});check(bool(p));f.state.health.at(f.recipient)={0,1000,200,2};check(!p->commit(f.state,200));check(!ChargeProposal::prepare(f.state,f.owned,f.event(2,201),ChargeConsumption::consume,{}));}
 {Fixture f;auto p=ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::consume,{});check(bool(p));f.state.health.erase(f.foe);check(!p->commit(f.state,200));
  auto recycled=f.foe;++recycled.generation;f.state.health.emplace(recycled,ChargedHealthFact{100,1000,200,1});check(!ChargeProposal::prepare(f.state,f.owned,f.event(2,210),ChargeConsumption::consume,{}));}
 {Fixture f;auto p=ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::consume,{});check(bool(p));
  const Entry hex{90,100,100,0,Category::hex,150,1000};check(f.state.registry.apply(f.recipient,hex,Replace::independent).accepted);check(p->commit(f.state,200));check(bool(f.state.registry.lookup(f.recipient,90)));}
 {Fixture f;auto event=f.event();event.attacker.generation++;check(!ChargeProposal::prepare(f.state,f.owned,event,ChargeConsumption::consume,{}));
  auto wrong=f.owned;++wrong.source.generation;check(!ChargeProposal::prepare(f.state,wrong,f.event(),ChargeConsumption::consume,{}));
  // No live-caster Health fact is required: source provenance is retained and
  // departure alone does not end an existing recipient-owned weapon spell.
  check(!f.state.health.contains(f.caster));check(bool(ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::consume,{})));}
 {Fixture f;combat::weapon::AttackOutcomeDelivery producer(f.recipient);
  for(unsigned id=1;id<=4096;++id){check(producer.release(f.recipient,f.foe,id,200,combat::weapon::AttackDelivery::ranged));check(bool(producer.qualify_hit(f.recipient,f.foe,id,200)));
   const auto event=producer.finish(f.recipient,f.foe,id,AttackOutcome::hit,200);check(bool(event));auto proposal=ChargeProposal::prepare(f.state,f.owned,*event,ChargeConsumption::ignore,{});check(proposal&&proposal->commit(f.state,200));}
  check(f.state.ledger.receipts()==4096&&f.state.ledger.remaining()==3);check(!ChargeProposal::prepare(f.state,f.owned,f.event(4097),ChargeConsumption::consume,{}));
  check(producer.release(f.recipient,f.foe,4097,200,combat::weapon::AttackDelivery::ranged));check(!producer.can_finish(f.recipient));
  check(!producer.finish(f.recipient,f.foe,4097,AttackOutcome::cancelled,200)&&producer.size()==1); // Capacity failure did not consume the producer's pending event.
  const auto premature=producer.completion_frontier(f.recipient);const auto produced=producer.producer_frontier(f.recipient);
  check(premature&&produced&&premature->through()==0&&produced->through()==4096);check(f.state.ledger.retire_receipts(*premature)&&f.state.ledger.receipts()==4096);
  const auto buffered=producer.buffered(f.recipient);check(buffered&&buffered->size()==4096);for(const auto&event:*buffered)check(producer.acknowledge(f.recipient,event));
  const auto closed=producer.completion_frontier(f.recipient);check(closed&&closed->through()==4096&&f.state.ledger.retire_receipts(*closed));
  check(f.state.ledger.receipts()==0&&f.state.ledger.sealed_through()==4096);check(!ChargeProposal::prepare(f.state,f.owned,f.event(1),ChargeConsumption::consume,{}));
  check(producer.can_finish(f.recipient));check(bool(producer.qualify_hit(f.recipient,f.foe,4097,200)));
  const auto event=producer.finish(f.recipient,f.foe,4097,AttackOutcome::hit,200);auto next=ChargeProposal::prepare(f.state,f.owned,*event,ChargeConsumption::consume,{});check(next&&next->commit(f.state,200)&&f.state.ledger.remaining()==2);}
 {Fixture f;auto invalid=f.owned;invalid.entry.instance=2;invalid.entry.expires=999;const auto before=f.state.ledger.revision();
  check(!replace_charged_weapon_spell(f.state,invalid,200));check(f.state.ledger.revision()==before&&f.state.registry.lookup(f.recipient,1)&&!f.state.registry.lookup(f.recipient,2));
  check(!replace_charged_weapon_spell(f.state,f.owned,200));check(f.state.ledger.remaining()==3&&f.state.registry.lookup(f.recipient,1));}
 {Fixture f;auto event=f.event();event.delivery=combat::weapon::AttackDelivery::unknown;check(!ChargeProposal::prepare(f.state,f.owned,event,ChargeConsumption::consume,{}));
  event=f.event();event.before_damage=201;check(!ChargeProposal::prepare(f.state,f.owned,event,ChargeConsumption::consume,{}));event.before_damage.reset();check(!ChargeProposal::prepare(f.state,f.owned,event,ChargeConsumption::consume,{}));
  f.state.health.at(f.foe).at=201;check(!ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::consume,{}));}
 {Fixture f;const auto before=f.state.health;auto change=f.change(f.foe,0);const ChargedHealthMutation duplicate[]={change,change};
  check(!ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::consume,duplicate));change.after.current=std::numeric_limits<double>::quiet_NaN();
  check(!ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::consume,std::span(&change,1)));check(f.state.health==before&&f.state.ledger.remaining()==3);}
 {Fixture f;combat::weapon::AttackOutcomeDelivery producer(f.recipient);
  check(producer.release(f.recipient,f.foe,9,150,combat::weapon::AttackDelivery::ranged)&&producer.release(f.recipient,f.foe,10,150,combat::weapon::AttackDelivery::ranged));
  check(bool(producer.qualify_hit(f.recipient,f.foe,10,200)));const auto newer=producer.finish(f.recipient,f.foe,10,AttackOutcome::hit,200);
  auto p=ChargeProposal::prepare(f.state,f.owned,*newer,ChargeConsumption::ignore,{});check(p&&p->commit(f.state,200));check(producer.acknowledge(f.recipient,*newer));const auto held=producer.completion_frontier(f.recipient);
  check(held&&held->through()==8&&f.state.ledger.retire_receipts(*held)&&f.state.ledger.receipts()==1);
  check(bool(producer.qualify_hit(f.recipient,f.foe,9,210)));const auto older=producer.finish(f.recipient,f.foe,9,AttackOutcome::hit,210);
  p=ChargeProposal::prepare(f.state,f.owned,*older,ChargeConsumption::ignore,{});check(p&&p->commit(f.state,210));check(producer.acknowledge(f.recipient,*older));const auto advanced=producer.completion_frontier(f.recipient);
  check(advanced&&advanced->through()==10&&f.state.ledger.retire_receipts(*advanced)&&f.state.ledger.receipts()==0&&!f.state.ledger.retire_receipts(*held));
  check(!ChargeProposal::prepare(f.state,f.owned,*older,ChargeConsumption::consume,{}));}
 {Fixture f;combat::weapon::AttackOutcomeDelivery producer(f.recipient),foreign(f.foe);check(!producer.completion_frontier(f.foe));
  check(!f.state.ledger.retire_receipts(*foreign.completion_frontier(f.foe)));check(producer.release(f.recipient,f.foe,1,150));producer.discard_target(f.foe);
  const auto closed=producer.completion_frontier(f.recipient);check(closed&&closed->through()==1&&f.state.ledger.retire_receipts(*closed));check(!ChargeProposal::prepare(f.state,f.owned,f.event(),ChargeConsumption::consume,{}));}
 {Fixture f;combat::weapon::AttackOutcomeDelivery producer(f.recipient);for(unsigned id=1;id<=10000;++id){
   check(producer.release(f.recipient,f.foe,id,200,combat::weapon::AttackDelivery::melee));check(bool(producer.qualify_hit(f.recipient,f.foe,id,200)));
   const auto event=producer.finish(f.recipient,f.foe,id,AttackOutcome::hit,200);auto proposal=ChargeProposal::prepare(f.state,f.owned,*event,ChargeConsumption::ignore,{});check(proposal&&proposal->commit(f.state,200));
   check(producer.acknowledge(f.recipient,*event));check(f.state.ledger.retire_receipts(*producer.completion_frontier(f.recipient))&&f.state.ledger.receipts()==0);}
  check(f.state.ledger.remaining()==3&&f.state.ledger.sealed_through()==10000);}
 {Fixture f;for(unsigned effect=2;effect<=5000;++effect)f.replace(effect,200);check(f.state.ledger.remaining()==3&&f.state.ledger.owned()->entry.instance==5000);check(!replace_charged_weapon_spell(f.state,f.owned,200));}
 {Fixture f;combat::weapon::AttackOutcomeDelivery producer(f.recipient);check(producer.release(f.recipient,f.foe,1,150,combat::weapon::AttackDelivery::ranged));check(bool(producer.qualify_hit(f.recipient,f.foe,1,200)));
  const auto event=producer.finish(f.recipient,f.foe,1,AttackOutcome::cancelled,200);check(bool(event));auto p=ChargeProposal::prepare(f.state,f.owned,*event,ChargeConsumption::ignore,{});check(p&&p->commit(f.state,200));
  check(producer.producer_frontier(f.recipient)->through()==1&&producer.completion_frontier(f.recipient)->through()==0);
  check(f.state.ledger.retire_receipts(*producer.completion_frontier(f.recipient))&&f.state.ledger.receipts()==1);
  auto forged=*event;++forged.target.generation;check(!producer.acknowledge(f.recipient,forged)&&producer.buffered_size()==1);
  check(producer.acknowledge(f.recipient,*event)&&!producer.acknowledge(f.recipient,*event));check(f.state.ledger.retire_receipts(*producer.completion_frontier(f.recipient))&&f.state.ledger.receipts()==0&&f.state.ledger.remaining()==3);}
 {Fixture f;combat::weapon::AttackOutcomeDelivery producer(f.recipient);check(producer.release(f.recipient,f.foe,1,150,combat::weapon::AttackDelivery::ranged)&&producer.release(f.recipient,f.foe,2,150,combat::weapon::AttackDelivery::ranged));
  check(bool(producer.qualify_hit(f.recipient,f.foe,1,200)));const auto event=producer.finish(f.recipient,f.foe,1,AttackOutcome::hit,200);
  auto p=ChargeProposal::prepare(f.state,f.owned,*event,ChargeConsumption::ignore,{});check(p&&p->commit(f.state,200));check(producer.retire(f.recipient));
  check(producer.producer_frontier(f.recipient)->through()==2&&producer.completion_frontier(f.recipient)->through()==0);check(!producer.release(f.recipient,f.foe,3,210)&&!producer.finish(f.recipient,f.foe,2,AttackOutcome::cancelled,210));
  check(producer.acknowledge(f.recipient,*event));const auto terminal=producer.completion_frontier(f.recipient);check(terminal&&terminal->through()==2&&f.state.ledger.retire_receipts(*terminal));
  auto recycled=f.recipient;++recycled.generation;WeaponSpellChargeLedger replacement(recycled);check(!replacement.retire_receipts(*terminal));check(f.state.ledger.retire(f.recipient)&&f.state.ledger.receipts()==0);}
 for(bool same_time:{false,true}){Fixture f;combat::weapon::AttackOutcomeDelivery producer(f.recipient);
  check(producer.release(f.recipient,f.foe,9,150,combat::weapon::AttackDelivery::ranged)&&producer.release(f.recipient,f.foe,10,150,combat::weapon::AttackDelivery::ranged));
  check(bool(producer.qualify_hit(f.recipient,f.foe,10,200)));check(bool(producer.finish(f.recipient,f.foe,10,AttackOutcome::hit,200)));
  const auto later=same_time?200u:210u;check(bool(producer.qualify_hit(f.recipient,f.foe,9,later)));check(bool(producer.finish(f.recipient,f.foe,9,AttackOutcome::hit,later)));
  const auto buffered=producer.buffered(f.recipient);check(buffered&&buffered->size()==2&&buffered->at(0).event==10&&buffered->at(1).event==9);
  for(const auto&event:*buffered){auto proposal=ChargeProposal::prepare(f.state,f.owned,event,ChargeConsumption::ignore,{});check(proposal&&proposal->commit(f.state,event.at));check(producer.acknowledge(f.recipient,event));}
  check(f.state.ledger.retire_receipts(*producer.completion_frontier(f.recipient))&&f.state.ledger.receipts()==0);}
 {Fixture f;combat::weapon::AttackOutcomeDelivery producer(f.recipient);check(producer.release(f.recipient,f.foe,1,150,combat::weapon::AttackDelivery::ranged));
  check(!producer.finish(f.recipient,f.other,1,AttackOutcome::hit,200)&&producer.buffered_size()==0&&producer.size()==1);
  check(bool(producer.qualify_hit(f.recipient,f.foe,1,200)));check(bool(producer.finish(f.recipient,f.foe,1,AttackOutcome::hit,200))&&producer.buffered_size()==1);}
 std::cout<<"detached charged weapon spell transactions passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
