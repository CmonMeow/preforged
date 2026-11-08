// Included in Instance public scope. The optional admitted event selects the
// paid Runtime completion path; legacy trusted callers retain their signature.
    bool energy_enchantment(skills::effects::Scope scope,skills::effects::Scope target,
            const skills::effects::EnergyEnchantmentPlan& plan,std::uint64_t now,
            std::optional<skills::sacrifice::Loss>* paid,std::vector<Packet>* visuals=nullptr,
            const skills::effects::EnchantmentDuration* adjusted=nullptr,
            std::optional<std::uint64_t> completion_event={},std::function<void()>* publication=nullptr){
        if(paid)paid->reset();if(visuals)visuals->clear();if(publication)*publication={};
        const auto canonical=skills::effects::energy_enchantment(plan.skill,plan.rank);
        const auto duration=adjusted?adjusted->for_enchantment(plan.skill,plan.duration,scope,now):std::optional(plan.duration);
        if(!paid||!canonical||*canonical!=plan||scope==target||!duration||now>UINT64_MAX-*duration)return false;
        std::lock_guard lock(mutex_);auto source=players_.find(scope.actor);
        if(source==players_.end()||source->second.effect_scope!=scope)return false;
        auto& caster=source->second;
        if(!caster.resources||!caster.team_token||caster.knocked_down_until>now||
           now<caster.health_updated||now<caster.energy_updated||
           !advance_resource_event_locked(caster,now)||caster.precise_health<=0)return false;
        if(completion_event&&!complete_ether_renewal_locked(scope,plan.skill,*completion_event,now,true,false))return false;
        enum class Recipient {ready,retired,invalid};
        std::optional<skills::effects::Registry> registry;
        std::optional<skills::regeneration::State> timers;
        std::optional<overcast::CeilingCurve> curve;
        std::optional<combat::npcs::LiveActor> npc_proposal;
        std::optional<skills::sacrifice::Loss> loss;
        skills::effects::Applied applied;skills::effects::Entry entry{};
        unsigned effect=0;bool suppressed=false;
        const auto prepare=[&]()->Recipient{
            registry.reset();timers.reset();curve.reset();npc_proposal.reset();loss.reset();suppressed=false;
            const auto origin=healing_position_locked(caster,now);if(!origin||caster.precise_health<=0)return Recipient::invalid;
            const auto player=players_.find(target.actor);
            if(player!=players_.end()){
                auto& recipient=player->second;
                if(recipient.effect_scope!=target||!recipient.resources||recipient.precise_health<=0)return Recipient::retired;
                if(recipient.team_token!=caster.team_token||recipient.surface.plane!=caster.surface.plane||
                   !recipient.effect_registry||!recipient.health_regeneration||recipient.next_health_effect>UINT32_MAX||
                   now<recipient.health_updated||now<recipient.energy_updated)return Recipient::invalid;
                const auto point=healing_position_locked(recipient,now);
                if(!point||std::hypot(point->x-origin->x,point->z-origin->z)>plan.range)return Recipient::invalid;
                if(navigation_){const auto line=layered_->trace(caster.surface,*origin,*point);if(!line||line->plane!=recipient.surface.plane)return Recipient::invalid;}
                if(!advance_resource_event_locked(recipient,now))return Recipient::invalid;
                if(recipient.precise_health<=0)return Recipient::retired;
                suppressed=well_blocks_player_enchantment_locked(recipient,now);
                if(!suppressed){
                    effect=static_cast<unsigned>(recipient.next_health_effect);
                    entry={effect,scope.actor,plan.skill,plan.rank,skills::effects::Category::enchantment,now,now+*duration};
                    registry=*recipient.effect_registry;timers=*recipient.health_regeneration;
                    applied=registry->apply(target,entry,skills::effects::Replace::same_skill);if(!applied.accepted)return Recipient::invalid;
                    for(const auto& old:applied.removed)timers->remove(target,old.instance);
                    if(!timers->apply(target,{effect,scope.actor,plan.skill,0,now,*entry.expires}))return Recipient::invalid;
                    curve=recipient.overcast_curve.value_or(overcast::CeilingCurve(target,now,33));
                    if(!curve->apply_energy_modifier(target,effect,plan.skill,plan.pips,*duration,now))return Recipient::invalid;
                }
            }else{
                auto live=npc_combat_.find(target.actor);auto placement=npcs_.find(target.actor);
                if(live==npc_combat_.end()||placement==npcs_.end()||live->second.hex_scope()!=target)return Recipient::retired;
                const auto life=live->second.lifetime();
                if(!live->second.healable_ally(life)||placement->second.plane!=caster.surface.plane||next_npc_hex_==UINT32_MAX||
                   now<live->second.health_clock(life).value_or(now))return Recipient::invalid;
                refresh_npc_movement_locked(target.actor,now);
                live=npc_combat_.find(target.actor);placement=npcs_.find(target.actor);
                if(live==npc_combat_.end()||placement==npcs_.end()||!live->second.matches(life))return Recipient::retired;
                if(placement->second.plane!=caster.surface.plane||std::hypot(placement->second.x-origin->x,placement->second.y-origin->z)>plan.range)return Recipient::invalid;
                if(navigation_){const auto line=layered_->trace(caster.surface,*origin,{placement->second.x,placement->second.y});if(!line||line->plane!=placement->second.plane)return Recipient::invalid;}
                advance_npc_conditions_locked(target.actor,live->second,now);
                live=npc_combat_.find(target.actor);placement=npcs_.find(target.actor);
                if(live==npc_combat_.end()||placement==npcs_.end()||!live->second.matches(life))return Recipient::retired;
                const auto* actor=live->second.actor(life);if(!actor||!actor->health||actor->health->current<=0)return Recipient::retired;
                if(!live->second.healable_ally(life)||placement->second.plane!=caster.surface.plane)return Recipient::invalid;
                suppressed=well_blocks_npc_enchantment_locked(target.actor,now);
                if(!suppressed){
                    effect=next_npc_hex_;npc_proposal=live->second;
                    if(!npc_proposal->apply_energy_enchantment(life,scope,plan,effect,now,adjusted))return Recipient::invalid;
                }
            }
            loss=skills::sacrifice::resolve(plan.skill,caster.precise_health,caster.resources->max_health);
            return loss?Recipient::ready:Recipient::invalid;
        };
        if(prepare()!=Recipient::ready)return false;
        bool retired=false;
        if(completion_event){
            if(!complete_ether_renewal_locked(scope,plan.skill,*completion_event,now,true))return false;
            if(caster.precise_health<=0){*paid=skills::sacrifice::Loss{0,0,0,true};return true;}
            const auto refreshed=prepare();
            if(refreshed==Recipient::invalid)throw std::logic_error("validated Energy enchantment changed during Renewal settlement");
            retired=refreshed==Recipient::retired;
            if(retired){loss=skills::sacrifice::resolve(plan.skill,caster.precise_health,caster.resources->max_health);
                if(!loss)throw std::logic_error("Energy enchantment sacrifice changed during Renewal settlement");}
        }
        using Queues=std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>>;
        std::shared_ptr<Queues> queues;std::vector<std::unique_ptr<Outbox::Capture>> captures;
        if(publication){queues=std::make_shared<Queues>();std::set<const Outbox*> seen;
            for(auto&[id,peer]:players_)if(seen.insert(peer.outbox.get()).second)queues->emplace_back(peer.outbox,Outbox::DeferredQueue{});
            for(auto&[box,queue]:*queues)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));}
        // The recipient effect survives a lethal caster sacrifice. Install it
        // first, then let the shared Health/death reactions run against it.
        if(!retired&&!suppressed){
            const auto player=players_.find(target.actor);
            if(player!=players_.end()){
                auto& recipient=player->second;*recipient.effect_registry=std::move(*registry);
                recipient.health_regeneration=std::move(*timers);recipient.overcast_curve=std::move(*curve);++recipient.next_health_effect;
                std::vector<Packet> packets;
                for(const auto& old:applied.removed){recipient.health_effects.erase(old.instance);packets.push_back(health_effect_remove(target.actor,old.instance));}
                recipient.health_effects[effect]=entry;packets.push_back(health_effect_add(entry,target.actor,now));
                for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
                publish_status_locked(recipient,now);recipient.outbox->enqueue(owner_vitals(recipient));
            }else{
                auto& live=npc_combat_.at(target.actor);live=std::move(*npc_proposal);++next_npc_hex_;
                for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,live.actor(live.lifetime())->effects)});
            }
        }
        health_event_locked(caster,loss->remaining-caster.precise_health,now);
        loss->remaining=caster.precise_health;loss->killed=caster.precise_health<=0;*paid=loss;
        if(caster.passive_recovery&&loss->lost>0)caster.passive_recovery->record(recovery::Event::health_loss,now);
        if(loss->killed)kill_locked(caster,now);else{publish_health_locked(caster);caster.outbox->enqueue(owner_vitals(caster));}
        if(!retired){
            commit_applications_locked(target,now);
            const auto player=players_.find(target.actor);const auto live=npc_combat_.find(target.actor);
            if((player!=players_.end()&&player->second.effect_scope==target)||(live!=npc_combat_.end()&&live->second.hex_scope()==target))
                publish_target_body_locked(target.actor,plan.skill,scope.actor,visuals);
        }
        dispatch_fevered_boundary_locked(now);captures.clear();
        if(publication)defer_or_publish_completion_locked(publication,[queues]{for(auto&[box,queue]:*queues)box->enqueue(std::move(queue.packets));queues->clear();});
        return true;
    }
