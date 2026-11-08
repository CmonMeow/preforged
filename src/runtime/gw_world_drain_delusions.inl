bool drain_delusions(skills::effects::Scope scope,skills::effects::Scope target,
        const skills::effects::DrainDelusionsPlan& plan,std::uint64_t now){
    const auto canonical=skills::effects::drain_delusions(plan.rank);
    if(!canonical||*canonical!=plan)return false;
    std::lock_guard lock(mutex_);
    auto perform=[&]()->bool{
        const auto source=players_.find(scope.actor);
        if(source==players_.end()||source->second.effect_scope!=scope||!source->second.resources||
           !advance_resource_event_locked(source->second,now)||source->second.precise_health<=0||source->second.knocked_down_until>now)return false;
        auto& caster=source->second;const auto origin=healing_position_locked(caster,now);if(!origin)return false;
        const auto curve=caster.overcast_curve?*caster.overcast_curve:overcast::CeilingCurve(scope,now,1);
        if(!overcast::prepare_gain_units(curve,scope,*caster.resources,caster.energy_regeneration,caster.energy_debt,
                std::uint64_t(plan.loss)*plan.multiplier*overcast::energy_unit,now,caster.energy_pips,caster.energy_fractional_tail))return false;
        auto reward=[&](std::uint64_t lost){
            const auto current=players_.find(scope.actor);
            if(current!=players_.end()&&current->second.effect_scope==scope&&current->second.precise_health>0){
                if(!advance_resource_event_locked(current->second,now)||current->second.precise_health<=0)return;
                const auto gain=gain_energy_units_locked(current->second,lost*plan.multiplier,now);
                if(gain.changed())current->second.outbox->enqueue(owner_vitals(current->second));
            }
        };
        const auto destination=players_.find(target.actor);
        if(destination!=players_.end()){
            auto& victim=destination->second;
            if(victim.effect_scope!=target||!victim.resources||!victim.effect_registry||!caster.team_token||!victim.team_token||
               caster.team_token==victim.team_token||caster.surface.plane!=victim.surface.plane)return false;
            const auto point=healing_position_locked(victim,now);
            if(!point||std::hypot(point->x-origin->x,point->z-origin->z)>plan.range)return false;
            if(navigation_){const auto trace=layered_->trace(caster.surface,*origin,*point);if(!trace||trace->plane!=victim.surface.plane)return false;}
            if(!preflight_debilitating_locked(victim,plan.loss,now))return false;
            const auto hex=victim.effect_registry->newest(target,skills::effects::Category::hex,now,5);
            if(caster.passive_recovery)caster.passive_recovery->record(recovery::Event::target_enemy_spell_or_signet,now);
            if(!hex)return true;
            // A removed hex may cause an end reaction. Never drain a dead
            // target or restore an earlier resource/actor proposal afterward.
            remove_health_effect_locked(victim,hex->instance,now);
            std::uint64_t lost=0;
            if(victim.precise_health<=0)kill_locked(victim,now);
            else lost=commit_debilitating_locked(victim,plan.loss,now);
            reward(lost);publish_target_body_locked(target.actor,1337,scope.actor);return true;
        }
        const auto placement=npcs_.find(target.actor);auto live=npc_combat_.find(target.actor);
        if(placement==npcs_.end()||live==npc_combat_.end())return false;
        const auto life=live->second.lifetime();
        if(live->second.hex_scope()!=target||!live->second.hostile_to_players(life)||caster.surface.plane!=placement->second.plane||
           std::hypot(placement->second.x-origin->x,placement->second.y-origin->z)>plan.range)return false;
        if(navigation_){const auto trace=layered_->trace(caster.surface,*origin,{placement->second.x,placement->second.y});if(!trace||trace->plane!=placement->second.plane)return false;}
        if(const auto clock=live->second.health_clock(life);clock&&now<*clock)return false;
        advance_npc_conditions_locked(target.actor,live->second,now);
        if(!settle_mind_wrack_caster_locked(target,now))return false;
        live=npc_combat_.find(target.actor);
        if(live==npc_combat_.end()||live->second.lifetime()!=life)return false;
        const auto* actor=live->second.actor(life);if(!actor||!actor->health||actor->health->current<=0||!actor->damageable)return false;
        const auto hex=live->second.mesmer_hex(life,now);
        if(caster.passive_recovery)caster.passive_recovery->record(recovery::Event::target_enemy_spell_or_signet,now);
        if(!hex)return true;
        // These owned NPC producers have reviewed no-reward removal endings.
        // Phantom Pain and other unreviewed endings must not silently vanish.
        if(hex->skill!=19&&hex->skill!=2998&&hex->skill!=26&&hex->skill!=49&&hex->skill!=2734&&hex->skill!=52)return false;
        auto preview=live->second;
        if(!preview.remove_mesmer_hex(life,hex->instance,now)||!preview.drain_energy_units(life,plan.loss,now))return false;
        if(!live->second.advance_energy(life,now))return false;
        const auto energy=live->second.energy(life);const auto precise=live->second.precise_energy_units(life);
        if(!energy||!precise)return false;
        const skills::effects::MindWrackEnergy before{energy->current,energy->maximum,0,unsigned(*precise-std::int64_t(energy->current)*overcast::energy_unit)};
        if(!live->second.remove_mesmer_hex(life,hex->instance,now))throw std::logic_error("preflighted Drain Delusions NPC removal failed");
        if(hex->skill==49||hex->skill==2734){const auto credit=mind_wrack_credit_.find(target.actor);
            if(credit!=mind_wrack_credit_.end()&&credit->second.target==target&&credit->second.effect==hex->instance)mind_wrack_credit_.erase(credit);}
        if(hex->skill==52){const auto credit=panic_credit_.find(target.actor);
            if(credit!=panic_credit_.end()&&credit->second.target==target&&credit->second.effect==hex->instance)panic_credit_.erase(credit);}
        const auto lost=live->second.drain_energy_units(life,plan.loss,now);
        if(!lost)throw std::logic_error("preflighted Drain Delusions NPC loss failed");
        const auto after_energy=*live->second.energy(life);const auto after_units=*live->second.precise_energy_units(life);
        const skills::effects::MindWrackEnergy after{after_energy.current,after_energy.maximum,0,unsigned(after_units-std::int64_t(after_energy.current)*overcast::energy_unit)};
        for(auto& [id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,live->second.actor(life)->effects)});
        observe_npc_mind_wrack_energy_locked(target,before,now,after);
        reward(*lost);
        const auto current=npc_combat_.find(target.actor);
        if(current!=npc_combat_.end()&&current->second.lifetime()==life&&npcs_.contains(target.actor))publish_target_body_locked(target.actor,1337,scope.actor);
        return true;
    };
    const bool accepted=perform();dispatch_fevered_boundary_locked(now);return accepted;
}
