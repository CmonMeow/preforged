// Included inside Instance after gw_selfless_spirit_registry.h is included at
// file scope. Runtime owns paid admission; this method owns effect publication.
public:
    bool complete_selfless_spirit(skills::effects::Scope source,
            const skills::effects::SelflessSpiritPlan& plan,unsigned divine_rank,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr){
        if(skills::effects::selfless_spirit(plan.skill,plan.rank)!=plan)return false;
        std::lock_guard lock(mutex_);
        if(!valid_ally_enchantment_target_locked(source,source,divine_rank,now))return false;
        auto& player=players_.at(source.actor);
        if(!player.effect_registry||!player.health_regeneration||player.next_health_effect>UINT32_MAX)return false;
        if(adjusted&&player.effect_registry->newest_skill(source,256,now))return false;
        const auto effect=static_cast<unsigned>(player.next_health_effect);
        const auto stage=skills::effects::stage_selfless_spirit_registry(*player.effect_registry,source,effect,plan,now,adjusted);
        if(!stage)return false;
        // Settlement above must have retired every unrelated timer before the
        // proposal is copied. Never silently bypass an owned end consumer.
        for(const auto& old:stage->removed)if(!skills::effects::selfless_spirit_alias(old.skill))return false;
        const std::vector<Packet> prefix{combat::detail::integer(58,source.actor,0),combat::detail::integer(21,source.actor,553)};
        if(well_blocks_player_enchantment_locked(player,now)){
            for(auto& [id,peer]:players_)peer.outbox->enqueue(prefix);
            return true;
        }
        auto timers=*player.health_regeneration;
        for(const auto& old:stage->removed)timers.remove(source,old.instance);
        if(!timers.apply(source,{effect,source.actor,plan.skill,0,now,*stage->added.expires}))return false;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;
        recipients.reserve(players_.size());std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:recipients)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        *player.effect_registry=stage->registry;*player.health_regeneration=std::move(timers);++player.next_health_effect;
        for(const auto& old:stage->removed){
            player.health_effects.erase(old.instance);
            for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_remove(source.actor,old.instance)});
        }
        player.health_effects.emplace(effect,stage->added);
        for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_add(stage->added,source.actor,now)});
        commit_applications_locked(source,now);
        if(divine_rank&&player.precise_health>0){
            const auto before=player.precise_health;
            health_event_locked(player,static_cast<unsigned>(std::lround(3.2*divine_rank)),now,true);
            const auto gained=player.precise_health-before;
            if(gained>0)for(auto& [id,peer]:players_)peer.outbox->enqueue({skills::particles::health_change_2022(source.actor,source.actor,gained,player.resources->max_health)});
            publish_health_locked(player);
        }
        publish_status_locked(player,now);captures.clear();
        for(auto& [box,queue]:recipients){auto batch=prefix;batch.insert(batch.end(),std::make_move_iterator(queue.packets.begin()),std::make_move_iterator(queue.packets.end()));box->enqueue(std::move(batch));}
        return true;
    }
    // Snapshot only: payment consumers must query again inside their debit lock.
    std::optional<std::optional<skills::effects::Entry>> selfless_spirit_state(
            skills::effects::Scope source,std::uint64_t now){
        std::lock_guard lock(mutex_);const auto found=players_.find(source.actor);
        if(found==players_.end()||found->second.effect_scope!=source||!found->second.effect_registry||
           now<found->second.health_updated||now<found->second.energy_updated||
           !advance_resource_event_locked(found->second,now)||found->second.precise_health<=0)return {};
        return skills::effects::live_selfless_spirit_entry(*found->second.effect_registry,source,now);
    }
