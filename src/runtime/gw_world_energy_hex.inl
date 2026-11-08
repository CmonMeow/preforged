// Included inside Instance. Trusted integration stage; no native casting binding.
private:
    std::string energy_hex_zero_deferred_;
    std::map<std::pair<unsigned,unsigned>,skills::effects::EnergyHexOwnership> energy_hex_links_;
    std::map<skills::effects::Scope,skills::effects::MindWrackEvents> energy_hex_zero_events_;
    std::map<skills::effects::Scope,skills::effects::MindWrackTransition> energy_hex_pending_zeros_;
    struct RetiredEnergyHexCaster {skills::regeneration::State timers;std::uint64_t at;};
    std::map<skills::effects::Scope,RetiredEnergyHexCaster> retired_energy_hex_casters_;
    std::optional<skills::regeneration::State> energy_hex_caster_state_locked(skills::effects::Scope scope)const{
        const auto live=players_.find(scope.actor);
        if(live!=players_.end()&&live->second.effect_scope==scope&&live->second.health_regeneration)return *live->second.health_regeneration;
        const auto retired=retired_energy_hex_casters_.find(scope);
        return retired==retired_energy_hex_casters_.end()?std::nullopt:std::optional(retired->second.timers);
    }
    std::optional<std::uint64_t> energy_hex_caster_retirement_locked(skills::effects::Scope scope)const{
        const auto live=players_.find(scope.actor);
        if(live!=players_.end()&&live->second.effect_scope==scope){
            if(live->second.precise_health<=0)return live->second.died_at;return {};
        }
        const auto retired=retired_energy_hex_casters_.find(scope);
        return retired==retired_energy_hex_casters_.end()?std::nullopt:std::optional(retired->second.at);
    }
    void commit_energy_hex_caster_states_locked(std::map<skills::effects::Scope,skills::regeneration::State>& states){
        for(auto& [scope,state]:states){const auto live=players_.find(scope.actor);
            if(live!=players_.end()&&live->second.effect_scope==scope&&live->second.health_regeneration)*live->second.health_regeneration=std::move(state);
            else if(const auto retired=retired_energy_hex_casters_.find(scope);retired!=retired_energy_hex_casters_.end())retired->second.timers=std::move(state);
            else throw std::logic_error("validated Energy-hex caster lifetime lost");
        }
    }
    void retire_energy_hex_caster_locked(skills::effects::Scope scope,std::uint64_t at){
        if(std::none_of(energy_hex_links_.begin(),energy_hex_links_.end(),[&](const auto& entry){return entry.second.caster==scope;}))return;
        // The departed lifetime has no remaining resource authority. Retain
        // exact retirement evidence, not a new Health pool or a reused actor ID.
        retired_energy_hex_casters_.insert_or_assign(scope,RetiredEnergyHexCaster{skills::regeneration::State(scope,at),at});
    }
    bool reset_energy_hex_target_locked(Player& target){
        std::vector<Packet> packets;
        for(auto it=energy_hex_links_.begin();it!=energy_hex_links_.end();){
            if(it->second.target!=target.effect_scope){++it;continue;}
            const auto owner=it->second;it=energy_hex_links_.erase(it);
            if(target.effect_registry)target.effect_registry->remove(owner.target,owner.effect);
            if(target.health_regeneration)target.health_regeneration->remove(owner.target,owner.effect);
            target.health_effects.erase(owner.effect);packets.push_back(health_effect_remove(target.id,owner.effect));
            if(!owner.caster_health_effect)continue;const auto source=players_.find(owner.caster.actor);
            if(source!=players_.end()&&source->second.effect_scope==owner.caster&&source->second.health_regeneration){
                auto& timers=*source->second.health_regeneration;
                // set_vitals is a trusted replacement with no event timestamp.
                // Keep already settled history; stop any remaining linked time.
                timers.end(owner.caster,owner.caster_health_effect,std::max(target.health_updated,timers.cursor()));
            }
        }
        energy_hex_pending_zeros_.erase(target.effect_scope);
        // Same lifetime: retain the consumed-event ledger across reinitialization.
        if(!packets.empty())for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
        return !packets.empty();
    }
    void retire_energy_hex_target_locked(skills::effects::Scope target,std::uint64_t at){
        // The target lifetime is leaving this instance. Its destruction is an
        // end, not a zero-Energy event; retain only the caster's prior history.
        for(auto it=energy_hex_links_.begin();it!=energy_hex_links_.end();){
            if(it->second.target!=target){++it;continue;}
            const auto owner=it->second;it=energy_hex_links_.erase(it);
            if(!owner.caster_health_effect)continue;const auto source=players_.find(owner.caster.actor);
            if(source!=players_.end()&&source->second.effect_scope==owner.caster&&source->second.health_regeneration)
                source->second.health_regeneration->end(owner.caster,owner.caster_health_effect,at);
        }
        energy_hex_pending_zeros_.erase(target);energy_hex_zero_events_.erase(target);
    }
    void expire_npc_energy_hex_links_locked(combat::npcs::LiveActor& live,std::uint64_t now){
        const auto target=live.hex_scope();const auto life=live.lifetime();
        const auto clock=live.health_clock(life);if(!clock)return;
        for(auto it=energy_hex_links_.begin();it!=energy_hex_links_.end();){
            const auto owner=it->second;
            if(owner.target!=target||now<owner.expires||*clock<owner.expires||live.hex_effect(life,owner.effect)){++it;continue;}
            // The authoritative NPC registry already expired this instance.
            // Preserve the caster's history through its original expiry only.
            if(owner.caster_health_effect){const auto source=players_.find(owner.caster.actor);
                if(source!=players_.end()&&source->second.effect_scope==owner.caster&&source->second.health_regeneration)
                    source->second.health_regeneration->end(owner.caster,owner.caster_health_effect,owner.expires);
            }
            it=energy_hex_links_.erase(it);
        }
    }
    std::set<skills::effects::Scope> energy_hex_participants_locked()const{
        std::set<skills::effects::Scope> result;
        for(const auto&[key,link]:energy_hex_links_){result.insert(link.target);result.insert(link.caster);
            if(const auto mind=mind_wrack_credit_.find(link.target.actor);mind!=mind_wrack_credit_.end()&&mind->second.target==link.target)
                result.insert(mind->second.caster);
        }
        for(const auto&[target,pending]:energy_hex_pending_zeros_)result.insert(target);
        return result;
    }
    bool energy_hex_boundary_locked(std::uint64_t& boundary){
        std::set<skills::effects::Scope> referenced_casters;
        for(const auto& [key,owner]:energy_hex_links_)referenced_casters.insert(owner.caster);
        std::erase_if(retired_energy_hex_casters_,[&](const auto& entry){return !referenced_casters.contains(entry.first);});
        std::erase_if(energy_hex_pending_zeros_,[&](const auto& entry){
            if(const auto p=players_.find(entry.first.actor);p!=players_.end())return p->second.effect_scope!=entry.first;
            const auto npc=npc_combat_.find(entry.first.actor);
            return npc==npc_combat_.end()||npc->second.hex_scope()!=entry.first;
        });
        for(const auto&[target,pending]:energy_hex_pending_zeros_)boundary=std::min(boundary,pending.at);
        for(const auto scope:energy_hex_participants_locked()){
            const auto found=players_.find(scope.actor);
            if(found==players_.end()){
                const auto npc=npc_combat_.find(scope.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=scope)continue;
                const auto forecast=npc->second.forecast_energy_zero(npc->second.lifetime(),boundary);if(!forecast)return false;
                if(forecast->first_zero)boundary=std::min(boundary,*forecast->first_zero);
                auto copy=npc->second;const auto health=copy.advance_conditions(copy.lifetime(),boundary);if(!health)return false;
                if(health->killed)boundary=std::min(boundary,health->death_time);
                continue;
            }
            if(found->second.effect_scope!=scope)continue;
            auto& p=found->second;if(p.health_updated>boundary||p.energy_updated>boundary)return false;
            boundary=std::min(boundary,next_parasitic_health_boundary_locked(p,boundary));
            if(!p.overcast_curve||!p.resources||p.precise_health<=0)continue;
            const auto proposal=overcast::settle_until_zero(*p.overcast_curve,scope,*p.resources,p.energy_regeneration,
                p.energy_debt,boundary,p.energy_pips,p.energy_fractional_tail);
            if(!proposal)return false;if(proposal->first_zero)boundary=std::min(boundary,*proposal->first_zero);
        }
        return true;
    }
    bool dispatch_energy_hex_resources_locked(std::uint64_t at){
        const auto participants=energy_hex_participants_locked();
        // NPC death must end linked caster degeneration before that caster
        // integrates a later interval. Copies used in forecasting never publish.
        for(const auto scope:participants)if(!players_.contains(scope.actor)){
            auto npc=npc_combat_.find(scope.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=scope)continue;
            advance_npc_conditions_locked(scope.actor,npc->second,at);
            npc=npc_combat_.find(scope.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=scope)continue;
            if(!energy_hex_pending_zeros_.contains(scope)){
                if(next_mind_wrack_event_==UINT64_MAX)return false;
                if(const auto event=npc->second.pending_energy_zero(npc->second.lifetime(),next_mind_wrack_event_)){
                    energy_hex_pending_zeros_.emplace(scope,*event);++next_mind_wrack_event_;
                }
            }
        }
        for(const auto scope:participants){const auto found=players_.find(scope.actor);
            if(found==players_.end()||found->second.effect_scope!=scope)continue;auto& p=found->second;
            if(p.health_updated>at||p.energy_updated>at)return false;
            if(!p.overcast_curve||!p.resources||p.precise_health<=0)continue;
            const auto proposal=overcast::settle_until_zero(*p.overcast_curve,scope,*p.resources,p.energy_regeneration,
                p.energy_debt,at,p.energy_pips,p.energy_fractional_tail);
            if(!proposal||proposal->curve.cursor()!=at)return false;
            if(proposal->first_zero&&!energy_hex_pending_zeros_.contains(scope)){
                if(next_mind_wrack_event_==UINT64_MAX)return false;
                const auto event=skills::effects::energy_zero_observation(*proposal,scope,next_mind_wrack_event_++);
                if(!event)return false;energy_hex_pending_zeros_.emplace(scope,*event);
            }
        }
        // Preserve the original stopped snapshot and event identity before
        // advancing any actor. A later rejected ownership proposal must leave
        // this frontier pending even though Energy is now already zero.
        for(const auto scope:participants){const auto found=players_.find(scope.actor);
            if(found!=players_.end()&&found->second.effect_scope==scope&&!advance_resource_event_locked(found->second,at))return false;
        }
        return commit_energy_hex_zeros_locked(at);
    }
    bool commit_energy_hex_zeros_locked(std::uint64_t at){
        const auto zeros=energy_hex_pending_zeros_;
        for(const auto&[target,event]:zeros){
            if(event.at!=at)continue;const auto found=players_.find(target.actor);
            if(found==players_.end()){
                if(!commit_npc_energy_hex_zero_locked(target,event))return false;
                continue;
            }
            if(found==players_.end()||found->second.effect_scope!=target||found->second.precise_health<=0){energy_hex_pending_zeros_.erase(target);continue;}
            auto& p=found->second;std::vector<skills::effects::EnergyHexOwnership> owners;
            std::map<skills::effects::Scope,skills::regeneration::State> casters;
            skills::effects::EnergyHexRetirements retired;
            for(const auto&[key,link]:energy_hex_links_)if(link.target==target){
                if(at>=link.expires)continue;owners.push_back(link);
                if(link.caster_health_effect){const auto state=energy_hex_caster_state_locked(link.caster);if(!state)return false;
                    casters.emplace(link.caster,*state);
                    if(const auto at=energy_hex_caster_retirement_locked(link.caster))retired.emplace(link.caster,*at);
                }
            }
            if(owners.empty()){energy_hex_pending_zeros_.erase(target);continue;} // Death or expiry already ended the hex.
            std::optional<skills::effects::MindWrackOwnership> mind;
            if(const auto credit=mind_wrack_credit_.find(target.actor);credit!=mind_wrack_credit_.end()&&credit->second.target==target&&
                skills::effects::owned_mind_wrack(*p.effect_registry,{target,credit->second.caster,credit->second.effect},at))
                mind=skills::effects::MindWrackOwnership{target,credit->second.caster,credit->second.effect};
            auto [ledger,inserted]=energy_hex_zero_events_.try_emplace(target,target);
            auto result=skills::effects::prepare_energy_zero_observers(*p.effect_registry,*p.health_regeneration,*p.overcast_curve,
                casters,owners,mind,ledger->second,event,retired);if(!result)return false;
            *p.effect_registry=std::move(result->hexes.registry);*p.health_regeneration=std::move(result->hexes.health);
            *p.overcast_curve=std::move(result->hexes.energy);ledger->second=std::move(result->events);
            energy_hex_pending_zeros_.erase(target);
            commit_energy_hex_caster_states_locked(result->hexes.caster_health);
            std::vector<Packet> packets;
            for(const auto& owner:owners){energy_hex_links_.erase({target.actor,owner.effect});p.health_effects.erase(owner.effect);packets.push_back(health_effect_remove(target.actor,owner.effect));}
            std::optional<BackfireCredit> mind_credit;
            if(mind){mind_credit=mind_wrack_credit_.at(target.actor);mind_wrack_credit_.erase(target.actor);
                p.health_effects.erase(mind->effect);p.health_regeneration->remove(target,mind->effect);packets.push_back(health_effect_remove(target.actor,mind->effect));}
            for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);publish_status_locked(p,at);p.outbox->enqueue(owner_vitals(p));
            // All owned endings are committed before any lethal/reentrant hit.
            for(const auto& hit:result->hexes.damage){const auto caster=players_.find(hit.owner.caster.actor);
                if(caster!=players_.end()&&caster->second.effect_scope==hit.owner.caster&&caster->second.precise_health>0)
                    reactive_hex_damage_locked(target,{target,hit.owner.caster,hit.owner.effect,hit.owner.level},hit.owner.skill,hit.damage,at);}
            if(mind_credit&&result->mind_wrack){const auto caster=players_.find(mind_credit->caster.actor);
                if(caster!=players_.end()&&caster->second.effect_scope==mind_credit->caster&&caster->second.precise_health>0)
                    reactive_hex_damage_locked(target,*mind_credit,result->mind_wrack->hex.skill,result->mind_wrack->zero_damage,at);
            }
        }
        return true;
    }
    bool commit_npc_energy_hex_zero_locked(skills::effects::Scope target,const skills::effects::MindWrackTransition& event){
        const auto live=npc_combat_.find(target.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=target||!live->second.actor(live->second.lifetime())->health||
           live->second.actor(live->second.lifetime())->health->current<=0||!live->second.pending_energy_zero(live->second.lifetime(),event.event)){
            energy_hex_pending_zeros_.erase(target);return true;
        }
        std::vector<skills::effects::EnergyHexOwnership> owners;
        std::map<skills::effects::Scope,skills::regeneration::State> casters;
        for(const auto&[key,owner]:energy_hex_links_)if(owner.target==target&&event.at<owner.expires){
            owners.push_back(owner);if(!owner.caster_health_effect)continue;
            const auto source_state=energy_hex_caster_state_locked(owner.caster);
            if(!source_state){energy_hex_zero_deferred_="NPC linked caster unavailable";return false;}
            auto [state,inserted]=casters.try_emplace(owner.caster,*source_state);
            const auto entry=state->second.lookup(owner.caster,owner.caster_health_effect);
            // kill_locked legitimately replaces the dead actor's regeneration
            // state. A proved same-lifetime death has already retired this
            // linked timer; do not demand that it survive into resurrection.
            const auto retirement=energy_hex_caster_retirement_locked(owner.caster);
            if(!entry&&retirement&&*retirement<=event.at&&state->second.cursor()>=*retirement&&state->second.cursor()<=event.at)continue;
            if(!entry||entry->cause!=owner.caster.actor||entry->skill!=140||entry->pips!=-1||
               entry->begins!=owner.applied||entry->ends!=owner.expires||!state->second.end(owner.caster,owner.caster_health_effect,event.at)){
                energy_hex_zero_deferred_="NPC linked timer mismatch or future cursor "+std::to_string(state->second.cursor())+" event "+std::to_string(event.at);return false;
            }
        }
        std::optional<skills::effects::MindWrackOwnership> mind;
        if(const auto credit=mind_wrack_credit_.find(target.actor);credit!=mind_wrack_credit_.end()&&credit->second.target==target){
            const auto entry=live->second.hex_effect(live->second.lifetime(),credit->second.effect);
            if(entry&&entry->applied<=event.at&&entry->expires&&event.at<*entry->expires)
                mind=skills::effects::MindWrackOwnership{target,credit->second.caster,credit->second.effect};
        }
        auto proposed=live->second;const auto result=proposed.commit_energy_hex_zero(proposed.lifetime(),owners,mind,event);if(!result){
            energy_hex_zero_deferred_="NPC zero proposal rejected; owners "+std::to_string(owners.size())+" mind "+std::to_string(mind.has_value())+" clock "+std::to_string(live->second.health_clock(live->second.lifetime()).value_or(0));return false;
        }
        energy_hex_zero_deferred_.clear();
        live->second=std::move(proposed);
        commit_energy_hex_caster_states_locked(casters);
        for(const auto& owner:owners)energy_hex_links_.erase({target.actor,owner.effect});
        energy_hex_pending_zeros_.erase(target);
        std::optional<BackfireCredit> mind_credit;
        if(mind){mind_credit=mind_wrack_credit_.at(target.actor);mind_wrack_credit_.erase(target.actor);}
        for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,live->second.actor(live->second.lifetime())->effects)});
        // Commit every ending and caster history before lethal damage/credit callbacks.
        for(const auto& hit:result->damage){const auto caster=players_.find(hit.owner.caster.actor);
            if(caster!=players_.end()&&caster->second.effect_scope==hit.owner.caster&&caster->second.precise_health>0)
                reactive_hex_damage_locked(target,{target,hit.owner.caster,hit.owner.effect,hit.owner.level},hit.owner.skill,hit.damage,event.at);
        }
        if(mind_credit&&result->mind_wrack){const auto caster=players_.find(mind_credit->caster.actor);
            if(caster!=players_.end()&&caster->second.effect_scope==mind_credit->caster&&caster->second.precise_health>0)
                reactive_hex_damage_locked(target,*mind_credit,result->mind_wrack->hex.skill,result->mind_wrack->zero_damage,event.at);
        }
        return true;
    }
    bool observe_energy_hex_zero_locked(Player& target,const skills::effects::MindWrackEnergy& before,
            const skills::effects::MindWrackEnergy& after,std::uint64_t at){
        if(std::none_of(energy_hex_links_.begin(),energy_hex_links_.end(),[&](const auto& entry){
            return entry.second.target==target.effect_scope&&at>=entry.second.applied&&at<entry.second.expires;
        }))return false;
        const skills::effects::MindWrackTransition event{target.effect_scope,next_mind_wrack_event_,at,before,after};
        if(!skills::effects::mind_wrack_zero_transition(event))return false;
        if(next_mind_wrack_event_!=UINT64_MAX&&!energy_hex_pending_zeros_.contains(target.effect_scope)){
            ++next_mind_wrack_event_;energy_hex_pending_zeros_.emplace(target.effect_scope,event);
        }
        // An unsuccessful shared transaction remains pending; do not let the
        // legacy Mind Wrack-only path consume one member of that same event.
        (void)commit_energy_hex_zeros_locked(at);return true;
    }
    bool observe_npc_energy_hex_zero_locked(skills::effects::Scope target,const skills::effects::MindWrackEnergy& before,
            std::optional<skills::effects::MindWrackEnergy> committed_after,std::uint64_t at){
        if(std::none_of(energy_hex_links_.begin(),energy_hex_links_.end(),[&](const auto& entry){
            return entry.second.target==target&&entry.second.applied<=at&&at<entry.second.expires;
        }))return false;
        const auto npc=npc_combat_.find(target.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=target)return false;
        const auto life=npc->second.lifetime();const auto energy=npc->second.energy(life);const auto units=npc->second.precise_energy_units(life);
        if(!energy||!units||*units<0)return false;
        const auto after=committed_after.value_or(skills::effects::MindWrackEnergy{energy->current,energy->maximum,0,unsigned(*units%overcast::energy_unit)});
        if(!npc->second.retain_discrete_energy_zero(life,before,after,at))return false;
        if(next_mind_wrack_event_!=UINT64_MAX&&!energy_hex_pending_zeros_.contains(target)){
            energy_hex_pending_zeros_.emplace(target,*npc->second.pending_energy_zero(life,next_mind_wrack_event_++));
        }
        // Do not split off the legacy Mind Wrack observer if staging defers.
        if(timed_effect_boundary_){const auto pending=energy_hex_pending_zeros_.find(target);
            if(pending!=energy_hex_pending_zeros_.end())(void)commit_npc_energy_hex_zero_locked(target,pending->second);
        }
        else (void)settle_timed_effects_locked(at);
        return true;
    }
    void end_energy_hex_link_locked(Player& target,unsigned effect,std::uint64_t now){
        const auto found=energy_hex_links_.find({target.id,effect});
        if(found==energy_hex_links_.end()||found->second.target!=target.effect_scope)return;
        const auto owner=found->second;energy_hex_links_.erase(found);
        if(target.overcast_curve){
            if(now>=target.energy_updated){
                regenerate_energy_locked(target,now-target.energy_updated);target.energy_updated=now;
            }
            target.overcast_curve->remove_energy_modifier(owner.target,effect,target.overcast_curve->cursor());
        }
        if(owner.caster_health_effect){const auto caster=players_.find(owner.caster.actor);
            if(caster!=players_.end()&&caster->second.effect_scope==owner.caster&&caster->second.health_regeneration)
                caster->second.health_regeneration->end(owner.caster,owner.caster_health_effect,now);
        }
    }
    // Caller holds the instance lock. NPC copy and caster timers are published
    // together; no player-shaped Health authority is created for the NPC.
