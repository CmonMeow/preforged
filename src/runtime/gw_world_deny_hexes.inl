public:
    // Trusted completion only. Runtime cooldown ordering and gateway binding
    // are separate; this never fabricates a recharge snapshot.
    bool complete_deny_hexes(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::DenyHexesPlan& plan,const skills::Slots& bar,
            const std::array<skills::effects::Time,8>& recharge_until,unsigned divine,std::uint64_t now){
        const auto canonical=skills::effects::deny_hexes();
        if(!canonical||plan!=*canonical)return false;
        std::lock_guard lock(mutex_);
        if(!valid_ally_enchantment_target_locked(source,target,divine,now))return false;
        auto player=players_.find(target.actor);auto npc=npc_combat_.find(target.actor);
        std::optional<skills::effects::Registry> npc_registry;
        const skills::effects::Registry* registry=nullptr;
        if(player!=players_.end()&&player->second.effect_scope==target&&player->second.effect_registry)registry=&*player->second.effect_registry;
        else if(npc!=npc_combat_.end()&&npc->second.hex_scope()==target){
            npc_registry=npc->second.owned_effect_snapshot(npc->second.lifetime());
            if(npc_registry)registry=&*npc_registry;
        }
        if(!registry)return false;
        const auto proposal=skills::effects::prepare_deny_hexes(plan,source,target,*registry,bar,recharge_until,now);
        if(!proposal)return false;
        std::optional<combat::npcs::LiveActor> proposed_npc;
        if(npc!=npc_combat_.end()&&npc->second.hex_scope()==target){
            proposed_npc=npc->second;
            const auto life=proposed_npc->lifetime();
            for(const auto& hex:proposal->removed)
                if(!proposed_npc->remove_hex_effect(life,hex.instance,now))return false;
            if(divine){const auto allowed=proposed_npc->healing_allowed(life);if(!allowed)return false;
                if(*allowed&&!proposed_npc->heal_ally(life,std::lround(3.2*divine),now))return false;}
        }
        const auto visual=skills::particles::targeted_body_2022(target.actor,source.actor,991);
        if(!visual)return false;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:recipients)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        if(proposed_npc){
            const auto life=proposed_npc->lifetime();
            const auto before=npc->second.actor(life)->health->current;
            npc->second=std::move(*proposed_npc);
            for(const auto& hex:proposal->removed){
                end_life_drain_locked({target.actor,hex.instance},now,false);
                if(const auto returned=parasitic_bonds_.end(target,hex.instance,now,true))deliver_parasitic_return_locked(*returned);
                for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_remove(target.actor,hex.instance)});
            }
            const auto* actor=npc->second.actor(life);
            if(!actor||!actor->health)throw std::logic_error("Deny Hexes NPC lifetime changed");
            const auto healed=std::max(0.,actor->health->current-before);
            for(auto& [id,peer]:players_){std::vector<Packet> packets{combat::detail::fraction(target.actor,*actor->health),emotes::effects_2022(target.actor,actor->effects)};
                if(healed)packets.push_back(skills::particles::health_change_2022(target.actor,source.actor,healed,actor->health->maximum));
                peer.outbox->enqueue(std::move(packets));}
        }else{
            auto& victim=player->second;
            for(const auto& hex:proposal->removed){
                if(victim.health_effects.contains(hex.instance))remove_health_effect_locked(victim,hex.instance,now);
                else{victim.effect_registry->remove(target,hex.instance);
                    for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_remove(target.actor,hex.instance)});}
            }
            if(divine&&victim.precise_health>0){health_event_locked(victim,std::lround(3.2*divine),now,true);publish_health_locked(victim);}
            publish_status_locked(victim,now);
        }
        commit_applications_locked(target,now);
        captures.clear();
        for(auto& [box,queue]:recipients){std::vector<Packet> batch{combat::detail::integer(58,source.actor,0),*visual};
            batch.insert(batch.end(),queue.packets.begin(),queue.packets.end());box->enqueue(std::move(batch));}
        return true;
    }
