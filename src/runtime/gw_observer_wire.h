#pragma once
#include "gw_agent_wire.h"
#include "gw_combat.h"
#include <charconv>
#include <map>
#include <string>
#include <string_view>
#include <optional>
namespace fw::gw::observer::wire {
using Packet=agent::Packet;
struct Event {std::string name;std::vector<std::string> fields;};
struct ActorState {std::uint16_t plane=0;float health_fraction=1;std::uint32_t effects=0,active_skill=0;std::optional<bool> ranged_attack;};
enum class Status {applied,unsupported,invalid,missing_actor};
struct Result {std::vector<Packet> packets;Status status=Status::unsupported;std::string reason;};
// Inputs are factual decoded records, never modern wire opcodes. The caller
// preserves timestamp/source order and creates agents before registering them.
class Adapter {
 std::map<std::uint32_t,ActorState> actors_;
 static void put(Packet& p,std::uint32_t v,unsigned width=4){for(unsigned i=0;i<width;++i)p.push_back(static_cast<std::uint8_t>(v>>(8*i)));}
 static Packet generic(unsigned kind,unsigned actor,unsigned value){return combat::detail::integer(kind,actor,value);}
 static Packet fraction(unsigned actor,float value){Packet p{161,0};put(p,34);put(p,actor);put(p,std::bit_cast<std::uint32_t>(value));return p;}
 static std::optional<unsigned> number(std::string_view s){unsigned v=0;auto r=std::from_chars(s.data(),s.data()+s.size(),v);if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size())return {};return v;}
 static std::optional<float> real(std::string_view s){float v=0;auto r=std::from_chars(s.data(),s.data()+s.size(),v);if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size()||!std::isfinite(v))return {};return v;}
