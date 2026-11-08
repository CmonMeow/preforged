private:
    bool complete_convert_hexes_npc_locked(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::ConvertHexesPlan& plan,unsigned divine,std::uint64_t now){
        if(!valid_ally_enchantment_target_locked(source,target,divine,now)||next_npc_hex_==UINT32_MAX)return false;
        const auto found=npc_combat_.find(target.actor);
        if(found==npc_combat_.end()||found->second.hex_scope()!=target)return false;
        auto& live=found->second;const auto life=live.lifetime();
        const auto original=live.owned_effect_snapshot(life);if(!original)return false;
        const auto old=original->newest_skill(target,303,now);const auto effect=next_npc_hex_;
        auto proposed=live;const auto outcome=proposed.apply_convert_hexes(life,source,plan,effect,now);if(!outcome)return false;
        if(divine){const auto allowed=proposed.healing_allowed(life);if(!allowed)return false;
            if(*allowed&&!proposed.heal_ally(life,std::lround(3.2*divine),now))return false;}
        const auto before=live.actor(life)->health->current;
        const auto divine_gained=std::max(0.,proposed.actor(life)->health->current-before);
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;
        std::set<const Outbox*> seen;for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;for(auto& [box,queue]:recipients)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        // Commit before end-credit callbacks;never overwrite their later state
        // with the preflight copy. The recipient's exact lifetime stays owned.
        live=std::move(proposed);
        for(const auto& hex:outcome->removed){
            end_life_drain_locked({target.actor,hex.instance},now,false);
            if(const auto returned=parasitic_bonds_.end(target,hex.instance,now,true))deliver_parasitic_return_locked(*returned);
            for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_remove(target.actor,hex.instance)});
        }
        if(outcome->armor){
            ++next_npc_hex_;
            if(old){npc_simple_enchantment_credit_.erase({target.actor,old->instance});for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_remove(target.actor,old->instance)});}
            npc_simple_enchantment_credit_[{target.actor,effect}]=target;
            const auto applied=live.hex_effect(life,effect);if(!applied)throw std::logic_error("committed NPC Convert Hexes armor absent");
            for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_add(*applied,target.actor,now)});
        }
        const auto* actor=live.actor(life);if(!actor||!actor->health)throw std::logic_error("Convert Hexes NPC lifetime changed during completion");
        for(auto& [id,peer]:players_){std::vector<Packet> packets{combat::detail::fraction(target.actor,*actor->health),emotes::effects_2022(target.actor,actor->effects)};
            if(divine_gained)packets.push_back(skills::particles::health_change_2022(target.actor,source.actor,divine_gained,actor->health->maximum));peer.outbox->enqueue(std::move(packets));}
        commit_applications_locked(target,now);captures.clear();const auto visual=skills::particles::targeted_body_2022(target.actor,source.actor,303);
        if(!visual)throw std::logic_error("NPC Convert Hexes target resource missing");
        for(auto& [box,queue]:recipients){std::vector<Packet> batch{combat::detail::integer(58,source.actor,0),*visual};batch.insert(batch.end(),queue.packets.begin(),queue.packets.end());box->enqueue(std::move(batch));}
        return true;
    }
public:
    // Trusted World completion. Normal Runtime/gateway binding remains staged.
    bool complete_convert_hexes(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::ConvertHexesPlan& plan,unsigned divine,std::uint64_t now){
        const auto canonical=skills::effects::convert_hexes(plan.rank);
        if(!canonical||*canonical!=plan||source.actor==target.actor)return false;
        std::lock_guard lock(mutex_);
        const auto recipient=players_.find(target.actor);
        if(recipient==players_.end())return complete_convert_hexes_npc_locked(source,target,plan,divine,now);
        if(recipient->second.effect_scope!=target||
           !valid_ally_enchantment_target_locked(source,target,divine,now))return false;
        auto& player=recipient->second;
        if(!player.effect_registry||!player.health_regeneration||player.next_health_effect>UINT32_MAX)return false;
        const auto proposal=skills::effects::prepare_convert_hexes(plan,source,target,*player.effect_registry,now);if(!proposal)return false;
        const auto old=player.effect_registry->newest_skill(target,303,now);
        // The captures do not establish whether a zero-count recast replaces
        // an existing armor bonus. Keep that case unbound rather than guess.
        if(old&&!proposal->necromancer_hexes)return false;
        const auto effect=static_cast<unsigned>(player.next_health_effect);
        const skills::effects::Entry armor{effect,source.actor,303,plan.rank,skills::effects::Category::other,now,proposal->armor_until};
        if(proposal->armor){
            auto preview=*player.effect_registry;auto timers=*player.health_regeneration;
            for(const auto& hex:proposal->removed){preview.remove(target,hex.instance);timers.remove(target,hex.instance);}
            if(old){preview.remove(target,old->instance);timers.remove(target,old->instance);}
            if(!preview.apply(target,armor,skills::effects::Replace::same_skill).accepted||
               !timers.apply(target,{effect,source.actor,303,0,now,proposal->armor_until}))return false;
        }
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;
        std::set<const Outbox*> seen;for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;for(auto& [box,queue]:recipients)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        for(const auto& hex:proposal->removed){
            if(player.health_effects.contains(hex.instance))remove_health_effect_locked(player,hex.instance,now);
            else{player.effect_registry->remove(target,hex.instance);for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_remove(target.actor,hex.instance)});}
        }
        if(proposal->armor&&player.precise_health>0){
            if(old)remove_health_effect_locked(player,old->instance,now);
            if(!player.effect_registry->apply(target,armor,skills::effects::Replace::same_skill).accepted||
               !player.health_regeneration->apply(target,{effect,source.actor,303,0,now,proposal->armor_until}))
                throw std::logic_error("preflighted Convert Hexes armor failed");
            ++player.next_health_effect;player.health_effects.emplace(effect,armor);
            player.convert_hexes_armor=skills::effects::ConvertHexesArmor{source,target,effect,plan.rank,proposal->necromancer_hexes,now,proposal->armor_until};
            for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_add(armor,target.actor,now)});
        }
        if(divine&&player.precise_health>0){health_event_locked(player,std::lround(3.2*divine),now,true);if(player.precise_health<=0)kill_locked(player,now);publish_health_locked(player);}
        commit_applications_locked(target,now);publish_status_locked(player,now);
        captures.clear();const auto visual=skills::particles::targeted_body_2022(target.actor,source.actor,303);
        if(!visual)throw std::logic_error("Convert Hexes target resource missing");
        for(auto& [box,queue]:recipients){std::vector<Packet> batch{combat::detail::integer(58,source.actor,0),*visual};batch.insert(batch.end(),queue.packets.begin(),queue.packets.end());box->enqueue(std::move(batch));}
        return true;
    }
    std::optional<unsigned> convert_hexes_armor(skills::effects::Scope target,std::uint64_t now){
        std::lock_guard lock(mutex_);const auto player=players_.find(target.actor);
        if(player==players_.end()){
            const auto npc=npc_combat_.find(target.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=target)return {};
            advance_npc_conditions_locked(target.actor,npc->second,now);return npc->second.convert_hexes_armor(npc->second.lifetime(),now);
        }
        if(player==players_.end()||player->second.effect_scope!=target||!player->second.effect_registry||
           !advance_resource_event_locked(player->second,now))return {};
        return skills::effects::active_convert_hexes_armor(*player->second.effect_registry,target,player->second.convert_hexes_armor,now);
    }
