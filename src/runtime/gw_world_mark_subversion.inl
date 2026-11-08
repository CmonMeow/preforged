// Included inside Instance. Authoritative Mark application and spell failure.
public:
    skills::effects::TargetedSpellFailure mark_subversion_spell_failure(skills::effects::Scope afflicted,unsigned spell,
            std::optional<skills::effects::Scope> selected,std::uint64_t event,std::uint64_t now){
        using Result=skills::effects::TargetedSpellFailure;
        std::lock_guard lock(mutex_);
        const auto* row=skills::find_skill(spell);const auto reserved=spell_activation_cursors_.find(afflicted.actor);
        if(!row||!skills::effects::is_spell(*row)||!event||reserved==spell_activation_cursors_.end()||
           reserved->second.actor!=afflicted||reserved->second.event!=event||reserved->second.skill!=spell||now>reserved->second.time)return Result::rejected;
        // before_spell_effect may advance to the authoritative health clock.
        // Reuse that reservation rather than rejecting or rewinding its event.
        now=reserved->second.time;
        const auto previous=mark_failure_cursors_.find(afflicted.actor);
        if(previous!=mark_failure_cursors_.end()&&previous->second.actor==afflicted&&event<=previous->second.event)return Result::rejected;
        mark_failure_cursors_[afflicted.actor]={afflicted,event,now,spell};
        const auto found=mark_subversion_credit_.find(afflicted.actor);
        if(found==mark_subversion_credit_.end()||found->second.afflicted!=afflicted||!selected)return Result::none;
        const auto owner=found->second;
        const auto relation=subversion_spell_target_locked(afflicted,*selected);
        if(!relation)return Result::rejected;
        if(relation->relation!=skills::effects::SpellTargetRelation::ally)return Result::none;
        auto caster=players_.find(owner.caster.actor),player=players_.find(afflicted.actor);auto npc=npc_combat_.find(afflicted.actor);
        // Private policy skips an absent/dead original source; never redirects
        // transfer to a replacement ID. Retail source-death policy is pending.
        if(caster==players_.end()||caster->second.effect_scope!=owner.caster||!caster->second.resources||
           now<caster->second.health_updated||!advance_health_locked(caster->second,now)||caster->second.precise_health<=0)return Result::none;
        const auto converted=[&](unsigned amount,double reduced)->Result{
            // The triggering hex has already been consumed. Do not restore its
            // proposal after conversion, or the spent Reversal would reappear.
            if(reduced>0&&!commit_reversal_life_steal_transfer_locked(owner.caster,afflicted,amount,now))
                std::clog<<"Mark of Subversion late transfer refusal actor="<<afflicted.actor<<" event="<<event<<'\n';
            mark_subversion_credit_.erase(afflicted.actor);
            const auto current_caster=players_.find(owner.caster.actor);
            if(current_caster!=players_.end()&&current_caster->second.effect_scope==owner.caster&&current_caster->second.precise_health<=0)
                kill_locked(current_caster->second,now);
            if(const auto current=players_.find(afflicted.actor);current!=players_.end()){
                if(current->second.effect_scope==afflicted)publish_status_locked(current->second,now);
            }else if(const auto current=npc_combat_.find(afflicted.actor);current!=npc_combat_.end()&&current->second.hex_scope()==afflicted){
                for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(afflicted.actor,current->second.actor(current->second.lifetime())->effects)});
            }
            // Zero payout, full conversion and healing-caused death do not
            // rescue the selected allied spell from Mark's failure.
            return Result::failed;
        };
        std::optional<skills::effects::MarkSubversionFailure> transfer;
        if(player!=players_.end()){
            auto& p=player->second;if(p.effect_scope!=afflicted||!p.effect_registry||!p.resources)return Result::rejected;
            if(!skills::effects::mark_subversion_reaction(*p.effect_registry,afflicted,spell,*relation,now))return Result::none;
            if(!advance_health_locked(p,now)||p.precise_health<=0)return Result::rejected;
            transfer=skills::effects::prepare_mark_subversion_failure(*p.effect_registry,owner,caster->second.effect_scope,afflicted,*selected,
                spell,*relation,event,now,{caster->second.precise_health,caster->second.resources->max_health},
                {p.precise_health,p.resources->max_health},shielding_reduction_locked(p,now));
            if(!transfer)return Result::rejected;
            const bool reversal=p.effect_registry->newest_skill(afflicted,307,now).has_value();
            const auto plan=skills::effects::mark_subversion(transfer->consumed.rank);
            const double reduced=plan?std::max(0.,double(plan->amount)-shielding_reduction_locked(p,now)):0;
            if(reversal&&(!plan||now<caster->second.energy_updated||
               (reduced>0&&!preview_player_reversal_life_steal_locked(p,reduced,now))))return Result::rejected;
            *p.effect_registry=transfer->registry;p.health_effects.erase(owner.effect);
            for(auto&[id,peer]:players_)peer.outbox->enqueue({health_effect_remove(p.id,owner.effect)});
            if(reversal)return converted(plan->amount,reduced);
            health_event_locked(p,transfer->health.target.current-p.precise_health,now);
            if(p.passive_recovery&&transfer->health.stolen)p.passive_recovery->record(recovery::Event::health_loss,now);
            publish_status_locked(p,now);
        }else{
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=afflicted)return Result::rejected;
            const auto registry=npc->second.hex_registry_snapshot(npc->second.lifetime(),now);
            if(!registry||!skills::effects::mark_subversion_reaction(*registry,afflicted,spell,*relation,now))return Result::none;
            advance_npc_conditions_locked(afflicted.actor,npc->second,now);
            npc=npc_combat_.find(afflicted.actor);
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=afflicted)return Result::rejected;
            const auto current_registry=npc->second.owned_effect_snapshot(npc->second.lifetime());
            if(current_registry&&current_registry->newest_skill(afflicted,307,now)){
                const auto* actor=npc->second.actor(npc->second.lifetime());
                const auto reduction=npc->second.life_steal_reduction(npc->second.lifetime(),now);
                if(!actor||!actor->health||!reduction||now<caster->second.energy_updated)return Result::rejected;
                const auto qualified=skills::effects::prepare_mark_subversion_failure(*current_registry,owner,caster->second.effect_scope,afflicted,*selected,
                    spell,*relation,event,now,{caster->second.precise_health,caster->second.resources->max_health},*actor->health,*reduction);
                if(!qualified)return Result::rejected;
                const auto plan=skills::effects::mark_subversion(qualified->consumed.rank);
                auto preview=npc->second;
                if(!plan||!preview_npc_reversal_life_steal_locked(preview,afflicted,plan->amount,now))return Result::rejected;
                if(!npc->second.remove_hex_effect(npc->second.lifetime(),owner.effect,now))return Result::rejected;
                return converted(plan->amount,std::max(0.,double(plan->amount)-*reduction));
            }
            transfer=npc->second.apply_mark_subversion_failure(npc->second.lifetime(),owner,caster->second.effect_scope,*selected,spell,*relation,
                event,now,{caster->second.precise_health,caster->second.resources->max_health});
            if(!transfer)return Result::rejected;
        }
        health_event_locked(caster->second,transfer->health.source.current-caster->second.precise_health,now);
        transfer->health.source.current=caster->second.precise_health;
        if(player!=players_.end()){transfer->health.target.current=player->second.precise_health;transfer->health.killed=player->second.precise_health<=0;}
        auto packets=skills::effects::life_steal_packets(owner.caster.actor,afflicted.actor,transfer->health);
        if(npc!=npc_combat_.end())packets.push_back(combat::detail::effects(afflicted.actor,npc->second.actor(npc->second.lifetime())->effects));
        for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
        if(transfer->health.killed){
            if(player!=players_.end())kill_locked(player->second,now);
            else{const auto body=npcs_.find(afflicted.actor);if(body!=npcs_.end())record_npc_death_locked(caster->second,body->second,npc->second,now);}
        }
        if(caster->second.precise_health<=0)kill_locked(caster->second,now);
        return Result::failed;
    }
    std::optional<skills::effects::SubversionSpellTarget> subversion_spell_target(
            skills::effects::Scope user,skills::effects::Scope target)const{
        std::lock_guard lock(mutex_);return subversion_spell_target_locked(user,target);
    }
