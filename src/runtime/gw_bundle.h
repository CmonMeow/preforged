#pragma once
#include "gw_item_wire.h"
#include "gw_effect_registry.h"
#include <cmath>
#include <optional>
namespace fw::gw::bundles {
using Scope=skills::effects::Scope;
using Time=std::uint64_t;
enum class Disposition {ground,destroy};
enum class Cause {manual,shadow_step,weapon_switch,death,zone_exit,replacement};
struct Held {
 Scope owner;std::uint64_t generation=0;items::Definition item;Time acquired=0;
 Disposition disposition=Disposition::ground;
};
struct Drop {
 Held held;std::uint64_t event=0;Time at=0;Cause cause=Cause::manual;
 float x=0,y=0;std::uint16_t plane=0;
};
inline bool valid_owner(Scope owner){return owner.instance&&owner.actor&&owner.actor<=65535&&owner.generation;}
inline bool valid(const Held& held){
 if(!(valid_owner(held.owner)&&held.generation&&held.item.id&&held.item.id<=65535&&
  held.item.type==6&&held.item.quantity==1&&
  (held.disposition==Disposition::ground||held.disposition==Disposition::destroy)))return false;
 try{(void)items::definition_2022(held.item);}catch(const std::invalid_argument&){return false;}
 return true;
}
// No bundle is inferred from an inventory bag or client request. A trusted
// pickup/item-spell producer supplies the complete definition and drop policy.
// The caller commits this proposed state with ground/item-spell effects and
// wire publication. Merely preparing it cannot consume a bundle or event.
class State {
 Scope owner_;Time clock_=0;std::uint64_t generation_=0,last_drop_=0;
 std::optional<Held> held_;
public:
 explicit State(Scope owner):owner_(owner){if(!valid_owner(owner))throw std::invalid_argument("invalid bundle owner");}
 const std::optional<Held>& held()const{return held_;}
 std::optional<State> acquire(Scope owner,const items::Definition& item,Disposition disposition,Time now)const{
  if(owner!=owner_||held_||now<clock_||generation_==UINT64_MAX)return {};
  Held next{owner,generation_+1,item,now,disposition};if(!valid(next))return {};
  auto proposed=*this;proposed.held_=std::move(next);++proposed.generation_;proposed.clock_=now;return proposed;
 }
 struct End;
 std::optional<End> drop(Scope owner,std::uint64_t generation,std::uint64_t event,Time now,Cause cause,
                         float x,float y,std::uint16_t plane)const;
};
struct State::End {State state;Drop drop;};
inline std::optional<State::End> State::drop(Scope owner,std::uint64_t generation,std::uint64_t event,Time now,
                                           Cause cause,float x,float y,std::uint16_t plane)const{
 if(owner!=owner_||!held_||held_->generation!=generation||!event||event<=last_drop_||now<clock_||
    !std::isfinite(x)||!std::isfinite(y)||static_cast<unsigned>(cause)>static_cast<unsigned>(Cause::replacement))return {};
 auto proposed=*this;proposed.held_.reset();proposed.clock_=now;proposed.last_drop_=event;
 return End{std::move(proposed),Drop{*held_,event,now,cause,x,y,plane}};
}
}
