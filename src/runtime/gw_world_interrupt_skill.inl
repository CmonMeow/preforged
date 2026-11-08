    bool interrupt_npc_locked(Player& source,skills::effects::Scope target,const skills::effects::InterruptPlan& plan,unsigned level,std::uint64_t now,bool completion_publication=false){
        settle_damage_fields_locked(now);
        auto placement=npcs_.find(target.actor);auto found=npc_combat_.find(target.actor);
        if(placement==npcs_.end()||found==npc_combat_.end()||found->second.hex_scope()!=target||
           !source.resources||source.precise_health<=0||source.knocked_down_until>now||
           now<source.health_updated||now<source.energy_updated)return false;
        auto& live=found->second;const auto life=live.lifetime();
        if(!live.hostile_to_players(life)||source.surface.plane!=placement->second.plane)return false;
        if(const auto clock=live.health_clock(life);clock&&now<*clock)return false;
        advance_npc_conditions_locked(target.actor,live,now);refresh_npc_movement_locked(target.actor,now);
        found=npc_combat_.find(target.actor);placement=npcs_.find(target.actor);
        if(found==npc_combat_.end()||placement==npcs_.end()||found->second.hex_scope()!=target)return false;
        const auto* body=found->second.actor(life);
        if(!body||!body->damageable||!body->health||body->health->current<=0)return false;
        const auto from=healing_position_locked(source,now);
        if(!from||std::hypot(from->x-placement->second.x,from->z-placement->second.y)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(source.surface,*from,{placement->second.x,placement->second.y});
            if(!line||line->plane!=placement->second.plane)return false;}
        if(!advance_health_locked(source,now)||source.precise_health<=0)return false;
        found=npc_combat_.find(target.actor);placement=npcs_.find(target.actor);
        if(found==npc_combat_.end()||placement==npcs_.end()||found->second.hex_scope()!=target)return false;
        const auto action=live.active_action(life,now);
        // A spell animation without authoritative skill identity cannot prove
        // attribute/type-dependent eligibility or Energy Drain's spell reward.
        if(action&&action->kind==combat::npcs::control::Action::spell&&!action->skill)return false;
        const auto active=action&&action->skill?unsigned(action->skill->skill):0;
        const bool attacking=action&&action->kind==combat::npcs::control::Action::attack;
        const bool hexed=live.newest_hex(life,now).has_value();
        if(!skills::effects::interrupt_matches(plan,active,attacking,hexed)){
            if(plan.skill==5)publish_target_body_locked(target.actor,plan.skill,source.id);
            return true;
        }
        const auto stance=live.newest_stance(life,now);
        const bool protected_action=stance&&(stance->skill==16||stance->skill==17);
        const bool energy_needed=plan.target_loss||plan.target_gain||(stance&&stance->skill==17);
        if(energy_needed){
            if(!settle_mind_wrack_caster_locked(target,now))return false;
            found=npc_combat_.find(target.actor);placement=npcs_.find(target.actor);
            if(found==npc_combat_.end()||placement==npcs_.end()||found->second.hex_scope()!=target)return false;
            if(!live.advance_energy(life,now)||!live.energy(life))return false;
        }
        body=live.actor(life);if(!body||!body->health||body->health->current<=0)return false;
        auto proposed=live;
        std::vector<std::pair<skills::effects::MindWrackEnergy,skills::effects::MindWrackEnergy>> energy_changes;
        const auto energy_state=[&](const combat::npcs::LiveActor& actor){
            const auto energy=*actor.energy(life);const auto units=actor.precise_energy_units(life);
            return skills::effects::MindWrackEnergy{energy.current,energy.maximum,0,units?unsigned(*units%300000):0};
        };
        std::optional<unsigned> removed_stance;
        if(protected_action){bool ends=stance->skill==16;
            if(stance->skill==17){const auto before=energy_state(proposed);const auto cost=skills::effects::interpolate(10,4,stance->rank);
                ends=before.current<cost;if(!proposed.drain_energy(life,cost))return false;
                energy_changes.push_back({before,energy_state(proposed)});
            }
            if(ends){if(!proposed.remove_interrupt_stance(life,stance->instance,now))return false;removed_stance=stance->instance;}
        }
        std::optional<skills::effects::PowerBlockDisables> power;
        if(plan.skill==5){const auto disable=skills::effects::power_block(plan.rank);if(!disable)return false;
            const auto existing=power_block_disables_.find(target);
            power=existing==power_block_disables_.end()?skills::effects::PowerBlockDisables(target):existing->second;
            if(!power->apply(target,source.effect_scope,*disable,active,now))return false;
        }
        std::optional<combat::Hit> hit;
        std::optional<combat::npcs::LiveActor::ReversalDamage> reversal;
        if(plan.damage&&!preflight_npc_reversal_or_damage_locked(proposed,
            {combat::Kind::armor_ignoring,double(plan.damage),level,plan.skill},now,nullptr,&hit,&reversal))return false;
        const bool alive=proposed.actor(life)->health->current>0;
        if(!protected_action&&alive&&!proposed.cancel_action(life,now))return false;
        if(alive&&(plan.target_loss||plan.target_gain)){
            const auto before=energy_state(proposed);
            if(plan.target_loss&&!proposed.drain_energy(life,plan.target_loss))return false;
            if(plan.target_gain&&!proposed.gain_energy(life,plan.target_gain))return false;
            energy_changes.push_back({before,energy_state(proposed)});
        }
        if(reversal&&!validate_npc_reversal_receipts_locked(target,{reversal}))return false;
        live=std::move(proposed);
        if(reversal)npc_simple_enchantment_credit_.erase({target.actor,reversal->removed.instance});
        if(power)power_block_disables_.insert_or_assign(target,std::move(*power));
        if(removed_stance)for(auto&[id,peer]:players_)peer.outbox->enqueue({health_effect_remove(target.actor,*removed_stance)});
        if(!protected_action&&alive){
            const auto* row=active?skills::find_skill(active):nullptr;
            Packet stopped;u16(stopped,158);u32(stopped,attacking||(row&&row->type==14)?49:59);u32(stopped,target.actor);u32(stopped,0);
            for(auto&[id,peer]:players_)peer.outbox->enqueue({stopped});
        }
        if(completion_publication&&plan.skill!=571)publish_target_body_locked(target.actor,plan.skill,source.id);
        if(reversal){
            const auto packets=combat::npcs::reversal_damage_packets_2022(target.actor,source.id,*reversal,reversal_healing_source_locked(target,reversal->removed));
            for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
        }else if(hit)for(auto&[id,peer]:players_)peer.outbox->enqueue(combat::damage_packets_2022(source.id,*hit));
        if(hit&&hit->applied>0)bonds_npc_caster_damage_locked(target,hit->applied,now,nullptr);
        found=npc_combat_.find(target.actor);placement=npcs_.find(target.actor);
        if(found!=npc_combat_.end()&&placement!=npcs_.end()&&found->second.hex_scope()==target&&
           ((hit&&hit->killed)||(reversal&&!hit&&reversal->after_healing.current<=0)))
            record_npc_death_locked(source,placement->second,found->second,now);
        found=npc_combat_.find(target.actor);
        if(reversal&&found!=npc_combat_.end()&&found->second.hex_scope()==target){
            const auto* current=found->second.actor(life);
            if(current)for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,current->effects)});
        }
        const auto* interrupted=active?skills::find_skill(active):nullptr;
        const auto gain=(!plan.energy_spell_only||(interrupted&&skills::effects::is_spell(*interrupted)))?plan.energy:0;
        if(gain){if(source.energy_updated)regenerate_energy_locked(source,now-source.energy_updated);
            source.energy_updated=now;gain_energy_locked(source,gain,now);source.outbox->enqueue(owner_vitals(source));}
        found=npc_combat_.find(target.actor);
        if(!completion_publication&&plan.skill!=571&&found!=npc_combat_.end()&&found->second.hex_scope()==target)
            publish_target_body_locked(target.actor,plan.skill,source.id);
        for(const auto& [before,after]:energy_changes)observe_npc_mind_wrack_energy_locked(target,before,now,after);
        return true;
    }