private:
    std::optional<skills::effects::SubversionSpellTarget> subversion_spell_target_locked(
            skills::effects::Scope user,skills::effects::Scope target)const{
        const auto player=players_.find(user.actor),other=players_.find(target.actor);
        const auto npc=npc_combat_.find(user.actor),recipient=npc_combat_.find(target.actor);
        const auto living=[&](skills::effects::Scope scope){
            if(const auto p=players_.find(scope.actor);p!=players_.end())return p->second.effect_scope==scope&&p->second.resources&&p->second.precise_health>0;
            const auto n=npc_combat_.find(scope.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=scope)return false;
            const auto* actor=n->second.actor(n->second.lifetime());return actor&&actor->health&&actor->health->current>0;
        };
        if(user.instance!=target.instance||!living(user)||!living(target))return {};
        std::optional<bool> allied;
        if(user==target)allied=true;
        else if(player!=players_.end()&&other!=players_.end()){
            if(player->second.team_token&&other->second.team_token)allied=player->second.team_token==other->second.team_token;
        }else if(npc!=npc_combat_.end()&&recipient!=npc_combat_.end()){
            const auto a=npcs_.find(user.actor),b=npcs_.find(target.actor);
            if(a!=npcs_.end()&&b!=npcs_.end())allied=skills::effects::spiteful_npc_alliance(npc->second.packed_allegiance(npc->second.lifetime()),a->second.definition.flags,
                recipient->second.packed_allegiance(recipient->second.lifetime()),b->second.definition.flags);
        }else{
            const auto& live=npc!=npc_combat_.end()?npc->second:recipient->second;
            const auto& current_player=player!=players_.end()?player->second:other->second;
            allied=npc_player_alliance_locked(live.hex_scope(),current_player.effect_scope);
        }
        using R=skills::effects::SpellTargetRelation;
        return skills::effects::SubversionSpellTarget{user,target,allied?(*allied?R::ally:R::foe):R::unknown};
    }
