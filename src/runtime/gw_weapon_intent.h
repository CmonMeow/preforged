#pragma once
#include "gw_equipment_metadata_provider.h"
#include "gw_effect_registry.h"
#include "gw_weapon_passives.h"
#include "gw_weapon_element.h"
#include "gw_weapon_critical.h"
#include "gw_arthur_weapon_base.h"
#include <limits>
#include <cmath>
namespace fw::gw::combat::weapon {
using Scope=skills::effects::Scope;
struct Policy {unsigned primary=0;std::string character;attributes::Ranks effective{};bool caster_weapon=false;std::optional<unsigned> mastery_attribute;};
struct Owned {std::uint32_t item=0;std::uint64_t revision=0;items::stats::WeaponDamage damage;unsigned mastery=0;bool customized=false,caster_weapon=false;unsigned item_type=26;std::optional<double> critical_penalty;};
inline std::optional<Owned> owned_weapon(const inventory::Profile& profile,std::uint64_t revision,const Policy& policy){
 if(policy.primary<1||policy.primary>10||(!policy.caster_weapon&&!policy.mastery_attribute)||
  (policy.mastery_attribute&&(*policy.mastery_attribute>=51||attributes::catalog[*policy.mastery_attribute].profession>10)))return {};
 const inventory::Item* equipped=nullptr;
 for(const auto&[id,item]:profile.items){const auto bag=profile.bags.find(item.bag);
  if(bag==profile.bags.end()||bag->second.type!=2||bag->second.index!=21||item.slot!=0)continue;
  if(equipped)return {};equipped=&item;
 }
 const auto type=equipped?equipped->definition.type:0;const bool melee=type==2||type==15||type==27||type==32,bow=type==5;
 if(!melee&&!bow&&policy.caster_weapon&&type!=26&&type!=22)return {};
 if(!equipped||!weapon_sets::valid_type(equipped->definition.type,weapon_sets::Hand::weapon))return {};
 const auto metadata=equipment::metadata::reviewed(profile,equipped->definition);
 if(!metadata||(metadata->profession&&metadata->profession!=policy.primary)||
  (!metadata->customized_character.empty()&&metadata->customized_character!=policy.character))return {};
 // Four exact complete Arthur definitions have independently reviewed base
 // ranges/types/requirements. Preserve all raw item words; this bounded path
 // grants base attacks only. Axe Health5 and Bow/Hammer Charr1 are not applied
 // here. Rinblade's Charr armor is explicitly nonfunctional in the wiki.
 const auto arthur=items::arthur_weapon::reviewed(equipped->definition);
 // Conditional/raw damage modifiers have no reviewed execution policy here.
 // Deny the strike rather than silently treating an upgraded item as vanilla.
 // Captured starter customization120% is already applied through the owned
 // customized-character multiplier; do not apply its modifier a second time.
 std::optional<std::uint32_t> captured_opaque;
 if(bow&&!arthur){
  if(equipped->definition.file!=9380||equipped->definition.model!=477||equipped->definition.modifiers!=
     std::vector<std::uint32_t>{0x26180300,0x24b80100,0xa4880503,0xa4987800})return {};
  captured_opaque=0x26180300;
 }
 if(type==22){
  const auto file=equipped->definition.file,model=equipped->definition.model;
  std::uint32_t damage_word=0,opaque=0;
  if((file==94937&&model==2742)||(file==94904&&model==2787)){damage_word=0x24b80500;opaque=0x26980004;}
  else if(file==93541&&model==2652){damage_word=0x24b80600;opaque=0x26980000;}
  else if(file==94936&&model==2694){damage_word=0x24b80700;opaque=0x26980002;}
  // Captures and each starter's wiki give3..5 damage/+20%customized. Allow
  // the opaque2698 word only in its exact complete baseline; do not infer
  // behavior for arbitrary upgrades sharing its prefix.
  if(damage_word&&equipped->definition.modifiers==std::vector<std::uint32_t>{damage_word,opaque,0xa4880503,0xa4987800})captured_opaque=opaque;
 }
 // The same reviewed Enchanting suffix used by spell duration must not
 // disable base attacks. Validate the complete property namespace first;
 // an arbitrary22B word or duplicate never acquires attack permission.
 if(!items::weapon_passives::enchanting_percent(equipped->definition.modifiers))return {};
 if(!arthur)for(auto raw:equipped->definition.modifiers){const auto id=raw>>16;if(id!=0xa7a8&&id!=0x2798&&id!=0x24b8&&raw!=0xa4880302&&raw!=0xa4880503&&raw!=0xa4987800&&captured_opaque!=raw&&!items::weapon_passives::naming_word(raw)&&(raw&0xfffc00ffu)!=0x62c80000u&&(raw&0x3ff00000u)!=0x22b00000u)return {};}
 if(!items::weapon_passives::energy(equipped->definition.modifiers))return {};
 const auto damage=arthur?items::arthur_weapon::usable_base(equipped->definition,policy.effective):
  items::requirements::usable_weapon_damage(equipped->definition.modifiers,policy.effective);
 if(!damage||!damage->raw_damage_type||*damage->raw_damage_type>=14)return {};
 const auto mastery=bow?25u:melee?(type==32?29u:type==2?18u:type==15?19u:20u):policy.mastery_attribute.value_or(0);
 if(bow&&*damage->raw_damage_type!=1)return {};
 // Daggers have reviewed piercing OR slashing base, with existing elemental conversions.
 if(melee&&(type==32?(*damage->raw_damage_type!=1&&*damage->raw_damage_type!=2):
     *damage->raw_damage_type!=(type==15?0u:2u))&&!element(damage->raw_damage_type))return {};
 return Owned{equipped->definition.id,revision,*damage,(melee||bow)?unsigned(policy.effective[mastery]):policy.mastery_attribute?unsigned(policy.effective[*policy.mastery_attribute]):0u,
  !metadata->customized_character.empty(),(melee||bow)?false:policy.caster_weapon,unsigned(type),(melee||bow)?std::optional<double>{0.}:std::nullopt};
}
struct Timing {std::uint64_t windup_ms=0,period_ms=0;double range=0;};
struct Snapshot {Scope attacker,target;std::uint32_t item=0;std::uint64_t revision=0;bool alive=false,target_alive=false,hostile=false,line_of_sight=false;double distance=0;};
struct StrikeIntent {Scope attacker,target;Owned weapon;std::uint64_t released=0;};
enum class Cancel {explicit_input,target_departed,actor_lifetime,equipment_changed,invalid_state};
class Intent {
 std::optional<StrikeIntent> active_;Timing timing_;std::uint64_t due_=0,last_=0,epoch_=0;bool first_release_=true;
 bool valid(const Snapshot& s)const{return active_&&s.attacker==active_->attacker&&s.target==active_->target&&s.item==active_->weapon.item&&s.revision==active_->weapon.revision&&s.alive&&s.target_alive&&s.hostile&&s.line_of_sight&&std::isfinite(s.distance)&&s.distance>=0&&s.distance<=timing_.range;}
public:
 bool start(Scope attacker,Scope target,const Owned& weapon,Timing timing,const Snapshot& current,std::uint64_t now){
  if(epoch_==std::numeric_limits<std::uint64_t>::max()||!attacker.instance||!attacker.actor||!attacker.generation||target.instance!=attacker.instance||!target.actor||!target.generation||target.actor==attacker.actor||!weapon.item||
   !timing.period_ms||timing.period_ms>60000||timing.windup_ms>timing.period_ms||!std::isfinite(timing.range)||timing.range<=0||timing.range>5000||now>std::numeric_limits<std::uint64_t>::max()-timing.windup_ms)return false;
  active_=StrikeIntent{attacker,target,weapon,0};timing_=timing;last_=now;due_=now+timing.windup_ms;first_release_=true;
  if(!valid(current)){active_.reset();return false;}++epoch_;return true;
 }
 void cancel(Cancel){if(active_&&epoch_!=std::numeric_limits<std::uint64_t>::max())++epoch_;active_.reset();}
 std::uint64_t epoch()const{return epoch_;}
 bool active()const{return active_.has_value();}
 // Preserve the remaining fraction of the current attack phase when a status
 // changes its speed. This is explicit scheduler policy; no past releases are
 // replayed. A caller must tick overdue work before applying a new timing.
 bool retime(Timing timing,std::uint64_t now){
  if(!active_||now<last_||now>due_||!timing.period_ms||timing.period_ms>60000||
     timing.windup_ms>timing.period_ms||!std::isfinite(timing.range)||
     timing.range<=0||timing.range>5000)return false;
  if(timing.windup_ms==timing_.windup_ms&&timing.period_ms==timing_.period_ms&&timing.range==timing_.range)return true;
  const auto old_phase=first_release_?timing_.windup_ms:timing_.period_ms;
  const auto new_phase=first_release_?timing.windup_ms:timing.period_ms;
  const auto remaining=due_-now;
  if(remaining>old_phase)return false;
  // Both phases are capped at60000, so this integer product cannot overflow.
  const auto adjusted=old_phase?(remaining*new_phase+old_phase-1)/old_phase:0;
  if(now>std::numeric_limits<std::uint64_t>::max()-adjusted)return false;
  timing_=timing;last_=now;due_=now+adjusted;return true;
 }
 bool windup_ready(std::uint64_t now)const{return active_&&now>=due_-timing_.windup_ms;}
 // Read-only deadline from the existing owned windup/repeat state. World may
 // prioritize this timer without introducing a second cadence or event clock.
 std::optional<std::uint64_t> release_deadline()const{return active_?std::optional<std::uint64_t>(due_):std::nullopt;}
 std::optional<Scope> target()const{return active_?std::optional<Scope>(active_->target):std::nullopt;}
 std::optional<StrikeIntent> tick(const Snapshot& current,std::uint64_t now){
  if(!active_)return {};if(now<last_||!valid(current)){active_.reset();return {};}last_=now;
  if(now<due_)return {};auto strike=*active_;strike.released=now;first_release_=false;
  // No catch-up burst after a stall. Revalidate actual world state each release.
  if(now>std::numeric_limits<std::uint64_t>::max()-timing_.period_ms)active_.reset();else due_=now+timing_.period_ms;
  return strike;
 }
 std::optional<StrikeIntent> tick(const Snapshot& current,std::uint64_t now,Timing timing){
  if(!active_)return {};
  if(now<=due_&&!retime(timing,now)){cancel(Cancel::invalid_state);return {};}
  auto strike=tick(current,now);
  // An overdue release stays single; the changed timing governs its next cycle.
  if(strike&&active_&&!retime(timing,now))cancel(Cancel::invalid_state);
  return strike;
 }
};
}

