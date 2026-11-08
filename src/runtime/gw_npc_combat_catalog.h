#include "gw_shield_absorption.h"
#include "gw_condition_damage_spells.h"
#include "gw_wound_spells.h"
#include "gw_glimmering_mark.h"
#include "gw_reversal_fortune.h"
#include "gw_vigorous_spirit.h"
#include "gw_healing_seed.h"
#include "gw_npc_friendly_heal.h"
#include "gw_weapon_spells.h"
#include "gw_ash_blast.h"
#include "gw_party_defense_shouts.h"
#include "gw_save_yourselves.h"
#include "gw_fevered_dreams.h"
#include "gw_mind_wrack.h"
#include "gw_panic_application.h"
#include "gw_hex_duration.h"
#include "gw_attack_trigger_hexes.h"
#include "gw_wandering_eye.h"
#include "gw_fragility.h"
#include "gw_frigid_armor.h"
#include "gw_stoneflesh_aura.h"
#include "gw_ether_renewal.h"
#pragma once
#include "gw_natural_stride.h"
#include "gw_convert_hexes.h"
#include "gw_watch_yourself.h"
#include "gw_npc_failed_target.h"
#include "gw_npc_offensive_plan.h"
#include "gw_storm_chaser.h"
#include "gw_hekets_rampage.h"
#include "gw_succor.h"
#include "gw_retribution.h"
#include "gw_maintained_energy.h"
#include "gw_watchful_healing.h"
#include "gw_health_regeneration.h"
#include "gw_shield_regeneration.h"
#include "gw_shield_judgment.h"
#include "gw_enchantment_duration.h"
#include "gw_price_of_failure.h"
#include "gw_lightning_strike.h"
#include "gw_lightning_surge.h"
#include "gw_insidious_parasite_transfer.h"
#include "gw_feast_of_corruption.h"
#include "gw_combat.h"
#include "gw_holy_damage.h"
#include "gw_backfire.h"
#include "gw_soul_leech.h"
#include "gw_mark_subversion.h"
#include "gw_spiteful_spirit.h"
#include "gw_soul_barbs.h"
#include "gw_life_stealing.h"
#include "gw_enchantment_removal.h"
#include "gw_grenths_balance.h"
#include "gw_diversion_authority.h"
#include "gw_diversion_prepared_use.h"
#include "gw_shadow_damage.h"
#include "gw_conditions.h"
#include "gw_creature_traits.h"
#include "gw_condition_transfer.h"
#include "gw_deep_wound.h"
#include "gw_healing_modifiers.h"
#include "gw_activation_modifiers.h"
#include "gw_weapon_hit.h"
#include "gw_weapon_element.h"
#include "gw_empathy.h"
#include "gw_faintheartedness.h"
#include "gw_degeneration_hexes.h"
#include "gw_action_damage_hexes.h"
#include "gw_parasitic_bond.h"
#include "gw_health_timing.h"
#include "gw_energy_regeneration.h"
#include "gw_overcast_settlement.h"
#include "gw_energy_enchantment.h"
#include "gw_owned_energy_hex.h"
#include "gw_protective_spirit.h"
#include "gw_shield_deflection.h"
#include "gw_tainted_flesh.h"
#include "gw_movement_effects.h"
#include "gw_npc_action_control.h"
#include "gw_npc_skill_ai.h"
#include "gw_aftershock.h"
#include <charconv>
#include <iomanip>
#include <istream>
#include <map>
#include <span>
#include <sstream>
#include <string>
#include <string_view>

