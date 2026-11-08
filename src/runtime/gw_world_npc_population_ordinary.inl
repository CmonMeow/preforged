// Include after gw_world_npc_population_ai.inl. Reuses the ordinary driver,
// aggro memory, retained navigation and wander state; no extra controller.
public:
    NpcPopulationEnrollment configure_npc_population_ordinary(skills::effects::Scope source,
            const combat::npcs::Binding& expected,combat::ai::FlarePolicy policy,
            combat::ai::ordinary::Reviewed attack,combat::ai::wander::Policy patrol,std::uint64_t now){
        using Result=NpcPopulationEnrollment;
        if(policy.source!=source||policy.target.instance||policy.target.actor||policy.target.generation||
           !policy.valid(true)||!attack.valid()||attack.delivery!=combat::ai::ordinary::Delivery::melee||
           !patrol.valid(policy.leash))return Result::invalid_policy;
        std::lock_guard lock(mutex_);
        const auto body=npcs_.find(source.actor);const auto live=npc_combat_.find(source.actor);
        if(body==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=source||
           !body->second.combat_definition||body->second.combat_definition->binding!=expected||
           expected.actor!=source.actor||expected.level!=policy.level||body->second.definition.level!=policy.level)
            return Result::identity_mismatch;
        if(quest_npc_owners_.contains(source.actor)||quest_npc_motion_.contains(source.actor)||
           npc_elemental_ai_.contains(source.actor)||npc_immediate_ai_.contains(source.actor))return Result::conflicting_controller;
        const auto life=live->second.lifetime();const auto actor=live->second.actor(life);
        const auto& metadata=*body->second.combat_definition;
        if(!actor||!actor->health||actor->health->current<=0||!live->second.hostile_to_players(life)||
           !metadata.physical_armor||!metadata.earth_armor||!metadata.fire_armor||!metadata.cold_armor||
           !metadata.lightning_armor||!metadata.general_armor)return Result::unknown_combat;
        if(now<live->second.health_clock(life).value_or(now))return Result::deferred;
        const auto motion=npc_flare_ai_.find(source.actor);const auto ordinary=npc_ordinary_ai_.find(source.actor);
        const auto wander=npc_wander_.find(source.actor);
        if(motion!=npc_flare_ai_.end()||ordinary!=npc_ordinary_ai_.end()||wander!=npc_wander_.end()){
            if(motion==npc_flare_ai_.end()||ordinary==npc_ordinary_ai_.end()||wander==npc_wander_.end()||
               !npc_hostile_ordinary_.contains(source)||motion->second.policy.source!=source||motion->second.melee_quest||
               ordinary->second.source!=source||wander->second.source!=source)return Result::conflicting_controller;
            const auto& p=motion->second.policy;const auto& a=ordinary->second.ledger.profile();const auto& w=wander->second.state.policy();
            // Include every authored profile member, including delivery and
            // flight policy even though this adapter admits melee only.
            if(p.proximity_acquisition!=policy.proximity_acquisition||p.level!=policy.level||p.fire_magic!=policy.fire_magic||p.speed!=policy.speed||p.acquire!=policy.acquire||
               p.follow!=policy.follow||p.leash!=policy.leash||p.flight.speed!=policy.flight.speed||
               p.flight.hit_radius!=policy.flight.hit_radius||p.max_flights!=policy.max_flights||p.evidence!=policy.evidence||
               a.minimum!=attack.minimum||a.maximum!=attack.maximum||a.mastery!=attack.mastery||a.damage_type!=attack.damage_type||
               a.caster_weapon!=attack.caster_weapon||a.delivery!=attack.delivery||a.windup_ms!=attack.windup_ms||
               a.period_ms!=attack.period_ms||a.range!=attack.range||a.flight.speed!=attack.flight.speed||
               a.flight.hit_radius!=attack.flight.hit_radius||a.evidence!=attack.evidence||
               w.pause_ms!=patrol.pause_ms||w.leg_timeout_ms!=patrol.leg_timeout_ms||w.evidence!=patrol.evidence||
               w.offsets.size()!=patrol.offsets.size())return Result::conflicting_controller;
            for(std::size_t i=0;i<w.offsets.size();++i)
                if(w.offsets[i].x!=patrol.offsets[i].x||w.offsets[i].y!=patrol.offsets[i].y)return Result::conflicting_controller;
            return now<motion->second.updated?Result::deferred:Result::already_armed;
        }
        if(live->second.active_action(life,now)||!live->second.ordinary_action_ready(life,now))return Result::deferred;
        // One World lock and complete preflight: no intermediate enabled
        // controller without its patrol. No Energy or spellbook is created.
        npc_flare_ai_.emplace(source.actor,combat::ai::FlareController{policy,body->second.x,body->second.y,body->second.plane,now});
        npc_ordinary_ai_.emplace(source.actor,NpcOrdinaryController{source,combat::ai::ordinary::Ledger(source,std::move(attack)),{}});
        npc_hostile_ordinary_.insert(source);
        npc_wander_.emplace(source.actor,NpcWander{source,combat::ai::wander::State(std::move(patrol),now)});
        return Result::armed;
    }