private:
    bool stage_npc_energy_hex_locked(Player& source,skills::effects::Scope target,
            const skills::effects::EnergyDegenerationHexPlan& plan,unsigned level,std::uint64_t now,std::vector<Packet>* visuals){
        if(!source.resources||source.next_health_effect>UINT32_MAX||next_npc_hex_==UINT32_MAX||source.knocked_down_until>now||
           !advance_resource_event_locked(source,now)||source.precise_health<=0||!source.health_regeneration)return false;
        auto live=npc_combat_.find(target.actor);if(live==npc_combat_.end()||live->second.hex_scope()!=target)return false;
        advance_npc_conditions_locked(target.actor,live->second,now);refresh_npc_movement_locked(target.actor,now);
        live=npc_combat_.find(target.actor);const auto body=npcs_.find(target.actor);
        if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=target||body->second.plane!=source.surface.plane)return false;
        const auto from=healing_position_locked(source,now);
        if(!from||std::hypot(body->second.x-from->x,body->second.y-from->z)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(source.surface,*from,{body->second.x,body->second.y});if(!line||line->plane!=body->second.plane)return false;}
        std::optional<skills::effects::EnergyHexOwnership> previous;
        for(const auto&[key,owner]:energy_hex_links_)if(owner.target==target&&owner.skill==plan.skill){if(previous)return false;previous=owner;}
        std::map<skills::effects::Scope,skills::regeneration::State> casters;
        casters.emplace(source.effect_scope,*source.health_regeneration);
        if(previous){const auto old=live->second.hex_effect(live->second.lifetime(),previous->effect);
            if(!old||old->cause!=previous->caster.actor||old->skill!=previous->skill||old->rank!=previous->rank||
               old->applied!=previous->applied||old->expires!=previous->expires)return false;
            if(previous->caster_health_effect){const auto old_state=energy_hex_caster_state_locked(previous->caster);if(!old_state)return false;
                casters.try_emplace(previous->caster,*old_state);
                auto& timers=casters.at(previous->caster);const auto entry=timers.lookup(previous->caster,previous->caster_health_effect);
                // Death already retired the same-lifetime linked timer. A new
                // caster may replace the still-active target hex without
                // recreating degeneration on the corpse.
                const auto retirement=energy_hex_caster_retirement_locked(previous->caster);
                const bool retired=!entry&&retirement&&*retirement<=now&&timers.cursor()>=*retirement&&timers.cursor()<=now;
                if(!retired&&(!entry||entry->cause!=previous->caster.actor||entry->skill!=140||entry->pips!=-1||
                   entry->begins!=previous->applied||entry->ends!=previous->expires||!timers.end(previous->caster,previous->caster_health_effect,now)))return false;
            }
        }
        const auto effect=next_npc_hex_;const auto linked=plan.caster_health_degen?static_cast<unsigned>(source.next_health_effect):0u;
        if(linked&&!casters.at(source.effect_scope).apply(source.effect_scope,
            {linked,source.id,plan.skill,-int(plan.caster_health_degen),now,now+plan.duration},false))return false;
        auto proposed=live->second;const auto applied=proposed.apply_energy_degeneration_hex(proposed.lifetime(),source.effect_scope,plan,effect,now);
        if(!applied||applied->removed.size()!=unsigned(previous.has_value())||
           (previous&&applied->removed.front().instance!=previous->effect))return false;
        live->second=std::move(proposed);commit_energy_hex_caster_states_locked(casters);
        if(previous)energy_hex_links_.erase({target.actor,previous->effect});
        energy_hex_links_[{target.actor,effect}]={source.effect_scope,target,effect,linked,plan.skill,plan.rank,level,now,now+plan.duration};
        ++next_npc_hex_;if(linked)++source.next_health_effect;
        npc_condition_credit_[{target.actor,source.id}]={target,source.effect_scope};
        for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,live->second.actor(live->second.lifetime())->effects)});
        commit_applications_locked(target,now);publish_target_body_locked(target.actor,plan.skill,source.id,visuals);
        return true;
    }
