// Staged227 pulse provider. Generic damage/condition reactions are shared with
// other skills; no normal binding is authorized by this provider alone.
private:
    std::uint64_t glimmering_refused_=0;
    void end_glimmering_mark_locked(skills::effects::Scope target,unsigned effect,std::uint64_t now){
        const auto owned=glimmering_mark_owners_.find(target);
        if(owned==glimmering_mark_owners_.end()||owned->second.effect!=effect||
           now>=owned->second.applied+owned->second.plan.duration)return;
        glimmering_marks_.end_target(target,effect);glimmering_mark_owners_.erase(owned);
    }
    using GlimmeringCentre=AreaDamageCentre;
    std::optional<GlimmeringCentre> glimmering_centre_locked(const skills::effects::GlimmeringMarkPulse& pulse){
        const auto owned=glimmering_mark_owners_.find(pulse.target);
        if(owned==glimmering_mark_owners_.end()||owned->second.effect!=pulse.effect||owned->second.caster!=pulse.caster||
           owned->second.applied!=pulse.applied||owned->second.plan!=pulse.plan)return {};
        std::optional<skills::effects::Entry> entry;GlimmeringCentre centre;
        if(const auto p=players_.find(pulse.target.actor);p!=players_.end()){
            if(p->second.effect_scope!=pulse.target||!p->second.resources||!p->second.effect_registry||
               !advance_health_locked(p->second,pulse.at)||p->second.precise_health<=0)return {};
            const auto point=healing_position_locked(p->second,pulse.at);if(!point)return {};
            centre={*point,p->second.surface.plane};entry=p->second.effect_registry->lookup(pulse.target,pulse.effect);
        }else{
            auto live=npc_combat_.find(pulse.target.actor);
            if(live==npc_combat_.end()||live->second.hex_scope()!=pulse.target)return {};
            advance_npc_conditions_locked(pulse.target.actor,live->second,pulse.at);refresh_npc_movement_locked(pulse.target.actor,pulse.at);
            live=npc_combat_.find(pulse.target.actor);const auto body=npcs_.find(pulse.target.actor);
            if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=pulse.target)return {};
            const auto life=live->second.lifetime();const auto actor=live->second.actor(life);
            if(!actor||!actor->health||actor->health->current<=0||live->second.glimmering_removed(life,pulse.effect))return {};
            centre={{body->second.x,body->second.y},body->second.plane};entry=live->second.hex_effect(life,pulse.effect);
        }
        // Natural status expiry can be consumed while settling Health at the
        // final deadline. Manual earlier removal cancels the owner/provider.
        const auto expires=pulse.applied+pulse.plan.duration;
        if(entry){if(entry->skill!=227||entry->cause!=pulse.caster.actor||entry->rank!=pulse.plan.rank||
            entry->applied!=pulse.applied||entry->expires!=expires)return {};}
        else if(pulse.at!=expires)return {};
        const auto current=glimmering_mark_owners_.find(pulse.target);
        if(current==glimmering_mark_owners_.end()||current->second.effect!=pulse.effect)return {};
        return centre;
    }
    std::optional<AreaDamageRecipients> admit_glimmering_pulse_locked(
            const skills::effects::GlimmeringMarkPulse& pulse,const GlimmeringCentre& centre,unsigned team){
        const combat::Damage damage{combat::Kind::armor_ignoring,double(pulse.plan.damage),pulse.level,227};
        return admit_area_damage_pulse_locked(pulse.caster,damage,centre,pulse.plan.adjacent,pulse.at,team,
            AreaDamageCondition{combat::conditions::Kind::blind,pulse.plan.blind_ms},
            [&](const Player& p){return skills::effects::glimmering_mark_blinds(player_attack_action_locked(p,pulse.at));},
            [&](const combat::npcs::LiveActor& live){
                combat::weapon::AttackActionFacts facts;
                if(const auto action=live.active_action(live.lifetime(),pulse.at)){
                    facts.kind=action->kind==combat::npcs::control::Action::attack?
                        combat::weapon::AttackTargetAction::attack:combat::weapon::AttackTargetAction::skill;facts.skill=action->skill;
                }
                return skills::effects::glimmering_mark_blinds(facts);
            });
    }
    void glimmering_pulse_body_locked(skills::effects::Scope target,skills::effects::Scope caster){
        Packet body;u16(body,159);u32(body,20);u32(body,target.actor);u32(body,caster.actor);u32(body,401);
        for(auto&[id,peer]:players_)peer.outbox->enqueue({body});
    }
    bool apply_glimmering_pulse_locked(const skills::effects::GlimmeringMarkPulse& pulse){
        const auto centre=glimmering_centre_locked(pulse);if(!centre)return false;
        const auto owner=glimmering_mark_owners_.at(pulse.target);
        const auto admitted=admit_glimmering_pulse_locked(pulse,*centre,owner.team);
        if(!admitted){++glimmering_refused_;std::cerr<<"Glimmering Mark pulse refused before mutation: cohort/protection/condition admission\n";return true;}
        if(!glimmering_centre_locked(pulse))return false;
        const combat::Damage damage{combat::Kind::armor_ignoring,double(pulse.plan.damage),pulse.level,227};
        commit_area_damage_pulse_locked(pulse.caster,damage,pulse.at,*admitted,
            AreaDamageCondition{combat::conditions::Kind::blind,pulse.plan.blind_ms},
            [&](skills::effects::Scope target){glimmering_pulse_body_locked(target,pulse.caster);});
        return glimmering_centre_locked(pulse).has_value();
    }
    void dispatch_glimmering_marks_locked(std::uint64_t now){
        const auto due=glimmering_marks_.next_deadline();
        if(!due||*due>now)return;
        if(!glimmering_marks_.advance(now,[&](const auto& pulse){
            const bool keep=apply_glimmering_pulse_locked(pulse);
            if(!keep||pulse.ordinal==pulse.plan.pulses){const auto found=glimmering_mark_owners_.find(pulse.target);
                if(found!=glimmering_mark_owners_.end()&&found->second.effect==pulse.effect)glimmering_mark_owners_.erase(found);}
            return keep;
        }))throw std::logic_error("Glimmering pulse dispatch exceeded work/reentry boundary");
    }
public:
    std::uint64_t glimmering_refused_pulses()const{std::lock_guard lock(mutex_);return glimmering_refused_;}
