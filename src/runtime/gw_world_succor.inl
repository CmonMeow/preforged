// Maintained Succor consumer with scoped player and reviewed NPC recipients.
private:
    std::map<std::pair<skills::effects::Scope,unsigned>,skills::effects::SuccorLease> succor_leases_;
    std::map<std::pair<skills::effects::Scope,unsigned>,skills::effects::SuccorSpellConsumer> succor_consumers_;
    void end_succor_monitor_locked(Player& player,const skills::effects::Entry& entry,std::uint64_t now){
        const auto key=std::pair{player.effect_scope,entry.instance};
        const auto lease=succor_leases_.find(key);
        if(lease==succor_leases_.end()||lease->second.caster.actor!=entry.cause||
           !player.overcast_curve||player.energy_updated>now)
            throw std::logic_error("Succor scoped end mismatch");
        if(!advance_energy_locked_for_succor(player,now))throw std::logic_error("Succor end crossed Energy");
        if(!player.overcast_curve->remove_energy_modifier(player.effect_scope,entry.instance,now))
            throw std::logic_error("Succor missing Energy contribution");
        succor_consumers_.erase(key);succor_leases_.erase(key);
        end_mending_monitor_locked(player.effect_scope,entry,now);
        player.outbox->enqueue(owner_vitals(player));
    }
    bool advance_energy_locked_for_succor(Player& player,std::uint64_t now){
        if(now<player.energy_updated)return false;
        regenerate_energy_locked(player,now-player.energy_updated);player.energy_updated=now;return true;
    }
    bool succor_npc_recipient_ready_locked(skills::effects::Scope target,std::uint64_t now)const{
        const auto npc=npc_combat_.find(target.actor);
        return npc!=npc_combat_.end()&&npc->second.hex_scope()==target&&
            npc->second.can_receive_energy_enchantment(npc->second.lifetime(),now)&&
            maintained_range_path_locked(target,now,now).has_value();
    }
    bool complete_succor_npc_locked(skills::effects::Scope source,skills::effects::Scope target,
            unsigned divine,std::uint64_t now,std::vector<Packet>* deferred,bool completion){
        auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !advance_resource_event_locked(caster->second,now)||caster->second.precise_health<=0||
           !succor_npc_recipient_ready_locked(target,now))return false;
        auto npc=npc_combat_.find(target.actor);const auto life=npc->second.lifetime();
        auto preview=npc->second;
        const auto allocated=std::max(next_mending_effect_,std::uint64_t(next_npc_hex_));
        // The NPC allocator reserves UINT32_MAX as its exhausted sentinel.
        if(!allocated||allocated>=UINT32_MAX)return false;
        const auto effect=static_cast<unsigned>(allocated);
        const auto proposal=preview.apply_succor(life,source,effect,now);
        const auto visual=skills::particles::targeted_body_2022(target.actor,source.actor,308);
        if(!proposal||!visual)return false;
        const auto entry=proposal->registry.lookup(target,effect);if(!entry)return false;
        const auto wire=skills::effects::mending_wire(source,target,*entry);if(!wire)return false;
        auto ownership=mending_ownership_;
        for(const auto& old:proposal->removed){
            const auto lease=succor_leases_.find({target,old.instance});
            const auto current=npc->second.owned_effect_snapshot(life);
            if(lease==succor_leases_.end()||!current||
               !skills::effects::succor_contribution(lease->second,lease->second.caster,target,*current,now)||
               !preflight_maintained_recipient_end_locked(lease->second.caster,target,old.instance,now)||
               !ownership.ended(target,old.instance,now))return false;
        }
        if(!ownership.add_succor(source,target,effect,proposal->registry,now))return false;
        // Divine Favor is preflighted with the same copied recipient authority.
        if(divine){const auto allowed=preview.healing_allowed(life);
            if(!allowed||(*allowed&&!preview.heal_ally(life,std::lround(3.2*divine),now)))return false;}
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;
        std::set<const Outbox*> seen;for(auto&[id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto&[box,queue]:recipients)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        for(const auto& old:proposal->removed)
            if(!remove_maintained_recipient_locked(target,old.instance,now))throw std::logic_error("preflighted NPC Succor replacement failed");
        const auto before=npc->second.actor(life)->health->current;
        npc->second=std::move(preview);
        if(!caster->second.overcast_curve)caster->second.overcast_curve.emplace(source,now,33);
        mending_ownership_=std::move(ownership);
        succor_leases_.emplace(std::pair{target,effect},proposal->lease);
        succor_consumers_.emplace(std::pair{target,effect},skills::effects::SuccorSpellConsumer(proposal->lease));
        next_npc_hex_=static_cast<unsigned>(allocated+1);next_mending_effect_=allocated+1;
        sync_mending_curve_locked(source,now);
        caster->second.outbox->enqueue({wire->upkeep});
        const auto health=*npc->second.actor(life)->health;
        for(auto&[id,peer]:players_){std::vector<Packet> packets{wire->effect,
            combat::detail::fraction(target.actor,health),emotes::effects_2022(target.actor,npc->second.actor(life)->effects)};
            if(divine&&health.current>before)packets.push_back(skills::particles::health_change_2022(target.actor,source.actor,health.current-before,health.maximum));
            peer.outbox->enqueue(std::move(packets));}
        commit_applications_locked(target,now);
        if(!settle_timed_effects_locked(now))throw std::logic_error("NPC Succor crossed maintained boundary");
        captures.clear();for(auto&[box,queue]:recipients){
            std::vector<Packet> batch;
            if(completion)batch={combat::detail::integer(58,source.actor,0),combat::detail::integer(21,source.actor,547),*visual};
            batch.insert(batch.end(),std::make_move_iterator(queue.packets.begin()),std::make_move_iterator(queue.packets.end()));box->enqueue(std::move(batch));
        }
        if(!completion){if(deferred)deferred->push_back(*visual);else for(auto&[id,peer]:players_)peer.outbox->enqueue({*visual});}
        return true;
    }
