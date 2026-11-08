#pragma once
#include "gw_companion_policy.h"
#include <cstdint>
#include <optional>

namespace fw::gw::ai::action {
using Actor = companion::Actor;
enum class Kind {weapon_attack, attack_skill, skill};
struct Context {
 Actor caster;
 std::optional<Actor> target;
 // Damage/credit attribution may differ from actual caster (captured Fire Imp).
 std::optional<Actor> credit_owner;
 Kind kind=Kind::weapon_attack;
 std::uint32_t skill=0;
};
struct Pending {std::uint64_t serial=0;Context context;};
struct Released {std::uint64_t serial=0;Context context;};

// Identity/provenance latch only. The caller supplies authoritative lifetime
// keys and decides costs, cooldown, range, LoS, interruption and release timing.
// A Released value owns its original context; retargeting the controller cannot
// rewrite it. Release does not imply a projectile or immediate damage.
class Latch {
 Actor caster_;
 std::uint64_t next_serial_=0;
 std::optional<Pending> pending_;
 static bool valid(const Context& c){
  if(!c.caster.valid())return false;
  for(const auto& actor:{c.target,c.credit_owner})
   if(actor&&(!actor->valid()||actor->instance!=c.caster.instance))return false;
  switch(c.kind){
  case Kind::weapon_attack:return c.target.has_value()&&c.skill==0;
  case Kind::attack_skill:return c.target.has_value()&&c.skill!=0;
  case Kind::skill:return c.skill!=0;
  }
  return false;
 }
public:
 void bind(Actor caster){if(caster_!=caster){pending_.reset();caster_=caster;}}
 const std::optional<Pending>& pending()const{return pending_;}
 std::optional<std::uint64_t> start(const Context& context){
  if(context.caster!=caster_||!valid(context)||pending_)return std::nullopt;
  // Serial zero is never valid, even after wraparound.
  if(++next_serial_==0)++next_serial_;
  pending_=Pending{next_serial_,context};return next_serial_;
 }
 bool cancel(std::uint64_t serial){
  if(!pending_||pending_->serial!=serial)return false;
  pending_.reset();return true;
 }
 std::optional<Released> release(std::uint64_t serial,Actor current_caster,
   std::optional<Actor> current_target){
  if(!pending_||pending_->serial!=serial)return std::nullopt;
  // A stale release consumes its matching pending action rather than leaving
  // an action eligible to fire later at an agent ID's replacement generation.
  const auto action=*pending_;pending_.reset();
  if(current_caster!=caster_||current_caster!=action.context.caster||
    current_target!=action.context.target)return std::nullopt;
  return Released{action.serial,action.context};
 }
};
} // namespace fw::gw::ai::action
