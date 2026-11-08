#pragma once
#include "gw_armor_bounds.h"
#include "gw_deep_wound.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>
#include <functional>

namespace fw::gw::combat {
using Packet=std::vector<std::uint8_t>;
enum class Kind {earth,armor_ignoring,lightning,fire,cold,piercing,slashing,holy};
enum class Reason {accepted,invalid,not_hostile,not_damageable,unknown_health,unknown_armor,dead};
inline const char* reason_text(Reason reason){switch(reason){
case Reason::accepted:return "accepted";case Reason::invalid:return "invalid combat state or damage";
case Reason::not_hostile:return "target is not an authoritative foe";
case Reason::not_damageable:return "target is not enabled for combat";
case Reason::unknown_health:return "target health is unknown";
case Reason::unknown_armor:return "target damage-type armor is unknown";
case Reason::dead:return "target is dead";}return "invalid combat result";}

// Unknown NPC health/armor remains absent. Never derive these values from an
// observed level, NPC body definition, town name or a player's base resources.
struct Health {double current=0;std::uint32_t maximum=0;};
// Server-side provenance for a cumulative flat-reduction proposal. It is
// never serialized to clients and is revalidated by the owning effect authority.
struct CumulativeReductionReceipt {
 unsigned skill=0,effect=0,reduction=0;
 std::uint64_t application=0,event=0,prior_hits=0,at=0;
 bool operator==(const CumulativeReductionReceipt&)const=default;
};
struct Actor {
    std::uint32_t id=0;
    std::uint64_t generation=0; // Instance owner validates this at effect time.
    std::optional<Health> health;
    std::optional<int> earth_armor; // Reviewed FINAL rating, not raw profession AR.
    std::uint32_t effects=0; // Full authoritative agent effects; not NPC flags.
    bool damageable=false;
    std::optional<int> lightning_armor;
    std::optional<int> fire_armor,cold_armor;
    // Authoritative direct-damage vulnerability; health loss/degeneration and
    // life stealing use separate resolvers and must not consult this value.
    double incoming_damage_multiplier=1;
    // Flat protection applies after armor and damage multipliers. Life steal
    // consumes the same protection in its separate transfer resolver.
    double incoming_reduction=0;
    std::optional<Kind> elemental_protection;
    double elemental_multiplier=1;
    // Separately owned armor added after penetration and the ordinary bonus
    // cap (Illusionary Weaponry). Unknown authority rejects armored damage.
    std::optional<unsigned> post_penetration_armor=0;
    // Override for damage-only protection; absent preserves legacy protection.
    std::optional<double> life_steal_reduction;
    // Threshold protection: after amplification, before Mantras and flat reduction.
    std::optional<double> incoming_damage_cap;
    // Reviewed physical armor after equipment/condition/skill modifiers.
    // Kept separate from elemental armor so touch skills cannot silently
    // borrow an unrelated Earth rating.
    std::optional<int> physical_armor;
    // Non-elemental, armor-respecting spell damage. Keep its reviewed rating
    // separate from Earth and physical damage-specific armor modifiers.
    std::optional<int> holy_armor;
    std::optional<CumulativeReductionReceipt> cumulative_reduction;
};
struct Damage {
    Kind kind=Kind::earth;
    double amount=0; // Server-owned skill base value, not a client request field.
    unsigned caster_level=0;
    std::uint32_t skill=0;
    unsigned penetration=0;
    // Armor bypass is independent of elemental identity (e.g. Conjures).
    // Keep the kind for elemental defenses and hit attribution.
    bool ignores_armor=false;
};
struct Hit;
// Sealed server-only evidence for a real damage event settling latent wound
// capacity debt. Ordinary Hit arithmetic remains mandatory without this proof.
class DamageHealthSettlement {
    std::uint32_t actor_;std::uint64_t generation_;Health before_;double damage_;
    deep_wound::State capacity_;
    DamageHealthSettlement(std::uint32_t actor,std::uint64_t generation,Health before,double damage,deep_wound::State capacity)
        :actor_(actor),generation_(generation),before_(before),damage_(damage),capacity_(capacity){}
    friend bool settle_damage_health(Hit&,deep_wound::State&);
public:
    bool matches(const Hit&)const;
};
struct Hit {
    Reason reason=Reason::invalid;
    std::uint32_t target=0,skill=0,effects=0;
    std::uint64_t generation=0;
    Kind kind=Kind::earth;
    double damage=0,applied=0;
    Health before{},after{};
    bool killed=false;
    std::optional<CumulativeReductionReceipt> cumulative_reduction;
    std::optional<DamageHealthSettlement> health_settlement;
};
// Optional pure preflight projection. Keep the original post-protection hit
// for authoritative commit;projected is the remaining pure-loss event after
// any healing conversion. No projected event means healing itself was lethal.
struct DamageStep {Hit incoming;std::optional<Hit> projected;};
using DamageProjection=std::function<std::optional<DamageStep>(Actor&,const Damage&)>;
// Optional copied batch preflight. Return unconditioned armor for the next
// damage event after inspecting the preceding resolved hit;never mutate World.
using ArmorSequence=std::function<std::optional<int>(const Hit*)>;
inline bool valid(const Health& health){
    // Defensive server bounds, not claims of retail creature limits.
    return health.maximum&&health.maximum<=1000000&&std::isfinite(health.current)&&
        health.current>=0&&health.current<=health.maximum;
}
inline bool DamageHealthSettlement::matches(const Hit& hit)const{
    if(hit.target!=actor_||hit.generation!=generation_||hit.damage!=damage_||
       hit.before.current!=before_.current||hit.before.maximum!=before_.maximum||
       !capacity_.active||!capacity_.base||capacity_.base>1000000||
       capacity_.reduction!=std::min(100u,capacity_.base/5)||
       capacity_.maximum()!=before_.maximum||!std::isfinite(capacity_.debt)||capacity_.debt<=0||damage_<=0)return false;
    auto capacity=capacity_;auto hp=before_.current;capacity.event(hp,-damage_);
    return hit.after.maximum==before_.maximum&&hit.after.current==hp;
}
inline bool valid_damage_health(const Hit& hit){
    return valid(hit.before)&&valid(hit.after)&&hit.before.maximum==hit.after.maximum&&
        (hit.health_settlement?hit.health_settlement->matches(hit):
            hit.after.current==std::max(0.,hit.before.current-hit.damage))&&
        hit.killed==(hit.applied>0&&hit.after.current==0)&&(!hit.killed||(hit.effects&0x10));
}
inline bool settle_damage_health(Hit& hit,deep_wound::State& capacity){
    if(hit.reason!=Reason::accepted||hit.health_settlement||!valid_damage_health(hit)||
       !std::isfinite(hit.damage)||hit.damage<0||hit.applied!=std::min(hit.damage,hit.before.current))return false;
    if(!capacity.base)return true;
    if(capacity.maximum()!=hit.before.maximum||!std::isfinite(capacity.debt)||capacity.debt<0)return false;
    if(hit.damage>0&&capacity.debt>0){
        if(!capacity.active||capacity.reduction!=std::min(100u,capacity.base/5))return false;
        hit.health_settlement=DamageHealthSettlement(hit.target,hit.generation,hit.before,hit.damage,capacity);
        auto hp=hit.before.current;capacity.event(hp,-hit.damage);hit.after.current=hp;
        hit.killed=hit.applied>0&&hp==0;hit.effects=(hit.effects&~0x10u)|(hit.killed?0x10u:0u);
    }
    return true;
}
inline bool valid(const Damage& damage){
    return (damage.kind==Kind::earth||damage.kind==Kind::lightning||damage.kind==Kind::fire||damage.kind==Kind::cold||damage.kind==Kind::piercing||damage.kind==Kind::slashing||damage.kind==Kind::holy||damage.kind==Kind::armor_ignoring)&&
        std::isfinite(damage.amount)&&damage.amount>=0&&damage.amount<=1000000&&
        // Creature levels use the native BYTE field; twenty is a player cap,
        // not a restriction on trusted NPC or boss skill execution.
        damage.caster_level>=1&&damage.caster_level<=255&&damage.skill&&damage.skill<=3432&&damage.penetration<=100&&
        ((damage.kind!=Kind::armor_ignoring&&!damage.ignores_armor)||!damage.penetration);
}
inline std::optional<double> amount(const Damage& damage,std::optional<int> earth_armor){
    if(!valid(damage))return std::nullopt;
    if(damage.kind==Kind::armor_ignoring||damage.ignores_armor)return damage.amount;
    if(!earth_armor||!valid_effective_armor(*earth_armor))return std::nullopt;
    // Local 2021 wiki Damage_calculation: skill strike level = 3*caster level;
    // actual = base * 2^((strike level - final armor rating)/40). This is the
    // documented community model, not a recovered ArenaNet server calculation.
    // Retain fractional health; wiki skill descriptions round their display.
    const double rating=*earth_armor*(1.0-double(damage.penetration)/100);
    const auto resolved=damage.amount*std::exp2((3.0*damage.caster_level-rating)/40.0);
    // The notification serializer has the same bounded damage limit. Reject
    // before mutating health, rather than discovering an unpublishable hit
    // after committing the actor state.
    if(!std::isfinite(resolved)||resolved>16000000)return std::nullopt;
    return resolved;
}
inline std::optional<double> capped_damage(double damage,std::optional<double> cap){
    if(!std::isfinite(damage)||damage<0||(cap&&(!std::isfinite(*cap)||*cap<0||*cap>1000000)))return {};
    return cap?std::min(damage,*cap):damage;
}
inline Hit apply(Actor& actor,const Damage& damage,bool authoritative_hostile){
    Hit hit;hit.target=actor.id;hit.generation=actor.generation;hit.skill=damage.skill;hit.kind=damage.kind;
    if(!actor.id||actor.id>65535||!valid(damage)||
       !std::isfinite(actor.incoming_damage_multiplier)||actor.incoming_damage_multiplier<=0||
       actor.incoming_damage_multiplier>16||!std::isfinite(actor.incoming_reduction)||actor.incoming_reduction<0||actor.incoming_reduction>1000000||
       (actor.life_steal_reduction&&(!std::isfinite(*actor.life_steal_reduction)||*actor.life_steal_reduction<0||*actor.life_steal_reduction>1000000))||
       !std::isfinite(actor.elemental_multiplier)||actor.elemental_multiplier<0||actor.elemental_multiplier>1||
       (actor.elemental_protection&&*actor.elemental_protection!=Kind::earth&&*actor.elemental_protection!=Kind::fire&&*actor.elemental_protection!=Kind::cold&&*actor.elemental_protection!=Kind::lightning))return hit;
    if(!authoritative_hostile){hit.reason=Reason::not_hostile;return hit;}
    if(!actor.damageable){hit.reason=Reason::not_damageable;return hit;}
    if(!actor.health){hit.reason=Reason::unknown_health;return hit;}
    if(!valid(*actor.health))return hit;
    if(!actor.health->current||(actor.effects&0x10)){hit.reason=Reason::dead;return hit;}
    const auto armor=damage.kind==Kind::lightning?actor.lightning_armor:damage.kind==Kind::fire?actor.fire_armor:damage.kind==Kind::cold?actor.cold_armor:damage.kind==Kind::holy?actor.holy_armor:(damage.kind==Kind::piercing||damage.kind==Kind::slashing)?actor.physical_armor:actor.earth_armor;
    if(damage.kind!=Kind::armor_ignoring&&!damage.ignores_armor&&!armor){hit.reason=Reason::unknown_armor;return hit;}
    const auto computed=amount(damage,armor);if(!computed)return hit;
    const bool respects_armor=damage.kind!=Kind::armor_ignoring&&!damage.ignores_armor;
    if(respects_armor&&(!actor.post_penetration_armor||*actor.post_penetration_armor>1000))return hit;
    const auto final_armor_factor=respects_armor?std::exp2(-double(*actor.post_penetration_armor)/40.):1.;
    const double protection=actor.elemental_protection==damage.kind?actor.elemental_multiplier:1;
    const auto capped=capped_damage(*computed*final_armor_factor*actor.incoming_damage_multiplier,actor.incoming_damage_cap);
    if(!capped)return hit;
    const double amplified=std::max(0.,*capped*protection-actor.incoming_reduction);
    if(!std::isfinite(amplified)||amplified>16000000)return hit;
    hit.reason=Reason::accepted;hit.before=*actor.health;hit.damage=amplified;
    hit.cumulative_reduction=actor.cumulative_reduction;
    hit.applied=std::min(hit.damage,hit.before.current);hit.after=hit.before;
    hit.after.current=std::max(0.0,hit.before.current-hit.damage);hit.killed=!hit.after.current&&hit.applied>0;
    hit.effects=actor.effects|(hit.killed?0x10u:0u);
    actor.health=hit.after;actor.effects=hit.effects;return hit;
}
inline std::optional<DamageStep> project_damage(Actor& actor,const Damage& damage,const DamageProjection& projection={}){
    if(projection)return projection(actor,damage);
    const auto hit=apply(actor,damage,true);
    return hit.reason==Reason::accepted?std::optional(DamageStep{hit,hit}):std::nullopt;
}
namespace detail {
inline void put(Packet& packet,std::uint32_t value){for(unsigned i=0;i<4;++i)packet.push_back(static_cast<std::uint8_t>(value>>(8*i)));}
inline Packet integer(unsigned kind,std::uint32_t actor,std::uint32_t value){
    Packet packet{158,0};put(packet,kind);put(packet,actor);put(packet,value);return packet;
}
inline Packet fraction(std::uint32_t actor,const Health& health){
    Packet packet{161,0};put(packet,34);put(packet,actor);
    put(packet,std::bit_cast<std::uint32_t>(static_cast<float>(health.current/health.maximum)));return packet;
}
inline Packet effects(std::uint32_t actor,std::uint32_t value){
    Packet packet{240,0};put(packet,actor);put(packet,value);return packet;
}
}
inline std::vector<Packet> health_packets_2022(const Actor& actor){
    if(!actor.id||actor.id>65535||!actor.health||!valid(*actor.health)||
       ((actor.effects&0x10)&&actor.health->current!=0))
        throw std::invalid_argument("invalid combat health publication");
    // Existing living-agent spawn must precede every reference. Capacity must
    // precede fraction. No invented NPC energy is needed for health packets.
    std::vector<Packet> packets{detail::integer(42,actor.id,actor.health->maximum),detail::fraction(actor.id,*actor.health)};
    if(actor.effects&0x10)packets.push_back(detail::effects(actor.id,actor.effects));
    return packets;
}
inline std::vector<Packet> damage_packets_2022(std::uint32_t cause,const Hit& hit,bool critical_weapon=false){
    if(hit.reason!=Reason::accepted||!hit.damage)return {};
    if(!cause||cause>65535||!hit.target||hit.target>65535||cause==hit.target||
       !hit.skill||hit.skill>3432||!valid(hit.before)||!valid(hit.after)||
       hit.before.maximum!=hit.after.maximum||!std::isfinite(hit.damage)||
       hit.damage<0||hit.damage>16000000||!std::isfinite(hit.applied)||hit.applied<0||
       hit.applied>hit.damage||hit.applied>hit.before.current||
       hit.applied!=std::min(hit.damage,hit.before.current)||
       !valid_damage_health(hit)||
       (hit.killed!=(!hit.after.current&&hit.applied>0))||
       (hit.killed&&!(hit.effects&0x10))||
       (hit.kind!=Kind::earth&&hit.kind!=Kind::lightning&&hit.kind!=Kind::fire&&hit.kind!=Kind::cold&&hit.kind!=Kind::piercing&&hit.kind!=Kind::slashing&&hit.kind!=Kind::holy&&hit.kind!=Kind::armor_ignoring))
        throw std::invalid_argument("invalid combat damage publication");
    if(critical_weapon&&hit.kind==Kind::armor_ignoring)throw std::invalid_argument("critical weapon notice cannot describe armor-ignoring skill damage");
    // Pinned 37130: 162 descriptor60be20, callback874c50 ->794f60.
    // kind16 (earth/armor-respecting) and55 (armor-ignore) apply signed health
    // fractions. Overkill notification retains RAW damage; absolute fraction
    // then reconciles the authoritative clamped health. No second delta packet.
    // Kind17 carries the same attributed damage notification plus critical
    // flag,but does not alter the native resource cache. The following absolute
    // Health fraction is required for both ordinary and critical publication.
    Packet modifier{162,0};detail::put(modifier,critical_weapon?17u:hit.kind==Kind::armor_ignoring?55u:16u);
    detail::put(modifier,hit.target);detail::put(modifier,cause);
    detail::put(modifier,std::bit_cast<std::uint32_t>(static_cast<float>(-hit.damage/hit.after.maximum)));
    // kind10 temporarily stores the damage skill ID in global client scratch.
    // Enqueue this complete batch atomically; interleaving attribution across
    // simultaneous effects would display the wrong skill on a later event.
    std::vector<Packet> packets{detail::integer(10,cause,hit.skill),std::move(modifier),detail::fraction(hit.target,hit.after)};
    if(hit.killed)packets.push_back(detail::effects(hit.target,hit.effects));
    return packets;
}
}
