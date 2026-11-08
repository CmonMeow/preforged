// Staged maintained application. Its reviewed attack consumer lives in
// gw_world_retribution_damage.inl; normal Runtime casting remains unbound.
private:
    std::map<std::pair<skills::effects::Scope,unsigned>,skills::effects::RetributionLease> retribution_leases_;
    std::optional<bool> retribution_npc_side_locked(skills::effects::Scope source)const{
        const auto n=npc_combat_.find(source.actor);const auto body=npcs_.find(source.actor);
        if(n==npc_combat_.end()||body==npcs_.end()||n->second.hex_scope()!=source||!body->second.combat_definition)return {};
        const auto relation=body->second.combat_definition->relation;
        if(relation==combat::npcs::Relation::hostile_to_players)return true;
        if(relation==combat::npcs::Relation::allied_to_players||relation==combat::npcs::Relation::protected_ally)return false;
        return {};
    }
    std::optional<unsigned> retribution_caster_level_locked(skills::effects::Scope source)const{
        if(const auto p=players_.find(source.actor);p!=players_.end()){
            if(p->second.effect_scope!=source||p->second.ordinary_level<1||p->second.ordinary_level>255)return {};return p->second.ordinary_level;
        }
        const auto n=npc_combat_.find(source.actor);const auto body=npcs_.find(source.actor);
        if(n==npc_combat_.end()||body==npcs_.end()||n->second.hex_scope()!=source||!body->second.definition.level)return {};
        return body->second.definition.level;
    }
    bool retribution_npc_source_ready_locked(skills::effects::Scope source,skills::effects::Scope target,unsigned divine,std::uint64_t now){
        if(divine>20||!settle_timed_effects_locked(now)||!retribution_caster_level_locked(source))return false;
        auto n=npc_combat_.find(source.actor);auto body=npcs_.find(source.actor);const auto side=retribution_npc_side_locked(source);
        if(n==npc_combat_.end()||body==npcs_.end()||!side||(divine&&body->second.definition.primary!=3)||
           n->second.knocked_down(n->second.lifetime(),now))return false;
        const auto life=n->second.lifetime();const auto actor=n->second.actor(life);if(!actor||!actor->health||actor->health->current<=0)return false;
        const auto source_owner=quest_npc_owners_.find(source.actor);
        if(source_owner!=quest_npc_owners_.end()){
            const auto owner=players_.find(source_owner->second.first.actor);
            if(owner==players_.end()||owner->second.effect_scope!=source_owner->second.first)return false;
        }
        refresh_npc_movement_locked(source.actor,now);body=npcs_.find(source.actor);if(body==npcs_.end())return false;
        const navigation::Point origin{body->second.x,body->second.y};const auto plane=body->second.plane;navigation::Point destination;
        if(const auto p=players_.find(target.actor);p!=players_.end()){
            if(source_owner!=quest_npc_owners_.end()&&source_owner->second.first!=target)return false;
            if(*side||p->second.effect_scope!=target||!p->second.team_token||p->second.surface.plane!=plane||
               !advance_health_locked(p->second,now)||p->second.precise_health<=0)return false;
            const auto point=healing_position_locked(p->second,now);if(!point)return false;destination=*point;
        }else{
            const auto target_owner=quest_npc_owners_.find(target.actor);
            if((source_owner==quest_npc_owners_.end())!=(target_owner==quest_npc_owners_.end())||
               (source_owner!=quest_npc_owners_.end()&&source_owner->second!=target_owner->second))return false;
            const auto other_side=retribution_npc_side_locked(target);if(!other_side||*other_side!=*side)return false;
            const auto other=npc_combat_.find(target.actor);if(other==npc_combat_.end()||!other->second.can_receive_retribution(other->second.lifetime(),now)||
                (divine&&!other->second.healing_allowed(other->second.lifetime()).has_value()))return false;
            if(source!=target){const auto allegiance=n->second.packed_allegiance(life),other_allegiance=other->second.packed_allegiance(other->second.lifetime());
                if(!allegiance||!*allegiance||allegiance!=other_allegiance)return false;}
            advance_npc_conditions_locked(target.actor,other->second,now);refresh_npc_movement_locked(target.actor,now);
            const auto placed=npcs_.find(target.actor);if(placed==npcs_.end()||placed->second.plane!=plane)return false;destination={placed->second.x,placed->second.y};
        }
        if(std::hypot(origin.x-destination.x,origin.z-destination.z)>1248)return false;
        if(navigation_){const auto cell=navigation_->locate_cell(plane,origin);if(!cell)return false;
            const auto trace=layered_->trace({plane,*cell},origin,destination);if(!trace||trace->plane!=plane)return false;}
        n=npc_combat_.find(source.actor);
        if(n==npc_combat_.end()||!n->second.matches(life)||!n->second.prepare_maintained_caster(life,now))return false;
        return maintained_range_path_locked(source,now,now).has_value();
    }
    bool end_npc_retribution_monitor_locked(skills::effects::Scope target,const skills::effects::Entry& entry,std::uint64_t now){
        const auto found=retribution_leases_.find({target,entry.instance});if(found==retribution_leases_.end())return false;
        if(entry.skill!=248||entry.cause!=found->second.caster.actor||entry.rank!=found->second.entry.rank||
           entry.applied!=found->second.entry.applied||entry.expires||
           mending_ownership_.current_maintainer(target,entry.instance)!=std::optional(found->second.caster))
            throw std::logic_error("NPC Retribution end generation mismatch");
        const auto caster=players_.find(found->second.caster.actor);
        if(caster!=players_.end()&&caster->second.effect_scope==found->second.caster&&
           !advance_energy_locked_for_succor(caster->second,now))throw std::logic_error("NPC Retribution end crossed Energy");
        retribution_leases_.erase(found);end_mending_monitor_locked(target,entry,now);return true;
    }
    void end_retribution_monitor_locked(Player& player,const skills::effects::Entry& entry,std::uint64_t now){
        const auto key=std::pair{player.effect_scope,entry.instance};const auto found=retribution_leases_.find(key);
        if(found==retribution_leases_.end()||entry.skill!=248||entry.cause!=found->second.caster.actor||
           entry.rank!=found->second.entry.rank||entry.applied!=found->second.entry.applied||entry.expires||
           mending_ownership_.current_maintainer(player.effect_scope,entry.instance)!=std::optional(found->second.caster))
            throw std::logic_error("invalid owned Retribution end");
        retribution_leases_.erase(found);end_mending_monitor_locked(player.effect_scope,entry,now);
    }
    bool stage_retribution_npc_locked(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::RetributionPlan& plan,std::uint64_t now,unsigned divine,bool completion,bool prepare_only=false){
        auto caster=players_.find(source.actor);auto npc=npc_combat_.find(target.actor);
        if(caster!=players_.end()&&(caster->second.effect_scope!=source||!advance_resource_event_locked(caster->second,now)||caster->second.precise_health<=0))return false;
        if(npc==npc_combat_.end()||npc->second.hex_scope()!=target||!npc->second.can_receive_retribution(npc->second.lifetime(),now)||
           !maintained_range_path_locked(target,now,now)||retribution_leases_.size()>=4096)return false;
        const auto visual=source==target?skills::particles::target_body_2022(target.actor,248):skills::particles::targeted_body_2022(target.actor,source.actor,248);if(completion&&!visual)return false;
        if(well_blocks_npc_enchantment_locked(target.actor,now)){
            if(prepare_only)return true;
            if(completion)for(auto&[id,peer]:players_)peer.outbox->enqueue({combat::detail::integer(58,source.actor,0),*visual});return true;
        }
        const auto allocated=std::max(next_mending_effect_,std::uint64_t(next_npc_hex_));if(!allocated||allocated>=UINT32_MAX)return false;
        const auto effect=static_cast<unsigned>(allocated);const auto life=npc->second.lifetime();
        auto preview=npc->second;const auto proposal=preview.apply_retribution(life,source,plan,effect,now,mending_ownership_);if(!proposal)return false;
        const auto wire=skills::effects::mending_wire(source,target,proposal->lease.entry);if(!wire)return false;
        const auto original=npc->second.owned_effect_snapshot(life);if(!original)return false;
        for(const auto& old:proposal->removed){
            const auto lease=retribution_leases_.find({target,old.instance});
            if(lease==retribution_leases_.end()||!skills::effects::retribution_lease_matches(lease->second,*original,mending_ownership_,now)||
               !preflight_maintained_recipient_end_locked(lease->second.caster,target,old.instance,now))return false;
        }
        if(divine){const auto allowed=preview.healing_allowed(life);if(!allowed)return false;
            // Source/target allegiance was checked by the World admission path.
            auto heal_preview=preview;
            if(*allowed&&!(caster!=players_.end()?heal_preview.heal_ally(life,std::lround(3.2*divine),now):heal_preview.heal(life,std::lround(3.2*divine),now)))return false;}
        if(prepare_only)return true;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;std::set<const Outbox*> seen;
        for(auto&[id,peer]:players_)if(seen.insert(peer.outbox.get()).second)recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::SubCapture>> captures;for(auto&[box,queue]:recipients)captures.push_back(std::make_unique<Outbox::SubCapture>(*box,queue));
        for(const auto& old:proposal->removed)if(!remove_maintained_recipient_locked(target,old.instance,now))throw std::logic_error("preflighted NPC Retribution replacement failed");
        npc->second=std::move(preview);
        if(caster!=players_.end()&&!caster->second.overcast_curve)caster->second.overcast_curve.emplace(source,now,33);
        mending_ownership_=proposal->ownership;retribution_leases_.emplace(std::pair{target,effect},proposal->lease);
        next_npc_hex_=static_cast<unsigned>(allocated+1);next_mending_effect_=allocated+1;sync_mending_curve_locked(source,now);
        if(caster!=players_.end())caster->second.outbox->enqueue({wire->upkeep});
        commit_applications_locked(target,now);
        npc=npc_combat_.find(target.actor);
        if(divine&&npc!=npc_combat_.end()&&npc->second.hex_scope()==target){
            const auto* current=npc->second.actor(life);const auto allowed=npc->second.healing_allowed(life);
            if(current&&current->health&&current->health->current>0&&allowed&&*allowed){
                const auto before=current->health->current;
                if(!(players_.contains(source.actor)?npc->second.heal_ally(life,std::lround(3.2*divine),now):npc->second.heal(life,std::lround(3.2*divine),now)))
                    throw std::logic_error("validated NPC Retribution Divine Favor failed");
                const auto healed=*npc->second.actor(life)->health;
                for(auto&[id,peer]:players_){std::vector<Packet> packets{combat::detail::fraction(target.actor,healed)};
                    if(healed.current>before)packets.push_back(skills::particles::health_change_2022(target.actor,source.actor,healed.current-before,healed.maximum));peer.outbox->enqueue(std::move(packets));}
            }
        }
        npc=npc_combat_.find(target.actor);
        if(npc!=npc_combat_.end()&&npc->second.hex_scope()==target){
            const auto* current=npc->second.actor(life);
            if(current&&current->health&&current->health->current<=0){
                const auto placed=npcs_.find(target.actor);
                if(placed!=npcs_.end())record_npc_death_locked(static_cast<Player*>(nullptr),placed->second,npc->second,now);
            }
        }
        npc=npc_combat_.find(target.actor);
        if(npc!=npc_combat_.end()&&npc->second.hex_scope()==target){
            const auto* current=npc->second.actor(life);
            if(current&&current->health&&current->health->current>0)
                for(auto&[id,peer]:players_)peer.outbox->enqueue({wire->effect,combat::detail::fraction(target.actor,*current->health),emotes::effects_2022(target.actor,current->effects)});
        }
        if(!settle_timed_effects_locked(now))throw std::logic_error("NPC Retribution crossed maintained boundary");
        captures.clear();for(auto&[box,queue]:recipients){std::vector<Packet> batch;if(completion)batch={combat::detail::integer(58,source.actor,0),*visual};
            batch.insert(batch.end(),std::make_move_iterator(queue.packets.begin()),std::make_move_iterator(queue.packets.end()));box->enqueue(std::move(batch));}
        return true;
    }
    bool stage_retribution_locked(skills::effects::Scope source,skills::effects::Scope target,const skills::effects::RetributionPlan& plan,std::uint64_t now,unsigned divine,bool completion,bool prepare_only=false){
        const auto canonical=skills::effects::retribution(plan.rank);if(!canonical||*canonical!=plan)return false;
        if(players_.contains(source.actor)){if(!valid_ally_enchantment_target_locked(source,target,divine,now))return false;}
        else if(!retribution_npc_source_ready_locked(source,target,divine,now))return false;
        const auto selected=players_.find(target.actor);if(selected==players_.end())return stage_retribution_npc_locked(source,target,plan,now,divine,completion,prepare_only);
        const auto visual=target==source?skills::particles::target_body_2022(target.actor,248):skills::particles::targeted_body_2022(target.actor,source.actor,248);if(completion&&!visual)return false;
        const auto caster=players_.find(source.actor);
        if(caster!=players_.end()&&!advance_resource_event_locked(caster->second,now))return false;
        auto& recipient=selected->second;
        if(!recipient.effect_registry||!recipient.health_regeneration||!recipient.resources||!recipient.resources->max_energy)return false;
        if(retribution_leases_.size()>=4096)return false;
        if(well_blocks_player_enchantment_locked(recipient,now)){if(prepare_only)return true;if(completion)for(auto&[id,peer]:players_)peer.outbox->enqueue({combat::detail::integer(58,source.actor,0),*visual});return true;}
        const auto allocated=std::max(next_mending_effect_,recipient.next_health_effect);if(!allocated||allocated>=UINT32_MAX)return false;
        const auto effect=static_cast<unsigned>(allocated);
        auto proposed=skills::effects::prepare_retribution(plan,source,target,effect,now,*recipient.effect_registry,mending_ownership_);if(!proposed)return false;
        const auto wire=skills::effects::mending_wire(source,target,proposed->lease.entry);if(!wire)return false;
        std::vector<std::pair<skills::effects::Scope,skills::effects::MendingWire>> old;
        for(const auto& entry:proposed->removed){const auto lease=retribution_leases_.find({target,entry.instance});if(lease==retribution_leases_.end()||!skills::effects::retribution_lease_matches(lease->second,*recipient.effect_registry,mending_ownership_,now))return false;const auto owner=mending_ownership_.current_maintainer(target,entry.instance);const auto ended=owner?skills::effects::mending_end_wire(*owner,target,entry):std::nullopt;if(!owner||!ended||!preflight_maintained_recipient_end_locked(*owner,target,entry.instance,now))return false;old.emplace_back(*owner,*ended);}
        if(prepare_only)return true;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::SubCapture>> captures;for(auto& [box,queue]:recipients)captures.push_back(std::make_unique<Outbox::SubCapture>(*box,queue));
        if(caster!=players_.end()&&!caster->second.overcast_curve)caster->second.overcast_curve.emplace(source,now,33);
        *recipient.effect_registry=std::move(proposed->registry);mending_ownership_=std::move(proposed->ownership);
        sync_mending_curve_locked(source,now);for(const auto& [owner,unused]:old)if(owner!=source)sync_mending_curve_locked(owner,now);
        for(const auto& entry:proposed->removed){recipient.health_effects.erase(entry.instance);retribution_leases_.erase({target,entry.instance});}
        retribution_leases_.emplace(std::pair{target,effect},proposed->lease);
        recipient.health_effects.emplace(effect,proposed->lease.entry);recipient.next_health_effect=next_mending_effect_=allocated+1;
        for(const auto& [owner,ended]:old){const auto caster=players_.find(owner.actor);if(caster!=players_.end()&&caster->second.effect_scope==owner)caster->second.outbox->enqueue({ended.upkeep});for(auto& [id,peer]:players_)peer.outbox->enqueue({ended.effect});}
        if(caster!=players_.end())caster->second.outbox->enqueue({wire->upkeep});
        commit_applications_locked(target,now);
        if(divine&&recipient.precise_health>0){const auto before=recipient.precise_health;health_event_locked(recipient,static_cast<unsigned>(std::lround(3.2*divine)),now,true);const auto gained=recipient.precise_health-before;
            if(gained>0)for(auto& [id,peer]:players_)peer.outbox->enqueue({skills::particles::health_change_2022(target.actor,source.actor,gained,recipient.resources->max_health)});publish_health_locked(recipient);}
        if(recipient.precise_health<=0)kill_locked(recipient,now);
        const auto current=players_.find(target.actor);
        if(current!=players_.end()&&current->second.effect_scope==target&&current->second.precise_health>0){
            for(auto& [id,peer]:players_)peer.outbox->enqueue({wire->effect});
            publish_status_locked(current->second,now);
        }
        if(!settle_timed_effects_locked(now))throw std::logic_error("Retribution application crossed resource boundary");
        captures.clear();for(auto& [box,queue]:recipients){std::vector<Packet> batch;if(completion)batch={combat::detail::integer(58,source.actor,0),*visual};batch.insert(batch.end(),std::make_move_iterator(queue.packets.begin()),std::make_move_iterator(queue.packets.end()));box->enqueue(std::move(batch));}return true;
    }
