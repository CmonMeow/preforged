// Earth-specific armor and condition sequencing over shared damage admission.
private:
    skills::effects::ShockwaveResult preview_player_earth_sequence_locked(
            const Player& p,const skills::effects::AoEPlan& plan,double distance,
            unsigned level,unsigned caster,std::uint64_t now,DamageAdmissionBatch& batch)const{
        using namespace skills::effects;
        auto* projected=admission_player_locked(batch,p,now);if(!projected)return {};
        auto& state=*projected;
        if(state.health.current<=0){ShockwaveResult result;result.accepted=true;return result;}
        auto conditions=p.conditions;
        std::optional<combat::Hit> previous_earth;
        const auto project=[&](combat::Actor& actor,const combat::Damage& damage,bool earth)->combat::Hit{
            const auto step=project_batch_player_damage_locked(batch,p,actor,damage,now);if(!step)return {};
            if(earth)previous_earth=step->projected;
            return step->incoming;
        };
        auto watch=watch_yourself_earth_sequence_locked(p,now);
        const combat::ArmorSequence armor=watch?combat::ArmorSequence{
            [&](const combat::Hit* previous){return watch(previous&&previous_earth?&*previous_earth:nullptr);}}:combat::ArmorSequence{};
        std::uint64_t next_effect=p.next_health_effect;
        const EarthConditionReaction reaction=[&](ShockwaveTarget& target,combat::conditions::Kind kind){
            const auto old=conditions.lookup(kind),current=target.conditions.lookup(kind);
            if(!current)return false;
            if(!old||old->expires!=current->expires||!p.condition_effects.contains(kind)){
                if(next_effect>UINT32_MAX)return false;
                ++next_effect;
            }
            const auto before=conditions;conditions=target.conditions;
            if(!project_batch_condition_reactions_locked(batch,p,before,conditions,now))return false;
            target.actor.health=combat::Health{state.health.current,state.health.capacity.maximum()};
            target.actor.effects=state.registry.presentation(p.effect_scope,target.actor.effects&~0x10u,now)|
                (state.health.current<=0?0x10u:0u);
            return true;
        };
        const ShockwaveTarget input{incoming_spell_actor_locked(p,{p.id,p.effect_scope.generation,
            combat::Health{state.health.current,state.health.capacity.maximum()},p.reviewed_earth_armor,p.effects,true},now),p.conditions};
        const EarthDamageResolver damage=[&](combat::Actor& actor,const combat::Damage& hit){return project(actor,hit,true);};
        return plan.skill==174?aftershock_target(plan,input,distance,level,caster,now,true,
            p.knocked_down_until>now,armor,damage):shockwave_target(plan,input,distance,level,caster,now,true,armor,damage,reaction);
    }
