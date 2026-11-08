// Include inside Instance after hostile drivers and wander state definitions.
public:
    enum class NpcPopulationEnrollment { armed, already_armed, identity_mismatch, unknown_combat, conflicting_controller, deferred, invalid_policy };
    // Exact content opt-in, not model/level-based automatic hostile discovery.
    // Callers provide authored/reviewed metadata separately from behavior policy.
    // A repeated call acknowledges the same configuration without resetting a
    // paid action, target memory, home, recharge, Energy or patrol deadline.
    NpcPopulationEnrollment configure_npc_population_elemental(skills::effects::Scope source,
            const combat::npcs::Binding& expected,combat::ai::FlarePolicy policy,
            std::vector<combat::ai::elemental::Reviewed> spells,combat::ai::wander::Policy patrol,std::uint64_t now){
        using Result=NpcPopulationEnrollment;
        if(policy.source!=source||policy.target.instance||policy.target.actor||policy.target.generation||
           !policy.valid()||!patrol.valid(policy.leash))return Result::invalid_policy;
        std::optional<combat::ai::elemental::Ledger> proposed;
        try{proposed.emplace(source,policy.level,std::move(spells),policy.flight,policy.max_flights);}
        catch(const std::invalid_argument&){return Result::invalid_policy;}
        std::lock_guard lock(mutex_);
        const auto body=npcs_.find(source.actor);const auto live=npc_combat_.find(source.actor);
        if(body==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=source||
           !body->second.combat_definition||body->second.combat_definition->binding!=expected||
           expected.actor!=source.actor||expected.level!=policy.level||body->second.definition.level!=policy.level)
            return Result::identity_mismatch;
        if(quest_npc_owners_.contains(source.actor)||quest_npc_motion_.contains(source.actor)||
           npc_flare_ai_.contains(source.actor)||npc_ordinary_ai_.contains(source.actor))return Result::conflicting_controller;
        const auto life=live->second.lifetime();const auto actor=live->second.actor(life);
        const auto& metadata=*body->second.combat_definition;
        if(!actor||!actor->health||actor->health->current<=0||!live->second.hostile_to_players(life)||
           !live->second.energy(life)||!metadata.energy_pips||!metadata.physical_armor||!metadata.earth_armor||
           !metadata.fire_armor||!metadata.cold_armor||!metadata.lightning_armor||!metadata.general_armor)
            return Result::unknown_combat;
        if(now<live->second.health_clock(life).value_or(now)||now<live->second.energy_clock(life).value_or(now))return Result::deferred;
        const auto old=npc_elemental_ai_.find(source.actor);
        if(old!=npc_elemental_ai_.end()){
            const auto& motion=old->second.motion;const auto& p=motion.policy;
            const auto wander=npc_wander_.find(source.actor);
            if(p.source!=source||old->second.owned_quest||wander==npc_wander_.end()||wander->second.source!=source||
               p.proximity_acquisition!=policy.proximity_acquisition||p.level!=policy.level||p.fire_magic!=policy.fire_magic||p.speed!=policy.speed||p.acquire!=policy.acquire||
               p.follow!=policy.follow||p.leash!=policy.leash||p.flight.speed!=policy.flight.speed||
               p.flight.hit_radius!=policy.flight.hit_radius||p.max_flights!=policy.max_flights||p.evidence!=policy.evidence)
                return Result::conflicting_controller;
            const auto& previous=wander->second.state.policy();
            if(previous.pause_ms!=patrol.pause_ms||previous.leg_timeout_ms!=patrol.leg_timeout_ms||
               previous.evidence!=patrol.evidence||previous.offsets.size()!=patrol.offsets.size())return Result::conflicting_controller;
            for(std::size_t i=0;i<patrol.offsets.size();++i)
                if(previous.offsets[i].x!=patrol.offsets[i].x||previous.offsets[i].y!=patrol.offsets[i].y)return Result::conflicting_controller;
            const auto before=old->second.ledger.reviewed_spells(),after=proposed->reviewed_spells();
            if(before.size()!=after.size())return Result::conflicting_controller;
            for(std::size_t i=0;i<before.size();++i){
                if(before[i].slot!=after[i].slot||before[i].range!=after[i].range||before[i].evidence!=after[i].evidence||
                   before[i].kind!=after[i].kind||before[i].targeting!=after[i].targeting)return Result::conflicting_controller;
                // Slot/copy, range and provenance do not encode effect rank.
                // Compare the complete canonical plan before acknowledging an
                // unchanged enrollment; never replace an active paid ledger.
                const auto old_plan=old->second.ledger.plan(before[i].slot),new_plan=proposed->plan(after[i].slot);
                if(!old_plan||!new_plan||*old_plan!=*new_plan)return Result::conflicting_controller;
            }
            return now<motion.updated?Result::deferred:Result::already_armed;
        }
        if(npc_wander_.contains(source.actor))return Result::conflicting_controller;
        if(live->second.active_action(life,now)||!live->second.ordinary_action_ready(life,now))return Result::deferred;
        auto prepared=live->second;
        if(!prepared.configure_spellbook(life,proposed->reviewed_spells()))return Result::invalid_policy;
        // All validation precedes mutation. The host cannot leave a caster
        // enabled without its bounded patrol because a later API call failed.
        live->second=std::move(prepared);
        npc_elemental_ai_.emplace(source.actor,NpcElementalController{{policy,body->second.x,body->second.y,body->second.plane,now},std::move(*proposed)});
        npc_wander_.emplace(source.actor,NpcWander{source,combat::ai::wander::State(std::move(patrol),now)});
        return Result::armed;
    }