public:
    struct RetributionCompletion {bool accepted=false;bool completion_prefix_published=false;};
    bool stage_retribution(skills::effects::Scope source,skills::effects::Scope target,const skills::effects::RetributionPlan& plan,std::uint64_t now,unsigned divine=0,bool completion=false){
        std::lock_guard lock(mutex_);return stage_retribution_locked(source,target,plan,now,divine,completion);
    }
    bool retribution_target_valid(skills::effects::Scope source,skills::effects::Scope target,unsigned divine,std::uint64_t now){
        std::lock_guard lock(mutex_);
        if(!players_.contains(source.actor))return false;
        const auto plan=skills::effects::retribution(0);
        return plan&&stage_retribution_locked(source,target,*plan,now,divine,false,true);
    }
    RetributionCompletion complete_retribution(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::RetributionPlan& plan,unsigned divine,std::uint64_t now,std::uint64_t completion_event,
            std::vector<Packet>* visuals=nullptr,std::function<void()>* publication=nullptr){
        if(visuals)visuals->clear();if(publication)*publication={};
        if(!completion_event)return {};
        std::lock_guard lock(mutex_);
        // Admission settles real due timelines. Capture that publication too,
        // so an accepted deferred completion has no preflight output leak.
        DamagePublication pending;begin_damage_publication_locked(pending);
        const auto reject=[&]()->RetributionCompletion{
            auto release=finish_damage_publication_locked(pending);
            if(release)release();return {};
        };
        if(!players_.contains(source.actor)||!stage_retribution_locked(source,target,plan,now,divine,false,true)||
           !complete_ether_renewal_locked(source,248,completion_event,now,true,false))return reject();
        const auto visual=source==target?skills::particles::target_body_2022(target.actor,248):skills::particles::targeted_body_2022(target.actor,source.actor,248);
        if(!visual)return reject();
        if(!complete_ether_renewal_locked(source,248,completion_event,now,true))return reject();
        // Reward reactions can retire either generation. An admitted paid cast
        // still completes, but must never restore its pre-reward proposal.
        const auto current_scope=[&](skills::effects::Scope scope){
            const auto player=players_.find(scope.actor);
            if(player!=players_.end())return player->second.effect_scope==scope&&player->second.precise_health>0;
            const auto npc=npc_combat_.find(scope.actor);
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=scope)return false;
            const auto actor=npc->second.actor(npc->second.lifetime());
            return actor&&actor->health&&actor->health->current>0;
        };
        bool consumed=!current_scope(source)||!current_scope(target);
        if(!consumed){
            const auto recipient=players_.find(target.actor);
            consumed=recipient!=players_.end()?well_blocks_player_enchantment_locked(recipient->second,now):
                well_blocks_npc_enchantment_locked(target.actor,now);
            if(!consumed&&!stage_retribution_locked(source,target,plan,now,divine,false))
                throw std::logic_error("preflighted Retribution changed after admitted Renewal");
            consumed=consumed||!current_scope(source)||!current_scope(target);
        }
        const bool owns_prefix=consumed||!publication||!visuals;
        // Prefix ownership includes deferred publication: keep it in the same
        // captured batch as reactions, before any effect/death output.
        if(owns_prefix){
            if(completion_prefix_publication_locked())
                skill_event_locked(source.actor,{combat::detail::integer(58,source.actor,0),*visual},now);
            for(auto& [box,queue]:pending.queues)
                queue.packets.insert(queue.packets.begin(),{combat::detail::integer(58,source.actor,0),*visual});
        }else visuals->push_back(*visual);
        auto release=finish_damage_publication_locked(pending);
        if(publication)*publication=std::move(release);else if(release)release();
        return {true,owns_prefix};
    }
