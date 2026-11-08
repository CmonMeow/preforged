// Included inside Instance. Only actual direct-damage producers call this;
// generic Health changes include sacrifices/transfers and are not a trigger.
private:
    // Target and caster generations are retained separately from wire actor IDs.
    std::map<skills::effects::Scope,std::pair<unsigned,skills::effects::Scope>> npc_seed_casters_;
    // Preserve the existing global-friendly NPC domain. Only an explicit
    // pet/quest/opposition owner establishes a specific player-team restriction.
    bool seed_npc_owned_team_locked(skills::effects::Scope scope,std::uint64_t team)const{
        if(!team)return false;
        if(!pet_owners_.contains(scope)&&!npc_opposed_allies_.contains(scope)&&!quest_npc_owners_.contains(scope.actor))return true;
        const auto owned_team=npc_ally_team_locked(scope);
        return owned_team&&*owned_team==team;
    }
    std::optional<skills::effects::HealingSeedReaction>
    prepare_npc_healing_seed_direct_locked(skills::effects::Scope damaged,double loss,std::uint64_t now){
        using namespace skills::effects;
        if(!std::isfinite(loss)||loss<0)return {};
        if(!loss)return HealingSeedReaction{};
        auto live=npc_combat_.find(damaged.actor);const auto body=npcs_.find(damaged.actor);
        if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=damaged)return {};
        const auto life=live->second.lifetime();const auto registry=live->second.owned_effect_snapshot(life);
        if(!registry)return {};
        const auto effect=registry->newest_skill(damaged,274,now);
        if(!effect)return HealingSeedReaction{};
        const auto credit=npc_seed_casters_.find(damaged);
        if(credit==npc_seed_casters_.end()||credit->second.first!=effect->instance)return {};
        const auto caster=players_.find(credit->second.second.actor);
        if(caster==players_.end()||caster->second.effect_scope!=credit->second.second||!caster->second.team_token)return HealingSeedReaction{};
        const auto team=caster->second.team_token;
        if(!seed_npc_owned_team_locked(damaged,team))return HealingSeedReaction{};
        const auto plan=healing_seed(effect->rank);if(!plan)return {};
        const auto x=body->second.x,y=body->second.y;const auto plane=body->second.plane;
        std::vector<HealingSeedRecipient> candidates;
        for(auto& [id,ally]:players_){
            if(!ally.resources||ally.team_token!=team||ally.surface.plane!=plane)continue;
            const auto point=healing_position_locked(ally,now);if(!point)return {};
            const auto distance=std::hypot(point->x-x,point->z-y);
            if(!std::isfinite(distance))return {};
            if(distance>plan->adjacent)continue;
            if(ally.precise_health>0&&!advance_health_locked(ally,now))return {};
            candidates.push_back({ally.effect_scope,true,ally.precise_health>0,true,float(distance)});
        }
        std::vector<unsigned> ids;for(const auto& [id,npc]:npcs_)ids.push_back(id);
        for(const auto id:ids){
            refresh_npc_movement_locked(id,now);
            const auto placed=npcs_.find(id);const auto found=npc_combat_.find(id);
            if(placed==npcs_.end()||found==npc_combat_.end())continue;
            const auto recipient_life=found->second.lifetime();
            if(!found->second.healable_ally(recipient_life)||!seed_npc_owned_team_locked(found->second.hex_scope(),team))continue;
            const auto distance=std::hypot(placed->second.x-x,placed->second.y-y);
            if(!std::isfinite(distance))return {};
            if(placed->second.plane!=plane||distance>plan->adjacent)continue;
            // Seed grants Health; healing immunity is not an ally filter.
            candidates.push_back({found->second.hex_scope(),true,true,true,float(distance)});
        }
        const auto result=healing_seed_reaction(*registry,damaged,now,loss,candidates);if(!result)return {};
        for(const auto recipient:result->recipients){
            if(players_.contains(recipient.actor))continue;
            const auto found=npc_combat_.find(recipient.actor);
            if(found==npc_combat_.end()||found->second.hex_scope()!=recipient)return {};
            auto preview=found->second;
            if(!preview.gain_ally_health(preview.lifetime(),result->healing,now))return {};
        }
        return result;
    }
    std::optional<skills::effects::HealingSeedReaction>
    prepare_healing_seed_direct_locked(Player& damaged,double loss,std::uint64_t now){
        using namespace skills::effects;
        if(!std::isfinite(loss)||loss<0)return {};
        if(!loss||!damaged.effect_registry)return HealingSeedReaction{};
        const auto effect=damaged.effect_registry->newest_skill(damaged.effect_scope,274,now);
        if(!effect)
            return HealingSeedReaction{};
        const auto plan=healing_seed(effect->rank);
        if(!plan)return {};
        if(!damaged.resources||!damaged.team_token||now<damaged.health_updated)return {};
        const auto center=healing_position_locked(damaged,now);if(!center)return {};
        std::vector<HealingSeedRecipient> candidates;
        for(auto& [id,ally]:players_){
            if(!ally.resources||!ally.team_token||ally.team_token!=damaged.team_token)continue;
            if(ally.surface.plane!=damaged.surface.plane)continue;
            const auto point=healing_position_locked(ally,now);if(!point)return {};
            const auto distance=std::hypot(point->x-center->x,point->z-center->z);
            if(!std::isfinite(distance))return {};
            if(distance>plan->adjacent)continue;
            if(ally.effect_scope!=damaged.effect_scope&&ally.precise_health>0&&!advance_health_locked(ally,now))return {};
            // This is a potential recipient snapshot, not a prediction from
            // pre-conversion Health. Publication checks actual survival.
            candidates.push_back({ally.effect_scope,true,ally.precise_health>0,
                                  ally.surface.plane==damaged.surface.plane,float(distance)});
        }
        std::vector<unsigned> npc_ids;
        npc_ids.reserve(npcs_.size());
        for(const auto& [id,body]:npcs_)npc_ids.push_back(id);
        for(const auto id:npc_ids){
            refresh_npc_movement_locked(id,now);
            const auto placed=npcs_.find(id);
            const auto found=npc_combat_.find(id);if(found==npc_combat_.end())continue;
            if(placed==npcs_.end())continue;
            const auto life=found->second.lifetime();
            if(!found->second.healable_ally(life)||!seed_npc_owned_team_locked(found->second.hex_scope(),damaged.team_token))continue;
            const auto& body=placed->second;
            const auto distance=std::hypot(body.x-center->x,body.y-center->z);
            if(!std::isfinite(distance))return {};
            if(body.plane!=damaged.surface.plane||distance>plan->adjacent)continue;
            // Eligibility for Health gain does not manufacture heal permission.
            candidates.push_back({found->second.hex_scope(),true,true,true,float(distance)});
        }
        const auto result=healing_seed_reaction(*damaged.effect_registry,
            damaged.effect_scope,now,loss,candidates);
        if(!result)return {};
        for(const auto recipient:result->recipients){
            if(players_.contains(recipient.actor))continue;
            const auto live=npc_combat_.find(recipient.actor);
            if(live==npc_combat_.end()||live->second.hex_scope()!=recipient)return {};
            auto preview=live->second;
            if(!preview.gain_ally_health(preview.lifetime(),result->healing,now))return {};
        }
        return result;
    }

    void publish_healing_seed_direct_locked(
            const skills::effects::HealingSeedReaction& reaction,std::uint64_t now){
        if(!reaction.effect)return;
        for(const auto recipient:reaction.recipients){
            if(auto player=players_.find(recipient.actor);player!=players_.end()){
                auto& ally=player->second;
                // Damage/end reactions may retire a preflighted recipient.
                // Never resurrect a corpse or heal a recycled actor scope.
                if(ally.effect_scope!=recipient||!ally.resources||ally.precise_health<=0)continue;
                const auto before=ally.precise_health;
                health_event_locked(ally,reaction.healing,now);
                const auto gained=ally.precise_health-before;
                publish_health_locked(ally);
                if(gained>0)for(auto& [id,peer]:players_)peer.outbox->enqueue({
                    skills::particles::health_change_2022(recipient.actor,reaction.cause,
                        gained,ally.resources->max_health)});
                if(ally.precise_health<=0)kill_locked(ally,now);
            }else{
                auto live=npc_combat_.find(recipient.actor);
                if(live==npc_combat_.end()||live->second.hex_scope()!=recipient)continue;
                const auto life=live->second.lifetime();const auto actor=live->second.actor(life);
                if(!actor||!actor->health||actor->health->current<=0)continue;
                const auto before=actor->health->current;
                if(!live->second.gain_ally_health(life,reaction.healing,now))
                    throw std::logic_error("preflighted Healing Seed NPC Health gain failed");
                const auto health=*live->second.actor(life)->health;
                const auto gained=health.current-before;
                for(auto& [id,peer]:players_){
                    std::vector<Packet> packets{combat::detail::fraction(recipient.actor,health)};
                    if(gained>0)packets.push_back(skills::particles::health_change_2022(
                        recipient.actor,reaction.cause,gained,health.maximum));
                    peer.outbox->enqueue(std::move(packets));
                }
                // A real gain can settle pre-existing lethal wound debt.
                // Reacquire rather than retaining state across publication.
                live=npc_combat_.find(recipient.actor);
                const auto placed=npcs_.find(recipient.actor);
                if(live!=npc_combat_.end()&&placed!=npcs_.end()&&live->second.hex_scope()==recipient){
                    const auto* current=live->second.actor(life);
                    if(current&&current->health&&current->health->current<=0)
                        record_npc_death_locked(static_cast<Player*>(nullptr),placed->second,live->second,now);
                }
            }
        }
    }
