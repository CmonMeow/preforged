#include "gw_npc_attack_interrupt.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace combat;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("NPC attack interrupt line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned attack:{340u,399u})for(unsigned spell:{194u,220u}){
  npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=npcs::Relation::hostile_to_players;d.spawn_health=Health{500,500};d.damageable=true;d.conditions_allowed=true;
  d.spawn_energy=npcs::Energy{50,50};d.energy_pips=0;
  npcs::LiveActor npc(d,{1,1,1,146,100});const auto life=npc.lifetime();const auto owner=npc.hex_scope();
  const skills::Slot selected{static_cast<std::uint16_t>(spell),1},other{static_cast<std::uint16_t>(spell),0};
  check(npc.configure_spellbook(life,{{selected,1248,"authored reviewed spell"},{other,1248,"independent copy"}}));
  ai::SpellTarget target{{1,2,1},100,true,true,true,true,true,true,false};
  const auto token=*npc.begin_spell(life,selected,target,1000);const auto paid=npc.energy(life)->current;
  const auto plan=*weapon::interrupt_disable_plan(attack);weapon::AttackInterruptDisables ledger(owner);
  const auto proposed=npcs::prepare_attack_interrupt(npc,life,token,ledger,plan,1200);check(proposed.has_value());
  check(npc.active_action(life,1200)==token&&ledger.disabled_until(owner,selected,1200)==0);
  const auto base=1200+std::uint64_t(skills::find_skill(spell)->recharge)*1000;
  check(proposed->receipt.recharge_until==base&&proposed->receipt.slot==selected);
  check(proposed->actor.spell_recharge_until(life,selected,1200)==base);
  check(proposed->actor.energy(life)->current==paid); // No refund or second payment.
  check(proposed->disables.disabled_until(owner,selected,1200)==base+20000);
  check(proposed->disables.disabled_until(owner,other,1200)==0);
  auto forged=token;++forged.sequence;check(!npcs::prepare_attack_interrupt(npc,life,forged,ledger,plan,1200));
  auto wrong_plan=plan;--wrong_plan.extra;check(!npcs::prepare_attack_interrupt(npc,life,token,ledger,wrong_plan,1200));
  auto stale=life;++stale.actor_generation;check(!npcs::prepare_attack_interrupt(npc,stale,token,ledger,plan,1200));
  check(!npcs::prepare_attack_interrupt(npc,life,token,ledger,plan,token.completes));
  npc=proposed->actor;ledger=proposed->disables;
  check(!npc.complete_action(life,token,token.completes));check(!npcs::prepare_attack_interrupt(npc,life,token,ledger,plan,1200));
  const ai::Spellbook::Availability available=[&](skills::Slot slot,auto now){return ledger.disabled_until(owner,slot,now);};
  check(npc.choose_spell(life,target,base,available)==other);
  check(npc.choose_spell(life,target,base+19999,available)==other);
  check(npc.choose_spell(life,target,base+20000,available)==selected);
  npcs::LiveActor manual(d,{1,1,2,146,100});const auto own=manual.lifetime();
  check(manual.configure_spellbook(own,{{selected,1248,"reviewed but not admitted"}}));
  const auto untracked=*manual.begin_action(own,npcs::control::Action::spell,1000,2000,selected);
  check(!npcs::prepare_attack_interrupt(manual,own,untracked,weapon::AttackInterruptDisables(manual.hex_scope()),plan,1200));
 }
 std::cout<<"PASS NPC exact reviewed action,zero/nonzero actual recharge,independent copy/disable,AI availability,expiry,no refund,replay/future/stale/untracked rejection;World/contact integration pending\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
