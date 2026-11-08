#pragma once
#include "gw_skillbar.h"
#include <cmath>
#include <limits>

namespace fw::gw::combat::adrenaline {
using Time=std::uint64_t;
struct Pool {skills::Slot slot; std::uint32_t cost=0,units=0;Time disabled_until=0;std::uint8_t half=0;};
// Transient, actor-lifetime state. Costs are pinned native units, not the
// rounded number of strikes shown in a skill description (one strike = 25).
class State {
 std::vector<Pool> pools_;
 Time last_combat_=0,clock_=0;
 bool in_combat_=false;
 void advance(Time now){clock_=std::max(clock_,now);if(in_combat_&&clock_-last_combat_>=25000){reset();}}
public:
 const std::vector<Pool>& pools()const{return pools_;}
 Time cursor()const{return clock_;}
 bool configure(const skills::Slots& bar,Time now){
  std::set<std::pair<std::uint32_t,std::uint32_t>> seen;std::vector<Pool> next;
  for(auto slot:bar){if(!slot.skill){if(slot.copy)return false;continue;}
   auto* definition=skills::find_skill(slot.skill);
   if(!definition||!seen.emplace(slot.skill,slot.copy).second)return false;
   if(!definition->adrenaline)continue;
   Pool pool{slot,definition->adrenaline};
   auto old=std::find_if(pools_.begin(),pools_.end(),[&](const Pool&p){return p.slot==slot;});
   if(old!=pools_.end()){pool.units=std::min(old->units,pool.cost);pool.half=pool.units<pool.cost?old->half:0;pool.disabled_until=old->disabled_until;}
   next.push_back(pool);
  }
  advance(now); // Expiry applies to the old values before bar reordering.
  if(!in_combat_)for(auto&p:next){p.units=0;p.half=0;}
  pools_=std::move(next);return true;
 }
 void reset(){for(auto&p:pools_){p.units=0;p.half=0;}in_combat_=false;}
 void tick(Time now){advance(now);}
 void combat(Time now){advance(now);last_combat_=clock_;in_combat_=true;}
 // Preserve half of a native unit per pool. Existing whole-unit callers are
 // exact; a 3/2 modifier on25 units accumulates37.5,then75 rather than74/76.
 // Wire snapshots remain whole native units; readiness uses the exact balance.
 bool gain_halves(std::uint64_t half_units,Time now){combat(now);bool changed=false;for(auto&p:pools_){
   if(p.disabled_until>clock_)continue;
   const auto cap=2ull*p.cost;
   const auto value=std::min<std::uint64_t>(cap,2ull*p.units+p.half+std::min<std::uint64_t>(half_units,cap));
   const auto units=static_cast<std::uint32_t>(value/2);const auto half=static_cast<std::uint8_t>(value%2);
   changed|=units!=p.units||half!=p.half;p.units=units;p.half=half;
  }return changed;
 }
 bool gain(std::uint32_t units,Time now){return gain_halves(2ull*units,now);}
 bool hit(Time now){return gain(25,now);}
 bool damage(double armor_resolved,std::uint32_t maximum_health,Time now,unsigned half_rate=2){
  if((half_rate!=2&&half_rate!=3&&half_rate!=4)||!std::isfinite(armor_resolved)||armor_resolved<0||!maximum_health)return false;
  const auto units=std::floor(100.*armor_resolved/maximum_health);
  return gain_halves(std::uint64_t(static_cast<std::uint32_t>(std::min<double>(units,std::numeric_limits<std::uint32_t>::max())))*half_rate,now);
 }
 bool ready(skills::Slot slot,Time now){advance(now);auto p=std::find_if(pools_.begin(),pools_.end(),[&](const Pool&p){return p.slot==slot;});return p!=pools_.end()&&p->disabled_until<=clock_&&p->units>=p->cost;}
 bool spend(skills::Slot slot,Time now){if(!ready(slot,now))return false;combat(now);for(auto&p:pools_){
  const auto before=2ull*p.units+p.half;const auto remaining=p.slot==slot||before<=50?0:before-50;
  p.units=static_cast<std::uint32_t>(remaining/2);p.half=static_cast<std::uint8_t>(remaining%2);
 }return true;}
 // Hammer Bash loses every pool's charge, including copies. This is a spend,
 // not leaving combat: disabled skill deadlines and the combat clock survive.
 bool spend_all(skills::Slot slot,Time now){if(!ready(slot,now))return false;combat(now);for(auto&p:pools_){p.units=0;p.half=0;}return true;}
 // Explicit loss has no ready-skill requirement. Keep combat history and
 // independent disables; unlike reset(),this is not leaving combat.
 bool lose_all(Time now){if(now<clock_)return false;advance(now);for(auto& p:pools_){p.units=0;p.half=0;}return true;}
 bool disable(skills::Slot slot,Time until,Time now){advance(now);auto p=std::find_if(pools_.begin(),pools_.end(),[&](const Pool&p){return p.slot==slot;});if(p==pools_.end()||until<clock_)return false;p->units=0;p->half=0;p->disabled_until=until;return true;}
};
inline skills::Packet snapshot_2022(std::uint32_t actor,const Pool& pool){
 if(!actor||!pool.slot.skill||pool.slot.skill>65535||!pool.cost||pool.units>pool.cost||pool.half>1||(pool.units==pool.cost&&pool.half))throw std::invalid_argument("invalid adrenaline snapshot");
 skills::Packet p;auto word=[&](unsigned v){p.push_back(v&255);p.push_back((v>>8)&255);};auto dword=[&](std::uint32_t v){word(v);word(v>>16);};
 word(208);dword(actor);word(pool.slot.skill);dword(pool.slot.copy);dword(pool.units);return p;
}
inline skills::Packet spend_2022(std::uint32_t actor,skills::Slot slot){
 if(!actor||!slot.skill||slot.skill>65535)throw std::invalid_argument("invalid adrenaline spend");
 skills::Packet p;auto word=[&](unsigned v){p.push_back(v&255);p.push_back((v>>8)&255);};auto dword=[&](std::uint32_t v){word(v);word(v>>16);};
 word(209);dword(actor);word(slot.skill);dword(slot.copy);return p;
}
}