namespace fw::gw::combat::npcs {
// This is reviewed server content, separate from captured appearance and from
// an agent's mutable instance state. Absence never supplies retail defaults.
enum class Relation {unknown,protected_ally,neutral,hostile_to_players,allied_to_players};
struct Binding {
    std::uint32_t map=0,actor=0,definition=0,model_file=0,skin_file=0;
    std::uint32_t appearance=0,definition_flags=0;
    std::uint8_t level=0;
    bool operator==(const Binding&)const=default;
};
struct Observed {Binding binding;bool protected_service=false;};
struct Source {std::string kind,sha256,reference;};
struct Evidence {std::string relation,health,armor,allegiance,conditions;};
struct Energy {unsigned current=0,maximum=0;bool operator==(const Energy&)const=default;};
struct Definition {
    Binding binding;
    Relation relation=Relation::unknown;
    std::optional<std::uint32_t> packed_allegiance;
    std::optional<Health> spawn_health;
    std::optional<int> earth_armor;
    bool damageable=false,conditions_allowed=false;
    Evidence evidence;
    std::optional<int> fire_armor,cold_armor;
    std::string fire_armor_evidence="-",cold_armor_evidence="-";
    std::optional<int> physical_armor;
    std::string physical_armor_evidence="-";
    unsigned fire_multiplier=1;std::string fire_multiplier_evidence="-";
    std::optional<int> lightning_armor;
    std::string lightning_armor_evidence="-";
    // Reviewed FINAL rating for nonphysical, nonelemental weapon damage.
    // Never infer this from a physical/elemental final rating.
    std::optional<int> general_armor;
    std::string general_armor_evidence="-";
    std::optional<Energy> spawn_energy;
    std::string energy_evidence="-";
    std::optional<unsigned> energy_pips;
    std::string energy_pips_evidence="-";
    skills::effects::HolyTraits holy_traits;
    std::string light_sensitive_evidence="-",summoned_evidence="-";
    std::optional<bool> knockdown_allowed;
    std::string knockdown_evidence="-";
    std::optional<bool> healing_allowed;
    std::string healing_evidence="-";
    std::optional<bool> fleshy;
    std::string fleshy_evidence="-";
    std::optional<std::string> creature_type;
    std::string creature_type_evidence="-";
    std::optional<bool> disease_immune;
    std::string disease_immunity_evidence="-";
    std::optional<bool> servant_of_abaddon;
    std::string servant_of_abaddon_evidence="-";
};
struct Lifetime {
    std::uint64_t world=0,instance_generation=0,actor_generation=0;
    std::uint32_t map=0,actor=0;
    bool operator==(const Lifetime&)const=default;
};
struct EnchantmentEnd {
    skills::effects::Entry entry;
    std::uint64_t at=0;
    Health after;
    double gained=0;
};

// The instance mutex must cover checking the handle, mutation and publication.
// Generation is not a lock. No actor pointers may escape that transaction.
class LiveActor {
    Lifetime lifetime_;
    Binding binding_;
    std::function<unsigned(skills::effects::Scope,std::uint64_t)> environmental_elemental_armor_;
    std::function<bool(skills::effects::Scope,std::uint64_t)> environmental_knockdown_protection_;
    unsigned elemental_skill_armor(std::uint64_t now,const skills::effects::EndingHexContext* ending_hex=nullptr)const{
        return skills::effects::stacked_skill_armor(skill_armor_bonus(now,ending_hex),environmental_elemental_armor_?environmental_elemental_armor_(hex_scope(),now):0);
    }
    std::vector<EnchantmentEnd> enchantment_ends_;
    Actor actor_;
    bool friendly_spell_source_=false;
    std::uint64_t health_loss_revision_=0;
    // Copies retain private receipts until their entire proposal is committed.
    void assign_health(std::optional<Health> next){
        if(actor_.health&&next&&next->current<actor_.health->current){
            if(health_loss_revision_==UINT64_MAX)throw std::overflow_error("NPC Health-loss revision exhausted");
            ++health_loss_revision_;
        }
        actor_.health=std::move(next);
    }
    Relation relation_;
    std::optional<std::uint32_t> allegiance_;
    std::optional<skills::effects::Scope> owned_stalker_owner_;
    bool owned_stalker_pve_=false;
    bool conditions_allowed_;
    std::optional<bool> fleshy_;
    std::optional<std::string> creature_type_;
    std::optional<bool> disease_immune_;
    std::optional<bool> servant_of_abaddon_;
    conditions::State conditions_;
    struct DiseaseCredit {conditions::Entry infection;skills::effects::Scope caster;};
    std::optional<DiseaseCredit> disease_credit_;
    deep_wound::State wound_;
    std::optional<std::uint64_t> condition_clock_;
    std::optional<std::pair<unsigned,std::uint64_t>> glimmering_removed_;
    void sync_wound(std::uint64_t now){
        if(!actor_.health)return;
        if(!wound_.base&&!conditions_.has(conditions::Kind::deep_wound,now))return;
        if(!wound_.base)wound_.reset(static_cast<unsigned>(actor_.health->maximum));
        auto wound=wound_;auto health=*actor_.health;
        wound.capacity(health.current,wound.base,conditions_.has(conditions::Kind::deep_wound,now));
        health.maximum=wound.maximum();assign_health(health);wound_=std::move(wound);
    }
    void finish_phantom_pain(const skills::effects::Entry& entry,std::uint64_t now){
        if(entry.skill!=44||!conditions_allowed_||!actor_.health||actor_.health->current<=0)return;
        const auto plan=skills::effects::wound_spell(44,entry.rank);if(!plan)return;
        const auto before=conditions_;
        if(!conditions_.apply(conditions::Kind::deep_wound,entry.cause,now,plan->wound_duration))return;
        sync_wound(now);react_fragility(before,now);
        actor_.effects=conditions_.presentation(actor_.effects,now);
    }
    std::optional<int> physical_armor_;
    std::optional<int> general_armor_;
    std::optional<Energy> energy_;
    std::uint64_t storm_chaser_ready_=0;
    std::uint64_t hekets_rampage_ready_=0;
    std::optional<unsigned> energy_pips_;
    fw::gw::energy::Regeneration energy_regeneration_;
    std::optional<std::uint64_t> energy_clock_;
    std::optional<overcast::CeilingCurve> energy_curve_;
    std::optional<skills::effects::MindWrackTransition> energy_zero_;
    std::optional<std::uint64_t> energy_zero_before_ms_;
    std::optional<skills::effects::MindWrackEvents> energy_zero_events_;
    bool settle_energy_clock(const Lifetime& expected,std::uint64_t now,int outside_pips=0){
        if(!matches(expected)||!energy_||!actor_.health||(energy_clock_&&now<*energy_clock_))return false;
        if(energy_zero_&&now>energy_zero_->at)return false;
        if(!energy_pips_){energy_clock_=now;return true;}
        if(!energy_clock_){energy_clock_=now;return true;}
        const int effective_pips=std::clamp(int(*energy_pips_)+outside_pips,-10,10);
        // This adapter uses Health only as an alive flag;it never replaces the
        // NPC Health authority with an integer or a player resource record.
        vitals::Resources resources{actor_.health->current>0?1u:0u,1,energy_->current,energy_->maximum};
        if(energy_curve_){
            const auto state=overcast::settle(*energy_curve_,hex_scope(),resources,energy_regeneration_,0,now,effective_pips);
            if(!state||state->debt||state->fractional_tail)return false;
            if(state->first_zero&&*state->first_zero<now)return false;
            if(state->first_zero&&!energy_zero_){
                energy_zero_=skills::effects::energy_zero_observation(*state,hex_scope(),1);
                if(!energy_zero_)return false;energy_zero_before_ms_=state->first_zero_before_units;
            }
            energy_curve_=state->curve;energy_regeneration_=state->regeneration;energy_->current=state->resources.energy;
            energy_curve_->prune_energy_modifiers(hex_scope(),now);
        }else{
            energy_regeneration_.advance(resources,now-*energy_clock_,effective_pips);energy_->current=resources.energy;
        }
        energy_clock_=now;return true;
    }
    std::optional<ai::offensive::OvercastObservation> overcast_observation_;
    skills::effects::Registry hexes_;
    skills::effects::ShieldAbsorption absorption_;
    std::uint64_t absorption_event_=0;
    std::optional<skills::effects::OwnedWeaponSpell> weapon_spell_;
    std::optional<skills::effects::ConvertHexesArmor> convert_hexes_armor_;
    std::optional<skills::effects::WatchYourselfOwned> watch_yourself_;
public:
    std::optional<skills::effects::Applied> apply_glimmering_mark(const Lifetime& expected,
            skills::effects::Scope caster,const skills::effects::GlimmeringMarkPlan& plan,unsigned effect,std::uint64_t now){
        const auto canonical=skills::effects::glimmering_mark(plan.rank);
        if(!canonical||*canonical!=plan||!matches(expected)||!hostile_to_players(expected)||
           !caster.instance||caster.instance!=hex_scope().instance||!caster.actor||caster.actor>65535||
           !caster.generation||caster.actor==expected.actor||!effect||!actor_.damageable||!actor_.health||actor_.health->current<=0||
           now>UINT64_MAX-plan.duration-plan.blind_ms||(condition_clock_&&now<*condition_clock_))return {};
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return {};
        if(proposed.actor_.health->current<=0)return {};
        const auto applied=proposed.hexes_.apply(proposed.hex_scope(),{effect,caster.actor,227,plan.rank,
            skills::effects::Category::hex,now,now+plan.duration},skills::effects::Replace::same_skill);
        if(!applied.accepted)return {};
        proposed.glimmering_removed_.reset();
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(proposed.hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return applied;
    }
    // End-effect hex has a dedicated lane; generic degeneration admission
    // intentionally excludes it. Replacement renews the hex without ending it.
    std::optional<skills::effects::Applied> apply_phantom_pain(const Lifetime& expected,
            const skills::effects::WoundSpellPlan& plan,unsigned cause,unsigned effect,std::uint64_t now,
            std::optional<skills::effects::HexDuration> adjusted={}){
        const auto duration=adjusted?adjusted->for_hex(plan.skill,plan.hex_duration,cause):std::optional(plan.hex_duration);
        if(!skills::effects::phantom_pain_deadlines(plan,now)||!matches(expected)||
           !duration||plan.wound_duration>UINT64_MAX-*duration||now>UINT64_MAX-*duration-plan.wound_duration||
           !hostile_to_players(expected)||!actor_.damageable||!actor_.health||actor_.health->current<=0||
           !cause||cause>65535||!effect||(condition_clock_&&now<*condition_clock_)||
           actor_.health->maximum<1||actor_.health->maximum>UINT32_MAX||
           std::floor(actor_.health->maximum)!=actor_.health->maximum)return {};
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return {};
        if(proposed.actor_.health->current<=0)return {};
        const auto applied=proposed.hexes_.apply(proposed.hex_scope(),{effect,cause,44,plan.rank,
            skills::effects::Category::hex,now,now+*duration},skills::effects::Replace::same_skill);
        if(!applied.accepted)return {};
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(proposed.hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return applied;
    }
    struct AttackHexHit {Hit hit;unsigned cause=0;std::uint64_t at=0;unsigned effect=0;bool interrupted=false;std::optional<skills::effects::ReversalDamageReceipt> reversal;unsigned skill=0;};
    struct FragilityHit {Hit hit;unsigned cause=0;std::uint64_t at=0;unsigned effect=0;std::optional<skills::effects::ReversalDamageReceipt> reversal;};
private:
    std::vector<AttackHexHit> attack_hex_hits_;
    struct AttackEnergyChange {skills::effects::MindWrackEnergy before,after;std::uint64_t at;};
    std::vector<AttackEnergyChange> attack_energy_changes_;
    std::map<unsigned,unsigned> attack_hex_levels_;
    std::vector<FragilityHit> fragility_hits_;
    std::map<unsigned,unsigned> fragility_levels_;
    std::optional<skills::effects::FeveredOwnership> fevered_owner_;
    skills::effects::FeveredDazedSpan fevered_span_=skills::effects::FeveredDazedSpan::attempted_duration;
    bool generating_fevered_dazed_=false;
    std::uint64_t next_fevered_observation_=1;
public:
    // Local journal sequence is scoped to this NPC lifetime. It is separate
    // from World application/use IDs; World must retain its own read cursor.
    struct FeveredObservation {std::uint64_t sequence=0;skills::effects::FeveredReaction reaction;bool alive=false;};
private:
    std::vector<FeveredObservation> fevered_observations_;
    void observe_fevered_conditions(const conditions::State& before,std::uint64_t now){
        if(generating_fevered_dazed_||!fevered_owner_||!actor_.health||actor_.health->current<=0||next_fevered_observation_==UINT64_MAX)return;
        auto reaction=skills::effects::fevered_condition_reaction(hexes_,*fevered_owner_,
            {hex_scope(),next_fevered_observation_,now,true},before,conditions_,fevered_span_);
        if(!reaction||reaction->pulses.empty())return;
        if(reaction->dazed_applied){
            generating_fevered_dazed_=true;
            const auto plan=skills::effects::fevered_dreams(reaction->hex.rank,reaction->hex.skill==3289);
            const bool applied=apply_condition(lifetime_,conditions::Kind::dazed,reaction->owner.caster.actor,now,plan->dazed_duration);
            generating_fevered_dazed_=false;
            if(!applied)return;
        }
        fevered_observations_.push_back({next_fevered_observation_++,std::move(*reaction),actor_.health->current>0});
    }
    void react_fragility(const std::vector<skills::effects::ConditionTransition>& events){
        for(const auto& event:events){
            if(!actor_.health||actor_.health->current<=0)break;
            const auto reaction=skills::effects::fragility_reaction(hexes_,hex_scope(),event);if(!reaction)continue;
            const auto level=fragility_levels_.find(reaction->cause.instance);if(level==fragility_levels_.end())continue;
            const Damage damage{Kind::armor_ignoring,double(reaction->plan.damage),level->second,reaction->plan.skill};
            // Resolve on a private actor without recursively advancing the
            // condition clock whose transition is currently being processed.
            auto raw=*this;const auto incoming=raw.apply_damage_impl(lifetime_,damage,event.at,true);
            if(incoming.reason!=Reason::accepted)continue;
            if(incoming.damage>0&&hexes_.newest_skill(hex_scope(),307,event.at)){
                auto proposed=*this;const auto receipt=proposed.resolve_reversal_hit(lifetime_,incoming,event.at);
                if(!receipt||!proposed.adopt_absorption_hit(*this,raw,lifetime_,event.at))continue;
                *this=std::move(proposed);
                fragility_hits_.push_back({receipt->damage.value_or(Hit{}),reaction->cause.cause,event.at,reaction->cause.instance,receipt});
            }else{
                *this=std::move(raw);fragility_hits_.push_back({incoming,reaction->cause.cause,event.at,reaction->cause.instance,{}});
            }
        }
    }
    void react_fragility(const conditions::State& before,std::uint64_t now){
        react_fragility(skills::effects::immediate_condition_transitions(before,conditions_,now));
        observe_fevered_conditions(before,now);
    }

    void react_attack_hexes(std::uint64_t now){
        for(unsigned skill:{43u,47u,66u}){
            if(!actor_.health||actor_.health->current<=0)break;
            const auto reaction=skills::effects::consume_attack_hex(hexes_,hex_scope(),skill,now);if(!reaction)continue;
            const auto level=attack_hex_levels_.find(reaction->cause.instance);if(level==attack_hex_levels_.end())continue;
            const auto source_level=level->second;if(reaction->plan.consume)attack_hex_levels_.erase(level);
            if(reaction->plan.energy_loss){if(advance_energy(lifetime_,now)&&energy_){
                const skills::effects::MindWrackEnergy before{energy_->current,energy_->maximum,0};
                if(lose_energy(lifetime_,std::min(reaction->plan.energy_loss,energy_->current)))attack_energy_changes_.push_back({before,{energy_->current,energy_->maximum,0},now});
            }continue;}
            if(reaction->plan.interrupt)cancel_action(lifetime_,now);
            if(reaction->plan.blind_duration&&conditions_allowed_)
                apply_condition(lifetime_,conditions::Kind::blind,reaction->cause.cause,now,reaction->plan.blind_duration);
            if(!actor_.health||actor_.health->current<=0)continue;
            const Damage damage{Kind::armor_ignoring,double(reaction->plan.damage),source_level,skill};
            if(hexes_.newest_skill(hex_scope(),307,now)){
                auto preview=*this;const auto raw=preview.apply_damage_impl(lifetime_,damage,now,true);
                if(raw.reason!=Reason::accepted)continue;
                if(raw.damage>0){
                    const auto converted=apply_reversal_damage(lifetime_,damage,now,true);if(!converted)continue;
                    attack_hex_hits_.push_back({converted->damage.value_or(Hit{}),reaction->cause.cause,now,reaction->cause.instance,reaction->plan.interrupt,converted,skill});
                    continue;
                }
                *this=std::move(preview);attack_hex_hits_.push_back({raw,reaction->cause.cause,now,reaction->cause.instance,reaction->plan.interrupt,{},skill});
            }else{
                const auto hit=apply_damage_impl(lifetime_,damage,now,true);
                if(hit.reason==Reason::accepted)attack_hex_hits_.push_back({hit,reaction->cause.cause,now,reaction->cause.instance,reaction->plan.interrupt,{},skill});
            }
        }
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);
    }
    skills::effects::MovementEffects movement_;
    control::State control_;
    std::optional<ai::Spellbook> spellbook_;
    void finish_spellbook_movement(std::uint64_t now,bool recharge,bool aftercast){
        if(!spellbook_||!spellbook_->pending())return;
        spellbook_->finish(now,recharge,aftercast);
        const auto resumes=spellbook_->movement_begin(now,UINT64_MAX);
        if(!control_.end_movement_pause(hex_scope(),now,resumes-now))
            throw std::logic_error("owned NPC spell movement pause missing");
    }

    std::optional<bool> knockdown_allowed_;
    std::uint64_t last_attack_event_=0;
    std::uint64_t last_attack_time_=0;
    unsigned fire_multiplier_=1;
    skills::effects::HolyTraits holy_traits_;
    std::optional<bool> healing_allowed_;
public:
    std::optional<bool> healing_allowed(Lifetime life)const{return matches(life)?healing_allowed_:std::nullopt;}
    const skills::effects::HolyTraits* holy_traits(Lifetime life)const{return life==lifetime_?&holy_traits_:nullptr;}
    // World owns this read-only query and all calls/copies under its mutex.
    // LiveActor objects never escape their owning Instance. Standalone actors
    // default to no environmental bonus. No cached Health/position is added.
    void environmental_elemental_armor(std::function<unsigned(skills::effects::Scope,std::uint64_t)> query){environmental_elemental_armor_=std::move(query);}
    void environmental_knockdown_protection(std::function<bool(skills::effects::Scope,std::uint64_t)> query){environmental_knockdown_protection_=std::move(query);}
    LiveActor(const Definition& definition,Lifetime lifetime):lifetime_(lifetime),binding_(definition.binding),
        actor_{lifetime.actor,lifetime.actor_generation,definition.spawn_health,definition.earth_armor,0,definition.damageable},
        relation_(definition.relation),allegiance_(definition.packed_allegiance),conditions_allowed_(definition.conditions_allowed),physical_armor_(definition.physical_armor),
        hexes_({lifetime.world,lifetime.actor,lifetime.actor_generation}),
        absorption_({lifetime.world,lifetime.actor,lifetime.actor_generation}),
        control_({lifetime.world,lifetime.actor,lifetime.actor_generation}),knockdown_allowed_(definition.knockdown_allowed){
        if(!lifetime.world||!lifetime.instance_generation||!lifetime.actor_generation||
           !lifetime.actor||lifetime.actor>65535||lifetime.actor!=definition.binding.actor||
           lifetime.map!=definition.binding.map)
            throw std::invalid_argument("invalid NPC combat lifetime");
        if(actor_.health&&(!valid(*actor_.health)||!actor_.health->current))
            throw std::invalid_argument("invalid NPC combat spawn health");
        if(actor_.earth_armor&&(*actor_.earth_armor< -100||*actor_.earth_armor>500))
            throw std::invalid_argument("invalid NPC earth armor");
        general_armor_=definition.general_armor;
        energy_=definition.spawn_energy;
        energy_pips_=definition.energy_pips;
        if(energy_pips_&&(!energy_||*energy_pips_>10))throw std::invalid_argument("invalid NPC energy regeneration");
        if(energy_&&(!energy_->maximum||energy_->maximum>1000000||energy_->current>energy_->maximum))
            throw std::invalid_argument("invalid NPC spawn energy");
        for(auto armor:{definition.fire_armor,definition.cold_armor,definition.physical_armor,definition.lightning_armor,definition.general_armor})
            if(armor&&(*armor< -100||*armor>500))throw std::invalid_argument("invalid NPC elemental armor");
        actor_.fire_armor=definition.fire_armor;actor_.cold_armor=definition.cold_armor;
        actor_.lightning_armor=definition.lightning_armor;
        if(definition.fire_multiplier<1||definition.fire_multiplier>4||
           (definition.fire_multiplier!=1&&definition.fire_multiplier_evidence=="-"))
            throw std::invalid_argument("invalid NPC fire vulnerability");
        fire_multiplier_=definition.fire_multiplier;
        holy_traits_=definition.holy_traits;
        if(definition.healing_allowed.has_value()&&(definition.healing_evidence.empty()||definition.healing_evidence=="-"))throw std::invalid_argument("NPC healing trait requires evidence");
        healing_allowed_=definition.healing_allowed;
        if(definition.fleshy.has_value()&&(definition.fleshy_evidence.empty()||definition.fleshy_evidence=="-"))throw std::invalid_argument("NPC flesh trait requires evidence");
        fleshy_=definition.fleshy;
        if(definition.creature_type&&(!creatures::valid_type(*definition.creature_type)||definition.creature_type_evidence.empty()||definition.creature_type_evidence=="-"))throw std::invalid_argument("NPC creature type requires valid key and evidence");
        creature_type_=definition.creature_type;
        if(definition.servant_of_abaddon.has_value()&&
           (definition.servant_of_abaddon_evidence.empty()||
            definition.servant_of_abaddon_evidence=="-"))
            throw std::invalid_argument("NPC demonic-servant trait requires evidence");
        servant_of_abaddon_=definition.servant_of_abaddon;
        if(definition.disease_immune.has_value()&&(definition.disease_immunity_evidence.empty()||definition.disease_immunity_evidence=="-"))throw std::invalid_argument("NPC Disease immunity requires evidence");
        disease_immune_=definition.disease_immune;
        if(knockdown_allowed_.has_value()&&(definition.knockdown_evidence.empty()||definition.knockdown_evidence=="-"))
            throw std::invalid_argument("NPC knockdown trait requires evidence");
        if(actor_.damageable&&!actor_.health)
            throw std::invalid_argument("damageable NPC requires reviewed spawn health");
        if(conditions_allowed_&&!actor_.health)
            throw std::invalid_argument("NPC conditions require reviewed spawn health");
        if(relation_!=Relation::unknown&&relation_!=Relation::protected_ally&&
           relation_!=Relation::neutral&&relation_!=Relation::hostile_to_players&&relation_!=Relation::allied_to_players)
            throw std::invalid_argument("invalid NPC relation");
        if(relation_==Relation::protected_ally&&(actor_.damageable||conditions_allowed_))
            throw std::invalid_argument("protected NPC cannot receive combat effects");
    }
    const Lifetime& lifetime()const{return lifetime_;}
    std::optional<std::uint64_t> health_clock(const Lifetime& expected)const{return matches(expected)?condition_clock_:std::nullopt;}
    bool begin_external_health(const Lifetime& expected,std::uint64_t now){
        if(!matches(expected)||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_))return false;
        if(!condition_clock_)condition_clock_=now;
        return true;
    }
    std::optional<std::uint64_t> energy_clock(const Lifetime& expected)const{return matches(expected)?energy_clock_:std::nullopt;}
    skills::effects::Scope hex_scope()const{return {lifetime_.world,lifetime_.actor,lifetime_.actor_generation};}
    std::optional<double> ash_blast_miss_chance(const Lifetime& expected,std::uint64_t now)const{
        if(!matches(expected))return {};
        return skills::effects::ash_blast_miss_chance(hexes_,hex_scope(),now);
    }
    bool apply_soul_barbs(const Lifetime& expected,const skills::effects::SoulBarbsPlan& plan,unsigned cause,unsigned effect,std::uint64_t now){
        if(!skills::effects::valid_soul_barbs(plan)||!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||
           !actor_.health||actor_.health->current<=0||now>UINT64_MAX-plan.duration||(condition_clock_&&now<*condition_clock_))return false;
        if(!skills::effects::apply_soul_barbs(hexes_,hex_scope(),effect,cause,plan,now))return false;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return true;
    }
    std::optional<skills::effects::Entry> natural_stride_application(const Lifetime& expected,
            const skills::effects::ApplicationEvent& event,std::uint64_t now){
        if(!matches(expected))return {};
        const auto ended=skills::effects::natural_stride_application(hexes_,hex_scope(),event);
        if(ended){movement_.remove(ended->instance,now);actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);}
        return ended;
    }
    std::vector<skills::effects::ApplicationEvent> take_applications(const Lifetime& expected){
        return matches(expected)?hexes_.take_applications(hex_scope()):std::vector<skills::effects::ApplicationEvent>{};
    }
    bool apply_spiteful_spirit(const Lifetime& expected,const skills::effects::SpitefulSpiritPlan& plan,
            unsigned cause,unsigned effect,std::uint64_t now){
        const auto canonical=skills::effects::spiteful_spirit(plan.rank);
        if(!canonical||*canonical!=plan||!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||
           !actor_.health||actor_.health->current<=0||!cause||cause>65535||now>UINT64_MAX-plan.duration||
           (condition_clock_&&now!=*condition_clock_))return false;
        // Settle existing Health first so a delayed death keeps its credit.
        // A clean actor can establish its first condition clock here.
        if(!hexes_.apply(hex_scope(),{effect,cause,121,plan.rank,skills::effects::Category::hex,now,now+plan.duration},skills::effects::Replace::same_skill).accepted)return false;
        if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return true;
    }
    std::optional<skills::effects::Applied> apply_soul_leech(const Lifetime& expected,
            skills::effects::Scope caster,const skills::effects::SoulLeechPlan& plan,unsigned effect,
            std::uint64_t now,std::optional<skills::effects::HexDuration> duration={}){
        if(!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||
           actor_.health->current<=0||(condition_clock_&&now!=*condition_clock_))return {};
        const auto proposed=skills::effects::prepare_soul_leech_application(hexes_,caster,hex_scope(),effect,plan,now,duration);
        if(!proposed)return {};
        hexes_=proposed->registry;if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return proposed->change;
    }
    std::optional<skills::effects::SoulLeechReaction> soul_leech_reaction(const Lifetime& expected,unsigned skill,std::uint64_t now)const{
        if(!matches(expected)||!actor_.health||actor_.health->current<=0||(condition_clock_&&now<*condition_clock_))return {};
        return skills::effects::soul_leech_reaction(hexes_,hex_scope(),skill,now);
    }
    std::optional<skills::effects::SoulLeechTransfer> apply_soul_leech_transfer(const Lifetime& expected,
            const skills::effects::SoulLeechOwnership& owner,skills::effects::Scope caster,
            unsigned spell,std::uint64_t event,std::uint64_t now,const Health& source){
        if(!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||
           !condition_clock_||now!=*condition_clock_)return {};
        const auto result=skills::effects::prepare_soul_leech_transfer(hexes_,owner,caster,hex_scope(),spell,event,now,source,*actor_.health);
        if(!result)return {};
        assign_health(result->health.target);
        if(result->health.killed){actor_.effects|=0x10;control_.invalidate();}
        return result;
    }
    std::optional<skills::effects::Applied> apply_mark_subversion(const Lifetime& expected,
            skills::effects::Scope caster,const skills::effects::MarkSubversionPlan& plan,unsigned effect,
            std::uint64_t now,std::optional<skills::effects::HexDuration> duration={}){
        if(!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||
           actor_.health->current<=0||(condition_clock_&&now!=*condition_clock_))return {};
        const auto proposed=skills::effects::prepare_mark_subversion_application(hexes_,caster,hex_scope(),effect,plan,now,duration);
        if(!proposed)return {};hexes_=proposed->registry;if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return proposed->change;
    }
    std::optional<skills::effects::MarkSubversionFailure> apply_mark_subversion_failure(const Lifetime& expected,
            const skills::effects::MarkSubversionOwnership& owner,skills::effects::Scope caster,
            skills::effects::Scope spell_target,unsigned spell,const skills::effects::SubversionSpellTarget& selected,
            std::uint64_t event,std::uint64_t now,const Health& source){
        if(!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||
           !condition_clock_||now!=*condition_clock_)return {};
        const auto proposed=skills::effects::prepare_mark_subversion_failure(hexes_,owner,caster,hex_scope(),spell_target,spell,selected,event,now,source,*actor_.health);
        if(!proposed)return {};
        hexes_=proposed->registry;assign_health(proposed->health.target);
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);
        if(proposed->health.killed){actor_.effects|=0x10;control_.invalidate();}
        return proposed;
    }
    bool apply_backfire(const Lifetime& expected,const skills::effects::BackfirePlan& plan,unsigned cause,unsigned effect,std::uint64_t now){
        if(!skills::effects::valid_backfire(plan)||!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||
           !actor_.health||actor_.health->current<=0||now>UINT64_MAX-plan.duration||(condition_clock_&&now<*condition_clock_))return false;
        if(!hexes_.apply(hex_scope(),{effect,cause,plan.skill,plan.rank,skills::effects::Category::hex,now,now+plan.duration},skills::effects::Replace::same_skill).accepted)return false;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return true;
    }
    std::optional<skills::effects::Entry> backfire_reaction(const Lifetime& expected,unsigned skill,std::uint64_t now)const{
        return matches(expected)?skills::effects::backfire_reaction(hexes_,hex_scope(),skill,now):std::nullopt;
    }
    bool apply_empathy(const Lifetime& expected,unsigned skill,unsigned rank,unsigned cause,unsigned effect,std::uint64_t now){
        const auto plan=skills::effects::empathy(skill,rank);
        if(!matches(expected)||!hostile_to_players(expected)||!actor_.health||actor_.health->current<=0||!plan||now>UINT64_MAX-plan->duration)return false;
        const auto applied=hexes_.apply(hex_scope(),{effect,cause,skill,rank,skills::effects::Category::hex,now,now+plan->duration},skills::effects::Replace::same_skill);
        if(!applied.accepted)return false;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return true;
    }
    std::optional<skills::effects::Applied> apply_faintheartedness(const Lifetime& expected,
            const skills::effects::FaintheartednessPlan& plan,unsigned cause,unsigned effect,std::uint64_t now){
        if(!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||actor_.health->current<=0||
           !skills::effects::valid_faintheartedness(plan)||now>UINT64_MAX-plan.duration||(condition_clock_&&now<*condition_clock_))return {};
        if(condition_clock_)advance_conditions(expected,now);
        if(actor_.health->current<=0)return {};
        const auto applied=hexes_.apply(hex_scope(),{effect,cause,plan.skill,plan.rank,skills::effects::Category::hex,now,now+plan.duration},skills::effects::Replace::same_skill);
        if(!applied.accepted)return {};if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return applied;
    }
    // Staged NPC recipient only. The world must atomically commit the caster's
    // return ledger and handle removal/death attribution alongside this state.
    std::optional<skills::effects::Applied> apply_parasitic_bond(const Lifetime& expected,
            unsigned rank,unsigned cause,unsigned effect,std::uint64_t now){
        const auto plan=skills::effects::parasitic_bond(rank);
        if(!plan||!cause||cause>65535||!effect||!matches(expected)||!hostile_to_players(expected)||
           !actor_.damageable||!actor_.health||actor_.health->current<=0||now>UINT64_MAX-plan->duration||
           (condition_clock_&&now<*condition_clock_))return {};
        if(condition_clock_)advance_conditions(expected,now);
        if(actor_.health->current<=0)return {};
        const auto applied=hexes_.apply(hex_scope(),{effect,cause,99,rank,skills::effects::Category::hex,now,now+plan->duration},skills::effects::Replace::same_skill);
        if(!applied.accepted)return {};if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return applied;
    }
    std::optional<skills::effects::Applied> apply_insidious_parasite(const Lifetime& expected,unsigned rank,unsigned cause,unsigned effect,std::uint64_t now){
        const auto plan=skills::effects::insidious_parasite(rank);
        if(!plan||!cause||cause>65535||!effect||!matches(expected)||!hostile_to_players(expected)||
           !actor_.damageable||!actor_.health||actor_.health->current<=0||now>UINT64_MAX-plan->duration||
           (condition_clock_&&now<*condition_clock_))return {};
        if(condition_clock_)advance_conditions(expected,now);if(actor_.health->current<=0)return {};
        const auto applied=hexes_.apply(hex_scope(),{effect,cause,123,rank,skills::effects::Category::hex,now,now+plan->duration},skills::effects::Replace::same_skill);
        if(!applied.accepted)return {};if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return applied;
    }
    std::optional<skills::effects::InsidiousParasiteTransfer> apply_insidious_transfer(const Lifetime& expected,
            skills::effects::Scope caster,const weapon::QualifiedHit& hit,const Health& source){
        if(!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||
           hit.attacker!=hex_scope()||(condition_clock_&&hit.at<*condition_clock_))return {};
        const auto entry=hexes_.newest_skill(hex_scope(),123,hit.at);if(!entry)return {};
        const auto transfer=skills::effects::insidious_parasite_transfer(caster,hex_scope(),*entry,hit,source,*actor_.health);if(!transfer)return {};
        assign_health(transfer->health.target);if(transfer->health.killed){actor_.effects|=0x10;control_.invalidate();}return transfer;
    }
    std::optional<skills::effects::Applied> apply_price_of_failure(const Lifetime& expected,unsigned rank,unsigned cause,unsigned effect,std::uint64_t now){
        const auto plan=skills::effects::price_of_failure(rank);
        if(!plan||!cause||cause>65535||!effect||!matches(expected)||!hostile_to_players(expected)||
           !actor_.damageable||!actor_.health||actor_.health->current<=0||now>UINT64_MAX-plan->duration||
           (condition_clock_&&now<*condition_clock_))return {};
        if(condition_clock_)advance_conditions(expected,now);if(actor_.health->current<=0)return {};
        const auto applied=hexes_.apply(hex_scope(),{effect,cause,103,rank,skills::effects::Category::hex,now,now+plan->duration},skills::effects::Replace::same_skill);
        if(!applied.accepted)return {};if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return applied;
    }
    std::optional<skills::effects::Applied> apply_diversion(const Lifetime& expected,const skills::effects::DiversionPlan& plan,
            unsigned cause,unsigned effect,std::uint64_t now){
        const auto canonical=skills::effects::diversion(plan.rank);
        if(!canonical||plan!=*canonical||!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||
           !actor_.health||actor_.health->current<=0||now>UINT64_MAX-plan.duration||(condition_clock_&&now<*condition_clock_))return {};
        if(condition_clock_)advance_conditions(expected,now);if(actor_.health->current<=0)return {};
        const auto applied=hexes_.apply(hex_scope(),{effect,cause,30,plan.rank,skills::effects::Category::hex,now,now+plan.duration},skills::effects::Replace::same_skill);
        if(!applied.accepted)return {};if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return applied;
    }
    std::optional<skills::effects::Entry> diversion_entry(const Lifetime& expected,std::uint64_t now)const{
        return matches(expected)?hexes_.newest_skill(hex_scope(),30,now):std::nullopt;
    }
    std::optional<skills::effects::PreparedDiversionUse> prepare_diversion_use(const Lifetime& expected,
            const skills::effects::DiversionUse& use,std::uint64_t now)const{
        if(!matches(expected)||use.owner!=hex_scope()||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_))return {};
        return skills::effects::capture_diversion_use(hexes_,use,now);
    }
    std::optional<skills::effects::FinalizedDiversionUse> finalize_diversion_use(const Lifetime& expected,
            const skills::effects::DiversionAuthority& authority,const skills::effects::PreparedDiversionUse& prepared,
            std::uint64_t now,std::uint64_t base,std::uint64_t existing){
        if(!matches(expected)||prepared.use.owner!=hex_scope()||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_))return {};
        const auto result=skills::effects::finalize_diversion_use(hexes_,authority,prepared,now,base,existing);if(!result)return {};
        hexes_=result->registry;actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return result;
    }
    std::optional<std::uint64_t> diversion_skill_used(const Lifetime& expected,skills::effects::DiversionAuthority& authority,
            const skills::effects::DiversionUse& use,std::uint64_t now,std::uint64_t base,std::uint64_t existing){
        if(!matches(expected)||use.owner!=hex_scope()||!hostile_to_players(expected)||!actor_.health||actor_.health->current<=0||
           base>86400000||now>UINT64_MAX-base||(condition_clock_&&now<*condition_clock_))return {};
        if(condition_clock_)advance_conditions(expected,now);if(actor_.health->current<=0)return {};
        auto proposed=authority;
        if(!hexes_.newest_skill(hex_scope(),30,now)){
            if(!proposed.record(use,now,{}))return {};authority=std::move(proposed);return 0;
        }
        const auto result=skills::effects::prepare_diversion_use(hexes_,authority,use,now,base,existing);if(!result)return {};
        hexes_=result->effect.registry;authority=result->authority;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return result->recharge_until.value_or(0);
    }
    struct FeveredApplicationResult {
        skills::effects::FeveredApplication application;
        skills::effects::FeveredReaction reaction;
        bool alive=false;
    };
    // World owns propagation and event allocation. Stage the whole NPC change
    // so invalid application cannot advance its clock or consume a condition.
    std::optional<FeveredApplicationResult> apply_fevered_dreams(const Lifetime& expected,
            const skills::effects::Registry& caster_registry,skills::effects::Scope caster,
            const skills::effects::FeveredDreamsPlan& plan,unsigned effect,std::uint64_t event,
            std::uint64_t now,skills::effects::FeveredDazedSpan span){
        if(!event||!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||
           !actor_.health||actor_.health->current<=0||(condition_clock_&&now<*condition_clock_))return {};
        auto proposed=*this;
        if(proposed.condition_clock_)proposed.advance_conditions(expected,now);
        if(proposed.actor_.health->current<=0)return {};
        auto app=skills::effects::prepare_fevered_dreams(proposed.hexes_,caster_registry,caster,
            proposed.hex_scope(),effect,plan,now);if(!app)return {};
        auto reaction=skills::effects::fevered_condition_reaction(app->registry,app->owner,
            {proposed.hex_scope(),event,now,true},proposed.conditions_,proposed.conditions_,span,true);
        if(!reaction)return {};
        proposed.hexes_=app->registry;
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        // Use the existing condition path: Dazed gains trigger Fragility and
        // preserve longer timers; a lethal reaction must not be resurrected.
        if(reaction->dazed_applied&&!proposed.apply_condition(expected,conditions::Kind::dazed,
            caster.actor,now,plan.dazed_duration))return {};
        proposed.actor_.effects=proposed.hexes_.presentation(proposed.hex_scope(),
            proposed.actor_.effects,now);
        const bool alive=proposed.actor_.health->current>0;
        proposed.fevered_owner_=app->owner;proposed.fevered_span_=span;
        *this=std::move(proposed);
        return FeveredApplicationResult{std::move(*app),std::move(*reaction),alive};
    }
    bool has_fevered_observations(const Lifetime& expected)const{return matches(expected)&&!fevered_observations_.empty();}
    bool fevered_observations_at(const Lifetime& expected,std::uint64_t now)const{
        return matches(expected)&&std::all_of(fevered_observations_.begin(),fevered_observations_.end(),
            [now](const auto& observation){return observation.reaction.event.at==now;});
    }
    std::vector<FeveredObservation> take_fevered_observations(const Lifetime& expected){
        if(!matches(expected))return {};
        auto result=std::move(fevered_observations_);fevered_observations_.clear();return result;
    }
    std::optional<skills::effects::MindWrackApplication> apply_mind_wrack(const Lifetime& expected,skills::effects::Scope caster,
            const skills::effects::MindWrackPlan& plan,unsigned effect,std::uint64_t now){
        if(!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_))return {};
        if(condition_clock_)advance_conditions(expected,now);if(actor_.health->current<=0)return {};
        auto proposed=skills::effects::prepare_mind_wrack(hexes_,caster,hex_scope(),effect,plan,now);if(!proposed)return {};
        hexes_=proposed->registry;if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return proposed;
    }
    std::optional<skills::effects::Applied> apply_panic(const Lifetime& expected,skills::effects::Scope caster,
            const skills::effects::PanicPlan& plan,unsigned effect,std::uint64_t now){
        if(!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_))return {};
        if(condition_clock_)advance_conditions(expected,now);if(actor_.health->current<=0)return {};
        auto proposed=skills::effects::prepare_panic_application(hexes_,caster,hex_scope(),effect,plan,now);if(!proposed)return {};
        hexes_=std::move(proposed->registry);if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return proposed->change;
    }
    // Staged recipient adapter: initial damage is an explicit intent,not
    // applied here. World must clone/preflight all recipients before publishing.
    std::optional<skills::effects::ActionDamageHexApplication> apply_action_damage_hex(const Lifetime& expected,
            skills::effects::Scope caster,const skills::effects::ActionDamageHexPlan& plan,unsigned effect,
            std::uint64_t now,std::optional<skills::effects::HexDuration> adjusted={}){
        const auto canonical=skills::effects::action_damage_hex(plan.skill,plan.rank);
        if(!canonical||*canonical!=plan||!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||
           !actor_.health||actor_.health->current<=0||(condition_clock_&&now<*condition_clock_))return {};
        auto proposed=*this;if(!proposed.condition_clock_)proposed.condition_clock_=now;
        if(!proposed.advance_conditions(expected,now)||!proposed.actor_.health||proposed.actor_.health->current<=0)return {};
        combat::weapon::AttackActionFacts facts;
        if(const auto action=proposed.active_action(expected,now)){
            facts.kind=action->kind==control::Action::attack?combat::weapon::AttackTargetAction::attack:combat::weapon::AttackTargetAction::skill;
            facts.skill=action->skill;
        }
        auto application=skills::effects::prepare_action_damage_hex_application(proposed.hexes_,caster,proposed.hex_scope(),effect,plan,facts,now,adjusted);
        if(!application)return {};
        proposed.hexes_=application->registry;
        proposed.actor_.effects=proposed.hexes_.presentation(proposed.hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return application;
    }
    std::optional<skills::effects::Applied> apply_single_degeneration_hex(const Lifetime& expected,
            const skills::effects::DegenerationHexPlan& plan,unsigned cause,unsigned effect,std::uint64_t now,std::optional<skills::effects::HexDuration> adjusted={}){
        return apply_single_degeneration_hex_impl(expected,plan,cause,effect,now,adjusted,hostile_to_players(expected));
    }
    // Trusted NPC-to-ally recipient lane. World must prove the source's
    // hostile action/lifetime;the player-facing lane above keeps its guard.
    std::optional<skills::effects::Applied> apply_conjure_phantasm_from_npc(const Lifetime& expected,
            skills::effects::Scope caster,unsigned rank,unsigned effect,std::uint64_t now){
        const auto plan=skills::effects::degeneration_hex(31,rank);
        if(!plan||caster.instance!=hex_scope().instance||!caster.actor||caster.actor>65535||
           !caster.generation||caster.actor==hex_scope().actor)return {};
        return apply_single_degeneration_hex_impl(expected,*plan,caster.actor,effect,now,{},healable_ally(expected));
    }
private:
    std::optional<skills::effects::Applied> apply_single_degeneration_hex_impl(const Lifetime& expected,
            const skills::effects::DegenerationHexPlan& plan,unsigned cause,unsigned effect,std::uint64_t now,
            std::optional<skills::effects::HexDuration> adjusted,bool hostile){
        const auto duration=adjusted?adjusted->for_hex(plan.skill,plan.duration,cause):std::optional(plan.duration);
        if(!duration)return {};
        // Linked Life Siphon benefits are committed by the world alongside this
        // target state. The world selects Suffering's area recipients; this
        // applies one staged recipient. End-effect hexes need separate handling.
        if((plan.skill!=883&&plan.skill!=137&&plan.skill!=123&&plan.skill!=150&&plan.skill!=101&&plan.skill!=31&&plan.skill!=35&&plan.skill!=56&&plan.skill!=235&&plan.skill!=36&&plan.skill!=53&&plan.skill!=3183&&plan.skill!=859&&plan.skill!=109&&plan.skill!=108&&plan.skill!=129&&plan.skill!=142&&plan.skill!=785&&plan.skill!=2058)||!skills::effects::valid_degeneration_hex(plan)||
           !matches(expected)||!hostile||!actor_.damageable||!actor_.health||actor_.health->current<=0||
           now>UINT64_MAX-*duration||(condition_clock_&&now<*condition_clock_))return {};
        if(condition_clock_)advance_conditions(expected,now);
        if(actor_.health->current<=0)return {};
        const auto applied=plan.skill==137?skills::effects::apply_rigor_mortis(hexes_,hex_scope(),effect,cause,*skills::effects::rigor_mortis(plan.rank),now).value_or(skills::effects::Applied{}):hexes_.apply(hex_scope(),{effect,cause,plan.skill,plan.rank,skills::effects::Category::hex,now,now+*duration},skills::effects::Replace::same_skill);
        if(!applied.accepted)return {};if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return applied;
    }
public:
    // Trusted conditional followup: World owns the pre-hit condition snapshot,
    // caster/target scopes, range and initial damage transaction.
    std::optional<skills::effects::Applied> apply_vile_miasma(const Lifetime& expected,
            const skills::effects::ConditionDamagePlan& plan,unsigned cause,unsigned effect,std::uint64_t now){
        if(plan.skill!=828||!skills::effects::valid_condition_damage(plan)||!matches(expected)||
           !hostile_to_players(expected)||!cause||cause>65535||!effect||now>UINT64_MAX-plan.duration||
           !actor_.damageable||!actor_.health||actor_.health->current<=0||(condition_clock_&&now<*condition_clock_))return {};
        auto proposed=*this;if(proposed.condition_clock_)proposed.advance_conditions(expected,now);
        if(proposed.actor_.health->current<=0)return {};
        const auto applied=proposed.hexes_.apply(proposed.hex_scope(),{effect,cause,828,plan.rank,
            skills::effects::Category::hex,now,now+plan.duration},skills::effects::Replace::same_skill);
        if(!applied.accepted)return {};if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(proposed.hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return applied;
    }
    std::optional<skills::effects::Applied> apply_lightning_strike_hex(const Lifetime& expected,unsigned rank,unsigned cause,unsigned effect,std::uint64_t now){
        const auto plan=skills::effects::lightning_strike(rank);
        if(!plan||!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||actor_.health->current<=0||
           now>UINT64_MAX-plan->duration||(condition_clock_&&now<*condition_clock_))return {};
        advance_conditions(expected,now);if(actor_.health->current<=0)return {};
        const auto applied=hexes_.apply(hex_scope(),{effect,cause,222,rank,skills::effects::Category::hex,now,now+plan->duration},skills::effects::Replace::same_skill);
        if(!applied.accepted)return {};if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return applied;
    }
    std::optional<skills::effects::Applied> apply_lightning_surge_hex(const Lifetime& expected,unsigned rank,unsigned cause,unsigned effect,std::uint64_t now){
        const auto plan=skills::effects::lightning_surge(rank);
        if(!plan||!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||actor_.health->current<=0||
           now>UINT64_MAX-plan->duration||(condition_clock_&&now<*condition_clock_))return {};
        advance_conditions(expected,now);if(actor_.health->current<=0)return {};
        const auto applied=hexes_.apply(hex_scope(),{effect,cause,205,rank,skills::effects::Category::hex,now,now+plan->duration},skills::effects::Replace::same_skill);
        if(!applied.accepted)return {};if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return applied;
    }
    bool apply_movement_hex(const Lifetime& expected,const skills::effects::MovementHexPlan& plan,unsigned cause,unsigned effect,std::uint64_t now,std::optional<skills::effects::HexDuration> adjusted={}){
        const auto duration=adjusted?adjusted->for_hex(plan.skill,plan.duration,cause):std::optional(plan.duration);
        if(!duration)return false;
        if(!skills::effects::valid_movement_hex(plan)||!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||actor_.health->current<=0||
           now>UINT64_MAX-*duration||(condition_clock_&&now<*condition_clock_))return false;
        advance_conditions(expected,now);if(actor_.health->current<=0)return false;
        const skills::effects::Entry entry{effect,cause,plan.skill,plan.rank,skills::effects::Category::hex,now,now+*duration};
        if(!hexes_.apply(hex_scope(),entry,skills::effects::Replace::same_skill).accepted)return false;
        movement_.apply(entry);if(plan.pips&&!condition_clock_)condition_clock_=now;actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return true;
    }
    // Trusted recipient application. World owns allegiance/range and initial
    // condition removal;this method owns registry replacement and motion time.
    std::optional<skills::effects::Applied> apply_charge(const Lifetime& expected,skills::effects::Scope caster,
            const skills::effects::ChargePlan& plan,unsigned effect,std::uint64_t now){
        const auto entry=skills::effects::prepare_charge(plan,caster,hex_scope(),effect,now);
        if(!matches(expected)||!entry||relation_==Relation::unknown||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||hexes_.lookup(hex_scope(),effect))return {};
        auto proposed=*this;if(proposed.condition_clock_)proposed.advance_conditions(expected,now);
        if(proposed.actor_.health->current<=0)return {};
        const auto change=proposed.hexes_.apply(proposed.hex_scope(),*entry,skills::effects::Replace::same_skill);if(!change.accepted)return {};
        for(const auto& old:change.removed)proposed.movement_.remove(old.instance,now);
        proposed.movement_.apply(*entry);if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(proposed.hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return change;
    }
    float movement_multiplier(const Lifetime& expected,std::uint64_t now)const{return matches(expected)?movement_.at(now):1.f;}
    float movement_multiplier_with_external(const Lifetime& expected,std::uint64_t now,float external)const{
        return matches(expected)?movement_.at_with_external(now,external):0.f;
    }
    // Read-only retained movement history, including spellbook aftercast and
    // knockdown. Future forecast does not manufacture completion of an open cast.
    std::optional<double> historical_movement_rate(const Lifetime& expected,std::uint64_t at,float external=1.f)const{
        if(!matches(expected))return {};return control_.historical_movement_rate(movement_,at,external);
    }
    std::optional<std::vector<std::uint64_t>> historical_movement_boundaries(const Lifetime& expected,std::uint64_t from,std::uint64_t through)const{
        if(!matches(expected))return {};return control_.historical_movement_boundaries(movement_,from,through);
    }
    std::optional<double> activation_multiplier(const Lifetime& expected,unsigned skill,std::uint64_t now)const{
        const auto* row=skills::find_skill(skill);
        if(!row||!matches(expected)||!actor_.health||actor_.health->current<=0||(condition_clock_&&now<*condition_clock_))return {};
        return skills::effects::activation_multiplier(*row,hexes_.active(hex_scope(),now),conditions_,now);
    }
    double movement_integral(const Lifetime& expected,std::uint64_t from,std::uint64_t until)const{
        if(!matches(expected))return 0;
        if(spellbook_)from=spellbook_->movement_begin(from,until);
        return control_.movement_integral(movement_,from,until);
    }
    std::vector<std::uint64_t> movement_boundaries(const Lifetime& expected,std::uint64_t from,std::uint64_t until)const{
        if(!matches(expected)||from>until)return {};
        auto result=control_.movement_boundaries(movement_,from,until);
        if(spellbook_){const auto begins=spellbook_->movement_begin(from,until);if(begins>from&&begins<until)result.push_back(begins);}
        std::sort(result.begin(),result.end());result.erase(std::unique(result.begin(),result.end()),result.end());return result;
    }
    void consume_movement(const Lifetime& expected,std::uint64_t now){if(matches(expected)){movement_.discard_before(now);control_.consume_movement(now);}}
    std::optional<bool> knockdown_allowed(const Lifetime& expected)const{return matches(expected)?knockdown_allowed_:std::nullopt;}
    // Timed admission and final control share the same environmental query.
    // Known protection may resolve unknown base susceptibility to immunity;
    // leaving that protection never manufactures a susceptible base fact.
    std::optional<bool> knockdown_allowed(const Lifetime& expected,std::uint64_t now)const{
        if(!matches(expected))return {};
        if(skills::effects::dont_trip_active(hexes_,hex_scope(),now)||
           (environmental_knockdown_protection_&&environmental_knockdown_protection_(hex_scope(),now)))return false;
        return knockdown_allowed_;
    }
    bool knocked_down(const Lifetime& expected,std::uint64_t now)const{return matches(expected)&&control_.down(now);}
    std::optional<control::Token> active_action(const Lifetime& expected,std::uint64_t now)const{
        return matches(expected)&&actor_.health&&actor_.health->current>0&&(!condition_clock_||now>=*condition_clock_)?control_.active(now):std::nullopt;
    }
    bool attacking(const Lifetime& expected,std::uint64_t now)const{return matches(expected)&&actor_.health&&actor_.health->current>0&&control_.attacking(now);}
    std::optional<std::uint64_t> control_epoch(const Lifetime& expected)const{return matches(expected)&&!control_.exhausted()?std::optional(control_.epoch()):std::nullopt;}
    std::optional<control::Token> begin_action(const Lifetime& expected,control::Action kind,std::uint64_t now,std::uint64_t completes,std::optional<skills::Slot> skill={}){
        if(!matches(expected)||!actor_.health||actor_.health->current<=0||(condition_clock_&&now<*condition_clock_))return {};
        if(skill){const auto* row=skills::find_skill(skill->skill);
            if(row&&skills::effects::control_skill_forbidden(*row,hexes_.active(hex_scope(),now),now))return {};}
        if(kind==control::Action::attack&&!ordinary_action_ready(expected,now))return {};
        if((kind==control::Action::attack||skill)&&hexes_.newest_skill(hex_scope(),886,now)){
            auto proposed=*this;
            const auto token=proposed.control_.begin(hex_scope(),kind,now,completes,skill);
            if(!token)return {};
            const auto restful=proposed.hexes_.newest_skill(hex_scope(),886,now);
            if(!restful||!proposed.remove_reviewed_enchantment(expected,restful->instance,now))return {};
            if(kind==control::Action::attack)proposed.react_attack_hexes(now);
            *this=std::move(proposed);return token;
        }
        const auto token=control_.begin(hex_scope(),kind,now,completes,skill);
        if(token&&kind==control::Action::attack)react_attack_hexes(now);
        return token;
    }
    bool ordinary_action_ready(const Lifetime& expected,std::uint64_t now)const{
        return matches(expected)&&actor_.health&&actor_.health->current>0&&
            (!condition_clock_||now>=*condition_clock_)&&(!spellbook_||spellbook_->action_ready(now));
    }
    bool observe_overcast(const Lifetime& expected,unsigned amount,std::uint64_t now,std::string evidence){
        if(!matches(expected)||amount>1000000||evidence.empty()||evidence.size()>1024||
           (overcast_observation_&&now<overcast_observation_->at))return false;
        overcast_observation_=ai::offensive::OvercastObservation{hex_scope(),amount,now,std::move(evidence)};return true;
    }
    bool known_zero_overcast(const Lifetime& expected,std::uint64_t now)const{
        return matches(expected)&&ai::offensive::known_zero(overcast_observation_,hex_scope(),now);
    }
    std::optional<unsigned> observed_overcast(const Lifetime& expected,std::uint64_t now)const{
        if(!matches(expected)||!overcast_observation_||overcast_observation_->source!=hex_scope()||overcast_observation_->at>now)return {};
        return overcast_observation_->amount;
    }
    std::optional<skills::effects::Entry> ice_spear_regeneration(const Lifetime& expected,std::uint64_t now)const{
        return matches(expected)?hexes_.newest_skill(hex_scope(),214,now):std::nullopt;
    }
    // Trusted recipient primitive; World owns caster payment, alliance and range.
    bool apply_protective_spirit(const Lifetime& expected,skills::effects::Scope caster,
            const skills::effects::ProtectiveSpiritPlan& plan,unsigned effect,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr){
        const auto canonical=skills::effects::protective_spirit(plan.rank);
        const auto duration=adjusted?adjusted->for_enchantment(245,plan.duration,caster,now):std::optional(plan.duration);
        if(!matches(expected)||!canonical||*canonical!=plan||!caster.actor||caster.actor>65535||!caster.generation||caster.instance!=hex_scope().instance||!effect||
           !duration||now>UINT64_MAX-*duration||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||hexes_.lookup(hex_scope(),effect))return false;
        auto proposed=*this;if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return false;
        if(proposed.actor_.health->current<=0)return false;
        const skills::effects::Entry entry{effect,caster.actor,245,plan.rank,skills::effects::Category::enchantment,now,now+*duration};
        if(!proposed.hexes_.apply(hex_scope(),entry,skills::effects::Replace::same_skill).accepted)return false;
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return true;
    }
    bool apply_shield_deflection(const Lifetime& expected,skills::effects::Scope caster,
            const skills::effects::ShieldDeflectionPlan& plan,unsigned effect,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr){
        const auto canonical=skills::effects::shield_deflection(plan.rank);
        const auto duration=adjusted?adjusted->for_enchantment(259,plan.duration,caster,now):std::optional(plan.duration);
        if(!matches(expected)||!canonical||*canonical!=plan||!caster.actor||caster.actor>65535||!caster.generation||caster.instance!=hex_scope().instance||!effect||
           !duration||now>UINT64_MAX-*duration||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||hexes_.lookup(hex_scope(),effect))return false;
        auto proposed=*this;if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return false;
        if(proposed.actor_.health->current<=0)return false;
        const skills::effects::Entry entry{effect,caster.actor,259,plan.rank,skills::effects::Category::enchantment,now,now+*duration};
        if(!proposed.hexes_.apply(hex_scope(),entry,skills::effects::Replace::same_skill).accepted)return false;
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return true;
    }
    // Scoped recipient primitive. World must validate the caster, allegiance,
    // range and payment before committing this copied actor.
    bool apply_shield_regeneration(const Lifetime& expected,skills::effects::Scope caster,
            const skills::effects::ShieldRegenerationPlan& plan,unsigned effect,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr){
        const auto canonical=skills::effects::shield_regeneration(plan.rank);
        const auto duration=adjusted?adjusted->for_enchantment(261,plan.duration,caster,now):std::optional(plan.duration);
        if(!matches(expected)||!canonical||*canonical!=plan||!caster.actor||caster.actor>65535||!caster.generation||caster.instance!=hex_scope().instance||!effect||
           !duration||now>UINT64_MAX-*duration||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||hexes_.lookup(hex_scope(),effect))return false;
        auto proposed=*this;if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return false;
        if(proposed.actor_.health->current<=0)return false;
        const skills::effects::Entry entry{effect,caster.actor,261,plan.rank,skills::effects::Category::enchantment,now,now+*duration};
        if(!proposed.hexes_.apply(hex_scope(),entry,skills::effects::Replace::same_skill).accepted)return false;
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return true;
    }
    // Scoped recipient primitive. World must validate the caster, allegiance,
    // range and payment before committing this copied actor.
    bool apply_shield_judgment(const Lifetime& expected,skills::effects::Scope caster,
            const skills::effects::ShieldJudgmentPlan& plan,unsigned effect,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr){
        const auto canonical=skills::effects::shield_judgment(plan.rank);
        const auto duration=adjusted?adjusted->for_enchantment(262,plan.duration,caster,now):std::optional(plan.duration);
        if(!matches(expected)||!canonical||*canonical!=plan||!caster.actor||caster.actor>65535||!caster.generation||caster.instance!=hex_scope().instance||!effect||
           !duration||now>UINT64_MAX-*duration||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||hexes_.lookup(hex_scope(),effect))return false;
        auto proposed=*this;if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return false;
        if(proposed.actor_.health->current<=0)return false;
        const skills::effects::Entry entry{effect,caster.actor,262,plan.rank,skills::effects::Category::enchantment,now,now+*duration};
        if(!proposed.hexes_.apply(hex_scope(),entry,skills::effects::Replace::same_skill).accepted)return false;
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return true;
    }
    bool apply_vigorous_spirit(const Lifetime& expected,skills::effects::Scope caster,
            const skills::effects::VigorousSpiritPlan& plan,unsigned effect,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr){
        const auto canonical=skills::effects::vigorous_spirit(plan.rank);
        const auto duration=adjusted?adjusted->for_enchantment(254,plan.duration,caster,now):std::optional(plan.duration);
        if(!matches(expected)||!canonical||*canonical!=plan||!caster.actor||caster.actor>65535||!caster.generation||caster.instance!=hex_scope().instance||!effect||
           !duration||now>UINT64_MAX-*duration||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||hexes_.lookup(hex_scope(),effect))return false;
        auto proposed=*this;if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return false;
        if(proposed.actor_.health->current<=0)return false;
        const skills::effects::Entry entry{effect,caster.actor,254,plan.rank,skills::effects::Category::enchantment,now,now+*duration};
        if(!proposed.hexes_.apply(hex_scope(),entry,skills::effects::Replace::same_skill).accepted)return false;
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return true;
    }
    bool apply_healing_seed(const Lifetime& expected,skills::effects::Scope caster,
            const skills::effects::HealingSeedPlan& plan,unsigned effect,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr){
        const auto canonical=skills::effects::healing_seed(plan.rank);
        const auto duration=adjusted?adjusted->for_enchantment(274,plan.duration,caster,now):std::optional(plan.duration);
        if(!matches(expected)||caster==hex_scope()||!healable_ally(expected)||!canonical||*canonical!=plan||!caster.actor||caster.actor>65535||!caster.generation||caster.instance!=hex_scope().instance||!effect||
           !duration||now>UINT64_MAX-*duration||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||hexes_.lookup(hex_scope(),effect))return false;
        auto proposed=*this;if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return false;
        if(proposed.actor_.health->current<=0)return false;
        const skills::effects::Entry entry{effect,caster.actor,274,plan.rank,skills::effects::Category::enchantment,now,now+*duration};
        if(!proposed.hexes_.apply(hex_scope(),entry,skills::effects::Replace::same_skill).accepted)return false;
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return true;
    }
    // Scoped recipient primitive. World must validate the caster, allegiance,
    // range and payment before committing this copied actor.
    bool apply_watchful_healing(const Lifetime& expected,const skills::effects::WatchfulHealingPlan& plan,
        unsigned cause,unsigned effect,std::uint64_t now,std::optional<std::uint64_t> adjusted_duration={}){
        const auto canonical=skills::effects::watchful_healing(plan.rank);
        const auto duration=adjusted_duration.value_or(plan.duration);
        if(!matches(expected)||!canonical||*canonical!=plan||!cause||!effect||
           duration<plan.duration||now>UINT64_MAX-duration||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||hexes_.lookup(hex_scope(),effect))return false;
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return false;
        if(proposed.actor_.health->current<=0)return false;
        const skills::effects::Entry entry{effect,cause,1392,plan.rank,skills::effects::Category::enchantment,now,now+duration};
        if(!proposed.hexes_.apply(hex_scope(),entry,skills::effects::Replace::same_skill).accepted)return false;
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return true;
    }
    // Recipient-owned Restful Breeze source. World validates ally allegiance,
    // range, caster rank/payment and generation before committing the copy.
    bool apply_restful_breeze(const Lifetime& expected,skills::effects::Scope caster,
            const skills::regeneration::Plan& plan,unsigned effect,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr){
        const auto canonical=skills::regeneration::reviewed(886,plan.rank);
        const auto duration=adjusted?adjusted->for_enchantment(886,plan.duration,caster,now):std::optional(plan.duration);
        if(!matches(expected)||!canonical||plan.skill!=886||plan.pips!=canonical->pips||
           plan.duration!=canonical->duration||plan.self_only!=canonical->self_only||
           !caster.actor||!caster.generation||caster.instance!=hex_scope().instance||!effect||
           !duration||!*duration||now>UINT64_MAX-*duration||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||hexes_.lookup(hex_scope(),effect))return false;
        auto proposed=*this;if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return false;
        if(proposed.actor_.health->current<=0)return false;
        const skills::effects::Entry entry{effect,caster.actor,886,plan.rank,skills::effects::Category::enchantment,now,now+*duration};
        if(!proposed.hexes_.apply(hex_scope(),entry,skills::effects::Replace::same_skill).accepted)return false;
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return true;
    }
    bool apply_ice_spear_regeneration(const Lifetime& expected,const skills::effects::WaterProjectilePlan& plan,unsigned effect,std::uint64_t now){
        const auto overcast=observed_overcast(expected,now);
        if(!overcast||!*overcast||plan.skill!=214||!skills::effects::valid_water_projectile(plan)||!effect||
           now>UINT64_MAX-plan.overcast_regeneration_ms||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_))return false;
        advance_conditions(expected,now);if(actor_.health->current<=0)return false;
        const skills::effects::Entry entry{effect,expected.actor,214,plan.rank,skills::effects::Category::enchantment,now,now+plan.overcast_regeneration_ms};
        if(!hexes_.apply(hex_scope(),entry,skills::effects::Replace::same_skill).accepted)return false;
        if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return true;
    }
    bool configure_spellbook(const Lifetime& expected,std::vector<ai::ReviewedSpell> spells){
        if(std::any_of(spells.begin(),spells.end(),[](const auto& entry){return entry.kind!=ai::activated::Kind::spell;}))return false;
        return configure_activated_skillbook(expected,std::move(spells));
    }
    bool configure_activated_skillbook(const Lifetime& expected,std::vector<ai::ReviewedSpell> spells){
        if(!matches(expected)||spellbook_)return false;
        try{spellbook_.emplace(std::move(spells),hex_scope());return true;}catch(const std::invalid_argument&){return false;}
    }
    bool configure_friendly_spellbook(const Lifetime& expected,const ai::friendly::HealProfile& profile){
        const auto heal=skills::effects::direct_heal(profile.skill);const auto* row=skills::find_skill(profile.skill);
        if(!matches(expected)||!healable_ally(expected)||!energy_||!energy_pips_||!heal||!row||row->type!=5||
           profile.rank>20||profile.divine>20||profile.copy>65535||profile.evidence.empty())return false;
        if(!configure_spellbook(expected,{{{profile.skill,profile.copy},heal->range,profile.evidence,ai::SpellTargetClass::ally}}))return false;
        friendly_spell_source_=true;return true;
    }
    bool configure_friendly_reversal_spellbook(const Lifetime& expected,std::string evidence){
        if(!matches(expected)||!healable_ally(expected)||!energy_||!energy_pips_||evidence.empty()||evidence.size()>1024)return false;
        if(!configure_spellbook(expected,{{{307,0},1248,std::move(evidence),ai::SpellTargetClass::ally}}))return false;
        friendly_spell_source_=true;return true;
    }
    std::optional<skills::Slot> choose_activated_skill(const Lifetime& expected,const ai::SpellTarget& target,std::uint64_t now,const ai::Spellbook::Availability& external={}){
        if(!matches(expected)||(!hostile_to_players(expected)&&!(friendly_spell_source_&&healable_ally(expected)))||!actor_.health||actor_.health->current<=0||
           !spellbook_||control_.down(now)||control_.active(now)||target.scope.instance!=hex_scope().instance||
           !advance_energy(expected,now))return {};
        const auto choice=spellbook_->choose(target,energy_?std::optional(energy_->current):std::nullopt,now,external);
        if(choice&&target.scope==hex_scope()&&choice->skill!=1392&&choice->skill!=180&&choice->skill!=181&&choice->skill!=2860&&
           !(friendly_spell_source_&&skills::effects::direct_heal(choice->skill)&&skills::effects::direct_heal(choice->skill)->self))return {};
        return choice;
    }
    std::optional<skills::Slot> choose_spell(const Lifetime& expected,const ai::SpellTarget& target,std::uint64_t now,const ai::Spellbook::Availability& external={}){
        const ai::Spellbook::Availability only_spells=[&](skills::Slot slot,std::uint64_t at)->std::optional<std::uint64_t>{
            if(ai::activated::kind(slot.skill)!=ai::activated::Kind::spell)return {};
            return external?external(slot,at):std::optional<std::uint64_t>(0);
        };
        return choose_activated_skill(expected,target,now,only_spells);
    }
    std::optional<control::Token> begin_spell(const Lifetime& expected,skills::Slot skill,const ai::SpellTarget& target,std::uint64_t now,const ai::Spellbook::Availability& external={}){
        if(ai::activated::kind(skill.skill)!=ai::activated::Kind::spell)return {};
        return begin_activated_skill(expected,skill,target,now,external);
    }
    std::optional<control::Token> begin_activated_skill(const Lifetime& expected,skills::Slot skill,const ai::SpellTarget& target,std::uint64_t now,const ai::Spellbook::Availability& external={}){
        if(!matches(expected))return {};
        if(const auto* row=skills::find_skill(skill.skill);row&&
           skills::effects::control_skill_forbidden(*row,hexes_.active(hex_scope(),now),now))return {};
        if(!control_.can_begin_movement_pause(hex_scope(),now))return {};
        const auto choice=choose_activated_skill(expected,target,now,external);
        if(!choice||!spellbook_->eligible(skill,target,energy_->current,now,external))return {};
        const auto multiplier=activation_multiplier(expected,skill.skill,now);const auto* row=skills::find_skill(skill.skill);
        if(!multiplier||!std::isfinite(*multiplier)||*multiplier<=0)return {};
        const auto duration=std::ceil(row->activation*1000* *multiplier);
        if(!std::isfinite(duration)||duration<=0||duration>60000||now>UINT64_MAX-static_cast<std::uint64_t>(duration))return {};
        const auto kind=ai::activated::kind(skill.skill);if(!kind)return {};
        const auto token=control_.begin(hex_scope(),*kind==ai::activated::Kind::spell?control::Action::spell:control::Action::skill,
            now,now+static_cast<std::uint64_t>(duration),skill);
        if(!token)return {};
        if(!spellbook_->begin(*token)||!lose_energy(expected,row->energy_cost())||!control_.begin_movement_pause(hex_scope(),now))throw std::logic_error("preflighted NPC spell admission failed");
        if(const auto restful=hexes_.newest_skill(hex_scope(),886,now))
            if(!remove_reviewed_enchantment(expected,restful->instance,now))
                throw std::logic_error("accepted NPC spell could not end Restful Breeze");
        return token;
    }
    bool fail_target_action(const Lifetime& expected,const control::Token& token,std::uint64_t now){
        if(!matches(expected)||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_)||
           token.kind!=control::Action::spell||!token.skill||(token.skill->skill!=31&&token.skill->skill!=191&&token.skill->skill!=194&&token.skill->skill!=214&&token.skill->skill!=1526&&token.skill->skill!=307&&!skills::effects::direct_heal(token.skill->skill))||
           !spellbook_||!spellbook_->tracks(token)||!control_.complete(token,now))return false;
        // Paid cost remains spent. External disables are independent authority.
        finish_spellbook_movement(now,false,false);return true;
    }
    bool fail_activated_skill_target_action(const Lifetime& expected,const control::Token& token,std::uint64_t now){
        if(!matches(expected)||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_)||
           !token.skill||!spellbook_||!spellbook_->tracks(token))return false;
        const auto kind=ai::activated::kind(token.skill->skill);
        if(!kind||(*kind==ai::activated::Kind::spell?token.kind!=control::Action::spell:token.kind!=control::Action::skill)||
           !control_.complete(token,now))return false;
        // Invalid target/geometry consumes the paid action, not another cost.
        // Match existing failed-target semantics: no base recharge/aftercast.
        finish_spellbook_movement(now,false,false);return true;
    }
    bool complete_action(const Lifetime& expected,const control::Token& token,std::uint64_t now){
        if(!matches(expected)||!actor_.health||actor_.health->current<=0||(condition_clock_&&now<*condition_clock_)||!control_.complete(token,now))return false;
        if(spellbook_&&spellbook_->tracks(token))finish_spellbook_movement(now,true,true);
        return true;
    }
    bool owns_spell_action(const Lifetime& expected,const control::Token& token)const{
        return token.kind==control::Action::spell&&owns_activated_skill_action(expected,token);
    }
    bool owns_activated_skill_action(const Lifetime& expected,const control::Token& token)const{
        return matches(expected)&&spellbook_&&spellbook_->tracks(token);
    }
    std::optional<std::uint64_t> spell_recharge_until(const Lifetime& expected,skills::Slot slot,std::uint64_t now)const{
        if(!matches(expected)||!actor_.health||actor_.health->current<=0||!spellbook_||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return {};
        return spellbook_->recharge_until(slot,now);
    }
    bool cancel_action(const Lifetime& expected,std::uint64_t now){
        if(!matches(expected)||!control_.cancel(hex_scope(),now))return false;
        if(spellbook_)finish_spellbook_movement(now,true,false);
        return true;
    }
    bool voluntarily_cancel_spell(const Lifetime& expected,std::uint64_t now){
        if(!matches(expected)||!spellbook_||!spellbook_->pending()||!control_.cancel(hex_scope(),now))return false;
        finish_spellbook_movement(now,false,false);return true;
    }
    control::Knockdown knock_down(const Lifetime& expected,std::uint64_t now,std::uint64_t duration=2000){
        if(!matches(expected)||!actor_.health||actor_.health->current<=0||!actor_.damageable||
           relation_==Relation::protected_ally||(condition_clock_&&now<*condition_clock_))return control::Knockdown::invalid;
        const auto allowed=knockdown_allowed(expected,now);
        const auto result=control_.knock_down(hex_scope(),allowed,now,duration);
        if(result==control::Knockdown::accepted&&spellbook_)finish_spellbook_movement(now,true,false);
        return result;
    }
    std::optional<std::uint64_t> attack_duration(const Lifetime& expected,std::uint64_t base,std::uint64_t now)const{
        if(!matches(expected))return {};
        const auto effects=hexes_.active(hex_scope(),now);
        return skills::effects::reviewed_attack_duration(base,skills::effects::hekets_rampage_active(effects,now),
            skills::effects::faintheartedness_active(effects,now));
    }
    std::optional<Hit> empathy_attack(const Lifetime& expected,std::uint64_t event,std::uint64_t now){
        if(!matches(expected)||!event||event<=last_attack_event_||now<last_attack_time_||!actor_.health||actor_.health->current<=0)return {};
        last_attack_event_=event;last_attack_time_=now;
        const auto damage=skills::effects::empathy_attack_reaction(hexes_,hex_scope(),now);
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);
        if(!damage)return {};
        const auto entry=hexes_.newest_skill(hex_scope(),26,now,3151);
        return apply_damage_impl(expected,{Kind::armor_ignoring,double(*damage),20,entry->skill},now,hostile_to_players(expected));
    }
    std::optional<double> empathy_attack_damage(const Lifetime& expected,double damage,std::uint64_t now)const{
        if(!matches(expected))return {};
        return skills::effects::empathy_attack_damage(hexes_,hex_scope(),now,damage);
    }
    std::optional<skills::effects::Entry> mesmer_hex(const Lifetime& expected,std::uint64_t now)const{
        return matches(expected)?hexes_.newest(hex_scope(),skills::effects::Category::hex,now,5):std::nullopt;
    }
    std::optional<skills::effects::Entry> newest_hex(const Lifetime& expected,std::uint64_t now)const{
        return matches(expected)?hexes_.newest(hex_scope(),skills::effects::Category::hex,now):std::nullopt;
    }
    std::optional<skills::effects::Entry> newest_enchantment(const Lifetime& expected,std::uint64_t now)const{
        return matches(expected)?hexes_.newest(hex_scope(),skills::effects::Category::enchantment,now):std::nullopt;
    }
    std::optional<unsigned> enchantment_count(const Lifetime& expected,std::uint64_t now)const{
        if(!matches(expected))return {};
        const auto count=hexes_.count(hex_scope(),skills::effects::Category::enchantment,now);
        // A presentation bit alone cannot prove a count for an unowned effect.
        if(!count&&(actor_.effects&0x80))return {};
        return count;
    }
    std::optional<skills::effects::OwnedWeaponSpell> weapon_spell_snapshot(const Lifetime& expected,std::uint64_t now)const{
        return matches(expected)&&actor_.health&&actor_.health->current>0&&weapon_spell_&&weapon_spell_->active(hexes_,hex_scope(),now)?weapon_spell_:std::nullopt;
    }
    bool apply_weapon_spell(const Lifetime& expected,skills::effects::Scope source,const skills::effects::WeaponSpellPlan&plan,unsigned effect,std::uint64_t now){
        if(!matches(expected)||!skills::effects::valid_weapon_spell(plan)||!effect||!source.actor||source.instance!=hex_scope().instance||!source.generation||
           !actor_.health||actor_.health->current<=0||(condition_clock_&&now<*condition_clock_)||now>UINT64_MAX-plan.duration||hexes_.lookup(hex_scope(),effect))return false;
        auto proposed=*this;if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return false;
        if(proposed.actor_.health->current<=0)return false;
        const skills::effects::Entry entry{effect,source.actor,plan.skill,plan.rank,skills::effects::Category::weapon_spell,now,now+plan.duration};
        if(!proposed.hexes_.apply(hex_scope(),entry,skills::effects::Replace::category).accepted)return false;
        proposed.weapon_spell_=skills::effects::OwnedWeaponSpell{source,hex_scope(),entry,plan};
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);*this=std::move(proposed);return true;
    }
    // A World-locked multi-recipient transaction may prepare armor-ignoring
    // damage without granting this catalog method target-selection authority.
    // The caller must prove hostility and settle all candidate clocks first.
    class PreparedHealthDamage {
        Lifetime lifetime;std::optional<std::uint64_t> clock;
        std::uint64_t loss_revision=0,at=0;Hit hit;deep_wound::State wound_before,wound_after;
        PreparedHealthDamage(Lifetime life,std::optional<std::uint64_t> observed,std::uint64_t revision,std::uint64_t now,Hit result,deep_wound::State before,deep_wound::State after):
            lifetime(life),clock(observed),loss_revision(revision),at(now),hit(result),wound_before(before),wound_after(after){}
        friend class LiveActor;
    public:
        const Hit& result()const{return hit;}
    };
    std::optional<PreparedHealthDamage> prepare_health_damage(const Lifetime& expected,const Damage& damage,std::uint64_t now,bool hostile)const{
        if(!matches(expected)||!hostile||damage.kind!=Kind::armor_ignoring||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return {};
        auto actor=incoming_damage_actor(now);auto hit=apply(actor,damage,true);auto wound=wound_;
        if(hit.reason==Reason::accepted&&!settle_damage_health(hit,wound))return {};
        if(hit.reason!=Reason::accepted||(hit.after.current<hit.before.current&&health_loss_revision_==UINT64_MAX))return {};
        return PreparedHealthDamage{expected,condition_clock_,health_loss_revision_,now,hit,wound_,wound};
    }
    bool can_commit_health_damage(const PreparedHealthDamage& prepared)const{
        const auto&hit=prepared.hit;const auto& before=prepared.wound_before;
        const bool same_capacity=wound_.base==before.base&&wound_.reduction==before.reduction&&
            wound_.debt==before.debt&&wound_.active==before.active;
        return same_capacity&&matches(prepared.lifetime)&&condition_clock_==prepared.clock&&health_loss_revision_==prepared.loss_revision&&
            actor_.health&&actor_.health->current==hit.before.current&&actor_.health->maximum==hit.before.maximum&&
            hit.reason==Reason::accepted&&hit.target==actor_.id&&hit.generation==actor_.generation&&hit.kind==Kind::armor_ignoring&&
            valid_damage_health(hit)&&hit.after.current<=hit.before.current&&
            (!condition_clock_||*condition_clock_<=prepared.at)&&(!energy_clock_||*energy_clock_<=prepared.at)&&
            (hit.after.current==hit.before.current||health_loss_revision_!=UINT64_MAX);
    }
    bool commit_health_damage(const PreparedHealthDamage& prepared){
        if(!can_commit_health_damage(prepared))return false;
        // No allocation or observer callbacks between preflight and this commit.
        wound_=prepared.wound_after;assign_health(prepared.hit.after);if(prepared.hit.killed){actor_.effects|=0x10u;control_.invalidate();}
        return true;
    }
    std::optional<skills::effects::Entry> retire_weapon_spell(const Lifetime&expected,std::uint64_t now,bool force=false){
        if(!matches(expected)||!weapon_spell_||(condition_clock_&&now<*condition_clock_))return {};
        const auto entry=weapon_spell_->entry;
        if(!force&&actor_.health&&actor_.health->current>0&&entry.expires&&now<*entry.expires&&hexes_.lookup(hex_scope(),entry.instance))return {};
        hexes_.remove(hex_scope(),entry.instance);weapon_spell_.reset();actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return entry;
    }
    // Caller holds the World lock across preparation, final preflight and
    // commit. Only the Registry/weapon-spell/presentation stores are exchanged;
    // an unrelated actor's Health, control and resource state is never copied.
    class PreparedWeaponSpellRemoval {
        Lifetime lifetime;skills::effects::OwnedWeaponSpell before;skills::effects::Registry ended;
        std::optional<std::uint64_t> clock;unsigned presentation=0;
        PreparedWeaponSpellRemoval(Lifetime life,skills::effects::OwnedWeaponSpell spell,skills::effects::Registry registry,
            std::optional<std::uint64_t> observed,unsigned effects):lifetime(life),before(spell),ended(std::move(registry)),clock(observed),presentation(effects){}
        friend class LiveActor;
    public:
        unsigned effects()const{return presentation;}
    };
    std::optional<PreparedWeaponSpellRemoval> prepare_weapon_spell_removal(const Lifetime&expected,unsigned effect,std::uint64_t now)const{
        if(!matches(expected)||!weapon_spell_||weapon_spell_->entry.instance!=effect||!weapon_spell_->active(hexes_,hex_scope(),now)||
           (condition_clock_&&now<*condition_clock_))return {};
        auto registry=hexes_;if(!registry.remove(hex_scope(),effect))return {};
        const auto presentation=registry.presentation(hex_scope(),actor_.effects,now);
        return PreparedWeaponSpellRemoval{expected,*weapon_spell_,std::move(registry),condition_clock_,presentation};
    }
    bool can_commit_weapon_spell_removal(const PreparedWeaponSpellRemoval&prepared)const noexcept{
        if(!matches(prepared.lifetime)||condition_clock_!=prepared.clock||!weapon_spell_||weapon_spell_->source!=prepared.before.source||
           weapon_spell_->target!=prepared.before.target||weapon_spell_->plan!=prepared.before.plan)return false;
        const auto&a=weapon_spell_->entry;const auto&b=prepared.before.entry;
        return a.instance==b.instance&&a.cause==b.cause&&a.skill==b.skill&&a.rank==b.rank&&a.kind==b.kind&&a.applied==b.applied&&a.expires==b.expires&&
            hexes_.lookup(hex_scope(),a.instance).has_value();
    }
    bool commit_weapon_spell_removal(PreparedWeaponSpellRemoval&prepared)noexcept{
        if(!can_commit_weapon_spell_removal(prepared))return false;
        hexes_.swap(prepared.ended);weapon_spell_.reset();actor_.effects=prepared.presentation;return true;
    }
    bool end_weapon_spell_on_attack(const Lifetime& expected,std::uint64_t now){
        const auto spell=weapon_spell_snapshot(expected,now);if(!spell||!spell->plan.ends_on_attack)return true;
        if(condition_clock_&&!advance_conditions(expected,now))return false;
        if(!hexes_.remove(hex_scope(),spell->entry.instance))return false;
        weapon_spell_.reset();actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return true;
    }
    // Trusted host effect installation; skill casting remains unbound.
    bool apply_frigid_armor(const Lifetime& expected,const skills::effects::FrigidArmorPlan& plan,
            unsigned effect,std::uint64_t now){
        if(!matches(expected)||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return false;
        const auto applied=skills::effects::prepare_frigid_armor(hexes_,hex_scope(),effect,plan,now);
        if(!applied)return false;
        auto proposed=*this;
        if(proposed.condition_clock_)proposed.advance_conditions(expected,now);
        if(!proposed.actor_.health||proposed.actor_.health->current<=0)return false;
        proposed.hexes_=applied->registry;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return true;
    }
    // Trusted staging only; paid1399 and World rebase consumers remain gated.
    bool stage_shield_absorption(const Lifetime& expected,unsigned rank,unsigned cause,unsigned effect,std::uint64_t now){
        const auto plan=skills::effects::shield_absorption(rank);
        if(!matches(expected)||!plan||!cause||cause>65535||!effect||!actor_.health||actor_.health->current<=0||
           now>UINT64_MAX-plan->duration||(condition_clock_&&now<*condition_clock_))return false;
        const auto result=hexes_.apply(hex_scope(),{effect,cause,1399,rank,skills::effects::Category::enchantment,now,now+plan->duration},skills::effects::Replace::same_skill);
        if(!result.accepted)return false;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return true;
    }
    bool apply_stoneflesh_aura(const Lifetime& expected,const skills::effects::StonefleshAuraPlan& plan,
            unsigned effect,std::uint64_t now){
        if(!matches(expected)||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return false;
        const auto applied=skills::effects::prepare_stoneflesh_aura(hexes_,hex_scope(),effect,plan,now);
        if(!applied)return false;
        auto proposed=*this;
        if(proposed.condition_clock_)proposed.advance_conditions(expected,now);
        if(!proposed.actor_.health||proposed.actor_.health->current<=0)return false;
        proposed.hexes_=applied->registry;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return true;
    }
    bool protect_stoneflesh_critical(const Lifetime& expected,weapon::Strike& strike,std::uint64_t now)const{
        if(!matches(expected))return false;
        if(const auto plan=skills::effects::active_stoneflesh_aura(hexes_,hex_scope(),now))
            return skills::effects::apply_stoneflesh_critical(strike,*plan);
        return true;
    }
    bool apply_ether_renewal(const Lifetime& expected,const skills::effects::EtherRenewalPlan& plan,
            unsigned effect,std::uint64_t now){
        if(!matches(expected)||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return false;
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return false;
        if(!proposed.actor_.health||proposed.actor_.health->current<=0)return false;
        const auto applied=skills::effects::prepare_ether_renewal(proposed.hexes_,hex_scope(),effect,plan,now);
        if(!applied)return false;
        proposed.hexes_=applied->registry;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return true;
    }
    struct EtherRenewalCompletion {
        skills::effects::EtherRenewalReward reward;
        Health before,after;Energy energy_before,energy_after;
    };
    // Explicit terminal primitive. Existing World producers remain unchanged
    // until they publish this receipt and commit the spell's own effect in the
    // correct phase. Speculative copies never mutate the live actor.
    std::optional<EtherRenewalCompletion> complete_ether_renewal_action(
            const Lifetime& expected,const control::Token& token,std::uint64_t now){
        if(!matches(expected)||token.actor!=hex_scope()||token.kind!=control::Action::spell||
           !token.skill||!token.sequence||now<token.completes||!healing_allowed_||
           !energy_||!energy_pips_||energy_zero_||(condition_clock_&&now<*condition_clock_)||
           (energy_clock_&&now<*energy_clock_))return {};
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return {};
        if(!proposed.settle_energy_clock(expected,now)||proposed.energy_zero_||
           !proposed.actor_.health||proposed.actor_.health->current<=0)return {};
        const auto reward=skills::effects::ether_renewal_reward(proposed.hexes_,hex_scope(),token.skill->skill,now);
        if(!reward||!proposed.complete_action(expected,token,now))return {};
        const auto before=*proposed.actor_.health;const auto energy_before=*proposed.energy_;
        if(*proposed.healing_allowed_&&!proposed.heal(expected,reward->healing,now))return {};
        proposed.energy_->current+=std::min(reward->energy,proposed.energy_->maximum-proposed.energy_->current);
        if(proposed.energy_->current==proposed.energy_->maximum)proposed.energy_regeneration_.reset();
        const EtherRenewalCompletion result{*reward,before,*proposed.actor_.health,energy_before,*proposed.energy_};
        *this=std::move(proposed);return result;
    }
    std::optional<skills::effects::Registry> owned_effect_snapshot(const Lifetime& expected)const{
        if(!matches(expected))return {};return hexes_;
    }
    std::optional<skills::effects::Entry> newest_stance(const Lifetime& expected,std::uint64_t now)const{
        return matches(expected)?hexes_.newest(hex_scope(),skills::effects::Category::stance,now):std::nullopt;
    }
    // Explicit NPC controller action. The caller supplies reviewed skill
    // ownership and a real release; this actor owns Energy and recharge.
    bool activate_hekets_rampage(const Lifetime& expected,unsigned rank,unsigned effect,std::uint64_t now){
        const auto plan=skills::effects::hekets_rampage(rank);
        if(!plan||!matches(expected)||relation_==Relation::unknown||!effect||!energy_||!energy_pips_||
           !actor_.health||actor_.health->current<=0||control_.down(now)||now<hekets_rampage_ready_||
           now>UINT64_MAX-std::max(plan->duration,std::uint64_t(10000))||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return false;
        auto proposed=*this;proposed.advance_conditions(expected,now);
        if(!proposed.actor_.health||proposed.actor_.health->current<=0||!proposed.advance_energy(expected,now)||
           proposed.energy_->current<5)return false;
        const skills::effects::Entry entry{effect,expected.actor,1728,rank,skills::effects::Category::stance,now,now+plan->duration};
        const auto applied=proposed.hexes_.apply(hex_scope(),entry,skills::effects::Replace::category);
        if(!applied.accepted)return false;
        for(const auto& old:applied.removed)proposed.movement_.remove(old.instance,now);
        proposed.energy_->current-=5;proposed.hekets_rampage_ready_=now+10000;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return true;
    }
    // Trusted host action, not a client request or an inferred NPC loadout.
    // Native base10Energy is explicit here; profession/cost modifiers need a
    // reviewed NPC profile before a controller chooses this action.
    bool activate_storm_chaser(const Lifetime& expected,unsigned rank,unsigned effect,std::uint64_t now){
        const auto plan=skills::effects::storm_chaser(rank);
        if(!plan||!matches(expected)||relation_==Relation::unknown||!effect||!energy_||!energy_pips_||
           !actor_.health||actor_.health->current<=0||control_.down(now)||now<storm_chaser_ready_||
           now>UINT64_MAX-std::max(plan->duration,std::uint64_t(20000))||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return false;
        auto proposed=*this;proposed.advance_conditions(expected,now);
        if(!proposed.actor_.health||proposed.actor_.health->current<=0||!proposed.advance_energy(expected,now)||proposed.energy_->current<10)return false;
        const skills::effects::Entry entry{effect,expected.actor,455,rank,skills::effects::Category::stance,now,now+plan->duration};
        if(proposed.hexes_.lookup(hex_scope(),effect))return false;
        const auto applied=proposed.hexes_.apply(hex_scope(),entry,skills::effects::Replace::category);if(!applied.accepted)return false;
        for(const auto& old:applied.removed)proposed.movement_.remove(old.instance,now);
        proposed.movement_.apply(entry);proposed.energy_->current-=10;proposed.storm_chaser_ready_=now+20000;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return true;
    }
    bool apply_interrupt_stance(const Lifetime& expected,unsigned skill,unsigned rank,unsigned effect,
        std::uint64_t now,std::uint64_t duration){
        if(!matches(expected)||(skill!=16&&skill!=17)||rank>20||!effect||!duration||duration>600000||
           now>UINT64_MAX-duration||!actor_.health||actor_.health->current<=0||(condition_clock_&&now<*condition_clock_))return false;
        advance_conditions(expected,now);if(actor_.health->current<=0)return false;
        const skills::effects::Entry entry{effect,expected.actor,skill,rank,skills::effects::Category::stance,now,now+duration};
        const auto applied=hexes_.apply(hex_scope(),entry,skills::effects::Replace::category);if(!applied.accepted)return false;
        for(const auto& old:applied.removed)movement_.remove(old.instance,now);
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return true;
    }
    // Reviewed stance removal for landed stance-ending attacks. Keep exact
    // effect/lifetime identity. World settles navigation under the old stance first.
    bool remove_stance(const Lifetime& expected,unsigned effect,std::uint64_t now){
        if(!matches(expected)||(condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return false;
        const auto stance=newest_stance(expected,now);
        if(!stance||stance->instance!=effect||(stance->skill!=16&&stance->skill!=17&&stance->skill!=455&&stance->skill!=1728))return false;
        if(!hexes_.remove(hex_scope(),effect))return false;

        movement_.remove(effect,now);actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return true;
    }
    bool remove_interrupt_stance(const Lifetime& expected,unsigned effect,std::uint64_t now){
        if(!matches(expected)||(condition_clock_&&now<*condition_clock_))return false;
        const auto stance=newest_stance(expected,now);
        if(!stance||stance->instance!=effect||(stance->skill!=16&&stance->skill!=17))return false;
        if(!hexes_.remove(hex_scope(),effect))return false;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return true;
    }
    std::optional<skills::effects::Entry> hex_effect(const Lifetime& expected,unsigned effect)const{
        return matches(expected)?hexes_.lookup(hex_scope(),effect):std::nullopt;
    }
    std::optional<bool> water_magic_hexed(const Lifetime& expected,std::uint64_t now)const{
        if(!matches(expected)||!actor_.health||actor_.health->current<=0)return {};
        return skills::effects::water_hex(hexes_,hex_scope(),now);
    }
    // Recipient primitive:World owns caster allegiance/range/payment and
    // external end credits. Commit this actor before delivering those credits.
    std::optional<skills::effects::ConvertHexesProposal> apply_convert_hexes(const Lifetime& expected,
            skills::effects::Scope caster,const skills::effects::ConvertHexesPlan& plan,unsigned effect,std::uint64_t now){
        if(!matches(expected)||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||!effect||hexes_.lookup(hex_scope(),effect))return {};
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return {};
        if(proposed.actor_.health->current<=0)return {};
        const auto result=skills::effects::prepare_convert_hexes(plan,caster,hex_scope(),proposed.hexes_,now);if(!result)return {};
        const auto old=proposed.hexes_.newest_skill(hex_scope(),303,now);
        if(old&&!result->necromancer_hexes)return {};
        for(const auto& hex:result->removed)if(!proposed.remove_hex_effect(expected,hex.instance,now))return {};
        if(result->armor){
            if(old&&!proposed.remove_hex_effect(expected,old->instance,now))return {};
            const skills::effects::Entry entry{effect,caster.actor,303,plan.rank,skills::effects::Category::other,now,result->armor_until};
            if(!proposed.hexes_.apply(hex_scope(),entry,skills::effects::Replace::same_skill).accepted)return {};
            proposed.convert_hexes_armor_=skills::effects::ConvertHexesArmor{caster,hex_scope(),effect,plan.rank,result->necromancer_hexes,now,result->armor_until};
            if(!proposed.condition_clock_)proposed.condition_clock_=now;
        }
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return result;
    }
    std::optional<unsigned> convert_hexes_armor(const Lifetime& expected,std::uint64_t now)const{
        if(!matches(expected)||(condition_clock_&&now<*condition_clock_))return {};
        if(!actor_.health||actor_.health->current<=0)return 0;
        return skills::effects::active_convert_hexes_armor(hexes_,hex_scope(),convert_hexes_armor_,now);
    }
    // Trusted recipient adapter. World owns party/range/adrenaline authority.
    std::optional<skills::effects::Applied> apply_watch_yourself(const Lifetime& expected,
            skills::effects::Scope caster,const skills::effects::WatchYourselfPlan& plan,unsigned effect,std::uint64_t now){
        const auto owned=skills::effects::prepare_watch_yourself(plan,caster,hex_scope(),effect,now);
        if(!matches(expected)||!owned||relation_==Relation::unknown||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||hexes_.lookup(hex_scope(),effect))return {};
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return {};
        if(proposed.actor_.health->current<=0)return {};
        skills::effects::Applied changes;
        if(proposed.watch_yourself_){const auto old=proposed.hexes_.lookup(proposed.hex_scope(),proposed.watch_yourself_->effect);
            if(old){changes.removed.push_back(*old);if(!proposed.remove_hex_effect(expected,old->instance,now))return {};}}
        const auto applied=proposed.hexes_.apply(proposed.hex_scope(),{effect,caster.actor,plan.skill,plan.rank,skills::effects::Category::other,now,owned->ends},skills::effects::Replace::same_skill);
        if(!applied.accepted)return {};
        changes.accepted=true;changes.removed.insert(changes.removed.end(),applied.removed.begin(),applied.removed.end());
        proposed.watch_yourself_=*owned;if(!proposed.condition_clock_)proposed.condition_clock_=now;
        *this=std::move(proposed);return changes;
    }
    std::optional<unsigned> watch_yourself_armor(const Lifetime& expected,std::uint64_t now)const{
        if(!matches(expected)||(condition_clock_&&now<*condition_clock_))return {};
        return actor_.health&&actor_.health->current>0&&watch_yourself_?skills::effects::watch_yourself_armor(hexes_,*watch_yourself_,now):0;
    }
    // Trusted recipient adapter; World must establish actual party ownership
    // before passing party_member=true. Nearby allied NPCs alone do not qualify.
    std::optional<skills::effects::Applied> apply_shields_up(const Lifetime& expected,
            skills::effects::Scope caster,const skills::effects::ShieldsUpPlan& plan,
            unsigned effect,std::uint64_t now,bool party_member,float distance){
        return apply_party_defense_shout(expected,caster,plan,effect,now,party_member,distance);
    }
    template<class Plan> std::optional<skills::effects::Applied> apply_party_defense_shout(const Lifetime& expected,
            skills::effects::Scope caster,const Plan& plan,
            unsigned effect,std::uint64_t now,bool party_member,float distance){
        if(!healable_ally(expected)||(condition_clock_&&now<*condition_clock_))return {};
        const auto entry=skills::effects::prepare_party_defense_shout(plan,caster,hex_scope(),
            effect,now,party_member,true,distance);
        if(!entry||hexes_.lookup(hex_scope(),effect))return {};
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return {};
        if(!proposed.healable_ally(expected))return {};
        const auto applied=proposed.hexes_.apply(proposed.hex_scope(),*entry,
            skills::effects::Replace::same_skill);
        if(!applied.accepted)return {};
        proposed.actor_.effects=proposed.hexes_.presentation(
            proposed.hex_scope(),proposed.actor_.effects,now);
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        *this=std::move(proposed);return applied;
    }
    std::optional<unsigned> shields_up_armor(const Lifetime& expected,std::uint64_t now)const{
        if(!matches(expected)||(condition_clock_&&now<*condition_clock_))return {};
        return actor_.health&&actor_.health->current>0?
            skills::effects::shields_up_armor(hexes_,hex_scope(),now,
                skills::effects::ShieldsUpDamage::projectile_attack):0;
    }
    std::optional<skills::effects::Applied> apply_save_yourselves(const Lifetime& expected,
            skills::effects::Scope caster,const skills::effects::SaveYourselvesPlan& plan,
            unsigned effect,std::uint64_t now,bool party_member,float distance){
        if(!healable_ally(expected)||(condition_clock_&&now<*condition_clock_))return {};
        const auto entry=skills::effects::prepare_save_yourselves(plan,caster,hex_scope(),
            effect,now,party_member,true,distance);
        if(!entry||hexes_.lookup(hex_scope(),effect))return {};
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return {};
        if(!proposed.healable_ally(expected))return {};
        const auto applied=proposed.hexes_.apply(proposed.hex_scope(),*entry,
            skills::effects::Replace::same_skill);
        if(!applied.accepted)return {};
        proposed.actor_.effects=proposed.hexes_.presentation(
            proposed.hex_scope(),proposed.actor_.effects,now);
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        *this=std::move(proposed);return applied;
    }
    // Must run after World commits actual damage,not on an uncommitted damage
    // preview. World publishes the returned final-charge end exactly once.
    std::optional<skills::effects::WatchYourselfConsumption> consume_watch_yourself_damage(const Lifetime& expected,
            const skills::effects::WatchYourselfDamage& damage){
        if(!matches(expected)||!watch_yourself_||(condition_clock_&&damage.at<*condition_clock_))return {};
        const auto next=skills::effects::consume_watch_yourself(hexes_,*watch_yourself_,damage);if(!next)return {};
        auto proposed=*this;proposed.watch_yourself_=next->owned;
        if(next->ended&&!proposed.remove_hex_effect(expected,next->owned.effect,damage.at))return {};
        *this=std::move(proposed);return next;
    }
    // World invokes this on a committed hit,never a speculative actor copy.
    // Death owns its own status cleanup rather than a last-charge end.
    std::optional<skills::effects::WatchYourselfConsumption> record_watch_yourself_hit(const Lifetime& expected,
            const Hit& hit,bool armor_respecting,std::uint64_t now){
        if(!matches(expected)||!watch_yourself_||hit.reason!=Reason::accepted||hit.target!=expected.actor||hit.generation!=expected.actor_generation||
           hit.killed||!armor_respecting||hit.applied<=0)return {};
        if(watch_yourself_->last_damage==UINT64_MAX)return {};
        return consume_watch_yourself_damage(expected,{hex_scope(),watch_yourself_->last_damage+1,now,true,true,hit.applied});
    }
    std::vector<EnchantmentEnd> take_enchantment_ends(const Lifetime& expected){
        if(!matches(expected))return {};
        auto result=std::move(enchantment_ends_);enchantment_ends_.clear();return result;
    }
    bool remove_hex_effect(const Lifetime& expected,unsigned effect,std::uint64_t now){
        if(!matches(expected)||!hexes_.lookup(hex_scope(),effect))return false;
        if(actor_.health&&actor_.health->current>0&&energy_zero_)return false;
        now=std::max(now,condition_clock_.value_or(now));
        if(condition_clock_&&!advance_conditions(expected,now))return false;
        if(actor_.health&&actor_.health->current>0&&energy_zero_)return false;
        const auto ending=hexes_.lookup(hex_scope(),effect);
        if(ending&&(ending->skill==119||ending->skill==157||ending->skill==125||ending->skill==140||ending->skill==308)&&energy_curve_){
            if(!settle_energy_clock(expected,now))return false;
            energy_curve_->remove_energy_modifier(hex_scope(),effect,now);
        }
        if(!hexes_.remove(hex_scope(),effect))return false;
        if(ending&&ending->skill==227&&ending->expires&&now<*ending->expires)glimmering_removed_=std::pair{effect,now};
        if(ending&&ending->skill==44)finish_phantom_pain(*ending,now);
        if(ending&&ending->skill==1392){
            const auto before=actor_.health?actor_.health->current:0.;
            const auto gain=skills::effects::watchful_early_gain(*ending,now,actor_.health&&actor_.health->current>0);
            if(gain&&*gain){
                // Health gain bypasses healing penalties, but retains Deep
                // Wound's modified maximum and its underlying Health ledger.
                if(wound_.base)wound_.event(actor_.health->current,double(*gain),false,1.);
                else actor_.health->current=std::min(double(actor_.health->maximum),actor_.health->current+double(*gain));
            }
            // Subtracting fractional Health can round the measured delta one
            // ULP above the requested integer gain. Never publish more than
            // the actual mechanic permits, even in that representation case.
            if(actor_.health)enchantment_ends_.push_back({*ending,now,*actor_.health,
                std::min(double(gain.value_or(0)),std::max(0.,actor_.health->current-before))});
        }
        if(ending&&(ending->skill==245||ending->skill==259||ending->skill==261||ending->skill==262||ending->skill==886||ending->skill==308||ending->skill==248)&&actor_.health)enchantment_ends_.push_back({*ending,now,*actor_.health,0});
        fragility_levels_.erase(effect);attack_hex_levels_.erase(effect);
        movement_.remove(effect,now);
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return true;
    }
    std::optional<std::uint64_t> glimmering_removed(const Lifetime& expected,unsigned effect)const{
        return matches(expected)&&glimmering_removed_&&glimmering_removed_->first==effect?
            std::optional(glimmering_removed_->second):std::nullopt;
    }
    // Only reviewed positive-status end behavior is admitted. Ice Spear's
    // regeneration is queried from its owned entry,so removing it ends gain.
    static bool reviewed_enchantment_end_skill(unsigned skill){
        return skill==180||skill==113||skill==214||skill==119||skill==157||
            skill==1261||skill==1375||skill==181||skill==2860||skill==1392||skill==245||skill==254||skill==259||skill==261||skill==262||skill==886||skill==308||skill==248;
    }
    bool remove_reviewed_enchantment(const Lifetime& expected,unsigned effect,std::uint64_t now){
        if(!matches(expected)||(condition_clock_&&now<*condition_clock_))return false;
        const auto entry=hexes_.lookup(hex_scope(),effect);
        return entry&&entry->kind==skills::effects::Category::enchantment&&reviewed_enchantment_end_skill(entry->skill)&&
            remove_hex_effect(expected,effect,now);
    }
    // World-owned upkeep retirement precedes the same-frontier Energy-zero
    // observer. Preserve its pending receipt; ordinary removals remain gated.
    bool remove_maintained_retribution(const Lifetime& expected,unsigned effect,std::uint64_t now){
        if(!matches(expected)||(condition_clock_&&now<*condition_clock_))return false;
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return false;
        const auto entry=proposed.hexes_.lookup(hex_scope(),effect);if(!entry||entry->skill!=248||entry->expires)return false;
        if(proposed.energy_zero_&&proposed.energy_zero_->at!=now)return false;
        // No248 recipient modifier exists to settle/remove. Do not re-enter
        // generic removal's resource advancement between zero and the owning
        // World's upkeep retirement. Keep the exact pending zero receipt.
        if(!proposed.hexes_.remove(hex_scope(),effect)||!proposed.actor_.health)return false;
        proposed.enchantment_ends_.push_back({*entry,now,*proposed.actor_.health,0});
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return true;
    }
    // Lease the exact application at the ordering boundary selected by the
    // skill. Damage/conversion may consume it before its later removal step.
    struct EnchantmentRemoval {
        Lifetime expected;
        skills::effects::Entry entry;
        std::uint64_t sequence=0;
    };
    enum class EffectRemovalResult {removed,already_absent,replaced};
    using EnchantmentRemovalResult=EffectRemovalResult;
    std::optional<EnchantmentRemoval> prepare_enchantment_removal(
            const Lifetime& expected,unsigned effect,std::uint64_t now)const{
        if(!matches(expected)||(condition_clock_&&now<*condition_clock_))return {};
        const auto entry=hexes_.lookup(hex_scope(),effect);
        const auto sequence=hexes_.entry_sequence(hex_scope(),effect);
        if(!entry||!sequence||entry->kind!=skills::effects::Category::enchantment||
           now<entry->applied||(entry->expires&&now>=*entry->expires)||
           (!reviewed_enchantment_end_skill(entry->skill)&&entry->skill!=1399&&entry->skill!=307))return {};
        return EnchantmentRemoval{expected,*entry,*sequence};
    }
    std::optional<EnchantmentRemovalResult> commit_enchantment_removal(
            const EnchantmentRemoval& removal,std::uint64_t now){
        const auto& expected=removal.expected;const auto& old=removal.entry;
        if(!matches(expected)||!removal.sequence||!old.instance||
           old.kind!=skills::effects::Category::enchantment||now<old.applied||
           (condition_clock_&&now!=*condition_clock_)||
           (!reviewed_enchantment_end_skill(old.skill)&&old.skill!=1399&&old.skill!=307))return {};
        // World settles time first. Removal must never discover natural expiry
        // halfway through validating/committing an exact application.
        const auto current=hexes_.lookup(hex_scope(),old.instance);
        if(!current)return EnchantmentRemovalResult::already_absent;
        if(old.expires&&now>=*old.expires)return {};
        const auto sequence=hexes_.entry_sequence(hex_scope(),old.instance);
        if(sequence!=removal.sequence)return EnchantmentRemovalResult::replaced;
        if(current->cause!=old.cause||current->skill!=old.skill||current->rank!=old.rank||
           current->kind!=old.kind||current->applied!=old.applied||current->expires!=old.expires)return {};
        // Use current end machinery: Watchful Health gain is intentional.
        // Never restore the actor/Health/counter snapshot used during admission.
        if(!remove_hex_effect(expected,old.instance,now))return {};
        return EnchantmentRemovalResult::removed;
    }
    struct HexRemoval {
        Lifetime expected;
        skills::effects::Entry entry;
        std::uint64_t sequence=0;
        bool mesmer_only=false;
    };
    using HexRemovalResult=EffectRemovalResult;
    // World owns selection/range and external end credits. The lease preserves
    // that selection if another hex becomes newest before actual removal.
    std::optional<HexRemoval> prepare_hex_removal(const Lifetime& expected,
            unsigned effect,std::uint64_t now,bool mesmer_only=false)const{
        if(!matches(expected)||(condition_clock_&&now<*condition_clock_))return {};
        const auto entry=hexes_.lookup(hex_scope(),effect);
        const auto sequence=hexes_.entry_sequence(hex_scope(),effect);
        if(!entry||!sequence||entry->kind!=skills::effects::Category::hex||
           now<entry->applied||(entry->expires&&now>=*entry->expires))return {};
        const auto* row=skills::find_skill(entry->skill);
        if(!row||(mesmer_only&&row->profession!=5))return {};
        return HexRemoval{expected,*entry,*sequence,mesmer_only};
    }
    std::optional<HexRemovalResult> commit_hex_removal(const HexRemoval& removal,std::uint64_t now){
        const auto& expected=removal.expected;const auto& old=removal.entry;
        if(!matches(expected)||!removal.sequence||!old.instance||
           old.kind!=skills::effects::Category::hex||now<old.applied||
           (condition_clock_&&now!=*condition_clock_))return {};
        const auto* row=skills::find_skill(old.skill);
        if(!row||(removal.mesmer_only&&row->profession!=5))return {};
        const auto current=hexes_.lookup(hex_scope(),old.instance);
        if(!current)return HexRemovalResult::already_absent;
        if(old.expires&&now>=*old.expires)return {};
        if(hexes_.entry_sequence(hex_scope(),old.instance)!=removal.sequence)return HexRemovalResult::replaced;
        if(current->cause!=old.cause||current->skill!=old.skill||current->rank!=old.rank||
           current->kind!=old.kind||current->applied!=old.applied||current->expires!=old.expires)return {};
        if(!remove_hex_effect(expected,old.instance,now))return {};
        return HexRemovalResult::removed;
    }
    bool remove_mesmer_hex(const Lifetime& expected,unsigned effect,std::uint64_t now){
        if(!matches(expected)||(condition_clock_&&now<*condition_clock_))return false;
        if(actor_.health&&actor_.health->current>0&&energy_zero_)return false;
        const auto selected=mesmer_hex(expected,now);if(!selected||selected->instance!=effect)return false;
        if(condition_clock_&&!advance_conditions(expected,now))return false;
        if(actor_.health&&actor_.health->current>0&&energy_zero_)return false;
        if(!hexes_.remove(hex_scope(),effect))return false;
        fragility_levels_.erase(effect);attack_hex_levels_.erase(effect);
        movement_.remove(effect,now);
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return true;
    }
    bool clear_hexes(const Lifetime& expected,std::uint64_t now){
        if(!matches(expected))return false;
        if(actor_.health&&actor_.health->current>0){
            if(condition_clock_&&!advance_conditions(expected,now))return false;
            if(energy_zero_&&actor_.health->current>0)return false;
        }
        if(const auto mark=hexes_.newest_skill(hex_scope(),227,now))glimmering_removed_=std::pair{mark->instance,now};
        if(energy_curve_)energy_curve_->clear_energy_modifiers(hex_scope(),energy_curve_->cursor());
        energy_zero_.reset();hexes_.clear(hex_scope());fragility_levels_.clear();attack_hex_levels_.clear();movement_.clear(now);actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return true;
    }
    bool refresh_hexes(const Lifetime& expected,std::uint64_t now){
        if(!matches(expected))return false;
        const auto before=actor_.effects;
        if(actor_.health&&actor_.health->current>0&&condition_clock_&&now>*condition_clock_&&
           (energy_curve_||weapon_spell_||hexes_.newest_skill(hex_scope(),44,*condition_clock_)||hexes_.newest_skill(hex_scope(),1392,*condition_clock_)||hexes_.newest_skill(hex_scope(),261,*condition_clock_))&&
           !advance_conditions(expected,now))return false;
        if(actor_.health&&actor_.health->current>0&&energy_zero_&&now>energy_zero_->at)return false;
        if(actor_.health&&actor_.health->current<=0){if(energy_curve_)energy_curve_->clear_energy_modifiers(hex_scope(),energy_curve_->cursor());energy_zero_.reset();hexes_.clear(hex_scope());fragility_levels_.clear();attack_hex_levels_.clear();movement_.clear(now);}
        else for(const auto& expired:hexes_.expire(hex_scope(),now)){fragility_levels_.erase(expired.instance);attack_hex_levels_.erase(expired.instance);}
        if(energy_zero_){const auto active=hexes_.active(hex_scope(),energy_zero_->at);
            if(std::none_of(active.begin(),active.end(),[](const auto& entry){return entry.skill==125||entry.skill==140||entry.skill==49||entry.skill==2734;}))energy_zero_.reset();
        }
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return before!=actor_.effects;
    }
    std::optional<Energy> energy(const Lifetime& expected)const{
        return matches(expected)?energy_:std::nullopt;
    }
    std::optional<std::int64_t> precise_energy_units(const Lifetime& expected)const{
        if(!matches(expected)||!energy_||!energy_pips_)return {};
        const auto fraction=energy_->current==energy_->maximum?0:energy_regeneration_.remainder()*100;
        return std::int64_t(energy_->current)*300000+static_cast<std::int64_t>(fraction);
    }
    bool can_receive_energy_enchantment(const Lifetime& expected,std::uint64_t now)const{
        return matches(expected)&&healable_ally(expected)&&energy_&&energy_pips_&&
            (!condition_clock_||now>=*condition_clock_)&&(!energy_clock_||now>=*energy_clock_);
    }
    bool has_energy_curve(const Lifetime& expected)const{return matches(expected)&&energy_curve_.has_value();}
    bool prepare_maintained_caster(const Lifetime& expected,std::uint64_t now){
        if(!matches(expected)||!actor_.health||actor_.health->current<=0||!energy_||!energy_pips_||energy_zero_||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return false;
        auto proposed=*this;if(!proposed.condition_clock_)proposed.condition_clock_=now;
        if(!proposed.advance_conditions(expected,now)||!proposed.actor_.health||proposed.actor_.health->current<=0||
           !proposed.settle_energy_clock(expected,now))return false;
        if(!proposed.energy_curve_)proposed.energy_curve_.emplace(hex_scope(),now,33);
        *this=std::move(proposed);return true;
    }
    std::optional<unsigned> maintained_upkeep(const Lifetime& expected)const{
        return matches(expected)&&energy_curve_?std::optional(energy_curve_->maintained_upkeep()):std::nullopt;
    }
    bool set_maintained_upkeep(const Lifetime& expected,unsigned count,std::uint64_t now){
        if(!matches(expected)||!energy_curve_||!energy_clock_||*energy_clock_!=now)return false;
        return energy_curve_->set_maintained_upkeep(hex_scope(),count,now);
    }
    std::optional<skills::effects::MaintainedEnergy> maintained_energy(const Lifetime& expected,
            const skills::effects::MaintainedEnchantments& ownership,std::uint64_t now)const{
        if(!matches(expected)||!actor_.health||actor_.health->current<=0||!energy_||!energy_pips_||!energy_curve_||
           !energy_clock_||*energy_clock_!=energy_curve_->cursor())return {};
        const vitals::Resources resources{1,1,energy_->current,energy_->maximum};
        auto proposal=skills::effects::prepare_maintained_energy(ownership,*energy_curve_,hex_scope(),resources,energy_regeneration_,0,now,int(*energy_pips_));
        if(!proposal||proposal->energy.debt||proposal->energy.fractional_tail)return {};return proposal;
    }
    bool commit_maintained_energy(const Lifetime& expected,const overcast::Settlement& state){
        const auto now=state.curve.cursor();
        if(!matches(expected)||!energy_||!energy_pips_||!energy_curve_||!energy_clock_||*energy_clock_!=energy_curve_->cursor()||
           now<*energy_clock_||state.debt||state.fractional_tail||state.resources.max_energy!=energy_->maximum||
           !state.resources.health||state.resources.max_health!=1)return false;
        if(state.first_zero&&!energy_zero_){
            energy_zero_=skills::effects::energy_zero_observation(state,hex_scope(),1);if(!energy_zero_)return false;
            energy_zero_before_ms_=state.first_zero_before_units;
        }
        energy_curve_=state.curve;energy_regeneration_=state.regeneration;energy_->current=state.resources.energy;energy_clock_=now;return true;
    }
    // Recipient staging only: World must atomically own upkeep, the full caster
    // generation and spell-completion monitoring before admitting this effect.
    std::optional<skills::effects::SuccorApplication> apply_succor(const Lifetime& expected,
            skills::effects::Scope caster,unsigned effect,std::uint64_t now){
        if(!can_receive_energy_enchantment(expected,now)||energy_zero_||hexes_.lookup(hex_scope(),effect)||
           !skills::effects::valid_succor_lease({caster,hex_scope(),effect,now}))return {};
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return {};
        if(!proposed.healable_ally(expected)||!proposed.settle_energy_clock(expected,now))return {};
        auto application=skills::effects::prepare_succor(caster,hex_scope(),effect,now,proposed.hexes_);
        if(!application)return {};
        auto curve=proposed.energy_curve_?*proposed.energy_curve_:overcast::CeilingCurve(hex_scope(),now,33);
        if(!curve.apply_succor_modifier(hex_scope(),effect,now))return {};
        proposed.hexes_=application->registry;proposed.energy_curve_=std::move(curve);
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return application;
    }
    // Trusted recipient stage; World validates allegiance/range and atomically
    // adopts the returned external maintainer authority with this actor copy.
    bool can_receive_retribution(const Lifetime& expected,std::uint64_t now)const{
        return matches(expected)&&actor_.health&&actor_.health->current>0&&energy_&&energy_pips_&&
            (!condition_clock_||now>=*condition_clock_)&&(!energy_clock_||now>=*energy_clock_);
    }
    std::optional<skills::effects::RetributionApplication> apply_retribution(const Lifetime& expected,
            skills::effects::Scope caster,const skills::effects::RetributionPlan& plan,unsigned effect,std::uint64_t now,
            const skills::effects::MaintainedEnchantments& ownership){
        if(!can_receive_retribution(expected,now)||energy_zero_||!effect||hexes_.lookup(hex_scope(),effect))return {};
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return {};
        if(!proposed.actor_.health||proposed.actor_.health->current<=0||!proposed.settle_energy_clock(expected,now))return {};
        auto application=skills::effects::prepare_retribution(plan,caster,hex_scope(),effect,now,proposed.hexes_,ownership);
        if(!application)return {};
        // The shared maintained frontier forecasts every NPC participant.
        // Retribution contributes no Energy/Health modifier to its recipient.
        if(!proposed.energy_curve_)proposed.energy_curve_.emplace(hex_scope(),now,33);
        proposed.hexes_=application->registry;
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return application;
    }
    bool advance_energy(const Lifetime& expected,std::uint64_t now){
        if(energy_zero_)return false;
        // Timed Energy owners must settle Health first so a delayed death does
        // not earn regeneration after its crossing. World retains death credit.
        if(energy_curve_&&condition_clock_&&now>*condition_clock_)return false;
        return settle_energy_clock(expected,now);
    }
    std::optional<skills::effects::Applied> apply_tainted_flesh(const Lifetime& expected,
            skills::effects::Scope caster,const skills::effects::TaintedFleshPlan& plan,unsigned effect,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr){
        const auto canonical=skills::effects::tainted_flesh(plan.rank);
        const auto duration=adjusted?adjusted->for_enchantment(113,plan.duration,caster,now):std::optional(plan.duration);
        if(!matches(expected)||!canonical||*canonical!=plan||!healable_ally(expected)||!effect||
           caster.instance!=lifetime_.world||!caster.actor||caster.actor>65535||!caster.generation||
           !duration||now>UINT64_MAX-*duration||(condition_clock_&&now<*condition_clock_))return {};
        advance_conditions(expected,now);if(actor_.health->current<=0)return {};
        auto registry=hexes_;const skills::effects::Entry entry{effect,caster.actor,113,plan.rank,skills::effects::Category::enchantment,now,now+*duration};
        const auto result=registry.apply(hex_scope(),entry,skills::effects::Replace::same_skill);if(!result.accepted)return {};
        hexes_=std::move(registry);
        if(conditions_.has(conditions::Kind::disease,now))remove_condition(expected,conditions::Kind::disease,now);
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return result;
    }
    std::optional<skills::effects::Applied> apply_energy_enchantment(const Lifetime& expected,
            skills::effects::Scope caster,const skills::effects::EnergyEnchantmentPlan& plan,unsigned effect,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr){
        const auto canonical=skills::effects::energy_enchantment(plan.skill,plan.rank);
        const auto duration=adjusted?adjusted->for_enchantment(plan.skill,plan.duration,caster,now):std::optional(plan.duration);
        if(!matches(expected)||!canonical||*canonical!=plan||!healable_ally(expected)||!energy_||!energy_pips_||!effect||energy_zero_||
           caster.instance!=lifetime_.world||!caster.actor||caster.actor>65535||!caster.generation||caster==hex_scope()||
           !duration||now>UINT64_MAX-*duration||(condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return {};
        if(condition_clock_)advance_conditions(expected,now);
        if(actor_.health->current<=0||!settle_energy_clock(expected,now))return {};
        auto registry=hexes_;const skills::effects::Entry entry{effect,caster.actor,plan.skill,plan.rank,skills::effects::Category::enchantment,now,now+*duration};
        const auto result=registry.apply(hex_scope(),entry,skills::effects::Replace::same_skill);if(!result.accepted)return {};
        auto curve=energy_curve_?*energy_curve_:overcast::CeilingCurve(hex_scope(),now,33);
        if(!curve.apply_energy_modifier(hex_scope(),effect,plan.skill,plan.pips,*duration,now))return {};
        hexes_=std::move(registry);energy_curve_=std::move(curve);if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return result;
    }
    // Trusted recipient staging only. World must own the caster link and
    // resolve/acknowledge the shared zero transaction before native admission.
    std::optional<skills::effects::Applied> apply_energy_degeneration_hex(const Lifetime& expected,
            skills::effects::Scope caster,const skills::effects::EnergyDegenerationHexPlan& plan,unsigned effect,std::uint64_t now){
        if(!matches(expected)||!skills::effects::valid_energy_degeneration_hex(plan)||!hostile_to_players(expected)||
           !actor_.damageable||!actor_.health||actor_.health->current<=0||!energy_||!energy_pips_||!effect||energy_zero_||
           caster.instance!=lifetime_.world||!caster.actor||caster.actor>65535||!caster.generation||caster.actor==lifetime_.actor||
           now>UINT64_MAX-plan.duration||(condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return {};
        if(condition_clock_&&!advance_conditions(expected,now))return {};
        if(actor_.health->current<=0||!settle_energy_clock(expected,now))return {};
        auto registry=hexes_;const skills::effects::Entry entry{effect,caster.actor,plan.skill,plan.rank,skills::effects::Category::hex,now,now+plan.duration};
        const auto result=registry.apply(hex_scope(),entry,skills::effects::Replace::same_skill);if(!result.accepted)return {};
        auto curve=energy_curve_.value_or(overcast::CeilingCurve(hex_scope(),now,33));
        if(!curve.apply_energy_modifier(hex_scope(),effect,plan.skill,-int(plan.energy_degen),plan.duration,now))return {};
        hexes_=std::move(registry);energy_curve_=std::move(curve);if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return result;
    }
    std::optional<overcast::Settlement> forecast_energy_zero(const Lifetime& expected,std::uint64_t until)const{
        if(!matches(expected)||!energy_||!energy_pips_||!energy_curve_||!actor_.health||until<energy_curve_->cursor())return {};
        const vitals::Resources resources{actor_.health->current>0?1u:0u,1,energy_->current,energy_->maximum};
        if(energy_zero_){
            auto stopped=overcast::settle(*energy_curve_,hex_scope(),resources,energy_regeneration_,0,energy_zero_->at,int(*energy_pips_));
            if(!stopped)return {};stopped->first_zero=energy_zero_->at;stopped->first_zero_before_units=energy_zero_before_ms_;return stopped;
        }
        return overcast::settle_until_zero(*energy_curve_,hex_scope(),resources,energy_regeneration_,0,until,int(*energy_pips_));
    }
    std::optional<skills::effects::MindWrackTransition> pending_energy_zero(const Lifetime& expected,std::uint64_t event)const{
        if(!matches(expected)||!energy_zero_||!event)return {};
        auto result=*energy_zero_;result.event=event;return result;
    }
    bool retain_discrete_energy_zero(const Lifetime& expected,const skills::effects::MindWrackEnergy& before,
            const skills::effects::MindWrackEnergy& after,std::uint64_t at){
        const skills::effects::MindWrackTransition event{hex_scope(),1,at,before,after};
        if(!matches(expected)||energy_zero_||!skills::effects::mind_wrack_zero_transition(event)||
           !energy_||!energy_curve_||!energy_clock_||*energy_clock_!=at||!condition_clock_||*condition_clock_!=at||
           energy_->current||energy_regeneration_.remainder()||after.maximum!=energy_->maximum||
           !actor_.health||actor_.health->current<=0)return false;
        const auto entries=hexes_.active(hex_scope(),at);
        if(std::none_of(entries.begin(),entries.end(),[](const auto& entry){return entry.skill==125||entry.skill==140;}))return false;
        energy_zero_=event;energy_zero_before_ms_.reset();return true;
    }
    struct EnergyHexZeroEnd {
        std::vector<skills::effects::EnergyHexZeroDamage> damage;
        std::optional<skills::effects::MindWrackReaction> mind_wrack;
        std::vector<skills::effects::Entry> ended;
    };
    // Call on a transaction-local NPC copy. World must validate caster
    // lifetimes and stage linked caster histories before publishing this copy
    // and applying these intents through its existing damage/credit authority.
    std::optional<EnergyHexZeroEnd> commit_energy_hex_zero(const Lifetime& expected,
            std::vector<skills::effects::EnergyHexOwnership> owners,
            std::optional<skills::effects::MindWrackOwnership> mind,
            const skills::effects::MindWrackTransition& event){
        const auto recorded=pending_energy_zero(expected,event.event);
        if(!recorded||recorded->target!=event.target||recorded->at!=event.at||recorded->before!=event.before||recorded->after!=event.after||
           !actor_.health||actor_.health->current<=0||!condition_clock_||*condition_clock_!=event.at||!energy_curve_||energy_curve_->cursor()!=event.at||
           !energy_||energy_->current||energy_regeneration_.remainder()||owners.size()>2)return {};
        unsigned hex_count=0,mind_count=0;
        for(const auto& entry:hexes_.active(hex_scope(),event.at)){
            hex_count+=entry.skill==125||entry.skill==140;
            if(entry.skill==49||entry.skill==2734){++mind_count;if(!mind||mind->effect!=entry.instance)return {};}
        }
        if(hex_count!=owners.size()||mind_count!=unsigned(mind.has_value())||(!hex_count&&!mind_count))return {};
        std::sort(owners.begin(),owners.end(),[](const auto& a,const auto& b){return a.effect<b.effect;});
        if(owners.size()==2&&owners[0].effect==owners[1].effect)return {};
        auto registry=hexes_;auto curve=*energy_curve_;
        auto events=energy_zero_events_.value_or(skills::effects::MindWrackEvents(hex_scope()));
        if(!events.accept(event))return {};EnergyHexZeroEnd result;
        for(const auto& owner:owners){
            if(owner.target!=hex_scope()||!skills::effects::matches_energy_hex(registry,owner)||
               event.at<owner.applied||event.at>=owner.expires)return {};
            const auto plan=*skills::effects::energy_degeneration_hex(owner.skill,owner.rank);
            if(!curve.matches_energy_modifier(hex_scope(),owner.effect,owner.skill,-int(plan.energy_degen),owner.applied,owner.expires))return {};
            const auto ended=registry.remove(hex_scope(),owner.effect);
            if(!ended||!curve.remove_energy_modifier(hex_scope(),owner.effect,event.at))return {};
            result.ended.push_back(*ended);result.damage.push_back({owner,plan.zero_damage});
        }
        if(mind){
            auto reaction=skills::effects::mind_wrack_energy_reaction(registry,*mind,event);
            if(!reaction||!reaction->ends)return {};
            registry=reaction->registry;result.ended.push_back(reaction->hex);result.mind_wrack=std::move(reaction);
        }
        hexes_=std::move(registry);energy_curve_=std::move(curve);energy_zero_events_=std::move(events);energy_zero_.reset();
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,event.at);return result;
    }
    bool lose_energy(const Lifetime& expected,unsigned amount){
        if(energy_zero_||(!hostile_to_players(expected)&&!(friendly_spell_source_&&healable_ally(expected)))||!actor_.health||actor_.health->current<=0||!energy_||amount>energy_->current)return false;
        energy_->current-=amount;return true;
    }
    std::optional<std::uint64_t> drain_energy_units(const Lifetime& expected,unsigned amount,std::uint64_t now){
        if(!hostile_to_players(expected)||!actor_.health||actor_.health->current<=0||!energy_pips_||
           !advance_energy(expected,now))return {};
        const auto current=precise_energy_units(expected);if(!current||*current<0)return {};
        const auto lost=std::min(std::uint64_t(*current),std::uint64_t(amount)*300000);
        const auto after=std::uint64_t(*current)-lost;
        energy_->current=static_cast<unsigned>(after/300000);
        energy_regeneration_.restore_fraction((after%300000)/100);
        return lost;
    }
    bool gain_energy(const Lifetime& expected,unsigned amount){
        if(energy_zero_||!hostile_to_players(expected)||!actor_.health||actor_.health->current<=0||!energy_)return false;
        energy_->current+=std::min(amount,energy_->maximum-energy_->current);
        if(energy_->current==energy_->maximum)energy_regeneration_.reset();
        return true;
    }
    // External capped loss preserves fractions unless the requested amount
    // exhausts the positive pool. No Energy/debt is inferred for unknown NPCs.
    std::optional<std::uint64_t> drain_energy(const Lifetime& expected,unsigned amount){
        if(energy_zero_)return {};
        const auto units=precise_energy_units(expected);
        if(!units||!hostile_to_players(expected)||!actor_.health||actor_.health->current<=0)return {};
        const auto lost=std::min<std::uint64_t>(std::uint64_t(amount)*300000,std::uint64_t(*units));
        if(amount>energy_->current){energy_->current=0;energy_regeneration_.reset();}
        else energy_->current-=amount;
        return lost;
    }
    std::optional<skills::effects::MindWrackReaction> react_mind_wrack_energy(const Lifetime& expected,
            const skills::effects::MindWrackOwnership& owner,const skills::effects::MindWrackTransition& event){
        if(!matches(expected)||hex_scope()!=owner.target||!energy_||event.after.maximum!=energy_->maximum)return {};
        auto result=skills::effects::mind_wrack_energy_reaction(hexes_,owner,event);if(!result)return {};
        hexes_=result->registry;actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,event.at);return result;
    }
    std::optional<skills::effects::MindWrackReaction> react_mind_wrack_target(const Lifetime& expected,
            const skills::effects::MindWrackOwnership& owner,const skills::effects::MindWrackUse& use){
        if(!matches(expected)||hex_scope()!=owner.target||(condition_clock_&&use.at<*condition_clock_)||!advance_energy(expected,use.at)||!actor_.health||actor_.health->current<=0)return {};
        const skills::effects::MindWrackEnergy before{energy_->current,energy_->maximum,0};auto after=before;after.current-=std::min(1u,after.current);
        const skills::effects::MindWrackTransition event{owner.target,use.event,use.at,before,after};
        auto result=skills::effects::mind_wrack_target_reaction(hexes_,owner,use,event);if(!result)return {};
        energy_->current=after.current;hexes_=result->registry;actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,use.at);return result;
    }
    bool neutral_guardian(const Lifetime& expected)const{
        return matches(expected)&&relation_==Relation::neutral&&actor_.damageable&&
            actor_.health&&actor_.health->current>0;
    }
    bool neutral_actor(const Lifetime& expected)const{return matches(expected)&&relation_==Relation::neutral;}
    enum class StalkerCharmTransition {hostility,adoption};
    struct StalkerCharmChange {std::optional<control::Token> cancelled;};
    // Trusted World-only lane for the exact reviewed pre Stalker. World also
    // verifies its model-file list, charmability enrollment, owner/cast receipt
    // and (for adoption) successful durable CAS. Never a generic relation setter.
    // Preserve generation, Health, conditions/effects and packed allegiance:
    // normalized captured neutral2/pet4 are NOT packed wire allegiance values.
    std::optional<StalkerCharmChange> transition_stalker_charm(const Lifetime& expected,
            const Binding& reviewed,StalkerCharmTransition transition,std::uint64_t now){
        if(!matches(expected)||reviewed!=binding_||reviewed.map!=162||reviewed.model_file!=128406||
           reviewed.skin_file||reviewed.appearance||reviewed.definition_flags!=13||reviewed.level!=5||
           !actor_.damageable||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return {};
        if((transition==StalkerCharmTransition::hostility&&relation_!=Relation::neutral)||
           (transition==StalkerCharmTransition::adoption&&relation_!=Relation::hostile_to_players)||
           (transition!=StalkerCharmTransition::hostility&&transition!=StalkerCharmTransition::adoption))return {};
        const auto action=active_action(expected,now);
        if(!cancel_action(expected,now))return {};
        relation_=transition==StalkerCharmTransition::hostility?Relation::hostile_to_players:Relation::allied_to_players;
        return StalkerCharmChange{action};
    }
    // Exact owned lifecycle enrollment only: World prepares this after durable
    // CAS admission, or from the authenticated persistent profile on entry.
    bool enroll_owned_stalker(const Lifetime& expected,const Binding& reviewed,
            skills::effects::Scope owner,std::uint32_t team,bool pve,std::uint64_t now){
        if(!matches(expected)||reviewed!=binding_||reviewed.model_file!=128406||reviewed.skin_file||
           reviewed.appearance||reviewed.definition_flags!=13||reviewed.level<5||reviewed.level>20||
           owner.instance!=lifetime_.world||!owner.actor||owner.actor>65535||!owner.generation||owner==hex_scope()||!team||
           relation_!=Relation::allied_to_players||healing_allowed_!=std::optional(true)||!actor_.damageable||
           !actor_.health||actor_.health->maximum!=20u*reviewed.level+80||
           physical_armor_!=int(3*reviewed.level+20)||general_armor_!=physical_armor_||
           actor_.earth_armor!=physical_armor_||actor_.fire_armor!=physical_armor_||
           actor_.cold_armor!=physical_armor_||actor_.lightning_armor!=physical_armor_||
           (owned_stalker_owner_&&*owned_stalker_owner_!=owner)||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return false;
        owned_stalker_owner_=owner;owned_stalker_pve_=pve;allegiance_=team;return true;
    }
    std::optional<skills::effects::Scope> owned_stalker_owner(const Lifetime& expected)const{
        return matches(expected)?owned_stalker_owner_:std::nullopt;
    }
    // World proves current pet_owners_ membership and retires the corpse's
    // external effect-credit/maintained records first. This exact-species lane
    // does not replace LiveActor or reuse its generation/control sequences.
    bool revive_owned_stalker(const Lifetime& expected,const Binding& reviewed,
            skills::effects::Scope owner,unsigned health_percent,std::uint64_t now){
        if(!matches(expected)||reviewed!=binding_||reviewed.model_file!=128406||reviewed.skin_file||
           reviewed.appearance||reviewed.definition_flags!=13||reviewed.level<5||reviewed.level>20||
           owner.instance!=lifetime_.world||!owner.actor||owner.actor>65535||!owner.generation||owner==hex_scope()||
           relation_!=Relation::allied_to_players||healing_allowed_!=std::optional(true)||
           !actor_.damageable||!actor_.health||actor_.health->current!=0||!health_percent||health_percent>100||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_)||
           !hexes_.active(hex_scope(),now).empty()||weapon_spell_)return false;
        const auto maximum=20u*reviewed.level+80;const auto armor=int(3*reviewed.level+20);
        if(physical_armor_!=armor||general_armor_!=armor||actor_.earth_armor!=armor||
           actor_.fire_armor!=armor||actor_.cold_armor!=armor||actor_.lightning_armor!=armor)return false;
        if(!control_.reset_after_death(hex_scope(),now))return false;
        conditions_.clear();disease_credit_.reset();wound_.reset(maximum);
        hexes_.clear(hex_scope());movement_.clear(now);glimmering_removed_.reset();
        weapon_spell_.reset();convert_hexes_armor_.reset();watch_yourself_.reset();
        fevered_owner_.reset();generating_fevered_dazed_=false;fevered_observations_.clear();
        attack_hex_hits_.clear();attack_energy_changes_.clear();attack_hex_levels_.clear();
        fragility_hits_.clear();fragility_levels_.clear();enchantment_ends_.clear();
        absorption_=skills::effects::ShieldAbsorption(hex_scope());spellbook_.reset();
        energy_curve_.reset();energy_zero_.reset();energy_zero_before_ms_.reset();energy_zero_events_.reset();
        energy_regeneration_.reset();overcast_observation_.reset();
        condition_clock_=now;if(energy_)energy_clock_=now;
        actor_.health=Health{std::max(1.,std::floor(double(maximum)*health_percent/100)),maximum};
        actor_.effects=conditions_.presentation(hexes_.presentation(hex_scope(),actor_.effects&~std::uint32_t(0x10),now),now);
        return true;
    }
    bool matches(const Lifetime& expected)const{return expected==lifetime_&&
        actor_.id==lifetime_.actor&&actor_.generation==lifetime_.actor_generation;}
    // Read-only protection snapshot for armor-ignoring holy proposals. The
    // normal damage methods still own elemental/condition armor resolution.
    std::optional<Actor> holy_damage_snapshot(const Lifetime& expected,std::uint64_t now)const{
        if(!matches(expected)||(condition_clock_&&now<*condition_clock_))return {};
        return incoming_damage_actor(now);
    }
    Actor* actor(const Lifetime& expected){return matches(expected)?&actor_:nullptr;}
    const Actor* actor(const Lifetime& expected)const{return matches(expected)?&actor_:nullptr;}
    bool healable_ally(const Lifetime& expected)const{
        return matches(expected)&&(relation_==Relation::protected_ally||relation_==Relation::allied_to_players)&&actor_.health&&actor_.health->current>0;
    }
    std::optional<skills::effects::Entry> aura_restoration_entry(const Lifetime& expected,std::uint64_t now)const{
        if(!matches(expected)||(condition_clock_&&now<*condition_clock_))return {};
        for(const auto& entry:hexes_.active(hex_scope(),now))
            if(entry.skill==180&&entry.cause==expected.actor&&entry.kind==skills::effects::Category::enchantment)return entry;
        return {};
    }
    bool apply_aura_restoration(const Lifetime& expected,const skills::effects::AuraRestorationPlan& plan,unsigned effect,std::uint64_t now){
        const auto canonical=skills::effects::aura_restoration_plan(plan.rank);
        if(!matches(expected)||!canonical||*canonical!=plan||healing_allowed_!=std::optional(true)||!effect||
           now>UINT64_MAX-plan.duration||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||hexes_.lookup(hex_scope(),effect))return false;
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return false;
        if(proposed.actor_.health->current<=0)return false;
        const skills::effects::Entry entry{effect,expected.actor,180,plan.rank,skills::effects::Category::enchantment,now,now+plan.duration};
        if(!proposed.hexes_.apply(hex_scope(),entry,skills::effects::Replace::same_skill).accepted)return false;
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return true;
    }
    std::optional<std::uint64_t> health_loss_revision(const Lifetime& expected)const{
        return matches(expected)?std::optional(health_loss_revision_):std::nullopt;
    }
    bool heal(const Lifetime& expected,double amount,std::uint64_t now){
        if(!matches(expected)||!actor_.health||actor_.health->current<=0||!std::isfinite(amount)||amount<0||(condition_clock_&&now<*condition_clock_))return false;
        if(condition_clock_)advance_conditions(expected,now);
        if(actor_.health->current<=0)return false;
        const auto multiplier=skills::effects::healing_hex_multiplier(hexes_.active(hex_scope(),now),now);
        if(wound_.base)wound_.event(actor_.health->current,amount,true,multiplier);
        else actor_.health->current=std::min(double(actor_.health->maximum),actor_.health->current+amount*std::max(.6,multiplier));
        if(actor_.health->current<=0){actor_.effects|=0x10u;control_.invalidate();}
        return true;
    }
    bool heal_ally(const Lifetime& expected,double amount,std::uint64_t now){
        return healing_allowed_!=std::optional(false)&&healable_ally(expected)&&heal(expected,amount,now);
    }
    // Explicit hostile-NPC support heal. World must also prove separate live
    // scopes, target range, plane and line of sight before invoking this.
    bool heal_hostile_ally(const Lifetime& expected,const LiveActor& source,
                           const Lifetime& source_life,double amount,std::uint64_t now){
        return matches(expected)&&source.matches(source_life)&&
            hostile_to_players(expected)&&source.hostile_to_players(source_life)&&
            actor_.id!=source.actor_.id&&actor_.health&&actor_.health->current>0&&
            source.actor_.health&&source.actor_.health->current>0&&
            allegiance_&&source.allegiance_&&*allegiance_!=0&&
            *allegiance_==*source.allegiance_&&
            healing_allowed_!=std::optional(false)&&heal(expected,amount,now);
    }
    // Summon Spirits grants Health to non-fleshy, healing-immune spirits. It
    // is not a healing spell and does not use healing hex or wound modifiers.
    bool gain_spirit_health(const Lifetime& expected,double amount,std::uint64_t now){
        if(healing_allowed_!=std::optional(false)||fleshy_!=std::optional(false)||
           !healable_ally(expected)||!std::isfinite(amount)||amount<0||
           (condition_clock_&&now<*condition_clock_))return false;
        if(condition_clock_&&!advance_conditions(expected,now))return false;
        if(!actor_.health||actor_.health->current<=0)return false;
        actor_.health->current=std::min(double(actor_.health->maximum),actor_.health->current+amount);
        return true;
    }
    // Health gain is separate from healing: it bypasses healing immunity,
    // healing hexes and Deep Wound's received-healing penalty. Reviewed
    // non-fleshy spirits retain the existing Summon Spirits gain authority.
    bool gain_ally_health(const Lifetime& expected,double amount,std::uint64_t now){
        if(creature_type_&&*creature_type_=="spirit")
            return gain_spirit_health(expected,amount,now);
        if(!healable_ally(expected)||!std::isfinite(amount)||amount<0||
           (condition_clock_&&now<*condition_clock_))return false;
        if(condition_clock_&&!advance_conditions(expected,now))return false;
        if(!actor_.health||actor_.health->current<=0)return false;
        if(wound_.base)wound_.event(actor_.health->current,amount,false);
        else actor_.health->current=std::min(double(actor_.health->maximum),actor_.health->current+amount);
        if(actor_.health->current<=0){actor_.effects|=0x10u;control_.invalidate();}
        return true;
    }
    bool hostile_to_players(const Lifetime& expected)const{
        return matches(expected)&&relation_==Relation::hostile_to_players;
    }
    std::optional<std::uint32_t> packed_allegiance(const Lifetime& expected)const{
        return matches(expected)?allegiance_:std::nullopt;
    }
    std::optional<skills::effects::Applied> apply_attack_hex(const Lifetime& expected,
            const skills::effects::AttackHexPlan& plan,unsigned cause,unsigned level,unsigned effect,std::uint64_t now,std::optional<skills::effects::HexDuration> adjusted={}){
        const auto duration=adjusted?adjusted->for_hex(plan.skill,plan.duration,cause):std::optional(plan.duration);
        if(!duration)return {};
        const auto verified=skills::effects::attack_hex(plan.skill,plan.rank);
        if(!verified||*verified!=plan||!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||
           !actor_.health||actor_.health->current<=0||(plan.energy_loss&&!energy_)||!level||level>255||now>UINT64_MAX-*duration||
           (condition_clock_&&now<*condition_clock_))return {};
        advance_conditions(expected,now);if(actor_.health->current<=0)return {};
        const auto result=hexes_.apply(hex_scope(),{effect,cause,plan.skill,plan.rank,skills::effects::Category::hex,now,now+*duration},skills::effects::Replace::same_skill);
        if(!result.accepted)return {};
        for(const auto& old:result.removed)attack_hex_levels_.erase(old.instance);
        attack_hex_levels_[effect]=level;if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return result;
    }
    std::optional<skills::effects::Applied> apply_wandering_eye(const Lifetime& expected,
            skills::effects::Scope caster,const skills::effects::WanderingEyePlan& plan,
            unsigned effect,std::uint64_t now,std::uint64_t duration){
        if(!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||
           !actor_.health||actor_.health->current<=0||(condition_clock_&&now<*condition_clock_))return {};
        advance_conditions(expected,now);
        if(!actor_.health||actor_.health->current<=0)return {};
        auto proposed=skills::effects::prepare_wandering_eye(hexes_,caster,hex_scope(),effect,plan,now,duration);
        if(!proposed)return {};
        skills::effects::Applied applied{true,proposed->replaced};
        hexes_=std::move(proposed->registry);
        if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);
        return applied;
    }
    auto take_attack_energy_changes(const Lifetime& expected){
        std::vector<AttackEnergyChange> result;if(!matches(expected))return result;
        result=std::move(attack_energy_changes_);attack_energy_changes_.clear();return result;
    }
    std::vector<AttackHexHit> take_attack_hex_hits(const Lifetime& expected){
        if(!matches(expected))return {};auto result=std::move(attack_hex_hits_);attack_hex_hits_.clear();return result;
    }
    std::optional<skills::effects::Applied> apply_fragility(const Lifetime& expected,
            const skills::effects::FragilityPlan& plan,unsigned cause,unsigned level,unsigned effect,std::uint64_t now,std::optional<skills::effects::HexDuration> adjusted={}){
        const auto duration=adjusted?adjusted->for_hex(plan.skill,plan.duration,cause):std::optional(plan.duration);
        if(!duration)return {};
        const auto verified=skills::effects::fragility(plan.skill,plan.rank);
        if(!verified||*verified!=plan||!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||
           !actor_.health||actor_.health->current<=0||!level||level>255||now>UINT64_MAX-*duration||
           (condition_clock_&&now<*condition_clock_))return {};
        advance_conditions(expected,now);if(actor_.health->current<=0)return {};
        const auto result=hexes_.apply(hex_scope(),{effect,cause,plan.skill,plan.rank,skills::effects::Category::hex,now,now+*duration},skills::effects::Replace::same_skill);
        if(!result.accepted)return {};
        for(const auto& old:result.removed)fragility_levels_.erase(old.instance);
        fragility_levels_[effect]=level;if(!condition_clock_)condition_clock_=now;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return result;
    }
    std::vector<FragilityHit> take_fragility_hits(const Lifetime& expected){
        if(!matches(expected))return {};auto result=std::move(fragility_hits_);fragility_hits_.clear();return result;
    }
    bool allows_conditions(const Lifetime& expected)const{return matches(expected)&&conditions_allowed_;}
    std::optional<bool> fleshy(const Lifetime& expected)const{return matches(expected)?fleshy_:std::nullopt;}
    std::optional<std::string> creature_type(const Lifetime& expected)const{return matches(expected)?creature_type_:std::nullopt;}
    std::optional<bool> servant_of_abaddon(const Lifetime& expected)const{
        return matches(expected)?servant_of_abaddon_:std::nullopt;
    }
    std::optional<bool> disease_immune(const Lifetime& expected,std::uint64_t now)const{
        if(!matches(expected))return std::nullopt;
        if(!conditions_allowed_||hexes_.newest_skill(hex_scope(),113,now)||hexes_.newest_skill(hex_scope(),1520,now))return true;
        return disease_immune_;
    }
    std::optional<skills::effects::Scope> disease_credit(const Lifetime& expected,std::uint64_t now)const{
        if(!matches(expected)||!conditions_.has(conditions::Kind::disease,now)||!disease_credit_)return {};
        const auto entry=conditions_.lookup(conditions::Kind::disease);const auto& saved=disease_credit_->infection;
        if(entry->cause!=saved.cause||entry->applied!=saved.applied||entry->expires!=saved.expires)return {};
        return disease_credit_->caster;
    }
    bool record_disease_credit(const Lifetime& expected,std::optional<skills::effects::Scope> caster,std::uint64_t now){
        if(!matches(expected)||!conditions_.has(conditions::Kind::disease,now))return false;
        const auto entry=*conditions_.lookup(conditions::Kind::disease);
        if(caster&&(caster->instance!=lifetime_.world||caster->actor!=entry.cause||!caster->generation))return false;
        disease_credit_.reset();if(caster)disease_credit_=DiseaseCredit{entry,*caster};return true;
    }
    bool apply_condition(const Lifetime& expected,conditions::Kind kind,std::uint32_t cause,
                         std::uint64_t now,std::uint64_t duration_ms){
        if(!matches(expected)||!conditions_allowed_||!actor_.health||!valid(*actor_.health)||
           !actor_.health->current||(actor_.effects&0x10)||!cause||cause>65535)return false;
        if(kind==conditions::Kind::burning&&skills::effects::active_frigid_armor(hexes_,hex_scope(),now))return false;
        if(kind==conditions::Kind::deep_wound&&!wound_.base&&
           (actor_.health->maximum<1||actor_.health->maximum>UINT32_MAX||std::floor(actor_.health->maximum)!=actor_.health->maximum))return false;
        if(condition_clock_&&now<*condition_clock_)return false;
        // Validate before advancing time, then integrate the old condition set
        // before renewal replaces its original application timestamp.
        auto proposed=conditions_;
        if(!proposed.apply(kind,cause,now,duration_ms))return false;
        if(condition_clock_){
            advance_conditions(expected,now);
            if(!actor_.health->current||(actor_.effects&0x10))return false;
        }
        const auto fragility_before=conditions_;
        if(!conditions_.apply(kind,cause,now,duration_ms))return false;
        if(kind==conditions::Kind::disease){const auto old=fragility_before.lookup(kind),entry=conditions_.lookup(kind);
            if(!old||old->cause!=entry->cause||old->applied!=entry->applied||old->expires!=entry->expires)disease_credit_.reset();}
        movement_.crippled(conditions_,now);
        if(!condition_clock_)condition_clock_=now;
        sync_wound(now);actor_.effects=conditions_.presentation(actor_.effects,now);react_fragility(fragility_before,now);return true;
    }
    struct ExternalHealth {
        std::vector<std::uint64_t> boundaries;
        std::function<int(std::uint64_t)> pips;
        std::function<int(std::uint64_t)> energy_pips;
        std::uint32_t cause=0;
    };
    struct ConditionUpdate {Health before,after;std::uint32_t cause=0;bool killed=false;std::uint64_t death_time=0;std::optional<conditions::Kind> condition_kind;std::optional<skills::effects::Scope> condition_credit;};
    std::optional<ConditionUpdate> advance_conditions(const Lifetime& expected,std::uint64_t now,
                                                       const ExternalHealth* external=nullptr){
        if(!matches(expected)||!actor_.health||!condition_clock_||now<*condition_clock_)return {};
        if(external&&(!external->pips||external->cause>65535))return {};
        // Do not partly advance Health and then discover an earlier Energy
        // reaction. The World scheduler must select that precise boundary.
        if(energy_curve_){const auto forecast=forecast_energy_zero(expected,now);
            if(!forecast||forecast->curve.cursor()<now)return {};
        }
        ConditionUpdate result{*actor_.health,*actor_.health};
        auto points=conditions_.expiry_boundaries(*condition_clock_,now);
        for(const auto& entry:hexes_.active(hex_scope(),*condition_clock_)){
            if(entry.expires&&*entry.expires<now)points.push_back(*entry.expires);
            if(entry.skill==44&&entry.expires){
                const auto plan=skills::effects::wound_spell(44,entry.rank);
                if(plan&&plan->wound_duration<=UINT64_MAX-*entry.expires&&
                   *entry.expires+plan->wound_duration<now)points.push_back(*entry.expires+plan->wound_duration);
            }
        }
        for(unsigned i=0;i<10;++i)if(const auto e=conditions_.lookup(static_cast<conditions::Kind>(i));e&&e->applied>*condition_clock_&&e->applied<now)points.push_back(e->applied);
        if(external)for(const auto edge:external->boundaries)if(*condition_clock_<edge&&edge<now)points.push_back(edge);
        points.push_back(now);std::sort(points.begin(),points.end());points.erase(std::unique(points.begin(),points.end()),points.end());
        std::map<std::uint64_t,int> outside_pips,outside_energy_pips;
        if(external){std::vector<std::uint64_t> samples{*condition_clock_};samples.insert(samples.end(),points.begin(),points.end());
          for(const auto edge:samples){
            const int pips=external->pips(edge);
            if(pips< -10||pips>10)return {};
            outside_pips.emplace(edge,pips);
            if(external->energy_pips){
                const int energy_pips=external->energy_pips(edge);
                if(energy_pips< -10||energy_pips>10)return {};
                outside_energy_pips.emplace(edge,energy_pips);
            }
          }
        }
        auto from=*condition_clock_;
        for(const auto until:points){
            if(actor_.health->current>0){
                result.condition_kind.reset();result.condition_credit.reset();
                int hex_pips=0;
                if(const auto entry=hexes_.newest_skill(hex_scope(),44,from))
                    if(const auto plan=skills::effects::wound_spell(44,entry->rank)){
                        hex_pips-=static_cast<int>(plan->degeneration);result.cause=entry->cause;
                    }
                if(const auto entry=hexes_.newest_skill(hex_scope(),135,from)){
                    if(const auto plan=skills::effects::faintheartedness(entry->rank)){hex_pips=-static_cast<int>(plan->pips);if(hex_pips)result.cause=entry->cause;}
                }
                for(const auto& entry:hexes_.active(hex_scope(),from))if(entry.skill==54||entry.skill==3152){
                    if(const auto plan=skills::effects::movement_hex(entry.skill,entry.rank)){hex_pips-=static_cast<int>(plan->pips);result.cause=entry.cause;}
                }
                // Use the same canonical plans as hex admission. A separate ID
                // list omitted Putrid Bile2058 even though its hex was accepted.
                for(const auto& entry:hexes_.active(hex_scope(),from))if(entry.kind==skills::effects::Category::hex){
                    if(const auto plan=skills::effects::degeneration_hex(entry.skill,entry.rank);plan&&plan->pips){
                        hex_pips-=static_cast<int>(plan->pips);result.cause=entry.cause;
                    }
                }
                for(const auto& entry:hexes_.active(hex_scope(),from))if(entry.skill==828){
                    if(const auto plan=skills::effects::condition_damage(828,entry.rank)){
                        hex_pips-=int(plan->pips);if(plan->pips)result.cause=entry.cause;
                    }
                }
                for(const auto& entry:hexes_.active(hex_scope(),from))if(entry.skill==898||entry.skill==899){
                    if(const auto plan=skills::effects::action_damage_hex(entry.skill,entry.rank)){
                        hex_pips-=int(plan->pips);if(plan->pips)result.cause=entry.cause;
                    }
                }
                for(const auto& entry:hexes_.active(hex_scope(),from))if(entry.skill==125||entry.skill==140){
                    if(const auto plan=skills::effects::energy_degeneration_hex(entry.skill,entry.rank)){
                        hex_pips-=int(plan->target_health_degen);if(plan->target_health_degen)result.cause=entry.cause;
                    }
                }
                if(const auto entry=ice_spear_regeneration(expected,from)){
                    if(entry->kind==skills::effects::Category::enchantment)
                        if(const auto plan=skills::effects::water_projectile(214,entry->rank))hex_pips+=static_cast<int>(plan->overcast_regeneration_pips);
                }
                if(const auto entry=hexes_.newest_skill(hex_scope(),1392,from))
                    if(entry->kind==skills::effects::Category::enchantment)
                        if(const auto plan=skills::effects::watchful_healing(entry->rank))hex_pips+=int(plan->pips);
                if(const auto entry=hexes_.newest_skill(hex_scope(),886,from);entry&&entry->kind==skills::effects::Category::enchantment)
                    if(const auto plan=skills::regeneration::reviewed(886,entry->rank))hex_pips+=plan->pips;
                if(const auto entry=hexes_.newest_skill(hex_scope(),261,from);entry&&entry->kind==skills::effects::Category::enchantment)
                    if(const auto plan=skills::effects::shield_regeneration(entry->rank))hex_pips+=int(plan->pips);
                if(const auto entry=hexes_.newest_skill(hex_scope(),308,from);entry&&entry->kind==skills::effects::Category::enchantment&&
                   !entry->rank&&!entry->expires)
                    if(const auto plan=skills::effects::succor())hex_pips+=int(plan->health_pips);
                if(weapon_spell_)hex_pips+=int(skills::effects::weapon_spell_pips(*weapon_spell_,hexes_,hex_scope(),conditions_,from));
                const auto outside=external?outside_pips.at(from):0;
                const auto owned_pips=conditions_.degeneration_pips(from)+hex_pips;
                const auto pips=std::clamp(owned_pips+outside,-10,10);
                if(outside<0&&owned_pips==0&&external->cause)result.cause=external->cause;
                const auto energy_until=pips<0?from+std::min(until-from,health_timing::death_delay(actor_.health->current,pips).value_or(until-from)):until;
                if((energy_||energy_curve_||external&&external->energy_pips)&&
                   !settle_energy_clock(expected,energy_until,external&&external->energy_pips?outside_energy_pips.at(from):0))return {};
                if(pips!=0){
                    for(auto kind:{conditions::Kind::bleeding,conditions::Kind::burning,conditions::Kind::disease,conditions::Kind::poison})
                        if(conditions_.has(kind,from)){
                            result.cause=conditions_.lookup(kind)->cause;result.condition_kind=kind;
                            result.condition_credit=kind==conditions::Kind::disease?disease_credit(expected,from):std::nullopt;
                        }
                    sync_wound(from);
                    const auto before=actor_.health->current;
                    const auto delta=2.*pips*double(until-from)/1000.;
                    auto health=*actor_.health;auto wound=wound_;
                    if(wound.base)wound.regeneration(health.current,delta);
                    else health.current=std::clamp(health.current+delta,0.,double(health.maximum));
                    if(pips<0&&health_timing::zero_residue(health.current,pips)){
                        if(wound.base)wound.regeneration(health.current,-health.current);
                        else health.current=0;
                    }
                    assign_health(health);wound_=std::move(wound);
                    if(actor_.health->current<=0)result.death_time=from+std::min(until-from,health_timing::death_delay(before,pips).value_or(until-from));
                }
            }
            const auto losses=skills::effects::expired_condition_transitions(conditions_,from,until);
            from=until;sync_wound(until);
            // Healing reactions may re-enter advancement. The segment and its
            // expiry deadline are already consumed before publishing callbacks.
            *condition_clock_=until;react_fragility(losses);
            // Snapshot at the previous segment's start, then consume the exact
            // application before its followup. Dead recipients gain no wound.
            for(const auto& entry:hexes_.active(hex_scope(),until?until-1:0))
                if(entry.skill==44&&entry.expires&&*entry.expires==until){
                    hexes_.remove(hex_scope(),entry.instance);finish_phantom_pain(entry,until);
                }
            if(!fragility_hits_.empty()&&(fragility_hits_.back().hit.killed||(fragility_hits_.back().reversal&&fragility_hits_.back().reversal->after_healing.current<=0))){result.cause=fragility_hits_.back().cause;result.death_time=fragility_hits_.back().at;result.condition_kind.reset();result.condition_credit.reset();}
        }
        *condition_clock_=now;conditions_.expire(now);
        result.after=*actor_.health;result.killed=result.before.current>0&&result.after.current<=0;
        if(result.killed)control_.invalidate();
        actor_.effects=conditions_.presentation(actor_.effects,now)|(result.killed?0x10u:0u);
        return result;
    }
    bool remove_condition(const Lifetime& expected,conditions::Kind kind,std::uint64_t now){
        if(!matches(expected)||(condition_clock_&&now<*condition_clock_))return false;
        advance_conditions(expected,now);
        const auto fragility_before=conditions_;
        const bool removed=conditions_.remove(kind);
        if(kind==conditions::Kind::disease)disease_credit_.reset();
        movement_.crippled(conditions_,now);
        sync_wound(now);actor_.effects=conditions_.presentation(actor_.effects,now);react_fragility(fragility_before,now);return removed;
    }
    // This is the server-owned registry (spawn combat state starts empty),
    // never an imported visual effects mask. Caller settles to now first.
    std::optional<skills::effects::Registry> hex_registry_snapshot(const Lifetime& expected,std::uint64_t now)const{
        if(!matches(expected)||(condition_clock_&&*condition_clock_>now))return {};
        return hexes_;
    }
    std::optional<conditions::State> condition_snapshot(const Lifetime& expected)const{
        return matches(expected)?std::optional(conditions_):std::nullopt;
    }
    bool has_condition(const Lifetime& expected,conditions::Kind kind,std::uint64_t now)const{
        return matches(expected)&&conditions_.has(kind,now);
    }
    std::optional<skills::effects::TransferredConditions> receive_conditions(const Lifetime& expected,
            const skills::effects::ConditionTransferPlan& plan,const conditions::State& source,unsigned cause,std::uint64_t now){
        if(!skills::effects::valid_condition_transfer(plan)||!matches(expected)||!hostile_to_players(expected)||!conditions_allowed_||!actor_.damageable||!actor_.health||actor_.health->current<=0||
           !cause||cause>65535||(condition_clock_&&now<*condition_clock_))return {};
        if(skills::effects::active_frigid_armor(hexes_,hex_scope(),now)){
            const auto preview=skills::effects::transfer_conditions(plan,source,conditions_,cause,now);
            if(!preview||std::find(preview->kinds.begin(),preview->kinds.end(),conditions::Kind::burning)!=preview->kinds.end())return {};
        }
        advance_conditions(expected,now);if(actor_.health->current<=0)return {};
        const auto result=skills::effects::transfer_conditions(plan,source,conditions_,cause,now);if(!result)return {};
        if(std::find(result->kinds.begin(),result->kinds.end(),conditions::Kind::deep_wound)!=result->kinds.end()&&!wound_.base&&
           (actor_.health->maximum<1||actor_.health->maximum>UINT32_MAX||std::floor(actor_.health->maximum)!=actor_.health->maximum))return {};
        const auto fragility_before=conditions_;
        conditions_=result->target;movement_.crippled(conditions_,now);if(!condition_clock_)condition_clock_=now;
        sync_wound(now);actor_.effects=conditions_.presentation(actor_.effects,now);react_fragility(fragility_before,now);return result;
    }
    bool expire_conditions(const Lifetime& expected,std::uint64_t now){
        if(!matches(expected)||(condition_clock_&&now<*condition_clock_))return false;
        advance_conditions(expected,now);
        conditions_.expire(now);sync_wound(now);actor_.effects=conditions_.presentation(actor_.effects,now);return true;
    }
    bool clear_conditions(const Lifetime& expected,std::optional<std::uint64_t> time={}){
        const auto now=time.value_or(condition_clock_.value_or(0));
        if(!matches(expected)||(condition_clock_&&now<*condition_clock_))return false;
        advance_conditions(expected,now);const auto before=conditions_;
        conditions_.clear();disease_credit_.reset();movement_.crippled(conditions_,now);sync_wound(now);
        // Enchantments share this resource clock with conditions. Clearing
        // conditions must not discard elapsed regeneration on the next tick.
        actor_.effects=conditions_.presentation(actor_.effects,now);react_fragility(before,now);condition_clock_=now;return true;
    }
    std::optional<int> final_earth_armor(const Lifetime& expected,std::uint64_t now,const skills::effects::EndingHexContext*ending_hex=nullptr)const{
        if(!matches(expected)||!actor_.earth_armor)return std::nullopt;
        return conditions_.earth_armor(*actor_.earth_armor+int(elemental_skill_armor(now,ending_hex)),now);
    }
    std::optional<int> final_holy_armor(const Lifetime& expected,std::uint64_t now)const{
        if(!matches(expected)||!general_armor_)return std::nullopt;
        return conditions_.armor(*general_armor_+int(skill_armor_bonus(now)),now);
    }
    std::optional<int> final_physical_armor(const Lifetime& expected,std::uint64_t now,
            bool projectile_attack=false)const{
        if(!matches(expected)||!physical_armor_)return std::nullopt;
        const auto frigid=skills::effects::active_frigid_armor(hexes_,hex_scope(),now);
        const auto ordinary=skills::effects::stacked_skill_armor(skill_armor_bonus(now),frigid?frigid->physical_armor:0);
        const auto bonus=projectile_attack?skills::effects::stacked_skill_armor(ordinary,
            skills::effects::shields_up_armor(hexes_,hex_scope(),now,
                skills::effects::ShieldsUpDamage::projectile_attack)):ordinary;
        return conditions_.armor(*physical_armor_+int(bonus),now);
    }
    std::optional<int> final_caster_armor(const Lifetime& expected,unsigned damage_type,
            std::uint64_t now,bool projectile_attack=false)const{
        if(!matches(expected))return {};
        const auto armor=damage_type==3?actor_.cold_armor:damage_type==4?actor_.lightning_armor:
            damage_type==5?actor_.fire_armor:damage_type==11?actor_.earth_armor:
            (damage_type==6||damage_type==7)?general_armor_:std::nullopt;
        const auto ordinary=weapon::element(damage_type)?elemental_skill_armor(now):skill_armor_bonus(now);
        const auto bonus=projectile_attack?skills::effects::stacked_skill_armor(ordinary,
            skills::effects::shields_up_armor(hexes_,hex_scope(),now,
                skills::effects::ShieldsUpDamage::projectile_attack)):ordinary;
        return armor?std::optional(conditions_.armor(*armor+int(bonus),now)):std::nullopt;
    }
    std::optional<Hit> apply_caster_weapon_damage(const Lifetime& expected,double damage,unsigned damage_type,std::uint64_t now){
        if(!matches(expected)||(energy_clock_&&now<*energy_clock_)||(!weapon::element(damage_type)&&damage_type!=6&&damage_type!=7))return {};
        return apply_resolved_weapon_damage(expected,damage,damage_type,now,hostile_to_players(expected));
    }
    std::optional<Hit> apply_weapon_damage(const Lifetime& expected,double damage,std::optional<std::uint64_t> at={}){
        if(!matches(expected)||(at&&condition_clock_&&*at<*condition_clock_))return {};
        return apply_resolved_weapon_damage(expected,damage,0,at.value_or(condition_clock_.value_or(0)),hostile_to_players(expected));
    }
    // Trusted World caller must prove current opposing player-owner team.
    std::optional<Hit> apply_opposed_owned_weapon_damage(const Lifetime& expected,double damage,unsigned damage_type,std::uint64_t now){
        if(!matches(expected)||relation_!=Relation::allied_to_players||(condition_clock_&&now<*condition_clock_)||
           (energy_clock_&&now<*energy_clock_)||(damage_type&&!weapon::element(damage_type)&&damage_type!=6&&damage_type!=7))return {};
        return apply_resolved_weapon_damage(expected,damage,damage_type,now,true);
    }
    bool opposed_npc_recipient(const Lifetime& expected,const LiveActor& source,const Lifetime& source_life,bool released=false)const{
        if(!matches(expected)||!source.matches(source_life)||hex_scope().instance!=source.hex_scope().instance||hex_scope()==source.hex_scope()||
           !actor_.damageable||!source.actor_.damageable||!actor_.health||actor_.health->current<=0||!source.actor_.health||
           (source.actor_.health->current<=0&&!(released&&source.actor_.health->current==0)))return false;
        return (relation_==Relation::allied_to_players&&source.relation_==Relation::hostile_to_players)||
            (relation_==Relation::hostile_to_players&&source.relation_==Relation::allied_to_players);
    }
    // World supplies actual opposing actor scopes plus an owned ordinary release;
    // player hostility and neutral-guardian permissions are separate entry points.
    std::optional<Hit> apply_opposed_npc_weapon_damage(const Lifetime& expected,const LiveActor& source,const Lifetime& source_life,
            double damage,unsigned damage_type,std::uint64_t now,bool released=false){
        if(!opposed_npc_recipient(expected,source,source_life,released)||(condition_clock_&&now<*condition_clock_)||
           (energy_clock_&&now<*energy_clock_)||(damage_type>2&&!weapon::element(damage_type)))return {};
        return apply_resolved_weapon_damage(expected,damage,damage_type,now,true);
    }
private:
    std::optional<Hit> apply_resolved_weapon_damage(const Lifetime& expected,double damage,unsigned damage_type,std::uint64_t now,bool hostile){
        const auto absorption=hexes_.newest_skill(hex_scope(),1399,now)?absorption_.prepare(hexes_,hex_scope(),absorption_event_+1,now):std::nullopt;
        if(hexes_.newest_skill(hex_scope(),1399,now)&&!absorption)return {};
        auto effective=incoming_damage_actor(now);
        // Resolved weapons bypass Actor's direct-damage multiplier. Apply only
        // the owned-pet term here, before existing cap/flat reduction policy.
        auto resolved=capped_damage(damage*(damage_type==5?fire_multiplier_:1)*(owned_stalker_pve_?.67:1.),effective.incoming_damage_cap);
        if(!resolved)return {};
        if(const auto plan=skills::effects::active_stoneflesh_aura(hexes_,hex_scope(),now))
            *resolved=std::max(0.,*resolved-plan->reduction);
        // The cap has already run before flat reduction. Do not apply it again
        // inside the resolved-hit publisher with a different phase ordering.
        effective.incoming_damage_cap.reset();
        if(absorption)*resolved=std::max(0.,*resolved-absorption->reduction);
        auto hit=weapon::apply_resolved(effective,*resolved,hostile);
        if(!hit)return {};
        if(const auto kind=weapon::element(damage_type)){hit->kind=*kind;if(!storm_energy_hit(expected,*hit,now))return {};}
        if(!settle_damage_health(*hit,wound_))return {};effective.health=hit->after;effective.effects=hit->effects;
        if(absorption){if(!absorption_.commit(hexes_,hex_scope(),*absorption,now))return {};++absorption_event_;}
        assign_health(effective.health);actor_.effects=effective.effects;if(hit->killed)control_.invalidate();return hit;
    }
    bool storm_energy_hit(const Lifetime& expected,const Hit& hit,std::uint64_t now){
        const auto storm=skills::effects::active_storm_chaser(hexes_,hex_scope(),now);
        const auto gain=storm?skills::effects::storm_chaser_energy(*storm,hit):0;
        if(!gain)return true;
        if(!advance_energy(expected,now)||!energy_||!actor_.health||actor_.health->current<=0)return false;
        // Self-owned reward is independent of player hostility. Keep the
        // public hostile-target Energy API's stricter targeting policy intact.
        energy_->current+=std::min(gain,energy_->maximum-energy_->current);
        if(energy_->current==energy_->maximum)energy_regeneration_.reset();
        return true;
    }
    Actor incoming_damage_actor(std::uint64_t now,const skills::effects::EndingHexContext*ending_hex=nullptr)const{
        auto effective=actor_;
        // Actor multiplier affects direct damage only. Health loss, degeneration
        // and life steal retain their separate existing transaction lanes.
        if(owned_stalker_pve_)effective.incoming_damage_multiplier*=.67;
        effective.physical_armor=final_physical_armor(lifetime_,now);
        const auto bonus=elemental_skill_armor(now,ending_hex);
        for(auto* armor:{&effective.earth_armor,&effective.fire_armor,&effective.cold_armor,&effective.lightning_armor})if(*armor)**armor+=int(bonus);
        effective.incoming_damage_cap=skills::effects::protective_spirit_cap(hexes_,hex_scope(),now,effective.health?effective.health->maximum:0);
        if(const auto plan=skills::effects::active_stoneflesh_aura(hexes_,hex_scope(),now))
            if(!skills::effects::apply_stoneflesh_damage(effective,*plan))
                effective.incoming_reduction=std::numeric_limits<double>::quiet_NaN();
        return effective;
    }
    unsigned skill_armor_bonus(std::uint64_t now,const skills::effects::EndingHexContext*ending_hex=nullptr)const{
        const auto prior=skills::effects::stacked_skill_armor(skills::effects::protection_enchantment_armor(hexes_,hex_scope(),now),
            skills::effects::active_convert_hexes_armor(hexes_,hex_scope(),convert_hexes_armor_,now));
        const auto ordinary=skills::effects::stacked_skill_armor(prior,watch_yourself_?skills::effects::watch_yourself_armor(hexes_,*watch_yourself_,now):0);
        const auto shouting=skills::effects::stacked_skill_armor(ordinary,skills::effects::save_yourselves_armor(hexes_,hex_scope(),now));
        return skills::effects::stacked_skill_armor(shouting,weapon_spell_?skills::effects::weapon_spell_armor(*weapon_spell_,hexes_,hex_scope(),conditions_,now,ending_hex):0);
    }
    Hit apply_damage_impl(const Lifetime& expected,const Damage& damage,std::uint64_t now,bool hostile,const skills::effects::EndingHexContext*ending_hex=nullptr,
            const skills::effects::EndingStonefleshContext* ending_stoneflesh=nullptr){
        if(!matches(expected)||(energy_clock_&&now<*energy_clock_)||
           (ending_stoneflesh&&!ending_stoneflesh->plan(hex_scope(),now)))return {};
        // Apply the current condition adjustment on a copy; do not overwrite
        // the reviewed base rating and leave reduced armor after expiry.
        auto effective=incoming_damage_actor(now,ending_hex);effective.earth_armor=final_earth_armor(expected,now,ending_hex);
        effective.holy_armor=final_holy_armor(expected,now);
        effective.physical_armor=final_physical_armor(expected,now);
        if(effective.fire_armor)effective.fire_armor=conditions_.armor(*effective.fire_armor,now);
        if(effective.cold_armor)effective.cold_armor=conditions_.armor(*effective.cold_armor,now);
        if(effective.lightning_armor)effective.lightning_armor=conditions_.armor(*effective.lightning_armor,now);
        if(ending_stoneflesh&&!skills::effects::active_stoneflesh_aura(hexes_,hex_scope(),now)){
            const auto plan=ending_stoneflesh->plan(hex_scope(),now);
            if(!plan||!skills::effects::apply_stoneflesh_damage(effective,*plan))return {};
        }
        const auto absorption=hexes_.newest_skill(hex_scope(),1399,now)?absorption_.prepare(hexes_,hex_scope(),absorption_event_+1,now):std::nullopt;
        if(hexes_.newest_skill(hex_scope(),1399,now)&&!absorption)return {};
        if(absorption)effective.incoming_reduction+=absorption->reduction;
        auto adjusted=damage;if(adjusted.kind==Kind::fire)adjusted.amount*=fire_multiplier_;
        auto hit=apply(effective,adjusted,hostile);
        if(hit.reason==Reason::accepted){if(!storm_energy_hit(expected,hit,now))return {};
            if(!settle_damage_health(hit,wound_))return {};effective.health=hit.after;effective.effects=hit.effects;
            if(absorption){if(!absorption_.commit(hexes_,hex_scope(),*absorption,now))return {};++absorption_event_;}
            assign_health(effective.health);actor_.effects=effective.effects;if(hit.killed)control_.invalidate();}
        return hit;
    }
public:
    // Preserve only a verified incoming hit's counter across a separate
    // Reversal healing/removal transaction. Does not restore Health or hexes.
    bool adopt_absorption_hit(const LiveActor& before,const LiveActor& incoming,const Lifetime& expected,std::uint64_t now){
        if(!matches(expected)||!before.matches(expected)||!incoming.matches(expected))return false;
        auto authority=before.absorption_;
        if(incoming.absorption_event_==before.absorption_event_){
            if(!authority.same_state(incoming.absorption_))return false;
        }else{
            if(before.absorption_event_==UINT64_MAX||incoming.absorption_event_!=before.absorption_event_+1)return false;
            const auto quote=authority.prepare(before.hexes_,before.hex_scope(),incoming.absorption_event_,now);
            if(!quote||!authority.commit(before.hexes_,before.hex_scope(),*quote,now)||!authority.same_state(incoming.absorption_))return false;
            if(hexes_.entry_sequence(hex_scope(),quote->effect.instance)!=before.hexes_.entry_sequence(before.hex_scope(),quote->effect.instance))return false;
        }
        absorption_=std::move(authority);absorption_event_=incoming.absorption_event_;return true;
    }
    // Rerun raw damage on current authority after intervening World reactions.
    // The resulting Health, counter and registry must be committed together.
    std::optional<Hit> refresh_damage_proposal(const LiveActor& current,const Lifetime& expected,const Damage& damage,std::uint64_t now,bool interrupt=false){
        if(!matches(expected)||!current.matches(expected))return {};
        auto refreshed=current;const auto hit=refreshed.apply_damage(expected,damage,now);
        if(hit.reason!=Reason::accepted)return {};
        if(interrupt&&!hit.killed&&refreshed.active_action(expected,now)&&!refreshed.cancel_action(expected,now))return {};
        *this=std::move(refreshed);return hit;
    }
    // Damage/control proposals do not own hex mutations. Rebase that registry
    // at commit so another actor's intervening death cleanup is not undone by
    // assigning the earlier whole-actor copy. Caller must hold the World lock.
    bool preserve_current_hexes(const LiveActor& current,const Lifetime& expected,std::uint64_t now,
            const std::vector<skills::effects::ReversalFortuneQuote>& retirements={}){
        if(!matches(expected)||!current.matches(expected))return false;
        auto registry=current.hexes_;
        for(const auto& quote:retirements)if(quote.target!=hex_scope()||hexes_.entry_sequence(hex_scope(),quote.effect)||
            !skills::effects::consume_reversal_fortune(registry,quote,now))return false;
        health_loss_revision_=std::max(health_loss_revision_,current.health_loss_revision_);
        hexes_=std::move(registry);weapon_spell_=current.weapon_spell_;convert_hexes_armor_=current.convert_hexes_armor_;watch_yourself_=current.watch_yourself_;fevered_owner_=current.fevered_owner_;fevered_span_=current.fevered_span_;
        movement_=current.movement_;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);return true;
    }
    using ReversalDamage=skills::effects::ReversalDamageReceipt;
    // Trusted staging until paid307 admission and all NPC effect consumers
    // are reviewed. Unknown healing permission must not grant conversion.
    bool apply_reversal_fortune(const Lifetime& expected,unsigned rank,unsigned caster,unsigned effect,std::uint64_t now){
        const auto plan=skills::effects::reversal_fortune(rank);
        return plan&&install_reversal_fortune(expected,rank,caster,effect,now,plan->duration);
    }
    bool apply_reversal_fortune(const Lifetime& expected,skills::effects::Scope caster,
            const skills::effects::ReversalFortunePlan& plan,unsigned effect,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr){
        const auto canonical=skills::effects::reversal_fortune(plan.rank);
        const auto duration=adjusted?adjusted->for_enchantment(307,plan.duration,caster,now):std::optional(plan.duration);
        return canonical&&*canonical==plan&&caster.instance==hex_scope().instance&&caster.generation&&
            duration&&install_reversal_fortune(expected,plan.rank,caster.actor,effect,now,*duration);
    }
private:
    bool install_reversal_fortune(const Lifetime& expected,unsigned rank,unsigned caster,unsigned effect,std::uint64_t now,std::uint64_t duration){
        const auto plan=skills::effects::reversal_fortune(rank);
        if(!matches(expected)||!plan||!caster||caster>65535||!effect||healing_allowed_!=std::optional(true)||
           !duration||now>UINT64_MAX-duration||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||hexes_.lookup(hex_scope(),effect))return false;
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return false;
        if(proposed.actor_.health->current<=0)return false;
        if(!proposed.hexes_.apply(hex_scope(),{effect,caster,307,rank,skills::effects::Category::enchantment,now,now+duration},skills::effects::Replace::same_skill).accepted)return false;
        if(!proposed.condition_clock_)proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.actor_.effects,now);
        *this=std::move(proposed);return true;
    }
    // Quote after armor/protection but before Health clamping. Commit a full
    // copy only after healing and remaining loss succeed; callers publish the
    // receipt in healing-before-damage order under the same World lock.
private:
    // Only call on a private proposal; failure may consume that proposal.
    std::optional<ReversalDamage> resolve_reversal_hit(const Lifetime& expected,const Hit& incoming,std::uint64_t now,bool weapon_contact=false){
        if(!matches(expected)||healing_allowed_!=std::optional(true)||!actor_.damageable||!actor_.health||
           actor_.health->current<=0||(condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_)||
           incoming.reason!=Reason::accepted||incoming.target!=actor_.id||incoming.generation!=actor_.generation||
           (!incoming.skill&&!weapon_contact)||incoming.skill>3432||static_cast<unsigned>(incoming.kind)>static_cast<unsigned>(Kind::holy)||
           !std::isfinite(incoming.damage)||incoming.damage<=0||incoming.damage>16000000||
           incoming.before.current!=actor_.health->current||incoming.before.maximum!=actor_.health->maximum||
           incoming.after.maximum!=incoming.before.maximum||
           incoming.applied!=std::min(incoming.damage,incoming.before.current)||
           !valid_damage_health(incoming)||
           incoming.killed!=(incoming.applied>0&&incoming.after.current==0))return {};
        auto& proposed=*this;
        const auto quote=skills::effects::prepare_reversal_fortune(proposed.hexes_,proposed.hex_scope(),
            skills::effects::ReversalInput::damage,incoming.damage,now);
        if(!quote)return {};
        const auto removed=skills::effects::consume_reversal_fortune(proposed.hexes_,*quote,now);if(!removed)return {};
        const auto before=*proposed.actor_.health;
        if(!proposed.heal(expected,quote->split.healing,now))return {};
        ReversalDamage result{*removed,before,*proposed.actor_.health,{},*quote};
        proposed.actor_.effects=proposed.hexes_.presentation(proposed.hex_scope(),proposed.actor_.effects,now);
        if(proposed.actor_.health->current>0){
            auto hit=incoming;hit.damage=quote->split.remaining;hit.before=*proposed.actor_.health;hit.after=hit.before;
            hit.applied=std::min(hit.damage,hit.before.current);
            // Conversion healing has settled its own capacity event. Never
            // reuse evidence attached to the pre-conversion incoming hit.
            hit.health_settlement.reset();hit.after.current=std::max(0.,hit.before.current-hit.damage);
            hit.killed=hit.after.current<=0&&hit.applied>0;
            hit.effects=proposed.actor_.effects|(hit.killed?0x10u:0u);
            if(!settle_damage_health(hit,proposed.wound_))return {};
            if(!proposed.storm_energy_hit(expected,hit,now))return {};
            proposed.assign_health(hit.after);proposed.actor_.effects=hit.effects;
            if(hit.killed)proposed.control_.invalidate();result.damage=hit;
        }
        return result;
    }
public:
    // Trusted post-protection input,not an unvalidated network Hit. The planner
    // owns armor/multiplier provenance;do not resolve those modifiers twice.
    std::optional<ReversalDamage> apply_reversal_hit(const Lifetime& expected,const Hit& incoming,std::uint64_t now){
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return {};
        const auto result=proposed.resolve_reversal_hit(expected,incoming,now);if(!result)return {};
        *this=std::move(proposed);return result;
    }
    // Distinct trusted weapon admission permits ordinary zero attribution;
    // generic spell Hit admission remains strict. No armor/protection rerun.
    std::optional<ReversalDamage> apply_reversal_weapon_hit(const Lifetime& expected,const Hit& incoming,std::uint64_t now){
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return {};
        const auto result=proposed.resolve_reversal_hit(expected,incoming,now,true);if(!result)return {};
        *this=std::move(proposed);return result;
    }
    std::optional<ReversalDamage> apply_reversal_damage(const Lifetime& expected,const Damage& damage,
            std::uint64_t now,bool hostile,const skills::effects::EndingHexContext* ending=nullptr,
            const skills::effects::EndingStonefleshContext* ending_stoneflesh=nullptr){
        if(!matches(expected)||!hostile||healing_allowed_!=std::optional(true)||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return {};
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return {};
        if(!proposed.actor_.health||proposed.actor_.health->current<=0)return {};
        auto raw=proposed;const auto incoming=raw.apply_damage_impl(expected,damage,now,hostile,ending,ending_stoneflesh);
        const auto result=proposed.resolve_reversal_hit(expected,incoming,now);if(!result)return {};
        if(!proposed.adopt_absorption_hit(*this,raw,expected,now))return {};
        *this=std::move(proposed);return result;
    }
    // Trusted post-reduction amount;not an ordinary Damage/Hit. This only
    // resolves the recipient. World caster gain/overkill policy stays separate.
    // First half of an owned World transfer: consume conversion and settle its
    // healing before the source gains Health and before recipient loss.
    std::optional<skills::effects::ReversalLifeStealReceipt> begin_reversal_life_steal(
            const Lifetime& expected,double reduced_amount,std::uint64_t now){
        if(!matches(expected)||!hostile_to_players(expected)||healing_allowed_!=std::optional(true)||!actor_.damageable||
           !actor_.health||actor_.health->current<=0||(condition_clock_&&now<*condition_clock_)||
           (energy_clock_&&now<*energy_clock_))return {};
        auto proposed=*this;if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return {};
        if(!proposed.actor_.health||proposed.actor_.health->current<=0)return {};
        const auto quote=skills::effects::prepare_reversal_fortune(proposed.hexes_,proposed.hex_scope(),
            skills::effects::ReversalInput::life_steal,reduced_amount,now);if(!quote)return {};
        const auto removed=skills::effects::consume_reversal_fortune(proposed.hexes_,*quote,now);if(!removed)return {};
        const auto before=*proposed.actor_.health;if(!proposed.heal(expected,quote->split.healing,now))return {};
        skills::effects::ReversalLifeStealReceipt receipt{*removed,before,*proposed.actor_.health,*proposed.actor_.health,*quote,0};
        proposed.actor_.effects=proposed.hexes_.presentation(proposed.hex_scope(),proposed.actor_.effects,now);
        if(proposed.actor_.health->current<=0){proposed.actor_.effects|=0x10;proposed.control_.invalidate();}
        *this=std::move(proposed);return receipt;
    }
    // Life-steal loss is neither damage nor degeneration. It settles pending
    // capacity debt without armor, damage reactions or healing modifiers.
    std::optional<Health> apply_life_steal_loss(const Lifetime& expected,double amount,std::uint64_t now){
        if(!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||
           actor_.health->current<=0||!std::isfinite(amount)||amount<0||amount>16000000||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return {};
        if(!amount)return *actor_.health;
        auto hp=actor_.health->current;
        if(wound_.base)wound_.event(hp,-amount);else hp=std::max(0.,hp-amount);
        assign_health(Health{hp,actor_.health->maximum});condition_clock_=now;
        if(hp<=0){actor_.effects|=0x10;control_.invalidate();}
        return *actor_.health;
    }
    std::optional<skills::effects::ReversalLifeStealReceipt> apply_reversal_life_steal(
            const Lifetime& expected,double reduced_amount,std::uint64_t now){
        auto proposed=*this;auto receipt=proposed.begin_reversal_life_steal(expected,reduced_amount,now);if(!receipt)return {};
        if(proposed.actor_.health->current>0){
            const auto health=proposed.apply_life_steal_loss(expected,receipt->quote.split.remaining,now);if(!health)return {};
            receipt->after_loss=*health;receipt->lost=receipt->after_healing.current-health->current;
        }
        *this=std::move(proposed);return receipt;
    }
    Hit apply_damage(const Lifetime& expected,const Damage& damage,std::uint64_t now,const skills::effects::EndingHexContext*ending_hex=nullptr,
            const skills::effects::EndingStonefleshContext* ending_stoneflesh=nullptr){
        return apply_damage_impl(expected,damage,now,hostile_to_players(expected),ending_hex,ending_stoneflesh);
    }
    Hit apply_removal_damage(const Lifetime& expected,const Damage& damage,std::uint64_t now,
            const skills::effects::EndingStonefleshContext* ending){
        if(ending&&!ending->plan(hex_scope(),now))return {};
        return apply_damage_impl(expected,damage,now,hostile_to_players(expected),nullptr,ending);
    }
    // Trusted World caller has already proven that the owner is on an
    // opposing team. The NPC itself cannot infer player-team hostility.
    Hit apply_opposed_owned_damage(const Lifetime& expected,const Damage& damage,
            std::uint64_t now){
        if(!matches(expected)||relation_!=Relation::allied_to_players)return {};
        return apply_damage_impl(expected,damage,now,true);
    }
private:
    struct ProtectedDamageSlice {DamageStep step;std::optional<ReversalDamage> reversal;};
    // Only invoke on a disposable transaction-local actor. Keep the supplied
    // slice armor intact; the owning skill decides its condition ordering.
    std::optional<ProtectedDamageSlice> resolve_protected_slice(const Lifetime& expected,
            Actor& actor,const Damage& damage,std::uint64_t now,bool convert_reversal){
            if(!actor.health||!actor_.health||actor.health->current!=actor_.health->current||
               actor.health->maximum!=actor_.health->maximum)return {};
            const auto quote=hexes_.newest_skill(hex_scope(),1399,now)?
                absorption_.prepare(hexes_,hex_scope(),absorption_event_+1,now):std::nullopt;
            if(hexes_.newest_skill(hex_scope(),1399,now)&&!quote)return {};
            auto effective=actor;if(quote)effective.incoming_reduction+=quote->reduction;
            auto incoming=apply(effective,damage,true);if(incoming.reason!=Reason::accepted)return {};
            auto capacity=wound_;if(!settle_damage_health(incoming,capacity))return {};
            std::optional<Hit> actual=incoming;std::optional<ReversalDamage> reversal;
            if(convert_reversal&&incoming.damage>0&&hexes_.newest_skill(hex_scope(),307,now)){
                const auto receipt=apply_reversal_hit(expected,incoming,now);if(!receipt)return {};
                reversal=receipt;actual=receipt->damage;
            }else{
                if(!storm_energy_hit(expected,incoming,now))return {};
                wound_=capacity;assign_health(incoming.after);actor_.effects=incoming.effects;
                if(incoming.killed)control_.invalidate();
            }
            if(quote){if(!absorption_.commit(hexes_,hex_scope(),*quote,now))return {};++absorption_event_;}
            actor.health=actor_.health;actor.effects=actor_.effects;
            return ProtectedDamageSlice{DamageStep{incoming,actual},reversal};
    }
public:
    std::optional<skills::effects::ElementalResult> apply_elemental(const Lifetime& expected,
            const skills::effects::ElementalPlan& plan,unsigned level,unsigned caster,std::uint64_t now,bool convert_reversal=false,
            skills::effects::SpiritProximity spirits=skills::effects::SpiritProximity::unknown){
        if(!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return {};
        const auto enchantments=plan.half_if_enchanted?enchantment_count(expected,now):std::optional<unsigned>{};
        auto proposed=*this;std::vector<std::optional<ReversalDamage>> reversals;
        DamageProjection projection=[&](Actor& actor,const Damage& damage)->std::optional<DamageStep>{
            const auto slice=proposed.resolve_protected_slice(expected,actor,damage,now,convert_reversal);
            if(!slice)return {};reversals.push_back(slice->reversal);return slice->step;
        };
        auto result=skills::effects::elemental_target(plan,incoming_damage_actor(now),conditions_,level,caster,now,
            skills::effects::water_hex(hexes_,hex_scope(),now),fire_multiplier_,conditions_allowed_,
            enchantments?std::optional<bool>{*enchantments!=0}:std::nullopt,projection,spirits);
        if(!result.accepted)return {};
        if(skills::effects::active_frigid_armor(hexes_,hex_scope(),now)){
            result.conditions.remove(conditions::Kind::burning);
            if(const auto old=conditions_.lookup(conditions::Kind::burning);old&&old->expires>now)
                result.conditions.apply(conditions::Kind::burning,old->cause,old->applied,old->expires-old->applied);
            std::erase(result.additions,conditions::Kind::burning);
        }
        result.reversals=std::move(reversals);
        const auto before=proposed.conditions_;
        proposed.assign_health(result.actor.health);proposed.actor_.effects=result.actor.effects;proposed.conditions_=result.conditions;
        proposed.sync_wound(now);proposed.react_fragility(before,now);proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(hex_scope(),proposed.conditions_.presentation(proposed.actor_.effects,now),now);
        if(proposed.actor_.health->current<=0)proposed.control_.invalidate();
        *this=std::move(proposed);return result;
    }
    std::optional<skills::effects::ShockwaveResult> apply_earth_aoe(const Lifetime& expected,
            const skills::effects::AoEPlan& plan,double distance,unsigned level,unsigned caster,std::uint64_t now){
        if(!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||(energy_clock_&&now<*energy_clock_))return {};
        const skills::effects::ShockwaveTarget input{incoming_damage_actor(now),conditions_};
        auto proposed=*this;std::vector<std::optional<ReversalDamage>> reversals;
        std::optional<Hit> previous_actual;bool previous_resolved=false;
        ArmorSequence armor_at_hit;
        if(watch_yourself_&&actor_.earth_armor&&skills::effects::watch_yourself_armor(hexes_,*watch_yourself_,now)){
            const auto ordinary=skills::effects::stacked_skill_armor(skills::effects::protection_enchantment_armor(hexes_,hex_scope(),now),
                skills::effects::active_convert_hexes_armor(hexes_,hex_scope(),convert_hexes_armor_,now));
            const auto other=skills::effects::stacked_skill_armor(ordinary,environmental_elemental_armor_?environmental_elemental_armor_(hex_scope(),now):0);
            // Preview receipts are private copies. World records the real hits
            // only after the complete recipient batch has passed admission.
            armor_at_hit=[owned=*watch_yourself_,registry=hexes_,base=*actor_.earth_armor,other,now,&previous_actual,&previous_resolved](const Hit* previous)mutable->std::optional<int>{
                if(previous&&previous_resolved)previous=previous_actual?&*previous_actual:nullptr;
                if(previous&&owned.remaining&&previous->reason==Reason::accepted&&previous->applied>0&&previous->kind!=Kind::armor_ignoring){
                    if(owned.last_damage==UINT64_MAX)return {};
                    const auto next=skills::effects::consume_watch_yourself(registry,owned,{owned.target,owned.last_damage+1,now,true,true,previous->applied});
                    if(!next)return {};owned=next->owned;
                }
                return base+int(skills::effects::stacked_skill_armor(other,skills::effects::watch_yourself_armor(registry,owned,now)));
            };
        }
        skills::effects::EarthDamageResolver damage_at_hit=[&](Actor& actor,const Damage& damage){
            const auto slice=proposed.resolve_protected_slice(expected,actor,damage,now,true);
            if(!slice)return Hit{};reversals.push_back(slice->reversal);
            previous_actual=slice->step.projected;previous_resolved=true;return slice->step.incoming;
        };
        skills::effects::EarthConditionReaction condition_at_hit=[&](skills::effects::ShockwaveTarget& target,conditions::Kind){
            const auto before=proposed.conditions_;proposed.conditions_=target.conditions;
            proposed.sync_wound(now);proposed.react_fragility(before,now);proposed.condition_clock_=now;
            target.actor.health=proposed.actor_.health;target.actor.effects=proposed.actor_.effects;return true;
        };
        auto result=plan.skill==174?skills::effects::aftershock_target(plan,input,distance,level,caster,now,true,control_.down(now),armor_at_hit,damage_at_hit):
            skills::effects::shockwave_target(plan,input,distance,level,caster,now,true,armor_at_hit,damage_at_hit,condition_at_hit);
        if(!result.accepted||(!result.applied_conditions.empty()&&!conditions_allowed_))return {};
        result.reversals=std::move(reversals);
        proposed.assign_health(result.after.actor.health);proposed.actor_.effects=result.after.actor.effects;
        proposed.conditions_=result.after.conditions;proposed.sync_wound(now);
        if(!result.applied_conditions.empty())proposed.condition_clock_=now;
        proposed.actor_.effects=proposed.hexes_.presentation(proposed.hex_scope(),proposed.actor_.effects,now);
        if(proposed.actor_.health->current<=0)proposed.control_.invalidate();
        *this=std::move(proposed);return result;
    }
    // Hex-only followup after a separately committed Ash Blast damage event.
    // The caller carries the impact-time Burning qualification across damage
    // reactions, while this primitive validates current life/Health/registry.
    std::optional<skills::effects::Applied> apply_ash_blast_hex(
            const Lifetime& expected,const skills::effects::AshBlastPlan& plan,
            skills::effects::Scope caster,unsigned effect,std::uint64_t now,
            bool burning_at_impact){
        if(!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||
           !actor_.health||actor_.health->current<=0||
           !condition_clock_||now!=*condition_clock_||(energy_clock_&&now<*energy_clock_))return {};
        const auto proposed=skills::effects::prepare_ash_blast_hex(plan,caster,hex_scope(),
            effect,now,hexes_,true,burning_at_impact);
        if(!proposed)return {};
        hexes_=proposed->registry;
        actor_.effects=hexes_.presentation(hex_scope(),actor_.effects,now);
        return skills::effects::Applied{true,proposed->removed};
    }
    // Legacy copied-recipient diagnostic. World uses shared damage followed
    // by apply_ash_blast_hex, never this aggregate actor-state transaction.
    std::optional<skills::effects::AshBlastTarget> apply_ash_blast(
            const Lifetime& expected,const skills::effects::AshBlastPlan& plan,
            skills::effects::Scope caster,unsigned effect,unsigned caster_level,
            float distance_from_center,std::uint64_t now){
        if(!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||
           !actor_.health||actor_.health->current<=0||
           (condition_clock_&&now<*condition_clock_)||
           (energy_clock_&&now<*energy_clock_))return {};
        auto proposed=*this;
        if(proposed.condition_clock_&&!proposed.advance_conditions(expected,now))return {};
        if(!proposed.actor_.health||proposed.actor_.health->current<=0)return {};
        auto incoming=proposed.incoming_damage_actor(now);
        incoming.earth_armor=proposed.final_earth_armor(expected,now);
        const auto absorption=proposed.hexes_.newest_skill(proposed.hex_scope(),1399,now)?
            proposed.absorption_.prepare(proposed.hexes_,proposed.hex_scope(),proposed.absorption_event_+1,now):std::nullopt;
        if(proposed.hexes_.newest_skill(proposed.hex_scope(),1399,now)&&!absorption)return {};
        if(absorption)incoming.incoming_reduction+=absorption->reduction;
        auto result=skills::effects::prepare_ash_blast_target(plan,caster,
            proposed.hex_scope(),effect,now,proposed.hexes_,proposed.conditions_,
            std::move(incoming),caster_level,true,
            distance_from_center);
        if(!result||!proposed.storm_energy_hit(expected,result->hit,now))return {};
        if(absorption){if(!proposed.absorption_.commit(proposed.hexes_,proposed.hex_scope(),*absorption,now))return {};++proposed.absorption_event_;}
        proposed.assign_health(result->actor.health);
        proposed.hexes_=result->registry;
        proposed.actor_.effects=proposed.hexes_.presentation(
            proposed.hex_scope(),result->actor.effects,now);
        if(result->hex&&!proposed.condition_clock_)proposed.condition_clock_=now;
        if(proposed.actor_.health->current<=0)proposed.control_.invalidate();
        *this=std::move(proposed);
        return result;
    }
    std::optional<skills::effects::BalanceResult> apply_grenths_balance(const Lifetime& expected,
            const Health& caster,std::uint64_t now,bool reaction_killed_caster=false){
        if(!matches(expected)||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||
           actor_.health->current<=0||(condition_clock_&&now<*condition_clock_))return {};
        sync_wound(now);
        auto result=skills::effects::grenths_balance(caster,*actor_.health,reaction_killed_caster);if(!result)return {};
        auto wound=wound_;if(!wound.base)wound.reset(static_cast<unsigned>(actor_.health->maximum));
        auto health=actor_.health->current;
        wound.event(health,result->target_gain-result->target_loss,false);
        const auto delta=health-actor_.health->current;
        result->target={health,actor_.health->maximum};
        result->target_gain=std::max(0.,delta);result->target_loss=std::max(0.,-delta);
        wound_=std::move(wound);assign_health(result->target);condition_clock_=now;
        if(health<=0){actor_.effects|=0x10;control_.invalidate();}
        return result;
    }
    std::optional<skills::effects::LifeStealResult> apply_life_steal(const Lifetime& expected,
        const skills::effects::LifeStealPlan& plan,const Health& source,std::optional<std::uint64_t> now={}){
        if(!hostile_to_players(expected)||!actor_.damageable||!actor_.health)return {};
        const auto reduction=now?life_steal_reduction(expected,*now):std::optional<double>(0);
        if(!reduction)return {};
        auto result=skills::effects::steal_life(plan,source,*actor_.health,*reduction);
        if(!result)return {};
        if(now){
            const auto health=apply_life_steal_loss(expected,result->stolen,*now);if(!health)return {};
            result->target=*health;result->killed=health->current<=0;
        }else assign_health(result->target);
        if(result->killed){actor_.effects|=0x10;control_.invalidate();}
        return result;
    }
    // World must first remove at least one enchantment. Keep this conditional
    // plan out of the unconditional life-steal skill dispatcher.
    std::optional<skills::effects::LifeStealResult> apply_strip_enchantment(const Lifetime& expected,
            const skills::effects::EnchantmentRemovalPlan& plan,const Health& source,std::uint64_t now){
        if(plan.skill!=143||!skills::effects::valid_enchantment_removal(plan)||!plan.count||
           !hostile_to_players(expected)||!actor_.damageable||!actor_.health||
           (condition_clock_&&now<*condition_clock_))return {};
        const auto reduction=life_steal_reduction(expected,now);if(!reduction)return {};
        auto result=skills::effects::steal_life_amount(plan.steal,source,*actor_.health,*reduction);
        if(!result)return {};
        if(result->stolen>0&&hexes_.newest_skill(hex_scope(),307,now))return {};
        const auto settled=apply_life_steal_loss(expected,result->stolen,now);if(!settled)return {};
        result->target=*settled;result->killed=settled->current<=0;
        return result;
    }
    std::optional<double> life_steal_reduction(const Lifetime& expected,std::uint64_t now)const{
        if(!matches(expected)||(condition_clock_&&now<*condition_clock_))return {};
        const auto effective=incoming_damage_actor(now);
        return effective.life_steal_reduction.value_or(effective.incoming_reduction);
    }
    // Typed Feast life-steal-only phase. World owns the saved pre-shadow hex
    // qualification and publishes source gain through its current authority.
    // This primitive neither resolves shadow damage nor consumes1399.
    std::optional<skills::effects::LifeStealResult> apply_feast_followup(const Lifetime& expected,
            const skills::effects::FeastOfCorruptionPlan& plan,const Health& source,std::uint64_t now){
        const auto canonical=skills::effects::feast_of_corruption(plan.rank);
        if(!canonical||*canonical!=plan||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||
           actor_.health->current<=0||(condition_clock_&&now<*condition_clock_)||
           (energy_clock_&&now<*energy_clock_))return {};
        const auto actor=incoming_damage_actor(now);
        auto result=skills::effects::steal_life_amount(plan.steal,source,*actor_.health,
            actor.life_steal_reduction.value_or(actor.incoming_reduction));
        if(!result)return {};
        // World must use the separate typed conversion transaction while307
        // remains active; never silently steal through its conversion lease.
        if(result->stolen>0&&hexes_.newest_skill(hex_scope(),307,now))return {};
        auto hp=actor_.health->current;
        if(wound_.base)wound_.event(hp,-result->stolen);else hp=result->target.current;
        result->target={hp,actor_.health->maximum};result->killed=hp<=0;
        assign_health(result->target);condition_clock_=now;
        if(result->killed){actor_.effects|=0x10;control_.invalidate();}
        return result;
    }
    // A World-validated above-half shadow follow-up, after its damage phase.
    // Re-read current protection/Health;never assign an old combined result.
    std::optional<skills::effects::LifeStealResult> apply_shadow_followup(const Lifetime& expected,
            const skills::effects::ShadowPlan& plan,const Health& source,std::uint64_t now){
        const auto canonical=skills::effects::shadow_damage(plan.skill,plan.rank);
        if(!canonical||*canonical!=plan||!plan.steal_above_half||!hostile_to_players(expected)||!actor_.damageable||!actor_.health||
           (condition_clock_&&now<*condition_clock_))return {};
        const auto actor=incoming_damage_actor(now);
        auto result=skills::effects::steal_life_amount(plan.steal_above_half,source,*actor_.health,
            actor.life_steal_reduction.value_or(actor.incoming_reduction));if(!result)return {};
        // World owns the separate staged conversion/source-gain transaction.
        if(result->stolen>0&&hexes_.newest_skill(hex_scope(),307,now))return {};
        auto hp=actor_.health->current;if(wound_.base)wound_.event(hp,-result->stolen);else hp=result->target.current;
        result->target={hp,actor_.health->maximum};result->killed=hp<=0;assign_health(result->target);condition_clock_=now;
        if(result->killed){actor_.effects|=0x10;control_.invalidate();}return result;
    }
    std::optional<skills::effects::ShadowResult> apply_shadow_damage(const Lifetime& expected,
        const skills::effects::ShadowPlan& plan,const Health& source,unsigned level,std::optional<std::uint64_t> at={}){
        if(!hostile_to_players(expected)||!actor_.damageable||!actor_.health||(at&&condition_clock_&&*at<*condition_clock_))return {};
        auto result=skills::effects::shadow_target(plan,source,incoming_damage_actor(at.value_or(condition_clock_.value_or(0))),level);
        if(!result)return {};
        assign_health(result->target.health);actor_.effects=result->target.effects;
        if(!actor_.health->current){actor_.effects|=0x10;control_.invalidate();}
        result->target.effects=actor_.effects;
        return result;
    }
    // Trusted world AI only: a living hostile source may damage a reviewed
    // neutral guardian. Player hostility and protected-service policy stay intact.
    Hit apply_guardian_damage(const Lifetime& expected,const Damage& damage,std::uint64_t now){
        if(!matches(expected)||relation_!=Relation::neutral)return {};
        return apply_damage_impl(expected,damage,now,true);
    }
    std::optional<Hit> apply_guardian_weapon_damage(const Lifetime& expected,double damage,std::optional<std::uint64_t> at={}){
        if(!matches(expected)||relation_!=Relation::neutral)return {};
        if(at&&condition_clock_&&*at<*condition_clock_)return {};
        return apply_resolved_weapon_damage(expected,damage,0,at.value_or(condition_clock_.value_or(0)),true);
    }
};

namespace detail {
inline bool sha256(std::string_view text){
    return text.size()==64&&std::all_of(text.begin(),text.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});
}
inline std::uint32_t number(std::string_view text,std::uint32_t maximum=0xffffffffu){
    std::uint32_t out=0;const auto result=std::from_chars(text.data(),text.data()+text.size(),out);
    if(result.ec!=std::errc{}||result.ptr!=text.data()+text.size()||out>maximum)
        throw std::invalid_argument("invalid unsigned field");
    return out;
}
inline int armor(std::string_view text){
    int out=0;const auto result=std::from_chars(text.data(),text.data()+text.size(),out);
    if(result.ec!=std::errc{}||result.ptr!=text.data()+text.size()||out< -100||out>500)
        throw std::invalid_argument("invalid earth armor field");
    return out;
}
inline double real(std::string_view text){
    double out=0;const auto result=std::from_chars(text.data(),text.data()+text.size(),out);
    if(result.ec!=std::errc{}||result.ptr!=text.data()+text.size()||!std::isfinite(out))
        throw std::invalid_argument("invalid spawn health field");
    return out;
}
inline bool token(std::string_view text){
    return !text.empty()&&text.size()<=64&&std::all_of(text.begin(),text.end(),[](char c){
        return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-';});
}
}
class Catalog {
    std::string population_sha256_;
    std::map<std::string,Source> sources_;
    std::map<std::pair<std::uint32_t,std::uint32_t>,Definition> entries_;
    friend Catalog load_catalog(std::istream&,std::string_view);
public:
    const std::string& population_sha256()const{return population_sha256_;}
    std::size_t size()const{return entries_.size();}
    const std::map<std::string,Source>& sources()const{return sources_;}
    const Definition* find(std::uint32_t map,std::uint32_t actor)const{
        const auto entry=entries_.find({map,actor});return entry==entries_.end()?nullptr:&entry->second;
    }
    const Definition* bind(const Observed& observed,std::string_view sha256)const{
        if(sha256!=population_sha256_)throw std::invalid_argument("NPC combat population revision mismatch");
        const auto* definition=find(observed.binding.map,observed.binding.actor);
        if(!definition)return nullptr;
        if(definition->binding!=observed.binding)
            throw std::invalid_argument("NPC combat row does not match installed appearance");
        if(observed.protected_service&&(definition->relation==Relation::hostile_to_players||
           definition->damageable||definition->conditions_allowed))
            throw std::invalid_argument("service NPC cannot be enabled for combat");
        return definition;
    }
    // Validate every row against the installed population BEFORE publishing any
    // live states. A rejected replacement cannot mutate the current instance.
    void validate_population(std::span<const Observed> population,std::string_view sha256)const{
        if(sha256!=population_sha256_)throw std::invalid_argument("NPC combat population revision mismatch");
        std::map<std::pair<std::uint32_t,std::uint32_t>,const Observed*> observed;
        for(const auto& actor:population){
            if(!actor.binding.map||actor.binding.map>65535||!actor.binding.actor||actor.binding.actor>65535||
               !observed.emplace(std::pair{actor.binding.map,actor.binding.actor},&actor).second)
                throw std::invalid_argument("duplicate or invalid observed NPC binding");
        }
        for(const auto& [key,definition]:entries_){
            const auto entry=observed.find(key);
            if(entry==observed.end())throw std::invalid_argument("NPC combat actor is not installed");
            bind(*entry->second,sha256);
        }
    }
};
inline Catalog load_catalog(std::istream& input,std::string_view expected_population_sha256){
    if(!detail::sha256(expected_population_sha256))throw std::invalid_argument("invalid expected NPC population SHA256");
    Catalog out;std::string line;std::size_t line_number=0;bool header=false;unsigned version=0;
    while(std::getline(input,line)){
        ++line_number;
        try{
            if(line.size()>4096)throw std::invalid_argument("line too long");
            if(line.find_first_not_of(" \t\r")==line.npos||line[line.find_first_not_of(" \t\r")]=='#')continue;
            std::istringstream row(line);std::vector<std::string> f;std::string field;
            for(;;){
                row>>std::ws;if(row.eof())break;
                if(!(row>>std::quoted(field)))throw std::invalid_argument("invalid quoted field");
                f.push_back(field);
            }
            if(f.empty())throw std::invalid_argument("empty NPC combat row");
            if(!header){
                if(f.size()!=3||f[0]!="gw-npc-combat"||(f[1]!="1"&&f[1]!="2"&&f[1]!="3"&&f[1]!="4"&&f[1]!="5"&&f[1]!="6"&&f[1]!="7"&&f[1]!="8"&&f[1]!="9"&&f[1]!="10"&&f[1]!="11"&&f[1]!="12"&&f[1]!="13"&&f[1]!="14"&&f[1]!="15"&&f[1]!="16"&&f[1]!="17")||
                   f[2]!=expected_population_sha256)throw std::invalid_argument("NPC combat header/revision mismatch");
                out.population_sha256_=f[2];version=detail::number(f[1]);header=true;continue;
            }
            if(f[0]=="source"){
                if(f.size()!=5||!detail::token(f[1])||f[1]=="-"||!detail::sha256(f[3])||
                   (f[2]!="capture"&&f[2]!="wiki"&&f[2]!="authored")||f[4].empty()||f[4].size()>1024||
                   std::any_of(f[4].begin(),f[4].end(),[](unsigned char c){return c<32||c==127;}))
                    throw std::invalid_argument("invalid evidence source");
                if(!out.sources_.emplace(f[1],Source{f[2],f[3],f[4]}).second||out.sources_.size()>4096)
                    throw std::invalid_argument("duplicate or excessive evidence sources");
                continue;
            }
            if(f.size()!=(version>=16?54u:version>=14?52u:version==13?50u:version==12?48u:version==11?46u:version==10?44u:version==9?42u:version==8?38u:version==7?36u:version==6?33u:version==5?31u:version==4?29u:version==3?27u:version==2?25u:21u)||f[0]!="npc")throw std::invalid_argument("invalid NPC combat field count");
            Definition d;auto& b=d.binding;
            b.map=detail::number(f[1],65535);b.actor=detail::number(f[2],65535);b.definition=detail::number(f[3],65535);
            b.model_file=detail::number(f[4]);b.skin_file=detail::number(f[5]);b.appearance=detail::number(f[6]);
            b.definition_flags=detail::number(f[7]);b.level=static_cast<std::uint8_t>(detail::number(f[8],255));
            if(!b.map||!b.actor)throw std::invalid_argument("zero NPC combat identity");
            if(f[9]=="unknown")d.relation=Relation::unknown;
            else if(f[9]=="protected")d.relation=Relation::protected_ally;
            else if(f[9]=="neutral")d.relation=Relation::neutral;
            else if(f[9]=="hostile-to-players")d.relation=Relation::hostile_to_players;
            else if(version>=15&&f[9]=="allied-to-players")d.relation=Relation::allied_to_players;
            else throw std::invalid_argument("invalid reviewed player relation");
            if(f[10]!="-")d.packed_allegiance=detail::number(f[10]);
            if((f[11]=="-")!=(f[12]=="-"))throw std::invalid_argument("health capacity/spawn health must be reviewed together");
            if(f[11]!="-"){
                d.spawn_health=Health{detail::real(f[12]),detail::number(f[11],1000000)};
                if(!valid(*d.spawn_health)||!d.spawn_health->current)throw std::invalid_argument("invalid reviewed spawn health");
            }
            if(f[13]!="-")d.earth_armor=detail::armor(f[13]);
            d.damageable=detail::number(f[14],1)!=0;d.conditions_allowed=detail::number(f[15],1)!=0;
            d.evidence={f[16],f[17],f[18],f[19],f[20]};
            if(version>=2){
                if(f[21]!="-")d.fire_armor=detail::armor(f[21]);
                if(f[22]!="-")d.cold_armor=detail::armor(f[22]);
                d.fire_armor_evidence=f[23];d.cold_armor_evidence=f[24];
            }
            if(version>=3){
                if(f[25]!="-")d.physical_armor=detail::armor(f[25]);
                d.physical_armor_evidence=f[26];
            }
            if(version>=4){d.fire_multiplier=detail::number(f[27],4);d.fire_multiplier_evidence=f[28];
                if(!d.fire_multiplier)throw std::invalid_argument("zero fire multiplier");}
            if(version>=5){if(f[29]!="-")d.lightning_armor=detail::armor(f[29]);d.lightning_armor_evidence=f[30];}
            if(version>=6){if(f[31]!="-")d.general_armor=detail::armor(f[31]);d.general_armor_evidence=f[32];}
            if(version>=7){
                if((f[33]=="-")!=(f[34]=="-"))throw std::invalid_argument("partial NPC energy");
                if(f[33]!="-"){d.spawn_energy=Energy{detail::number(f[33],1000000),detail::number(f[34],1000000)};
                    if(!d.spawn_energy->maximum||d.spawn_energy->current>d.spawn_energy->maximum)throw std::invalid_argument("invalid NPC energy");}
                d.energy_evidence=f[35];
            }
            if(version>=8){if(f[36]!="-")d.energy_pips=detail::number(f[36],10);d.energy_pips_evidence=f[37];
                if(d.energy_pips&&!d.spawn_energy)throw std::invalid_argument("NPC regeneration without energy pool");}
            if(version>=9){
                if(f[38]!="-")d.holy_traits.light_sensitive=detail::number(f[38],1)!=0;
                d.light_sensitive_evidence=f[39];
                if(f[40]!="-")d.holy_traits.summoned=detail::number(f[40],1)!=0;
                d.summoned_evidence=f[41];
            }
            if(version>=10){if(f[42]!="-")d.knockdown_allowed=detail::number(f[42],1)!=0;d.knockdown_evidence=f[43];}
            if(version>=11){if(f[44]!="-")d.healing_allowed=detail::number(f[44],1)!=0;d.healing_evidence=f[45];}
            if(version>=12){if(f[46]!="-")d.fleshy=detail::number(f[46],1)!=0;d.fleshy_evidence=f[47];}
            if(version>=14){if(f[50]!="-")d.disease_immune=detail::number(f[50],1)!=0;d.disease_immunity_evidence=f[51];}
            if(version>=16){if(f[52]!="-")d.servant_of_abaddon=detail::number(f[52],1)!=0;d.servant_of_abaddon_evidence=f[53];}
            if(version>=13){if(f[48]!="-"){if(!creatures::valid_type(f[48]))throw std::invalid_argument("invalid NPC creature type key");d.creature_type=f[48];}d.creature_type_evidence=f[49];}
            if(d.damageable&&!d.spawn_health)throw std::invalid_argument("damageable NPC has no reviewed health");
            if(d.conditions_allowed&&!d.spawn_health)throw std::invalid_argument("conditions require reviewed living health");
            if(d.relation==Relation::protected_ally&&(d.damageable||d.conditions_allowed))
                throw std::invalid_argument("protected NPC cannot receive combat effects");
            if(!out.entries_.emplace(std::pair{b.map,b.actor},std::move(d)).second||out.entries_.size()>65536)
                throw std::invalid_argument("duplicate or excessive NPC combat definitions");
        }catch(const std::exception& error){throw std::invalid_argument("NPC combat catalog line "+std::to_string(line_number)+": "+error.what());}
    }
    if(input.bad()||!header)throw std::invalid_argument("incomplete NPC combat catalog");
    auto evidence=[&](const std::string& reference,bool required){
        if(required?(reference=="-"||!out.sources_.contains(reference)):(reference!="-"))
            throw std::invalid_argument("missing, unknown or inapplicable NPC combat evidence");
    };
    for(const auto& [key,d]:out.entries_){
        evidence(d.evidence.relation,d.relation!=Relation::unknown);
        evidence(d.evidence.health,d.spawn_health.has_value());evidence(d.evidence.armor,d.earth_armor.has_value());
        evidence(d.evidence.allegiance,d.packed_allegiance.has_value());evidence(d.evidence.conditions,d.conditions_allowed);
        evidence(d.fire_armor_evidence,d.fire_armor.has_value());evidence(d.cold_armor_evidence,d.cold_armor.has_value());
        evidence(d.physical_armor_evidence,d.physical_armor.has_value());
        evidence(d.fire_multiplier_evidence,d.fire_multiplier!=1);
        evidence(d.lightning_armor_evidence,d.lightning_armor.has_value());
        evidence(d.general_armor_evidence,d.general_armor.has_value());
        evidence(d.energy_evidence,d.spawn_energy.has_value());
        evidence(d.energy_pips_evidence,d.energy_pips.has_value());
        evidence(d.light_sensitive_evidence,d.holy_traits.light_sensitive.has_value());
        evidence(d.summoned_evidence,d.holy_traits.summoned.has_value());
        evidence(d.knockdown_evidence,d.knockdown_allowed.has_value());
        evidence(d.healing_evidence,d.healing_allowed.has_value());
        evidence(d.fleshy_evidence,d.fleshy.has_value());
        evidence(d.creature_type_evidence,d.creature_type.has_value());
        evidence(d.disease_immunity_evidence,d.disease_immune.has_value());
        evidence(d.servant_of_abaddon_evidence,d.servant_of_abaddon.has_value());
    }
    return out;
}
}