public:
    std::string energy_hex_zero_diagnostic()const{std::lock_guard lock(mutex_);return energy_hex_zero_deferred_;}
    std::optional<skills::effects::MindWrackTransition> pending_energy_zero_snapshot(skills::effects::Scope target)const{
        std::lock_guard lock(mutex_);const auto found=energy_hex_pending_zeros_.find(target);
        return found==energy_hex_pending_zeros_.end()?std::nullopt:std::optional{found->second};
    }
    // Trusted World integration. Remaining lifecycle/NPC and native casting
    // acceptance must be completed before exposing125/140 through Runtime.
    bool stage_energy_degeneration_hex(skills::effects::Scope caster,skills::effects::Scope target,
            const skills::effects::EnergyDegenerationHexPlan& plan,unsigned level,std::uint64_t now,
            std::vector<Packet>* completion_visuals=nullptr){
        if(completion_visuals)completion_visuals->clear();
        if(!skills::effects::valid_energy_degeneration_hex(plan)||!level||level>255||now>UINT64_MAX-plan.duration)return false;
        std::lock_guard lock(mutex_);const auto c=players_.find(caster.actor),t=players_.find(target.actor);
        if(c==players_.end()||caster.actor==target.actor||c->second.effect_scope!=caster)return false;
        if(t==players_.end())return stage_npc_energy_hex_locked(c->second,target,plan,level,now,completion_visuals);
        if(t->second.effect_scope!=target)return false;
        auto& source=c->second;auto& victim=t->second;
        if(!source.resources||!victim.resources||!source.effect_registry||!victim.effect_registry||
           !source.health_regeneration||!victim.health_regeneration||!source.team_token||!victim.team_token||source.team_token==victim.team_token||
           source.surface.plane!=victim.surface.plane||source.knocked_down_until>now||
           source.next_health_effect>UINT32_MAX||victim.next_health_effect>UINT32_MAX)return false;
        const auto from=healing_position_locked(source,now),to=healing_position_locked(victim,now);
        if(!from||!to||std::hypot(from->x-to->x,from->z-to->z)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(source.surface,*from,*to);if(!line||line->plane!=victim.surface.plane)return false;}
        if(!advance_resource_event_locked(source,now)||!advance_resource_event_locked(victim,now)||source.precise_health<=0||victim.precise_health<=0)return false;
        std::optional<skills::effects::EnergyHexOwnership> previous;
        for(const auto&[key,link]:energy_hex_links_)if(link.target==target&&link.skill==plan.skill){if(previous)return false;previous=link;}
        std::map<skills::effects::Scope,skills::regeneration::State> casters;
        casters.emplace(caster,*source.health_regeneration);
        if(previous&&previous->caster!=caster){const auto old=energy_hex_caster_state_locked(previous->caster);if(!old)return false;
            casters.emplace(previous->caster,*old);
        }
        const auto effect=static_cast<unsigned>(victim.next_health_effect);
        const auto linked=plan.caster_health_degen?static_cast<unsigned>(source.next_health_effect):0u;
        const skills::effects::EnergyHexOwnership owner{caster,target,effect,linked,plan.skill,plan.rank,level,now,now+plan.duration};
        auto curve=victim.overcast_curve.value_or(overcast::CeilingCurve(target,now,33));
        skills::effects::EnergyHexRetirements retired;
        if(previous)if(const auto at=energy_hex_caster_retirement_locked(previous->caster))retired.emplace(previous->caster,*at);
        auto change=skills::effects::prepare_energy_hex(*victim.effect_registry,*victim.health_regeneration,curve,casters,owner,previous,retired);
        if(!change)return false;
        const auto prevented=prevent_selected_hex_locked(source,victim,plan.skill,now);
        if(!prevented)return false;if(*prevented)return true;
        *victim.effect_registry=std::move(change->registry);*victim.health_regeneration=std::move(change->health);victim.overcast_curve=std::move(change->energy);
        commit_energy_hex_caster_states_locked(change->caster_health);
        if(previous)energy_hex_links_.erase({target.actor,previous->effect});
        energy_hex_links_[{target.actor,effect}]=owner;++victim.next_health_effect;if(linked)++source.next_health_effect;
        std::vector<Packet> packets;
        for(const auto& old:change->removed){victim.health_effects.erase(old.instance);packets.push_back(health_effect_remove(victim.id,old.instance));}
        const auto entry=*victim.effect_registry->lookup(target,effect);victim.health_effects.emplace(effect,entry);
        packets.push_back(health_effect_add(entry,victim.id,now));for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
        publish_status_locked(victim,now);victim.outbox->enqueue(owner_vitals(victim));
        commit_applications_locked(target,now);
        publish_target_body_locked(target.actor,plan.skill,caster.actor,completion_visuals);
        if(source.passive_recovery)source.passive_recovery->record(recovery::Event::target_enemy_spell_or_signet,now);
        if(victim.passive_recovery)victim.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
        return true;
    }
private:
