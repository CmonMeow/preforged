"""Developer-only fail-closed dispatcher coverage audit regressions."""
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/protocol"))
from skill_runtime_coverage import dispatcher_ids, live_hook_bindings, FAMILIES
from unittest.mock import patch


class Coverage(unittest.TestCase):

    def test_simple_enchantment_result_assignments_must_be_executable(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        start = runtime.index("}else if(p.simple_enchantment_completion||aegis)")
        end = runtime.index("}else if(vigorous||seed||reversal)", start)
        branch = runtime[start:end]
        assignments = ("completion_visuals=std::move(result.visuals);",
                       "completion_prefix_sent=result.completion_prefix_published;")
        live_hook_bindings(runtime, gateway)
        for token in assignments:
            # Other completion families still contain the same assignments;
            # those occurrences must not satisfy this specific branch's guard.
            for replacement in ("", "/*" + token + "*/", '"' + token + '";'):
                changed = branch.replace(token, replacement, 1)
                tampered = runtime[:start] + changed + runtime[end:]
                with self.assertRaisesRegex(ValueError, "Simple enchantment successful-result assignments"):
                    live_hook_bindings(tampered, gateway)
        # Reproduce the actual accidental same-line comment: both tokens remain
        # in the file but C++ no longer executes either assignment.
        changed = branch.replace(assignments[0], "// " + assignments[0], 1)
        with self.assertRaisesRegex(ValueError, "Simple enchantment successful-result assignments"):
            live_hook_bindings(runtime[:start] + changed + runtime[end:], gateway)


    def test_hexers_vigor_requires_paid_regen_and_completion_consumers(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        self.assertIn(2138, dispatcher_ids(runtime))
        live_hook_bindings(runtime, gateway)
        for token in ("request.skill==2138&&(!regeneration::reviewed(2138,effective[6])||!simple_enchantment_completion||!hooks.completed_action)",
                      "p.request.skill==2138&&!hooks.completed_action"):
            with self.assertRaisesRegex(ValueError, "Hexers Vigor paid regeneration/completion"):
                live_hook_bindings(runtime.replace(token, "MISSING_VIGOR_CONSUMER"), gateway)
        original = Path.read_text
        for name, token in (("gw_world_self_enchantments.inl", "vigor?vigor->pips:0"),
                            ("gw_world_simulation.h", "if(skill&&row&&row->type!=4&&skill!=2138)")):
            def tampered(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_VIGOR_CONSUMER") if path.name == name else text
            with patch.object(Path, "read_text", tampered):
                with self.assertRaisesRegex(ValueError, "Hexers Vigor paid regeneration/completion"):
                    live_hook_bindings(runtime, gateway)

    def test_shared_completion_prefix_publication_remains_owned(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        for token in ("completion_prefix_scope.lease=hooks.prepare_completion_publication(*owner,now)",
                      "completion_prefix_scope.prepare();hooks.events(finish_packets);completion_prefix_scope.publish();"):
            with self.assertRaisesRegex(ValueError, "Shared completion prefix publication"):
                live_hook_bindings(runtime.replace(token, "MISSING_PREFIX_OWNER"), gateway)
        for token in ("world.prepare_completion_publication(owner,now)", "outbox->enqueue(result->packets)"):
            with self.assertRaisesRegex(ValueError, "Shared completion prefix publication"):
                live_hook_bindings(runtime, gateway.replace(token, "MISSING_PREFIX_PUBLICATION"))

    def test_shared_completion_reaction_publication_remains_owned(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        live_hook_bindings(runtime, gateway)
        for token in ("completion_reactions.reaction=std::move(result.publication);",
                      "~CompletionReactionPublication(){publish();}"):
            with self.assertRaisesRegex(ValueError, "Shared completion reaction publication"):
                live_hook_bindings(runtime.replace(token, "MISSING_REACTION_PUBLICATION"), gateway)
        with self.assertRaisesRegex(ValueError, "Shared completion reaction publication"):
            live_hook_bindings(runtime, gateway.replace("world.completed_skill_action(*skill_actor_scope,skill,now,nullptr,&result.publication)", "MISSING_CAPTURE_CAPABILITY"))

    def test_paid_retribution_requires_opt_in_capability_guards(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        self.assertNotIn(248, dispatcher_ids(runtime))
        live_hook_bindings(runtime, gateway)
        for token in ["request.skill==248&&hooks.retribution_completed",
                      "if(request.skill==248&&(!retribution||!hooks.retribution_completed||!hooks.retribution_target_valid||!hooks.before_spell_effect||!hooks.target_scope))"]:
            with self.assertRaisesRegex(ValueError, "live skill hooks mismatch"):
                live_hook_bindings(runtime.replace(token, "MISSING_RETRIBUTION_GATE", 1), gateway)

    def test_paid_reversal_remains_explicitly_staged(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        self.assertNotIn(307, dispatcher_ids(runtime))
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        for hook in ["reversal_target_valid", "reversal_fortune_completed"]:
            with self.assertRaisesRegex(ValueError, "unexpected empty live skill callbacks"):
                live_hook_bindings(runtime, gateway.replace(f".{hook} = {{}}", f".{hook} = SOME_CALLBACK"))

    def test_mind_burn_shared_consumers_remain_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        self.assertIn(185, dispatcher_ids(runtime))
        original = Path.read_text
        for token in ["project_batch_player_damage_locked(batch,p,actor,damage,now)", "project_batch_condition_reactions_locked(batch,p,p.conditions,conditions,now)", "!burning_immune_locked(p,now)", "begin_damage_publication_locked(pending)"]:
            def tampered(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_MIND_BURN_CONSUMER") if path.name == "gw_world_mind_burn.inl" else text
            with patch.object(Path, "read_text", tampered):
                with self.assertRaisesRegex(ValueError, "Mind Burn shared damage/condition/publication consumers incomplete"):
                    dispatcher_ids(runtime)

    def test_shared_route_provenance_covers_bindings_without_metadata_inference(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        routes = {}
        bindings = dispatcher_ids(runtime, provenance=routes)
        self.assertEqual(set(bindings), set(routes))
        self.assertEqual(bindings, dispatcher_ids(runtime))
        self.assertEqual(routes[74], {"gw_skill_casting.h::explicit_supported_id"})
        self.assertEqual(routes[1720], {"gw_weapon_attack_skills.h::attack_skill"})
        self.assertEqual(routes[571], {"gw_skill_casting.h::explicit_supported_id"})
        self.assertEqual(routes[435], {"gw_weapon_preparations.h::preparation_contact_definition"})
        for skill in [787,793,794,2893]:
            self.assertEqual(routes[skill], {"gw_weapon_spells.h::weapon_spell"})
        for skill in [135,136,1260]:
            self.assertEqual(routes[skill], {"gw_faintheartedness.h::attack_slow_hex"})
        self.assertEqual(routes[175], {"gw_skill_casting.h::ward_elements_plan + gw_world_wards.inl::apply_ward_elements"})
        self.assertEqual(routes[176], {"gw_skill_casting.h::ward_melee_plan + gw_world_wards.inl::apply_ward_melee"})
        self.assertEqual(routes[177], {"gw_skill_casting.h::ward_foes_plan + gw_world_wards.inl::apply_ward_foes"})
        self.assertNotIn(0, routes)
        self.assertTrue(all(isinstance(value, set) and value for value in routes.values()))

    def test_stoneflesh_removal_context_reaches_shared_conversion(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        self.assertIn(1375, dispatcher_ids(runtime))
        original = Path.read_text
        for name, token in [
            ("gw_world_simulation.h", "preflight_npc_reversal_or_damage_locked(preview,damage,now,nullptr,nullptr,nullptr,ending_stoneflesh?&*ending_stoneflesh:nullptr)"),
            ("gw_world_simulation.h", "nullptr,nullptr,nullptr,true,scope,ending_stoneflesh?&*ending_stoneflesh:nullptr)"),
            ("gw_world_reversal_npc.inl", "apply_reversal_damage(life,damage,now,true,ending,ending_stoneflesh)"),
            ("gw_npc_combat_catalog.h", "raw.apply_damage_impl(expected,damage,now,hostile,ending,ending_stoneflesh)"),
            ("gw_npc_combat_catalog.h", "ending_stoneflesh&&!ending_stoneflesh->plan(hex_scope(),now)"),
        ]:
            def tampered(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_STONEFLESH_CONTEXT") if path.name == name else text
            with patch.object(Path, "read_text", tampered):
                with self.assertRaisesRegex(ValueError, "Stoneflesh production damage, critical, removal or casting consumers incomplete"):
                    dispatcher_ids(runtime)

    def test_energy_area_shared_seed_and_scoped_energy_remain_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        self.assertTrue({39, 42, 59}.issubset(dispatcher_ids(runtime)))
        original = Path.read_text
        for name, token in [
            ("gw_world_energy_drain_area.inl", "DamageAdmissionBatch batch;batch.seed_facts=&seed_facts"),
            ("gw_world_energy_drain_area.inl", "project_batch_player_damage_locked(batch,player,actor,damage,now)"),
            ("gw_world_energy_drain_area.inl", "project_batch_condition_reactions_locked(batch,player,player.conditions,conditions,now)"),
            ("gw_world_energy_drain_area.inl", "prepare_healing_seed_direct_locked(player->second,1.,now)"),
            ("gw_world_energy_drain_area.inl", "commit_player_reactive_hit_locked(player,hit,scope.actor,now,"),
            ("gw_world_energy_drain_area.inl", "struct PlayerDrain {skills::effects::Scope scope;"),
            ("gw_world_energy_drain_area.inl", "if(plan.drain_each||c.scope==target){"),
            ("gw_world_energy_drain_area.inl", "c.energy_committed=true"),
            ("gw_world_energy_drain_area.inl", "preview.take_fragility_hits(life)"),
            ("gw_world_energy_drain_area.inl", "validate_npc_reversal_receipts_locked(c.scope,{event.reversal})"),
            ("gw_world_energy_drain_area.inl", "preflighted current drain condition capacity exhausted"),
            ("gw_world_damage_admission.inl", "project_player_protected_hit(state,incoming,now)"),
            ("gw_world_damage_admission.inl", "project_batch_seed_locked(batch,state,step->actual,now)"),
        ]:
            def tampered(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_ENERGY_CONSUMER") if path.name == name else text
            with patch.object(Path, "read_text", tampered):
                with self.assertRaisesRegex(ValueError, "Energy area shared protection, Seed or scoped Energy consumers incomplete"):
                    dispatcher_ids("static bool supported(unsigned id){return bool(effects::energy_drain(id));}")

    def test_shockwave_npc_conversion_and_feedback_remain_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        self.assertIn(937, dispatcher_ids(runtime))
        original = Path.read_text
        for name, token in [
            ("gw_npc_combat_catalog.h", "proposed.resolve_protected_slice(expected,actor,damage,now,true)"),
            ("gw_npc_combat_catalog.h", "previous=previous_actual?&*previous_actual:nullptr"),
            ("gw_npc_combat_catalog.h", "proposed.adopt_absorption_hit(*this,raw,lifetime_,event.at)"),
            ("gw_world_simulation.h", "validate_npc_reversal_receipts_locked(found->second.hex_scope(),result->reversals)"),
            ("gw_world_simulation.h", "commit_npc_earth_sequence_locked(scope,target.owner,plan,target.distance,level,now)"),
            ("gw_world_earth_npc.inl", "if(publish_npc_fragility_locked(target.actor,found->second))return true"),
            ("gw_world_earth_npc.inl", "commit_npc_reversal_or_damage_locked(credit(),placed->second,found->second"),
            ("gw_npc_combat_catalog.h", "target.actor.health=proposed.actor_.health;target.actor.effects=proposed.actor_.effects"),
            ("gw_npc_combat_catalog.h", "armor_at_hit,damage_at_hit,condition_at_hit)"),
            ("gw_shockwave.h", "if(condition_at_hit&&!condition_at_hit(result.after,condition))"),
            ("gw_world_simulation.h", "commit_player_earth_sequence_locked(scope,target.owner,plan,target.distance,level,now)"),
            ("gw_world_simulation.h", "preview_player_earth_sequence_locked(found->second,plan,target.distance,level,scope.actor,now,batch)"),
            ("gw_world_earth_player.inl", "player->reviewed_earth_armor,player->effects,true},now,true)"),
            ("gw_world_earth_player.inl", "commit_conditions_locked(*player,conditions,{kind},now)"),
            ("gw_world_damage_admission.inl", "project_player_protected_hit(state,incoming,now)"),
            ("gw_world_damage_admission.inl", "immediate_condition_transitions(before,after,now)"),
            ("gw_world_earth_player_preview.inl", "state.health.current,state.health.capacity.maximum()"),
            ("gw_world_earth_player_preview.inl", "project_batch_player_damage_locked(batch,p,actor,damage,now)"),
            ("gw_world_earth_player_preview.inl", "project_batch_condition_reactions_locked(batch,p,before,conditions,now)"),
            ("gw_world_damage_admission.inl", "project_batch_seed_locked(batch,state,step->actual,now)"),
            ("gw_world_damage_admission.inl", "state->health.capacity.event(state->health.current,seed.healing)"),
            ("gw_world_damage_admission.inl", "proposed.gain_ally_health(life,seed.healing,now)"),
            ("gw_world_healing_seed.inl", "health_event_locked(ally,reaction.healing,now)"),
            ("gw_world_healing_seed.inl", "live->second.gain_ally_health(life,reaction.healing,now)"),
            ("gw_npc_combat_catalog.h", "if(wound_.base)wound_.event(actor_.health->current,amount,false)"),
            ("gw_world_damage_admission.inl", "batch.missing_seed=target"),
            ("gw_world_damage_admission.inl", "admitted->second.healing!=plan->healing"),
            ("gw_world_simulation.h", "prepare_healing_seed_direct_locked(player->second,1.,now)"),
            ("gw_world_simulation.h", "batch.seed_facts=&seed_facts"),
        ]:
            def tampered(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_EARTH_CONSUMER") if path.name == name else text
            with patch.object(Path, "read_text", tampered):
                with self.assertRaisesRegex(ValueError, "Shockwave NPC shared protection or feedback consumers incomplete"):
                    dispatcher_ids(runtime)

    def test_repeated_shared_route_is_not_double_counted(self):
        runtime = "static bool supported(unsigned id){return id==74||id==74;}"
        routes = {}
        self.assertEqual(set(dispatcher_ids(runtime, provenance=routes)), {74})
        self.assertEqual(routes[74], {"gw_skill_casting.h::explicit_supported_id"})

    def test_party_defense_shouts_require_lifetime_and_immunity_consumers(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        self.assertIn(2216, dispatcher_ids(runtime))
        original = Path.read_text
        for name, token in [
            ("gw_world_shields_up.inl", "npc_party_defense_effects_.insert_or_assign"),
            ("gw_world_simulation.h", "npc_party_defense_effects_.erase(effect)"),
            ("gw_world_simulation.h", "std::erase_if(npc_party_defense_effects_"),
            ("gw_world_simulation.h", "npc_party_members_.erase(retired)"),
            ("gw_npc_combat_catalog.h", "skills::effects::dont_trip_active(hexes_,hex_scope(),now)"),
            ("gw_world_simulation.h", "skills::effects::dont_trip_active(*player.effect_registry,player.effect_scope,now)"),
            ("gw_cast_timing.h", "skill.id==2216&&effects::dont_trip(0).has_value()"),
            ("gw_party_defense_shouts.h", "return prepare_dont_trip("),
        ]:
            def tampered(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "missing_consumer", 1) if path.name == name else text
            with self.subTest(file=name, token=token):
                with patch.object(Path, "read_text", tampered):
                    with self.assertRaises(ValueError):
                        dispatcher_ids(runtime)

    def test_vocal_minority_requires_admission_completion_and_publication(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        self.assertIn(883, dispatcher_ids(runtime))
        for token in ["effects::control_skill_forbidden", "hex->skill==883",
                      "hooks.degeneration_hex_completed(*p.resurrection_scope,*hex,spell_effect_event,now)"]:
            with self.subTest(token=token), self.assertRaisesRegex(ValueError, "Vocal Minority"):
                dispatcher_ids(runtime.replace(token, "missing_consumer"))
        original = Path.read_text
        for filename, token in [("gw_control_hexes.h", "vocal_forbidden"),
                                ("gw_npc_combat_catalog.h", "skills::effects::control_skill_forbidden"),
                                ("gw_world_simulation.h", "if(publication)defer_or_publish_completion_locked(publication,[queues]"),
                                ("gw_auth.cpp", ".degeneration_hex_completed =")]:
            def incomplete(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "missing_consumer") if path.name == filename else text
            with self.subTest(file=filename), patch.object(Path, "read_text", incomplete):
                with self.assertRaisesRegex(ValueError, "Vocal Minority"):
                    dispatcher_ids(runtime)

    def test_signet_corruption_requires_receipt_commit_with_damage_context(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        self.assertIn(1950, dispatcher_ids(runtime))
        token = "commit_player_resolved_hit_locked(p,incoming,cause,now,&actual,nullptr,true,npc_visibility,armor_respecting)"
        original = Path.read_text
        def incomplete(path, *args, **kwargs):
            text = original(path, *args, **kwargs)
            return text.replace(token, "missing_damage_commit") if path.name == "gw_world_reversal_player_hit.inl" else text
        with patch.object(Path, "read_text", incomplete):
            with self.assertRaisesRegex(ValueError, "Signet of Corruption"):
                dispatcher_ids(runtime)

    def test_ward_binding_requires_completion_hook(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        with self.assertRaises(ValueError):
            dispatcher_ids(runtime.replace("hooks.ward_elements(*plan,now)", "hooks.missing_ward(*plan,now)", 1))
        with self.assertRaises(ValueError):
            dispatcher_ids(runtime.replace("hooks.ward_melee(*plan,now)", "hooks.missing_melee_ward(*plan,now)", 1))
        with self.assertRaises(ValueError):
            dispatcher_ids(runtime.replace("hooks.ward_foes(*plan,now)", "hooks.missing_foes_ward(*plan,now)", 1))

    def test_shield_bash_requires_runtime_completion_consumer(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        self.assertIn(363, dispatcher_ids(runtime))
        with self.assertRaises(ValueError):
            dispatcher_ids(runtime.replace("completion_extra_seconds=15", "completion_extra_seconds=0", 1))

    def test_battle_rage_requires_runtime_ranked_plan(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        self.assertIn(317, dispatcher_ids(runtime))
        with self.assertRaises(ValueError):
            dispatcher_ids(runtime.replace("effects::battle_rage(effective[17])", "effects::battle_rage(0)", 1))

    def test_hekets_binding_requires_rank_and_early_end_snapshot(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        self.assertIn(1728, dispatcher_ids(runtime))
        for broken in [runtime.replace("effects::hekets_rampage(effective[22])", "effects::hekets_rampage(0)", 1),
                       runtime.replace("request.skill==1728&&(!effects::hekets_rampage(effective[22])||!hooks.status_mutation||!hooks.effect_snapshot)",
                                       "request.skill==1728&&(!effects::hekets_rampage(effective[22])||!hooks.status_mutation)", 1)]:
            with self.assertRaisesRegex(ValueError, "Heket's Rampage"):
                dispatcher_ids(broken)

    def test_succor_requires_actual_production_callbacks(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        self.assertIn(308, dispatcher_ids(runtime))
        for hook in ("succor", "succor_target_valid", "succor_spell_completed"):
            with self.assertRaises(ValueError):
                live_hook_bindings(runtime, gateway.replace(f".{hook} =", f".missing_{hook} =", 1))

    def test_bulls_charge_requires_runtime_ranked_plan_and_contact_consumers(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        self.assertIn(379, dispatcher_ids(runtime))
        with self.assertRaises(ValueError):
            dispatcher_ids(runtime.replace("effects::bulls_charge(effective[17])", "effects::bulls_charge(0)", 1))

    def test_weapon_spell_wiring_is_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        for hook in ["weapon_spell", "weapon_spell_target_valid"]:
            with self.assertRaises(ValueError):
                live_hook_bindings(runtime, gateway.replace(f".{hook} =", f".missing_{hook} =", 1))

    def test_snow_storm_admitted_and_typed_area_routes_are_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        damaged = runtime.replace("const bool captured_area_field=p.request.skill==2222||", "const bool captured_area_field=")
        with self.assertRaisesRegex(ValueError, "Snow Storm title/admitted"):
            live_hook_bindings(damaged, gateway)
        original = Path.read_text
        for name,token in (("gw_world_snow_storm.inl", "complete_ether_renewal_locked(source,2222,*event,now,true,false)"),
                           ("gw_world_area_damage_pulse.inl", "p.reviewed_lightning_armor,p.reviewed_fire_armor,p.reviewed_cold_armor")):
            def tampered(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_SNOW_CONSUMER") if path.name == name else text
            with patch.object(Path, "read_text", tampered):
                with self.assertRaisesRegex(ValueError, "Snow Storm title/admitted"):
                    live_hook_bindings(runtime, gateway)

    def test_glimmering_shared_area_reactions_remain_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        original = Path.read_text
        for token in ("project_batch_player_damage_locked(batch,p->second,actor,damage,at)",
                      "prepare_healing_seed_direct_locked(p->second,1.,at)",
                      "preflight_npc_reversal_or_damage_locked(preview,damage,at,nullptr,nullptr,nullptr,nullptr,recipient.opposed_owned)"):
            def tampered(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_AREA_CONSUMER") if path.name == "gw_world_area_damage_pulse.inl" else text
            with patch.object(Path, "read_text", tampered):
                with self.assertRaisesRegex(ValueError, "Glimmering shared area"):
                    dispatcher_ids(runtime)

    def test_glimmering_observer_requires_retained_guard(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        live_hook_bindings(runtime, gateway)
        damaged = runtime.replace("pending_->observe_glimmering=observe_glimmering;", "")
        with self.assertRaisesRegex(ValueError, "Glimmering Mark admitted"):
            live_hook_bindings(damaged, gateway)

    def test_live_wiring_omission_is_detected(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        result = live_hook_bindings(runtime, gateway)
        self.assertIn("completed_skill_use", result["explicit_bindings"])
        self.assertIn("ward_foes", result["explicit_bindings"])
        self.assertIn("recall_feedback_pending", result["explicit_bindings"])
        self.assertIn("heal_scoped", result["explicit_bindings"])
        self.assertIn("complete_life_steal", result["explicit_bindings"])
        self.assertIn("wary_stance_reward_ready", result["explicit_bindings"])
        self.assertIn(377, dispatcher_ids(runtime))
        self.assertIn(1076, dispatcher_ids(runtime))
        self.assertIn(902, dispatcher_ids(runtime))
        with self.assertRaises(ValueError):
            live_hook_bindings(runtime, gateway.replace(".complete_life_steal =", ".missing_blood_drinker =", 1))
        with self.assertRaises(ValueError):
            live_hook_bindings(runtime, gateway.replace(".wary_stance_reward_ready =", ".missing_wary_reward =", 1))
        self.assertIn("skill_roll", result["explicit_bindings"])
        self.assertIn("activation_multiplier", result["explicit_bindings"])
        self.assertIn("prepare_channeling", result["explicit_bindings"])
        self.assertIn("complete_channeling", result["explicit_bindings"])
        self.assertIn("enchantment_removal", result["explicit_bindings"])
        self.assertIn("hex_breaker", result["explicit_bindings"])
        self.assertIn("grenths_balance", result["explicit_bindings"])
        self.assertIn("take_grenths_reaction_death", result["explicit_bindings"])
        for name in ["consume_energy_at", "mind_wrack", "mind_wrack_targeted", "armor_earth", "panic", "panic_skill_used", "mantra_recall", "cry_frustration", "inspired_hex", "admit_session_cast", "valid_session_cast", "diversion", "prepare_skill_use", "finalize_skill_use", "abort_skill_use", "skill_disabled_slot"]:
            self.assertIn(name, result["explicit_bindings"])
        self.assertIn("watch_yourself", result["explicit_bindings"])
        self.assertIn("endure_pain", result["explicit_bindings"])
        self.assertIn("defy_pain", result["explicit_bindings"])
        self.assertIn("vigorous_spirit", result["explicit_bindings"])
        self.assertIn("charge", result["explicit_bindings"])
        self.assertIn("spear_light", result["explicit_bindings"])
        self.assertIn("disrupting_dagger", result["explicit_bindings"])
        self.assertIn("spirit_rift", result["explicit_bindings"])
        self.assertIn("discord", result["explicit_bindings"])
        for broken in [gateway.replace(".dont_trip =", ".missing_dont_trip =", 1),
                       gateway.replace("world.stage_dont_trip_party(owner,plan,now,&publication)", "false", 1),
                       gateway.replace(".party_heal_published =", ".missing_party_publication =", 1),
                       gateway.replace("world.party_heal(*skill_actor_scope,plan,now,event,&publication)", "false", 1),
                       gateway.replace(".flame_djinn_haste =", ".missing_flame_djinn =", 1),
                       gateway.replace("world.flame_djinn_haste(owner,plan,grant->level,now,&*duration,event)", "false", 1),
                       gateway.replace(".discord =", ".missing_discord =", 1),
                       gateway.replace(".spirit_rift =", ".missing_spirit_rift =", 1),
                       gateway.replace(".spear_light =", ".missing_spear_light =", 1),
                       gateway.replace(".charge =", ".missing_charge =", 1),
                       gateway.replace(".vigorous_spirit =", ".missing_vigorous_spirit =", 1),
                       gateway.replace(".defy_pain =", ".missing_defy_pain =", 1),
                       gateway.replace(".endure_pain =", ".missing_endure_pain =", 1),
                       gateway.replace(".watch_yourself =", ".missing_watch_yourself =", 1),
                       gateway.replace(".mending =", ".missing_mending =", 1),
                       gateway.replace(".mending_target_valid =", ".missing_mending_valid =", 1),
                       gateway.replace(".blessed_signet =", ".missing_blessed =", 1),
                       gateway.replace(".blessed_signet_ready =", ".missing_blessed_ready =", 1),
                       gateway.replace(".mark_subversion =", ".missing_mark =", 1),
                       gateway.replace(".targeted_spell_failure =", ".missing_failure =", 1),
                       gateway.replace(".soul_leech =", ".missing_soul_leech =", 1),
                       gateway.replace(".icy_prism =", ".missing_icy_prism =", 1),
                       gateway.replace(".action_damage_hex =", ".missing_action_hex =", 1),
                       gateway.replace(".heal_scoped =", ".wrong_hook =", 1),
                       gateway.replace(".heal_scoped =", ".holy_damage =", 1),
                       gateway.replace(".heal_scoped = [&]", ".heal_scoped = {}", 1),
                       gateway.replace(".hex_breaker =", ".missing_hex_breaker =", 1),
                       gateway.replace(".grenths_balance =", ".missing_balance =", 1),
                       gateway.replace(".take_grenths_reaction_death =", ".missing_death_proof =", 1),
                       gateway.replace(".finalize_skill_use =", ".missing_use_commit =", 1),
                       gateway.replace(".inspired_hex =", ".missing_copy_commit =", 1),
                       gateway.replace(".cry_frustration =", ".missing_cry_commit =", 1),
                       gateway.replace(".mantra_recall =", ".missing_recall_commit =", 1),
                       gateway.replace(".armor_earth =", ".missing_armor_earth_commit =", 1),
                       gateway.replace(".armor_earth_completed =", ".missing_armor_earth_completed =", 1),
                       gateway.replace(".simple_enchantment_completed =", ".missing_simple_enchantment_completed =", 1),
                       gateway.replace("world.complete_aegis(source,p,divine,now,d,event)", "world.complete_aegis(source,p,divine,now,d)", 1),
                       gateway.replace("world.resurrect_target(owner,target,*plan,now,event,&result.publication)", "world.resurrect_target(owner,target,*plan,now)", 1),
                       gateway.replace("world.healing_ring(owner,plan,now,event,&result.publication)", "world.healing_ring(owner,plan,now)", 1),
                       gateway.replace("world.channeling_enchantment(source,rank,now,event,&*duration,&completed_at)", "world.channeling_enchantment(source,rank,now)", 1),
                       gateway.replace("world.aura_restoration_enchantment(source,skill,rank,now,event,&*duration,&completed_at)", "world.aura_restoration_enchantment(source,skill,rank,now)", 1),
                       gateway.replace("world.attunement_enchantment(source,p,now,event,d,&completed_at)", "world.attunement_enchantment(source,p,now)", 1),
                       gateway.replace("if(accepted&&completed_at)result.effect_snapshot_at=completed_at;", "", 1),
                       gateway.replace("world.mantra_recall(source,rank,now,event,d)", "world.mantra_recall(source,rank,now)", 1),
                       gateway.replace("const bool prefix_published=fw::gw::skills::casting::owned_self_enchantment_base(skill,rank)==0;", "const bool prefix_published=true;", 1),
                       gateway.replace("world.complete_healing_seed(source,target,p,divine,now,d,event)", "world.complete_healing_seed(source,target,p,divine,now,d)", 1),
                       gateway.replace("world.armor_earth(owner,rank,now,event,&*duration)", "world.armor_earth(owner,rank,now)", 1),
                       gateway.replace(".consume_energy_at =", ".missing_timed_payment =", 1),
                       gateway.replace(".mind_wrack =", ".missing_wrack_apply =", 1),
                       gateway.replace(".mind_wrack_targeted =", ".missing_wrack_observer =", 1),
                       gateway.replace(".panic_skill_used =", ".missing_panic_observer =", 1),
                       gateway.replace(".valid_session_cast =", ".missing_copy_validation =", 1)]:
            with self.assertRaises(ValueError):
                live_hook_bindings(runtime, broken)

    def test_attunement_refund_requires_settled_snapshot_time(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        for broken in (
            runtime.replace("hooks.effect_snapshot(completion_snapshot_at)", "hooks.effect_snapshot(now)", 1),
            runtime.replace("attunement_cast_refund(active,*skill,completion_snapshot_at)", "attunement_cast_refund(active,*skill,now)", 1),
        ):
            with self.assertRaisesRegex(ValueError, "Shared enchantment"):
                live_hook_bindings(broken, gateway)

    def test_aura_channeling_observers_require_authority_routes(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        for broken in (
            runtime.replace("aura->skill==3375", "false", 1),
            runtime.replace("hooks.complete_channeling(channeling.event,true,completion_snapshot_at)",
                            "hooks.complete_channeling(channeling.event,true,now)", 1),
        ):
            with self.assertRaisesRegex(ValueError, "Aura/Channeling"):
                live_hook_bindings(broken, gateway)

    def test_stoneflesh_requires_deferred_captured_completion(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        broken = runtime.replace("deferred_stoneflesh=p.request.skill==1375&&p.simple_enchantment_completion;",
                                 "deferred_stoneflesh=false;", 1)
        with self.assertRaisesRegex(ValueError, "Shared enchantment"):
            live_hook_bindings(broken, gateway)

    def test_stoneflesh_legacy_path_keeps_status_and_duration_guards(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        start = runtime.index("if(request.skill==1375&&")
        end = runtime.index(";", start)
        for token in ("!hooks.status_mutation", "!hooks.local_enchantment_duration"):
            broken = runtime[:start] + runtime[start:end].replace(token, "false") + runtime[end:]
            with self.assertRaisesRegex(ValueError, "Stoneflesh production"):
                dispatcher_ids(broken)

    def test_burning_completion_requires_event_and_application_clock(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        with self.assertRaisesRegex(ValueError, "Burning Speed admitted"):
            live_hook_bindings(runtime, gateway.replace("&result.publication,event,&completed_at", "&result.publication,{},&completed_at", 1))
        with self.assertRaisesRegex(ValueError, "Burning Speed admitted"):
            live_hook_bindings(runtime.replace("hooks.effect_snapshot(effect_applied)", "hooks.effect_snapshot(now)", 1), gateway)
        with self.assertRaisesRegex(ValueError, "Burning Speed admitted"):
            live_hook_bindings(runtime.replace("if(p.burning_speed_completion)effects_.erase(cache_skill);", "", 1), gateway)

    def test_echo_requires_production_hook_and_energy_hexes_stay_staged(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        result = live_hook_bindings(runtime, gateway)
        self.assertEqual(set(result["intentional_empty"]), { "actor_generation", "energy_degeneration_hex", "recall", "recall_target_valid", "mimicry", "mimicry_target_valid", "well_corpse", "necrotic_available", "necrotic_traversal", "divine_spirit", "aneurysm", "reversal_target_valid", "reversal_fortune_completed"})
        self.assertIn(1261, dispatcher_ids(runtime))
        self.assertTrue({74,75}.issubset(dispatcher_ids(runtime)))
        for skill in (65, 97, 125, 140, 925, 1336):
            enabled = runtime.replace("static bool supported(unsigned id){return ", f"static bool supported(unsigned id){{return id=={skill}||", 1)
            with patch.dict(FAMILIES, {skill: "test-only staged Echo"}):
                with self.assertRaisesRegex(ValueError, "without production hooks"):
                    live_hook_bindings(enabled, gateway)
        with self.assertRaises(ValueError):
            live_hook_bindings(runtime, gateway.replace(".arm_echo =", ".missing_echo_arm =", 1))
        start=gateway.index(".arm_echo =")
        end=gateway.index(".completed_skill_use =",start)
        with self.assertRaises(ValueError):
            live_hook_bindings(runtime,gateway[:start]+".arm_echo = {},\n"+gateway[end:])
        with self.assertRaises(ValueError):
            live_hook_bindings(runtime, gateway.replace(".energy_degeneration_hex = {},", "", 1))
        for name in ("mimicry", "mimicry_target_valid"):
            with self.assertRaises(ValueError):
                live_hook_bindings(runtime, gateway.replace(f".{name} = {{}},", "", 1))
        with self.assertRaises(ValueError):
            live_hook_bindings(runtime, gateway.replace(".aneurysm = {},", "", 1))

        with self.assertRaises(ValueError):
            live_hook_bindings(runtime, gateway.replace(".chilblains =", ".unrecognized_chilblains_hook =", 1))

        with self.assertRaises(ValueError):
            live_hook_bindings(runtime, gateway.replace(".watchful_healing =", ".unrecognized_watchful_hook =", 1))

    def test_judgment_requires_both_live_callbacks(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        for name in ("shield_judgment", "shield_judgment_ready", "signet_judgment", "signet_judgment_ready"):
            with self.assertRaises(ValueError):
                live_hook_bindings(runtime, gateway.replace(f".{name} =", f".missing_{name} =", 1))

    def test_real_binding_is_not_an_unlock_or_metadata(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        ids = dispatcher_ids(runtime)
        for skill in [290, 297, 294, 2887, 262, 261, 259, 245, 144, 1392, 127, 128, 903, 898, 899, 959, 49, 2734, 165, 52, 82, 57, 22, 1049, 10, 1, 2, 281, 1120, 305, 314, 1128, 304, 171, 937, 26, 105, 115, 814, 135, 180, 195, 197, 200, 230, 346, 394, 396, 407, 431, 432, 27, 76, 330, 299, 252, 28, 100, 247, 1113,
                      6, 7, 8, 9, 13, 14, 15, 16, 17, 19, 35, 36, 37, 38, 43, 47, 53, 54, 56, 66, 67, 68, 69, 110, 129, 132, 136, 141, 142, 143, 292, 337, 338, 339, 349, 385, 391, 392, 397, 404, 1136, 146, 155, 159, 1396, 1401, 2057, 881, 1365, 2998, 3373, 2061]:
            self.assertIn(skill, ids)
        # Existing constants and legitimate player unlocks are insufficient.
        self.assertIn(11, ids)
        self.assertIn(33, ids)
        self.assertIn(455, ids)
        self.assertIn(316, ids)
        self.assertIn(303, ids)
        self.assertIn(298, ids)
        self.assertIn(254, ids)
        self.assertIn(364, ids)
        for skill in [0, 3, 12, 18, 306, 310, 325, 1263, 1993, 2007]:
            self.assertNotIn(skill, ids)
        self.assertEqual(ids[304], "party area resurrection")
        self.assertEqual(ids[1128], "targeted resurrection")

    def test_chilblains_shared_damage_consumers_remain_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        original = Path.read_text
        for token in [
            "preflight_npc_reversal_or_damage_locked(projected,npc_damage,now)",
            "commit_npc_reversal_or_damage_locked(owner->second,placed->second,npc_combat_.at(expected.actor),npc_damage,now",
            "prepare_player_reactive_seed_locked(p,hit,now)",
            "commit_player_reactive_hit_locked(p,hit,source.actor,now",
            "entry_sequence(scope,removal.entry.instance)!=removal.sequence",
            "commit_enchantment_removal(removal,now)",
            "chilblains_poison(plan,owner.conditions,source.actor,now)",
        ]:
            def incomplete(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_CHILBLAINS_CONSUMER") if path.name == "gw_world_chilblains.inl" else text
            with self.subTest(consumer=token), patch.object(Path, "read_text", incomplete):
                with self.assertRaisesRegex(ValueError, "Chilblains shared damage/removal consumers are incomplete"):
                    dispatcher_ids(runtime)

    def test_corruption_shared_damage_consumers_remain_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        original = Path.read_text
        for filename, token in [
            ("gw_world_signet_of_corruption.inl", "commit_player_reactive_hit_locked(victim,hit,source_scope.actor,now)"),
            ("gw_world_signet_of_corruption.inl", "prepare_player_reactive_seed_locked(p->second,hit,now)"),
            ("gw_world_reversal_player_hit.inl", "commit_player_resolved_hit_locked(p,incoming,cause,now,&actual,nullptr,true,npc_visibility,armor_respecting)"),
            ("gw_world_reversal_player_hit.inl", "actual&&actual->applied>0&&seed->effect"),
            ("gw_world_signet_of_corruption.inl", "commit_npc_reversal_or_damage_locked(caster,body->second,n->second,damage,now)"),
            ("gw_world_reversal_player_hit.inl", "combat::damage_packets_2022(*cause,hit)"),
            ("gw_world_reversal_player_hit.inl", "cause?combat::damage_packets_2022(*cause,hit):combat::health_packets_2022("),
            ("gw_world_reversal_npc.inl", "reversal_damage_packets_2022(target.actor,cause,*receipt,reversal_healing_source_locked(target,receipt->removed))"),
            ("gw_world_reversal_npc.inl", "source?std::optional<unsigned>(source->id):npc_damage_attribution_locked(actor_cause)"),
        ]:
            def incomplete(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_DAMAGE_CONSUMER") if path.name == filename else text
            with self.subTest(consumer=token), patch.object(Path, "read_text", incomplete):
                with self.assertRaisesRegex(ValueError, "Signet of Corruption production effect consumers are incomplete"):
                    dispatcher_ids(runtime)

    def test_holy_shared_damage_consumers_remain_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text(encoding="utf-8")
        original = Path.read_text
        for token in ["admit_player_damage_sequence_locked", "commit_player_reactive_hit_locked",
                      "begin_damage_publication_locked", "finish_damage_publication_locked",
                      "commit_npc_reversal_or_damage_locked(source_at()"]:
            def incomplete(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_HOLY_CONSUMER") if path.name == "gw_world_holy_damage.inl" else text
            with self.subTest(consumer=token), patch.object(Path, "read_text", incomplete):
                with self.assertRaisesRegex(ValueError, "Holy shared damage consumers are incomplete"):
                    dispatcher_ids(runtime)

    def test_holy_paid_publication_remains_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text(encoding="utf-8")
        first = runtime.index("}else if(holy){")
        last = runtime.index("}else if(feast){", first)
        for token in ["completion_publication=std::move(result.publication)",
                      "early_completion_visuals=result.visuals_before_publication"]:
            changed = runtime[:first] + runtime[first:last].replace(token, "missing_holy_publication") + runtime[last:]
            with self.subTest(consumer=token), self.assertRaisesRegex(ValueError, "Holy shared damage consumers are incomplete"):
                dispatcher_ids(changed)

    def test_swarm_shared_damage_consumers_remain_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text(encoding="utf-8")
        original = Path.read_text
        for token in ["admit_player_damage_sequence_locked(player->second,{damage},now,actor_at)",
                      "commit_player_reactive_hit_locked(p,hit,source_scope.actor,now",
                      "std::vector<skills::effects::Scope> npc_scopes"]:
            def incomplete(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_SWARM_CONSUMER") if path.name == "gw_world_deathly_swarm.inl" else text
            with self.subTest(consumer=token), patch.object(Path, "read_text", incomplete):
                with self.assertRaisesRegex(ValueError, "Deathly Swarm delayed damage consumers are incomplete"):
                    dispatcher_ids(runtime)

    def test_elemental_adapter_binding_is_live_and_dynamic(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        for token in ('#include "gw_elemental_completion.h"',
                      '.elemental_completed = fw::gw::skills::casting::make_admitted_elemental_completion(world,[&]{return skill_actor_scope;},[&]{return grant->level;}),'):
            self.assertIn(token, gateway)
            for replacement in ("", "/*" + token + "*/", '"' + token + '";'):
                with self.subTest(token=token, replacement=replacement):
                    with self.assertRaisesRegex(ValueError, "Elemental production adapter binding"):
                        live_hook_bindings(runtime, gateway.replace(token, replacement, 1))
        with self.assertRaisesRegex(ValueError, "Elemental production adapter binding"):
            live_hook_bindings(runtime, gateway.replace('[&]{return grant->level;}', 'grant->level', 1))

    def test_elemental_adapter_consumers_must_be_executable(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        original = Path.read_text
        for token in ('const auto source=owner();if(!source)return {};',
                      'level()', '&visuals,nullptr,&outcome,event,&publication',
                      'if(!world.elemental(*source,target,plan,level(),now,&visuals,nullptr,&outcome,event,&publication))return {};',
                      'return ConditionalElementalCompletion{outcome,std::move(visuals),std::move(publication),true};'):
            self.assertIn(token, original(ROOT / "src/runtime/gw_elemental_completion.h"))
            for replacement in ('', '/*' + token + '*/', '"' + token + '";'):
                def incomplete(path, *args, **kwargs):
                    text = original(path, *args, **kwargs)
                    return text.replace(token, replacement, 1) if path.name == 'gw_elemental_completion.h' else text
                with self.subTest(token=token, replacement=replacement), patch.object(Path, 'read_text', incomplete):
                    with self.assertRaisesRegex(ValueError, 'Elemental production adapter consumers'):
                        dispatcher_ids(runtime)

    def test_elemental_adapter_rejects_partial_family_and_early_publication(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        original = Path.read_text
        for before, after in (('const auto source=owner();', 'if(plan.skill!=919)return {};const auto source=owner();'),
                              ('std::move(publication),true', 'std::move(publication),false'),
                              ('return ConditionalElementalCompletion', 'publication();return ConditionalElementalCompletion'),
                              ('level()', '20')):
            def incomplete(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(before, after, 1) if path.name == 'gw_elemental_completion.h' else text
            with self.subTest(change=after), patch.object(Path, 'read_text', incomplete):
                with self.assertRaisesRegex(ValueError, 'Elemental production adapter consumers'):
                    dispatcher_ids(runtime)

    def test_elemental_shared_damage_consumers_remain_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text(encoding="utf-8")
        original = Path.read_text
        for filename, token in [
            ("gw_elemental_damage.h", "plan.kind==combat::Kind::cold||plan.kind==combat::Kind::lightning"),
            ("gw_world_elemental_impact.inl", "project_batch_player_damage_locked(batch,*p,current,damage,now)"),
            ("gw_world_elemental_impact.inl", "project_batch_condition_reactions_locked(batch,*p,p->conditions,condition->first,now)"),
            ("gw_world_elemental_impact.inl", "commit_player_reactive_hit_locked(*live,incoming,source.actor,now"),
            ("gw_world_elemental_impact.inl", "n->second.active_action(life,now)==action"),
            ("gw_world_simulation.h", "const auto active=current->newest_stance(life,now)"),
            ("gw_world_elemental.inl", "begin_damage_publication_locked(pending)"),
            ("gw_world_elemental.inl", "finish_damage_publication_locked(pending)"),
        ]:
            def incomplete(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_ELEMENTAL_CONSUMER") if path.name == filename else text
            with self.subTest(consumer=token), patch.object(Path, "read_text", incomplete):
                with self.assertRaisesRegex(ValueError, "Elemental shared damage/publication consumers are incomplete"):
                    dispatcher_ids(runtime)

    def test_earthquake_shared_damage_consumers_remain_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text(encoding="utf-8")
        original = Path.read_text
        for filename, token in [
            ("gw_world_earthquake.inl", "project_batch_player_damage_locked(batch,p,current,hit,now)"),
            ("gw_world_earthquake.inl", "prepare_healing_seed_direct_locked(p->second,1.,now)"),
            ("gw_world_earthquake.inl", "commit_player_reactive_hit_locked(p,hit,scope.actor,now"),
            ("gw_world_earthquake.inl", "begin_damage_publication_locked(pending)"),
            ("gw_world_elemental.inl", "earthquake(scope,target,*q,level,now,completion_visuals,completion_event,publication)"),
        ]:
            def incomplete(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_QUAKE_CONSUMER") if path.name == filename else text
            with self.subTest(consumer=token), patch.object(Path, "read_text", incomplete):
                with self.assertRaisesRegex(ValueError, "Earthquake shared damage/control/publication consumers are incomplete"):
                    dispatcher_ids(runtime)

    def test_conditional_damage_shared_consumers_remain_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text(encoding="utf-8")
        original = Path.read_text
        for filename, token in [
            ("gw_world_condition_damage.inl", "project_batch_player_damage_locked(batch,p,actor,damage,now)"),
            ("gw_world_condition_damage.inl", "project_batch_condition_reactions_locked(batch,p,p.conditions,conditions,now)"),
            ("gw_world_condition_damage.inl", "commit_player_reactive_hit_locked(p,hit,scope.actor,now"),
            ("gw_world_condition_damage.inl", "begin_damage_publication_locked(pending)"),
            ("gw_auth.cpp", "world.condition_damage(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication)"),
        ]:
            def incomplete(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_CONDITION_CONSUMER") if path.name == filename else text
            with self.subTest(consumer=token), patch.object(Path, "read_text", incomplete):
                with self.assertRaisesRegex(ValueError, "Conditional damage shared reaction/publication consumers are incomplete"):
                    dispatcher_ids(runtime)

    def test_shadow_damage_shared_consumers_remain_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text(encoding="utf-8")
        original = Path.read_text
        for filename, token in [
            ("gw_world_shadow_damage.inl", "admit_player_damage_sequence_locked(victim,{damage},now,actor_at,&projected)"),
            ("gw_world_shadow_damage.inl", "commit_player_reactive_hit_locked(victim,hit,scope.actor,now"),
            ("gw_world_shadow_damage.inl", "begin_damage_publication_locked(pending)"),
            ("gw_world_shadow_damage.inl", "finish_damage_publication_locked(pending)"),
            ("gw_auth.cpp", "world.shadow_damage(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication)"),
        ]:
            def incomplete(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_SHADOW_CONSUMER") if path.name == filename else text
            with self.subTest(consumer=token), patch.object(Path, "read_text", incomplete):
                with self.assertRaisesRegex(ValueError, "Shadow damage shared reaction/publication consumers are incomplete"):
                    dispatcher_ids(runtime)

    def test_cry_shared_consumers_remain_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text(encoding="utf-8")
        original = Path.read_text
        for filename, token in [
            ("gw_world_cry_frustration.inl", "project_batch_player_damage_locked(batch,p,actor,damage,now)"),
            ("gw_world_cry_frustration.inl", "prepare_healing_seed_direct_locked(found->second,1.,now)"),
            ("gw_world_cry_frustration.inl", "commit_player_reactive_hit_locked(victim,hit,scope.actor,now"),
            ("gw_world_cry_frustration.inl", "begin_damage_publication_locked(pending)"),
            ("gw_auth.cpp", "world.cry_frustration(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication)"),
        ]:
            def incomplete(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_CRY_CONSUMER") if path.name == filename else text
            with self.subTest(consumer=token), patch.object(Path, "read_text", incomplete):
                with self.assertRaisesRegex(ValueError, "Cry damage/control/publication consumers are incomplete"):
                    dispatcher_ids(runtime)

    def test_putrid_bile_death_consumers_remain_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text(encoding="utf-8")
        original = Path.read_text
        for filename, token in [
            ("gw_world_putrid_bile.inl", "putrid_bile_events_.reserve(link,plane,x,y,now,std::move(population))"),
            ("gw_world_putrid_bile.inl", "project_batch_player_damage_locked(batch,p,actor,damage,now)"),
            ("gw_world_putrid_bile.inl", "project_putrid_tree_locked(scope,graph,batch,exploded,now)"),
            ("gw_world_putrid_bile.inl", "prepare_healing_seed_direct_locked(p->second,1.,event.at)"),
            ("gw_world_putrid_bile.inl", "commit_player_reactive_hit_locked(p,hit,event.link.caster.actor,event.at"),
            ("gw_world_simulation.h", "if(!dispatch_putrid_deaths_locked(boundary))return false;"),
        ]:
            def incomplete(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_BILE_CONSUMER") if path.name == filename else text
            with self.subTest(consumer=token), patch.object(Path, "read_text", incomplete):
                with self.assertRaisesRegex(ValueError, "Putrid Bile death ownership/reaction consumers are incomplete"):
                    dispatcher_ids(runtime)

    def test_whirlwind_shared_consumers_remain_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text(encoding="utf-8")
        original = Path.read_text
        for filename, token in [
            ("gw_world_whirlwind.inl", "DamageAdmissionBatch batch;batch.seed_facts=&facts"),
            ("gw_world_whirlwind.inl", "project_batch_player_damage_locked(batch,p,current,hit,now)"),
            ("gw_world_whirlwind.inl", "prepare_healing_seed_direct_locked(p->second,1.,now)"),
            ("gw_world_whirlwind.inl", "commit_player_reactive_hit_locked(p,hit,caster.id,now"),
            ("gw_world_elemental.inl", "whirlwind_locked(scope,plan,level,now,feedback_visuals,completion_event,publication!=nullptr)"),
        ]:
            def incomplete(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_WHIRLWIND_CONSUMER") if path.name == filename else text
            with self.subTest(consumer=token), patch.object(Path, "read_text", incomplete):
                with self.assertRaisesRegex(ValueError, "Whirlwind shared reaction/publication consumers are incomplete"):
                    dispatcher_ids(runtime)

    def test_interrupt_shared_consumers_remain_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text(encoding="utf-8")
        original = Path.read_text
        for filename, token in [
            ("gw_world_interrupt_skill.inl", "admit_player_damage_sequence_locked(victim,{damage},now,actor_at)"),
            ("gw_world_interrupt_skill.inl", "commit_player_reactive_hit_locked(victim,hit,scope.actor,now"),
            ("gw_world_interrupt_skill.inl", "begin_damage_publication_locked(pending)"),
            ("gw_world_interrupt_skill.inl", "finish_damage_publication_locked(pending)"),
            ("gw_auth.cpp", "world.interrupt_skill(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication)"),
        ]:
            def incomplete(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "MISSING_INTERRUPT_CONSUMER") if path.name == filename else text
            with self.subTest(consumer=token), patch.object(Path, "read_text", incomplete):
                with self.assertRaisesRegex(ValueError, "Interrupt damage/control/publication consumers are incomplete"):
                    dispatcher_ids(runtime)

    def test_unknown_dispatcher_forms_fail_instead_of_inflate_coverage(self):
        for expression in ["true", "id>0", "other_engine(id)", "bool(effects::unreviewed(id))"]:
            source = "static bool supported(unsigned id){return " + expression + ";}"
            with self.assertRaises((ValueError, KeyError)):
                dispatcher_ids(source)


    def test_life_steal_production_adapter_cannot_reintroduce_partial_family_filter(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        live_hook_bindings(runtime, gateway)
        with self.assertRaisesRegex(ValueError, "Life-steal production family adapter"):
            live_hook_bindings(runtime, gateway.replace("make_life_steal_completion(world", "missing_adapter(world", 1))
        with self.assertRaisesRegex(ValueError, "Life-steal production family adapter"):
            live_hook_bindings(runtime.replace("pending_->life_steal_completion=life_steal_completion;", "pending_->life_steal_completion=false;", 1), gateway)
        original = Path.read_text
        def partial(path, *args, **kwargs):
            text = original(path, *args, **kwargs)
            return text.replace("accepted=world.life_steal(", "accepted=(plan.skill==1076||plan.skill==902)&&world.life_steal(", 1) if path.name == "gw_life_steal_completion.h" else text
        with patch.object(Path, "read_text", partial):
            with self.assertRaisesRegex(ValueError, "Life-steal production family adapter"):
                live_hook_bindings(runtime, gateway)

    def test_life_steal_admitted_completion_route_is_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        with self.assertRaisesRegex(ValueError, "Life-steal admitted spell completion"):
            live_hook_bindings(runtime, gateway.replace("make_admitted_life_steal_completion(world", "missing_admitted(world", 1))
        with self.assertRaisesRegex(ValueError, "Life-steal admitted spell completion"):
            live_hook_bindings(runtime.replace("pending_->life_steal_spell_completion=life_steal_spell_completion;", "pending_->life_steal_spell_completion=false;", 1), gateway)

    def test_accumulated_pain_admitted_completion_route_is_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        for broken_runtime, broken_gateway in (
            (runtime, gateway.replace("make_wound_spell_completion(world", "missing_completion(world", 1)),
            (runtime.replace("pending_->wound_spell_completion=wound_spell_completion;", "pending_->wound_spell_completion=false;", 1), gateway),
        ):
            with self.assertRaisesRegex(ValueError, "Accumulated Pain admitted completion"):
                live_hook_bindings(broken_runtime, broken_gateway)

    def test_direct_condition_equipment_route_is_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        live_hook_bindings(runtime, gateway)
        for broken in (gateway.replace("world.condition_spell_equipped(", "world.condition_spell(", 1),
                       gateway.replace("!plan.spread&&(plan.kind==", "(plan.kind==", 1)):
            with self.assertRaisesRegex(ValueError, "equipment route missing"):
                live_hook_bindings(runtime, broken)


    def test_condition_completion_publication_route_is_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        live_hook_bindings(runtime, gateway)
        for broken in (gateway.replace("world.complete_condition_spell(", "world.condition_spell(", 1),
                       gateway.replace("event?std::optional<std::uint64_t>{event}:std::nullopt", "std::nullopt", 1),
                       gateway.replace("EffectCompletion{accepted,std::move(visuals),{},std::move(publication),true};", "EffectCompletion{accepted,std::move(visuals),{},std::move(publication),false};", 1)):
            with self.assertRaisesRegex(ValueError, "completion route missing"):
                live_hook_bindings(runtime, broken)
        broken = runtime.replace("hooks.condition_spell_completed(*p.resurrection_scope,*condition,spell_effect_event,now)",
                                 "hooks.condition_spell_completed(*p.resurrection_scope,*condition,0,now)", 1)
        with self.assertRaisesRegex(ValueError, "completion route missing"):
            live_hook_bindings(broken, gateway)


    def test_ward_production_completion_and_control_routes_are_required(self):
        runtime = (ROOT / "src/runtime/gw_skill_casting.h").read_text()
        gateway = (ROOT / "src/apps/gw_auth.cpp").read_text()
        self.assertIn(938, dispatcher_ids(runtime))
        live_hook_bindings(runtime, gateway)
        for r, g in (
            (runtime, gateway.replace("make_defensive_ward_completion(world", "missing_completion(world", 1)),
            (runtime.replace("hooks.defensive_ward_completed(*defensive,spell_effect_event,now)", "hooks.defensive_ward_completed(*defensive,0,now)", 1), gateway),
            (runtime.replace("!(p.request.skill==938&&completion_visuals.empty())", "true", 1), gateway),
        ):
            with self.assertRaisesRegex(ValueError, "Ward938 production"):
                live_hook_bindings(r, g)
        original = Path.read_text
        for filename, token in (
            ("gw_world_wards.inl", "actor.environmental_knockdown_protection([this]"),
            ("gw_world_wards.inl", "refresh_npc_movement_locked(scope.actor,now)"),
            ("gw_world_simulation.h", "if(ward_knockdown_protection_locked(player.effect_scope,now))return true;"),
        ):
            def incomplete(path, *args, **kwargs):
                text = original(path, *args, **kwargs)
                return text.replace(token, "missing_consumer", 1) if path.name == filename else text
            with self.subTest(consumer=token), patch.object(Path, "read_text", incomplete):
                with self.assertRaisesRegex(ValueError, "Ward938 production"):
                    live_hook_bindings(runtime, gateway)


if __name__ == "__main__":
    unittest.main()


