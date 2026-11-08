#pragma once
#include "gw_soul_leech.h"
namespace fw::gw::skills::effects {
enum class TargetedSpellFailure {none,failed,rejected};
struct MarkSubversionPlan {
 unsigned rank=0,amount=10;Time duration=6000;
 unsigned energy=10;Time activation=2000,aftercast=750,recharge=30000;
 float range=1248;
 auto operator<=>(const MarkSubversionPlan&)const=default;
};
inline std::optional<MarkSubversionPlan> mark_subversion(unsigned rank){
 const auto* r=find_skill(127);
 if(rank>20||!r||r->profession!=4||r->attribute!=4||r->type!=4||r->special!=0||
    r->energy_cost()!=10||r->health_sacrifice||r->activation!=2.f||r->aftercast!=.75f||r->recharge!=30||
    r->scale0!=10||r->scale15!=92||r->duration0!=6||r->duration15!=6||
    r->caster_body_resource!=2077||r->target_body_resource!=235)return {};
 return MarkSubversionPlan{rank,interpolate(10,92,rank)};
}
struct MarkSubversionApplication {Registry registry;Applied change;Entry entry;};
inline std::optional<MarkSubversionApplication> prepare_mark_subversion_application(const Registry& registry,
 Scope caster,Scope target,unsigned effect,const MarkSubversionPlan& plan,Time now,
 std::optional<HexDuration> adjusted={}){
 const auto canonical=mark_subversion(plan.rank);
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!canonical||*canonical!=plan||!valid(caster)||!valid(target)||caster.instance!=target.instance||
    caster.actor==target.actor||!registry.matches(target)||!effect||registry.lookup(target,effect))return {};
 const auto duration=adjusted?adjusted->for_hex(127,plan.duration,caster.actor):std::optional(plan.duration);
 if(!duration||!*duration||now>UINT64_MAX-*duration)return {};
 const Entry entry{effect,caster.actor,127,plan.rank,Category::hex,now,now+*duration};
 auto proposed=registry;auto change=proposed.apply(target,entry,Replace::same_skill);if(!change.accepted)return {};
 return MarkSubversionApplication{std::move(proposed),std::move(change),entry};
}
enum class SpellTargetRelation {unknown,ally,foe};
// World supplies the resolved spell target and its relationship to the spell
// user. A skill's generic targeting metadata cannot prove this relationship.
struct SubversionSpellTarget {Scope user,target;SpellTargetRelation relation=SpellTargetRelation::unknown;};
struct MarkSubversionReaction {Entry hex;MarkSubversionPlan plan;};
inline std::optional<MarkSubversionReaction> mark_subversion_reaction(const Registry& registry,
 Scope owner,unsigned spell,const SubversionSpellTarget& selected,Time now){
 const auto* row=find_skill(spell);
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!row||!is_spell(*row)||!valid(owner)||selected.user!=owner||!valid(selected.target)||
    selected.target.instance!=owner.instance||selected.relation!=SpellTargetRelation::ally||
    (selected.target.actor==owner.actor&&selected.target!=owner))return {};
 const auto hex=registry.newest_skill(owner,127,now);
 if(!hex||hex->kind!=Category::hex||!hex->cause||hex->cause==owner.actor)return {};
 const auto plan=mark_subversion(hex->rank);if(!plan)return {};
 return MarkSubversionReaction{*hex,*plan};
}
struct MarkSubversionOwnership {Scope caster,afflicted;unsigned effect=0;Time applied=0;};
struct MarkSubversionFailure {
 Registry registry;Entry consumed;MarkSubversionOwnership owner;
 std::uint64_t event=0;Time at=0;LifeStealResult health;
};
// Atomic proposal only: World reserves the spell activation, validates the
// resolved ally relationship, and commits removal, failure and both Health
// changes together. A discarded proposal leaves the hex and Health untouched.
inline std::optional<MarkSubversionFailure> prepare_mark_subversion_failure(const Registry& registry,
 const MarkSubversionOwnership& owner,Scope live_caster,Scope live_afflicted,Scope live_spell_target,
 unsigned spell,const SubversionSpellTarget& selected,std::uint64_t event,Time now,
 const combat::Health& caster_health,const combat::Health& afflicted_health,double reduction=0){
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!event||!owner.effect||!valid(owner.caster)||!valid(owner.afflicted)||
    owner.caster.instance!=owner.afflicted.instance||owner.caster.actor==owner.afflicted.actor||
    live_caster!=owner.caster||live_afflicted!=owner.afflicted||live_spell_target!=selected.target)return {};
 const auto reaction=mark_subversion_reaction(registry,live_afflicted,spell,selected,now);
 if(!reaction||reaction->hex.instance!=owner.effect||reaction->hex.applied!=owner.applied||
    reaction->hex.cause!=owner.caster.actor)return {};
 const auto transfer=steal_life_amount(reaction->plan.amount,caster_health,afflicted_health,reduction);
 if(!transfer)return {};
 auto proposed=registry;const auto removed=proposed.remove(live_afflicted,owner.effect);
 if(!removed)return {};
 return MarkSubversionFailure{std::move(proposed),*removed,owner,event,now,*transfer};
}
}