public:
 void seed(std::uint32_t actor,const ActorState& state){if(!actor||actor>65535||!std::isfinite(state.health_fraction)||state.health_fraction<0||state.health_fraction>1)throw std::invalid_argument("invalid replay actor seed");actors_[actor]=state;}
 void erase(std::uint32_t actor){actors_.erase(actor);}
 const ActorState* state(std::uint32_t actor)const{auto it=actors_.find(actor);return it==actors_.end()?nullptr:&it->second;}
 std::vector<Packet> sync(std::uint32_t actor,const ActorState& observed){auto old=state(actor);if(!old)throw std::invalid_argument("unspawned replay actor");std::vector<Packet> p;if(old->health_fraction!=observed.health_fraction)p.push_back(fraction(actor,observed.health_fraction));if(old->effects!=observed.effects)p.push_back(combat::detail::effects(actor,observed.effects));auto copy=observed;copy.active_skill=observed.effects&0x10?0:old->active_skill;if(!copy.ranged_attack)copy.ranged_attack=old->ranged_attack;seed(actor,copy);return p;}
 Result apply(const Event& e){
  Result r;const auto invalid=[&](const char* why){r.status=Status::invalid;r.reason=why;return r;};
  // Retail37130 registry60D148/60D16C: object IDs are WORDs,
  // animation index BYTE + flags DWORD; state value/mask are DWORDs.
  // These reference DAT objects, not living-agent IDs. Do not require a
  // player seed, or replay doors and vine bridges disappear from playback.
  if(e.name=="MAP_OBJECT"||e.name=="MAP_OBJECT_STATE"){
   if(e.fields.size()!=3)return invalid("incorrect map object field count");
   auto object=number(e.fields[0]),value=number(e.fields[1]),flags=number(e.fields[2]);
   if(!object||*object>65535||!value||!flags)return invalid("invalid map object fields");
   const bool animation=e.name=="MAP_OBJECT";
    // Pinned37130 7BDA30 indexes the DWORD hash table at9E3108.
    // Entry16 is303419C9, the vine-growth segment captured in match6.
    // Keep unknown entries quarantined rather than allowing arbitrary table reads.
    if(animation&&*value>9&&*value!=16){r.reason="unverified 2022 map object animation index";return r;}
   Packet p;put(p,animation?269:272,2);put(p,*object,2);put(p,*value,animation?1:4);put(p,*flags);
   r.packets.push_back(std::move(p));r.status=Status::applied;return r;
  }
  const bool move=e.name=="GAME_SMSG_AGENT_MOVE_TO_POINT";
  const bool attack_start=e.name=="ATTACK_STARTED"; const bool attack_end=e.name=="ATTACK_FINISHED"||e.name=="ATTACK_STOPPED"; const bool interrupt=e.name=="INTERRUPTED";
  const bool attack_skill=e.name=="ATTACK_SKILL_ACTIVATED"||e.name=="ATTACK_SKILL_FINISHED"||e.name=="ATTACK_SKILL_STOPPED";
  const bool start=e.name=="SKILL_ACTIVATED"||e.name=="INSTANT_SKILL_USED"||e.name=="ATTACK_SKILL_ACTIVATED";
  const bool finish=e.name=="SKILL_FINISHED"||e.name=="SKILL_STOPPED"||e.name=="ATTACK_SKILL_FINISHED"||e.name=="ATTACK_SKILL_STOPPED";
  const bool damage=e.name=="DAMAGE",heal=e.name=="HEAL";
  if(!move&&!start&&!finish&&!damage&&!heal&&!attack_start&&!attack_end&&!interrupt){r.reason=e.name=="KNOCKED_DOWN"?"knockdown record omits required duration":"event has no verified 2022 adapter";return r;}
  const auto n=move?4u:attack_start?2u:start||finish||attack_end||interrupt?3u:4u;
  if(e.fields.size()!=n)return invalid("incorrect field count");
  auto id=number(e.fields[start?1:damage||heal?1:0]);if(!id||!*id||*id>65535)return invalid("invalid actor");
  auto it=actors_.find(*id);if(it==actors_.end()){r.status=Status::missing_actor;r.reason="actor not spawned";return r;}
  auto& a=it->second;
  if(move){auto x=real(e.fields[1]),y=real(e.fields[2]);auto plane=number(e.fields[3]);if(!x||!y||!plane||*plane>65535)return invalid("invalid destination");Packet p{41,0};put(p,*id);put(p,std::bit_cast<unsigned>(*x));put(p,std::bit_cast<unsigned>(*y));put(p,*plane,2);put(p,a.plane,2);r.packets={agent::movement_speed_2022(*id,1.f,1),std::move(p)};}
  else if(start){auto skill=number(e.fields[0]),target=number(e.fields[2]);if(!skill||!*skill||!target||*target>65535)return invalid("invalid skill/target fields");if(*skill>3432){r.reason="modern skill unavailable in build37130";return r;}if(*target&&!actors_.contains(*target)){r.status=Status::missing_actor;r.reason="skill target not spawned";return r;}Packet p{159,0};put(p,attack_skill?50:e.name=="INSTANT_SKILL_USED"?48:60);put(p,*id);put(p,*target);put(p,*skill);r.packets.push_back(std::move(p));if(e.name!="INSTANT_SKILL_USED")a.active_skill=*skill;}
  else if(finish){if(!number(e.fields[1])||!number(e.fields[2]))return invalid("invalid terminal fields");if(!a.active_skill){r.reason="terminal skill has no preceding activation";return r;}r.packets.push_back(generic(attack_skill?(e.name=="ATTACK_SKILL_FINISHED"?46:49):(e.name=="SKILL_FINISHED"?58:59),*id,a.active_skill));a.active_skill=0;}
  else if(attack_start){auto target=number(e.fields[1]);if(!target||!*target||*target>65535)return invalid("invalid attack target");if(!actors_.contains(*target)){r.status=Status::missing_actor;r.reason="attack target not spawned";return r;}Packet p{159,0};put(p,4);put(p,*id);put(p,*target);put(p,0);r.packets.push_back(std::move(p));}
  else if(attack_end||interrupt){auto value=number(e.fields[1]),target=number(e.fields[2]);if(!value||!target)return invalid("invalid attack terminal/interrupt fields");if(e.name=="ATTACK_FINISHED"&&!a.ranged_attack){r.reason="attack completion melee/ranged kind not recorded";return r;}r.packets.push_back(generic(interrupt?35:e.name=="ATTACK_STOPPED"?3:*a.ranged_attack?2:1,*id,*value));}
  else {auto cause=number(e.fields[0]);auto amount=real(e.fields[2]);if(!cause||*cause>65535||!amount)return invalid("invalid normalized combat event");if(*cause&&!actors_.contains(*cause)){r.status=Status::missing_actor;r.reason="combat cause not spawned";return r;}
   // Official traces use cause zero for anonymous/environmental effects.
   // Preserve it rather than inventing an attacker or rejecting regeneration.
   // Logged values are SIGNED normalized health deltas. Preserve raw overkill;
   // absolute health comes from snapshots, not guessed maximum HP or armor.
   if(damage||heal){unsigned kind=0;if(e.fields[3]=="55"||e.fields[3]=="ARMORIGNORING")kind=55;else if(e.fields[3]=="16")kind=16;else if(e.fields[3]=="17")kind=17;else {r.reason="unverified combat damage type";return r;}Packet p{162,0};put(p,kind);put(p,*id);put(p,*cause);put(p,std::bit_cast<unsigned>(*amount));r.packets.push_back(std::move(p));}
   a.health_fraction=std::clamp(a.health_fraction+*amount,0.f,1.f);r.packets.push_back(fraction(*id,a.health_fraction));
  }
  r.status=Status::applied;return r;
 }
 // These helpers consume verified snapshot facts. Missing stop/facing events
 // are not synthesized from destination changes; caller compares snapshots.
 static Packet stop(std::uint32_t id){if(!id)throw std::invalid_argument("invalid replay stop");Packet p{40,0};put(p,id);return p;}
 static Packet position(std::uint32_t id,float x,float y,std::uint16_t plane){if(!id||!std::isfinite(x)||!std::isfinite(y))throw std::invalid_argument("invalid replay position");Packet p{44,0};put(p,id);put(p,std::bit_cast<unsigned>(x));put(p,std::bit_cast<unsigned>(y));put(p,plane,2);return p;}
 static Packet facing(std::uint32_t id,float radians,float rate){return agent::facing_2022(id,radians,rate);}
 static Packet speed(std::uint32_t id,float units_per_second){return agent::movement_base_speed_2022(id,units_per_second);}
};
}