public:
    bool succor_target_valid(skills::effects::Scope source,skills::effects::Scope target,unsigned divine,std::uint64_t now){
        std::lock_guard lock(mutex_);
        return skills::effects::succor()&&source.actor!=target.actor&&
            valid_ally_enchantment_target_locked(source,target,divine,now)&&
            (players_.contains(target.actor)||succor_npc_recipient_ready_locked(target,now));
    }
    bool complete_succor(skills::effects::Scope source,skills::effects::Scope target,
                        unsigned divine,std::uint64_t now,std::vector<Packet>* deferred=nullptr,bool completion=false){
        if(deferred)deferred->clear();
        if(completion&&deferred)return false;
        std::lock_guard lock(mutex_);
        const auto plan=skills::effects::succor();
        if(!plan||source.actor==target.actor||
           !valid_ally_enchantment_target_locked(source,target,divine,now))return false;
        if(!players_.contains(target.actor))return complete_succor_npc_locked(source,target,divine,now,deferred,completion);
        auto& caster=players_.at(source.actor);auto& recipient=players_.at(target.actor);
        if(!advance_resource_event_locked(caster,now)||!advance_resource_event_locked(recipient,now)||
           caster.precise_health<=0||recipient.precise_health<=0||!recipient.effect_registry||!recipient.health_regeneration)return false;
        // IDs share the recipient registry and the caster upkeep namespace.
        // Both counters are authoritative and include prior maintained spells.
        const auto allocated=std::max(next_mending_effect_,recipient.next_health_effect);
        if(allocated>UINT32_MAX)return false;
        const auto effect=static_cast<unsigned>(allocated);
        const auto proposal=skills::effects::prepare_succor(source,target,effect,now,*recipient.effect_registry);
        const auto visual=skills::particles::targeted_body_2022(target.actor,source.actor,308);
        if(!proposal||!visual)return false;
        const auto entry=proposal->registry.lookup(target,effect);if(!entry)return false;
        const auto wire=skills::effects::mending_wire(source,target,*entry);if(!wire)return false;
        auto timers=*recipient.health_regeneration;auto ownership=mending_ownership_;
        for(const auto& old:proposal->removed){
            if(!succor_leases_.contains({target,old.instance})||!timers.remove(target,old.instance)||
               !ownership.ended(target,old.instance,now))return false;
        }
        if(!timers.apply(target,{effect,source.actor,308,1,now,{}},true)||
           !ownership.add_succor(source,target,effect,proposal->registry,now))return false;
        auto curve=recipient.overcast_curve?*recipient.overcast_curve:overcast::CeilingCurve(target,now,33);
        if(!curve.apply_succor_modifier(target,effect,now))return false;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;
        std::set<const Outbox*> seen;for(auto&[id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto&[box,queue]:recipients)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        // Generic removal closes both ownership and spell-trigger generations.
        for(const auto& old:proposal->removed)remove_health_effect_locked(recipient,old.instance,now);
        if(!caster.overcast_curve)caster.overcast_curve.emplace(source,now,33);
        *recipient.effect_registry=proposal->registry;*recipient.health_regeneration=std::move(timers);
        recipient.overcast_curve=std::move(curve);mending_ownership_=std::move(ownership);
        recipient.health_effects[effect]=*entry;
        recipient.next_health_effect=std::max(recipient.next_health_effect,std::uint64_t(effect)+1);
        next_mending_effect_=std::max(next_mending_effect_,std::uint64_t(effect)+1);
        succor_leases_.emplace(std::pair{target,effect},proposal->lease);
        succor_consumers_.emplace(std::pair{target,effect},skills::effects::SuccorSpellConsumer(proposal->lease));
        sync_mending_curve_locked(source,now);
        caster.outbox->enqueue({wire->upkeep});for(auto&[id,peer]:players_)peer.outbox->enqueue({wire->effect});
        commit_applications_locked(target,now);
        if(divine&&recipient.precise_health>0){
            const auto before=recipient.precise_health;
            health_event_locked(recipient,static_cast<unsigned>(std::lround(3.2*divine)),now,true);
            const auto gained=recipient.precise_health-before;
            if(gained>0)for(auto&[id,peer]:players_)peer.outbox->enqueue({skills::particles::health_change_2022(target.actor,source.actor,gained,recipient.resources->max_health)});
            publish_health_locked(recipient);
        }
        publish_status_locked(recipient,now);recipient.outbox->enqueue(owner_vitals(recipient));
        if(!settle_timed_effects_locked(now))throw std::logic_error("Succor crossed maintained boundary");
        captures.clear();for(auto&[box,queue]:recipients){
            std::vector<Packet> batch;
            if(completion)batch={combat::detail::integer(58,source.actor,0),combat::detail::integer(21,source.actor,547),*visual};
            batch.insert(batch.end(),std::make_move_iterator(queue.packets.begin()),std::make_move_iterator(queue.packets.end()));box->enqueue(std::move(batch));
        }
        if(!completion){if(deferred)deferred->push_back(*visual);else for(auto&[id,peer]:players_)peer.outbox->enqueue({*visual});}
        return true;
    }
#include "gw_world_succor_spells.inl"
