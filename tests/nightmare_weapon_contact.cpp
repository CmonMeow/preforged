#include "gw_nightmare_weapon_contact.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Nightmare contact line "+std::to_string(at.line()));
}
struct Fixture {
 Scope caster{1,1,1},recipient{1,2,2},foe{1,3,3};ChargedSpellSnapshot state{recipient};OwnedChargedWeaponSpell owned;
 Fixture(){const auto plan=*charged_weapon_spell(795,12,8,0);owned={caster,recipient,{1,caster.actor,795,12,Category::weapon_spell,100,12100},plan};
  check(state.registry.apply(recipient,owned.entry,Replace::category).accepted);check(state.ledger.attach(owned,state.registry,100));
  state.health={{recipient,{500,1000,100,1}},{foe,{100,1000,100,1}}};}
 combat::weapon::ResolvedAttack event(unsigned id=1,combat::weapon::AttackOutcome outcome=combat::weapon::AttackOutcome::hit)const{
  return {recipient,foe,id,150,200+id,outcome,outcome==combat::weapon::AttackOutcome::hit?std::optional<std::uint64_t>(200+id):std::nullopt,combat::weapon::AttackDelivery::ranged};}
};
int main()try{
 {Fixture f;const auto before=f.state.health;auto p=prepare_nightmare_contact(f.state,f.owned,f.event());
  check(p&&p->transfer&&p->transfer->stolen==42&&p->transfer->gained==42&&f.state.health==before&&f.state.ledger.remaining()==3);
  check(p->charge.commit(f.state,201));check(f.state.health.at(f.recipient).current==542&&f.state.health.at(f.foe).current==58&&f.state.ledger.remaining()==2);
  check(!prepare_nightmare_contact(f.state,f.owned,f.event())&&!p->charge.commit(f.state,201));}
 {Fixture f;f.state.health.at(f.recipient).current=990;f.state.health.at(f.foe).current=7;
  auto p=prepare_nightmare_contact(f.state,f.owned,f.event());check(p&&p->transfer->stolen==8&&p->transfer->gained==8&&p->transfer->killed);
  check(p->charge.commit(f.state,201)&&f.state.health.at(f.recipient).current==998&&f.state.health.at(f.foe).current==0);}
 {Fixture f;f.state.health.at(f.recipient).current=1000;auto p=prepare_nightmare_contact(f.state,f.owned,f.event(),50);
  check(p&&p->transfer->stolen==0&&p->transfer->gained==0&&p->charge.commit(f.state,201)&&f.state.ledger.remaining()==2);}
 for(auto failure:{combat::weapon::AttackOutcome::miss,combat::weapon::AttackOutcome::block,combat::weapon::AttackOutcome::cancelled}){
  Fixture f;const auto before=f.state.health;auto p=prepare_nightmare_contact(f.state,f.owned,f.event(1,failure));
  check(p&&!p->transfer&&p->charge.commit(f.state,201)&&f.state.ledger.remaining()==3&&f.state.health==before);}
 {Fixture f;f.state.health.at(f.foe).current=1000;for(unsigned i=1;i<=3;++i){auto p=prepare_nightmare_contact(f.state,f.owned,f.event(i));check(p&&p->charge.commit(f.state,200+i));}
  check(!f.state.registry.lookup(f.recipient,1)&&!f.state.ledger.remaining()&&!f.state.ledger.owned());}
 {Fixture f;auto p=prepare_nightmare_contact(f.state,f.owned,f.event());check(bool(p));++f.state.health.at(f.foe).revision;
  check(!p->charge.commit(f.state,201)&&f.state.health.at(f.recipient).current==500&&f.state.ledger.remaining()==3);}
 {Fixture f;auto p=prepare_nightmare_contact(f.state,f.owned,f.event());check(bool(p));auto replacement=f.owned;replacement.entry.instance=2;replacement.entry.applied=200;replacement.entry.expires=12200;
  check(replace_charged_weapon_spell(f.state,replacement,200));check(!p->charge.commit(f.state,201)&&f.state.ledger.remaining()==3);}
 {Fixture f;check(nightmare_subtract_at_selected_phase(f.owned.plan,60)==18&&nightmare_subtract_at_selected_phase(f.owned.plan,10)==0);
  check(!nightmare_subtract_at_selected_phase(f.owned.plan,-1));auto forged=f.owned;++forged.plan.amount;
  check(!prepare_nightmare_contact(f.state,forged,f.event())&&!nightmare_subtract_at_selected_phase(forged.plan,60));}
 {Fixture f;f.state.health.at(f.foe).maximum=1000.5;check(!prepare_nightmare_contact(f.state,f.owned,f.event()));}
 check(!weapon_spell(795,12,8,0));
 std::cout<<"Detached Nightmare paired transfer, hit charges and atomic rejection passed; World phase unbound\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
