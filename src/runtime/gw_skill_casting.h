#include "gw_completion_publication.h"
#include "gw_pet_lifecycle.h"
#include "gw_pet_cast_lease.h"
#include "gw_glimmering_mark.h"
#include "gw_weapon_preparations.h"
#include "gw_fevered_dreams.h"
#include "gw_wandering_eye.h"
#include "gw_unnatural_signet.h"
#include "gw_keystone_recharge.h"
#include "gw_flourish.h"
#include "gw_bestial_fury.h"
#include "gw_cooldown_bar_authority.h"
#include "gw_recall_feedback.h"
#include "gw_recall.h"
#include "gw_determined_delivery.h"
#include "gw_mind_wrack.h"
#include "gw_panic.h"
#include "gw_armor_earth.h"
#include "gw_mantra_recall.h"
#include "gw_healing_ring.h"
#include "gw_extinguish.h"
#include "gw_martyr.h"
#include "gw_conjures.h"
#include "gw_cry_frustration.h"
#include "gw_illusion_haste.h"
#include "gw_burning_speed.h"
#include "gw_frigid_armor.h"
#include "gw_stoneflesh_aura.h"
#include "gw_slippery_ground.h"
#include "gw_succor.h"
#include "gw_enchantment_removal.h"
#include "gw_drain_delusions.h"
#include "gw_imbue_health.h"
#include "gw_channeling.h"
#include "gw_hex_copy_session.h"
#include "gw_temporary_skill_restoration.h"
#pragma once
#include "gw_weapon_intent.h"
#include "gw_weapon_attack_skills.h"
#include "gw_attack_admission.h"
#include "gw_dagger_paid_completion_draft.h"
#include "gw_weapon_hit.h"
#include "gw_character_skills.h"
#include "gw_attributes.h"
#include "gw_vitals.h"
#include "gw_skill_effect_plan.h"
#include "gw_necrosis.h"
#include "gw_energy_blast.h"
#include "gw_aneurysm.h"
#include "gw_ear_bite.h"
#include "gw_well_corpse_cast.h"
#include "gw_heal_effects.h"
#include "gw_gift_health.h"
#include "gw_party_defense_shouts.h"
#include "gw_save_yourselves.h"
#include "gw_signet_of_corruption.h"
#include "gw_quickening_zephyr.h"
#include "gw_skill_particle_wire.h"
#include "gw_health_regeneration.h"
#include "gw_party_healing.h"
#include "gw_resurrection.h"
#include "gw_lightning.h"
#include "gw_lightning_javelin.h"
#include "gw_deathly_swarm.h"
#include "gw_blindness.h"
#include "gw_condition_transfer.h"
#include "gw_condition_spells.h"
#include "gw_virulence.h"
#include "gw_rotting_flesh.h"
#include "gw_tainted_flesh.h"
#include "gw_spiteful_spirit.h"
#include "gw_energy_enchantment.h"
#include "gw_life_stealing.h"
#include "gw_elemental_damage.h"
#include "gw_fire_storm.h"
#include "gw_energy_drain_skills.h"
#include "gw_resource_gain.h"
#include "gw_interrupt_skills.h"
#include "gw_signet_humility.h"
#include "gw_hex_breaker.h"
#include "gw_diversion.h"
#include "gw_empathy.h"
#include "gw_attunements.h"
#include "gw_aura_restoration.h"
#include "gw_protective_spirit.h"
#include "gw_reversal_fortune.h"
#include "gw_shield_deflection.h"
#include "gw_vigorous_spirit.h"
#include "gw_healing_seed.h"
#include "gw_shield_regeneration.h"
#include "gw_shield_judgment.h"
#include "gw_signet_judgment.h"
#include "gw_signet_sorrow.h"
#include "gw_blessed_signet.h"
#include "gw_to_limit.h"
#include "gw_endure_pain.h"
#include "gw_stamina_signet.h"
#include "gw_defy_pain.h"
#include "gw_watch_yourself.h"
#include "gw_charge.h"
#include "gw_convert_hexes.h"
#include "gw_mending.h"
#include "gw_retribution.h"
#include "gw_frenzy.h"
#include "gw_tiger_stance.h"
#include "gw_faintheartedness.h"
#include "gw_ward_against_elements.h"
#include "gw_defensive_ground_ward.h"
#include "gw_ward_against_melee.h"
#include "gw_ward_against_foes.h"
#include "gw_shatter_delusions.h"
#include "gw_health_sacrifice.h"
#include "gw_illusionary_weaponry.h"
#include "gw_shadow_damage.h"
#include "gw_death_charge.h"
#include "gw_necrotic_traversal.h"
#include "gw_ash_blast.h"
#include "gw_spiritual_pain.h"
#include "gw_finish_him.h"
#include "gw_you_move_like_a_dwarf.h"
#include "gw_you_are_all_weaklings.h"
#include "gw_norn_attack_shouts.h"
#include "gw_feast_of_corruption.h"
#include "gw_holy_damage.h"
#include "gw_spear_light.h"
#include "gw_spirit_rift.h"
#include "gw_spirit_light.h"
#include "gw_signet_rejuvenation.h"
#include "gw_mend_body_soul.h"
#include "gw_ghostmirror_light.h"
#include "gw_healing_ribbon.h"
#include "gw_discord.h"
#include "gw_aegis.h"
#include "gw_signet_shadows.h"
#include "gw_backfire.h"
#include "gw_soul_barbs.h"
#include "gw_damage_fields.h"
#include "gw_cast_timing.h"
#include "gw_elemental_mantras.h"
#include <atomic>
#include "gw_wound_spells.h"
#include "gw_degeneration_hexes.h"
#include "gw_energy_degeneration_hexes.h"
#include "gw_action_damage_hexes.h"
#include "gw_icy_prism.h"
#include "gw_soul_leech.h"
#include "gw_mark_subversion.h"
#include "gw_chilblains.h"
#include "gw_movement_effects.h"
#include "gw_condition_damage_spells.h"
#include "gw_ally_cleansing.h"
#include "gw_weapon_spells.h"
#include "gw_expertise.h"
#include "gw_effect_removal.h"
#include "gw_condition_removal.h"
#include "gw_status_healing.h"
#include "gw_crystal_wave.h"
#include "gw_earth_projectile.h"
#include "gw_stone_daggers.h"
#include "gw_dancing_daggers.h"
#include "gw_iron_palm.h"
#include "gw_disrupting_dagger.h"
#include "gw_demonic_flesh.h"
#include "gw_divine_spirit.h"
#include "gw_flame_djinn_haste.h"
#include "gw_selfless_spirit.h"
#include "gw_mind_burn.h"
#include "gw_elemental_glyphs.h"
#include "gw_water_projectiles.h"
#include <cmath>
#include <functional>
#include <optional>
#include <bit>
#include <set>

namespace fw::gw::skills::casting {
using Milliseconds=std::uint64_t;
using SpellFailure=effects::TargetedSpellFailure;
// Map catalog types1(explorable)/4(mission)/6(active arena). Native InstanceInfo is_explorable
// must agree with server eligibility: client798560 refuses to send unless1.
inline constexpr bool map_allows_casts(unsigned map_type){return map_type==1||map_type==4||map_type==6;}
struct Request {std::uint32_t skill=0,copy=0,target=0;bool call=false;};
struct ActorContext {std::uint32_t actor=0;std::uint64_t generation=0;unsigned primary=0;attributes::Ranks effective{};bool pvp=false;};
// A successful authoritative effect may return particles for the completion
// batch. Boolean callbacks remain valid for effects with no deferred visuals.
struct EffectCompletion {
    bool accepted=false;
    std::vector<Packet> visuals;
    std::optional<sacrifice::Loss> paid_sacrifice;
    std::function<void()> publication;
    bool visuals_before_publication=false;
    bool completion_prefix_published=false;
    std::optional<Milliseconds> effect_snapshot_at; // Trusted World settlement time for completion resource observers.
    EffectCompletion(bool success=false,std::vector<Packet> packets={},std::optional<sacrifice::Loss> paid={},std::function<void()> publish={},bool early_visuals=false,bool published_prefix=false):accepted(success),visuals(success?std::move(packets):std::vector<Packet>{}),paid_sacrifice(success?paid:std::nullopt),publication(success?std::move(publish):std::function<void()>{}),visuals_before_publication(success&&early_visuals),completion_prefix_published(success&&published_prefix){}
    operator bool()const{return accepted;}
};
// Completion reactions have already changed authoritative state. Refusal can
// still own causal settlement packets, so publication is independent of accepted.
// Bool callbacks remain source-compatible; World-backed callers opt into capture.
struct ActionCompletion {
    bool accepted=false;std::function<void()> publication;
    ActionCompletion(bool success=false,std::function<void()> publish={}):accepted(success),publication(std::move(publish)){}
    operator bool()const{return accepted;}
};
struct CleansingCompletion {EffectCompletion effect;effects::CleansingOutcome outcome;};
struct ConditionalElementalCompletion {
    effects::ElementalOutcome outcome;
    std::vector<Packet> visuals;
    std::function<void()> publication;
    bool visuals_before_publication=false;
};
struct SignetSorrowCompletion {EffectCompletion effect;unsigned recharge_ms=30000;};
// Runtime owns the captured caster117/target118 completion bodies. Visuals
// returned here are additional copy/publication packets, not duplicate bodies.
struct MimicryCompletion {EffectCompletion effect;bool copied=false;};
enum class Reason {accepted,mode,slot,policy,unsupported,busy,recharge,target,energy,adrenaline,pve_only,weapon};
inline const char* reason_text(Reason reason){switch(reason){
case Reason::accepted:return "accepted";case Reason::mode:return "skills unavailable in this instance";
case Reason::slot:return "skill copy not equipped";case Reason::policy:return "invalid learned/profession/campaign bar";
case Reason::unsupported:return "skill effect not implemented";case Reason::busy:return "cast or aftercast active";
case Reason::recharge:return "skill recharging";case Reason::target:return "target unavailable/out of range";
case Reason::energy:return "not enough energy or caster unavailable";case Reason::adrenaline:return "not enough adrenaline";
case Reason::pve_only:return "PvE-only skill unavailable in PvP";case Reason::weapon:return "required weapon not equipped";}return "invalid cast";}
struct Hooks {
    // Staged capability: absent from static supported()/normal gateway. World
    // alone constructs and consumes the exact scoped/equipped AttackPermit.
    std::function<combat::weapon::AttackPermit(effects::Scope,const combat::weapon::Owned&,const combat::weapon::AttackPlan&,Milliseconds,Milliseconds)> prepare_dagger_attack;
    std::function<combat::weapon::DaggerPaidCompletionDraft(const std::shared_ptr<combat::weapon::DaggerDualFailureFeedbackDraft>&,Milliseconds)> complete_dagger_failed_second;
    std::function<combat::weapon::DaggerPaidCompletionDraft(const std::shared_ptr<combat::weapon::DaggerSuccessfulSecondDraft>&,const effects::LocalCooldownSnapshot&,Milliseconds)> complete_dagger_successful_second;
    std::function<combat::weapon::DaggerPaidCompletionDraft(effects::Scope,const combat::weapon::Owned&,const combat::weapon::AttackPlan&,const combat::weapon::AttackPermit&,Milliseconds,unsigned)> complete_dagger_attack;
    // Staged pet capabilities: not advertised by effect_supported. Runtime owns
    // paid receipts; callbacks capture authenticated World handles, never clients.
    std::function<std::optional<pets::CharmReservation>(effects::Scope,effects::Scope,Milliseconds)> pet_charm_reserve;
    std::function<std::optional<pets::CharmAttempt>(const pets::CharmReservation&,std::uint64_t,Milliseconds,Milliseconds)> pet_charm_accept;
    std::function<void(const pets::CharmReservation&)> pet_charm_cancel_reservation;
    std::function<void(const pets::CharmAttempt&)> pet_charm_cancel;
    // Completion callbacks return committed effect visuals only; World-owned
    // settlement precedes the captured finish/body/returned-visual batch.
    std::function<EffectCompletion(const pets::CharmAttempt&,Milliseconds)> pet_charm_completed;
    std::function<void(effects::Scope,effects::Scope,unsigned,std::uint64_t,Milliseconds)> pet_offensive_completed;
    std::function<std::optional<effects::Scope>(effects::Scope)> pet_owned_scope;
    std::function<bool(effects::Scope,effects::Scope,Milliseconds)> pet_comfort_ready;
    std::function<EffectCompletion(effects::Scope,effects::Scope,unsigned,std::uint64_t,Milliseconds)> pet_comfort_completed;
    std::function<bool(std::uint32_t,float,bool)> target_valid;
    std::function<bool(unsigned)> consume_energy;
    std::function<bool(unsigned,Milliseconds)> consume_energy_at;
    // Optional atomic payment paths receive only an admitted semantic ally target.
    std::function<bool(Slot,std::optional<effects::Scope>,unsigned,Milliseconds)> consume_selfless_energy;
    std::function<std::optional<vitals::Resources>(std::uint32_t)> resources;
    std::function<bool(std::uint32_t,unsigned,float,bool)> heal;
    // Caller routes through owned instance only. Never broadcast learned/profile
    // packets through this public activation/recharge callback.
    std::function<void(const std::vector<Packet>&)> events;
    std::function<bool(unsigned)> restore_energy;
    std::function<bool(double)> speed_multiplier;
    std::function<bool(const effects::AoEPlan&)> hostile_aoe;
    std::function<EffectCompletion(effects::Scope,const effects::SlipperyGroundPlan&,Milliseconds)> slippery_ground;
    std::function<void(unsigned,const char*)> diagnostic;
    // Server-owned successful effect completion; no raw packet/cast-start IDs.
    // Attack release reactions use their separate logical-action producer.
    std::function<ActionCompletion(unsigned,Milliseconds)> completed_action;
    std::function<std::optional<CompletionPublication>(effects::Scope,Milliseconds)> prepare_completion_publication;
    std::function<bool(effects::Scope,unsigned,std::uint64_t,Milliseconds)> succor_spell_completed;
    std::function<std::optional<std::uint64_t>()> actor_generation;
    // World implementation validates the captured lifetime under its mutex.
    // Add is a preflight/commit before client effect publication; removal uses
    // the exact effect ID so renewing a skill cannot remove its replacement.
    std::function<bool(unsigned,std::uint32_t,unsigned,Milliseconds,std::optional<Milliseconds>,bool)> status_mutation;
    // Commit the owned self-enchantment, deferring its World packets until
    // Runtime has sent the captured finish and body-resource prefix.
    std::function<EffectCompletion(unsigned,std::uint32_t,Milliseconds,Milliseconds)> burning_speed;
    std::function<EffectCompletion(unsigned,std::uint32_t,std::uint64_t,Milliseconds,Milliseconds)> burning_speed_completed;
    std::function<bool(std::uint32_t,Milliseconds)> has_enchantment;
    std::function<bool(unsigned,Milliseconds,Milliseconds)> enchantment_duration_ready;
    // Local timed mechanics must share the authoritative equipment-adjusted expiry.
    std::function<std::optional<Milliseconds>(unsigned,Milliseconds,Milliseconds)> local_enchantment_duration;
    std::function<bool(std::uint32_t,const regeneration::Plan&,Milliseconds,Milliseconds,float,bool)> apply_health_regeneration;
    // Notification after cost/adrenaline payment; preserve exact copy/event time.
    std::function<void(Slot,Milliseconds)> begin_action;
    std::function<bool(const effects::PartyHealPlan&,Milliseconds)> party_heal;
    std::function<bool(const effects::PartyHealPlan&,std::uint64_t,Milliseconds)> party_heal_completed;
    std::function<EffectCompletion(const effects::PartyHealPlan&,std::uint64_t,Milliseconds)> party_heal_published;
    std::function<bool(std::uint32_t,const effects::RemovalPlan&,Milliseconds)> remove_effects;
    std::function<EffectCompletion(effects::Scope,const effects::RemovalPlan&,std::uint64_t,Milliseconds)> remove_effects_completed;
    std::function<bool(std::uint32_t,const effects::ConditionRemovalPlan&,Milliseconds)> remove_conditions;
    std::function<bool(effects::Scope,const effects::ConditionRemovalPlan&,std::uint64_t,Milliseconds)> remove_conditions_completed;
    std::function<bool(std::uint32_t,const effects::StatusHealPlan&,Milliseconds)> status_heal;
    std::function<bool(effects::Scope,const effects::StatusHealPlan&,std::uint64_t,Milliseconds)> status_heal_completed;
    std::function<bool(effects::Scope,unsigned,Milliseconds)> status_heal_target_valid;
    std::function<bool(const effects::CrystalWavePlan&,Milliseconds)> crystal_wave;
    std::function<bool(std::uint32_t,float)> projectile_target_valid;
    // Rodgort's Invocation retains its selected center if the foe dies while
    // casting. Admission still uses the ordinary live-foe validator.
    std::function<bool(effects::Scope,Milliseconds)> rodgort_target_valid;
    std::function<bool(std::uint32_t,const effects::EarthProjectilePlan&,Milliseconds)> launch_projectile;
    std::function<bool(std::uint32_t,float)> shout_target_valid;
    std::function<bool(std::uint32_t,float)> resurrection_target_valid;
    std::function<std::optional<effects::Scope>(std::uint32_t)> target_scope;
    std::function<bool(effects::Scope,const effects::ResurrectionPlan&,Milliseconds)> resurrect;
    std::function<EffectCompletion(effects::Scope,const effects::ResurrectionPlan&,Milliseconds)> flesh_my_flesh;
    std::function<bool(const effects::PartyResurrectionPlan&,Milliseconds)> resurrect_party;
    std::function<EffectCompletion(effects::Scope,effects::Scope,unsigned,unsigned,std::uint64_t,Milliseconds)> resurrection_completed;
    std::function<EffectCompletion(effects::Scope,const effects::LightningPlan&,Milliseconds)> lightning;
    std::function<bool(effects::Scope,const effects::BlindnessPlan&,Milliseconds)> blindness;
    std::function<bool(effects::Scope,const effects::ConditionTransferPlan&,Milliseconds,std::optional<double>)> condition_transfer;
    std::function<bool(effects::Scope,const effects::ConditionSpellPlan&,Milliseconds)> condition_spell;
    std::function<EffectCompletion(effects::Scope,const effects::ConditionSpellPlan&,std::uint64_t,Milliseconds)> condition_spell_completed;
    std::function<EffectCompletion(effects::Scope,const effects::VirulencePlan&,Milliseconds)> virulence;
    std::function<EffectCompletion(effects::Scope,const effects::RottingFleshPlan&,Milliseconds)> rotting_flesh;
    std::function<bool(std::uint32_t,const effects::TaintedFleshPlan&,Milliseconds)> tainted_flesh_target_valid;
    std::function<EffectCompletion(effects::Scope,const effects::TaintedFleshPlan&,Milliseconds)> tainted_flesh;
    std::function<EffectCompletion(effects::Scope,const effects::SpitefulSpiritPlan&,Milliseconds)> spiteful_spirit;
    std::function<bool(std::uint32_t,const effects::EnergyEnchantmentPlan&,Milliseconds)> energy_enchantment_target_valid;
    std::function<EffectCompletion(effects::Scope,const effects::EnergyEnchantmentPlan&,Milliseconds)> energy_enchantment;
    std::function<EffectCompletion(effects::Scope,const effects::EnergyEnchantmentPlan&,std::uint64_t,Milliseconds)> energy_enchantment_completed;
    std::function<bool(effects::Scope,const effects::LifeStealPlan&,Milliseconds)> life_steal;
    std::function<EffectCompletion(effects::Scope,const effects::LifeStealPlan&,Milliseconds)> complete_life_steal;
    std::function<EffectCompletion(effects::Scope,const effects::LifeStealPlan&,std::uint64_t,Milliseconds)> life_steal_spell_completed;
    std::function<bool(const effects::AreaLifeStealPlan&,Milliseconds)> area_life_steal;
    std::function<EffectCompletion(effects::Scope,const effects::ElementalPlan&,Milliseconds)> elemental;
    std::function<std::optional<ConditionalElementalCompletion>(effects::Scope,const effects::ElementalPlan&,Milliseconds)> conditional_elemental;
    std::function<std::optional<ConditionalElementalCompletion>(effects::Scope,const effects::ElementalPlan&,std::uint64_t,Milliseconds)> elemental_completed;
    std::function<EffectCompletion(effects::Scope,const effects::MindBurnPlan&,Milliseconds)> mind_burn;
    std::function<EffectCompletion(effects::Scope,const effects::EnergyDrainPlan&,Milliseconds)> energy_drain;
    std::function<EffectCompletion(effects::Scope,const effects::EnergyDrainPlan&,std::uint64_t,Milliseconds)> energy_drain_completed;
    std::function<std::optional<effects::ResourceGainOutcome>(effects::Scope,const effects::ResourceGainPlan&,Milliseconds)> resource_gain;
    std::function<std::optional<std::vector<effects::Entry>>(Milliseconds)> effect_snapshot;
    // World decides whether a living Quickening Zephyr spirit covers this
    // caster at completion; no spirit is inferred from a skill-bar entry.
    std::function<std::optional<bool>(Milliseconds)> quickening_zephyr_aura;
    std::function<std::optional<sacrifice::Loss>(unsigned,Milliseconds)> health_sacrifice;
    std::function<EffectCompletion(effects::Scope,const effects::ShadowPlan&,Milliseconds)> shadow_damage;
    std::function<EffectCompletion(effects::Scope,const effects::NecrosisPlan&,Milliseconds)> necrosis;
    std::function<EffectCompletion(effects::Scope,const effects::EnergyBlastPlan&,Milliseconds)> energy_blast;
    std::function<EffectCompletion(effects::Scope,const effects::AneurysmPlan&,Milliseconds)> aneurysm;
    std::function<EffectCompletion(effects::Scope,const effects::EarBitePlan&,Milliseconds)> ear_bite;
    std::function<bool(effects::Scope,const effects::DeathCharge&,Milliseconds)> death_charge_target_valid;
    std::function<EffectCompletion(effects::Scope,const effects::DeathCharge&,Milliseconds)> death_charge;
    std::function<EffectCompletion(effects::Scope,const effects::AshBlastPlan&,Milliseconds)> ash_blast;
    std::function<EffectCompletion(effects::Scope,const effects::SpiritualPainPlan&,Milliseconds)> spiritual_pain;
    // Opt-in until all Well visual/field consumers are reviewed. The callback
    // must claim the corpse and create its World field before returning visuals.
    std::function<EffectCompletion(effects::Scope,const effects::WellCastPlan&,std::uint64_t,Milliseconds)> well_corpse;
    // Optional staged97 adapter; absent in the published gateway until all effect gates close.
    std::function<std::optional<std::size_t>(effects::Scope,const effects::NecroticTraversalPlan&,Milliseconds)> necrotic_available;
    std::function<EffectCompletion(effects::Scope,const effects::NecroticTraversalPlan&,std::uint64_t,Milliseconds)> necrotic_traversal;
    std::function<bool(const effects::FeastCastAnchor&,const effects::FeastOfCorruptionPlan&,Milliseconds)> feast_of_corruption;
    std::function<bool(effects::Scope,const effects::AllyCleansingPlan&,Milliseconds)> ally_cleansing;
    std::function<std::optional<effects::CleansingOutcome>(effects::Scope,const effects::AllyCleansingPlan&,Milliseconds)> ally_cleansing_outcome;
    std::function<CleansingCompletion(effects::Scope,const effects::AllyCleansingPlan&,std::uint64_t,Milliseconds)> ally_cleansing_completed;
    std::function<bool(effects::Scope,const effects::WeaponSpellPlan&,Milliseconds)> weapon_spell_target_valid;
    std::function<EffectCompletion(effects::Scope,const effects::WeaponSpellPlan&,Milliseconds)> weapon_spell;
    std::function<bool(effects::Scope,const effects::WoundSpellPlan&,Milliseconds)> wound_spell;
    std::function<EffectCompletion(effects::Scope,const effects::WoundSpellPlan&,std::uint64_t,Milliseconds)> wound_spell_completed;
    std::function<EffectCompletion(effects::Scope,const effects::GlimmeringMarkPlan&,std::uint64_t,Milliseconds)> glimmering_mark_completed;
    std::function<EffectCompletion(effects::Scope,const effects::SnowStormPlan&,std::uint64_t,Milliseconds)> snow_storm_completed;
    std::function<EffectCompletion(effects::Scope,const effects::DegenerationHexPlan&,Milliseconds)> degeneration_hex;
    std::function<EffectCompletion(effects::Scope,const effects::DegenerationHexPlan&,std::uint64_t,Milliseconds)> degeneration_hex_completed;
    // Explicit trusted integration capability;not advertised by effect_supported
    // until remaining lifecycle/retail/rendered acceptance is complete.
    std::function<EffectCompletion(effects::Scope,const effects::EnergyDegenerationHexPlan&,Milliseconds)> energy_degeneration_hex;
    std::function<EffectCompletion(effects::Scope,const effects::ActionDamageHexPlan&,Milliseconds)> action_damage_hex;
    std::function<EffectCompletion(effects::Scope,const effects::IcyPrismPlan&,Milliseconds)> icy_prism;
    std::function<EffectCompletion(effects::Scope,const effects::SoulLeechPlan&,Milliseconds)> soul_leech;
    std::function<EffectCompletion(effects::Scope,const effects::MarkSubversionPlan&,Milliseconds)> mark_subversion;
    std::function<EffectCompletion(effects::Scope,const effects::ChilblainsPlan&,Milliseconds)> chilblains;
    std::function<bool(effects::Scope,unsigned,Milliseconds)> ally_enchantment_target_valid;
    std::function<EffectCompletion(effects::Scope,const effects::WatchfulHealingPlan&,unsigned,Milliseconds)> watchful_healing;
    std::function<EffectCompletion(effects::Scope,const regeneration::Plan&,unsigned,Milliseconds)> restful_breeze;
    std::function<EffectCompletion(effects::Scope,const effects::ProtectiveSpiritPlan&,unsigned,Milliseconds)> protective_spirit;
    // Staged capability: normal gateway remains disabled until all consumers pass.
    std::function<bool(effects::Scope,unsigned,Milliseconds)> reversal_target_valid;
    std::function<EffectCompletion(effects::Scope,const effects::ReversalFortunePlan&,unsigned,std::uint64_t,Milliseconds)> reversal_fortune_completed;
    std::function<EffectCompletion(effects::Scope,unsigned,unsigned,unsigned,std::uint64_t,Milliseconds)> simple_enchantment_completed;
    std::function<EffectCompletion(const effects::AegisPlan&,unsigned,Milliseconds)> aegis;
    std::function<EffectCompletion(effects::Scope,const effects::VigorousSpiritPlan&,unsigned,Milliseconds)> vigorous_spirit;
    std::function<bool(effects::Scope,unsigned,Milliseconds)> healing_seed_target_valid;
    std::function<EffectCompletion(effects::Scope,const effects::HealingSeedPlan&,unsigned,Milliseconds)> healing_seed;
    std::function<EffectCompletion(effects::Scope,const effects::ShieldDeflectionPlan&,unsigned,Milliseconds)> shield_deflection;
    std::function<EffectCompletion(effects::Scope,const effects::ShieldRegenerationPlan&,unsigned,Milliseconds)> shield_regeneration;
    std::function<EffectCompletion(effects::Scope,const effects::ShieldJudgmentPlan&,unsigned,Milliseconds)> shield_judgment;
    std::function<bool(Milliseconds)> shield_judgment_ready;
    std::function<std::optional<combat::weapon::Owned>()> weapon_snapshot;
    std::function<bool(std::uint32_t,const combat::weapon::Owned&,Milliseconds)> critical_attack_ready;
    std::function<bool(std::uint32_t,Milliseconds)> melee_target;
    std::function<bool(effects::Scope,const combat::weapon::Owned&,const combat::weapon::AttackPlan&,Milliseconds)> melee_attack;
    std::function<std::optional<Milliseconds>(effects::Scope,const combat::weapon::Owned&,const combat::weapon::AttackPlan&,std::uint64_t,Milliseconds,Milliseconds,Milliseconds,std::vector<Packet>&)> completed_bow_attack;
    std::function<void(std::optional<Milliseconds>)> attack_activity;
    std::function<EffectCompletion(effects::Scope,const effects::ConditionDamagePlan&,Milliseconds)> condition_damage;
    std::function<bool(Slot,Milliseconds)> consume_adrenaline;
    std::function<void(Milliseconds)> reconcile_adrenaline;
    std::function<bool(Milliseconds,std::optional<Milliseconds>)> healing_signet_activity;
    std::function<std::optional<std::uint64_t>()> attack_interrupt_epoch;
    std::function<bool(effects::Scope,const effects::LightningJavelinPlan&,Milliseconds)> lightning_javelin;
    std::function<bool(effects::Scope,const effects::EmpathyPlan&,Milliseconds)> empathy;
    std::function<bool(effects::Scope,const effects::DeathlySwarmPlan&,Milliseconds)> deathly_swarm;
    std::function<EffectCompletion(effects::Scope,const effects::FaintheartednessPlan&,Milliseconds)> faintheartedness;
    // Admitted stationary defensive ward completion.
    std::function<EffectCompletion(const effects::DefensiveWardPlan&,std::uint64_t,Milliseconds)> defensive_ward_completed;
    std::function<EffectCompletion(const effects::WardElementsPlan&,Milliseconds)> ward_elements;
    std::function<EffectCompletion(const effects::WardMeleePlan&,Milliseconds)> ward_melee;
    std::function<EffectCompletion(const effects::WardFoesPlan&,Milliseconds)> ward_foes;
    std::function<std::optional<Milliseconds>(Milliseconds,Milliseconds)> attack_duration;
    std::function<bool(std::uint32_t,unsigned,Milliseconds)> attack_skill_target;
    std::function<bool(effects::Scope,const effects::ShatterDelusionsPlan&,Milliseconds)> shatter_delusions;
    std::function<EffectCompletion(effects::Scope,const effects::MovementHexPlan&,Milliseconds)> movement_hex;
    std::function<EffectCompletion(effects::Scope,const effects::WaterProjectilePlan&,Milliseconds)> water_projectile;
    std::function<bool(effects::Scope,const regeneration::Plan&,unsigned,Milliseconds)> protective_enchantment;
    std::function<EffectCompletion(effects::Scope,const effects::SignetJudgmentPlan&,Milliseconds)> signet_judgment;
    std::function<SignetSorrowCompletion(effects::Scope,const effects::SignetSorrowPlan&,Milliseconds)> signet_sorrow;
    std::function<bool(Milliseconds)> signet_judgment_ready;
    std::function<EffectCompletion(effects::Scope,const effects::BlessedSignetPlan&,Milliseconds)> blessed_signet;
    std::function<bool(effects::Scope,Milliseconds)> blessed_signet_ready;
    std::function<EffectCompletion(effects::Scope,const effects::MendingPlan&,unsigned,Milliseconds)> mending;
    std::function<bool(effects::Scope,unsigned,Milliseconds)> mending_target_valid;
    // Explicit opt-in while maintained NPC recipients and spell-loss consumer
    // coverage are unfinished. The production gateway must leave this empty.
    std::function<EffectCompletion(effects::Scope,const effects::SuccorPlan&,unsigned,Milliseconds)> succor;
    std::function<bool(effects::Scope,unsigned,Milliseconds)> succor_target_valid;
    // Staged paid capability only; normal skill248 remains unbound. The event
    // is admitted by before_spell_effect and consumed exactly once by World.
    std::function<EffectCompletion(effects::Scope,const effects::RetributionPlan&,unsigned,std::uint64_t,Milliseconds)> retribution_completed;
    std::function<bool(effects::Scope,unsigned,Milliseconds)> retribution_target_valid;
    // Opt-in until Recall's remaining bundle/recast/fidelity gates are closed.
    std::function<EffectCompletion(effects::Scope,Milliseconds)> recall;
    std::function<bool(effects::Scope,Milliseconds)> recall_target_valid;
    std::function<MimicryCompletion(Slot,effects::Scope,Milliseconds)> mimicry;
    std::function<bool(Slot,effects::Scope,Milliseconds)> mimicry_target_valid;
    std::function<std::optional<bool>(Slot,Milliseconds)> recall_feedback_pending;
    std::function<EffectCompletion(effects::Scope,const effects::HolyPlan&,Milliseconds)> holy_damage;
    std::function<EffectCompletion(effects::Scope,const effects::SpearLight&,Milliseconds)> spear_light;
    std::function<EffectCompletion(effects::Scope,const effects::SpiritRift&,Milliseconds)> spirit_rift;
    std::function<EffectCompletion(effects::Scope,const effects::Discord&,Milliseconds)> discord;
    std::function<EffectCompletion(effects::Scope,const effects::SignetShadowsPlan&,Milliseconds)> signet_shadows;
    std::function<bool(effects::Scope,const effects::BackfirePlan&,Milliseconds)> backfire;
    std::function<bool(unsigned,std::uint64_t,Milliseconds)> before_spell_effect;
    std::function<SpellFailure(unsigned,std::optional<effects::Scope>,std::uint64_t,Milliseconds)> targeted_spell_failure;
    std::function<bool(effects::Scope,unsigned,float,bool,Milliseconds)> heal_scoped;
    std::function<bool(effects::Scope,unsigned,Milliseconds)> direct_heal_target_valid;
    std::function<bool(effects::Scope,unsigned,unsigned,unsigned,std::uint64_t,Milliseconds)> direct_heal_completed;
    // World computes the below-half condition and Energy reward in the same
    // locked transaction as the heal, using the target's live pre-heal Health.
    std::function<bool(effects::Scope,unsigned,unsigned,Milliseconds)> zealous_benediction;
    std::function<bool(std::uint32_t,float,Milliseconds)> zealous_target_valid;
    std::function<bool(effects::Scope,Milliseconds)> gift_health_target_valid;
    std::function<EffectCompletion(effects::Scope,const effects::GiftHealthPlan&,unsigned,std::uint64_t,Milliseconds)> gift_health;
    std::function<EffectCompletion(effects::Scope,const effects::GiftHealthPlan&,unsigned,std::uint64_t,std::uint64_t,Milliseconds)> gift_health_completed;
    std::function<bool(effects::Scope,Milliseconds)> spirit_light_target_valid;
    std::function<EffectCompletion(effects::Scope,const effects::SpiritLightPlan&,Milliseconds)> spirit_light;
    std::function<EffectCompletion(effects::Scope,const effects::SpiritLightPlan&,std::uint64_t,Milliseconds)> spirit_light_completed;
    std::function<bool(effects::Scope,Milliseconds)> mend_body_soul_target_valid;
    std::function<EffectCompletion(effects::Scope,const effects::MendBodySoulPlan&,Milliseconds)> mend_body_soul;
    std::function<EffectCompletion(effects::Scope,const effects::MendBodySoulPlan&,std::uint64_t,Milliseconds)> mend_body_soul_completed;
    std::function<bool(effects::Scope,Milliseconds)> ghostmirror_light_target_valid;
    std::function<EffectCompletion(effects::Scope,const effects::GhostmirrorLightPlan&,Milliseconds)> ghostmirror_light;
    std::function<EffectCompletion(effects::Scope,const effects::GhostmirrorLightPlan&,std::uint64_t,Milliseconds)> ghostmirror_light_completed;
    std::function<bool(effects::Scope,Milliseconds)> healing_ribbon_target_valid;
    std::function<EffectCompletion(effects::Scope,const effects::HealingRibbonPlan&,unsigned,Milliseconds)> healing_ribbon;
    std::function<EffectCompletion(effects::Scope,const effects::HealingRibbonPlan&,unsigned,std::uint64_t,Milliseconds)> healing_ribbon_completed;
    std::function<bool(effects::Scope,Milliseconds)> rejuvenation_target_valid;
    std::function<EffectCompletion(effects::Scope,const effects::RejuvenationPlan&,Milliseconds)> rejuvenation;
    std::function<bool(effects::Scope,const effects::SoulBarbsPlan&,Milliseconds)> soul_barbs;
    std::function<bool(const effects::DamageFieldPlan&,Milliseconds)> damage_field;
    // Accepted control events survive recovery. A pending activation compares
    // this lifetime-scoped epoch even if its next update is after knockdown.
    std::function<std::optional<std::uint64_t>()> control_epoch;
    std::function<std::optional<Milliseconds>(std::uint64_t)> control_event_time;
    // World-owned activation window. Zero endpoints clear only this skill.
    std::function<void(unsigned,Milliseconds,Milliseconds)> cast_activity;
    std::function<void(Slot,Milliseconds,Milliseconds)> cast_slot_activity;
    std::function<EffectCompletion(effects::Scope,const effects::InterruptPlan&,Milliseconds)> interrupt_skill;
    std::function<bool(effects::Scope,const effects::HumilityPlan&,Milliseconds)> signet_humility;
    std::function<bool(effects::Scope,const effects::HexBreakerPlan&,Milliseconds)> hex_breaker;
    std::function<std::optional<std::uint64_t>(std::uint64_t,Milliseconds)> take_grenths_reaction_death;
    std::function<bool(effects::Scope,std::optional<std::uint64_t>,Milliseconds)> grenths_balance;
    std::function<bool(effects::Scope,const effects::DiversionPlan&,Milliseconds)> diversion;
    std::function<bool(effects::Scope,const effects::InspiredHexPlan&,Slot,Milliseconds)> inspired_hex;
    std::function<EffectCompletion(effects::Scope,const effects::CryFrustrationPlan&,Milliseconds)> cry_frustration;
    std::function<bool(effects::Scope,unsigned,Milliseconds)> mantra_recall;
    std::function<bool(effects::Scope,const effects::UnnaturalPlan&,std::uint64_t,Milliseconds)> unnatural_signet;
    std::function<bool(effects::Scope,const effects::SurvivalPlan&,Milliseconds)> i_will_survive;
    std::function<bool(effects::Scope,const effects::UnstoppablePlan&,Milliseconds)> i_am_unstoppable;
    std::function<bool(effects::Scope,const effects::IAmTheStrongestPlan&,Milliseconds)> i_am_the_strongest;
    std::function<bool(effects::Scope,const effects::DodgeThisPlan&,Milliseconds)> dodge_this;
    std::function<bool(effects::Scope,const effects::JusticePlan&,Milliseconds)> for_great_justice;
    std::function<bool(effects::Scope,const effects::ToLimitPlan&,Milliseconds)> to_limit;
    std::function<EffectCompletion(effects::Scope,const effects::WatchYourselfPlan&,Milliseconds)> watch_yourself;
    // Opt-in until every party/NPC projectile consumer has been reviewed.
    std::function<EffectCompletion(effects::Scope,const effects::ShieldsUpPlan&,Milliseconds)> shields_up;
    std::function<EffectCompletion(effects::Scope,const effects::DontTripPlan&,Milliseconds)> dont_trip;
    std::function<EffectCompletion(effects::Scope,const effects::SaveYourselvesPlan&,Milliseconds)> save_yourselves;
    std::function<bool(effects::Scope,const effects::SignetOfCorruptionPlan&,Milliseconds)> signet_of_corruption;
    // Trusted host supplies a reviewed spirit template and deferred spawn.
    // The production gateway omits this until the 2022 appearance is verified.
    std::function<EffectCompletion(effects::Scope,const effects::QuickeningZephyrPlan&,Milliseconds)> quickening_zephyr_spawn;
    std::function<EffectCompletion(effects::Scope,const effects::ChargePlan&,Milliseconds)> charge;
    std::function<EffectCompletion(effects::Scope,const effects::EndurePainPlan&,Milliseconds)> endure_pain;
    std::function<EffectCompletion(effects::Scope,const effects::StaminaSignetPlan&,Milliseconds)> stamina_signet;
    std::function<EffectCompletion(effects::Scope,const effects::DefyPainPlan&,Milliseconds)> defy_pain;
    std::function<bool(effects::Scope,effects::Scope,const effects::ConvertHexesPlan&,unsigned,Milliseconds)> convert_hexes;
    std::function<bool(effects::Scope,unsigned,Milliseconds)> armor_earth;
    std::function<bool(effects::Scope,unsigned,std::uint64_t,Milliseconds)> armor_earth_completed;
    std::function<std::optional<bool>(effects::Scope,const effects::FlameDjinnHastePlan&,std::uint64_t,Milliseconds)> flame_djinn_haste;
    std::function<bool(effects::Scope,const effects::HealingRingPlan&,Milliseconds)> healing_ring;
    std::function<EffectCompletion(effects::Scope,const effects::HealingRingPlan&,std::uint64_t,Milliseconds)> healing_ring_completed;
    std::function<bool(effects::Scope,Milliseconds)> antidote_signet;
    std::function<bool(effects::Scope,const effects::ExtinguishPlan&,Milliseconds)> extinguish;
    std::function<bool(effects::Scope,const effects::MartyrPlan&,unsigned,Milliseconds)> martyr;
    std::function<bool(effects::Scope,const effects::ConjurePlan&,Milliseconds)> conjure_enchantment;
    std::function<bool(effects::Scope,unsigned,Milliseconds)> panic;
    std::function<bool(const effects::PanicUse&)> panic_skill_used;
    std::function<bool(effects::Scope,const effects::MindWrackPlan&,Milliseconds)> mind_wrack;
    std::function<bool(effects::Scope,const effects::WanderingEyePlan&,Milliseconds)> wandering_eye;
    std::function<bool(const effects::MindWrackUse&)> mind_wrack_targeted;
    // Optional staged completion observer; admission retains its presence and
    // exact selected target. Runs after validation, before the new effect.
    std::function<bool(const effects::GlimmeringTargetedUse&)> glimmering_targeted_use;
    std::function<bool(effects::Scope,const effects::FeveredDreamsPlan&,Milliseconds)> fevered_dreams;
    std::function<std::optional<effects::DisableRevisions>(effects::Scope)> prepare_keystone_reset;
    std::function<bool(effects::Scope,const effects::KeystonePlan&,const std::vector<Slot>&,const effects::DisableRevisions&,std::uint64_t,Milliseconds)> commit_keystone_reset;
    std::function<bool(const effects::KeystoneUse&)> keystone_skill_used;
    std::function<std::optional<effects::FlourishPermit>(effects::Scope,Milliseconds)> prepare_flourish_reset;
    std::function<std::optional<effects::CooldownStamp>(const effects::FlourishRecharge&,const effects::FlourishPermit&,Milliseconds)> commit_flourish_reset;
    std::function<bool(Slot,std::uint64_t,Milliseconds)> prepare_skill_use;
    std::function<std::optional<Milliseconds>(std::uint64_t,Milliseconds,Milliseconds,Milliseconds,bool)> finalize_skill_use;
    std::function<bool(std::uint64_t)> abort_skill_use;
    std::function<std::optional<effects::CooldownCompletion>(std::uint64_t)> cooldown_completion_source;
    std::function<std::optional<effects::CooldownAmendment>(std::uint64_t)> cooldown_amendment_source;
    std::function<std::optional<effects::CooldownControl>(std::uint64_t)> cooldown_control_source;
    // Trusted owner settlement:0 means no attack-disable request. A failure
    // must leave World unchanged. Positive results are independent disables,
    // never replacements for the supplied ordinary recharge deadline.
    std::function<std::optional<Milliseconds>(const effects::CooldownOrigin&,Slot,Milliseconds,Milliseconds)> settle_attack_interrupt;
    std::function<std::optional<Milliseconds>(Slot,Milliseconds)> skill_disabled_slot;
    std::function<std::optional<Milliseconds>(unsigned,Milliseconds)> skill_disabled_until;
    std::function<std::optional<Slots>()> current_skillbar;
    // Session-owned capability; never a persistent learned-skill grant.
    std::function<std::optional<effects::HexCopySession::CastAdmission>(const Slots&,const Policy&,Request,Milliseconds)> admit_session_cast;
    std::function<bool(const effects::HexCopySession::CastAdmission&,Milliseconds)> valid_session_cast;
    std::function<bool(effects::Scope,const effects::InfuseHealthPlan&,Milliseconds)> infuse_health;
    // Atomic World producer: admits the exact spell-reaction terminal and
    // commits Renewal before computing Infuse's Health loss and healing.
    std::function<bool(effects::Scope,const effects::InfuseHealthPlan&,std::uint64_t,Milliseconds)> infuse_health_completed;
    std::function<bool(effects::Scope,const effects::ImbueHealthPlan&,Milliseconds)> imbue_health;
    std::function<bool(effects::Scope,const effects::ImbueHealthPlan&,std::uint64_t,Milliseconds)> imbue_health_completed;
    std::function<bool(unsigned,Milliseconds)> infuse_target_valid;
    std::function<std::optional<combat::weapon::Timing>(Milliseconds)> weapon_attack_timing;
    std::function<std::optional<double>()> skill_roll;
    std::function<std::optional<double>(unsigned,Milliseconds)> activation_multiplier;
    std::function<bool(unsigned,std::uint64_t,Milliseconds)> prepare_channeling;
    std::function<bool(std::uint64_t,bool,Milliseconds)> complete_channeling;
    std::function<bool(std::uint32_t,float,Milliseconds)> shatter_ally_valid;
    std::function<bool(effects::Scope,const effects::EnchantmentRemovalPlan&,Milliseconds)> enchantment_removal;
    std::function<std::optional<effects::EnchantmentRemovalOutcome>(effects::Scope,const effects::EnchantmentRemovalPlan&,Milliseconds)> enchantment_removal_result;
    std::function<bool(effects::Scope,const effects::DrainDelusionsPlan&,Milliseconds)> drain_delusions;
    std::function<combat::weapon::AttackPermit(effects::Scope,const combat::weapon::Owned&,const combat::weapon::AttackPlan&,Milliseconds)> prepare_melee_attack;
    std::function<bool(effects::Scope,const combat::weapon::Owned&,const combat::weapon::AttackPlan&,const combat::weapon::AttackPermit&,Milliseconds)> prepared_melee_attack;
    // Atomic ordinary-cost plus Overcast transaction; never pay either twice.
    std::function<bool(unsigned,unsigned,std::uint64_t,Milliseconds)> consume_overcast_energy;
    std::function<bool(unsigned,std::optional<effects::Scope>,unsigned,std::uint64_t,Milliseconds)> consume_selfless_overcast_energy;
    std::function<bool(effects::Scope,unsigned,std::optional<double>,Milliseconds)> gale;
    std::function<std::optional<std::vector<Packet>>(const effects::DamageFieldPlan&,Milliseconds)> complete_damage_field;
    std::function<std::optional<std::vector<Packet>>(effects::Scope,const effects::FireStormPlan&,Milliseconds)> complete_fire_storm;
    std::function<std::optional<std::vector<Packet>>(effects::Scope,const effects::RayOfJudgmentPlan&,Milliseconds)> complete_ray_of_judgment;
    std::function<EffectCompletion(unsigned,const effects::ElementalGlyphPlan&,Milliseconds)> apply_elemental_glyph;
    std::function<std::optional<effects::ElementalGlyphState>(Milliseconds)> elemental_glyph_snapshot;
    std::function<std::optional<effects::ElementalGlyphQuote>(Slot,unsigned,std::uint64_t,Milliseconds,const effects::ElementalGlyphState&)> pay_elemental_glyph;
    std::function<std::optional<effects::ElementalGlyphQuote>(Slot,std::optional<effects::Scope>,unsigned,std::uint64_t,Milliseconds,const effects::ElementalGlyphState&)> pay_selfless_elemental_glyph;
    std::function<effects::ElementalGlyphSettlement(std::uint64_t,effects::GlyphOutcome,Milliseconds)> settle_elemental_glyph;
    std::function<Milliseconds()> glyph_clock;
    std::function<std::optional<effects::RenewalTerminal>(std::uint64_t,std::uint64_t,Slot,effects::GlyphOutcome,Milliseconds,Milliseconds,Milliseconds,std::optional<std::uint64_t>)> finish_renewal_spell;
    std::function<EffectCompletion(effects::Scope,const effects::StoneDaggersPlan&,Milliseconds)> stone_daggers;
    std::function<EffectCompletion(effects::Scope,const effects::DancingDaggersPlan&,Milliseconds)> dancing_daggers;
    // Staged until the 2022 chain-stage consumer and publication are verified.
    std::function<EffectCompletion(effects::Scope,const effects::IronPalmPlan&,Milliseconds)> iron_palm;
    std::function<EffectCompletion(effects::Scope,const effects::FinishHimPlan&,Milliseconds)> finish_him;
    std::function<EffectCompletion(effects::Scope,const effects::YouMoveLikeADwarfPlan&,Milliseconds)> you_move_like_a_dwarf;
    std::function<EffectCompletion(effects::Scope,const effects::YouAreAllWeaklingsPlan&,Milliseconds)> you_are_all_weaklings;
    std::function<EffectCompletion(effects::Scope,const effects::DisruptingDaggerPlan&,Milliseconds)> disrupting_dagger;
    std::function<EffectCompletion(unsigned,const effects::DemonicFleshPlan&,Milliseconds)> demonic_flesh;
    // Opt-in until the Divine Spirit/Glyph Energy modifier order is proven.
    std::function<EffectCompletion(const effects::DivineSpiritPlan&,unsigned,Milliseconds)> divine_spirit;
    std::function<EffectCompletion(const effects::SelflessSpiritPlan&,unsigned,Milliseconds)> selfless_spirit;
    std::function<std::optional<effects::DemonicFleshReservation>(const effects::DemonicFleshUse&)> prepare_demonic_flesh_use;
    std::function<bool(const effects::DemonicFleshReservation&)> commit_demonic_flesh_use;
    std::function<bool(const effects::DemonicFleshReservation&)> cancel_demonic_flesh_use;
    // Opt-in while the owning session's simultaneous-arm driver is unfinished.
    std::function<bool(Slot,unsigned,Milliseconds,Milliseconds)> arm_echo;
    std::function<bool(const effects::CooldownCompletion&)> completed_skill_use;
    // PvE title skills use the character's earned track, not allocated attributes.
    std::function<std::optional<unsigned>()> ebon_vanguard_rank;
    std::function<std::optional<unsigned>()> norn_rank;
    std::function<std::optional<unsigned>()> deldrimor_rank;
    std::function<std::optional<unsigned>()> sunspear_rank;
    std::function<std::optional<unsigned>(unsigned)> allegiance_rank;
    // Opt-in only when the caller owns the exact attributed block reward.
    std::function<bool()> wary_stance_reward_ready;
};
// Process-wide trusted IDs survive Runtime replacement and never collide
// across actors. Exhaustion fails closed instead of wrapping into old events.
inline std::uint64_t spell_activation_event(){
    static std::atomic<std::uint64_t> next{1};auto value=next.load(std::memory_order_relaxed);
    while(value!=UINT64_MAX&&!next.compare_exchange_weak(value,value+1,std::memory_order_relaxed)){}
    return value==UINT64_MAX?0:value;
}
inline Packet slot_event(unsigned opcode,std::uint32_t actor,Slot slot,std::optional<unsigned> seconds={}){
    Packet p;word(p,static_cast<std::uint16_t>(opcode));dword(p,actor);word(p,static_cast<std::uint16_t>(slot.skill));dword(p,slot.copy);
    if(seconds)dword(p,*seconds);return p;
}
inline Packet generic(unsigned kind,std::uint32_t actor,unsigned value){
    Packet p;word(p,158);dword(p,kind);dword(p,actor);dword(p,value);return p;
}
// Completed/stopped phases carry value0; captured failed dual follow-up47
// is original158 -> actor action18 (pinned1360 lines710818/710856/710881).
// Skill/copy identity belongs to the separate slot messages.
inline Packet terminal(unsigned kind,std::uint32_t actor){
    if(kind!=46&&kind!=47&&kind!=49&&kind!=58&&kind!=59)throw std::invalid_argument("invalid terminal event kind");
    return generic(kind,actor,0);
}
// Native159 descriptor60bdfc ->874ba0 ->794810 ->77a370 queues
// action19 with target at+30 and skill at+34. The first wire ID is caster;
// historical GWCA field names reverse these roles. Generic158kind60 uses
// target0;159 preserves the explicit self/ally target.
// Exact build37130 Skill+78 caster-body resources; original kind21 consumes
// resource IDs through85a370/77caa0, not skill IDs. Reviewed success-only cases.
inline std::optional<Packet> success_body_effect(std::uint32_t actor,unsigned skill){
    const auto* row=find_skill(skill);
    if(actor&&row&&row->caster_body_resource<2077)
        return generic(21,actor,row->caster_body_resource);
    return {};
}
// Reviewed success dispatch may differ from the caster-only metadata field.
// Defensive Stance's fixed capture explicitly emits its target-body618 on self.
inline std::optional<Packet> skill_success_body_effect(std::uint32_t actor,unsigned skill){
    // World publishes this charged shout's body before its maintained status.
    if(skill==2355&&effects::i_am_the_strongest(0))return {};
    if(actor&&skill==345&&effects::defensive_stance(0))return generic(21,actor,618);
    if(actor&&skill==823&&effects::burning_speed(0))return generic(21,actor,339);
    return success_body_effect(actor,skill);
}
inline Packet activation(std::uint32_t caster,std::uint32_t target,unsigned skill,bool attack=false){
    Packet p;word(p,159);dword(p,attack?50:60);dword(p,caster);dword(p,target);dword(p,skill);return p;
}
// Admit only reviewed PvP aliases here. A native alias alone does not establish
// its mechanics; Crippling Anguish and Migraine have reviewed mode-specific plans.
inline const SkillRecord* reviewed_cast_record(unsigned skill,bool pvp){
    return find_skill(pvp&&skill==792?2868:pvp&&skill==793?2893:pvp&&skill==1191?2864:pvp&&skill==1720?3147:pvp&&skill==817?2863:pvp&&skill==180?3375:pvp&&skill==318?3204:pvp&&skill==348?2858:pvp&&skill==294?2887:pvp&&skill==934?3188:pvp&&skill==374?3002:pvp&&skill==448?3060:pvp&&skill==453?3141:pvp&&skill==55?3289:pvp&&skill==49?2734:pvp&&skill==37?3373:pvp&&skill==19?2998:pvp&&skill==54?3152:pvp&&skill==1052?3184:pvp&&skill==53?3183:skill);
}
inline unsigned scaled(unsigned low,unsigned high,unsigned rank){
    // The pinned constants give endpoints0/15. Values round to nearest integer,
    // as reviewed local healing/DivineFavor articles specify.
    return static_cast<unsigned>(std::lround(double(low)+(double(high)-low)*std::min(rank,20u)/15.0));
}
// Exact65 descriptor60b9ac: actorDWORD,skillWORD,rankDWORD,effectDWORD,
// FLOATseconds. Old GWCA's timestamp label is disproved by79eec0+10 duration.
inline Packet effect_add(std::uint32_t actor,unsigned skill,unsigned rank,std::uint32_t effect,float duration){if(!actor||!effect||!skill||skill>3432||rank>20||!std::isfinite(duration)||duration<=0)throw std::invalid_argument("invalid skill effect");Packet p;word(p,65);dword(p,actor);word(p,skill);dword(p,rank);dword(p,effect);dword(p,std::bit_cast<std::uint32_t>(duration));return p;}
inline Packet effect_remove(std::uint32_t actor,std::uint32_t effect){Packet p;word(p,67);dword(p,actor);dword(p,effect);return p;}
// Base durations for enchantments whose mechanics are also cached locally.
// Keep this list explicit: stances/glyphs in the same effect branch are not enchantments.
inline std::optional<Milliseconds> local_enchantment_base(unsigned skill,unsigned rank){
    if(skill==1375){const auto plan=effects::stoneflesh_aura(rank);return plan?std::optional<Milliseconds>{plan->duration}:std::nullopt;}
    if(skill==1261){const auto plan=effects::frigid_armor(rank);return plan?std::optional<Milliseconds>{plan->duration}:std::nullopt;}
    if(skill==823){const auto plan=effects::burning_speed(rank);return plan?std::optional<Milliseconds>{plan->duration_ms}:std::nullopt;}
    if(skill==180||skill==3375)return 60000;
    if(skill==38){const auto plan=effects::channeling(rank);return plan?std::optional<Milliseconds>(plan->duration):std::nullopt;}
    if(const auto plan=effects::attunement(skill,rank))return plan->duration;
    return {};
}
inline unsigned attunement_refund(unsigned cost){const auto numerator=10u+3u*cost,q=numerator/10,r=numerator%10;return q+(r>5||(r==5&&(q&1)));}
inline Milliseconds owned_self_enchantment_base(unsigned skill,unsigned rank){
    if(skill==2138){const auto plan=regeneration::reviewed(skill,rank);return plan?plan->duration:0;}
    if(skill==1261){const auto plan=effects::frigid_armor(rank);return plan?plan->duration:0;}
    if(skill==1375){const auto plan=effects::stoneflesh_aura(rank);return plan?plan->duration:0;}
    if(skill==38){const auto plan=effects::channeling(rank);return plan?plan->duration:0;}
    if(skill==180||skill==3375){const auto plan=effects::aura_restoration_plan(rank);return plan?plan->duration:0;}
    if(const auto plan=effects::attunement(skill,rank))return plan->duration;
    if(skill==165){const auto plan=effects::armor_earth(rank);return plan?plan->duration:0;}
    if(skill==82){const auto plan=effects::mantra_recall(rank);return plan?plan->duration:0;}
    const auto plan=effects::conjure(skill,rank);return plan?plan->duration:0;
}
class Runtime {
    std::uint32_t actor_;
    std::uint64_t next_well_attempt_=1,next_necrotic_attempt_=1;
    struct Pending {Request request;unsigned healing_rank=0,divine_rank=0;float range=0;bool allow_self=true;Milliseconds complete=0;unsigned effect_rank=0,energy_storage_rank=0;std::optional<effects::Scope> resurrection_scope;std::optional<combat::weapon::Owned> weapon;unsigned strength=0;std::optional<std::uint64_t> control_epoch;unsigned primary=0,fast_rank=0;bool pvp=false;std::optional<effects::FeastCastAnchor> feast_anchor;std::optional<effects::Scope> balance_caster;std::optional<effects::HexCopySession::CastAdmission> session_admission;Milliseconds began=0;std::optional<effects::Scope> use_owner;bool observe_panic=false;std::optional<effects::Scope> use_target;bool observe_mind=false,observe_keystone=false,observe_demonic=false,observe_copy=false,observe_glimmering=false;std::unique_ptr<combat::weapon::AttackLease> attack_lease;std::optional<effects::ElementalGlyphQuote> glyph_quote;std::uint64_t glyph_event=0;bool observe_duration=false,observe_local_duration=false;std::optional<effects::WeaponSpellPlan> weapon_spell;bool health_completion=false,elemental_completion=false,direct_heal_completion=false,gift_health_completion=false,spirit_light_completion=false,mend_body_soul_completion=false,ghostmirror_light_completion=false,healing_ribbon_completion=false,status_heal_completion=false,condition_removal_completion=false,party_heal_completion=false,effect_removal_completion=false,ally_cleansing_completion=false,energy_drain_completion=false,armor_earth_completion=false,simple_enchantment_completion=false,healing_ring_completion=false,resurrection_completion=false,burning_speed_completion=false,energy_enchantment_completion=false,life_steal_completion=false,life_steal_spell_completion=false,wound_spell_completion=false;std::shared_ptr<pets::CharmAttempt> pet_charm;std::unique_ptr<pets::CastLease> pet_lease;std::optional<effects::Scope> pet_target;std::uint64_t pet_sequence=0;std::optional<effects::Scope> pet_offensive_owner,pet_offensive_target;bool dagger_staged=false;};
    struct DaggerFailedSecondDraft {
        std::shared_ptr<combat::weapon::DaggerDualFailureFeedbackDraft> feedback;
        combat::weapon::AttackPermit attempt;effects::Scope source,target;
        unsigned skill=0,copy=0;std::optional<std::uint64_t> control;
    };
    std::shared_ptr<combat::weapon::DaggerSuccessfulSecondDraft> dagger_successful_second_draft_;
    unsigned dagger_successful_skill_draft_=0,dagger_successful_copy_draft_=0;
    std::optional<std::uint64_t> dagger_successful_control_draft_;
    std::optional<effects::HexCopySession::CastAdmission> dagger_successful_session_draft_;
    std::optional<effects::CooldownAmendment> pending_contact_amendment_draft_;
    void cancel_dagger_successful_second_draft(const Hooks& hooks,bool publish){
        if(!dagger_successful_second_draft_||(publish&&!hooks.target_scope))return;
        auto phase=std::move(dagger_successful_second_draft_);phase->cancel();dagger_successful_session_draft_.reset();
        if(publish&&hooks.target_scope&&hooks.target_scope(actor_)==phase->source())
            hooks.events({cancellation_packet(actor_,{dagger_successful_skill_draft_,dagger_successful_copy_draft_})});
    }
    bool update_dagger_successful_second_draft(Milliseconds now,const Hooks& hooks){
        if(!dagger_successful_second_draft_)return true;
        const auto phase=dagger_successful_second_draft_;
        if(!hooks.target_scope)return false; // absent authority is retry, not retirement
        if(hooks.target_scope(actor_)!=phase->source()){
            cancel_dagger_successful_second_draft(hooks,false);return true;
        }
        if(!phase->open()||hooks.target_scope(phase->target().actor)!=phase->target()){
            cancel_dagger_successful_second_draft(hooks,true);return true;
        }
        if(dagger_successful_session_draft_&&(!hooks.valid_session_cast||!hooks.valid_session_cast(*dagger_successful_session_draft_,now))){
            cancel_dagger_successful_second_draft(hooks,true);return true;
        }
        const auto resources=hooks.resources?hooks.resources(actor_):std::nullopt;
        if(!resources)return false; // unavailable authority is not proof of death
        if(!resources->health){cancel_dagger_successful_second_draft(hooks,true);return true;}
        if(dagger_successful_control_draft_){
            if(!hooks.control_epoch)return false;
            const auto control=hooks.control_epoch();if(!control)return false;
            if(control!=dagger_successful_control_draft_){cancel_dagger_successful_second_draft(hooks,true);return true;}
        }
        if(now<phase->due())return true;
        if(!hooks.complete_dagger_successful_second)return false;
        const auto before=cooldown_source_snapshot(phase->source(),now,hooks);if(!before)return false;
        auto result=hooks.complete_dagger_successful_second?hooks.complete_dagger_successful_second(phase,*before,now):combat::weapon::DaggerPaidCompletionDraft{};
        if(result.retry){if(result.publication)result.publication();return false;}
        dagger_successful_second_draft_.reset();dagger_successful_session_draft_.reset();
        if(result.accepted)hooks.events({terminal(47,actor_),cancellation_packet(actor_,{dagger_successful_skill_draft_,dagger_successful_copy_draft_})});
        else{phase->cancel();hooks.events({cancellation_packet(actor_,{dagger_successful_skill_draft_,dagger_successful_copy_draft_})});}
        if(result.amendment){
            const auto& amendment=*result.amendment;
            const bool actual=std::any_of(result.outcomes.begin(),result.outcomes.end(),[&](const auto& attack){return attack.attacker==phase->source()&&attack.event==amendment.physical_event&&!attack.skill_failed&&(attack.outcome==combat::weapon::AttackOutcome::block||attack.outcome==combat::weapon::AttackOutcome::hit);});
            if(actual&&amendment.slot==phase->slot()&&amendment.skill_use==phase->skill_use()){
                pending_contact_amendment_draft_=amendment;
                if(consume_cooldown_amendment(amendment,now,hooks))pending_contact_amendment_draft_.reset();
            }else if(hooks.diagnostic)hooks.diagnostic(dagger_successful_skill_draft_,"contact_amendment_scope_rejected");
        }
        if(result.publication)result.publication();
        return !pending_contact_amendment_draft_;
    }
    std::optional<DaggerFailedSecondDraft> dagger_failed_second_draft_;
    void cancel_dagger_failed_second_draft(const Hooks& hooks,bool publish){
        if(!dagger_failed_second_draft_)return;
        auto phase=std::move(*dagger_failed_second_draft_);dagger_failed_second_draft_.reset();phase.feedback->cancel();
        if(publish&&hooks.target_scope&&hooks.target_scope(actor_)==phase.source)
            hooks.events({cancellation_packet(actor_,{phase.skill,phase.copy})});
    }
    void update_dagger_failed_second_draft(Milliseconds now,const Hooks& hooks){
        if(!dagger_failed_second_draft_)return;
        const auto& phase=*dagger_failed_second_draft_;
        if(!hooks.target_scope||hooks.target_scope(actor_)!=phase.source||hooks.target_scope(phase.target.actor)!=phase.target||
           (phase.control&&(!hooks.control_epoch||hooks.control_epoch()!=phase.control))){
            cancel_dagger_failed_second_draft(hooks,false);return;
        }
        const auto source_resources=hooks.resources?hooks.resources(actor_):std::nullopt;
        if(!source_resources||!source_resources->health){cancel_dagger_failed_second_draft(hooks,false);return;}
        if(now<phase.feedback->second_due())return;
        auto due=std::move(*dagger_failed_second_draft_);dagger_failed_second_draft_.reset();
        if(!hooks.complete_dagger_failed_second){due.feedback->cancel();return;}
        auto result=hooks.complete_dagger_failed_second(due.feedback,now);
        if(result.publication)result.publication();
        if(!result.accepted){due.feedback->cancel();return;}
        if(auto wire=due.feedback->consume(1,due.source,due.target,due.skill,due.copy,due.attempt,now)){
            // Original158/kind47->event18, then original159/kind38 component7.
            // World has already owned the real failed physical attempt; no hit or fee is fabricated.
            hooks.events({terminal(47,actor_),std::move(*wire),cancellation_packet(actor_,{due.skill,due.copy})});
        }
    }
    std::optional<Pending> pending_;
    std::optional<std::uint64_t> pending_attack_epoch_;
    Milliseconds aftercast_until_=0;
    std::map<std::pair<unsigned,unsigned>,Milliseconds> recharging_;
    // Observed World authority is a separate layer, never a local cooldown.
    std::map<std::pair<unsigned,unsigned>,Milliseconds> external_disables_;
    std::map<std::pair<unsigned,unsigned>,std::optional<effects::CooldownOrigin>> cooldown_sources_;
    std::optional<effects::Scope> cooldown_source_owner_;std::uint64_t cooldown_source_revision_=1;
    bool cooldown_sources_valid_=true;
    std::optional<effects::OrderedAttackCooldowns> ordered_attack_cooldowns_;
    std::uint64_t processed_control_epoch_=0,processed_completion_order_=0,processed_reset_order_=0,processed_amendment_order_=0;
    std::map<std::pair<unsigned,unsigned>,effects::CooldownStamp> renewal_resets_;
    std::map<std::pair<unsigned,unsigned>,effects::CopyRestorationCooldown> copy_restorations_;
    void touch_cooldown_sources(){if(cooldown_source_revision_==UINT64_MAX)cooldown_sources_valid_=false;else ++cooldown_source_revision_;}
    bool cooldown_owner_matches(const effects::CooldownStamp& stamp,Milliseconds now,const Hooks& hooks){
        const auto owner=hooks.target_scope?hooks.target_scope(actor_):std::nullopt;
        if(!owner||!effects::cooldown_owner(*owner)||stamp.owner!=*owner||stamp.owner.actor!=actor_||!stamp.order||stamp.at!=now)return false;
        if(cooldown_source_owner_&&*cooldown_source_owner_!=*owner){cooldown_sources_valid_=false;return false;}
        cooldown_source_owner_=*owner;return true;
    }
    bool record_local_cooldown(Slot slot,Milliseconds until,std::optional<effects::CooldownOrigin> source){
        const auto reset=renewal_resets_.find({slot.skill,slot.copy});
        if(reset!=renewal_resets_.end()&&(!source||source->stamp.owner!=reset->second.owner||source->stamp.order<=reset->second.order))return false;
        if(ordered_attack_cooldowns_&&source&&effects::flourish_attack_skill(slot.skill)){
            auto ordered=*ordered_attack_cooldowns_;
            const auto admitted=ordered.put(source->stamp,slot,until);
            using Result=effects::OrderedAttackCooldowns::Result;
            if(admitted==Result::accepted||admitted==Result::duplicate){
                const auto suppressed=ordered.reset_supersedes(source->stamp,slot);
                if(suppressed){ordered_attack_cooldowns_=std::move(ordered);if(*suppressed)return false;}
                else cooldown_sources_valid_=false;
            }else if(admitted==Result::stale)return false; // Confirmed old work cannot install a timer after a checkpoint.
            else cooldown_sources_valid_=false; // Never guess on a conflicting/full ordering channel.
        }
        const auto key=std::pair{slot.skill,slot.copy};const auto old=recharging_.find(key);
        if(old==recharging_.end()||until>=old->second){recharging_[key]=until;cooldown_sources_[key]=std::move(source);touch_cooldown_sources();}
        return true;
    }
    bool accept_renewal_terminal(const effects::RenewalTerminal& receipt,const Pending& p,std::uint64_t use_event,
            Milliseconds now,Milliseconds base,Milliseconds existing,std::optional<std::uint64_t> control,const Hooks& hooks){
        const auto& c=receipt.completion;const Slot slot{p.request.skill,p.request.copy};
        if(!p.glyph_quote||p.glyph_quote->glyph.plan.skill!=203||receipt.glyph_effect!=p.glyph_quote->glyph.effect||
           receipt.control_epoch!=control.value_or(0)||!c.instant_recharge||c.skill_use!=(control?p.glyph_event:use_event)||
           c.slot!=slot||c.base_ms!=base||c.existing_until!=existing||!cooldown_owner_matches(c.stamp,now,hooks))return false;
        const auto key=std::pair{slot.skill,slot.copy};const auto reset=renewal_resets_.find(key);
        if(reset!=renewal_resets_.end()&&reset->second.order>=c.stamp.order)return false;
        renewal_resets_.insert_or_assign(key,c.stamp);
        const auto source=cooldown_sources_.find(key);
        if(source==cooldown_sources_.end()||!source->second||source->second->stamp.order<=c.stamp.order){recharging_.erase(key);cooldown_sources_.erase(key);}
        processed_completion_order_=std::max(processed_completion_order_,c.stamp.order);
        touch_cooldown_sources();return true;
    }

    std::set<std::pair<unsigned,unsigned>> spent_signets_;
    std::uint64_t last_signet_reset_=0,last_flourish_reset_=0;
    struct Effect {std::uint32_t id;Milliseconds ends;unsigned rank=0;Milliseconds applied=0;};
    std::map<unsigned,Effect> effects_;
    unsigned glyph_charges_=0;
    std::uint32_t next_effect_=1;
    std::optional<std::uint64_t> trusted_generation_;
    static unsigned remaining_seconds(Milliseconds until,Milliseconds now){
        const auto remaining=until>now?until-now:0;
        return static_cast<unsigned>(std::min<std::uint64_t>(UINT32_MAX,remaining/1000+(remaining%1000!=0)));
    }
    bool synchronize_disables(Milliseconds now,const Hooks& hooks){
        if((!hooks.skill_disabled_until&&!hooks.skill_disabled_slot)||!hooks.current_skillbar)return true;
        const auto bar=hooks.current_skillbar();if(!bar)return false;
        auto proposed=external_disables_;std::vector<Packet> packets;std::set<std::pair<unsigned,unsigned>> seen;
        for(const auto& slot:*bar){
            if(!slot.skill){if(slot.copy)return false;continue;}
            if(!find_skill(slot.skill)||!seen.emplace(slot.skill,slot.copy).second)return false;
            const auto global=hooks.skill_disabled_until?hooks.skill_disabled_until(slot.skill,now):std::optional<Milliseconds>{0};
            const auto local=hooks.skill_disabled_slot?hooks.skill_disabled_slot(slot,now):std::optional<Milliseconds>{0};
            if(!global||!local)return false;const auto until=std::max(*global,*local);
            const auto key=std::pair{slot.skill,slot.copy};const auto base=recharging_.find(key),old=proposed.find(key);
            const auto local_deadline=base==recharging_.end()?0:base->second;
            const auto old_deadline=old==proposed.end()?0:old->second;
            if(until!=old_deadline&&hooks.recall_feedback_pending){
                const auto pending=hooks.recall_feedback_pending(slot,now);if(!pending)return false;
                if(*pending)continue; // Retained return publication owns this UI/cache import.
            }
            const auto before=std::max(local_deadline,old_deadline),after=std::max(local_deadline,until);
            if(until>now)proposed[key]=until;else proposed.erase(key);
            if(after>now&&after!=before)packets.push_back(slot_event(228,actor_,slot,remaining_seconds(after,now)));
            else if(after<=now&&old_deadline&&base==recharging_.end())packets.push_back(slot_event(229,actor_,slot));
        }
        external_disables_=std::move(proposed);if(!packets.empty())hooks.events(packets);return true;
    }
    static bool supported(unsigned id){return id==2138||id==938||id==2222||id==227||id==274||id==2216||id==2221||id==1381||id==1375||id==308||id==2191||id==1261||id==1727||id==1728||id==1950||id==2093||id==1954||id==2097||id==377|| id==2354||id==2355||id==1061||id==786||id==858||id==2055||id==2056||id==830||id==814||id==571||id==823||id==2213||id==2193||id==2103||id==1382||id==1689||id==2356||id==2353||id==2358||id==2359||id==2229||id==1363||id==379||id==317||id==363||id==1700||id==1121||id==915||id==1234||id==1741||id==2062||id==887||id==1209||bool(effects::weapon_spell(id,0,8,0))||id==2006||id==74||id==75||id==257||id==450||id==876||id==817||id==2863||id==910||id==1130||id==364||id==367||id==254||id==318||id==3204||id==347||id==348||id==2858||id==298||id==303||id==316||id==290||id==297||id==294||id==2887||id==262||id==261||id==259||id==245||id==144||id==1392||id==886||id==1411||id==127||id==128||id==903||id==121||id==113||id==106||id==157||id==119||id==107||id==456||id==21||id==1048||id==1337||id==1347||id==936||id==955||id==877||id==766||id==882||id==3188||id==934||id==343||id==2883||id==368||id==302||id==389||id==375||id==380||id==372||id==370||id==345||id==378||id==312||id==455||id==33||id==11||id==130||id==172||id==198||id==199||id==203|| id==185|| id==374||id==3002||id==362||id==448||id==3060||id==1404||id==453||id==3141||id==344||id==371||id==361||id==187||id==192||id==167||id==215||id==204||(id==175&&bool(effects::ward_elements_plan(0)))||(id==176&&bool(effects::ward_melee_plan(0)))||(id==177&&bool(effects::ward_foes_plan(0)))||id==195||id==1094||id==162||id==182||id==207||id==221||id==943||id==427||id==1262||id==63||id==55||id==3289||id==49||id==2734||id==165||id==52||id==82||id==57||id==22||id==1049||id==30||id==86||id==10||id==62||id==151||bool(effects::interrupt_skill(id))||(id>=6&&id<=9)||id==349||id==292||id==1526||id==110||id==13||id==15||id==14||id==16||id==17||id==146||id==296||id==247||id==1113||id==100||id==28||id==252||id==258||id==299||id==2061||id==76||id==54||id==3152||id==209||id==2803||id==210||id==211||id==212||id==213||id==214||id==237||id==234||id==27||id==67||id==68||id==69||id==141||id==143||id==431||id==432||bool(combat::weapon::preparation_contact_definition(id,0))||bool(effects::attack_slow_hex(id,0))||id==105||id==115||id==230||id==346||id==995||id==572||id==952||id==1085||id==38||id==37||id==3373||id==180||id==3375||id==200||id==197||bool(effects::condition_damage(id))||bool(combat::weapon::attack_skill(id,0))||bool(effects::action_damage_hex(id))||bool(effects::degeneration_hex(id))||bool(effects::wound_spell(id))||bool(effects::ally_cleansing(id))||bool(effects::shadow_damage(id))||bool(effects::energy_drain(id))||bool(effects::resource_gain(id))||bool(effects::empathy(id))||bool(effects::elemental(id))||bool(effects::life_steal(id))||bool(effects::condition_spell(id))||bool(effects::condition_transfer(id))||bool(effects::blindness(id))||bool(effects::lightning(id))||bool(effects::party_resurrection(id))||bool(effects::resurrection(id))||bool(effects::direct_heal(id))||id==171||id==1374||id==1661||id==217||id==283||id==1396||bool(effects::attunement(id))||id==1043||id==174||id==937||id==288||id==446||id==287||id==3232||id==1397||id==279||id==1117||id==280||id==1119||id==1691||id==1401||id==301||id==2003||(id>=275&&id<=278);}
    Reason begin(Milliseconds now,Request request,unsigned primary,
                 const attributes::Ranks& effective,bool can_cast,const Hooks& hooks,bool pvp,std::optional<effects::HexCopySession::CastAdmission> admission={}){
        auto reject=[&](Reason r){hooks.events({cancellation_packet(actor_,{request.skill,request.copy})});return r;};
        if(request.skill>3432)return Reason::unsupported;
        if(!can_cast)return reject(Reason::mode);
        if(request.skill==3147&&!pvp)return reject(Reason::unsupported);
        if(request.skill==2864&&!pvp)return reject(Reason::unsupported);
        if(request.skill==2863&&!pvp)return reject(Reason::unsupported);
        if(request.skill==3375&&!pvp)return reject(Reason::unsupported);
        const auto* skill=reviewed_cast_record(request.skill,pvp);
        const bool pet_charm_cast=request.skill==411;
        const bool pet_comfort_cast=request.skill==436;
        if(pet_charm_cast&&(!hooks.pet_charm_reserve||!hooks.pet_charm_accept||!hooks.pet_charm_cancel_reservation||
           !hooks.pet_charm_cancel||!hooks.pet_charm_completed||!hooks.target_scope))return reject(Reason::unsupported);
        if(pet_comfort_cast&&(!hooks.pet_owned_scope||!hooks.pet_comfort_ready||!hooks.pet_comfort_completed||!hooks.target_scope))return reject(Reason::unsupported);

        const auto well_cast=effects::well_cast_plan(request.skill,
            skill->attribute<51?effective[skill->attribute]:0u);
        const auto traversal=request.skill==97?effects::necrotic_traversal(effective[5]):std::nullopt;
        if(request.skill==97&&(!traversal||!hooks.necrotic_available||!hooks.necrotic_traversal||
           !hooks.target_scope||!hooks.before_spell_effect||!hooks.consume_energy_at||next_necrotic_attempt_==UINT64_MAX))return reject(Reason::unsupported);
        const auto sorrow=request.skill==1363?effects::signet_sorrow(effective[6]):std::nullopt;
        if(request.skill==1363&&(!sorrow||!hooks.signet_sorrow||!hooks.target_scope||!hooks.projectile_target_valid))return reject(Reason::unsupported);
        if(well_cast&&(!hooks.well_corpse||!hooks.target_scope))return reject(Reason::unsupported);
        const auto glyph_cast=(request.skill==198||request.skill==199||request.skill==203)?effects::elemental_glyph(request.skill,primary==6?effective[12]:0u):std::nullopt;
        const auto demonic_cast=request.skill==130?effects::demonic_flesh(effective[4]):std::nullopt;
        const auto divine_cast=request.skill==310?effects::divine_spirit(effective[16]):std::nullopt;
        if(request.skill==310&&(!divine_cast||!hooks.divine_spirit))return reject(Reason::unsupported);
        const auto selfless_cast=(request.skill==1952||request.skill==2095)&&hooks.allegiance_rank
            ?hooks.allegiance_rank(request.skill):std::optional<unsigned>{};
        if((request.skill==1952||request.skill==2095)&&(!selfless_cast||!effects::selfless_spirit(request.skill,*selfless_cast)||!hooks.selfless_spirit))return reject(Reason::unsupported);
        if(hooks.prepare_demonic_flesh_use&&(!hooks.commit_demonic_flesh_use||!hooks.cancel_demonic_flesh_use))return reject(Reason::unsupported);
        if(demonic_cast&&(!hooks.demonic_flesh||!hooks.prepare_demonic_flesh_use||!hooks.commit_demonic_flesh_use||!hooks.cancel_demonic_flesh_use||!hooks.target_scope||next_effect_>=0x80000000u))return reject(Reason::unsupported);
        if(glyph_cast&&(!hooks.apply_elemental_glyph||!hooks.elemental_glyph_snapshot||!hooks.pay_elemental_glyph||!hooks.settle_elemental_glyph||!hooks.glyph_clock||next_effect_>=0x80000000u))return reject(Reason::unsupported);
        if(request.skill==203&&(!hooks.finish_renewal_spell||!hooks.prepare_skill_use||!hooks.abort_skill_use||!hooks.finalize_skill_use||!hooks.control_epoch||!hooks.cooldown_control_source))return reject(Reason::unsupported);
        const auto echo_cast=effects::echo(request.skill);
        const auto recall_cast=request.skill==925?effects::recall_shadow():std::nullopt;
        const auto mimicry_cast=request.skill==65?effects::arcane_mimicry():std::nullopt;
        if(hooks.completed_skill_use&&(!hooks.prepare_skill_use||!hooks.abort_skill_use||!hooks.finalize_skill_use||!hooks.cooldown_completion_source||!hooks.target_scope))return reject(Reason::unsupported);
        if(echo_cast&&(!hooks.arm_echo||!hooks.completed_skill_use||!hooks.status_mutation||!hooks.effect_snapshot||next_effect_>=0x80000000u))return reject(Reason::unsupported);
        const bool resurrection_completion=bool(hooks.resurrection_completed)&&effects::is_spell(*skill)&&
            (bool(effects::resurrection(request.skill))||bool(effects::party_resurrection(request.skill)));
        const bool energy_enchantment_completion=(request.skill==119||request.skill==157)&&bool(hooks.energy_enchantment_completed);
        const bool life_steal_spell_completion=bool(effects::life_steal(request.skill))&&effects::is_spell(*skill)&&bool(hooks.life_steal_spell_completed);
        const bool life_steal_completion=bool(effects::life_steal(request.skill))&&(bool(hooks.complete_life_steal)||life_steal_spell_completion);
        if(request.skill==227&&(!effects::glimmering_mark(skill->attribute<51?effective[skill->attribute]:0u)||
           !hooks.glimmering_mark_completed||!hooks.glimmering_targeted_use||!hooks.before_spell_effect||
           !hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const bool wound_spell_completion=(request.skill==44||request.skill==1052||request.skill==3184)&&bool(hooks.wound_spell_completed);
        if((life_steal_spell_completion||wound_spell_completion)&&!hooks.before_spell_effect)return reject(Reason::unsupported);
        if(energy_enchantment_completion&&(!hooks.before_spell_effect||!hooks.enchantment_duration_ready))return reject(Reason::unsupported);
        const bool simple_enchantment_completion=bool(hooks.simple_enchantment_completed)&&
            (request.skill==2138||request.skill==113||request.skill==1261||request.skill==1375||request.skill==38||request.skill==180||request.skill==3375||bool(effects::attunement(request.skill))||request.skill==82||request.skill==182||request.skill==207||request.skill==221||request.skill==245||request.skill==254||request.skill==257||request.skill==259||request.skill==261||request.skill==262||request.skill==274||request.skill==310);
        if(simple_enchantment_completion&&!hooks.before_spell_effect)return reject(Reason::unsupported);
        if(request.skill==2138&&(!regeneration::reviewed(2138,effective[6])||!simple_enchantment_completion||!hooks.completed_action))return reject(Reason::unsupported);
        const bool observe_glimmering=bool(hooks.glimmering_targeted_use)&&request.target&&effects::glimmering_direct_target_skill(request.skill);
        std::optional<effects::Scope> use_owner;
        if(pet_charm_cast||pet_comfort_cast||request.skill==938||request.skill==2222||observe_glimmering||wound_spell_completion||life_steal_completion||energy_enchantment_completion||(request.skill==823&&hooks.burning_speed_completed)||resurrection_completion||simple_enchantment_completion||traversal||well_cast||hooks.completed_skill_use||hooks.prepare_demonic_flesh_use||hooks.panic_skill_used||hooks.mind_wrack_targeted||hooks.keystone_skill_used||request.skill==298||request.skill==303||request.skill==316||request.skill==318||request.skill==3204||request.skill==347||request.skill==1411||request.skill==348||request.skill==2858||request.skill==1954||request.skill==2097||request.skill==364||request.skill==343||request.skill==2883||request.skill==368||request.skill==2354||request.skill==2355||request.skill==2356||request.skill==389||request.skill==63||request.skill==55||request.skill==3289){
            if(!hooks.target_scope)return reject(Reason::unsupported);
            use_owner=hooks.target_scope(actor_);
            if(!use_owner||!use_owner->instance||use_owner->actor!=actor_||!use_owner->generation)return reject(Reason::policy);
        }
        if(traversal){
            const auto available=hooks.necrotic_available(*use_owner,*traversal,now);
            if(!available||!*available)return reject(Reason::target);
        }
        if(request.skill==367||request.skill==2216){
            if((request.skill==367?!hooks.shields_up:!hooks.dont_trip)||!hooks.target_scope||!hooks.target_valid)return reject(Reason::unsupported);
            use_owner=hooks.target_scope(actor_);
            if(!use_owner||!use_owner->instance||use_owner->actor!=actor_||!use_owner->generation)return reject(Reason::policy);
        }
        if((request.skill==1950||request.skill==2093)&&
           (!hooks.signet_of_corruption||!hooks.allegiance_rank||!hooks.projectile_target_valid||!hooks.target_scope))
            return reject(Reason::unsupported);
        if((request.skill==1954||request.skill==2097)&&
           (!hooks.save_yourselves||!hooks.allegiance_rank||!hooks.target_scope||!hooks.target_valid))
            return reject(Reason::unsupported);
        if(request.skill==475){
            if(!hooks.quickening_zephyr_spawn||!hooks.target_scope||!hooks.target_valid||
               !effects::quickening_zephyr(effective[24]))return reject(Reason::unsupported);
            use_owner=hooks.target_scope(actor_);
            if(!use_owner||!use_owner->instance||use_owner->actor!=actor_||
               !use_owner->generation)return reject(Reason::policy);
        }
        if((request.skill==343||request.skill==2883)&&(!hooks.for_great_justice||!hooks.target_valid))return reject(Reason::unsupported);
        if(request.skill==303&&(!hooks.convert_hexes||!hooks.before_spell_effect||!hooks.target_scope||!hooks.ally_enchantment_target_valid))return reject(Reason::unsupported);
        if(request.skill==364&&(!hooks.charge||!hooks.target_valid))return reject(Reason::unsupported);
        if((request.skill==348||request.skill==2858)&&(!hooks.watch_yourself||!hooks.target_valid||(request.skill==2858&&!pvp)))return reject(Reason::unsupported);
        if((request.skill==318||request.skill==3204)&&(!hooks.defy_pain||!hooks.target_valid||!hooks.effect_snapshot||(request.skill==3204&&!pvp)))return reject(Reason::unsupported);
        if(request.skill==347&&(!hooks.endure_pain||!hooks.target_valid))return reject(Reason::unsupported);
        if(request.skill==1411&&(!hooks.stamina_signet||!hooks.target_valid))return reject(Reason::unsupported);
        if(request.skill==316&&(!hooks.to_limit||!hooks.target_valid))return reject(Reason::unsupported);
        if((request.skill==934||request.skill==3188)&&((request.skill==3188&&!pvp)||!hooks.unnatural_signet||!hooks.target_scope||!hooks.projectile_target_valid||(pvp&&(!hooks.prepare_skill_use||!hooks.finalize_skill_use||!hooks.abort_skill_use||!hooks.cooldown_completion_source))))return reject(Reason::unsupported);
        if(request.skill==368&&(!hooks.i_will_survive||!hooks.target_valid))return reject(Reason::unsupported);
        if(request.skill==2354&&(!hooks.dodge_this||!hooks.norn_rank||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==2355&&(!hooks.i_am_the_strongest||!hooks.norn_rank||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==2356&&(!hooks.i_am_unstoppable||!hooks.norn_rank||!hooks.target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==2353&&(!hooks.finish_him||!hooks.norn_rank||!hooks.projectile_target_valid||!hooks.target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==2358&&(!hooks.you_move_like_a_dwarf||!hooks.norn_rank||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==2359&&(!hooks.you_are_all_weaklings||!hooks.norn_rank||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==389&&(!hooks.prepare_flourish_reset||!hooks.commit_flourish_reset||!hooks.current_skillbar||!hooks.skill_disabled_slot||!hooks.prepare_skill_use||!hooks.finalize_skill_use||!hooks.abort_skill_use||!hooks.cooldown_completion_source))return reject(Reason::unsupported);
        if(request.skill==63&&(!hooks.prepare_keystone_reset||!hooks.commit_keystone_reset||!hooks.keystone_skill_used||!hooks.current_skillbar||!hooks.skill_disabled_slot))return reject(Reason::unsupported);
        if((skill->type==7||skill->type==15||skill->type==27)&&hooks.effect_snapshot){
            const auto effects=hooks.effect_snapshot(now);
            if(!effects)return reject(Reason::policy);
            if(effects::control_skill_forbidden(*skill,*effects,now))return reject(Reason::policy);
        }
        // The overcast BYTE has meaning only with specialbit0 set. Ordinary
        // healing rows retain unrelated nonzero values there (e.g.Orison5).
        const bool dagger_staged=combat::weapon::dagger_paid_id_draft(request.skill)&&bool(hooks.prepare_dagger_attack)&&bool(hooks.complete_dagger_attack)&&(!combat::weapon::dagger_dual_role(request.skill)||(bool(hooks.complete_dagger_failed_second)&&bool(hooks.complete_dagger_successful_second)&&bool(hooks.cooldown_amendment_source)));
        if(dagger_staged&&combat::weapon::dagger_dual_role(request.skill)&&(!hooks.prepare_skill_use||!hooks.finalize_skill_use||!hooks.abort_skill_use||!hooks.cooldown_completion_source||!hooks.control_epoch||!hooks.current_skillbar||!hooks.skill_disabled_slot))return reject(Reason::unsupported);
        if(dagger_staged&&request.skill==976&&(!hooks.prepare_skill_use||!hooks.finalize_skill_use||!hooks.abort_skill_use||!hooks.cooldown_completion_source||!hooks.control_epoch||!hooks.cooldown_control_source))return reject(Reason::unsupported);
        if(dagger_staged&&!combat::weapon::dagger_mode_allowed(request.skill,pvp))return reject(Reason::unsupported); // exact native reciprocal aliases only
        if((!supported(request.skill)&&!dagger_staged&&!pet_charm_cast&&!pet_comfort_cast&&!(traversal&&hooks.necrotic_traversal)&&!(request.skill==475&&hooks.quickening_zephyr_spawn)&&!(request.skill==1336&&hooks.spiritual_pain)&&!(request.skill==307&&hooks.reversal_fortune_completed)&&!(request.skill==2353&&hooks.finish_him)&&!(request.skill==377&&hooks.wary_stance_reward_ready)&&!(request.skill==367&&hooks.shields_up)&&!(well_cast&&hooks.well_corpse)&&!(sorrow&&hooks.signet_sorrow)&&!(divine_cast&&hooks.divine_spirit)&&!(selfless_cast&&hooks.selfless_spirit)&&!echo_cast&&!(request.skill==308&&hooks.succor)&&!(request.skill==248&&hooks.retribution_completed)&&!(mimicry_cast&&hooks.mimicry)&&!(recall_cast&&hooks.recall)&&!(hooks.energy_degeneration_hex&&effects::energy_degeneration_hex(request.skill,0)))||(skill->health_sacrifice&&request.skill!=119&&request.skill!=157&&!hooks.health_sacrifice)||((skill->special&1)&&skill->overcast&&!hooks.consume_overcast_energy)||(skill->adrenaline&&(!hooks.consume_adrenaline||!hooks.reconcile_adrenaline)))return reject(Reason::unsupported);
        if(hooks.skill_disabled_until){const auto until=hooks.skill_disabled_until(request.skill,now);if(!until)return reject(Reason::policy);if(now<*until)return reject(Reason::recharge);}
        if(hooks.skill_disabled_slot){const auto until=hooks.skill_disabled_slot({request.skill,request.copy},now);if(!until)return reject(Reason::policy);if(now<*until)return reject(Reason::recharge);}
        if((hooks.prepare_skill_use||hooks.finalize_skill_use||hooks.abort_skill_use)&&
           (!hooks.prepare_skill_use||!hooks.finalize_skill_use||!hooks.abort_skill_use))return reject(Reason::unsupported);
        if((bool(effects::attunement(request.skill))&&!hooks.restore_energy)||(request.skill==1043&&!hooks.speed_multiplier))return reject(Reason::unsupported);
        if((request.skill==431||request.skill==432||bool(combat::weapon::preparation_contact_definition(request.skill,0)))&&(!hooks.status_mutation||!hooks.target_valid))return reject(Reason::unsupported);
        if((request.skill==374||request.skill==3002)&&(!effects::warriors_endurance(request.skill,effective[17])||!hooks.status_mutation||!hooks.effect_snapshot))return reject(Reason::unsupported);
        if(request.skill==362&&(!effects::warriors_cunning(effective[17])||!hooks.status_mutation||!hooks.effect_snapshot))return reject(Reason::unsupported);
        if(request.skill==456&&(!effects::serpents_quickness(effective[24])||!hooks.status_mutation||!hooks.effect_snapshot))return reject(Reason::unsupported);
        if(request.skill==375&&(!effects::dwarven_battle_stance(effective[17])||!hooks.status_mutation||!hooks.effect_snapshot))return reject(Reason::unsupported);
        if(request.skill==380&&(!effects::bonetti_defense(effective[21])||!hooks.status_mutation||!hooks.effect_snapshot||!hooks.begin_action))return reject(Reason::unsupported);
        if(request.skill==377&&(!effects::wary_stance(effective[21])||!hooks.wary_stance_reward_ready||!hooks.wary_stance_reward_ready()||!hooks.status_mutation||!hooks.effect_snapshot||!hooks.begin_action))return reject(Reason::unsupported);
        if(request.skill==450&&(!effects::whirling_defense(effective[23])||!hooks.status_mutation||!hooks.effect_snapshot))return reject(Reason::unsupported);
        if(request.skill==372&&(!effects::gladiator_defense(effective[21])||!hooks.status_mutation||!hooks.effect_snapshot))return reject(Reason::unsupported);
        if(request.skill==370&&(!effects::berserker_stance(effective[17])||!hooks.status_mutation||!hooks.effect_snapshot||!hooks.begin_action))return reject(Reason::unsupported);
        if(request.skill==1727&&(!effects::natural_stride(effective[24])||!hooks.status_mutation||!hooks.effect_snapshot))return reject(Reason::unsupported);
        if(request.skill==1728&&(!effects::hekets_rampage(effective[22])||!hooks.status_mutation||!hooks.effect_snapshot))return reject(Reason::unsupported);
        if(request.skill==1261&&(!effects::frigid_armor(effective[11])||!hooks.effect_snapshot||
           (!simple_enchantment_completion&&(!hooks.status_mutation||!hooks.local_enchantment_duration||next_effect_>=0x80000000u))))return reject(Reason::unsupported);
        if(request.skill==1375&&(!effects::stoneflesh_aura(effective[9])||!hooks.effect_snapshot||
           (!simple_enchantment_completion&&(!hooks.status_mutation||!hooks.local_enchantment_duration||next_effect_>=0x80000000u))))return reject(Reason::unsupported);
        if(request.skill==995&&(!effects::tiger_stance(effective[17])||!hooks.status_mutation||!hooks.effect_snapshot))return reject(Reason::unsupported);
        if(request.skill==572&&(!effects::deadly_paradox(effective[30])||!hooks.status_mutation||!hooks.effect_snapshot||!hooks.current_skillbar||!hooks.skill_disabled_slot||!hooks.target_scope||next_effect_>=0x80000000u))return reject(Reason::unsupported);
        if(request.skill==379&&(!effects::bulls_charge(effective[17])||!hooks.status_mutation||!hooks.effect_snapshot||!hooks.begin_action))return reject(Reason::unsupported);
        if(request.skill==317&&(!effects::battle_rage(effective[17])||!hooks.status_mutation||!hooks.effect_snapshot||!hooks.begin_action))return reject(Reason::unsupported);
        if(request.skill==1209&&(!effects::bestial_fury(effective[22])||!hooks.status_mutation||!hooks.effect_snapshot||
           !hooks.current_skillbar||!hooks.skill_disabled_slot||!hooks.target_scope||next_effect_>=0x80000000u))return reject(Reason::unsupported);
        if(hooks.effect_snapshot&&!hooks.begin_action){const auto snapshot=hooks.effect_snapshot(now);
            if(!snapshot||effects::berserker_stance_active(*snapshot,now)||effects::bonetti_defense_active(*snapshot,now)||effects::wary_stance_active(*snapshot,now)||std::any_of(snapshot->begin(),snapshot->end(),[](const auto& e){return e.skill==379;})||effects::battle_rage_active(*snapshot,now).value_or(true))return reject(Reason::unsupported);
        }
        if(request.skill==345&&(!effects::defensive_stance(effective[21])||!hooks.status_mutation||!hooks.effect_snapshot))return reject(Reason::unsupported);
        if(request.skill==378&&(!effects::shield_stance(effective[21])||!hooks.status_mutation||!hooks.effect_snapshot))return reject(Reason::unsupported);
        if(request.skill==361&&(!effects::dolyak_signet(effective[17])||!hooks.status_mutation||!hooks.effect_snapshot))return reject(Reason::unsupported);
        if(request.skill==346&&(!effects::frenzy(346)||!hooks.status_mutation||!hooks.effect_snapshot))return reject(Reason::unsupported);
        if(request.skill==1700&&(!effects::frenzied_defense(0)||!hooks.status_mutation||!hooks.effect_snapshot))return reject(Reason::unsupported);
        if(request.skill==363&&(!effects::shield_bash(effective[17])||!hooks.status_mutation||!hooks.effect_snapshot))return reject(Reason::unsupported);
        // Lower half belongs to per-caster self buffs; target-owned timed
        // regeneration allocates the upper half to avoid ID collisions.
        if(((bool(effects::elemental_mantra(request.skill))||bool(timing::stance(request.skill)))||bool(effects::attunement(request.skill))||request.skill==1043||request.skill==363||request.skill==346||request.skill==374||request.skill==362||request.skill==361||request.skill==200||request.skill==38||request.skill==37||request.skill==3373||(request.skill==180||request.skill==3375)||(request.skill==431||request.skill==432||bool(combat::weapon::preparation_contact_definition(request.skill,0))))&&next_effect_>=0x80000000u)return reject(Reason::unsupported);
        if(request.skill==1&&((!hooks.heal&&!hooks.heal_scoped)||!hooks.healing_signet_activity))return reject(Reason::unsupported);
        if(hooks.prepare_channeling&&!hooks.complete_channeling)return reject(Reason::unsupported);
        if(request.skill==38&&((!simple_enchantment_completion&&!hooks.status_mutation)||!hooks.prepare_channeling||!hooks.complete_channeling))return reject(Reason::unsupported);
        if((request.skill==37||request.skill==3373)&&!hooks.status_mutation)return reject(Reason::unsupported);
        if((request.skill==180||request.skill==3375)&&(!hooks.heal||!hooks.restore_energy))return reject(Reason::unsupported);
        if(request.skill==292&&((!hooks.infuse_health&&!hooks.infuse_health_completed)||!hooks.target_scope||!hooks.infuse_target_valid||
            (hooks.infuse_health_completed&&!hooks.before_spell_effect)))return reject(Reason::unsupported);
        if(request.skill==1526&&(!effects::imbue_health(effective[44])||(!hooks.imbue_health&&!hooks.imbue_health_completed)||
           !hooks.target_scope||!hooks.infuse_target_valid||(hooks.imbue_health_completed&&!hooks.before_spell_effect)))return reject(Reason::unsupported);
        if(request.skill==146&&!hooks.restore_energy)return reject(Reason::unsupported);
        if((effects::elemental_mantra(request.skill)||timing::stance(request.skill))&&!hooks.status_mutation)return reject(Reason::unsupported);
        if(request.skill==33&&(!hooks.status_mutation||!hooks.target_scope||next_effect_>=0x80000000u))return reject(Reason::unsupported);
        if(request.skill==455&&(!hooks.status_mutation||!hooks.effect_snapshot||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==823&&(!effects::burning_speed(effective[10])||(!hooks.burning_speed&&!hooks.burning_speed_completed)||
            (!hooks.burning_speed_completed&&!hooks.status_mutation)||!hooks.effect_snapshot||!hooks.local_enchantment_duration||
            (hooks.burning_speed_completed&&!hooks.before_spell_effect)||next_effect_>=0x80000000u))return reject(Reason::unsupported);
        if(request.skill==177&&!hooks.ward_foes)return reject(Reason::unsupported);
        if(request.skill==176&&!hooks.ward_melee)return reject(Reason::unsupported);
        if(request.skill==938&&(!effects::defensive_ground_ward_plan(938,effective[9])||!hooks.defensive_ward_completed||!hooks.before_spell_effect||!hooks.target_scope||!hooks.target_valid))return reject(Reason::unsupported);
        if(request.skill==175&&!hooks.ward_elements)return reject(Reason::unsupported);
        if(request.skill==195&&!hooks.complete_damage_field)return reject(Reason::unsupported);
        if((request.skill==247||request.skill==1113)&&!hooks.damage_field)return reject(Reason::unsupported);
        if(request.skill==217&&!hooks.crystal_wave)return reject(Reason::unsupported);
        if((request.skill==283||request.skill==1396)&&((!hooks.status_heal_completed&&!hooks.status_heal)||(hooks.status_heal_completed&&(!hooks.before_spell_effect||!hooks.target_scope||!hooks.status_heal_target_valid))))return reject(Reason::unsupported);
        if(request.skill==867&&!hooks.direct_heal_completed&&(!hooks.has_enchantment||!hooks.restore_energy))return reject(Reason::unsupported);
        if(request.skill==1121&&(!effects::gift_health(effective[13])||(!hooks.gift_health_completed&&!hooks.gift_health)||(hooks.gift_health_completed&&!hooks.before_spell_effect)||!hooks.gift_health_target_valid||
           !hooks.target_scope||!hooks.prepare_skill_use||!hooks.finalize_skill_use||!hooks.abort_skill_use||
           !hooks.current_skillbar||!hooks.skill_disabled_slot))return reject(Reason::unsupported);
        if(request.skill==915&&(!effects::spirit_light(effective[33])||(!hooks.spirit_light_completed&&!hooks.spirit_light)||(hooks.spirit_light_completed&&!hooks.before_spell_effect)||
           !hooks.spirit_light_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==1234&&(!effects::mend_body_soul(effective[33])||
           (!hooks.mend_body_soul_completed&&!hooks.mend_body_soul)||(hooks.mend_body_soul_completed&&!hooks.before_spell_effect)||!hooks.mend_body_soul_target_valid||
           !hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==1741&&(!effects::ghostmirror_light(effective[33])||
           (!hooks.ghostmirror_light_completed&&!hooks.ghostmirror_light)||(hooks.ghostmirror_light_completed&&!hooks.before_spell_effect)||!hooks.ghostmirror_light_target_valid||
           !hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==2062&&(!effects::healing_ribbon(effective[13])||
           (!hooks.healing_ribbon_completed&&!hooks.healing_ribbon)||(hooks.healing_ribbon_completed&&!hooks.before_spell_effect)||!hooks.healing_ribbon_target_valid||
           !hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==887&&(!effects::signet_rejuvenation(effective[13])||
           !hooks.rejuvenation||!hooks.rejuvenation_target_valid||
           !hooks.target_scope))return reject(Reason::unsupported);
        if((request.skill==814||request.skill==115||request.skill==446||(request.skill==288&&!hooks.protective_enchantment))&&!hooks.apply_health_regeneration)return reject(Reason::unsupported);
        if((request.skill==258||request.skill==299||request.skill==2061||(request.skill==288&&hooks.protective_enchantment))&&(!hooks.protective_enchantment||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==288&&!hooks.protective_enchantment&&primary==3&&effective[16]&&!hooks.heal)return reject(Reason::unsupported);
        const auto duration_plan=regeneration::reviewed(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        const auto duration_energy=effects::energy_enchantment(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        const auto duration_tainted=request.skill==113?effects::tainted_flesh(effective[5]):std::nullopt;
        const auto duration_spirit=request.skill==245?effects::protective_spirit(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto duration_aegis=request.skill==257?effects::aegis(effective[15]):std::nullopt;
        const auto duration_vigorous=request.skill==254?effects::vigorous_spirit(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto duration_seed=request.skill==274?effects::healing_seed(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto duration_reversal=request.skill==307?effects::reversal_fortune(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto duration_deflection=request.skill==259?effects::shield_deflection(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto duration_shield_regeneration=request.skill==261?effects::shield_regeneration(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto duration_judgment=request.skill==262?effects::shield_judgment(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto duration_owned_self=(simple_enchantment_completion||(request.skill==165&&hooks.armor_earth_completed))?owned_self_enchantment_base(request.skill,skill->attribute<51?effective[skill->attribute]:0u):0;
        if(duration_owned_self&&!hooks.enchantment_duration_ready)return reject(Reason::unsupported);
        if(request.skill==113&&simple_enchantment_completion&&!hooks.enchantment_duration_ready)return reject(Reason::unsupported);
        const auto duration_base=duration_owned_self?duration_owned_self:duration_tainted?duration_tainted->duration:duration_aegis?duration_aegis->duration:duration_plan?duration_plan->duration:duration_energy?duration_energy->duration:duration_spirit?duration_spirit->duration:duration_deflection?duration_deflection->duration:duration_shield_regeneration?duration_shield_regeneration->duration:duration_judgment?duration_judgment->duration:duration_vigorous?duration_vigorous->duration:duration_seed?duration_seed->duration:duration_reversal?duration_reversal->duration:0;
        const auto local_base=local_enchantment_base(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        const bool observe_local_duration=local_base&&bool(hooks.local_enchantment_duration);
        if(observe_local_duration){const auto duration=hooks.local_enchantment_duration(request.skill,*local_base,now);
            if(!duration||!*duration||now>UINT64_MAX-*duration)return reject(Reason::unsupported);}
        const bool observe_duration=duration_base&&skill->type==6&&bool(hooks.enchantment_duration_ready);
        if(observe_duration&&!hooks.enchantment_duration_ready(request.skill,duration_base,now))return reject(Reason::unsupported);
        if(effects::party_heal(request.skill,0,0)&&((request.skill==2221?!hooks.party_heal_published:(!hooks.party_heal_completed&&!hooks.party_heal))||((request.skill==2221||hooks.party_heal_completed)&&(!hooks.before_spell_effect||!hooks.target_scope))))return reject(Reason::unsupported);
        if((request.skill==301||request.skill==2003)&&((!hooks.remove_effects_completed&&!hooks.remove_effects)||(hooks.remove_effects_completed&&(!hooks.before_spell_effect||!hooks.target_scope))))return reject(Reason::unsupported);
        if(((request.skill>=275&&request.skill<=278)||request.skill==1691||request.skill==1401)&&((!hooks.remove_conditions_completed&&!hooks.remove_conditions)||(hooks.remove_conditions_completed&&(!hooks.before_spell_effect||!hooks.target_scope))||(request.skill==1691&&!hooks.target_scope)))return reject(Reason::unsupported);
        if((request.skill==174||request.skill==937)&&!hooks.hostile_aoe)return reject(Reason::unsupported);
        const bool projectile=bool(effects::earth_projectile(request.skill,0,0));
        const auto sunspear_rank=(request.skill==1816||request.skill==2103)&&hooks.sunspear_rank?hooks.sunspear_rank():std::optional<unsigned>{};
        if((request.skill==1816||request.skill==2103)&&!sunspear_rank)return reject(Reason::unsupported);
        const auto necrosis=request.skill==2103?effects::necrosis(*sunspear_rank):std::nullopt;
        if(request.skill==2103&&(!necrosis||!hooks.necrosis||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto energy_blast=request.skill==2193?effects::energy_blast(effective[12]):std::nullopt;
        if(request.skill==2193&&(!energy_blast||!hooks.energy_blast||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto aneurysm=request.skill==2055?effects::aneurysm(effective[2]):std::nullopt;
        if(request.skill==2055&&(!aneurysm||!hooks.aneurysm||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto deldrimor_rank=(request.skill==2222||request.skill==2213||request.skill==2221||request.skill==2216)&&hooks.deldrimor_rank?hooks.deldrimor_rank():std::optional<unsigned>{};
        const auto snow=request.skill==2222&&deldrimor_rank?effects::snow_storm(*deldrimor_rank):std::nullopt;
        if(request.skill==2222&&(!snow||!hooks.snow_storm_completed||!hooks.before_spell_effect||
           !hooks.target_scope||!hooks.projectile_target_valid))return reject(Reason::unsupported);
        if(request.skill==2216&&(!deldrimor_rank||!effects::dont_trip(*deldrimor_rank)))return reject(Reason::unsupported);
        if(request.skill==2221&&(!deldrimor_rank||!effects::party_heal(2221,*deldrimor_rank,0)))return reject(Reason::unsupported);
        const auto ear_bite=request.skill==2213&&deldrimor_rank?effects::ear_bite(*deldrimor_rank):std::nullopt;
        if(request.skill==2213&&(!ear_bite||!hooks.ear_bite||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto resurrection=effects::resurrection(request.skill,request.skill==1816?*sunspear_rank:(skill->attribute<51?effective[skill->attribute]:0u));
        const auto interrupt=effects::interrupt_skill(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        const bool balance=request.skill==86;
        const auto panic=request.skill==52?effects::panic(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto mind=(request.skill==49||request.skill==2734)?effects::mind_wrack(skill->attribute<51?effective[skill->attribute]:0u,pvp||request.skill==2734):std::nullopt;
        const auto wandering=request.skill==2056&&!pvp?effects::wandering_eye(effective[1]):std::nullopt;
        const auto fevered=(request.skill==55||request.skill==3289)?effects::fevered_dreams(skill->attribute<51?effective[skill->attribute]:0u,pvp||request.skill==3289):std::nullopt;
        if(fevered&&(!hooks.fevered_dreams||!hooks.consume_energy_at||!hooks.target_scope||!hooks.projectile_target_valid||!hooks.before_spell_effect))return reject(Reason::unsupported);
        if(mind&&(!hooks.mind_wrack||!hooks.mind_wrack_targeted||!hooks.consume_energy_at||!hooks.target_scope||!hooks.projectile_target_valid||!hooks.before_spell_effect))return reject(Reason::unsupported);
        if(request.skill==2056&&(!wandering||!hooks.wandering_eye||!hooks.consume_energy_at||!hooks.target_scope||!hooks.projectile_target_valid||!hooks.before_spell_effect))return reject(Reason::unsupported);
        if(panic&&(!hooks.panic||!hooks.panic_skill_used||!hooks.target_scope||!hooks.projectile_target_valid||!hooks.before_spell_effect))return reject(Reason::unsupported);
        if(request.skill==1381&&(!effects::flame_djinn_haste(effective[10])||!hooks.flame_djinn_haste||!hooks.target_scope||!hooks.target_valid||!hooks.before_spell_effect||!hooks.consume_energy_at))return reject(Reason::unsupported);
        if(request.skill==165&&((!hooks.armor_earth&&!hooks.armor_earth_completed)||!hooks.target_scope||!hooks.target_valid||!hooks.before_spell_effect))return reject(Reason::unsupported);
        if(request.skill==427&&(!hooks.antidote_signet||!hooks.target_scope||!hooks.target_valid))return reject(Reason::unsupported);
        if(request.skill==1262&&((!hooks.healing_ring&&!hooks.healing_ring_completed)||!hooks.target_scope||!hooks.target_valid||!hooks.before_spell_effect||!hooks.consume_energy_at))return reject(Reason::unsupported);
        if(request.skill==298&&(!effects::martyr()||!hooks.martyr||!hooks.target_scope||!hooks.target_valid||!hooks.before_spell_effect||!hooks.consume_energy_at))return reject(Reason::unsupported);
        if(request.skill==943&&(!hooks.extinguish||!hooks.target_scope||!hooks.target_valid||!hooks.before_spell_effect||!hooks.consume_energy_at))return reject(Reason::unsupported);
        if(effects::conjure(request.skill,0)&&(!hooks.conjure_enchantment||!hooks.target_scope||!hooks.target_valid||!hooks.before_spell_effect||!hooks.consume_energy_at))return reject(Reason::unsupported);
        if(request.skill==82&&(!hooks.mantra_recall||!hooks.target_scope||!hooks.target_valid||!hooks.before_spell_effect))return reject(Reason::unsupported);
        const auto cry=request.skill==57?effects::cry_frustration(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        if(cry&&(!hooks.cry_frustration||!hooks.target_scope||!hooks.projectile_target_valid||!hooks.before_spell_effect))return reject(Reason::unsupported);
        const auto inspired=effects::inspired_hex(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        if(inspired&&(!hooks.inspired_hex||!hooks.target_scope||(inspired->allied?!hooks.target_valid:!hooks.projectile_target_valid)||!hooks.before_spell_effect))return reject(Reason::unsupported);
        const auto diversion=request.skill==30?effects::diversion(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        if(diversion&&(!hooks.diversion||!hooks.target_scope||!hooks.projectile_target_valid||!hooks.prepare_skill_use||!hooks.skill_disabled_slot))return reject(Reason::unsupported);
        if(balance&&(!hooks.grenths_balance||!hooks.take_grenths_reaction_death||!hooks.before_spell_effect||!hooks.target_scope||!hooks.projectile_target_valid))return reject(Reason::unsupported);
        const auto humility=request.skill==62?effects::signet_humility(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        if(request.skill==10&&(!hooks.hex_breaker||!hooks.target_scope))return reject(Reason::unsupported);
        if(humility&&(!hooks.signet_humility||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto lightning=effects::lightning(request.skill,skill->attribute<51?effective[skill->attribute]:0u,primary==6?effective[12]:0u);
        const auto disenchant=effects::enchantment_removal(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        const auto delusions=request.skill==1337?effects::drain_delusions(effective[3]):std::nullopt;
        if(delusions&&(!hooks.drain_delusions||!hooks.target_scope||!hooks.projectile_target_valid))return reject(Reason::unsupported);
        if(disenchant&&(!(request.skill==1347?bool(hooks.enchantment_removal_result):bool(hooks.enchantment_removal))||!hooks.target_scope||!hooks.projectile_target_valid))return reject(Reason::unsupported);
        const auto shatter=request.skill==27?effects::shatter_delusions(effective[2]):request.skill==67?effects::shatter_hex(effective[2]):request.skill==302?effects::smite_hex(effective[14],primary==3?effective[16]:0u):std::nullopt;
        if(shatter&&(!hooks.shatter_delusions||!hooks.target_scope||(shatter->ally?!hooks.shatter_ally_valid:!hooks.projectile_target_valid)))return reject(Reason::unsupported);
        const auto faint=effects::attack_slow_hex(request.skill,effective[7]);
        const auto swarm=request.skill==105?effects::deathly_swarm(effective[5]):std::nullopt;
        const auto javelin=request.skill==230?effects::lightning_javelin(effective[8]):std::nullopt;
        const auto blindness=effects::blindness(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        const auto transfer=effects::condition_transfer(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        const bool allegiance_skill=request.skill==1950||request.skill==2093||request.skill==1952||request.skill==2095||request.skill==1953||request.skill==2096||request.skill==1954||request.skill==2097;
        const auto allegiance_rank=allegiance_skill&&hooks.allegiance_rank?hooks.allegiance_rank(request.skill):std::optional<unsigned>{};
        if(allegiance_skill&&(!allegiance_rank||*allegiance_rank>12||pvp))return reject(Reason::unsupported);
        const auto ebon_rank=request.skill==2229&&hooks.ebon_vanguard_rank?hooks.ebon_vanguard_rank():std::optional<unsigned>{};
        if(request.skill==2229&&(!ebon_rank||!effects::signet_infection_condition(*ebon_rank)))return reject(Reason::unsupported);
        const auto norn_rank=(request.skill==2354||request.skill==2355||request.skill==2356||request.skill==2353||request.skill==2358||request.skill==2359)&&hooks.norn_rank?hooks.norn_rank():std::optional<unsigned>{};
        if(request.skill==2354&&(!norn_rank||!effects::dodge_this(*norn_rank)))return reject(Reason::unsupported);
        if(request.skill==2355&&(!norn_rank||!effects::i_am_the_strongest(*norn_rank)))return reject(Reason::unsupported);
        if(request.skill==2356&&(!norn_rank||!effects::i_am_unstoppable(*norn_rank)))return reject(Reason::unsupported);
        const auto finish_him=request.skill==2353&&norn_rank?effects::finish_him(*norn_rank):std::nullopt;
        if(request.skill==2353&&!finish_him)return reject(Reason::unsupported);
        const auto dwarf=request.skill==2358&&norn_rank?effects::you_move_like_a_dwarf(*norn_rank):std::nullopt;
        if(request.skill==2358&&!dwarf)return reject(Reason::unsupported);
        const auto weaklings=request.skill==2359&&norn_rank?effects::you_are_all_weaklings(*norn_rank):std::nullopt;
        if(request.skill==2359&&!weaklings)return reject(Reason::unsupported);
        const auto condition=request.skill==2229?effects::signet_infection_condition(*ebon_rank):effects::condition_spell(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        const auto virulence=request.skill==107?effects::virulence(effective[5]):std::nullopt;
        const auto rotting_flesh=request.skill==106?effects::rotting_flesh(effective[5]):std::nullopt;
        const auto tainted_flesh=request.skill==113?effects::tainted_flesh(effective[5]):std::nullopt;
        const auto spiteful=request.skill==121?effects::spiteful_spirit(effective[7]):std::nullopt;
        if(request.skill==121&&(!spiteful||!hooks.spiteful_spirit||!hooks.completed_action||!hooks.before_spell_effect||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto energy_enchantment=(request.skill==119||request.skill==157)?effects::energy_enchantment(request.skill,effective[4]):std::nullopt;
        const auto steal=effects::life_steal(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        const auto storm=(request.skill==197||request.skill==1094||request.skill==215||request.skill==167||request.skill==187||request.skill==192)?effects::fire_storm(skill->attribute<51?effective[skill->attribute]:0u,request.skill):std::nullopt;
        const auto ray=request.skill==830?effects::ray_of_judgment(effective[14]):std::nullopt;
        const auto elemental=effects::elemental(request.skill,skill->attribute<51?effective[skill->attribute]:0u,primary==6?effective[12]:0u);
        const auto slippery=request.skill==2191?effects::slippery_ground(effective[11]):std::nullopt;
        if(request.skill==2191&&(!slippery||!hooks.slippery_ground||!hooks.target_scope||
           !hooks.projectile_target_valid||!hooks.before_spell_effect))return reject(Reason::unsupported);
        const auto burn=effects::mind_burn(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        const auto daggers=request.skill==172?effects::stone_daggers(effective[9]):std::nullopt;
        const auto dancing=request.skill==858?effects::dancing_daggers(effective[30]):std::nullopt;
        const auto iron=request.skill==786?effects::iron_palm(effective[30]):std::nullopt;
        const auto disrupting=request.skill==571?effects::disrupting_dagger(effective[30]):std::nullopt;
        if(request.skill==571&&(!disrupting||!hooks.disrupting_dagger||!hooks.target_scope||!hooks.projectile_target_valid))return reject(Reason::unsupported);
        if(daggers&&(!hooks.stone_daggers||!hooks.target_scope||!hooks.projectile_target_valid))return reject(Reason::unsupported);
        if(request.skill==858&&(!dancing||!hooks.dancing_daggers||!hooks.target_scope||!hooks.projectile_target_valid))return reject(Reason::unsupported);
        if(request.skill==786&&(!iron||!hooks.iron_palm||!hooks.target_scope||!hooks.projectile_target_valid))return reject(Reason::unsupported);
        if(burn&&(!hooks.mind_burn||!hooks.target_scope||!hooks.projectile_target_valid))return reject(Reason::unsupported);
        const bool weapon_spell_skill=request.skill==787||request.skill==792||request.skill==2868||request.skill==793||request.skill==794||request.skill==2893;
        const auto weapon_spell=weapon_spell_skill?effects::weapon_spell(request.skill==792&&pvp?2868:request.skill==793&&pvp?2893:request.skill,skill->attribute<51?effective[skill->attribute]:0u,primary,effective[36]):std::nullopt;
        if(weapon_spell_skill&&(!weapon_spell||!hooks.weapon_spell||!hooks.weapon_spell_target_valid||!hooks.target_scope||!hooks.before_spell_effect))return reject(Reason::unsupported);
        const auto cleansing=effects::ally_cleansing(request.skill,skill->attribute<51?effective[skill->attribute]:0u,primary==3?effective[16]:0u);
        if(cleansing&&((!hooks.ally_cleansing&&!hooks.ally_cleansing_outcome&&!(cleansing->draw&&hooks.ally_cleansing_completed))||(request.skill==2057&&!hooks.ally_cleansing_outcome&&!hooks.ally_cleansing_completed)||!hooks.target_scope||(cleansing->draw&&hooks.ally_cleansing_completed&&!hooks.before_spell_effect)))return reject(Reason::unsupported);
        const auto barbs=request.skill==100?effects::soul_barbs(effective[7]):std::nullopt;
        if(barbs&&(!hooks.soul_barbs||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto backfire=effects::backfire(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        if(backfire&&(!hooks.backfire||!hooks.before_spell_effect||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        //262uses holy damage only for its owned attack reaction,not an immediate foe cast.
        const auto mend=request.skill==290?effects::mending(effective[13]):std::nullopt;
        const auto succor=request.skill==308?effects::succor():std::nullopt;
        const auto retribution=request.skill==248?effects::retribution(effective[14]):std::nullopt;
        if(request.skill==248&&(!retribution||!hooks.retribution_completed||!hooks.retribution_target_valid||!hooks.before_spell_effect||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==308&&(!succor||!hooks.succor||!hooks.succor_target_valid||
           !hooks.before_spell_effect||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==925&&(!recall_cast||!hooks.recall||!hooks.recall_target_valid||!hooks.before_spell_effect||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==65&&(!mimicry_cast||!hooks.mimicry||!hooks.mimicry_target_valid||!hooks.before_spell_effect||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==290&&(!mend||!hooks.mending||!hooks.mending_target_valid||!hooks.before_spell_effect||!hooks.target_scope))return reject(Reason::unsupported);
        const auto blessed=request.skill==297?effects::blessed_signet(effective[16]):std::nullopt;
        if(request.skill==303&&!effects::convert_hexes(effective[15]))return reject(Reason::unsupported);
        if(request.skill==364&&!effects::charge(effective[21]))return reject(Reason::unsupported);
        if((request.skill==348||request.skill==2858)&&!effects::watch_yourself(effective[21],pvp))return reject(Reason::unsupported);
        if(request.skill==367&&!effects::shields_up(effective[21]))return reject(Reason::unsupported);
        if((request.skill==1954||request.skill==2097)&&!effects::save_yourselves(request.skill,*allegiance_rank))return reject(Reason::unsupported);
        if((request.skill==1950||request.skill==2093)&&!effects::signet_of_corruption(request.skill,*allegiance_rank))return reject(Reason::unsupported);
        if((request.skill==318||request.skill==3204)&&!effects::defy_pain(effective[17],pvp))return reject(Reason::unsupported);
        if(request.skill==347&&!effects::endure_pain(effective[17]))return reject(Reason::unsupported);
        if(request.skill==1411&&!effects::stamina_signet(effective[17]))return reject(Reason::unsupported);
        if(request.skill==316&&!effects::to_limit(effective[21]))return reject(Reason::unsupported);
        if(request.skill==297&&(!blessed||!hooks.blessed_signet||!hooks.blessed_signet_ready||!hooks.target_scope||!hooks.target_valid))return reject(Reason::unsupported);
        if(blessed){const auto owner=hooks.target_scope(actor_);if(!owner||!hooks.blessed_signet_ready(*owner,now))return reject(Reason::target);}
        const auto signet=(request.skill==294||request.skill==2887)?effects::signet_judgment(effective[14],pvp):std::nullopt;
        if((request.skill==294||request.skill==2887)&&(!signet||(request.skill==2887&&!pvp)))return reject(Reason::unsupported);
        if(signet&&(!hooks.signet_judgment||!hooks.signet_judgment_ready||!hooks.signet_judgment_ready(now)||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto shadows=request.skill==876?effects::signet_shadows(effective[30]):std::nullopt;
        if(request.skill==876&&(!shadows||!hooks.signet_shadows||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto discord=(request.skill==817||request.skill==2863)?effects::discord(effective[5],pvp):std::nullopt;
        if((request.skill==817||request.skill==2863)&&(!discord||!hooks.discord||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto rift=request.skill==910?effects::spirit_rift(effective[34]):std::nullopt;
        if(request.skill==910&&(!rift||!hooks.spirit_rift||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto spear=request.skill==1130?effects::spear_light(effective[14]):std::nullopt;
        if(request.skill==1130&&(!spear||!hooks.spear_light||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto holy=request.skill==262?std::nullopt:effects::holy_damage(signet?signet->skill:request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        if(holy&&!signet&&(!hooks.holy_damage||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto shadow=effects::shadow_damage(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        const auto death_charge=request.skill==952?effects::death_charge(effective[31]):std::nullopt;
        if(request.skill==952&&(!death_charge||!hooks.death_charge_target_valid||!hooks.death_charge||!hooks.target_scope))return reject(Reason::unsupported);
        const auto ash_blast=request.skill==1085?effects::ash_blast(effective[9]):std::nullopt;
        if(request.skill==1085&&(!ash_blast||!hooks.ash_blast||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto spiritual_pain=request.skill==1336?effects::spiritual_pain(effective[2]):std::nullopt;
        if(request.skill==1336&&(!spiritual_pain||!hooks.spiritual_pain||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const bool gale=request.skill==162;
        if(gale&&(!hooks.gale||!hooks.projectile_target_valid||!hooks.target_scope||(effective[8]<=4&&!hooks.skill_roll)))return reject(Reason::unsupported);
        const auto feast=request.skill==151?effects::feast_of_corruption(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        if(feast&&(!hooks.feast_of_corruption||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto glimmering=request.skill==227?effects::glimmering_mark(effective[8]):std::nullopt;
        const auto wound=effects::wound_spell(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        const bool triple_shot=request.skill==1953||request.skill==2096;
        const auto melee=combat::weapon::attack_skill(pvp&&request.skill==1191?2864:pvp&&request.skill==1720?3147:request.skill,triple_shot?*allegiance_rank:skill->attribute<51?effective[skill->attribute]:0u,effective[17],request.copy,dagger_staged);
        if((request.skill==402||request.skill==395||request.skill==2144)&&(!hooks.completed_bow_attack||!hooks.prepare_skill_use||!hooks.cooldown_completion_source))return reject(Reason::unsupported);
        if(request.skill==321&&!hooks.begin_action)return reject(Reason::unsupported);
        if(request.skill==359&&(!hooks.prepare_melee_attack||!hooks.prepared_melee_attack))return reject(Reason::unsupported);
        if(melee&&(melee->skill==330||melee->skill==396||melee->skill==394||melee->skill==1953||melee->skill==2096)&&!hooks.attack_skill_target)return reject(Reason::unsupported);
        if(melee&&now>UINT64_MAX-melee->windup_ms)return reject(Reason::policy);
        const auto weapon=melee&&hooks.weapon_snapshot?hooks.weapon_snapshot():std::nullopt;
        if(melee&&(!hooks.melee_target||(!dagger_staged&&!hooks.melee_attack)||!hooks.attack_activity||!hooks.target_scope))return reject(Reason::unsupported);
        if((request.skill==321||request.skill==329)&&weapon&&weapon->item_type!=2&&weapon->item_type!=15&&weapon->item_type!=27)return reject(Reason::weapon);
        if(melee&&(!weapon||weapon->caster_weapon||(weapon->item_type==5&&melee->required_type!=5)||(melee->required_type&&weapon->item_type!=melee->required_type)))return reject(Reason::weapon);
        if(melee&&melee->requires_critical_authority&&(!hooks.critical_attack_ready||!hooks.critical_attack_ready(request.target,*weapon,now)))return reject(Reason::unsupported);
        const auto attack_timing=melee&&hooks.weapon_attack_timing?hooks.weapon_attack_timing(now):std::nullopt;
        if(melee&&(!attack_timing||!attack_timing->period_ms||attack_timing->period_ms>60000||attack_timing->windup_ms>attack_timing->period_ms))return reject(Reason::unsupported);
        const auto water=(request.skill==214||request.skill==237)?effects::water_projectile(request.skill,skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        if(water&&(!hooks.water_projectile||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto movement=effects::movement_hex(request.skill,skill->attribute<51?effective[skill->attribute]:0u,pvp);
        if(movement&&(!hooks.movement_hex||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto hex=effects::degeneration_hex(request.skill,skill->attribute<51?effective[skill->attribute]:0u,pvp);
        const auto energy_hex=effects::energy_degeneration_hex(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        const auto action_hex=effects::action_damage_hex(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        const auto spirit=request.skill==245?effects::protective_spirit(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto aegis=request.skill==257?effects::aegis(effective[15]):std::nullopt;
        if(request.skill==257&&(pvp||!aegis||!hooks.aegis||!hooks.before_spell_effect||!hooks.target_scope))return reject(Reason::unsupported);
        const auto vigorous=request.skill==254?effects::vigorous_spirit(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto seed=request.skill==274?effects::healing_seed(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto reversal=request.skill==307?effects::reversal_fortune(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto deflection=request.skill==259?effects::shield_deflection(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto regeneration_shield=request.skill==261?effects::shield_regeneration(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto judgment=request.skill==262?effects::shield_judgment(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto watchful=request.skill==1392?effects::watchful_healing(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto restful=request.skill==886?regeneration::reviewed(886,effective[13]):std::nullopt;
        const auto chilblains=request.skill==144?effects::chilblains(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto subversion=request.skill==127?effects::mark_subversion(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto leech=request.skill==128?effects::soul_leech(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        const auto prism=request.skill==903?effects::icy_prism(skill->attribute<51?effective[skill->attribute]:0u):std::nullopt;
        if((request.skill==36||request.skill==53||request.skill==3183)&&!hooks.activation_multiplier)return reject(Reason::unsupported);
        const auto condition_damage=effects::condition_damage(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        if(condition_damage&&(!hooks.condition_damage||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(hex&&((request.skill==883?(!hooks.degeneration_hex_completed||!hooks.before_spell_effect):!hooks.degeneration_hex)||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(energy_hex&&(!hooks.energy_degeneration_hex||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(action_hex&&(!hooks.action_damage_hex||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==254&&(!vigorous||!hooks.vigorous_spirit||!hooks.before_spell_effect||!hooks.target_scope||!hooks.ally_enchantment_target_valid))return reject(Reason::unsupported);
        if(request.skill==307&&(!reversal||!hooks.reversal_fortune_completed||!hooks.before_spell_effect||!hooks.target_scope||!hooks.reversal_target_valid||!hooks.enchantment_duration_ready))return reject(Reason::unsupported);
        if(request.skill==274&&(!seed||!hooks.healing_seed||!hooks.before_spell_effect||!hooks.target_scope||!hooks.healing_seed_target_valid))return reject(Reason::unsupported);
        if(request.skill==259&&(!deflection||!hooks.shield_deflection||!hooks.before_spell_effect||!hooks.target_scope||!hooks.ally_enchantment_target_valid))return reject(Reason::unsupported);
        if(request.skill==261&&(!regeneration_shield||!hooks.shield_regeneration||!hooks.before_spell_effect||!hooks.target_scope||!hooks.ally_enchantment_target_valid))return reject(Reason::unsupported);
        if(request.skill==262&&(!judgment||!hooks.shield_judgment||!hooks.shield_judgment_ready||!hooks.shield_judgment_ready(now)||!hooks.before_spell_effect||!hooks.target_scope||!hooks.ally_enchantment_target_valid))return reject(Reason::unsupported);
        if(request.skill==245&&(!spirit||!hooks.protective_spirit||!hooks.before_spell_effect||!hooks.target_scope||!hooks.ally_enchantment_target_valid))return reject(Reason::unsupported);
        if(request.skill==1392&&(!watchful||!hooks.watchful_healing||!hooks.before_spell_effect||!hooks.target_scope||!hooks.ally_enchantment_target_valid))return reject(Reason::unsupported);
        if(request.skill==886&&(!regeneration::reviewed(886,effective[13])||!hooks.restful_breeze||!hooks.before_spell_effect||!hooks.target_scope||!hooks.ally_enchantment_target_valid))return reject(Reason::unsupported);
        if(request.skill==144&&(!chilblains||!hooks.chilblains||!hooks.before_spell_effect||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==127&&(!subversion||!hooks.mark_subversion||!hooks.before_spell_effect||!hooks.targeted_spell_failure||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==128&&(!leech||!hooks.soul_leech||!hooks.before_spell_effect||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(request.skill==903&&(!prism||!hooks.icy_prism||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(wound&&((wound_spell_completion?!hooks.wound_spell_completed:!hooks.wound_spell)||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(shadow&&(!hooks.shadow_damage||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto empathy=effects::empathy(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        if(empathy&&(!hooks.empathy||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        const auto drain=effects::energy_drain(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        const auto gain=effects::resource_gain(request.skill,skill->attribute<51?effective[skill->attribute]:0u);
        if(gain&&(!hooks.resource_gain||!hooks.target_scope||(effects::gain_targets_foe(*gain)&&!((request.skill==365||request.skill==1141)?hooks.shout_target_valid:hooks.projectile_target_valid))))return reject(Reason::unsupported);
        if(drain&&((!hooks.energy_drain&&!(effects::is_spell(*skill)&&hooks.energy_drain_completed))||!hooks.projectile_target_valid||!hooks.target_scope||(effects::is_spell(*skill)&&hooks.energy_drain_completed&&!hooks.before_spell_effect)))return reject(Reason::unsupported);
        if(storm&&(!hooks.complete_fire_storm||!hooks.target_scope||!hooks.projectile_target_valid))return reject(Reason::unsupported);
        if(request.skill==830&&(!ray||!hooks.complete_ray_of_judgment||!hooks.target_scope||!hooks.projectile_target_valid))return reject(Reason::unsupported);
        const bool elemental_completion=elemental&&effects::is_spell(*skill)&&bool(hooks.elemental_completed);
        if(elemental&&((elemental_completion?!hooks.before_spell_effect:(request.skill==1068?!hooks.conditional_elemental:!hooks.elemental))||!hooks.target_scope||(!elemental->caster_center&&!hooks.projectile_target_valid)))return reject(Reason::unsupported);
        if(request.skill==110&&!hooks.area_life_steal)return reject(Reason::unsupported);
        if(steal&&(!hooks.projectile_target_valid||!hooks.target_scope||
            (!life_steal_completion&&((request.skill==1076||request.skill==902||request.skill==2189)||!hooks.life_steal))))return reject(Reason::unsupported);
        if(tainted_flesh&&((!simple_enchantment_completion&&!hooks.tainted_flesh)||!hooks.tainted_flesh_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(energy_enchantment&&((!energy_enchantment_completion&&!hooks.energy_enchantment)||!hooks.energy_enchantment_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(virulence&&(!hooks.virulence||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(rotting_flesh&&(!hooks.rotting_flesh||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(condition&&((!hooks.condition_spell&&!hooks.condition_spell_completed)||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(condition&&hooks.condition_spell_completed&&effects::is_spell(*skill)&&!hooks.before_spell_effect)return reject(Reason::unsupported);
        if(transfer&&(!hooks.condition_transfer||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(transfer&&effects::transfer_roll_required(*transfer)&&!hooks.skill_roll)return reject(Reason::unsupported);
        if(faint&&(!hooks.faintheartedness||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(swarm&&(!hooks.deathly_swarm||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(javelin&&(!hooks.lightning_javelin||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(interrupt&&(!hooks.interrupt_skill||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(lightning&&(!hooks.lightning||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(blindness&&(!hooks.blindness||!hooks.projectile_target_valid||!hooks.target_scope))return reject(Reason::unsupported);
        if(effects::party_resurrection(request.skill)&&!hooks.resurrect_party&&!hooks.resurrection_completed)return reject(Reason::unsupported);
        if(resurrection&&(!hooks.resurrection_target_valid||!hooks.target_scope||
            ((request.skill==791?!hooks.flesh_my_flesh:!hooks.resurrect)&&!resurrection_completion)))return reject(Reason::unsupported);
        if(resurrection_completion&&(!hooks.before_spell_effect||!hooks.target_scope))return reject(Reason::unsupported);
        if(projectile&&(!hooks.projectile_target_valid||!hooks.launch_projectile))return reject(Reason::unsupported);
        if((pending_||dagger_failed_second_draft_||dagger_successful_second_draft_||pending_contact_amendment_draft_||now<aftercast_until_)&&!timing::instant_action(*skill))return reject(Reason::busy);
        if(request.skill==1209)request.target=actor_; // Untargeted stance.
        if(request.skill==2138||request.skill==823||request.skill==1261||request.skill==1375)request.target=actor_; // Untargeted self-enchantment.
        const auto key=std::pair{request.skill,request.copy};if(spent_signets_.contains(key)||(recharging_.contains(key)&&now<recharging_.at(key))||
            (!hooks.skill_disabled_until&&!hooks.skill_disabled_slot&&external_disables_.contains(key)&&now<external_disables_.at(key)))return reject(Reason::recharge);
        if(effects::party_heal(request.skill,0,0)||request.skill==2216||request.skill==367||request.skill==475||request.skill==1952||request.skill==2095||request.skill==1954||request.skill==2097)request.target=actor_;
        if(pet_comfort_cast)request.target=actor_;
        if(!request.target||traversal||well_cast||blessed||(elemental&&elemental->caster_center)||echo_cast||request.skill==33||bool(demonic_cast)||bool(glyph_cast)||request.skill==374||request.skill==362||request.skill==361||request.skill==316||request.skill==318||request.skill==3204||request.skill==347||request.skill==1411||request.skill==348||request.skill==2858||request.skill==1954||request.skill==2097||request.skill==364||request.skill==343||request.skill==2883||request.skill==368||request.skill==2354||request.skill==2355||request.skill==2356||request.skill==389||request.skill==63||request.skill==427||request.skill==182||request.skill==207||request.skill==221||request.skill==298||request.skill==943||request.skill==1262||request.skill==82||request.skill==257||(request.skill==165||request.skill==1381)||request.skill==10||request.skill==110||effects::elemental_mantra(request.skill)||timing::stance(request.skill)||request.skill==881||request.skill==938||request.skill==175||request.skill==176||request.skill==177||request.skill==195||request.skill==247||request.skill==1113||request.skill==330||(request.skill==431||request.skill==432||bool(combat::weapon::preparation_contact_definition(request.skill,0)))||request.skill==814||request.skill==115||request.skill==146||request.skill==1||request.skill==363||request.skill==346||request.skill==38||request.skill==37||request.skill==3373||(request.skill==180||request.skill==3375)||request.skill==200||request.skill==183||request.skill==163||request.skill==212||request.skill==304||bool(effects::attunement(request.skill))||request.skill==1043||request.skill==174||request.skill==937||request.skill==217||request.skill==446||request.skill==287||request.skill==3232||request.skill==1397||request.skill==279||request.skill==1117||request.skill==280||request.skill==1119)request.target=actor_;
        const auto healing=effects::direct_heal(request.skill);
        const bool direct_heal_completion=healing&&effects::is_spell(*skill)&&bool(hooks.direct_heal_completed);
        if(healing&&(direct_heal_completion?(!hooks.direct_heal_target_valid||!hooks.target_scope||!hooks.before_spell_effect):
                ((!hooks.heal&&!hooks.heal_scoped)||(hooks.heal_scoped&&!hooks.target_scope)||
                 (request.skill==1687&&(!hooks.zealous_benediction||!hooks.zealous_target_valid)))))return reject(Reason::unsupported);
        const auto condition_plan=effects::condition_removal(request.skill,skill->attribute<51?effective[skill->attribute]:0u,primary==3?effective[16]:0u);
        const float range=tainted_flesh?tainted_flesh->range:energy_enchantment?energy_enchantment->range:healing?healing->range:condition_plan?condition_plan->range:1248.f;const bool allow_self=(finish_him||dwarf||weaklings)?false:tainted_flesh?true:energy_enchantment?false:(request.skill==283||request.skill==292||request.skill==1526)?false:cleansing?cleansing->self:condition_plan?condition_plan->self:!healing||healing->self;
        const bool foe_plan=bool(snow||glimmering||slippery||finish_him||dwarf||weaklings||iron||disrupting||ear_bite||energy_blast||aneurysm||necrosis||sorrow||request.skill==1950||request.skill==2093||request.skill==934||request.skill==3188||daggers||dancing||burn||water||fevered||wandering||mind||balance||diversion||cry||panic||humility||feast||interrupt||barbs||backfire||spiteful||holy||spear||rift||discord||shadows||delusions||disenchant||movement||(shatter&&!shatter->ally)||faint||swarm||javelin||storm||ray||condition_damage||hex||energy_hex||action_hex||prism||leech||subversion||chilblains||wound||gale||death_charge||ash_blast||spiritual_pain||shadow||lightning||blindness||transfer||condition||virulence||rotting_flesh||steal||drain||empathy||(gain&&effects::gain_targets_foe(*gain))||(elemental&&!elemental->caster_center));const float foe_range=slippery?slippery->range:finish_him?finish_him->range:(dwarf||weaklings)?1248.f:iron?iron->range:disrupting?disrupting->range:ear_bite?ear_bite->range:energy_blast?energy_blast->range:dancing?dancing->range:aneurysm?aneurysm->range:necrosis?necrosis->range:sorrow?sorrow->range:(request.skill==1950||request.skill==2093||request.skill==934||request.skill==3188||daggers||dancing||burn||water||fevered||wandering||mind||balance||diversion||cry||panic)?1248.f:humility?1248.f:feast?feast->range:interrupt?interrupt->range:barbs?barbs->range:backfire?backfire->range:spiteful?spiteful->range:(spear||rift||discord||shadows)?1248.f:holy?holy->range:delusions?delusions->range:disenchant?disenchant->range:movement?movement->range:shatter?shatter->range:faint?faint->range:swarm?swarm->range:javelin?javelin->range:condition_damage?condition_damage->range:hex?hex->range:energy_hex?energy_hex->range:action_hex?action_hex->range:prism?prism->range:leech?leech->range:subversion?subversion->range:chilblains?chilblains->range:snow?snow->range:glimmering?glimmering->range:wound?wound->range:gale?1248.f:death_charge?death_charge->range:ash_blast?ash_blast->range:spiritual_pain?spiritual_pain->range:shadow?shadow->range:lightning?lightning->range:blindness?blindness->range:transfer?transfer->range:condition?condition->range:rotting_flesh?rotting_flesh->range:virulence?virulence->range:steal?steal->range:drain?drain->range:gain?gain->range:empathy?1248:elemental?elemental->range:storm?1248:ray?ray->range:0;
        const auto weapon_spell_scope=weapon_spell?hooks.target_scope(request.target):std::nullopt;
        const auto death_charge_scope=death_charge?hooks.target_scope(request.target):std::nullopt;
        if(death_charge&&!death_charge_scope)return reject(Reason::target);
        if(weapon_spell&&(!weapon_spell_scope||!hooks.weapon_spell_target_valid(*weapon_spell_scope,*weapon_spell,now)))return reject(Reason::target);
        if(seed&&request.target==actor_)return reject(Reason::target);
        const auto watchful_scope=(request.skill==303||mimicry_cast||recall_cast||mend||succor||retribution||watchful||restful||spirit||vigorous||seed||reversal||deflection||regeneration_shield||judgment)?hooks.target_scope(request.target):std::optional<effects::Scope>{};
        if(request.skill==303&&(!watchful_scope||!use_owner||watchful_scope->actor==actor_||watchful_scope->instance!=use_owner->instance))return reject(Reason::target);
        if(!pet_charm_cast&&!pet_comfort_cast&&!weapon_spell&&!traversal&&((hooks.status_heal_completed&&(request.skill==283||request.skill==1396))?!(hooks.target_scope(request.target)&&hooks.status_heal_target_valid(*hooks.target_scope(request.target),request.skill,now)):direct_heal_completion?!(hooks.target_scope(request.target)&&hooks.direct_heal_target_valid(*hooks.target_scope(request.target),request.skill,now)):(request.skill==303||mimicry_cast||recall_cast||mend||succor||retribution||watchful||restful||spirit||vigorous||seed||reversal||deflection||regeneration_shield||judgment)?(!watchful_scope||(reversal?!hooks.reversal_target_valid(*watchful_scope,primary==3?effective[16]:0,now):seed?!hooks.healing_seed_target_valid(*watchful_scope,primary==3?effective[16]:0u,now):mimicry_cast?!hooks.mimicry_target_valid({request.skill,request.copy},*watchful_scope,now):recall_cast?!hooks.recall_target_valid(*watchful_scope,now):retribution?!hooks.retribution_target_valid(*watchful_scope,primary==3?effective[16]:0u,now):mend?!hooks.mending_target_valid(*watchful_scope,primary==3?effective[16]:0u,now):succor?!hooks.succor_target_valid(*watchful_scope,primary==3?effective[16]:0u,now):!hooks.ally_enchantment_target_valid(*watchful_scope,primary==3?effective[16]:0u,now))):tainted_flesh?!hooks.tainted_flesh_target_valid(request.target,*tainted_flesh,now):energy_enchantment?!hooks.energy_enchantment_target_valid(request.target,*energy_enchantment,now):request.skill==212?!(hooks.target_valid&&hooks.target_valid(actor_,1248,true)):inspired?(inspired->allied?!(hooks.target_valid&&hooks.target_valid(request.target,inspired->range,true)):!hooks.projectile_target_valid(request.target,inspired->range)):shatter&&shatter->ally?!hooks.shatter_ally_valid(request.target,shatter->range,now):(request.skill==292||request.skill==1526)?!hooks.infuse_target_valid(request.target,now):request.skill==1687?!hooks.zealous_target_valid(request.target,range,now):request.skill==1121?!(hooks.target_scope(request.target)&&hooks.gift_health_target_valid(*hooks.target_scope(request.target),now)):request.skill==1234?!(hooks.target_scope(request.target)&&hooks.mend_body_soul_target_valid(*hooks.target_scope(request.target),now)):request.skill==1741?!(hooks.target_scope(request.target)&&hooks.ghostmirror_light_target_valid(*hooks.target_scope(request.target),now)):request.skill==2062?!(hooks.target_scope(request.target)&&hooks.healing_ribbon_target_valid(*hooks.target_scope(request.target),now)):request.skill==887?!(hooks.target_scope(request.target)&&hooks.rejuvenation_target_valid(*hooks.target_scope(request.target),now)):request.skill==915?!(hooks.target_scope(request.target)&&hooks.spirit_light_target_valid(*hooks.target_scope(request.target),now)):melee?!(hooks.attack_skill_target?hooks.attack_skill_target(request.target,request.skill,now):hooks.melee_target(request.target,now)):resurrection?!hooks.resurrection_target_valid(request.target,resurrection->range):foe_plan?!(death_charge?hooks.death_charge_target_valid(*death_charge_scope,*death_charge,now):((request.skill==365||request.skill==1141)?hooks.shout_target_valid(request.target,foe_range):hooks.projectile_target_valid(request.target,foe_range))):projectile?!hooks.projectile_target_valid(request.target,range):!(hooks.target_valid&&hooks.target_valid(request.target,range,allow_self))))return reject(Reason::target);
        std::optional<effects::Scope> pet_target;
        if(pet_charm_cast){pet_target=hooks.target_scope(request.target);if(!pet_target)return reject(Reason::target);}
        if(pet_comfort_cast){pet_target=hooks.pet_owned_scope(*use_owner);
            if(!pet_target||!hooks.pet_comfort_ready(*use_owner,*pet_target,now))return reject(Reason::target);}
        const auto corpse_scope=weapon_spell?weapon_spell_scope:(request.skill==303||mimicry_cast||recall_cast||mend||succor||retribution||watchful||restful||spirit||vigorous||seed||reversal||deflection||regeneration_shield||judgment)?watchful_scope:((hooks.remove_effects_completed&&(request.skill==301||request.skill==2003))||((request.skill==2221||hooks.party_heal_completed)&&effects::party_heal(request.skill,0,0))||(hooks.remove_conditions_completed&&condition_plan)||(hooks.status_heal_completed&&(request.skill==283||request.skill==1396))||blessed||inspired||request.skill==427||request.skill==182||request.skill==207||request.skill==221||request.skill==298||request.skill==943||request.skill==1262||request.skill==82||request.skill==257||(request.skill==165||request.skill==1381)||request.skill==10||request.skill==292||request.skill==1526||request.skill==1121||request.skill==915||request.skill==1234||request.skill==1741||request.skill==2062||request.skill==887||melee||(healing&&(hooks.heal_scoped||direct_heal_completion))||request.skill==258||request.skill==299||request.skill==2061||request.skill==1392||(request.skill==288&&hooks.protective_enchantment)||request.skill==1691||energy_enchantment||tainted_flesh||cleansing||resurrection||foe_plan||elemental||gain||shatter)?hooks.target_scope(request.target):std::nullopt;
        if((blessed||inspired||request.skill==427||request.skill==182||request.skill==207||request.skill==221||request.skill==298||request.skill==943||request.skill==1262||request.skill==82||request.skill==257||(request.skill==165||request.skill==1381)||request.skill==10||request.skill==292||request.skill==1526||request.skill==1121||request.skill==915||request.skill==1234||request.skill==1741||request.skill==2062||request.skill==887||melee||(healing&&(hooks.heal_scoped||direct_heal_completion))||request.skill==258||request.skill==299||request.skill==2061||request.skill==1392||(request.skill==288&&hooks.protective_enchantment)||request.skill==1691||energy_enchantment||tainted_flesh||cleansing||resurrection||foe_plan||elemental||gain||shatter)&&!corpse_scope)return reject(Reason::target);
        // A selected ally or a generic retained target scope is not sufficient:
        // only these admitted effect plans actually target an individual ally.
        const bool completion_scope_required=
            (drain&&effects::is_spell(*skill)&&hooks.energy_drain_completed)||
            (cleansing&&cleansing->draw&&hooks.ally_cleansing_completed)||
            (hooks.remove_effects_completed&&(request.skill==301||request.skill==2003))||
            (hooks.remove_conditions_completed&&condition_plan)||
            (hooks.status_heal_completed&&(request.skill==283||request.skill==1396))||
            ((request.skill==2221||hooks.party_heal_completed)&&effects::party_heal(request.skill,0,0));
        if(completion_scope_required&&!corpse_scope)return reject(Reason::target);
        const bool selfless_ally_plan=healing||condition_plan||request.skill==283||request.skill==1396||
            request.skill==292||request.skill==1526||request.skill==1121||request.skill==915||
            request.skill==1234||request.skill==1741||request.skill==2062||request.skill==887||
            weapon_spell||request.skill==303||mimicry_cast||recall_cast||mend||succor||retribution||watchful||restful||
            spirit||vigorous||seed||reversal||deflection||regeneration_shield||judgment||tainted_flesh||
            energy_enchantment||cleansing||(inspired&&inspired->allied)||(shatter&&shatter->ally)||
            resurrection||request.skill==301||request.skill==2003||request.skill==288||
            request.skill==258||request.skill==299||request.skill==2061;
        const bool selfless_targeted=effects::is_spell(*skill)&&selfless_ally_plan&&request.target!=actor_;
        const bool selfless_payment=bool(hooks.consume_selfless_energy||hooks.consume_selfless_overcast_energy||hooks.pay_selfless_elemental_glyph);
        const auto selfless_target=selfless_payment&&selfless_targeted&&hooks.target_scope?hooks.target_scope(request.target):std::nullopt;
        if(selfless_payment&&selfless_targeted&&(!selfless_target||selfless_target->actor!=request.target))return reject(Reason::target);
        const auto use_target=(observe_glimmering||hooks.prepare_demonic_flesh_use||hooks.mind_wrack_targeted||hooks.keystone_skill_used)?hooks.target_scope(request.target):std::nullopt;
        if((observe_glimmering||hooks.prepare_demonic_flesh_use||hooks.mind_wrack_targeted||hooks.keystone_skill_used)&&!use_target)return reject(Reason::target);
        // Optional observation must not broaden skill admission or attach a
        // mandatory target-lifetime guard to self/untargeted/legacy effects.
        const auto pet_offensive_owner=hooks.pet_offensive_completed&&hooks.target_scope&&request.target!=actor_
            ?hooks.target_scope(actor_):std::nullopt;
        const auto pet_offensive_target=pet_offensive_owner?hooks.target_scope(request.target):std::nullopt;
        std::optional<effects::FeastCastAnchor> feast_anchor;
        if(feast){const auto owner=hooks.target_scope(actor_);
            if(!owner||!effects::valid_feast_anchor_scope(*owner)||owner->actor!=actor_||!corpse_scope||!effects::valid_feast_anchor_scope(*corpse_scope)||corpse_scope->actor!=request.target||owner->instance!=corpse_scope->instance||owner->actor==corpse_scope->actor)return reject(Reason::target);
            feast_anchor=effects::FeastCastAnchor{*owner,*corpse_scope,now};
        }
        const auto balance_caster=balance?hooks.target_scope(actor_):std::nullopt;
        if(balance&&(!balance_caster||!effects::valid_feast_anchor_scope(*balance_caster)||balance_caster->actor!=actor_||!corpse_scope||balance_caster->instance!=corpse_scope->instance))return reject(Reason::target);
        const auto caster=hooks.resources(actor_);
        if(!caster||!caster->health)return reject(Reason::energy);
        auto attack_windup=melee?(hooks.attack_duration?hooks.attack_duration(melee->windup_ms,now):std::optional<Milliseconds>{melee->windup_ms}):std::optional<Milliseconds>{0};
        if(!attack_windup||(melee&&!*attack_windup)||*attack_windup>90000||now>UINT64_MAX-*attack_windup)return reject(Reason::unsupported);
        const bool interruptible=melee||skill->activation>0||skill->aftercast>0;
        const auto control_epoch=interruptible&&hooks.control_epoch?hooks.control_epoch():std::nullopt;
        if(interruptible&&hooks.control_epoch&&!control_epoch)return reject(Reason::policy);
        std::unique_ptr<combat::weapon::AttackLease> attack_lease;
        if(request.skill==359){
            const auto permit=hooks.prepare_melee_attack(*corpse_scope,*weapon,*melee,now);
            if(!permit)return reject(Reason::target);
            attack_lease=std::make_unique<combat::weapon::AttackLease>(permit);
            if(!permit->open()||permit->release_time()<=now||permit->release_time()-now>90000)return reject(Reason::target);
            *attack_windup=permit->release_time()-now;
        }
        if(dagger_staged){
            const auto permit=hooks.prepare_dagger_attack(*corpse_scope,*weapon,*melee,now,now+*attack_windup);
            if(!permit||!permit->open()||permit->release_time()!=now+*attack_windup)return reject(Reason::target);
            attack_lease=std::make_unique<combat::weapon::AttackLease>(permit);
        }
        // Lesser Energy's legacy path is separate from typed owned glyph leases.
        const auto typed_glyph=hooks.elemental_glyph_snapshot?hooks.elemental_glyph_snapshot(now):std::nullopt;
        if(typed_glyph&&effects::is_spell(*skill)&&((typed_glyph->plan.skill==203&&(!hooks.finish_renewal_spell||!hooks.prepare_skill_use||!hooks.abort_skill_use||!hooks.finalize_skill_use||!hooks.control_epoch||!hooks.cooldown_control_source))||!hooks.pay_elemental_glyph||!hooks.settle_elemental_glyph||!hooks.glyph_clock))return reject(Reason::unsupported);
        const auto glyph=effects_.find(200);
        const bool spell=effects::is_spell(*skill);
        const bool discounted=!typed_glyph&&spell&&glyph_charges_&&glyph!=effects_.end()&&now<glyph->second.ends;
        unsigned base_cost=expertise_energy_cost(*skill,primary,effective[23]);
        const bool zephyr_at_start=hooks.quickening_zephyr_aura&&
            hooks.quickening_zephyr_aura(now).value_or(false);
        const unsigned catalog_cost=skill->energy_cost();
        const auto aura_cost=timing::quickening_zephyr_energy_cost(catalog_cost,zephyr_at_start);
        if(!aura_cost||*aura_cost<catalog_cost||
           base_cost>UINT_MAX-(*aura_cost-catalog_cost))return reject(Reason::policy);
        // Add 30% of the catalog base, not 30% of Expertise's reduced cost.
        base_cost+=*aura_cost-catalog_cost;
        const unsigned reduction=discounted?scaled(10,18,glyph->second.rank):0;
        const auto activation_factor=hooks.activation_multiplier?hooks.activation_multiplier(request.skill,now):std::optional<double>{1.};
        const auto cast_time=activation_factor?timing::activation(*skill,primary,primary==5?effective[0]:0u,*activation_factor):std::nullopt;
        if(!cast_time||now>UINT64_MAX-*cast_time)return reject(Reason::policy);
        const auto cost=base_cost>reduction?base_cost-reduction:0;
        const bool overcast_payment=(skill->special&1)&&skill->overcast;
        // Guards exist before reservation/payment callbacks, including lethal
        // reactive costs and exceptions. Their captured callbacks outlive Hooks.
        std::optional<pets::CharmReservation> pet_reservation;
        pets::CastLease reservation_guard([cancel=hooks.pet_charm_cancel_reservation,&pet_reservation]{
            if(pet_reservation&&cancel)cancel(*pet_reservation);
        });
        std::shared_ptr<pets::CharmAttempt> pet_attempt;
        std::unique_ptr<pets::CastLease> pet_lease;
        std::uint64_t pet_sequence=0;
        if(pet_charm_cast||pet_comfort_cast){pet_sequence=spell_activation_event();if(!pet_sequence)return reject(Reason::policy);}
        if(pet_charm_cast){
            pet_attempt=std::make_shared<pets::CharmAttempt>();
            pet_lease=std::make_unique<pets::CastLease>([cancel=hooks.pet_charm_cancel,pet_attempt]{
                if(pet_attempt->serial)cancel(*pet_attempt);
            });
            pet_reservation=hooks.pet_charm_reserve(*use_owner,*pet_target,now);
            if(!pet_reservation||pet_reservation->owner!=*use_owner||pet_reservation->target!=*pet_target)return reject(Reason::target);
        }
        std::optional<effects::ElementalGlyphQuote> glyph_quote;std::uint64_t glyph_event=0;
        if(typed_glyph&&spell){glyph_event=spell_activation_event();
            if(!glyph_event)return reject(Reason::policy);
            glyph_quote=hooks.pay_selfless_elemental_glyph?hooks.pay_selfless_elemental_glyph({request.skill,request.copy},selfless_target,base_cost,glyph_event,now,*typed_glyph):hooks.pay_elemental_glyph({request.skill,request.copy},base_cost,glyph_event,now,*typed_glyph);
            if(!glyph_quote||glyph_quote->glyph!=*typed_glyph)return reject(Reason::energy);
        }else if(overcast_payment){const auto event=spell_activation_event();
            if(!event||!(hooks.consume_selfless_overcast_energy?hooks.consume_selfless_overcast_energy(request.skill,selfless_target,cost,event,now):hooks.consume_overcast_energy(request.skill,cost,event,now)))return reject(Reason::energy);
        }else if(!(hooks.consume_selfless_energy?hooks.consume_selfless_energy({request.skill,request.copy},selfless_target,cost,now):hooks.consume_energy_at?hooks.consume_energy_at(cost,now):hooks.consume_energy&&hooks.consume_energy(cost)))return reject(Reason::energy);
        // A real payment can trigger lethal reactive damage before this cast
        // exists. Do not admit a pending action or spend adrenaline on a corpse.
        if(const auto paid=hooks.resources(actor_);!paid||!paid->health)return reject(Reason::policy);
        if(discounted&&!--glyph_charges_){
            if(hooks.status_mutation)hooks.status_mutation(200,glyph->second.id,0,now,{},false);
            hooks.events({effect_remove(actor_,glyph->second.id)});effects_.erase(glyph);
        }
        if(skill->adrenaline&&!hooks.consume_adrenaline({request.skill,request.copy},now))return reject(Reason::adrenaline);
        if(pet_charm_cast){
            auto accepted=hooks.pet_charm_accept(*pet_reservation,pet_sequence,now,*cast_time);
            if(!accepted)return reject(Reason::target); // Genuine payment/reactions remain committed.
            *pet_attempt=std::move(*accepted);
            if(pet_attempt->owner!=*use_owner||pet_attempt->target!=*pet_target||pet_attempt->cast_sequence!=pet_sequence||
               pet_attempt->starts!=now||pet_attempt->completes!=now+*cast_time)return reject(Reason::policy);
            reservation_guard.consumed();
        }
        if(pet_comfort_cast&&(!hooks.target_scope||hooks.target_scope(actor_)!=use_owner||
           !hooks.pet_comfort_ready(*use_owner,*pet_target,now)))return reject(Reason::target);
        pending_=Pending{request,effective[13],primary==3?effective[16]:0u,range,allow_self,
            now+(melee?*attack_windup:*cast_time),allegiance_skill?*allegiance_rank:request.skill==2229?*ebon_rank:(request.skill==2354||request.skill==2355||request.skill==2356||request.skill==2353||request.skill==2358||request.skill==2359)?*norn_rank:(request.skill==2222||request.skill==2213||request.skill==2221||request.skill==2216)?*deldrimor_rank:(request.skill==1816||request.skill==2103)?*sunspear_rank:(skill->attribute<51?effective[skill->attribute]:0u),primary==6?effective[12]:0u};
        pending_->pet_charm=std::move(pet_attempt);pending_->pet_lease=std::move(pet_lease);
        pending_->pet_target=pet_target;pending_->pet_sequence=pet_sequence;
        pending_->pet_offensive_owner=pet_offensive_owner;pending_->pet_offensive_target=pet_offensive_target;
        pending_->weapon_spell=weapon_spell;
        pending_->glyph_quote=glyph_quote;pending_->glyph_event=glyph_event;
        pending_->observe_duration=observe_duration;
        pending_->observe_local_duration=observe_local_duration;
        pending_->elemental_completion=elemental_completion;
        pending_->direct_heal_completion=direct_heal_completion;
        pending_->armor_earth_completion=request.skill==165&&bool(hooks.armor_earth_completed);
        pending_->simple_enchantment_completion=simple_enchantment_completion;
        pending_->healing_ring_completion=request.skill==1262&&bool(hooks.healing_ring_completed);
        pending_->resurrection_completion=resurrection_completion;
        pending_->burning_speed_completion=request.skill==823&&bool(hooks.burning_speed_completed);
        pending_->energy_enchantment_completion=energy_enchantment_completion;
        pending_->life_steal_completion=life_steal_completion;
        pending_->life_steal_spell_completion=life_steal_spell_completion;
        pending_->wound_spell_completion=wound_spell_completion;
        pending_->gift_health_completion=request.skill==1121&&bool(hooks.gift_health_completed);
        pending_->spirit_light_completion=request.skill==915&&bool(hooks.spirit_light_completed);
        pending_->mend_body_soul_completion=request.skill==1234&&bool(hooks.mend_body_soul_completed);
        pending_->ghostmirror_light_completion=request.skill==1741&&bool(hooks.ghostmirror_light_completed);
        pending_->healing_ribbon_completion=request.skill==2062&&bool(hooks.healing_ribbon_completed);
        pending_->status_heal_completion=(request.skill==283||request.skill==1396)&&bool(hooks.status_heal_completed);
        pending_->condition_removal_completion=bool(condition_plan)&&bool(hooks.remove_conditions_completed);
        pending_->party_heal_completion=bool(effects::party_heal(request.skill,0,0))&&(request.skill==2221||bool(hooks.party_heal_completed));
        pending_->energy_drain_completion=bool(drain)&&effects::is_spell(*skill)&&bool(hooks.energy_drain_completed);
        pending_->ally_cleansing_completion=cleansing&&cleansing->draw&&bool(hooks.ally_cleansing_completed);
        pending_->effect_removal_completion=(request.skill==301||request.skill==2003)&&bool(hooks.remove_effects_completed);
        pending_->health_completion=request.skill==292?bool(hooks.infuse_health_completed):request.skill==1526&&bool(hooks.imbue_health_completed);
        pending_->session_admission=admission;
        pending_->attack_lease=std::move(attack_lease);pending_->dagger_staged=dagger_staged;
        pending_->began=now;pending_->use_owner=use_owner?use_owner:(admission?std::optional(admission->cast.owner):(glyph_quote?std::optional(glyph_quote->glyph.owner):(hooks.prepare_skill_use&&hooks.target_scope?hooks.target_scope(actor_):std::nullopt)));pending_->observe_panic=bool(hooks.panic_skill_used);pending_->use_target=use_target;pending_->observe_mind=bool(hooks.mind_wrack_targeted);pending_->observe_keystone=bool(hooks.keystone_skill_used);pending_->observe_demonic=bool(hooks.prepare_demonic_flesh_use);pending_->observe_copy=bool(hooks.completed_skill_use);pending_->observe_glimmering=observe_glimmering;
        pending_->balance_caster=balance_caster;pending_->resurrection_scope=corpse_scope;pending_->weapon=weapon;pending_->strength=effective[17];
        pending_->control_epoch=control_epoch;pending_->feast_anchor=feast_anchor;
        pending_->primary=primary;pending_->fast_rank=primary==5?effective[0]:0u;pending_->pvp=pvp;
        if(request.skill==1&&!hooks.healing_signet_activity(now,pending_->complete)){pending_.reset();return reject(Reason::policy);}
        if(!timing::instant_action(*skill)){if(hooks.cast_slot_activity)hooks.cast_slot_activity({request.skill,request.copy},now,pending_->complete);else if(hooks.cast_activity)hooks.cast_activity(request.skill,now,pending_->complete);}
        if(hooks.begin_action)hooks.begin_action({request.skill,request.copy},now);
        pending_attack_epoch_=melee&&hooks.attack_interrupt_epoch?hooks.attack_interrupt_epoch():std::nullopt;
        // A synchronous attack reaction may interrupt inside attack_activity.
        // Retain the admitted attack's earlier epoch rather than adopting the
        // new interrupted epoch as if it were the original state.
        if(melee)hooks.attack_activity(pending_->complete);
        // Pending-slot state and agent animation are independent messages.
        // Dash is an instant stance (native48/UI10000025), not a spell cast.
        std::vector<Packet> begin_packets;
        if(attack_timing){
            // Native attack animation multiplies either the skill's activation or
            // the supplied weapon interval by this factor. Preserve both fields:
            // baking the factor into the interval alone misses fixed-activation skills.
            const float modifier=float(*attack_windup)/float(melee->windup_ms);
            begin_packets.push_back(combat::weapon::timing_2022(actor_,float(attack_timing->period_ms)/1000.f/modifier,modifier));
        }
        begin_packets.push_back(slot_event(227,actor_,{request.skill,request.copy}));
        if(!melee&&*cast_time!=static_cast<Milliseconds>(std::llround(skill->activation*1000.))){
            Packet override;word(override,161);dword(override,61);dword(override,actor_);
            dword(override,std::bit_cast<std::uint32_t>(float(*cast_time)/1000.f));begin_packets.push_back(std::move(override));
        }
        begin_packets.push_back(timing::instant_action(*skill)?generic(48,actor_,request.skill):activation(actor_,request.target,request.skill,bool(melee)));
        hooks.events(begin_packets);
        // Kind50/action21 starts the skill attack itself, including self-centered
        // Cyclone Axe. An ordinary swing here would overwrite its animation.
        if(hooks.diagnostic)hooks.diagnostic(request.skill,"began");return Reason::accepted;
    }
    Reason execute(Milliseconds now,Request request,unsigned primary,const attributes::Ranks& ranks,bool can_cast,const Hooks& hooks,bool pvp,std::optional<effects::HexCopySession::CastAdmission> admission={}){
        const auto* row=reviewed_cast_record(request.skill,pvp);
        if(!row||!timing::instant_action(*row))return begin(now,request,primary,ranks,can_cast,hooks,pvp,admission);
        auto previous=std::move(pending_);const auto attack_epoch=pending_attack_epoch_;const auto aftercast=aftercast_until_;
        pending_.reset();pending_attack_epoch_.reset();
        const auto result=begin(now,request,primary,ranks,can_cast,hooks,pvp,admission);
        if(result==Reason::accepted)update(now,hooks);
        pending_=std::move(previous);pending_attack_epoch_=attack_epoch;aftercast_until_=std::max(aftercast_until_,aftercast);
        return result;
    }

public:
    // Effect coverage only. Eligibility, target hooks and resources are checked
    // separately by start(); this must never be interpreted as a learned grant.
    static bool effect_supported(unsigned id){return supported(id);}
    static bool knockdown_restricted(unsigned id){
        if(id>3432)return true;const auto* skill=find_skill(id);
        return skill->activation>0||skill->aftercast>0||bool(combat::weapon::attack_skill(id,0));
    }
    explicit Runtime(std::uint32_t actor):actor_(actor){if(!actor)throw std::invalid_argument("cast actor required");}
    bool active()const{return bool(pending_)||bool(dagger_failed_second_draft_)||bool(dagger_successful_second_draft_)||bool(pending_contact_amendment_draft_);}
    std::optional<effects::LocalCooldownSnapshot> cooldown_source_snapshot(effects::Scope owner,Milliseconds now,const Hooks& hooks)const{
        if(pending_contact_amendment_draft_||owner.actor!=actor_||!effects::cooldown_owner(owner)||!cooldown_sources_valid_||!hooks.target_scope||hooks.target_scope(actor_)!=owner||
           (cooldown_source_owner_&&*cooldown_source_owner_!=owner))return {};
        effects::LocalCooldownSnapshot snapshot{owner,cooldown_source_revision_,now,{}};
        snapshot.control_epoch=processed_control_epoch_;snapshot.completion_order=processed_completion_order_;snapshot.reset_order=processed_reset_order_;snapshot.amendment_order=processed_amendment_order_;
        for(const auto& [key,until]:recharging_)if(until>now){
            const auto origin=cooldown_sources_.find(key);
            snapshot.timers.push_back({{key.first,key.second},until,origin==cooldown_sources_.end()?std::nullopt:origin->second});
        }
        return snapshot;
    }
    bool consume_cooldown_amendment(const effects::CooldownAmendment& receipt,Milliseconds now,const Hooks& hooks,bool publish_feedback=true){
        if((publish_feedback&&!hooks.events)||!hooks.cooldown_amendment_source||!hooks.target_scope||!hooks.control_epoch||
           receipt.stamp.owner.actor!=actor_||hooks.target_scope(actor_)!=std::optional(receipt.stamp.owner)||
           !receipt.stamp.order||receipt.stamp.at>now||!receipt.physical_event||!receipt.skill_use||
           !effects::cooldown_slot(receipt.slot)||receipt.until<=receipt.stamp.at)return false;
        const auto actual=hooks.cooldown_amendment_source(receipt.physical_event);
        const auto control=hooks.control_epoch();
        if(!actual||*actual!=receipt||receipt.completion_stamp.owner!=receipt.stamp.owner||
           !receipt.completion_stamp.order||receipt.completion_stamp.order>=receipt.stamp.order||
           receipt.completion_stamp.at>receipt.stamp.at||!control||
           effects::contact_amendment_deadline(receipt.stamp.at,receipt.before_until)!=std::optional(receipt.until))return false;
        if(cooldown_source_owner_&&*cooldown_source_owner_!=receipt.stamp.owner)return false;
        if(receipt.stamp.order<=processed_amendment_order_)return true; // authenticated replay, even after timer expiry
        const auto key=std::pair{receipt.slot.skill,receipt.slot.copy};
        const auto before=recharging_.find(key);const auto old_until=before==recharging_.end()?0:before->second;
        const auto old_source=cooldown_sources_.find(key);
        const effects::CooldownOrigin origin{receipt.stamp,effects::CooldownOriginKind::amendment,receipt.physical_event};
        const bool duplicate=old_until==receipt.until&&old_source!=cooldown_sources_.end()&&old_source->second==std::optional(origin);
        cooldown_source_owner_=receipt.stamp.owner;
        // Control belongs to this already-resolved physical attack. Do not run
        // another interruption/completion merely to import its recharge change.
        processed_control_epoch_=std::max(processed_control_epoch_,*control);
        processed_amendment_order_=std::max(processed_amendment_order_,receipt.stamp.order);
        if(duplicate)return true;
        const bool admitted=record_local_cooldown(receipt.slot,receipt.until,origin);
        if(!admitted)return true; // A newer ordered reset legitimately supersedes old source work.
        const auto applied=cooldown_sources_.find(key);
        if(applied==cooldown_sources_.end()||applied->second!=std::optional(origin))return true;
        if(publish_feedback){const auto external=external_disables_.find(key);const auto until=std::max(receipt.until,external==external_disables_.end()?0:external->second);
            hooks.events({until>now?slot_event(228,actor_,receipt.slot,remaining_seconds(until,now)):slot_event(229,actor_,receipt.slot)});}
        return true;
    }
    // Consume only World-minted restoration receipts. Per-slot ordering keeps
    // an older delivery for another slot valid without replaying timers/UI.
    bool consume_copy_restoration(const effects::CopyRestorationReceipt& receipt,
        Milliseconds now,const Hooks& hooks,bool publish_feedback=true){
        const auto owner=receipt.owner();
        if(owner.actor!=actor_||!effects::cooldown_owner(owner)||!hooks.events||!hooks.target_scope||
           hooks.target_scope(actor_)!=owner||!hooks.current_skillbar||!cooldown_sources_valid_||
           cooldown_source_revision_==UINT64_MAX||(cooldown_source_owner_&&*cooldown_source_owner_!=owner))return false;
        const auto bar=hooks.current_skillbar();if(!bar)return false;
        auto recharge=recharging_,external=external_disables_;auto sources=cooldown_sources_;
        auto observed=copy_restorations_;std::vector<Packet> packets;bool changed=false;
        for(const auto& event:receipt.cooldowns()){
            if(event.stamp.owner!=owner||!event.stamp.order||event.stamp.at>now||!event.generation||
               std::find(bar->begin(),bar->end(),event.slot)==bar->end())return false;
            const auto key=std::pair{event.slot.skill,event.slot.copy};const auto old=observed.find(key);
            if(old!=observed.end()){
                if(old->second.stamp.order>event.stamp.order)continue;
                if(old->second.stamp.order==event.stamp.order){if(old->second!=event)return false;continue;}
            }
            observed.insert_or_assign(key,event);changed=true;
            if(event.until<=now)continue;
            const auto normal=recharge.find(key),disabled=external.find(key);
            const auto before=std::max(normal==recharge.end()?0:normal->second,disabled==external.end()?0:disabled->second);
            if(event.kind==effects::EchoRestorationKind::independent_disable){
                if(!hooks.skill_disabled_slot)return false;
                const auto authoritative=hooks.skill_disabled_slot(event.slot,now);
                if(!authoritative||*authoritative<event.until)return false;
                external[key]=std::max(disabled==external.end()?0:disabled->second,*authoritative);
            }else{
                const auto reset=renewal_resets_.find(key);
                if(reset!=renewal_resets_.end()&&reset->second.order>=event.stamp.order)continue;
                if(normal==recharge.end()||normal->second<=event.until){
                    recharge[key]=event.until;
                    sources[key]=effects::CooldownOrigin{event.stamp,effects::CooldownOriginKind::restoration,event.generation};
                }
            }
            const auto after=std::max(recharge.contains(key)?recharge.at(key):0,external.contains(key)?external.at(key):0);
            if(after>before)packets.push_back(slot_event(228,actor_,event.slot,remaining_seconds(after,now)));
        }
        if(!changed)return true;
        recharging_=std::move(recharge);external_disables_=std::move(external);cooldown_sources_=std::move(sources);
        copy_restorations_=std::move(observed);cooldown_source_owner_=owner;touch_cooldown_sources();
        if(publish_feedback&&!packets.empty())hooks.events(packets);return true;
    }
    // Native216 clears the client's old recharge even when our effective
    // deadline did not change. Rebuild UI from current authority after identity
    // publication; this is not a new timer or a recharge reset.
    bool restored_slot_feedback(const std::vector<Slot>& slots,Milliseconds now,const Hooks& hooks)const{
        if(slots.size()>8||!hooks.events||!hooks.target_scope||!hooks.current_skillbar||!hooks.skill_disabled_slot)return false;
        const auto owner=hooks.target_scope(actor_);const auto bar=hooks.current_skillbar();
        if(!owner||!effects::cooldown_owner(*owner)||owner->actor!=actor_||!bar||
           (cooldown_source_owner_&&*cooldown_source_owner_!=*owner))return false;
        std::vector<Packet> packets;std::set<std::pair<unsigned,unsigned>> seen;
        for(const auto slot:slots){
            if(!slot.skill||!seen.emplace(slot.skill,slot.copy).second||std::find(bar->begin(),bar->end(),slot)==bar->end())return false;
            const auto key=std::pair{slot.skill,slot.copy};
            const auto global=hooks.skill_disabled_until?hooks.skill_disabled_until(slot.skill,now):std::optional<Milliseconds>{0};
            const auto owned=hooks.skill_disabled_slot(slot,now);if(!global||!owned)return false;
            const auto local=recharging_.find(key);
            const auto until=std::max({*global,*owned,local==recharging_.end()?0:local->second});
            packets.push_back(until>now?slot_event(228,actor_,slot,remaining_seconds(until,now)):slot_event(229,actor_,slot));
        }
        if(!packets.empty())hooks.events(packets);return true;
    }
    std::optional<effects::RecallFeedbackReply> prepare_recall_feedback(
        const effects::RecallFeedbackNotice& notice,Milliseconds now,const Hooks& hooks)const{
        if(!effects::cooldown_owner(notice.owner)||notice.owner.actor!=actor_||!notice.event||notice.at>now||
           !hooks.target_scope||hooks.target_scope(actor_)!=notice.owner||!hooks.current_skillbar||!cooldown_sources_valid_||
           (cooldown_source_owner_&&*cooldown_source_owner_!=notice.owner))return {};
        const auto bar=hooks.current_skillbar();if(!bar)return {};
        effects::RecallFeedbackReply result{notice,now,*bar,{},{}};
        std::set<std::pair<unsigned,unsigned>> seen;
        for(const auto slot:notice.bar){
            if(!slot.skill){if(slot.copy)return {};continue;}
            if(!effects::cooldown_slot(slot)||!seen.emplace(slot.skill,slot.copy).second)return {};
            if(std::find(bar->begin(),bar->end(),slot)==bar->end())continue;
            if(!hooks.skill_disabled_slot)return {};
            const auto global=hooks.skill_disabled_until?hooks.skill_disabled_until(slot.skill,now):std::optional<Milliseconds>{0};
            const auto owned=hooks.skill_disabled_slot(slot,now);if(!global||!owned)return {};
            const auto key=std::pair{slot.skill,slot.copy};const auto normal=recharging_.find(key),cached=external_disables_.find(key);
            const auto ordinary=normal==recharging_.end()?0:normal->second,previous=cached==external_disables_.end()?0:cached->second;
            const auto disabled=std::max(*global,*owned),before=std::max(ordinary,previous),after=std::max(ordinary,disabled);
            result.slots.push_back({slot,ordinary,previous,disabled});
            if(after>std::max(before,now))result.packets.push_back(slot_event(228,actor_,slot,remaining_seconds(after,now)));
        }
        return result;
    }
    bool consume_recall_feedback(const effects::RecallFeedbackNotice& notice,Milliseconds now,const Hooks& hooks,
        const std::function<bool(const effects::RecallFeedbackReply&)>& publish){
        if(!publish)return false;const auto reply=prepare_recall_feedback(notice,now,hooks);if(!reply)return false;
        auto proposed=external_disables_;
        for(const auto& observation:reply->slots){
            const auto key=std::pair{observation.slot.skill,observation.slot.copy};
            // A different source's earlier reset owns any downward UI change.
            if(observation.cached_until>std::max(observation.disabled_until,now))continue;
            if(observation.disabled_until>now)proposed[key]=observation.disabled_until;else proposed.erase(key);
        }
        if(!publish(*reply))return false;
        external_disables_=std::move(proposed);cooldown_source_owner_=notice.owner;return true;
    }
    // Session-thread consumer of a trusted retained World notice. Stage every
    // source import and authority query before changing local state or wire UI.
    effects::OrderedAttackCooldowns::Result consume_attack_reset(const combat::weapon::DeterminedNotice& notice,
        Milliseconds now,const Hooks& hooks){
        using Result=effects::OrderedAttackCooldowns::Result;
        const auto owner=notice.stamp.owner;
        if(owner.actor!=actor_||!effects::cooldown_owner(owner)||notice.stamp.at>now||!hooks.events||
           !hooks.target_scope||hooks.target_scope(actor_)!=owner||!hooks.skill_disabled_slot||
           !cooldown_sources_valid_||cooldown_source_revision_==UINT64_MAX||
           (cooldown_source_owner_&&*cooldown_source_owner_!=owner))return Result::invalid;
        auto ordered=ordered_attack_cooldowns_.value_or(effects::OrderedAttackCooldowns(owner));
        const auto result=combat::weapon::deliver_determined_recharge(ordered,notice.recharge,notice.stamp);
        if(result!=Result::accepted)return result; // Duplicate deliveries have no wire side effects.
        for(const auto slot:notice.recharge.barriers){
            const auto key=std::pair{slot.skill,slot.copy};const auto timer=recharging_.find(key);
            if(timer==recharging_.end()||timer->second<=now)continue;
            const auto origin=cooldown_sources_.find(key);
            if(origin==cooldown_sources_.end()||!origin->second||origin->second->stamp.owner!=owner||
               !origin->second->token||!origin->second->stamp.order||origin->second->stamp.at>now)return Result::invalid;
            const auto admitted=ordered.put(origin->second->stamp,slot,timer->second);
            if(admitted!=Result::accepted&&admitted!=Result::duplicate)return admitted;
        }
        auto recharge=recharging_,external=external_disables_;auto sources=cooldown_sources_;std::vector<Packet> packets;
        for(const auto slot:notice.recharge.barriers){
            const auto key=std::pair{slot.skill,slot.copy};const auto timer=recharge.find(key),cached=external.find(key);
            const auto before=std::max(timer==recharge.end()?0:timer->second,cached==external.end()?0:cached->second);
            const auto remaining=ordered.until(owner,slot,now),disabled=hooks.skill_disabled_slot(slot,now);
            if(!remaining||!disabled)return Result::invalid;
            // A morale/other reset may already have removed a base timer. Never
            // recreate an absent timer from the delivery timeline's history.
            if(timer!=recharge.end()&&!*remaining){recharge.erase(timer);sources.erase(key);}
            if(*disabled>now)external[key]=*disabled;else external.erase(key);
            const auto local=recharge.find(key);const auto after=std::max(local==recharge.end()?0:local->second,*disabled);
            if(after>now&&after!=before)packets.push_back(slot_event(228,actor_,slot,remaining_seconds(after,now)));
            else if(after<=now&&(before>now||std::find(notice.recharge.cleared.begin(),notice.recharge.cleared.end(),slot)!=notice.recharge.cleared.end()))
                packets.push_back(slot_event(229,actor_,slot));
        }
        ordered_attack_cooldowns_=std::move(ordered);recharging_=std::move(recharge);external_disables_=std::move(external);
        cooldown_sources_=std::move(sources);cooldown_source_owner_=owner;processed_reset_order_=std::max(processed_reset_order_,notice.stamp.order);touch_cooldown_sources();
        if(!packets.empty())hooks.events(packets);return Result::accepted;
    }
    // Only the owning World may issue this permit after validating source
    // completeness and the acknowledged reset prefix. It is not client data.
    bool apply_cooldown_checkpoint(const effects::CooldownCheckpointPermit& permit,Milliseconds now,const Hooks& hooks){
        const auto& accepted=permit.accepted.snapshot;const auto owner=accepted.owner;
        if(permit.watermark.owner!=owner||accepted.at>permit.watermark.at||permit.watermark.at>now||
           cooldown_source_snapshot(owner,accepted.at,hooks)!=std::optional(accepted))return false;
        std::vector<effects::CooldownCheckpointSource> active;
        for(const auto& timer:accepted.timers){const auto* row=find_skill(timer.slot.skill);if(!row)return false;
            if(!effects::flourish_attack_skill(timer.slot.skill)||timer.until<=permit.watermark.at)continue;
            if(!timer.origin||std::find(permit.accepted.verified.begin(),permit.accepted.verified.end(),timer.slot)==permit.accepted.verified.end())return false;
            active.push_back({timer.origin->stamp,timer.slot,timer.until});
        }
        auto ordered=ordered_attack_cooldowns_.value_or(effects::OrderedAttackCooldowns(owner));
        if(!ordered.checkpoint_attack_skills(permit.watermark,active))return false;
        ordered_attack_cooldowns_=std::move(ordered);return true;
    }
    bool needs_cooldown_checkpoint()const{return ordered_attack_cooldowns_&&ordered_attack_cooldowns_->needs_checkpoint();}

    // Stage the complete Session half before the World reset/Energy commit.
    // No post-commit authority queries or fallible timer imports are permitted.
    bool reset_flourish(const effects::FlourishPlan& plan,const effects::FlourishPermit& permit,
        Milliseconds now,std::uint64_t event,const Hooks& hooks){
        using Result=effects::OrderedAttackCooldowns::Result;const auto owner=permit.stamp.owner;
        if(owner.actor!=actor_||permit.stamp.at!=now||!hooks.target_scope||hooks.target_scope(actor_)!=owner||
           !hooks.events||!hooks.current_skillbar||!hooks.skill_disabled_slot||!hooks.commit_flourish_reset||
           !cooldown_sources_valid_||cooldown_source_revision_==UINT64_MAX||
           (cooldown_source_owner_&&*cooldown_source_owner_!=owner))return false;
        const auto bar=hooks.current_skillbar();if(!bar||*bar!=permit.bar.bar)return false;
        effects::FlourishCooldowns snapshot{owner,cooldown_source_revision_,last_flourish_reset_,now,true,{}};
        for(const auto slot:*bar)if(slot.skill){
            const auto key=std::pair{slot.skill,slot.copy};const auto local=recharging_.find(key);
            const auto disabled=hooks.skill_disabled_slot(slot,now);
            const auto global=hooks.skill_disabled_until?hooks.skill_disabled_until(slot.skill,now):std::optional<Milliseconds>{0};
            if(!disabled||!global)return false;
            snapshot.slots.push_back({slot,local==recharging_.end()?0:local->second,std::max(*disabled,*global)});
        }
        const auto proposal=effects::prepare_flourish_recharge(snapshot,plan,event);if(!proposal)return false;
        auto ordered=ordered_attack_cooldowns_.value_or(effects::OrderedAttackCooldowns(owner));
        for(const auto& entry:snapshot.slots)if(effects::flourish_attack_skill(entry.slot.skill)&&entry.recharge_until>now){
            const auto origin=cooldown_sources_.find({entry.slot.skill,entry.slot.copy});
            if(origin==cooldown_sources_.end()||!origin->second||!origin->second->token||origin->second->stamp.owner!=owner||
               origin->second->stamp.at>now)return false;
            const auto admitted=ordered.put(origin->second->stamp,entry.slot,entry.recharge_until);
            if(admitted!=Result::accepted&&admitted!=Result::duplicate)return false;
        }
        if(effects::deliver_flourish_recharge(ordered,*proposal,permit.stamp)!=Result::accepted)return false;
        auto recharge=recharging_,external=external_disables_;auto sources=cooldown_sources_;std::vector<Packet> packets;
        for(const auto slot:proposal->barriers){const auto key=std::pair{slot.skill,slot.copy};
            recharge.erase(key);external.erase(key);sources.erase(key);
            if(std::find(proposal->cleared.begin(),proposal->cleared.end(),slot)!=proposal->cleared.end())packets.push_back(slot_event(229,actor_,slot));
        }
        const auto committed=hooks.commit_flourish_reset(*proposal,permit,now);
        if(!committed)return false;
        if(*committed!=permit.stamp)throw std::logic_error("Flourish World returned a different committed stamp");
        ordered_attack_cooldowns_=std::move(ordered);recharging_=std::move(recharge);external_disables_=std::move(external);
        cooldown_sources_=std::move(sources);cooldown_source_owner_=owner;touch_cooldown_sources();last_flourish_reset_=event;
        processed_reset_order_=std::max(processed_reset_order_,permit.stamp.order);
        if(!packets.empty())hooks.events(packets);return true;
    }

    // Called by a successful initial-effect transaction, under the owning
    // session lock. The World callback must commit only the supplied reset
    // keys against its captured revision permit, without reentering Runtime.
    bool reset_other_signets(effects::Scope owner,const std::vector<Slot>& eligible,
        Milliseconds now,std::uint64_t event,const std::function<bool(const std::vector<Slot>&)>& commit_world,const Hooks& hooks){
        if(owner.actor!=actor_||!hooks.target_scope||hooks.target_scope(actor_)!=owner||!hooks.events||!commit_world)return false;
        effects::KeystoneCooldowns snapshot{owner,1,last_signet_reset_,now,{}};
        for(const auto slot:eligible){
            const auto key=std::pair{slot.skill,slot.copy};const auto recharge=recharging_.find(key);
            const auto global=hooks.skill_disabled_until?hooks.skill_disabled_until(slot.skill,now):std::optional<Milliseconds>{0};
            const auto local=hooks.skill_disabled_slot?hooks.skill_disabled_slot(slot,now):std::optional<Milliseconds>{0};
            if(!global||!local)return false;
            snapshot.slots.push_back({slot,recharge==recharging_.end()?0:recharge->second,std::max({*global,*local,external_disables_.contains(key)?external_disables_.at(key):0}),spent_signets_.contains(key)});
        }
        const auto proposal=effects::prepare_keystone_recharge(snapshot,owner,now,event);if(!proposal)return false;
        auto recharge=recharging_;std::vector<Packet> packets;
        for(const auto slot:proposal->cleared){recharge.erase({slot.skill,slot.copy});packets.push_back(slot_event(229,actor_,slot));}
        if(!commit_world(proposal->cleared))return false;
        recharging_=std::move(recharge);for(const auto slot:proposal->cleared){external_disables_.erase({slot.skill,slot.copy});cooldown_sources_.erase({slot.skill,slot.copy});}touch_cooldown_sources();last_signet_reset_=event;
        if(!packets.empty())hooks.events(packets);return true;
    }
    void interrupt_control(Milliseconds now,const Hooks& hooks){
        if(pending_&&knockdown_restricted(pending_->request.skill))interrupt(now,hooks);
    }
    // Call only for a genuine server-authorized morale boost. A new map owns
    // a fresh Runtime, matching wiki's explicit zoning reset for this signet.
    void morale_boost(const Hooks& hooks){
        std::set<std::pair<unsigned,unsigned>> cleared;for(const auto& [slot,deadline]:recharging_)cleared.insert(slot);
        for(const auto& [slot,deadline]:external_disables_)cleared.insert(slot);
        for(const auto slot:cleared)hooks.events({slot_event(229,actor_,{slot.first,slot.second})});
        recharging_.clear();external_disables_.clear();cooldown_sources_.clear();touch_cooldown_sources();
        for(auto [skill,copy]:spent_signets_)hooks.events({slot_event(229,actor_,{skill,copy})});
        spent_signets_.clear();
    }
    Reason start(Milliseconds now,Request request,const Slots& bar,const Policy& policy,
                 const attributes::Ranks& effective,bool can_cast,const Hooks& hooks){
        auto reject=[&](Reason r){if(request.skill<=3432)hooks.events({cancellation_packet(actor_,{request.skill,request.copy})});return r;};
        if(trusted_generation_)return reject(Reason::policy);
        if(request.skill>3432)return Reason::unsupported;
        if(!can_cast)return reject(Reason::mode);
        if(!request.skill||std::find(bar.begin(),bar.end(),Slot{request.skill,request.copy})==bar.end())return reject(Reason::slot);
        std::optional<effects::HexCopySession::CastAdmission> admission;
        if(hooks.admit_session_cast||hooks.valid_session_cast){
            if(!hooks.admit_session_cast||!hooks.valid_session_cast)return reject(Reason::policy);
            admission=hooks.admit_session_cast(bar,policy,request,now);
            if(!admission||admission->cast.owner.actor!=actor_||admission->started!=now||
               admission->cast.index>=bar.size()||bar[admission->cast.index]!=Slot{request.skill,request.copy}||
               admission->cast.requested!=Slot{request.skill,request.copy}||!hooks.valid_session_cast(*admission,now))return reject(Reason::policy);
        }else if(validate(bar,policy)!=skills::Reason::accepted)return reject(Reason::policy);
        if(policy.pvp_match&&request.skill==791)return reject(Reason::unsupported); // PvP split2866 is not reviewed.
        if(policy.pvp_match&&(find_skill(request.skill)->special&0x80000u))return reject(Reason::pve_only);
        return execute(now,request,policy.primary,effective,can_cast,hooks,policy.pvp_match,admission);
    }
    // Server-owned NPC/environment entrypoint. Never route a client actor/skill
    // request here. Explicit definition/ranks/resources must come from its own
    // instance, and actor identity/generation is checked again each update.
    Reason start_trusted(Milliseconds now,Request request,const ActorContext& actor,
                         bool can_cast,const Hooks& hooks){
        if(actor.actor!=actor_||!actor.generation||actor.primary>10||!hooks.actor_generation||
           hooks.actor_generation()!=std::optional(actor.generation)||
           (trusted_generation_&&*trusted_generation_!=actor.generation)||
           std::any_of(actor.effective.begin(),actor.effective.end(),[](auto r){return r>20;}))return Reason::policy;
        trusted_generation_=actor.generation;
        return execute(now,request,actor.primary,actor.effective,can_cast,hooks,actor.pvp);
    }
    void cancel(const Hooks& hooks,std::optional<Milliseconds> at={},effects::GlyphOutcome outcome=effects::GlyphOutcome::voluntary_cancel){if(pending_&&pending_->use_owner&&!hooks.target_scope&&!(trusted_generation_&&(!hooks.actor_generation||hooks.actor_generation()!=trusted_generation_)))return;cancel_dagger_successful_second_draft(hooks,true);cancel_dagger_failed_second_draft(hooks,true);if(!pending_)return;auto p=std::move(*pending_);pending_.reset();if((trusted_generation_&&(!hooks.actor_generation||hooks.actor_generation()!=trusted_generation_))||(p.use_owner&&(!hooks.target_scope||hooks.target_scope(actor_)!=p.use_owner)))return;
        if(p.glyph_event&&hooks.settle_elemental_glyph){const auto now=std::max(at.value_or(p.began),hooks.glyph_clock?hooks.glyph_clock():p.began);
            const auto result=hooks.settle_elemental_glyph(p.glyph_event,outcome,now);
            if(!result.accepted&&hooks.diagnostic)hooks.diagnostic(p.request.skill,"glyph_terminal_unsupported_or_stale");}
        if(!timing::instant_action(*find_skill(p.request.skill))){if(hooks.cast_slot_activity)hooks.cast_slot_activity({p.request.skill,p.request.copy},0,0);else if(hooks.cast_activity)hooks.cast_activity(p.request.skill,0,0);}if(p.request.skill==1&&hooks.healing_signet_activity)hooks.healing_signet_activity(0,{});if(p.weapon&&hooks.attack_activity)hooks.attack_activity({});if(p.weapon)hooks.events({combat::weapon::stop_2022(actor_)});hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(p.weapon?49:59,actor_)});}
    // Involuntary interruption starts normal recharge; voluntary cancellation
    // does not. Completion-only recharge modifiers are deliberately excluded.
    void interrupt(Milliseconds now,const Hooks& hooks){
        cancel_dagger_successful_second_draft(hooks,true);cancel_dagger_failed_second_draft(hooks,true);
        if(!pending_)return;
        if(trusted_generation_&&(!hooks.actor_generation||hooks.actor_generation()!=trusted_generation_)){pending_.reset();return;}
        const auto request=pending_->request;std::optional<effects::CooldownOrigin> origin;
        if(pending_->control_epoch&&hooks.cooldown_control_source){
            const auto control=hooks.cooldown_control_source(*pending_->control_epoch);
            if(control&&control->stamp&&control->slot==Slot{request.skill,request.copy}&&control->epoch==*pending_->control_epoch+1&&
               cooldown_owner_matches(*control->stamp,now,hooks))origin=effects::CooldownOrigin{*control->stamp,effects::CooldownOriginKind::control,control->epoch};
        }
        const auto seconds=reviewed_cast_record(request.skill,pending_->pvp)->recharge;
        if(pending_->glyph_quote&&pending_->glyph_quote->glyph.plan.skill==203){
            const auto settled_at=std::max(now,hooks.glyph_clock?hooks.glyph_clock():now);
            if(!origin||!hooks.finish_renewal_spell||!synchronize_disables(settled_at,hooks))return;
            const auto previous=recharging_.find({request.skill,request.copy});
            const auto existing=previous==recharging_.end()?0:previous->second;
            const auto receipt=hooks.finish_renewal_spell(pending_->glyph_event,0,{request.skill,request.copy},effects::GlyphOutcome::interrupted,
                settled_at,Milliseconds(seconds)*1000,existing,origin->token);
            if(!receipt||!accept_renewal_terminal(*receipt,*pending_,0,settled_at,Milliseconds(seconds)*1000,existing,origin->token,hooks)){
                if(hooks.diagnostic)hooks.diagnostic(request.skill,"renewal_interrupt_commit_failed");return;
            }
            pending_->glyph_event=0;processed_control_epoch_=std::max(processed_control_epoch_,origin->token);
            if(receipt->independent_disable_until>settled_at){auto& end=external_disables_[{request.skill,request.copy}];end=std::max(end,receipt->independent_disable_until);}
            cancel(hooks,settled_at,effects::GlyphOutcome::interrupted);
            const auto local=recharging_.find({request.skill,request.copy}),external=external_disables_.find({request.skill,request.copy});
            const auto deadline=std::max(local==recharging_.end()?0:local->second,external==external_disables_.end()?0:external->second);
            hooks.events({deadline>settled_at?slot_event(228,actor_,{request.skill,request.copy},remaining_seconds(deadline,settled_at)):slot_event(229,actor_,{request.skill,request.copy})});return;
        }
        if(!synchronize_disables(now,hooks))return;
        if(hooks.settle_attack_interrupt&&!origin&&pending_->control_epoch&&hooks.control_epoch&&hooks.control_epoch()!=pending_->control_epoch){
            if(hooks.diagnostic)hooks.diagnostic(request.skill,"attack_interrupt_control_unavailable");return;
        }
        const bool settle_attack=bool(hooks.settle_attack_interrupt)&&origin.has_value();
        if(!settle_attack)cancel(hooks,now,effects::GlyphOutcome::interrupted);
        bool recorded=true;
        const auto old_recharging=settle_attack?std::optional(recharging_):std::nullopt;
        const auto old_sources=settle_attack?std::optional(cooldown_sources_):std::nullopt;
        const auto old_ordered=ordered_attack_cooldowns_;const auto old_revision=cooldown_source_revision_;const auto old_valid=cooldown_sources_valid_;
        if(seconds){recorded=record_local_cooldown({request.skill,request.copy},now+std::uint64_t(seconds)*1000,origin);
            if(hooks.cooldown_control_source&&!origin&&hooks.diagnostic)hooks.diagnostic(request.skill,"cooldown_unsequenced_interrupt");}
        if(settle_attack){
            const auto local=recharging_.find({request.skill,request.copy});
            const auto base=local==recharging_.end()?now:local->second;
            const auto settled=recorded?hooks.settle_attack_interrupt(*origin,{request.skill,request.copy},base,now):std::nullopt;
            if(!settled||(*settled&&*settled<base)){
                recharging_=*old_recharging;cooldown_sources_=*old_sources;ordered_attack_cooldowns_=old_ordered;
                cooldown_source_revision_=old_revision;cooldown_sources_valid_=old_valid;
                if(hooks.diagnostic)hooks.diagnostic(request.skill,"attack_interrupt_settlement_failed");return;
            }
            if(*settled){auto& until=external_disables_[{request.skill,request.copy}];until=std::max(until,*settled);}
        }
        if(settle_attack)cancel(hooks,now,effects::GlyphOutcome::interrupted);
        const auto deadline=recharging_.find({request.skill,request.copy});
        const auto external=external_disables_.find({request.skill,request.copy});
        const auto displayed=std::max({recorded?std::uint64_t(seconds):0,deadline==recharging_.end()?0:std::uint64_t(remaining_seconds(deadline->second,now)),external==external_disables_.end()?0:std::uint64_t(remaining_seconds(external->second,now))});
        hooks.events({displayed?slot_event(228,actor_,{request.skill,request.copy},static_cast<unsigned>(displayed)):slot_event(229,actor_,{request.skill,request.copy})});
    }
    void update(Milliseconds now,const Hooks& hooks){
        if((trusted_generation_&&(!hooks.actor_generation||hooks.actor_generation()!=trusted_generation_))||
           (pending_&&pending_->use_owner&&hooks.target_scope&&hooks.target_scope(actor_)!=pending_->use_owner)||
           (dagger_successful_second_draft_&&hooks.target_scope&&hooks.target_scope(actor_)!=std::optional(dagger_successful_second_draft_->source()))||
           (pending_contact_amendment_draft_&&hooks.target_scope&&hooks.target_scope(actor_)!=std::optional(pending_contact_amendment_draft_->stamp.owner))){
            // A recycled numeric actor ID is another actor. Do not emit expiry,
            // healing, speed resets or recharge notifications to its replacement.
            cancel_dagger_successful_second_draft(hooks,false);cancel_dagger_failed_second_draft(hooks,false);pending_contact_amendment_draft_.reset();pending_.reset();pending_attack_epoch_.reset();effects_.clear();glyph_charges_=0;recharging_.clear();external_disables_.clear();cooldown_sources_.clear();cooldown_source_owner_.reset();renewal_resets_.clear();copy_restorations_.clear();ordered_attack_cooldowns_.reset();processed_control_epoch_=processed_completion_order_=processed_reset_order_=processed_amendment_order_=0;touch_cooldown_sources();spent_signets_.clear();aftercast_until_=0;
            if(hooks.diagnostic)hooks.diagnostic(0,"actor_replaced");return;
        }
        // Missing scoped authority is unavailable, not proof of retirement.
        // The trusted-generation contract above retains its existing priority.
        if(pending_&&pending_->use_owner&&!hooks.target_scope)return;
        if(pending_&&pending_->session_admission&&(!hooks.valid_session_cast||!hooks.valid_session_cast(*pending_->session_admission,now))){
            if(hooks.diagnostic)hooks.diagnostic(pending_->request.skill,"session_cast_expired");
            cancel(hooks);return;
        }
        if((dagger_successful_second_draft_||pending_contact_amendment_draft_)&&!hooks.target_scope)return;
        if(pending_contact_amendment_draft_){
            if(!consume_cooldown_amendment(*pending_contact_amendment_draft_,now,hooks))return;
            pending_contact_amendment_draft_.reset();
        }
        if(!update_dagger_successful_second_draft(now,hooks))return;
        update_dagger_failed_second_draft(now,hooks);
        if(!synchronize_disables(now,hooks))return;
        const auto observed_control=hooks.control_epoch?hooks.control_epoch():std::nullopt;
        if(pending_&&pending_->control_epoch&&observed_control!=pending_->control_epoch){
            if(hooks.diagnostic)hooks.diagnostic(pending_->request.skill,"control_interrupted");
            const auto event=hooks.control_event_time?hooks.control_event_time(*pending_->control_epoch):std::nullopt;
            interrupt(event&&*event<=now?*event:now,hooks);
            // A failed authoritative terminal transaction retains the cast for
            // retry. It must never fall through and execute its spell effect.
            if(pending_&&pending_->control_epoch!=observed_control)return;
        }
        // If interruption was deferred (for example an authority query failed),
        // the pending old epoch remains and must not be acknowledged yet.
        if(observed_control&&(!pending_||!pending_->control_epoch||pending_->control_epoch==observed_control))
            processed_control_epoch_=std::max(processed_control_epoch_,*observed_control);
        const auto self=(pending_||!effects_.empty())?hooks.resources(actor_):std::nullopt;
        if(pending_&&pending_->weapon&&pending_attack_epoch_&&hooks.attack_interrupt_epoch&&
           hooks.attack_interrupt_epoch()!=pending_attack_epoch_){cancel(hooks);pending_attack_epoch_.reset();}
        if(pending_&&(!self||!self->health))cancel(hooks);
        if(!effects_.empty()&&hooks.effect_snapshot){const auto snapshot=hooks.effect_snapshot(now);
            // An unavailable authority query is not evidence of removal.
            // Preserve cached mechanics until a successful snapshot or expiry.
            for(auto it=effects_.begin();it!=effects_.end();)if(snapshot&&now<it->second.ends&&std::none_of(snapshot->begin(),snapshot->end(),[&](const auto& e){return e.instance==it->second.id;})){
                if(it->first==1043&&hooks.speed_multiplier)hooks.speed_multiplier(1.0);it=effects_.erase(it);
            }else ++it;
        }
        for(auto it=effects_.begin();it!=effects_.end();)if(now>=it->second.ends||!self||!self->health){const bool removed=!hooks.status_mutation||hooks.status_mutation(it->first,it->second.id,0,now,{},false);if(removed){if(it->first==1043&&hooks.speed_multiplier)hooks.speed_multiplier(1.0);hooks.events({effect_remove(actor_,it->second.id)});}if(hooks.diagnostic)hooks.diagnostic(it->first,"expired");it=effects_.erase(it);}else ++it;
        std::set<std::pair<unsigned,unsigned>> expired_cooldowns;
        for(const auto& [slot,until]:recharging_)if(until<=now)expired_cooldowns.insert(slot);
        for(const auto& [slot,until]:external_disables_)if(until<=now)expired_cooldowns.insert(slot);
        for(const auto slot:expired_cooldowns){
            if(recharging_.contains(slot)&&recharging_.at(slot)<=now){recharging_.erase(slot);cooldown_sources_.erase(slot);touch_cooldown_sources();}
            if(external_disables_.contains(slot)&&external_disables_.at(slot)<=now)external_disables_.erase(slot);
            if(!recharging_.contains(slot)&&!external_disables_.contains(slot)){
                if(hooks.recall_feedback_pending){
                    const auto retained=hooks.recall_feedback_pending({slot.first,slot.second},now);
                    if(!retained)return;if(*retained)continue;
                }
                hooks.events({slot_event(229,actor_,{slot.first,slot.second})});
            }
        }
        if(!pending_||now<pending_->complete)return;const auto p=std::move(*pending_);pending_.reset();
        if((p.pet_charm&&(!hooks.pet_charm_completed||!hooks.target_scope))||
           (p.request.skill==436&&(!hooks.pet_comfort_completed||!hooks.pet_comfort_ready||!hooks.target_scope))){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        struct GlyphEnd {const Hooks& hooks;std::uint64_t event;unsigned skill;Milliseconds now;bool done=false;
            void finish(effects::GlyphOutcome outcome){if(done||!event)return;done=true;
                const auto result=hooks.settle_elemental_glyph?hooks.settle_elemental_glyph(event,outcome,std::max(now,hooks.glyph_clock?hooks.glyph_clock():now)):effects::ElementalGlyphSettlement{};
                if(!result.accepted&&hooks.diagnostic)hooks.diagnostic(skill,"glyph_terminal_unsupported_or_stale");}
            ~GlyphEnd(){finish(effects::GlyphOutcome::failed);}
        } glyph_end{hooks,p.glyph_event,p.request.skill,now};
        if(!timing::instant_action(*find_skill(p.request.skill))){if(hooks.cast_slot_activity)hooks.cast_slot_activity({p.request.skill,p.request.copy},0,0);else if(hooks.cast_activity)hooks.cast_activity(p.request.skill,0,0);}
        if(p.request.skill==1&&hooks.healing_signet_activity)hooks.healing_signet_activity(now,{});
        if(p.weapon&&hooks.attack_activity)hooks.attack_activity({});
        if(p.use_owner&&(!hooks.target_scope||hooks.target_scope(actor_)!=p.use_owner||(p.observe_glimmering&&!hooks.glimmering_targeted_use)||(p.observe_copy&&!hooks.completed_skill_use)||(p.observe_panic&&!hooks.panic_skill_used)||(p.observe_mind&&!hooks.mind_wrack_targeted)||(p.observe_keystone&&!hooks.keystone_skill_used)||(p.observe_demonic&&(!hooks.prepare_demonic_flesh_use||!hooks.commit_demonic_flesh_use||!hooks.cancel_demonic_flesh_use))||(p.use_target&&hooks.target_scope(p.request.target)!=p.use_target))){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"skill_use_owner_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(p.weapon?49:59,actor_)});return;
        }
        if(p.energy_drain_completion&&(!hooks.energy_drain_completed||!hooks.before_spell_effect||!hooks.target_scope||!p.resurrection_scope||hooks.target_scope(p.request.target)!=p.resurrection_scope)){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"energy_drain_completion_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.ally_cleansing_completion&&(!hooks.ally_cleansing_completed||!hooks.before_spell_effect||!hooks.target_scope||!p.resurrection_scope||hooks.target_scope(p.request.target)!=p.resurrection_scope)){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"ally_cleansing_completion_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.effect_removal_completion&&(!hooks.remove_effects_completed||!hooks.before_spell_effect||!hooks.target_scope||!p.resurrection_scope||hooks.target_scope(p.request.target)!=p.resurrection_scope)){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"effect_removal_completion_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.burning_speed_completion&&(!hooks.burning_speed_completed||!hooks.before_spell_effect||!hooks.target_scope||!p.use_owner||hooks.target_scope(actor_)!=p.use_owner)){
            if(hooks.diagnostic)hooks.diagnostic(823,"burning_speed_completion_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.resurrection_completion&&(!hooks.resurrection_completed||!hooks.before_spell_effect||!hooks.target_scope||!p.use_owner||hooks.target_scope(actor_)!=p.use_owner||
                (p.resurrection_scope&&hooks.target_scope(p.request.target)!=p.resurrection_scope))){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"resurrection_completion_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.healing_ring_completion&&(!hooks.healing_ring_completed||!hooks.before_spell_effect||!hooks.target_scope||!p.resurrection_scope||hooks.target_scope(actor_)!=p.resurrection_scope)){
            if(hooks.diagnostic)hooks.diagnostic(1262,"healing_ring_completion_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.party_heal_completion&&((p.request.skill==2221?!hooks.party_heal_published:!hooks.party_heal_completed)||!hooks.before_spell_effect||!hooks.target_scope||!p.resurrection_scope||hooks.target_scope(actor_)!=p.resurrection_scope)){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"party_heal_completion_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.condition_removal_completion&&(!hooks.remove_conditions_completed||!hooks.before_spell_effect||!hooks.target_scope||!p.resurrection_scope||hooks.target_scope(p.request.target)!=p.resurrection_scope)){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"condition_removal_completion_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.status_heal_completion&&(!hooks.status_heal_target_valid||!hooks.status_heal_completed||!hooks.before_spell_effect||!hooks.target_scope||!p.resurrection_scope||hooks.target_scope(p.request.target)!=p.resurrection_scope)){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"status_heal_completion_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.request.skill==2062&&(p.healing_ribbon_completion?(!hooks.healing_ribbon_completed||!hooks.before_spell_effect):!hooks.healing_ribbon)){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"healing_ribbon_completion_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.request.skill==1741&&(p.ghostmirror_light_completion?(!hooks.ghostmirror_light_completed||!hooks.before_spell_effect):!hooks.ghostmirror_light)){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"ghostmirror_light_completion_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.request.skill==1234&&(p.mend_body_soul_completion?(!hooks.mend_body_soul_completed||!hooks.before_spell_effect):!hooks.mend_body_soul)){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"mend_body_soul_completion_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.request.skill==915&&(p.spirit_light_completion?(!hooks.spirit_light_completed||!hooks.before_spell_effect):!hooks.spirit_light)){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"spirit_light_completion_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.request.skill==1121&&(p.gift_health_completion?(!hooks.gift_health_completed||!hooks.before_spell_effect):!hooks.gift_health)){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"gift_health_completion_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        const auto glyph_cast=(p.request.skill==198||p.request.skill==199||p.request.skill==203)?effects::elemental_glyph(p.request.skill,p.energy_storage_rank):std::nullopt;
        if(p.direct_heal_completion&&(!hooks.direct_heal_completed||!hooks.direct_heal_target_valid||!hooks.before_spell_effect)){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"direct_heal_completion_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if((p.request.skill==292&&(p.health_completion?(!hooks.infuse_health_completed||!hooks.before_spell_effect):!hooks.infuse_health))||
           (p.request.skill==1526&&(p.health_completion?(!hooks.imbue_health_completed||!hooks.before_spell_effect):!hooks.imbue_health))){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"health_completion_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        const auto demonic_cast=p.request.skill==130?effects::demonic_flesh(p.effect_rank):std::nullopt;
        const auto divine_cast=p.request.skill==310?effects::divine_spirit(p.effect_rank):std::nullopt;
        const auto selfless_cast=(p.request.skill==1952||p.request.skill==2095)?effects::selfless_spirit(p.request.skill,p.effect_rank):std::nullopt;
        const auto* skill=reviewed_cast_record(p.request.skill,p.pvp);auto target=hooks.resources(p.request.target);const auto caster=hooks.resources(actor_);
        const auto resurrection=effects::resurrection(p.request.skill,p.effect_rank);
        const auto interrupt=effects::interrupt_skill(p.request.skill,p.effect_rank);
        const bool balance=p.request.skill==86;
        const auto panic=p.request.skill==52?effects::panic(p.effect_rank):std::nullopt;
        const auto mind=(p.request.skill==49||p.request.skill==2734)?effects::mind_wrack(p.effect_rank,p.pvp||p.request.skill==2734):std::nullopt;
        const auto wandering=p.request.skill==2056&&!p.pvp?effects::wandering_eye(p.effect_rank):std::nullopt;
        const auto fevered=(p.request.skill==55||p.request.skill==3289)?effects::fevered_dreams(p.effect_rank,p.pvp||p.request.skill==3289):std::nullopt;
        const auto cry=p.request.skill==57?effects::cry_frustration(p.effect_rank):std::nullopt;
        const auto inspired=effects::inspired_hex(p.request.skill,p.effect_rank);
        const auto diversion=p.request.skill==30?effects::diversion(p.effect_rank):std::nullopt;
        const auto humility=p.request.skill==62?effects::signet_humility(p.effect_rank):std::nullopt;
        const auto lightning=effects::lightning(p.request.skill,p.effect_rank,p.energy_storage_rank);
        const auto disenchant=effects::enchantment_removal(p.request.skill,p.effect_rank);
        const auto delusions=p.request.skill==1337?effects::drain_delusions(p.effect_rank):std::nullopt;
        const auto shatter=p.request.skill==27?effects::shatter_delusions(p.effect_rank):p.request.skill==67?effects::shatter_hex(p.effect_rank):p.request.skill==302?effects::smite_hex(p.effect_rank,p.divine_rank):std::nullopt;
        const auto faint=effects::attack_slow_hex(p.request.skill,p.effect_rank);
        const auto swarm=p.request.skill==105?effects::deathly_swarm(p.effect_rank):std::nullopt;
        const auto javelin=p.request.skill==230?effects::lightning_javelin(p.effect_rank):std::nullopt;
        const auto blindness=effects::blindness(p.request.skill,p.effect_rank);
        const auto transfer=effects::condition_transfer(p.request.skill,p.effect_rank);
        const auto condition=p.request.skill==2229?effects::signet_infection_condition(p.effect_rank):effects::condition_spell(p.request.skill,p.effect_rank);
        const auto virulence=p.request.skill==107?effects::virulence(p.effect_rank):std::nullopt;
        const auto rotting_flesh=p.request.skill==106?effects::rotting_flesh(p.effect_rank):std::nullopt;
        const auto tainted_flesh=p.request.skill==113?effects::tainted_flesh(p.effect_rank):std::nullopt;
        const auto spiteful=p.request.skill==121?effects::spiteful_spirit(p.effect_rank):std::nullopt;
        const auto energy_enchantment=(p.request.skill==119||p.request.skill==157)?effects::energy_enchantment(p.request.skill,p.effect_rank):std::nullopt;
        if(energy_enchantment&&(!hooks.energy_enchantment_target_valid||
           (p.energy_enchantment_completion?(!hooks.energy_enchantment_completed||!hooks.before_spell_effect):!hooks.energy_enchantment))){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"energy_enchantment_completion_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        const auto steal=effects::life_steal(p.request.skill,p.effect_rank);
        if(steal&&(!hooks.target_scope||!hooks.projectile_target_valid||
           (p.life_steal_spell_completion?(!hooks.life_steal_spell_completed||!hooks.before_spell_effect):
            p.life_steal_completion?!hooks.complete_life_steal:!hooks.life_steal))){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"life_steal_completion_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        const auto storm=(p.request.skill==197||p.request.skill==1094||p.request.skill==215||p.request.skill==167||p.request.skill==187||p.request.skill==192)?effects::fire_storm(p.effect_rank,p.request.skill):std::nullopt;
        const auto ray=p.request.skill==830?effects::ray_of_judgment(p.effect_rank):std::nullopt;
        const auto elemental=effects::elemental(p.request.skill,p.effect_rank,p.energy_storage_rank);
        if(elemental&&(p.elemental_completion?(!hooks.elemental_completed||!hooks.before_spell_effect):
                (p.request.skill==1068?!hooks.conditional_elemental:!hooks.elemental))){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"elemental_completion_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        const auto slippery=p.request.skill==2191?effects::slippery_ground(p.effect_rank):std::nullopt;
        const auto burn=effects::mind_burn(p.request.skill,p.effect_rank);
        const auto daggers=p.request.skill==172?effects::stone_daggers(p.effect_rank):std::nullopt;
        const auto dancing=p.request.skill==858?effects::dancing_daggers(p.effect_rank):std::nullopt;
        const auto iron=p.request.skill==786?effects::iron_palm(p.effect_rank):std::nullopt;
        const auto disrupting=p.request.skill==571?effects::disrupting_dagger(p.effect_rank):std::nullopt;
        if(p.request.skill==571&&(!disrupting||!hooks.disrupting_dagger||!hooks.target_scope||!hooks.projectile_target_valid)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        const auto& weapon_spell=p.weapon_spell;
        if(weapon_spell&&(!effects::valid_weapon_spell(*weapon_spell)||!hooks.weapon_spell||!hooks.weapon_spell_target_valid||!hooks.before_spell_effect||!hooks.target_scope)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        const auto cleansing=effects::ally_cleansing(p.request.skill,p.effect_rank,p.divine_rank);
        const auto barbs=p.request.skill==100?effects::soul_barbs(p.effect_rank):std::nullopt;
        const auto backfire=effects::backfire(p.request.skill,p.effect_rank);
        const auto mend=p.request.skill==290?effects::mending(p.effect_rank):std::nullopt;
        const auto succor=p.request.skill==308?effects::succor():std::nullopt;
        const auto retribution=p.request.skill==248?effects::retribution(p.effect_rank):std::nullopt;
        if(p.request.skill==308&&(!succor||!hooks.succor||!hooks.succor_target_valid||
           !hooks.before_spell_effect||!hooks.target_scope)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        const auto recall_cast=p.request.skill==925?effects::recall_shadow():std::nullopt;
        const auto mimicry_cast=p.request.skill==65?effects::arcane_mimicry():std::nullopt;
        if(p.request.skill==65&&(!mimicry_cast||!hooks.mimicry||!hooks.mimicry_target_valid||!hooks.before_spell_effect||!hooks.target_scope)){hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;}
        if(p.request.skill==925&&(!recall_cast||!hooks.recall||!hooks.recall_target_valid||!hooks.before_spell_effect||!hooks.target_scope)){hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;}
        if(p.request.skill==290&&(!mend||!hooks.mending||!hooks.mending_target_valid||!hooks.before_spell_effect||!hooks.target_scope)){hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;}
        const auto blessed=p.request.skill==297?effects::blessed_signet(p.effect_rank):std::nullopt;
        if(p.request.skill==297&&(!blessed||!hooks.blessed_signet||!hooks.blessed_signet_ready||!hooks.target_scope||!p.resurrection_scope||hooks.target_scope(actor_)!=p.resurrection_scope||!hooks.blessed_signet_ready(*p.resurrection_scope,now))){hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;}
        const auto signet=(p.request.skill==294||p.request.skill==2887)?effects::signet_judgment(p.effect_rank,p.pvp):std::nullopt;
        if((p.request.skill==294||p.request.skill==2887)&&(!signet||!hooks.signet_judgment||!hooks.signet_judgment_ready||!hooks.signet_judgment_ready(now))){hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;}
        const auto shadows=p.request.skill==876?effects::signet_shadows(p.effect_rank):std::nullopt;
        const auto discord=(p.request.skill==817||p.request.skill==2863)?effects::discord(p.effect_rank,p.pvp):std::nullopt;
        const auto rift=p.request.skill==910?effects::spirit_rift(p.effect_rank):std::nullopt;
        const auto spear=p.request.skill==1130?effects::spear_light(p.effect_rank):std::nullopt;
        const auto holy=p.request.skill==262?std::nullopt:effects::holy_damage(signet?signet->skill:p.request.skill,p.effect_rank);
        const auto shadow=effects::shadow_damage(p.request.skill,p.effect_rank);
        const auto necrosis=p.request.skill==2103?effects::necrosis(p.effect_rank):std::nullopt;
        const auto energy_blast=p.request.skill==2193?effects::energy_blast(p.effect_rank):std::nullopt;
        const auto aneurysm=p.request.skill==2055?effects::aneurysm(p.effect_rank):std::nullopt;
        const auto ear_bite=p.request.skill==2213?effects::ear_bite(p.effect_rank):std::nullopt;
        const bool gale=p.request.skill==162;
        const auto feast=p.request.skill==151?effects::feast_of_corruption(p.effect_rank):std::nullopt;
        if(p.request.skill==938&&(!effects::defensive_ground_ward_plan(938,p.effect_rank)||
           !hooks.defensive_ward_completed||!hooks.before_spell_effect||!hooks.target_scope||!hooks.target_valid)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        const auto snow=p.request.skill==2222?effects::snow_storm(p.effect_rank):std::nullopt;
        if(p.request.skill==2222&&(!snow||!hooks.snow_storm_completed||!hooks.before_spell_effect||
           !hooks.target_scope||!hooks.projectile_target_valid)){
            if(hooks.diagnostic)hooks.diagnostic(2222,"snow_storm_completion_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        const auto glimmering=p.request.skill==227?effects::glimmering_mark(p.effect_rank):std::nullopt;
        if(p.request.skill==227&&(!glimmering||!hooks.glimmering_mark_completed||!hooks.glimmering_targeted_use||
           !hooks.before_spell_effect||!hooks.target_scope||!hooks.projectile_target_valid)){
            if(hooks.diagnostic)hooks.diagnostic(227,"glimmering_completion_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        const auto wound=effects::wound_spell(p.pvp&&p.request.skill==1052?3184:p.request.skill,p.effect_rank);
        if(wound&&(!hooks.target_scope||!hooks.projectile_target_valid||
           (p.wound_spell_completion?(!hooks.wound_spell_completed||!hooks.before_spell_effect):!hooks.wound_spell))){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"wound_spell_completion_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.dagger_staged&&(!hooks.complete_dagger_attack||!p.attack_lease||!p.attack_lease->permit)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(49,actor_)});return;
        }
        const auto melee=combat::weapon::attack_skill(p.pvp&&p.request.skill==1191?2864:p.pvp&&p.request.skill==1720?3147:p.request.skill,p.effect_rank,p.strength,p.request.copy,p.dagger_staged);
        const auto water=(p.request.skill==214||p.request.skill==237)?effects::water_projectile(p.request.skill,p.effect_rank):std::nullopt;
        const auto movement=effects::movement_hex(p.request.skill,p.effect_rank,p.pvp);
        const auto hex=effects::degeneration_hex(p.request.skill,p.effect_rank,p.pvp);
        const auto energy_hex=effects::energy_degeneration_hex(p.request.skill,p.effect_rank);
        const auto action_hex=effects::action_damage_hex(p.request.skill,p.effect_rank);
        const auto spirit=p.request.skill==245?effects::protective_spirit(p.effect_rank):std::nullopt;
        const auto aegis=p.request.skill==257?effects::aegis(p.effect_rank):std::nullopt;
        if(p.request.skill==257&&(p.pvp||!aegis||!hooks.aegis||!hooks.before_spell_effect||!hooks.target_scope)){hooks.events({terminal(46,actor_)});return;}
        const auto vigorous=p.request.skill==254?effects::vigorous_spirit(p.effect_rank):std::nullopt;
        const auto seed=p.request.skill==274?effects::healing_seed(p.effect_rank):std::nullopt;
        const auto reversal=p.request.skill==307?effects::reversal_fortune(p.effect_rank):std::nullopt;
        const auto deflection=p.request.skill==259?effects::shield_deflection(p.effect_rank):std::nullopt;
        const auto regeneration_shield=p.request.skill==261?effects::shield_regeneration(p.effect_rank):std::nullopt;
        const auto judgment=p.request.skill==262?effects::shield_judgment(p.effect_rank):std::nullopt;
        const auto watchful=p.request.skill==1392?effects::watchful_healing(p.effect_rank):std::nullopt;
        const auto restful=p.request.skill==886?regeneration::reviewed(886,p.effect_rank):std::nullopt;
        std::optional<Milliseconds> local_duration;
        if(p.observe_local_duration){
            const auto base=local_enchantment_base(p.request.skill,p.effect_rank);
            if(base&&hooks.local_enchantment_duration)local_duration=hooks.local_enchantment_duration(p.request.skill,*base,now);
            if(!local_duration||!*local_duration||now>UINT64_MAX-*local_duration){
                hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
            }
        }
        if(p.observe_duration){const auto duration_plan=regeneration::reviewed(p.request.skill,p.effect_rank);
            const auto duration_energy=effects::energy_enchantment(p.request.skill,p.effect_rank);
            const auto duration_tainted=p.request.skill==113?effects::tainted_flesh(p.effect_rank):std::nullopt;
            const auto duration_spirit=p.request.skill==245?effects::protective_spirit(p.effect_rank):std::nullopt;
            const auto duration_aegis=p.request.skill==257?effects::aegis(p.effect_rank):std::nullopt;
            const auto duration_vigorous=p.request.skill==254?effects::vigorous_spirit(p.effect_rank):std::nullopt;
            const auto duration_seed=p.request.skill==274?effects::healing_seed(p.effect_rank):std::nullopt;
        const auto duration_reversal=p.request.skill==307?effects::reversal_fortune(p.effect_rank):std::nullopt;
            const auto duration_deflection=p.request.skill==259?effects::shield_deflection(p.effect_rank):std::nullopt;
            const auto duration_shield_regeneration=p.request.skill==261?effects::shield_regeneration(p.effect_rank):std::nullopt;
            const auto duration_judgment=p.request.skill==262?effects::shield_judgment(p.effect_rank):std::nullopt;
            const auto duration_owned_self=(p.simple_enchantment_completion||p.armor_earth_completion)?owned_self_enchantment_base(p.request.skill,p.effect_rank):0;
            const auto duration_base=duration_owned_self?duration_owned_self:duration_tainted?duration_tainted->duration:duration_aegis?duration_aegis->duration:duration_plan?duration_plan->duration:duration_energy?duration_energy->duration:duration_spirit?duration_spirit->duration:duration_deflection?duration_deflection->duration:duration_shield_regeneration?duration_shield_regeneration->duration:duration_judgment?duration_judgment->duration:duration_vigorous?duration_vigorous->duration:duration_seed?duration_seed->duration:duration_reversal?duration_reversal->duration:0;
            if(!duration_base||!hooks.enchantment_duration_ready||!hooks.enchantment_duration_ready(p.request.skill,duration_base,now)){
                hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
            }
        }
        const auto chilblains=p.request.skill==144?effects::chilblains(p.effect_rank):std::nullopt;
        const auto subversion=p.request.skill==127?effects::mark_subversion(p.effect_rank):std::nullopt;
        const auto leech=p.request.skill==128?effects::soul_leech(p.effect_rank):std::nullopt;
        const auto prism=p.request.skill==903?effects::icy_prism(p.effect_rank):std::nullopt;
        const auto condition_damage=effects::condition_damage(p.request.skill,p.effect_rank);
        const auto empathy=effects::empathy(p.request.skill,p.effect_rank);
        const auto drain=effects::energy_drain(p.request.skill,p.effect_rank);
        const auto gain=effects::resource_gain(p.request.skill,p.effect_rank);
        // NPC Flare targets have reviewed health in the world, but no player
        // Resources/energy profile. Lifetime and the authoritative projectile
        // guard establish their live target state; do not invent NPC energy.
        const bool foe_plan=bool(snow||glimmering||slippery||p.request.skill==2353||p.request.skill==2358||p.request.skill==2359||iron||disrupting||ear_bite||energy_blast||aneurysm||necrosis||p.request.skill==1363||p.request.skill==1950||p.request.skill==2093||p.request.skill==934||p.request.skill==3188||daggers||dancing||burn||water||fevered||wandering||mind||balance||diversion||cry||panic||humility||feast||interrupt||barbs||backfire||spiteful||holy||spear||rift||discord||shadows||delusions||disenchant||movement||(shatter&&!shatter->ally)||faint||swarm||javelin||storm||ray||condition_damage||hex||energy_hex||action_hex||prism||leech||subversion||chilblains||wound||gale||p.request.skill==952||p.request.skill==1085||p.request.skill==1336||shadow||lightning||blindness||transfer||condition||virulence||rotting_flesh||steal||drain||empathy||(gain&&effects::gain_targets_foe(*gain))||(elemental&&!elemental->caster_center));const float foe_range=slippery?slippery->range:(p.request.skill==2353||p.request.skill==2358||p.request.skill==2359)?1248.f:iron?iron->range:disrupting?disrupting->range:ear_bite?ear_bite->range:energy_blast?energy_blast->range:dancing?dancing->range:aneurysm?aneurysm->range:necrosis?necrosis->range:(p.request.skill==1363||p.request.skill==1950||p.request.skill==2093||p.request.skill==934||p.request.skill==3188||daggers||dancing||burn||water||fevered||wandering||mind||balance||diversion||cry||panic)?1248.f:humility?1248.f:feast?feast->range:interrupt?interrupt->range:barbs?barbs->range:backfire?backfire->range:spiteful?spiteful->range:(spear||rift||discord||shadows)?1248.f:holy?holy->range:delusions?delusions->range:disenchant?disenchant->range:movement?movement->range:shatter?shatter->range:faint?faint->range:swarm?swarm->range:javelin?javelin->range:condition_damage?condition_damage->range:hex?hex->range:energy_hex?energy_hex->range:action_hex?action_hex->range:prism?prism->range:leech?leech->range:subversion?subversion->range:chilblains?chilblains->range:snow?snow->range:glimmering?glimmering->range:wound?wound->range:gale?1248.f:p.request.skill==952?1248.f:p.request.skill==1085?1248.f:p.request.skill==1336?1248.f:shadow?shadow->range:lightning?lightning->range:blindness?blindness->range:transfer?transfer->range:condition?condition->range:rotting_flesh?rotting_flesh->range:virulence?virulence->range:steal?steal->range:drain?drain->range:gain?gain->range:empathy?1248:elemental?elemental->range:storm?1248:ray?ray->range:0;
        if(chilblains&&(!hooks.chilblains||!hooks.before_spell_effect||!hooks.projectile_target_valid||!hooks.target_scope)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(vigorous&&(!hooks.vigorous_spirit||!hooks.before_spell_effect||!hooks.target_scope||!hooks.ally_enchantment_target_valid)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(reversal&&(!hooks.reversal_fortune_completed||!hooks.before_spell_effect||!hooks.target_scope||!hooks.reversal_target_valid||!hooks.enchantment_duration_ready)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(seed&&(!hooks.healing_seed||!hooks.before_spell_effect||!hooks.target_scope||!hooks.healing_seed_target_valid)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(deflection&&(!hooks.shield_deflection||!hooks.before_spell_effect||!hooks.target_scope||!hooks.ally_enchantment_target_valid)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(regeneration_shield&&(!hooks.shield_regeneration||!hooks.before_spell_effect||!hooks.target_scope||!hooks.ally_enchantment_target_valid)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(judgment&&(!hooks.shield_judgment||!hooks.shield_judgment_ready||!hooks.shield_judgment_ready(now)||!hooks.before_spell_effect||!hooks.target_scope||!hooks.ally_enchantment_target_valid)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(spirit&&(!hooks.protective_spirit||!hooks.before_spell_effect||!hooks.target_scope||!hooks.ally_enchantment_target_valid)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(watchful&&(!hooks.watchful_healing||!hooks.before_spell_effect||!hooks.target_scope||!hooks.ally_enchantment_target_valid)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(restful&&(!hooks.restful_breeze||!hooks.before_spell_effect||!hooks.target_scope||!hooks.ally_enchantment_target_valid)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.request.skill==298&&(!hooks.martyr||!hooks.before_spell_effect||!hooks.target_scope||!hooks.target_valid)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.request.skill==303&&(!hooks.convert_hexes||!hooks.before_spell_effect||!hooks.target_scope||!hooks.ally_enchantment_target_valid)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(retribution&&(!hooks.retribution_completed||!hooks.retribution_target_valid||!hooks.before_spell_effect||!hooks.target_scope)){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(seed&&p.request.target==actor_){
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        const bool valid_target=p.pet_charm?
            p.use_owner&&hooks.target_scope(actor_)==p.use_owner&&hooks.target_scope(p.request.target)==p.pet_target:
            p.request.skill==436?p.use_owner&&p.pet_target&&hooks.target_scope(actor_)==p.use_owner&&
                hooks.target_scope(p.pet_target->actor)==p.pet_target&&hooks.pet_comfort_ready(*p.use_owner,*p.pet_target,now):p.request.skill==97?
            p.use_owner&&hooks.target_scope&&hooks.target_scope(actor_)==p.use_owner&&
            bool(effects::necrotic_traversal(p.effect_rank))&&bool(hooks.necrotic_traversal)&&bool(hooks.before_spell_effect):p.status_heal_completion?p.resurrection_scope&&hooks.target_scope(p.request.target)==p.resurrection_scope&&hooks.status_heal_target_valid(*p.resurrection_scope,p.request.skill,now):p.direct_heal_completion?p.resurrection_scope&&hooks.target_scope(p.request.target)==p.resurrection_scope&&hooks.direct_heal_target_valid(*p.resurrection_scope,p.request.skill,now):weapon_spell?p.resurrection_scope&&hooks.weapon_spell_target_valid(*p.resurrection_scope,*weapon_spell,now):(p.request.skill==303||mimicry_cast||recall_cast||mend||succor||retribution||watchful||restful||spirit||vigorous||seed||reversal||deflection||regeneration_shield||judgment)?p.resurrection_scope&&hooks.target_scope(p.request.target)==p.resurrection_scope&&(reversal?hooks.reversal_target_valid(*p.resurrection_scope,p.divine_rank,now):seed?hooks.healing_seed_target_valid(*p.resurrection_scope,p.divine_rank,now):mimicry_cast?hooks.mimicry_target_valid({p.request.skill,p.request.copy},*p.resurrection_scope,now):recall_cast?hooks.recall_target_valid(*p.resurrection_scope,now):retribution?(hooks.retribution_completed&&hooks.retribution_target_valid&&hooks.before_spell_effect&&hooks.retribution_target_valid(*p.resurrection_scope,p.divine_rank,now)):mend?hooks.mending_target_valid(*p.resurrection_scope,p.divine_rank,now):succor?hooks.succor_target_valid(*p.resurrection_scope,p.divine_rank,now):hooks.ally_enchantment_target_valid(*p.resurrection_scope,p.divine_rank,now)):tainted_flesh?p.resurrection_scope&&hooks.target_scope(p.request.target)==p.resurrection_scope&&hooks.tainted_flesh_target_valid(p.request.target,*tainted_flesh,now):energy_enchantment?p.resurrection_scope&&hooks.target_scope(p.request.target)==p.resurrection_scope&&hooks.energy_enchantment_target_valid(p.request.target,*energy_enchantment,now):p.request.skill==212?p.resurrection_scope&&hooks.target_scope(actor_)==p.resurrection_scope:(p.request.skill==427||p.request.skill==182||p.request.skill==207||p.request.skill==221||p.request.skill==298||p.request.skill==943||p.request.skill==1262||p.request.skill==82||p.request.skill==257||(p.request.skill==165||p.request.skill==1381))?p.resurrection_scope&&hooks.target_scope(actor_)==p.resurrection_scope:inspired?p.resurrection_scope&&hooks.target_scope(p.request.target)==p.resurrection_scope&&(inspired->allied?(hooks.target_valid&&hooks.target_valid(p.request.target,inspired->range,true)):hooks.projectile_target_valid(p.request.target,inspired->range)):p.request.skill==10?p.resurrection_scope&&hooks.target_scope(actor_)==p.resurrection_scope:feast?p.feast_anchor&&p.resurrection_scope&&p.feast_anchor->selected==*p.resurrection_scope&&hooks.target_scope(actor_)==p.feast_anchor->caster&&hooks.target_scope(p.request.target)==p.resurrection_scope:shatter&&shatter->ally?p.resurrection_scope&&hooks.target_scope(p.request.target)==p.resurrection_scope&&hooks.shatter_ally_valid(p.request.target,shatter->range,now):(p.request.skill==292||p.request.skill==1526)?hooks.infuse_target_valid(p.request.target,now):p.request.skill==952?p.resurrection_scope&&hooks.target_scope(p.request.target)==p.resurrection_scope&&effects::death_charge(p.effect_rank)&&hooks.death_charge_target_valid(*p.resurrection_scope,*effects::death_charge(p.effect_rank),now):melee?(!target||target->health)&&p.weapon&&p.resurrection_scope&&hooks.target_scope(p.request.target)==p.resurrection_scope&&(hooks.attack_skill_target?hooks.attack_skill_target(p.request.target,p.request.skill,now):hooks.melee_target(p.request.target,now)):cleansing?target&&target->health&&p.resurrection_scope&&hooks.target_scope(p.request.target)==p.resurrection_scope&&(hooks.target_valid&&hooks.target_valid(p.request.target,cleansing->range,cleansing->self)):elemental&&elemental->caster_center?target&&target->health&&p.resurrection_scope&&hooks.target_scope(actor_)==p.resurrection_scope:resurrection?target&&!target->health&&p.resurrection_scope&&hooks.target_scope(p.request.target)==p.resurrection_scope&&hooks.resurrection_target_valid(p.request.target,resurrection->range):elemental&&elemental->skill==189&&hooks.rodgort_target_valid?p.resurrection_scope&&hooks.target_scope(p.request.target)==p.resurrection_scope&&hooks.rodgort_target_valid(*p.resurrection_scope,now):elemental&&elemental->skill==189&&target&&!target->health?p.resurrection_scope&&hooks.target_scope(p.request.target)==p.resurrection_scope:foe_plan?((target&&(target->health||p.request.skill==187))||(!target&&(p.request.skill==2353||p.request.skill==2358||p.request.skill==2359||p.request.skill==1363||p.request.skill==1950||p.request.skill==2093||p.request.skill==934||p.request.skill==3188||p.request.skill==2056||iron||daggers||dancing||disrupting||burn||interrupt||fevered||mind||balance||diversion||cry||panic||blindness||transfer||barbs||backfire||spiteful||gale||p.request.skill==1085||p.request.skill==2191||shadow||holy||spear||rift||discord||shadows||delusions||disenchant||water||movement||shatter||(lightning&&(lightning->skill==1082||lightning->skill==226||lightning->skill==2804||lightning->skill==222||lightning->skill==224||lightning->skill==865||lightning->skill==3396||lightning->skill==1369||lightning->skill==229||lightning->skill==232||lightning->skill==2059||lightning->skill==1664||lightning->skill==228||lightning->skill==205))||virulence||rotting_flesh||(condition&&!condition->spread)||(hex&&(hex->skill==883||hex->skill==785||hex->skill==99||hex->skill==103||hex->skill==123||hex->skill==150||hex->skill==101||hex->skill==137||hex->skill==43||hex->skill==47||hex->skill==66||hex->skill==19||hex->skill==2998||hex->skill==31||hex->skill==35||hex->skill==56||hex->skill==235||hex->skill==36||hex->skill==53||hex->skill==3183||hex->skill==859||hex->skill==2058||hex->skill==109||hex->skill==108||hex->skill==129||hex->skill==142))||energy_hex||action_hex||prism||leech||subversion||chilblains||bool(snow)||bool(glimmering)||bool(wound)||steal||faint||swarm||javelin||storm||ray||gain||empathy||bool(drain)||(elemental&&(elemental->skill==194||elemental->skill==186||(!elemental->projectile&&!elemental->caster_center))))))&&p.resurrection_scope&&hooks.target_scope(p.request.target)==p.resurrection_scope&&(p.request.skill==187||((p.request.skill==365||p.request.skill==1141)?hooks.shout_target_valid&&hooks.shout_target_valid(p.request.target,foe_range):hooks.projectile_target_valid(p.request.target,foe_range))):p.request.skill==1121?p.resurrection_scope&&hooks.gift_health_target_valid(*p.resurrection_scope,now):p.request.skill==1234?p.resurrection_scope&&hooks.mend_body_soul_target_valid(*p.resurrection_scope,now):p.request.skill==1741?p.resurrection_scope&&hooks.ghostmirror_light_target_valid(*p.resurrection_scope,now):p.request.skill==2062?p.resurrection_scope&&hooks.healing_ribbon_target_valid(*p.resurrection_scope,now):p.request.skill==887?p.resurrection_scope&&hooks.rejuvenation_target_valid(*p.resurrection_scope,now):p.request.skill==915?p.resurrection_scope&&hooks.spirit_light_target_valid(*p.resurrection_scope,now):p.request.skill==1687?p.resurrection_scope&&hooks.zealous_target_valid(p.request.target,p.range,now):target&&target->health&&(effects::earth_projectile(p.request.skill,0,0)?hooks.projectile_target_valid(p.request.target,p.range):(hooks.target_valid&&hooks.target_valid(p.request.target,p.range,p.allow_self)));
        const bool scoped_heal=p.direct_heal_completion||bool(effects::direct_heal(p.request.skill)&&hooks.heal_scoped);
        if(!caster||!caster->health||!valid_target||(p.request.skill==475&&(!p.use_owner||hooks.target_scope(actor_)!=p.use_owner))||(balance&&(!p.balance_caster||hooks.target_scope(actor_)!=p.balance_caster))||(gain&&(!p.resurrection_scope||hooks.target_scope(p.request.target)!=p.resurrection_scope))||((scoped_heal||p.request.skill==1121||p.request.skill==2062||p.request.skill==887)&&(!p.resurrection_scope||!hooks.target_scope||hooks.target_scope(p.request.target)!=p.resurrection_scope))||((p.request.skill==292||p.request.skill==1526||p.request.skill==258||p.request.skill==299||p.request.skill==2061||p.request.skill==1392||p.request.skill==886||(p.request.skill==288&&hooks.protective_enchantment)||p.request.skill==1691)&&(!p.resurrection_scope||hooks.target_scope(p.request.target)!=p.resurrection_scope))){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"target_lost");
            if(p.weapon)hooks.events({combat::weapon::stop_2022(actor_)});
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(p.weapon?49:59,actor_)});return;
        }
        std::optional<std::uint64_t> balance_reaction_epoch;
        std::uint64_t spell_effect_event=0;
        if(p.simple_enchantment_completion&&(!hooks.simple_enchantment_completed||!hooks.before_spell_effect||(p.request.skill==2138&&!hooks.completed_action))){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"simple_enchantment_completion_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.request.skill==165&&((p.armor_earth_completion?!hooks.armor_earth_completed:!hooks.armor_earth)||!hooks.before_spell_effect)){
            if(hooks.diagnostic)hooks.diagnostic(165,"armor_earth_completion_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(condition&&((!hooks.condition_spell&&!hooks.condition_spell_completed)||
           (hooks.condition_spell_completed&&effects::is_spell(*skill)&&!hooks.before_spell_effect))){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"condition_completion_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(p.request.skill==883&&(!hooks.degeneration_hex_completed||!hooks.before_spell_effect)){
            if(hooks.diagnostic)hooks.diagnostic(883,"control_hex_completion_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(subversion&&(!hooks.before_spell_effect||!hooks.targeted_spell_failure)){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"mark_subversion_reaction_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
        }
        if(leech&&!hooks.before_spell_effect){
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"soul_leech_reaction_hook_lost");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(p.weapon?49:59,actor_)});return;
        }
        if(effects::is_spell(*skill)&&hooks.before_spell_effect){
            const auto event=spell_activation_event();
            spell_effect_event=event;
            if(!event||!hooks.before_spell_effect(p.request.skill,event,now)){
                if(balance&&event)balance_reaction_epoch=hooks.take_grenths_reaction_death(event,now);
                if(!balance_reaction_epoch){
                if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"spell_reaction_blocked");
                // Normal death cleanup on update removes active effects;
                // cooldowns and spent signets survive death/resurrection.
                hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(p.weapon?49:59,actor_)});return;
                }
            }
            if(!balance_reaction_epoch&&hooks.targeted_spell_failure){
                const auto failure=hooks.targeted_spell_failure(p.request.skill,p.resurrection_scope,event,now);
                if(failure!=SpellFailure::none){
                    if(hooks.diagnostic)hooks.diagnostic(p.request.skill,failure==SpellFailure::failed?"targeted_spell_failed":"targeted_spell_rejected");
                    // Failure spends activation costs but adds no normal recharge.
                    // Existing independently disabled slots remain authoritative.
                    hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(p.weapon?49:59,actor_)});return;
                }
            }
            // Conditional self healing must see post-reaction health.
            if(p.request.target==actor_){
                target=hooks.resources(actor_);
                if(!target||!target->health){
                    if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"actor_lost_after_reaction");
                    hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(p.weapon?49:59,actor_)});return;
                }
            }
        }
        struct ChannelingGuard {
            const Hooks& hooks;Milliseconds now;std::uint64_t event=0;
            ~ChannelingGuard(){if(event)try{hooks.complete_channeling(event,false,now);}catch(...){}}
        } channeling{hooks,now};
        struct SkillUseGuard {
            const Hooks& hooks;std::uint64_t event=0;
            ~SkillUseGuard(){if(event)try{hooks.abort_skill_use(event);}catch(...){}}
        } used{hooks};
        if(!balance_reaction_epoch&&hooks.prepare_skill_use){
            const auto event=spell_activation_event();
            if(!event||!hooks.prepare_skill_use({p.request.skill,p.request.copy},event,now)){
                if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"skill_use_prepare_failed");
                hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(p.weapon?49:59,actor_)});return;
            }
            used.event=event;
        }
        if(!balance_reaction_epoch&&effects::is_spell(*skill)&&hooks.prepare_channeling){
            const auto event=spell_activation_event();
            if(!event||!hooks.prepare_channeling(p.request.skill,event,now)){
                if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"spell_observer_prepare_failed");
                hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;
            }
            channeling.event=event;
        }
        const auto completed_use_event=used.event?used.event:(p.observe_glimmering||hooks.prepare_demonic_flesh_use||hooks.panic_skill_used||hooks.mind_wrack_targeted||hooks.keystone_skill_used)?spell_activation_event():0;
        struct DemonicUseGuard {
            const Hooks& hooks;std::optional<effects::DemonicFleshReservation> receipt;
            ~DemonicUseGuard(){if(receipt)try{hooks.cancel_demonic_flesh_use(*receipt);}catch(...){}}
        } demonic_use{hooks};
        if(p.observe_demonic&&!balance_reaction_epoch){
            if(completed_use_event&&p.use_owner&&p.use_target)
                demonic_use.receipt=hooks.prepare_demonic_flesh_use({*p.use_owner,*p.use_target,p.request.skill,completed_use_event,now,true,false});
            if(!demonic_use.receipt){
                if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"demonic_flesh_prepare_failed");
                hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(p.weapon?49:59,actor_)});return;
            }
        }
        // Capture the old enchantment before applying/renewing this cast.
        // The first Aura cannot trigger itself; renewing a still-active Aura can.
        struct CompletionPrefixExit {
            std::optional<CompletionPublication> lease;
            void prepare(){if(lease&&lease->prepare_prefix)lease->prepare_prefix();}
            void publish(){if(lease&&lease->publish_prefix)lease->publish_prefix();}
            ~CompletionPrefixExit(){if(lease&&lease->finish)lease->finish();}
        } completion_prefix_scope;
        if(hooks.prepare_completion_publication){
            const auto owner=p.use_owner?p.use_owner:(hooks.target_scope?hooks.target_scope(actor_):std::nullopt);
            if(owner)completion_prefix_scope.lease=hooks.prepare_completion_publication(*owner,now);
            if(!completion_prefix_scope.lease){hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(p.weapon?49:59,actor_)});return;}
        }
        std::optional<effects::AuraRestoration> aura_trigger;
        if(effects::is_spell(*skill)&&hooks.effect_snapshot){
            const auto snapshot=hooks.effect_snapshot(now);
            if(!snapshot){if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"aura_observer_unavailable");
                hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(59,actor_)});return;}
            const effects::Entry* aura=nullptr;
            for(const auto& entry:*snapshot)if((entry.skill==180||entry.skill==3375)&&entry.cause==actor_&&
                entry.kind==effects::Category::enchantment&&entry.applied<=now&&(!entry.expires||now<*entry.expires)&&
                (!aura||entry.applied>aura->applied||(entry.applied==aura->applied&&entry.instance>aura->instance)))aura=&entry;
            if(aura)aura_trigger=effects::aura_restoration(*skill,aura->rank,aura->skill==3375);
        }else if(const auto aura=effects_.find(180);aura!=effects_.end()&&now<aura->second.ends)
            aura_trigger=effects::aura_restoration(*skill,aura->second.rank,p.pvp);
        // A completed direct-target use ends the caster's old Mark before the
        // new effect samples hex counts. Canceled/failed validation never gets
        // here; a hook added after admission cannot upgrade an older cast.
        if(p.observe_glimmering){
            if(!completed_use_event||!p.use_owner||!p.use_target||
               !hooks.glimmering_targeted_use({*p.use_owner,*p.use_target,p.request.skill,completed_use_event,now})){
                if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"glimmering_completed_use_refused");
                hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(p.weapon?49:59,actor_)});return;
            }
        }
        const auto haste=effects::illusion_haste(p.request.skill,p.effect_rank,p.pvp);
        bool applied=false,fixed_recharge=false,mimicry_success=false,dagger_prerequisite_failed=false,dagger_dual_failure_deferred=false,dagger_dual_success_deferred=false;unsigned recharge_seconds=skill->recharge;double intrinsic_recharge=1.;const char* success_phase="applied";
        std::optional<Milliseconds> atomic_attack_recharge;
        std::vector<Packet> completion_visuals;
        std::function<void()> completion_publication;
        auto completion_snapshot_at=now;
        bool early_completion_visuals=false;
        bool deferred_gift_health=false;
        bool deferred_spirit_light=false;
        bool deferred_mend_body_soul=false;
        bool deferred_ghostmirror_light=false;
        bool deferred_healing_ribbon=false;
        bool deferred_rejuvenation=false,deferred_stoneflesh=false;
        bool completion_prefix_sent=false;
        std::optional<sacrifice::Loss> completed_sacrifice;
        bool deferred_aneurysm=false,deferred_energy_blast=false,deferred_holy=false,deferred_hex=false,deferred_lightning=false,deferred_elemental=false,deferred_movement=false,deferred_necrosis=false,deferred_virulence=false,deferred_rotting_flesh=false,deferred_energy_enchantment=false,deferred_tainted_flesh=false,deferred_spiteful=false,deferred_life_steal=false,deferred_energy_drain=false;
        const bool deferred_copy=inspired&&!inspired->allied;
        if(p.request.skill==1121){
            const auto plan=effects::gift_health(p.effect_rank);
            auto result=plan&&p.resurrection_scope&&used.event?(p.gift_health_completion?hooks.gift_health_completed(*p.resurrection_scope,*plan,p.divine_rank,used.event,spell_effect_event,now):hooks.gift_health(*p.resurrection_scope,*plan,p.divine_rank,used.event,now)):EffectCompletion{};
            applied=result.accepted;
            if(applied){completion_publication=std::move(result.publication);deferred_gift_health=true;}
        }else if(p.request.skill==915||p.request.skill==1234||p.request.skill==1741||p.request.skill==2062||p.request.skill==887){
            if(p.request.skill==915){
                const auto plan=effects::spirit_light(p.effect_rank);
                auto result=plan&&p.resurrection_scope?
                    (p.spirit_light_completion?hooks.spirit_light_completed(*p.resurrection_scope,*plan,spell_effect_event,now):hooks.spirit_light(*p.resurrection_scope,*plan,now)):EffectCompletion{};
                applied=result.accepted&&bool(result.paid_sacrifice);
                if(applied){
                    completed_sacrifice=result.paid_sacrifice;
                    completion_publication=std::move(result.publication);
                    completion_visuals=std::move(result.visuals);
                    deferred_spirit_light=true;
                }
            }else if(p.request.skill==1234){
                const auto plan=effects::mend_body_soul(p.effect_rank);
                auto result=plan&&p.resurrection_scope?
                    (p.mend_body_soul_completion?hooks.mend_body_soul_completed(*p.resurrection_scope,*plan,spell_effect_event,now):hooks.mend_body_soul(*p.resurrection_scope,*plan,now)):EffectCompletion{};
                applied=result.accepted;
                if(applied){
                    completion_publication=std::move(result.publication);
                    deferred_mend_body_soul=true;
                }
            }else if(p.request.skill==1741){
                const auto plan=effects::ghostmirror_light(p.effect_rank);
                auto result=plan&&p.resurrection_scope?
                    (p.ghostmirror_light_completion?hooks.ghostmirror_light_completed(*p.resurrection_scope,*plan,spell_effect_event,now):hooks.ghostmirror_light(*p.resurrection_scope,*plan,now)):EffectCompletion{};
                applied=result.accepted;
                if(applied){
                    completion_publication=std::move(result.publication);
                    completion_visuals=std::move(result.visuals);
                    deferred_ghostmirror_light=true;
                }
            }else if(p.request.skill==2062){
                const auto plan=effects::healing_ribbon(p.effect_rank);
                auto result=plan&&p.resurrection_scope?
                    (p.healing_ribbon_completion?hooks.healing_ribbon_completed(*p.resurrection_scope,*plan,p.divine_rank,spell_effect_event,now):hooks.healing_ribbon(*p.resurrection_scope,*plan,p.divine_rank,now)):EffectCompletion{};
                applied=result.accepted;
                if(applied){
                    completion_publication=std::move(result.publication);
                    completion_visuals=std::move(result.visuals);
                    deferred_healing_ribbon=true;
                }
            }else{
                const auto plan=effects::signet_rejuvenation(p.effect_rank);
                auto result=plan&&p.resurrection_scope?
                    hooks.rejuvenation(*p.resurrection_scope,*plan,now):EffectCompletion{};
                applied=result.accepted;
                if(applied){
                    completion_publication=std::move(result.publication);
                    completion_visuals=std::move(result.visuals);
                    deferred_rejuvenation=true;
                }
            }
        }else if(p.request.skill==934||p.request.skill==3188||p.request.skill==1950||p.request.skill==2093){
            if(p.request.skill==1950||p.request.skill==2093){
                const auto plan=effects::signet_of_corruption(p.request.skill,p.effect_rank);
                applied=plan&&p.resurrection_scope&&hooks.signet_of_corruption&&hooks.signet_of_corruption(*p.resurrection_scope,*plan,now);
            }else{
                const auto plan=effects::unnatural_signet(p.effect_rank,p.pvp);
                applied=plan&&p.resurrection_scope&&hooks.unnatural_signet&&hooks.unnatural_signet(*p.resurrection_scope,*plan,used.event,now);
            }
        }else if(p.request.skill==303){
            const auto plan=effects::convert_hexes(p.effect_rank);
            applied=plan&&p.use_owner&&p.resurrection_scope&&hooks.convert_hexes&&hooks.convert_hexes(*p.use_owner,*p.resurrection_scope,*plan,p.divine_rank,now);
            completion_prefix_sent=applied;
        }else if(p.request.skill==318||p.request.skill==3204){
            const auto plan=effects::defy_pain(p.effect_rank,p.pvp);
            if(plan&&p.use_owner&&hooks.defy_pain){auto result=hooks.defy_pain(*p.use_owner,*plan,now);
                applied=result.accepted;completion_publication=std::move(result.publication);
                if(applied&&plan->stance){
                    // World replaced and published the previous stance. Retire
                    // only its local mechanics; do not send a duplicate67.
                    std::erase_if(effects_,[&](const auto& effect){const auto* row=find_skill(effect.first);
                        if(!row||row->type!=3)return false;
                        if(effect.first==1043&&hooks.speed_multiplier)hooks.speed_multiplier(1.0);
                        return true;});
                }
            }
        }else if(p.request.skill==347){
            const auto plan=effects::endure_pain(p.effect_rank);
            if(plan&&p.use_owner&&hooks.endure_pain){auto result=hooks.endure_pain(*p.use_owner,*plan,now);
                applied=result.accepted;completion_publication=std::move(result.publication);}
        }else if(p.request.skill==1411){
            const auto plan=effects::stamina_signet(p.effect_rank);
            if(plan&&p.use_owner&&hooks.stamina_signet){auto result=hooks.stamina_signet(*p.use_owner,*plan,now);
                applied=result.accepted;completion_publication=std::move(result.publication);}
        }else if(p.request.skill==364){
            const auto plan=effects::charge(p.effect_rank);
            if(plan&&p.use_owner&&hooks.charge){auto result=hooks.charge(*p.use_owner,*plan,now);
                applied=result.accepted;completion_publication=std::move(result.publication);}
        }else if(p.request.skill==348||p.request.skill==2858){
            const auto plan=effects::watch_yourself(p.effect_rank,p.pvp);
            if(plan&&p.use_owner&&hooks.watch_yourself){auto result=hooks.watch_yourself(*p.use_owner,*plan,now);
                applied=result.accepted;completion_publication=std::move(result.publication);}
        }else if(p.request.skill==367||p.request.skill==2216||p.request.skill==475||p.request.skill==1954||p.request.skill==2097){
            EffectCompletion result;
            if(p.request.skill==367){
                const auto plan=effects::shields_up(p.effect_rank);
                if(plan&&p.use_owner&&hooks.shields_up)result=hooks.shields_up(*p.use_owner,*plan,now);
            }else if(p.request.skill==2216){
                const auto plan=effects::dont_trip(p.effect_rank);
                if(plan&&p.use_owner&&hooks.dont_trip)result=hooks.dont_trip(*p.use_owner,*plan,now);
            }else if(p.request.skill==475){
                const auto plan=effects::quickening_zephyr(p.effect_rank);
                if(plan&&p.use_owner&&hooks.quickening_zephyr_spawn)result=hooks.quickening_zephyr_spawn(*p.use_owner,*plan,now);
            }else{
                const auto plan=effects::save_yourselves(p.request.skill,p.effect_rank);
                if(plan&&p.use_owner&&hooks.save_yourselves)result=hooks.save_yourselves(*p.use_owner,*plan,now);
            }
            applied=result.accepted;
            completion_visuals=std::move(result.visuals);
            completion_publication=std::move(result.publication);
        }else if(p.request.skill==316){
            const auto plan=effects::to_limit(p.effect_rank);
            applied=plan&&p.use_owner&&hooks.to_limit&&hooks.to_limit(*p.use_owner,*plan,now);
        }else if(p.request.skill==343||p.request.skill==2883){
            const auto plan=effects::for_great_justice(p.request.skill,p.pvp);
            applied=plan&&p.use_owner&&hooks.for_great_justice&&hooks.for_great_justice(*p.use_owner,*plan,now);
        }else if(p.request.skill==368||p.request.skill==2354||p.request.skill==2355||p.request.skill==2356){
            if(p.request.skill==2354){const auto plan=effects::dodge_this(p.effect_rank);
                applied=plan&&p.use_owner&&hooks.dodge_this&&hooks.dodge_this(*p.use_owner,*plan,now);
            }else if(p.request.skill==2355){const auto plan=effects::i_am_the_strongest(p.effect_rank);
                applied=plan&&p.use_owner&&hooks.i_am_the_strongest&&hooks.i_am_the_strongest(*p.use_owner,*plan,now);
            }else if(p.request.skill==368){const auto plan=effects::i_will_survive(p.effect_rank);
                applied=plan&&p.use_owner&&hooks.i_will_survive&&hooks.i_will_survive(*p.use_owner,*plan,now);
            }else{const auto plan=effects::i_am_unstoppable(p.effect_rank);
                applied=plan&&p.use_owner&&hooks.i_am_unstoppable&&hooks.i_am_unstoppable(*p.use_owner,*plan,now);}
        }else if(p.request.skill==389){
            const auto plan=effects::flourish(p.effect_rank);
            const auto permit=p.use_owner&&hooks.prepare_flourish_reset?hooks.prepare_flourish_reset(*p.use_owner,now):std::nullopt;
            if(plan&&permit&&used.event)applied=reset_flourish(*plan,*permit,now,used.event,hooks);
        }else if(p.request.skill==63){
            const auto plan=effects::keystone_signet(p.effect_rank);
            const auto permit=p.use_owner&&hooks.prepare_keystone_reset?hooks.prepare_keystone_reset(*p.use_owner):std::nullopt;
            const auto bar=hooks.current_skillbar?hooks.current_skillbar():std::nullopt;const auto event=used.event?used.event:spell_activation_event();
            if(plan&&permit&&bar&&event&&hooks.commit_keystone_reset){std::vector<Slot> eligible;for(const auto slot:*bar)if(slot.skill)eligible.push_back(slot);
                applied=reset_other_signets(*p.use_owner,eligible,now,event,[&](const auto& cleared){return hooks.commit_keystone_reset(*p.use_owner,*plan,cleared,*permit,event,now);},hooks);
            }
        }else if(fevered){
            applied=hooks.fevered_dreams&&hooks.fevered_dreams(*p.resurrection_scope,*fevered,now);
        }else if(mind||wandering){
            applied=mind?(hooks.mind_wrack&&hooks.mind_wrack(*p.resurrection_scope,*mind,now)):
                (hooks.wandering_eye&&p.resurrection_scope&&hooks.wandering_eye(*p.resurrection_scope,*wandering,now));
        }else if(panic){
            applied=hooks.panic(*p.resurrection_scope,p.effect_rank,now);
        }else if(p.request.skill==165||p.request.skill==1381){
            if(p.request.skill==165)applied=p.armor_earth_completion?
                hooks.armor_earth_completed(*p.resurrection_scope,p.effect_rank,spell_effect_event,now):hooks.armor_earth(*p.resurrection_scope,p.effect_rank,now);
            else{
                const auto plan=effects::flame_djinn_haste(p.effect_rank);
                const auto result=plan&&p.resurrection_scope&&spell_effect_event?
                    hooks.flame_djinn_haste(*p.resurrection_scope,*plan,spell_effect_event,now):std::nullopt;
                applied=result.has_value();
                if(applied){completion_prefix_sent=true;if(*result)intrinsic_recharge=.5;}
            }
        }else if(p.request.skill==82&&!p.simple_enchantment_completion){
            applied=hooks.mantra_recall(*p.resurrection_scope,p.effect_rank,now);
        }else if(p.request.skill==1262){
            const auto plan=effects::healing_ring(p.effect_rank,p.divine_rank);
            if(p.healing_ring_completion){
                auto result=plan&&spell_effect_event?hooks.healing_ring_completed(*p.resurrection_scope,*plan,spell_effect_event,now):EffectCompletion{};
                applied=result.accepted;if(applied)completion_publication=std::move(result.publication);
            }else applied=plan&&hooks.healing_ring(*p.resurrection_scope,*plan,now);
        }else if(const auto conjure=effects::conjure(p.request.skill,p.effect_rank);conjure&&!p.simple_enchantment_completion){
            applied=hooks.conjure_enchantment(*p.resurrection_scope,*conjure,now);
        }else if(p.request.skill==298){
            const auto plan=effects::martyr();
            applied=plan&&p.use_owner&&hooks.martyr&&hooks.martyr(*p.use_owner,*plan,p.divine_rank,now);
            completion_prefix_sent=applied;
        }else if(p.request.skill==943){
            const auto plan=effects::extinguish(p.effect_rank,p.divine_rank);
            applied=plan&&hooks.extinguish(*p.resurrection_scope,*plan,now);
            // One native target-body publication;the captured duplicate has
            // unresolved provenance and is not a universal per-cast rule.
            if(applied)if(const auto body=particles::target_body_2022(actor_,943))hooks.events({*body});
        }else if(p.request.skill==427){
            applied=hooks.antidote_signet(*p.resurrection_scope,now);
        }else if(cry){
            {auto result=hooks.cry_frustration(*p.resurrection_scope,*cry,now);applied=result.accepted;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}}
        }else if(inspired){
            applied=hooks.inspired_hex(*p.resurrection_scope,*inspired,{p.request.skill,p.request.copy},now);
            if(applied&&!inspired->allied)if(const auto body=particles::targeted_body_2022(p.request.target,actor_,p.request.skill))completion_visuals.push_back(*body);
        }else if(balance){
            applied=hooks.grenths_balance(*p.resurrection_scope,balance_reaction_epoch,now);
        }else if(diversion){
            applied=hooks.diversion(*p.resurrection_scope,*diversion,now);
        }else if(melee){
            if(p.dagger_staged){
                auto result=hooks.complete_dagger_attack(*p.resurrection_scope,*p.weapon,*melee,p.attack_lease->permit,now,p.request.copy);
                applied=result.accepted;
                if(applied){
                    completion_publication=std::move(result.publication);
                    if(result.successful_second){
                        dagger_successful_second_draft_=std::move(result.successful_second);
                        dagger_successful_skill_draft_=p.request.skill;dagger_successful_copy_draft_=p.request.copy;
                        dagger_successful_control_draft_=hooks.control_epoch?hooks.control_epoch():std::nullopt;
                        dagger_successful_session_draft_=p.session_admission;
                        dagger_dual_success_deferred=true;
                    }
                    dagger_prerequisite_failed=result.prerequisite_failed;
                    // Repeating extra comes from authoritative World completion only.
                    if(result.reset_recharge){recharge_seconds=0;fixed_recharge=true;}
                    if(dagger_prerequisite_failed){
                        // Captured failure: base recharge then finish46, reset
                        // (for Fox/duals) then pending release. Energy stays paid.
                        std::vector<Packet> prefix{slot_event(228,actor_,{p.request.skill,p.request.copy},skill->recharge),terminal(46,actor_)};
                        const auto current_source=hooks.target_scope?hooks.target_scope(actor_):std::nullopt;
                        const auto current_target=hooks.target_scope?hooks.target_scope(p.request.target):std::nullopt;
                        bool feedback=false;
                        if(result.failure_feedback&&current_source&&current_target&&*current_target==*p.resurrection_scope)
                            if(auto wire=result.failure_feedback->consume(*current_source,*current_target,p.request.skill,p.request.copy,p.attack_lease->permit)){
                                prefix.push_back(std::move(*wire));feedback=true;
                            }
                        if(result.dual_failure_feedback&&current_source&&current_target&&*current_target==*p.resurrection_scope)
                            if(auto wire=result.dual_failure_feedback->consume(0,*current_source,*current_target,p.request.skill,p.request.copy,p.attack_lease->permit,now)){
                                prefix.push_back(std::move(*wire));feedback=true;dagger_dual_failure_deferred=true;
                                dagger_failed_second_draft_=DaggerFailedSecondDraft{result.dual_failure_feedback,p.attack_lease->permit,
                                    *current_source,*current_target,p.request.skill,p.request.copy,hooks.control_epoch?hooks.control_epoch():std::nullopt};
                            }
                        hooks.events(prefix);
                        completion_prefix_sent=true;
                        success_phase=feedback?"dagger_prerequisite_failure_native_component7":"dagger_prerequisite_failed_feedback_unverified";
                    }
                }
            }else if(melee->requires_critical_authority&&(!hooks.melee_attack||!hooks.critical_attack_ready||!hooks.critical_attack_ready(p.request.target,*p.weapon,now))){applied=false;
            }else if(p.request.skill==402||p.request.skill==395||p.request.skill==2144){
                std::vector<effects::Entry> active;
                if(hooks.effect_snapshot){if(auto snapshot=hooks.effect_snapshot(now))active=*snapshot;}
                else for(const auto& [id,e]:effects_)active.push_back({e.id,actor_,id,e.rank,effects::category(*find_skill(id)),e.applied,e.ends});
                if(auto adjusted=timing::recharge(*skill,p.primary,p.fast_rank,p.pvp,active,now,recharge_seconds))recharge_seconds=*adjusted;
                fixed_recharge=true;const auto previous=recharging_.find({p.request.skill,p.request.copy});
                if(used.event&&hooks.completed_bow_attack)atomic_attack_recharge=hooks.completed_bow_attack(*p.resurrection_scope,*p.weapon,*melee,used.event,now,
                    Milliseconds(recharge_seconds)*1000,previous==recharging_.end()?0:previous->second,completion_visuals);
                applied=atomic_attack_recharge.has_value();
            }else applied=p.request.skill==359?
                p.attack_lease&&hooks.prepared_melee_attack&&hooks.prepared_melee_attack(*p.resurrection_scope,*p.weapon,*melee,p.attack_lease->permit,now):
                hooks.melee_attack(*p.resurrection_scope,*p.weapon,*melee,now);
        }else if(iron||daggers||dancing||disrupting){
            auto result=iron?hooks.iron_palm(*p.resurrection_scope,*iron,now):
                daggers?hooks.stone_daggers(*p.resurrection_scope,*daggers,now):
                dancing?hooks.dancing_daggers(*p.resurrection_scope,*dancing,now):
                hooks.disrupting_dagger(*p.resurrection_scope,*disrupting,now);
            applied=result.accepted;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
        }else if(water){
            auto result=hooks.water_projectile(*p.resurrection_scope,*water,now);applied=result.accepted;
            deferred_movement=true;if(applied)completion_visuals=std::move(result.visuals);
        }else if(movement){
            auto result=hooks.movement_hex(*p.resurrection_scope,*movement,now);applied=result.accepted;
            deferred_movement=movement->skill==204||movement->skill==209||movement->skill==2803||movement->skill==210||movement->skill==211||movement->skill==212||movement->skill==213||movement->skill==234||movement->skill==1382;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
        }else if(delusions){
            applied=hooks.drain_delusions(*p.resurrection_scope,*delusions,now);
        }else if(disenchant){
            if(p.request.skill==1347){
                const auto result=hooks.enchantment_removal_result(*p.resurrection_scope,*disenchant,now);
                applied=bool(result);
                if(result&&result->target_hexed)intrinsic_recharge=1.-double(disenchant->hexed_recharge_percent)/100.;
            }else applied=hooks.enchantment_removal(*p.resurrection_scope,*disenchant,now);
        }else if(shatter){
            applied=hooks.shatter_delusions(*p.resurrection_scope,*shatter,now);
        }else if(condition_damage){
            auto result=hooks.condition_damage(*p.resurrection_scope,*condition_damage,now);applied=result.accepted;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
        }else if(p.simple_enchantment_completion||aegis){
            // These self-buffs already publish both reviewed bodies in World.
            // The deferred elemental tail would emit the Conjure caster twice.
            const bool owned_self=owned_self_enchantment_base(p.request.skill,p.effect_rank)!=0;
            deferred_elemental=!owned_self;
            const auto target=(owned_self||p.request.skill==310)?p.use_owner:p.resurrection_scope;
            auto result=p.simple_enchantment_completion?
                (target?hooks.simple_enchantment_completed(*target,p.pvp&&p.request.skill==180?3375:p.request.skill,p.effect_rank,p.divine_rank,spell_effect_event,now):EffectCompletion{}):
                hooks.aegis(*aegis,p.divine_rank,now);
            applied=result.accepted;if(applied){completion_publication=std::move(result.publication);deferred_stoneflesh=p.request.skill==1375&&p.simple_enchantment_completion;if(p.request.skill==2138)deferred_stoneflesh=p.simple_enchantment_completion; // Prefix frame can publish without a deferred callback.
                completion_visuals=std::move(result.visuals);completion_prefix_sent=result.completion_prefix_published;
                if(result.effect_snapshot_at)completion_snapshot_at=std::max(now,*result.effect_snapshot_at);}
        }else if(vigorous||seed||reversal){
            deferred_elemental=true;
            auto result=reversal?hooks.reversal_fortune_completed(*p.resurrection_scope,*reversal,p.divine_rank,spell_effect_event,now):seed?hooks.healing_seed(*p.resurrection_scope,*seed,p.divine_rank,now):hooks.vigorous_spirit(*p.resurrection_scope,*vigorous,p.divine_rank,now);
            applied=result.accepted;if(applied){completion_visuals=std::move(result.visuals);completion_prefix_sent=result.completion_prefix_published;completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;if(result.effect_snapshot_at)completion_snapshot_at=std::max(now,*result.effect_snapshot_at);}
        }else if(deflection){
            deferred_elemental=true;auto result=hooks.shield_deflection(*p.resurrection_scope,*deflection,p.divine_rank,now);
            applied=result.accepted;if(applied){completion_visuals=std::move(result.visuals);completion_prefix_sent=result.completion_prefix_published;
                completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
        }else if(regeneration_shield){
            deferred_elemental=true;auto result=hooks.shield_regeneration(*p.resurrection_scope,*regeneration_shield,p.divine_rank,now);
            applied=result.accepted;if(applied){completion_visuals=std::move(result.visuals);completion_prefix_sent=result.completion_prefix_published;}
        }else if(judgment){
            deferred_elemental=true;auto result=hooks.shield_judgment(*p.resurrection_scope,*judgment,p.divine_rank,now);
            applied=result.accepted;if(applied){completion_visuals=std::move(result.visuals);completion_prefix_sent=result.completion_prefix_published;}
        }else if(spirit){
            deferred_elemental=true;auto result=hooks.protective_spirit(*p.resurrection_scope,*spirit,p.divine_rank,now);
            applied=result.accepted;if(applied){completion_visuals=std::move(result.visuals);completion_prefix_sent=result.completion_prefix_published;}
        }else if(watchful){
            deferred_elemental=true;
            auto result=hooks.watchful_healing(*p.resurrection_scope,*watchful,p.divine_rank,now);
            applied=result.accepted;if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
        }else if(restful){
            deferred_elemental=true;
            auto result=hooks.restful_breeze(*p.resurrection_scope,*restful,p.divine_rank,now);
            applied=result.accepted;if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
        }else if(chilblains){
            deferred_elemental=true;
            auto result=hooks.chilblains(*p.resurrection_scope,*chilblains,now);
            applied=result.accepted;if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);completion_prefix_sent=result.completion_prefix_published;}
        }else if(subversion){
            deferred_hex=true;
            if(hooks.mark_subversion){
                auto result=hooks.mark_subversion(*p.resurrection_scope,*subversion,now);
                applied=result.accepted;if(applied)completion_visuals=std::move(result.visuals);
            }
        }else if(leech){
            deferred_hex=true;
            if(hooks.soul_leech){
                auto result=hooks.soul_leech(*p.resurrection_scope,*leech,now);
                applied=result.accepted;if(applied)completion_visuals=std::move(result.visuals);
            }
        }else if(prism){
            deferred_elemental=true;
            if(hooks.icy_prism){
                auto result=hooks.icy_prism(*p.resurrection_scope,*prism,now);
                applied=result.accepted;if(applied)completion_visuals=std::move(result.visuals);
            }
        }else if(action_hex){
            deferred_hex=true;
            if(hooks.action_damage_hex){
                auto result=hooks.action_damage_hex(*p.resurrection_scope,*action_hex,now);
                applied=result.accepted;if(applied)completion_visuals=std::move(result.visuals);
            }
        }else if(energy_hex){
            deferred_hex=true;
            if(hooks.energy_degeneration_hex){
                auto result=hooks.energy_degeneration_hex(*p.resurrection_scope,*energy_hex,now);
                applied=result.accepted;if(applied)completion_visuals=std::move(result.visuals);
            }
        }else if(hex){
            deferred_hex=true;
            auto result=p.request.skill==883?hooks.degeneration_hex_completed(*p.resurrection_scope,*hex,spell_effect_event,now):hooks.degeneration_hex(*p.resurrection_scope,*hex,now);
            applied=result.accepted;if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);}
        }else if(snow||glimmering){
            // Admitted scoped completions share publication and reward ordering.
            // Keep a single dispatcher branch as this family grows.
            auto result=snow?hooks.snow_storm_completed(*p.resurrection_scope,*snow,spell_effect_event,now):
                hooks.glimmering_mark_completed(*p.resurrection_scope,*glimmering,spell_effect_event,now);
            applied=result.accepted;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);}
        }else if(wound){
            if(p.wound_spell_completion){
                auto result=hooks.wound_spell_completed(*p.resurrection_scope,*wound,spell_effect_event,now);
                applied=result.accepted;if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);}
            }else applied=hooks.wound_spell(*p.resurrection_scope,*wound,now);
        }else if(weapon_spell){
            auto result=hooks.weapon_spell(*p.resurrection_scope,*weapon_spell,now);applied=result.accepted;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;recharge_seconds=find_skill(weapon_spell->skill)->recharge;}
        }else if(cleansing){
            if(p.ally_cleansing_completion){
                auto result=hooks.ally_cleansing_completed(*p.resurrection_scope,*cleansing,spell_effect_event,now);
                applied=result.effect.accepted;
                if(applied){completion_visuals=std::move(result.effect.visuals);completion_publication=std::move(result.effect.publication);early_completion_visuals=result.effect.visuals_before_publication;
                    if(cleansing->disease_half_recharge&&result.outcome.removed_disease){recharge_seconds=2;fixed_recharge=true;}}
            }else if(hooks.ally_cleansing_outcome){
                const auto outcome=hooks.ally_cleansing_outcome(*p.resurrection_scope,*cleansing,now);
                applied=outcome.has_value();
                if(applied&&cleansing->disease_half_recharge&&outcome->removed_disease){recharge_seconds=2;fixed_recharge=true;}
            }else applied=hooks.ally_cleansing(*p.resurrection_scope,*cleansing,now);
            if(applied&&!p.ally_cleansing_completion){const auto body=p.request.skill==311?particles::targeted_body_2022(p.request.target,actor_,p.request.skill):particles::target_body_2022(p.request.target,p.request.skill);if(body)hooks.events({*body});}
        }else if(barbs){
            applied=hooks.soul_barbs(*p.resurrection_scope,*barbs,now);
        }else if(backfire){
            applied=hooks.backfire(*p.resurrection_scope,*backfire,now);
        }else if(recall_cast){
            deferred_elemental=true;auto result=hooks.recall(*p.resurrection_scope,now);
            applied=result.accepted;if(applied){completion_visuals=std::move(result.visuals);completion_prefix_sent=result.completion_prefix_published;
                completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
        }else if(mimicry_cast){
            deferred_elemental=true;auto result=hooks.mimicry({p.request.skill,p.request.copy},*p.resurrection_scope,now);
            applied=result.effect.accepted;
            if(applied){completion_visuals=std::move(result.effect.visuals);mimicry_success=result.copied;
                if(const auto body=particles::targeted_body_2022(p.request.target,actor_,65))completion_visuals.insert(completion_visuals.begin(),*body);
                completion_publication=std::move(result.effect.publication);
                early_completion_visuals=result.effect.visuals_before_publication;
                completion_prefix_sent=result.effect.completion_prefix_published;
                if(mimicry_success){recharge_seconds=0;fixed_recharge=true;}}
        }else if(mend||succor||retribution){
            // Keep maintained ally effects in one dispatch arm: MSVC counts
            // each else-if as nesting in this completion chain.
            deferred_elemental=true;
            auto result=retribution?hooks.retribution_completed(*p.resurrection_scope,*retribution,p.divine_rank,spell_effect_event,now)
                :succor?hooks.succor(*p.resurrection_scope,*succor,p.divine_rank,now)
                :hooks.mending(*p.resurrection_scope,*mend,p.divine_rank,now);
            applied=result.accepted;
            if(applied){completion_visuals=std::move(result.visuals);
                completion_prefix_sent=result.completion_prefix_published;
                completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;
                if(retribution&&result.effect_snapshot_at)completion_snapshot_at=std::max(now,*result.effect_snapshot_at);}
        }else if(blessed){
            auto result=hooks.blessed_signet(*p.resurrection_scope,*blessed,now);applied=result.accepted;deferred_holy=true;
            if(applied){completion_visuals=std::move(result.visuals);completion_prefix_sent=result.completion_prefix_published;
                completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
        }else if(signet){
            auto result=hooks.signet_judgment(*p.resurrection_scope,*signet,now);applied=result.accepted;deferred_holy=true;
            if(applied){completion_visuals=std::move(result.visuals);completion_prefix_sent=result.completion_prefix_published;
                completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
        }else if(shadows){
            auto result=hooks.signet_shadows(*p.resurrection_scope,*shadows,now);applied=result.accepted;deferred_holy=true;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;
                completion_prefix_sent=result.completion_prefix_published;}
        }else if(discord){
            auto result=hooks.discord(*p.resurrection_scope,*discord,now);applied=result.accepted;deferred_holy=true;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;
                completion_prefix_sent=result.completion_prefix_published;}
        }else if(rift){
            auto result=hooks.spirit_rift(*p.resurrection_scope,*rift,now);applied=result.accepted;deferred_holy=true;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;
                completion_prefix_sent=result.completion_prefix_published;}
        }else if(spear){
            auto result=hooks.spear_light(*p.resurrection_scope,*spear,now);applied=result.accepted;deferred_holy=true;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;
                completion_prefix_sent=result.completion_prefix_published;}
        }else if(holy){
            auto result=hooks.holy_damage?hooks.holy_damage(*p.resurrection_scope,*holy,now):EffectCompletion{};applied=result.accepted;deferred_holy=true;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;
                completion_prefix_sent=result.completion_prefix_published;}
        }else if(feast){
            applied=hooks.feast_of_corruption(*p.feast_anchor,*feast,now);
        }else if(gale){
            const auto roll=p.effect_rank<=4&&hooks.skill_roll?hooks.skill_roll():std::nullopt;
            applied=hooks.gale(*p.resurrection_scope,p.effect_rank,roll,now);
            if(applied&&p.effect_rank<=4&&roll&&*roll<.5)success_phase="attribute_failure";
        }else if(ear_bite||energy_blast||aneurysm||necrosis||shadow||p.request.skill==2353||p.request.skill==2358||p.request.skill==2359){
            if(p.request.skill==2359){
                const auto plan=effects::you_are_all_weaklings(p.effect_rank);
                auto result=plan&&p.resurrection_scope&&hooks.you_are_all_weaklings?
                    hooks.you_are_all_weaklings(*p.resurrection_scope,*plan,now):EffectCompletion{};
                applied=result.accepted;
                completion_visuals=std::move(result.visuals);
                completion_publication=std::move(result.publication);
            }else if(p.request.skill==2358){
                const auto plan=effects::you_move_like_a_dwarf(p.effect_rank);
                auto result=plan&&p.resurrection_scope&&hooks.you_move_like_a_dwarf?
                    hooks.you_move_like_a_dwarf(*p.resurrection_scope,*plan,now):EffectCompletion{};
                applied=result.accepted;
                completion_visuals=std::move(result.visuals);
                completion_publication=std::move(result.publication);
            }else if(p.request.skill==2353){
                const auto plan=effects::finish_him(p.effect_rank);
                auto result=plan&&p.resurrection_scope&&hooks.finish_him?
                    hooks.finish_him(*p.resurrection_scope,*plan,now):EffectCompletion{};
                applied=result.accepted;
                completion_visuals=std::move(result.visuals);
                completion_publication=std::move(result.publication);
            }else if(ear_bite){
                auto result=hooks.ear_bite(*p.resurrection_scope,*ear_bite,now);
                applied=result.accepted;if(applied)completion_publication=std::move(result.publication);
            }else if(energy_blast){
                auto result=hooks.energy_blast(*p.resurrection_scope,*energy_blast,now);
                applied=result.accepted;deferred_energy_blast=true;if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
            }else if(aneurysm){
                auto result=hooks.aneurysm(*p.resurrection_scope,*aneurysm,now);
                applied=result.accepted;deferred_aneurysm=true;
                if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);}
            }else if(necrosis){
                auto result=hooks.necrosis(*p.resurrection_scope,*necrosis,now);
                applied=result.accepted;deferred_necrosis=true;if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
            }else{
                auto result=hooks.shadow_damage(*p.resurrection_scope,*shadow,now);applied=result.accepted;
                if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
            }
        }else if(p.request.skill==952){
            const auto plan=effects::death_charge(p.effect_rank);
            auto result=plan&&p.resurrection_scope?hooks.death_charge(*p.resurrection_scope,*plan,now):EffectCompletion{};
            applied=result.accepted;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
        }else if(p.request.skill==1085||p.request.skill==1336){
            EffectCompletion result;
            if(p.resurrection_scope&&p.request.skill==1085){
                const auto plan=effects::ash_blast(p.effect_rank);
                if(plan)result=hooks.ash_blast(*p.resurrection_scope,*plan,now);
            }else if(p.resurrection_scope&&p.request.skill==1336){
                const auto plan=effects::spiritual_pain(p.effect_rank);
                if(plan&&hooks.spiritual_pain)result=hooks.spiritual_pain(*p.resurrection_scope,*plan,now);
            }
            applied=result.accepted;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
        }else if(empathy){
            applied=hooks.empathy(*p.resurrection_scope,*empathy,now);
        }else if(gain){
            const auto result=hooks.resource_gain?hooks.resource_gain(*p.resurrection_scope,*gain,now):std::optional<effects::ResourceGainOutcome>{};
            applied=result.has_value();if(applied&&!result->condition_met)success_phase="applied_no_resource_gain";
        }else if(drain){
            deferred_energy_drain=p.request.skill==79||p.request.skill==80;
            if(p.energy_drain_completion||hooks.energy_drain){
                auto result=p.energy_drain_completion?hooks.energy_drain_completed(*p.resurrection_scope,*drain,spell_effect_event,now):hooks.energy_drain(*p.resurrection_scope,*drain,now);applied=result.accepted;
                if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
            }else applied=false;
        }else if(faint){
            auto result=hooks.faintheartedness(*p.resurrection_scope,*faint,now);
            applied=result.accepted;if(applied)completion_publication=std::move(result.publication);
        }else if(swarm){
            applied=hooks.deathly_swarm(*p.resurrection_scope,*swarm,now);
        }else if(p.request.skill==175||p.request.skill==938){
            deferred_hex=true;const auto plan=effects::ward_elements_plan(p.effect_rank);
            const auto defensive=effects::defensive_ground_ward_plan(p.request.skill,p.effect_rank);
            const auto result=p.request.skill==938?
                (defensive&&hooks.defensive_ward_completed?hooks.defensive_ward_completed(*defensive,spell_effect_event,now):EffectCompletion{}):
                (plan&&hooks.ward_elements?hooks.ward_elements(*plan,now):EffectCompletion{});
            applied=result.accepted;if(applied)completion_visuals=result.visuals;
        }else if(p.request.skill==176){
            deferred_hex=true;const auto plan=effects::ward_melee_plan(p.effect_rank);
            const auto result=plan&&hooks.ward_melee?hooks.ward_melee(*plan,now):EffectCompletion{};
            applied=result.accepted;if(applied)completion_visuals=result.visuals;
        }else if(p.request.skill==177){
            deferred_hex=true;const auto plan=effects::ward_foes_plan(p.effect_rank);
            const auto result=plan&&hooks.ward_foes?hooks.ward_foes(*plan,now):EffectCompletion{};
            applied=result.accepted;if(applied)completion_visuals=result.visuals;
        }else if(p.request.skill==195){
            const auto field=effects::damage_field(p.request.skill,p.effect_rank);
            const auto result=field&&hooks.complete_damage_field?hooks.complete_damage_field(*field,now):std::nullopt;
            applied=result.has_value();if(result)completion_visuals=*result;
        }else if(p.request.skill==247||p.request.skill==1113){
            const auto field=effects::damage_field(p.request.skill,p.effect_rank);
            applied=field&&hooks.damage_field(*field,now);
        }else if(storm||ray){
            const auto result=storm?hooks.complete_fire_storm(*p.resurrection_scope,*storm,now):
                hooks.complete_ray_of_judgment(*p.resurrection_scope,*ray,now);
            applied=result.has_value();if(result)completion_visuals=*result;
        }else if(divine_cast||demonic_cast||selfless_cast){
            if(divine_cast){
                auto result=hooks.divine_spirit(*divine_cast,p.divine_rank,now);
                applied=result.accepted;if(applied){completion_visuals=std::move(result.visuals);
                    completion_prefix_sent=result.completion_prefix_published;}
            }else if(selfless_cast){
                if(hooks.selfless_spirit){
                    auto result=hooks.selfless_spirit(*selfless_cast,p.divine_rank,now);
                    applied=result.accepted;if(applied){completion_visuals=std::move(result.visuals);
                        completion_prefix_sent=result.completion_prefix_published;}}
            }else{
                auto result=hooks.demonic_flesh(next_effect_,*demonic_cast,now);applied=result.accepted;
                if(applied){++next_effect_;completion_visuals=std::move(result.visuals);}
            }
        }else if(glyph_cast){
            auto result=hooks.apply_elemental_glyph(next_effect_,*glyph_cast,now);applied=result.accepted;
            if(applied){++next_effect_;completion_visuals=std::move(result.visuals);
                if(p.request.skill==198)completion_visuals.insert(completion_visuals.begin(),generic(21,actor_,393));
                std::erase_if(effects_,[](const auto& effect){const auto* row=find_skill(effect.first);return row&&row->type==12;});glyph_charges_=0;}
        }else if(burn){
            auto result=hooks.mind_burn(*p.resurrection_scope,*burn,now);applied=result.accepted;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
        }else if(elemental&&(p.elemental_completion||elemental->skill==1068)){
            auto result=p.elemental_completion?
                hooks.elemental_completed(*p.resurrection_scope,*elemental,spell_effect_event,now):
                hooks.conditional_elemental(*p.resurrection_scope,*elemental,now);
            applied=result.has_value();deferred_elemental=!elemental->projectile;
            if(result){completion_visuals=std::move(result->visuals);completion_publication=std::move(result->publication);early_completion_visuals=result->visuals_before_publication;
                if(elemental->skill==1068&&result->outcome.instant_recharge){recharge_seconds=0;fixed_recharge=true;}}
        }else if(elemental){
            auto result=hooks.elemental(*p.resurrection_scope,*elemental,now);
            applied=result.accepted&&(!elemental->self_bleeding_ms||result.paid_sacrifice.has_value());
            if(applied&&elemental->self_bleeding_ms)completed_sacrifice=result.paid_sacrifice;
            deferred_elemental=!elemental->projectile;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
        }else if(p.request.skill==292||p.request.skill==1526){
            if(p.request.skill==292){
                const auto plan=effects::infuse_health(p.effect_rank,p.divine_rank);
                applied=plan&&(p.health_completion?
                    hooks.infuse_health_completed(*p.resurrection_scope,*plan,spell_effect_event,now):
                    hooks.infuse_health(*p.resurrection_scope,*plan,now));
            }else{
                const auto plan=effects::imbue_health(p.effect_rank);
                applied=plan&&(p.health_completion?
                    hooks.imbue_health_completed(*p.resurrection_scope,*plan,spell_effect_event,now):
                    hooks.imbue_health(*p.resurrection_scope,*plan,now));
            }
        }else if(p.request.skill==110){
            const auto plan=effects::area_life_steal(110,p.effect_rank);
            applied=plan&&hooks.area_life_steal(*plan,now);
        }else if(steal){
            if(p.life_steal_completion){
                auto result=p.life_steal_spell_completion?
                    hooks.life_steal_spell_completed(*p.resurrection_scope,*steal,spell_effect_event,now):
                    hooks.complete_life_steal(*p.resurrection_scope,*steal,now);
                applied=result.accepted;deferred_life_steal=true;
                if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
            }else applied=hooks.life_steal(*p.resurrection_scope,*steal,now);
        }else if(spiteful){
            const auto result=hooks.spiteful_spirit(*p.resurrection_scope,*spiteful,now);
            applied=result.accepted;deferred_spiteful=true;if(applied)completion_visuals=result.visuals;
        }else if(tainted_flesh){
            const auto result=hooks.tainted_flesh(*p.resurrection_scope,*tainted_flesh,now);
            applied=result.accepted;deferred_tainted_flesh=true;if(applied)completion_visuals=result.visuals;
        }else if(energy_enchantment){
            auto result=p.energy_enchantment_completion?
                hooks.energy_enchantment_completed(*p.resurrection_scope,*energy_enchantment,spell_effect_event,now):
                hooks.energy_enchantment(*p.resurrection_scope,*energy_enchantment,now);
            applied=result.accepted&&result.paid_sacrifice.has_value();deferred_energy_enchantment=true;
            if(applied){completed_sacrifice=result.paid_sacrifice;completion_visuals=std::move(result.visuals);
                completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
        }else if(rotting_flesh){
            const auto result=hooks.rotting_flesh(*p.resurrection_scope,*rotting_flesh,now);
            applied=result.accepted;deferred_rotting_flesh=true;if(applied)completion_visuals=result.visuals;
        }else if(virulence){
            const auto result=hooks.virulence(*p.resurrection_scope,*virulence,now);
            applied=result.accepted;deferred_virulence=true;if(applied)completion_visuals=result.visuals;
        }else if(condition){
            if(hooks.condition_spell_completed){
                auto result=hooks.condition_spell_completed(*p.resurrection_scope,*condition,spell_effect_event,now);applied=result.accepted;
                if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
            }else applied=hooks.condition_spell&&hooks.condition_spell(*p.resurrection_scope,*condition,now);
        }else if(transfer){
            if(effects::transfer_roll_required(*transfer)){
                const auto roll=hooks.skill_roll?hooks.skill_roll():std::nullopt;
                const auto failed=roll?effects::condition_transfer_failed(*transfer,*roll):std::nullopt;
                if(failed){
                    applied=hooks.condition_transfer(*p.resurrection_scope,*transfer,now,roll);
                    if(applied&&*failed&&hooks.diagnostic)hooks.diagnostic(p.request.skill,"attribute_failure");
                }
            }else applied=hooks.condition_transfer(*p.resurrection_scope,*transfer,now,std::nullopt);
        }else if(blindness){
            applied=hooks.blindness(*p.resurrection_scope,*blindness,now);
        }else if(javelin){
            applied=hooks.lightning_javelin(*p.resurrection_scope,*javelin,now);
        }else if(p.request.skill==10){
            const auto plan=effects::hex_breaker(p.effect_rank);
            applied=plan&&p.resurrection_scope&&hooks.hex_breaker(*p.resurrection_scope,*plan,now);
        }else if(humility){
            applied=hooks.signet_humility(*p.resurrection_scope,*humility,now);
        }else if(interrupt){
            {auto result=hooks.interrupt_skill(*p.resurrection_scope,*interrupt,now);applied=result.accepted;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}}
        }else if(lightning){
            auto result=hooks.lightning(*p.resurrection_scope,*lightning,now);
            applied=result.accepted;deferred_lightning=lightning->skill==1082||lightning->skill==226||lightning->skill==2804||lightning->skill==222||lightning->skill==224||lightning->skill==865||lightning->skill==3396||lightning->skill==1369||lightning->skill==229||lightning->skill==232||lightning->skill==2059||lightning->skill==1664||lightning->skill==228||lightning->skill==205||lightning->skill==842;
            if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);early_completion_visuals=result.visuals_before_publication;}
        }else if(const auto party=effects::party_resurrection(p.request.skill)){
            if(p.resurrection_completion){
                auto result=hooks.resurrection_completed(*p.use_owner,*p.use_owner,p.request.skill,p.effect_rank,spell_effect_event,now);
                applied=result.accepted;if(applied)completion_publication=std::move(result.publication);
            }else applied=hooks.resurrect_party(*party,now);
        }else if(resurrection){
            if(p.resurrection_completion){
                auto result=hooks.resurrection_completed(*p.use_owner,*p.resurrection_scope,p.request.skill,p.effect_rank,spell_effect_event,now);
                applied=result.accepted;if(applied){completion_publication=std::move(result.publication);
                    completion_visuals=std::move(result.visuals);early_completion_visuals=result.visuals_before_publication;}
            }else if(p.request.skill==791){
                auto result=hooks.flesh_my_flesh(*p.resurrection_scope,*resurrection,now);
                applied=result.accepted;
                if(applied){completion_visuals=std::move(result.visuals);
                    completion_publication=std::move(result.publication);
                    early_completion_visuals=result.visuals_before_publication;}
            }else applied=hooks.resurrect(*p.resurrection_scope,*resurrection,now);
        }else if(const auto plan=effects::echo(p.request.skill)){
            if(hooks.arm_echo&&now<=UINT64_MAX-plan->armed_duration){
                const auto id=next_effect_;const auto ends=now+plan->armed_duration;
                applied=hooks.arm_echo({p.request.skill,p.request.copy},id,now,ends);
                if(applied){
                    if(const auto old=effects_.find(p.request.skill);old!=effects_.end())hooks.events({effect_remove(actor_,old->second.id)});
                    ++next_effect_;effects_[p.request.skill]={id,ends,0,now};
                    hooks.events({effect_add(actor_,p.request.skill,0,id,float(plan->armed_duration)/1000.f)});
                }
            }
        }else if(p.request.skill==33){
            const auto plan=effects::illusionary_weaponry(p.pvp?3181:33,p.effect_rank);
            if(plan&&hooks.status_mutation&&now<=UINT64_MAX-plan->duration){
                const auto id=next_effect_;const auto ends=now+plan->duration;
                applied=hooks.status_mutation(plan->skill,id,p.effect_rank,now,ends,true);
                if(applied){
                    if(const auto old=effects_.find(33);old!=effects_.end())hooks.events({effect_remove(actor_,old->second.id)});
                    ++next_effect_;effects_[33]={id,ends,p.effect_rank,now};
                    hooks.events({effect_add(actor_,33,p.effect_rank,id,float(plan->duration)/1000.f)});
                }
            }
        }else if(p.request.skill==1209){
            const auto plan=effects::bestial_fury(p.effect_rank);
            if(plan&&hooks.status_mutation&&now<=UINT64_MAX-plan->duration){
                const auto id=next_effect_;
                const auto ends=now+plan->duration;
                applied=hooks.status_mutation(1209,id,p.effect_rank,now,ends,true);
                if(applied){
                    for(auto it=effects_.begin();it!=effects_.end();){
                        const auto* prior=find_skill(it->first);
                        if(prior&&prior->type==3){
                            if(it->first==1209)hooks.events({effect_remove(actor_,it->second.id)});
                            it=effects_.erase(it);
                        }else ++it;
                    }
                    ++next_effect_;
                    effects_[1209]={id,ends,p.effect_rank,now};
                    hooks.events({effect_add(actor_,1209,p.effect_rank,id,float(plan->duration)/1000.f)});
                }
            }
        }else if(p.request.skill==823||p.request.skill==1261||p.request.skill==1375||(bool(effects::elemental_mantra(p.request.skill))||bool(timing::stance(p.request.skill)))||bool(effects::attunement(p.request.skill))||p.request.skill==1043||p.request.skill==363||p.request.skill==346||p.request.skill==374||p.request.skill==362||p.request.skill==361||p.request.skill==200||p.request.skill==38||bool(haste)||(p.request.skill==180||p.request.skill==3375)||(p.request.skill==431||p.request.skill==432||bool(combat::weapon::preparation_contact_definition(p.request.skill,0)))){
            const float duration=local_duration?float(*local_duration)/1000.f:haste?float(haste->duration)/1000.f:effects::elemental_mantra(p.request.skill)?float(effects::elemental_mantra(p.request.skill,p.effect_rank)->duration)/1000.f:timing::stance(p.request.skill)?float(timing::stance(p.request.skill,p.effect_rank)->duration)/1000.f:effects::attunement(p.request.skill)?static_cast<float>(effects::attunement(p.request.skill,p.effect_rank)->duration)/1000.f:(p.request.skill==431||p.request.skill==432||bool(combat::weapon::preparation_contact_definition(p.request.skill,0)))?(combat::weapon::preparation_contact_definition(p.request.skill,p.effect_rank)?float(combat::weapon::preparation_contact_definition(p.request.skill,p.effect_rank)->duration)/1000.f:24.f):p.request.skill==374?float(effects::warriors_endurance(374,p.effect_rank)->duration)/1000.f:p.request.skill==362?float(*effects::warriors_cunning(p.effect_rank))/1000.f:p.request.skill==361?float(effects::dolyak_signet(p.effect_rank)->duration)/1000.f:p.request.skill==363?float(effects::shield_bash(p.effect_rank)->duration)/1000.f:p.request.skill==346?8.f:p.request.skill==38?float(effects::channeling(p.effect_rank)->duration)/1000.f:(p.request.skill==180||p.request.skill==3375)?60.f:p.request.skill==200?15.f:3.f;
            const auto id=next_effect_;auto effect_applied=now;auto ends=now+local_duration.value_or(static_cast<Milliseconds>(duration*1000.f));
            const unsigned owned_skill=p.pvp&&(p.request.skill==180||p.request.skill==3375)?3375:p.pvp&&p.request.skill==374?3002:p.pvp&&p.request.skill==448?3060:haste?haste->skill:p.request.skill;
            const auto burning=p.request.skill==823?(p.burning_speed_completion?
                hooks.burning_speed_completed(p.effect_rank,id,spell_effect_event,now,ends):hooks.burning_speed(p.effect_rank,id,now,ends)):EffectCompletion{};
            if(burning.accepted&&burning.effect_snapshot_at){effect_applied=std::max(now,*burning.effect_snapshot_at);completion_snapshot_at=effect_applied;
                ends=effect_applied+local_duration.value_or(static_cast<Milliseconds>(duration*1000.f));}
            const bool registered=p.request.skill==823?burning.accepted:
                (!combat::weapon::preparation_contact_definition(p.request.skill,0)||bool(hooks.status_mutation))&&
                (!hooks.status_mutation||hooks.status_mutation(owned_skill,id,p.effect_rank,now,ends,true));
            if(burning.accepted)completion_publication=std::move(burning.publication);
            applied=registered&&(p.request.skill==823||p.request.skill==1261||p.request.skill==1375||(bool(effects::elemental_mantra(p.request.skill))||bool(timing::stance(p.request.skill)))||(p.request.skill==431||p.request.skill==432||bool(combat::weapon::preparation_contact_definition(p.request.skill,0)))||p.request.skill==363||p.request.skill==346||p.request.skill==374||p.request.skill==362||p.request.skill==361||p.request.skill==38||bool(haste)||(p.request.skill==180||p.request.skill==3375)||p.request.skill==200||bool(effects::attunement(p.request.skill))||hooks.speed_multiplier(1.5));
            if(registered&&!applied&&hooks.status_mutation)hooks.status_mutation(p.request.skill,id,p.effect_rank,now,{},false);
            const auto cache_skill=p.request.skill==3375?180u:p.request.skill;
            if(applied){if(auto old=effects_.find(cache_skill);old!=effects_.end()&&!haste&&p.request.skill!=823)
                hooks.events({effect_remove(actor_,old->second.id)}); // Burning Speed World publishes the replaced instance.
                if(skill->type==3||skill->type==19)for(auto old=effects_.begin();old!=effects_.end();){
                    const auto* row=find_skill(old->first);
                    if(old->first!=p.request.skill&&row&&row->type==skill->type){
                        // The authoritative mutation publishes old-effect67.
                        // Clear local stance mechanics now, so replacing Dash
                        // does not leave its movement bonus until another tick.
                        if(old->first==1043&&hooks.speed_multiplier)hooks.speed_multiplier(1.0);
                        old=effects_.erase(old);
                    }else ++old;
                }
                if(p.request.skill==200)glyph_charges_=2;
                ++next_effect_;
                bool retained=true;
                // A completed enchantment can be suppressed by an external
                // field such as Well of the Profane. World owns admission;
                // only mirror an effect that survived that decision.
                if((skill->type==6||p.request.skill==456)&&hooks.effect_snapshot){const auto owned=hooks.effect_snapshot(effect_applied);
                    retained=owned&&std::any_of(owned->begin(),owned->end(),[&](const auto& entry){return entry.instance==id&&entry.skill==owned_skill;});}
                if(retained){
                    // Typed Burning Speed expires in World, including its end
                    // burst. A local expiry cache would publish a second67.
                    if(p.burning_speed_completion)effects_.erase(cache_skill);
                    else effects_[cache_skill]={id,ends,p.effect_rank,effect_applied};
                    // The World commits Deadly Paradox and its equipped attack
                    // disables together. The fixed instant cast has its body
                    // cue before attack-slot timers and stance publication.
                    if(p.request.skill==572){
                        if(const auto body=skill_success_body_effect(actor_,572))hooks.events({*body});
                        if(!synchronize_disables(now,hooks)&&hooks.diagnostic)
                            hooks.diagnostic(572,"attack_disable_feedback_deferred");
                    }
                    const auto packet=effect_add(actor_,(p.request.skill==180||p.request.skill==3375)?owned_skill:haste?haste->skill:p.request.skill,p.effect_rank,id,duration);
                    if(p.request.skill==823){completion_visuals.push_back(packet);early_completion_visuals=true;}
                    else if(p.request.skill==1375)completion_visuals.push_back(packet);
                    else hooks.events({packet});}
                else effects_.erase(p.request.skill);
                // Success-body publication below uses the pinned resource;
                // do not publish Dash twice here and at cast completion.
            }
        }else if(((p.request.skill>=275&&p.request.skill<=278)||p.request.skill==1691||p.request.skill==1401)){
            const auto plan=effects::condition_removal(p.request.skill,p.effect_rank,p.divine_rank);
            applied=plan&&(p.condition_removal_completion?hooks.remove_conditions_completed(*p.resurrection_scope,*plan,spell_effect_event,now):hooks.remove_conditions(p.request.target,*plan,now));
            if(applied)if(const auto body=particles::targeted_body_2022(p.request.target,actor_,p.request.skill))hooks.events({*body});
        }else if(p.request.skill==301||p.request.skill==2003){
            const auto plan=effects::removal(p.request.skill,p.healing_rank,p.divine_rank);
            if(p.effect_removal_completion){
                auto result=plan?hooks.remove_effects_completed(*p.resurrection_scope,*plan,spell_effect_event,now):EffectCompletion{};
                applied=result.accepted;if(applied){completion_visuals=std::move(result.visuals);
                    completion_publication=std::move(result.publication);early_completion_visuals=true;}
            }else applied=plan&&hooks.remove_effects(p.request.target,*plan,now);
        }else if(bool(effects::party_heal(p.request.skill,0,0))){
            const auto plan=effects::party_heal(p.request.skill,p.effect_rank,p.divine_rank);
            if(plan&&p.request.skill==2221){const auto result=hooks.party_heal_published(*plan,spell_effect_event,now);
                applied=result.accepted;if(applied){completion_visuals=result.visuals;completion_publication=result.publication;}}
            else applied=plan&&(p.party_heal_completion?hooks.party_heal_completed(*plan,spell_effect_event,now):hooks.party_heal(*plan,now));
        }else if(p.request.skill==258||p.request.skill==299||p.request.skill==2061||p.request.skill==115||p.request.skill==288||p.request.skill==446||p.request.skill==814){
            const auto plan=regeneration::reviewed(p.request.skill,p.effect_rank);
            applied=plan&&((p.request.skill==258||p.request.skill==299||p.request.skill==2061||(p.request.skill==288&&hooks.protective_enchantment))?hooks.protective_enchantment(*p.resurrection_scope,*plan,p.divine_rank,now):hooks.apply_health_regeneration(p.request.target,*plan,now,now+plan->duration,p.range,p.allow_self));
            // Production commits Breeze and Divine Favor together through the
            // captured-scope hook. Retain the old split callbacks for legacy
            // callers; Troll Unguent cannot trigger Divine Favor.
            if(applied&&p.request.skill==288&&!hooks.protective_enchantment&&p.divine_rank)
                hooks.heal(p.request.target,static_cast<unsigned>(std::lround(3.2*p.divine_rank)),p.range,p.allow_self);
        }else if(p.request.skill==146){
            // Pinned scale fields are sacrifice metadata; bonus8..20 is the
            // reviewed Blood Magic energy gain. Generic sacrifice below is
            // still paid on success, including a lethal health cost.
            applied=hooks.restore_energy(scaled(skill->bonus0,skill->bonus15,p.effect_rank));
        }else if(slippery||p.request.skill==174||p.request.skill==937){
            if(slippery){
                deferred_elemental=true;
                const auto result=p.resurrection_scope&&hooks.slippery_ground
                    ?hooks.slippery_ground(*p.resurrection_scope,*slippery,now):EffectCompletion{};
                applied=result.accepted;
                if(applied)completion_visuals=std::move(result.visuals);
            }else{
                const auto plan=effects::caster_earth_aoe(p.request.skill,p.effect_rank);
                applied=plan&&hooks.hostile_aoe(*plan);
            }
        }else if(p.request.skill==171||p.request.skill==1374||p.request.skill==1661){
            const auto plan=effects::earth_projectile(p.request.skill,p.effect_rank,p.energy_storage_rank);
            applied=plan&&hooks.launch_projectile(p.request.target,*plan,now);
        }else if(p.request.skill==217){
            const auto plan=effects::crystal_wave(p.effect_rank);
            applied=plan&&hooks.crystal_wave(*plan,now);
        }else if(p.request.skill==283||p.request.skill==1396){
            const auto plan=effects::status_heal(p.request.skill,p.healing_rank,p.divine_rank);
            applied=plan&&(p.status_heal_completion?hooks.status_heal_completed(*p.resurrection_scope,*plan,spell_effect_event,now):hooks.status_heal(p.request.target,*plan,now));
            if(applied)if(const auto body=particles::targeted_body_2022(p.request.target,actor_,p.request.skill))hooks.events({*body});
        }else{
        const bool light_enchanted=!p.direct_heal_completion&&p.request.skill==867&&hooks.has_enchantment(p.request.target,now);
        if(p.request.skill==1687||p.direct_heal_completion)applied=p.resurrection_scope&&(p.direct_heal_completion?
            hooks.direct_heal_completed(*p.resurrection_scope,p.request.skill,p.effect_rank,p.divine_rank,spell_effect_event,now):
            hooks.zealous_benediction(*p.resurrection_scope,p.effect_rank,p.divine_rank,now));
        else{
            const auto amount=(!p.pet_target&&target)?effects::direct_heal_amount(p.request.skill,p.effect_rank,p.divine_rank,target->health,target->max_health):std::nullopt;
            applied=amount&&(scoped_heal?hooks.heal_scoped(*p.resurrection_scope,*amount,p.range,p.allow_self,now):hooks.heal(p.request.target,*amount,p.range,p.allow_self));
        }
        // Reviewed wiki semantics and pinned bonus endpoints1/3: caster energy
        // is returned only on a successful heal of an enchanted target. Query
        // live authoritative status at completion, never decoration/selection.
        if(applied&&light_enchanted)hooks.restore_energy(scaled(skill->bonus0,skill->bonus15,p.healing_rank));
        if(applied)if(const auto body=((p.request.skill==286||p.request.skill==958||p.request.skill==1120)?particles::targeted_body_2022(p.request.target,actor_,p.request.skill):particles::target_body_2022(p.request.target,p.request.skill)))hooks.events({*body});
        }
        // Independent staged dispatcher: adding pet cases to the legacy
        // else-if chain exceeds MSVC's fixed block nesting limit. Pet skills
        // have no legacy effect plan; its healing fallback is explicitly inert.
        if(p.pet_charm){
            auto result=hooks.pet_charm_completed(*p.pet_charm,now);applied=result.accepted;
            if(applied){completion_publication=std::move(result.publication);early_completion_visuals=true;
                if(p.pet_lease)p.pet_lease->consumed();
                if(const auto body=particles::targeted_body_2022(p.pet_charm->target.actor,actor_,411))completion_visuals.push_back(*body);
                completion_visuals.insert(completion_visuals.end(),result.visuals.begin(),result.visuals.end());}
        }else if(p.request.skill==436){
            auto result=hooks.pet_comfort_completed(*p.use_owner,*p.pet_target,p.effect_rank,p.pet_sequence,now);applied=result.accepted;
            if(applied){completion_publication=std::move(result.publication);early_completion_visuals=true;
                completion_visuals={generic(21,actor_,747),generic(21,actor_,748)};
                completion_visuals.insert(completion_visuals.end(),result.visuals.begin(),result.visuals.end());}
        }
        if(p.request.skill==97){
            const auto plan=effects::necrotic_traversal(p.effect_rank);
            if(plan&&p.use_owner&&hooks.necrotic_traversal&&next_necrotic_attempt_!=UINT64_MAX){
                auto result=hooks.necrotic_traversal(*p.use_owner,*plan,next_necrotic_attempt_++,now);
                applied=result.accepted;
                if(applied){completion_visuals=std::move(result.visuals);completion_publication=std::move(result.publication);}
            }
        }
        if(p.request.skill>=91&&p.request.skill<=94){
            const auto plan=effects::well_cast_plan(p.request.skill,p.effect_rank);
            if(plan&&p.use_owner&&hooks.well_corpse&&next_well_attempt_!=UINT64_MAX){
                auto result=hooks.well_corpse(*p.use_owner,*plan,next_well_attempt_++,now);
                applied=result.accepted;
                if(applied)completion_visuals=std::move(result.visuals);
            }
        }
        if(p.request.skill==1363&&p.resurrection_scope&&hooks.signet_sorrow){
            const auto plan=effects::signet_sorrow(p.effect_rank);
            if(plan){
                auto result=hooks.signet_sorrow(*p.resurrection_scope,*plan,now);
                applied=result.effect.accepted;
                if(applied){
                    deferred_holy=true;
                    completion_visuals=std::move(result.effect.visuals);
                    completion_publication=std::move(result.effect.publication);
                    early_completion_visuals=true;
                    if(result.recharge_ms==0){recharge_seconds=0;fixed_recharge=true;}
                    else if(result.recharge_ms!=30000)applied=false;
                }
            }
        }
        if(!applied){
            if(p.weapon)hooks.events({combat::weapon::stop_2022(actor_)});
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"effect_failed");
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy}),terminal(p.weapon?49:59,actor_)});return;
        }
        if(completion_prefix_sent)completion_prefix_scope.publish();
        // Succor observes successful spell effects, not starts, interrupts or
        // rejected completions. This dedicated nonwrapping sequence covers
        // zero-recharge spells and casts without a prepare-skill-use token.
        if(effects::is_spell(*skill)&&hooks.succor_spell_completed){
            const auto owner=hooks.target_scope?hooks.target_scope(actor_):std::nullopt;
            const auto sequence=spell_activation_event();
            if((!owner||!sequence||!hooks.succor_spell_completed(*owner,p.request.skill,sequence,now))&&hooks.diagnostic)
                hooks.diagnostic(p.request.skill,"succor_completion_observer_failed");
        }
        if(completion_publication&&!deferred_stoneflesh&&!deferred_gift_health&&!deferred_spirit_light&&!deferred_mend_body_soul&&!deferred_ghostmirror_light&&!deferred_healing_ribbon&&!deferred_rejuvenation){
            std::vector<Packet> prefix;
            // A lethal World completion may already own the finish/body batch.
            // Deferred recipient publication still runs, without replaying it.
            if(!completion_prefix_sent){
                if(!timing::instant_action(*skill))prefix.push_back(terminal(melee?46:58,actor_));
                if(!p.pet_target&&!weapon_spell)if(const auto body=skill_success_body_effect(actor_,p.request.skill))prefix.push_back(*body);
            }
            if(early_completion_visuals){prefix.insert(prefix.end(),completion_visuals.begin(),completion_visuals.end());completion_visuals.clear();}
            completion_prefix_scope.prepare();hooks.events(prefix);completion_prefix_scope.publish();completion_prefix_sent=true;
            // Attack-slow effects may also have a sacrifice cost. Publish that
            // cost after the native finish/caster prefix and before recipients.
            if(!faint){completion_publication();completion_publication={};}
        }
        if(demonic_use.receipt){
            if(hooks.commit_demonic_flesh_use(*demonic_use.receipt))demonic_use.receipt.reset();
            else if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"demonic_flesh_commit_failed");
        }
        // Keep reaction STATE at this frontier (before glyph/cooldown/copy
        // finalization), while its packets wait for the owning native prefix.
        // Retain producer-before-reaction order even on an existing early exit;
        // committed causal packets must not disappear with a failed finalizer.
        struct CompletionReactionPublication {
            std::function<void()>& producer;std::function<void()> reaction;
            void publish(){auto first=std::exchange(producer,{}),second=std::exchange(reaction,{});if(first)first();if(second)second();}
            ~CompletionReactionPublication(){publish();}
        } completion_reactions{completion_publication,{}};
        if(hooks.completed_action){
            auto result=hooks.completed_action(p.request.skill,now);
            completion_reactions.reaction=std::move(result.publication);
            if(!result.accepted&&hooks.diagnostic)hooks.diagnostic(p.request.skill,"completion_reaction_failed");
        }
        if(completion_prefix_sent&&!completion_publication)completion_reactions.publish();
        if(!p.glyph_quote||p.glyph_quote->glyph.plan.skill!=203)glyph_end.finish(effects::GlyphOutcome::completed);
        if(p.request.skill==395||p.request.skill==2144){
            std::erase_if(effects_,[&](const auto& effect){const auto* row=find_skill(effect.first);
                return row&&(row->type==19||(p.request.skill==2144&&row->type==12));
            }); // World already removed and published these records atomically.
            if(p.request.skill==2144)glyph_charges_=0;
        }
        if(aura_trigger&&!balance_reaction_epoch){
            if(hooks.heal)hooks.heal(actor_,aura_trigger->healing,0,true);
            if(aura_trigger->energy&&hooks.restore_energy)hooks.restore_energy(aura_trigger->energy);
        }
        if(!p.pet_target&&!completion_prefix_sent&&!deferred_stoneflesh&&!deferred_gift_health&&!deferred_spirit_light&&!deferred_mend_body_soul&&!deferred_ghostmirror_light&&!deferred_healing_ribbon&&!deferred_rejuvenation&&!weapon_spell&&p.request.skill!=318&&p.request.skill!=3204&&p.request.skill!=347&&p.request.skill!=348&&p.request.skill!=2858&&p.request.skill!=364&&!deferred_copy&&!demonic_cast&&!daggers&&!disrupting&&!iron&&!glyph_cast&&!deferred_holy&&!deferred_hex&&!deferred_lightning&&!deferred_elemental&&!deferred_movement&&!deferred_energy_blast&&!deferred_aneurysm&&!deferred_necrosis&&!deferred_virulence&&!deferred_rotting_flesh&&!deferred_energy_enchantment&&!deferred_tainted_flesh&&!deferred_life_steal&&!deferred_energy_drain&&p.request.skill!=195&&p.request.skill!=830&&p.request.skill!=1262&&p.request.skill!=165&&p.request.skill!=63&&p.request.skill!=572&&p.request.skill!=1375&&!effects::conjure(p.request.skill,0))if(const auto body=skill_success_body_effect(actor_,p.request.skill))hooks.events({*body});
        if(p.request.skill==174)hooks.events({particles::aftershock_screen_shake_2022(actor_)});
        bool sacrifice_death=bool(balance_reaction_epoch);
        if(completed_sacrifice){const auto owner=hooks.resources(actor_);sacrifice_death|=!owner||!owner->health;}
        if(skill->health_sacrifice){
            const auto loss=completed_sacrifice?completed_sacrifice:hooks.health_sacrifice(p.request.skill,now);
            if(!loss){if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"sacrifice_commit_failed");}
            else if(loss->killed){
                // The effect succeeded: retain previous recharge/signets and
                // publish this skill's recharge even when its cost is lethal.
                // Normal death cleanup removes owned effects on the next tick.
                sacrifice_death=true;
            }
        }
        if(faint&&completion_publication){completion_publication();completion_publication={};}
        if(completion_prefix_sent&&!completion_publication)completion_reactions.publish();
        if(!sacrifice_death&&hooks.restore_energy){
            std::vector<effects::Entry> active;
            if(hooks.effect_snapshot){if(auto snapshot=hooks.effect_snapshot(completion_snapshot_at))active=*snapshot;}
            else for(const auto& [id,e]:effects_)if(effects::attunement(id))active.push_back({e.id,actor_,id,e.rank,effects::Category::enchantment,e.applied,e.ends});
            const auto refund=effects::attunement_cast_refund(active,*skill,completion_snapshot_at);if(refund)hooks.restore_energy(refund);
        }
        if(channeling.event){
            if(hooks.complete_channeling(channeling.event,true,completion_snapshot_at))channeling.event=0;
            else if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"spell_observer_commit_failed");
        }
        if(!fixed_recharge){
            std::vector<effects::Entry> active;
            if(hooks.effect_snapshot){if(auto snapshot=hooks.effect_snapshot(now))active=*snapshot;}
            else for(const auto& [id,e]:effects_)active.push_back({e.id,actor_,id,e.rank,effects::category(*find_skill(id)),e.applied,e.ends});
            const bool zephyr=hooks.quickening_zephyr_aura&&
                hooks.quickening_zephyr_aura(now).value_or(false);
            if(auto adjusted=timing::recharge(*skill,p.primary,p.fast_rank,p.pvp,active,now,recharge_seconds,intrinsic_recharge,zephyr))recharge_seconds=*adjusted;
        }

        std::optional<effects::CooldownOrigin> cooldown_origin;unsigned completion_extra_seconds=0;
        if(used.event){
            const auto previous=recharging_.find({p.request.skill,p.request.copy});
            if(p.glyph_quote&&p.glyph_quote->glyph.plan.skill==203){
                const auto existing=previous==recharging_.end()?0:previous->second;
                const auto receipt=hooks.finish_renewal_spell?hooks.finish_renewal_spell(p.glyph_event,used.event,{p.request.skill,p.request.copy},effects::GlyphOutcome::completed,
                    now,Milliseconds(recharge_seconds)*1000,existing,{}):std::nullopt;
                if(!receipt||!accept_renewal_terminal(*receipt,p,used.event,now,Milliseconds(recharge_seconds)*1000,existing,{},hooks)){
                    if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"renewal_completion_commit_failed");return;
                }
                glyph_end.done=true;used.event=0;recharge_seconds=0;
                if(receipt->independent_disable_until>now){auto& end=external_disables_[{p.request.skill,p.request.copy}];end=std::max(end,receipt->independent_disable_until);}
            }else{
            const auto until=atomic_attack_recharge?atomic_attack_recharge:hooks.finalize_skill_use(used.event,now,Milliseconds(recharge_seconds)*1000,
                previous==recharging_.end()?0:previous->second,fixed_recharge&&recharge_seconds==0&&!mimicry_success);
            if(until){
                if(hooks.cooldown_completion_source){const auto source=hooks.cooldown_completion_source(used.event);
                    if(source&&source->slot==Slot{p.request.skill,p.request.copy}&&source->skill_use==used.event&&source->base_ms==Milliseconds(recharge_seconds)*1000&&
                       source->existing_until==(previous==recharging_.end()?0:previous->second)&&cooldown_owner_matches(source->stamp,now,hooks)){
                        cooldown_origin=effects::CooldownOrigin{source->stamp,effects::CooldownOriginKind::completion,used.event};
                        if(source->extra_ms==15000)completion_extra_seconds=15;
                        processed_completion_order_=std::max(processed_completion_order_,source->stamp.order);
                    }
                }
                if(*until>now){auto& end=external_disables_[{p.request.skill,p.request.copy}];end=std::max(end,*until);}
                used.event=0;
            }else if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"skill_use_finalize_failed");
            }
        }
        if(hooks.pet_offensive_completed&&!p.weapon&&p.pet_offensive_owner&&p.pet_offensive_target&&hooks.target_scope&&
           hooks.target_scope(actor_)==p.pet_offensive_owner&&hooks.target_scope(p.pet_offensive_target->actor)==p.pet_offensive_target&&
           !sacrifice_death&&!balance_reaction_epoch){
            const auto sequence=spell_activation_event();
            if(sequence)hooks.pet_offensive_completed(*p.pet_offensive_owner,*p.pet_offensive_target,p.request.skill,sequence,now);
        }
        if(hooks.panic_skill_used&&p.use_owner&&!sacrifice_death&&!balance_reaction_epoch){
            const auto current=hooks.resources(actor_);
            if(current&&current->health&&(!completed_use_event||!hooks.panic_skill_used({*p.use_owner,p.request.skill,completed_use_event,p.began,now,true})))
                if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"panic_observer_failed");
        }
        if(hooks.mind_wrack_targeted&&p.use_owner&&p.use_target&&!sacrifice_death&&!balance_reaction_epoch){
            const auto current=hooks.resources(actor_);
            if(current&&current->health&&(!completed_use_event||!hooks.mind_wrack_targeted({*p.use_owner,*p.use_target,p.request.skill,completed_use_event,now,true})))
                if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"mind_wrack_target_observer_failed");
        }
        if(hooks.keystone_skill_used&&skill->type==7&&p.use_owner&&p.use_target&&!sacrifice_death&&!balance_reaction_epoch){
            const auto current=hooks.resources(actor_);
            if(current&&current->health&&(!completed_use_event||!hooks.keystone_skill_used({*p.use_owner,*p.use_target,p.request.skill,completed_use_event,now,true,false})))
                if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"keystone_observer_failed");
        }
        if(melee)hooks.events({combat::weapon::finish_2022(actor_,p.weapon&&p.weapon->item_type==5)});
        // Archived Shadow_step.html: caster-moving steps not exclusively targeting foes
        // have no aftercast.97 is untargeted corpse selection, unlike952.
        aftercast_until_=sacrifice_death?0:now+(p.request.skill==97?0:p.request.skill==399?combat::weapon::distracting_shot(p.effect_rank)->aftercast:static_cast<Milliseconds>(std::llround(skill->aftercast*1000.0)));
        if(resurrection&&resurrection->morale_recharge)spent_signets_.insert({p.request.skill,p.request.copy});
        recharge_seconds+=completion_extra_seconds;
        if(recharge_seconds){if(!record_local_cooldown({p.request.skill,p.request.copy},now+std::uint64_t(recharge_seconds)*1000,cooldown_origin))recharge_seconds=0;
            if(hooks.cooldown_completion_source&&!cooldown_origin&&hooks.diagnostic)hooks.diagnostic(p.request.skill,"cooldown_unsequenced_completion");}
        if(p.observe_copy&&cooldown_origin&&cooldown_origin->kind==effects::CooldownOriginKind::completion&&
           hooks.completed_skill_use&&hooks.cooldown_completion_source){
            const auto completed=hooks.cooldown_completion_source(cooldown_origin->token);
            if(!completed||completed->stamp!=cooldown_origin->stamp||completed->slot!=Slot{p.request.skill,p.request.copy}||
               !hooks.completed_skill_use(*completed))
                if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"temporary_copy_completion_failed");
        }
        const auto deadline=recharging_.find({p.request.skill,p.request.copy});
        if(deadline!=recharging_.end())recharge_seconds=std::max(recharge_seconds,remaining_seconds(deadline->second,now));
        if(const auto external=external_disables_.find({p.request.skill,p.request.copy});external!=external_disables_.end())recharge_seconds=std::max(recharge_seconds,remaining_seconds(external->second,now));
        if(deferred_stoneflesh||deferred_gift_health||deferred_spirit_light||deferred_mend_body_soul||deferred_ghostmirror_light||deferred_healing_ribbon||deferred_rejuvenation){
            std::vector<Packet> prefix{slot_event(228,actor_,{p.request.skill,p.request.copy},recharge_seconds),terminal(58,actor_)};
            if(const auto body=skill_success_body_effect(actor_,p.request.skill))prefix.push_back(*body);
            prefix.insert(prefix.end(),completion_visuals.begin(),completion_visuals.end());
            completion_prefix_scope.prepare();hooks.events(prefix);completion_prefix_scope.publish();
            completion_reactions.publish();
            synchronize_disables(now,hooks);
            hooks.events({cancellation_packet(actor_,{p.request.skill,p.request.copy})});
            if(hooks.diagnostic)hooks.diagnostic(p.request.skill,"applied");return;
        }
        // 226 releases one pending-cast reference/UI marker;228 clears
        // adrenaline and starts recharge. Neither substitutes for the other.
        // Attack lifecycle46/49 is distinct from spell lifecycle58/59.
        std::vector<Packet> finish_packets;
        // Frozen Stoneflesh receipt: recharge, finish58, body1184, status.
        const bool stoneflesh_completion=p.request.skill==1375;
        if(stoneflesh_completion)finish_packets.push_back(slot_event(228,actor_,{p.request.skill,p.request.copy},recharge_seconds));
        // Frozen Snow Storm: recharge precedes finish58 and both ground forms.
        if(p.request.skill==2222)finish_packets.push_back(slot_event(228,actor_,{p.request.skill,p.request.copy},recharge_seconds));
        if(p.request.skill==938)finish_packets.push_back(slot_event(228,actor_,{p.request.skill,p.request.copy},recharge_seconds));
        const bool captured_area_field=p.request.skill==2222||p.request.skill==195||p.request.skill==197||p.request.skill==1094||p.request.skill==830;
        //28 hash-verified bow windows release their pending slot without a
        // melee46 terminal. Native ranged finish and slot/recharge events remain.
        const bool bow=melee&&p.weapon&&p.weapon->item_type==5;
        if(!completion_prefix_sent&&!timing::instant_action(*skill)&&!bow)finish_packets.push_back(terminal(melee?46:58,actor_));
        if(p.pet_target||stoneflesh_completion||weapon_spell||(p.request.skill>=91&&p.request.skill<=94)||deferred_copy||demonic_cast||daggers||dancing||disrupting||iron||glyph_cast||burn||captured_area_field||deferred_holy||deferred_hex||deferred_lightning||deferred_elemental||deferred_movement||deferred_energy_blast||deferred_aneurysm||deferred_necrosis||deferred_virulence||deferred_rotting_flesh||deferred_energy_enchantment||deferred_tainted_flesh||deferred_spiteful||deferred_life_steal||deferred_energy_drain||(atomic_attack_recharge&&!completion_visuals.empty())){
            // An admitted Ward938 terminal can be consumed by a lethal
            // pre-effect reward. No field means no orphan ward-success body.
            if(!p.pet_target&&!completion_prefix_sent&&!weapon_spell&&!(p.request.skill==938&&completion_visuals.empty()))if(const auto body=skill_success_body_effect(actor_,p.request.skill))finish_packets.push_back(*body);
            finish_packets.insert(finish_packets.end(),completion_visuals.begin(),completion_visuals.end());
        }
        if(dagger_prerequisite_failed){
            if(recharge_seconds!=skill->recharge)finish_packets.push_back(slot_event(228,actor_,{p.request.skill,p.request.copy},recharge_seconds));
            if(!dagger_dual_failure_deferred)finish_packets.push_back(cancellation_packet(actor_,{p.request.skill,p.request.copy}));
        }else{
        if(!dagger_dual_success_deferred)finish_packets.push_back(cancellation_packet(actor_,{p.request.skill,p.request.copy}));
        if(!mimicry_success&&!stoneflesh_completion&&p.request.skill!=2222&&p.request.skill!=938)finish_packets.push_back(resurrection&&resurrection->morale_recharge?slot_event(230,actor_,{p.request.skill,p.request.copy}):
                slot_event(228,actor_,{p.request.skill,p.request.copy},recharge_seconds));
        }
        completion_prefix_scope.prepare();hooks.events(finish_packets);completion_prefix_scope.publish();
        completion_reactions.publish();
        if(skill->adrenaline&&hooks.reconcile_adrenaline)hooks.reconcile_adrenaline(now);
        if(hooks.diagnostic)hooks.diagnostic(p.request.skill,balance_reaction_epoch?"applied_reaction_death":sacrifice_death?"applied_self_sacrifice_death":success_phase);
    }
};
}

