public:
    bool stage_mark_subversion(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::MarkSubversionPlan& plan,std::uint64_t now,std::vector<Packet>* visuals=nullptr){
        if(visuals)visuals->clear();const auto canonical=skills::effects::mark_subversion(plan.rank);
        if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);if(!settle_timed_effects_locked(now))return false;
        auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||!caster->second.resources||!caster->second.effect_registry||
           !advance_resource_event_locked(caster->second,now)||caster->second.precise_health<=0||caster->second.knocked_down_until>now)return false;
        const auto duration=skills::effects::hex_duration(127,plan.duration,source,caster->second.effect_registry->active(source,now),now);
        const auto origin=healing_position_locked(caster->second,now);if(!duration||!origin)return false;
        auto player=players_.find(target.actor);auto npc=npc_combat_.find(target.actor);
        unsigned effect=0;
        if(player!=players_.end()){
            auto& p=player->second;
            if(p.effect_scope!=target||!p.resources||!p.effect_registry||!p.team_token||!caster->second.team_token||
               p.team_token==caster->second.team_token||p.surface.plane!=caster->second.surface.plane||
               p.next_health_effect>UINT32_MAX||!advance_resource_event_locked(p,now)||p.precise_health<=0)return false;
            const auto point=healing_position_locked(p,now);if(!point||std::hypot(point->x-origin->x,point->z-origin->z)>plan.range)return false;
            if(navigation_){const auto line=layered_->trace(caster->second.surface,*origin,*point);if(!line||line->plane!=p.surface.plane)return false;}
            const auto blocked=prevent_selected_hex_locked(caster->second,p,127,now,false);if(!blocked)return false;
            if(*blocked){const auto done=prevent_selected_hex_locked(caster->second,p,127,now);return done&&*done;}
            effect=static_cast<unsigned>(p.next_health_effect);
            const auto application=skills::effects::prepare_mark_subversion_application(*p.effect_registry,source,target,effect,plan,now,duration);
            if(!application)return false;
            *p.effect_registry=application->registry;++p.next_health_effect;
            std::vector<Packet> packets;
            for(const auto& old:application->change.removed){p.health_effects.erase(old.instance);packets.push_back(health_effect_remove(p.id,old.instance));}
            p.health_effects.emplace(effect,application->entry);packets.push_back(health_effect_add(application->entry,p.id,now));
            mark_subversion_credit_[target.actor]={source,target,effect,now};
            for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
            commit_applications_locked(target,now);publish_status_locked(p,now);
        }else{
            const auto body=npcs_.find(target.actor);
            if(npc==npc_combat_.end()||body==npcs_.end()||npc->second.hex_scope()!=target)return false;
            advance_npc_conditions_locked(target.actor,npc->second,now);refresh_npc_movement_locked(target.actor,now);
            npc=npc_combat_.find(target.actor);
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=target||body->second.plane!=caster->second.surface.plane||
               std::hypot(body->second.x-origin->x,body->second.y-origin->z)>plan.range||next_npc_hex_==UINT32_MAX)return false;
            if(navigation_){const auto line=layered_->trace(caster->second.surface,*origin,{body->second.x,body->second.y});if(!line||line->plane!=body->second.plane)return false;}
            effect=next_npc_hex_;
            if(!npc->second.apply_mark_subversion(npc->second.lifetime(),source,plan,effect,now,duration))return false;
            ++next_npc_hex_;mark_subversion_credit_[target.actor]={source,target,effect,now};
            for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,npc->second.actor(npc->second.lifetime())->effects)});
            commit_applications_locked(target,now);
        }
        publish_target_body_locked(target.actor,127,source.actor,visuals);
        return true;
    }