public:
    bool interrupt_skill(skills::effects::Scope scope,skills::effects::Scope target,const skills::effects::InterruptPlan& plan,unsigned level,std::uint64_t now,std::vector<Packet>* visuals=nullptr,std::function<void()>* publication=nullptr){
        if(visuals)visuals->clear();if(publication)*publication={};if(publication&&!visuals)return false;
        if(!skills::effects::valid_interrupt(plan)||!level||level>255)return false;
        std::lock_guard lock(mutex_);DamagePublication pending;if(publication)begin_damage_publication_locked(pending);
        const auto perform=[&]()->bool{
        const auto a=players_.find(scope.actor),b=players_.find(target.actor);
        if(a==players_.end()||a->second.effect_scope!=scope)return false;
        if(b==players_.end())return interrupt_npc_locked(a->second,target,plan,level,now,publication!=nullptr);
        if(b->second.effect_scope!=target)return false;
        auto& source=a->second;auto& victim=b->second;
        if(!source.resources||!victim.resources||source.precise_health<=0||victim.precise_health<=0||source.knocked_down_until>now||
           !source.team_token||!victim.team_token||source.team_token==victim.team_token||source.surface.plane!=victim.surface.plane||
           now<source.health_updated||now<victim.health_updated||now<source.energy_updated||now<victim.energy_updated)return false;
        const auto from=healing_position_locked(source,now),to=healing_position_locked(victim,now);
        if(!from||!to||std::hypot(from->x-to->x,from->z-to->z)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(source.surface,*from,*to);if(!line||line->plane!=victim.surface.plane)return false;}
        if(!advance_health_locked(source,now)||!advance_health_locked(victim,now)||source.precise_health<=0||victim.precise_health<=0)return false;
        const unsigned active=victim.casting_skill&&now>=victim.casting_begins&&now<victim.casting_ends?victim.casting_skill:0;
        const bool attack=victim.ordinary_animation||now<victim.melee_skill_until;
        const bool hexed=victim.effect_registry&&bool(victim.effect_registry->newest(target,skills::effects::Category::hex,now));
        if(!skills::effects::interrupt_matches(plan,active,attack,hexed)){
            // Power Block body11 is captured on the armor practice target
            // without an observed target cast. Feedback is separate from
            // interruption/disable eligibility;never grant a mechanical effect.
            if(plan.skill==5)publish_target_body_locked(target.actor,plan.skill,scope.actor);
            return true;
        }
        if(victim.control_epoch==UINT64_MAX||victim.attack_interrupt_epoch==UINT64_MAX||!preflight_interrupt_protection_locked(victim,now)||(plan.target_loss&&!settle_mind_wrack_caster_locked(target,now)))return false;
        const combat::Damage damage{combat::Kind::armor_ignoring,double(plan.damage),level,plan.skill};
        const auto actor_at=[&](Player& p,combat::Health health){return incoming_spell_actor_locked(p,
            {p.id,p.effect_scope.generation,health,std::nullopt,p.effects,true},now);};
        if(plan.damage&&!admit_player_damage_sequence_locked(victim,{damage},now,actor_at))return false;
        if(source.precise_health<=0||victim.precise_health<=0)return false;
        if(plan.skill==5){
            const auto disable=skills::effects::power_block(plan.rank);if(!disable)return false;
            const auto existing=power_block_disables_.find(target);
            auto proposed=existing==power_block_disables_.end()?skills::effects::PowerBlockDisables(target):existing->second;
            if(!proposed.apply(target,scope,*disable,active,now))return false;
            power_block_disables_.insert_or_assign(target,std::move(proposed));
        }
        const auto* interrupted=skills::find_skill(active);
        const unsigned gain=(!plan.energy_spell_only||(interrupted&&skills::effects::is_spell(*interrupted)))?plan.energy:0;
        std::vector<DeferredEnergyReaction> energy_reactions;
        if(!prevent_interrupt_locked(victim,now,&energy_reactions)){
            record_control(victim,now);++victim.attack_interrupt_epoch;victim.casting_skill=0;victim.melee_skill_until=0;
            cancel_ordinary_locked(victim,combat::weapon::Cancel::explicit_input);
        }
        if(publication)publish_target_body_locked(target.actor,plan.skill,scope.actor);
        if(plan.damage&&victim.precise_health>0){
            auto actor=actor_at(victim,{victim.precise_health,victim.resources->max_health});const auto hit=combat::apply(actor,damage,true);
            if(hit.reason!=combat::Reason::accepted||!commit_player_reactive_hit_locked(victim,hit,scope.actor,now,false,nullptr,false,true,nullptr,scope))
                throw std::logic_error("preflighted interrupt player damage failed");
            publish_health_locked(victim);}
        if(gain){if(source.energy_updated)regenerate_energy_locked(source,now-source.energy_updated);
            source.energy_updated=now;gain_energy_locked(source,gain, now);source.outbox->enqueue(owner_vitals(source));}
        if(victim.precise_health>0&&(plan.target_loss||plan.target_gain)){
            if(victim.energy_updated)regenerate_energy_locked(victim,now-victim.energy_updated);
            victim.energy_updated=now;
            const auto before=mind_wrack_energy_locked(victim);
            drain_energy_locked(victim,plan.target_loss,now);
            energy_reactions.push_back({target,before,mind_wrack_energy_locked(victim),now});
            gain_energy_locked(victim,plan.target_gain, now);
            victim.outbox->enqueue(owner_vitals(victim));
        }
        if(!publication)publish_target_body_locked(target.actor,plan.skill,scope.actor);commit_energy_reactions_locked(energy_reactions);return true;
        };
        const bool accepted=perform();dispatch_fevered_boundary_locked(now);
        auto release=finish_damage_publication_locked(pending);
        if(publication&&accepted)*publication=std::move(release);else if(release)release();return accepted;
    }
