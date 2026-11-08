// Conversion-owned full transfer. The observed prefix is healing, retirement,
// source gain, recipient loss. Never use a damaging Hit for life stealing.
private:
    bool preview_npc_reversal_life_steal_locked(combat::npcs::LiveActor& proposal,skills::effects::Scope target,
            unsigned amount,std::uint64_t now)const{
        const auto life=proposal.lifetime();const auto registry=proposal.owned_effect_snapshot(life);
        const auto active=registry?registry->newest_skill(target,307,now):std::nullopt;
        const auto reduction=proposal.life_steal_reduction(life,now);
        if(proposal.hex_scope()!=target||!active||!reduction||!valid_reversal_owner_locked(target,*active))return false;
        const auto reduced=std::max(0.,double(amount)-*reduction);
        return reduced==0||proposal.apply_reversal_life_steal(life,reduced,now).has_value();
    }
    std::optional<double> preview_player_reversal_life_steal_locked(const Player& p,double reduced,std::uint64_t now)const{
        if(!p.resources||!p.effect_registry||p.precise_health<=0||now<p.health_updated||now<p.energy_updated)return {};
        auto registry=*p.effect_registry;const auto active=registry.newest_skill(p.effect_scope,307,now);
        if(!active||!valid_reversal_owner_locked(p.effect_scope,*active))return {};
        const auto quote=skills::effects::prepare_reversal_fortune(registry,p.effect_scope,skills::effects::ReversalInput::life_steal,reduced,now);
        if(!quote||!skills::effects::consume_reversal_fortune(registry,*quote,now))return {};
        const auto multiplier=skills::effects::healing_hex_multiplier(registry.active(p.effect_scope,now),now);
        if(!std::isfinite(multiplier)||multiplier<=0||multiplier>1)return {};
        auto wound=p.deep_wound;auto hp=p.precise_health;
        wound.event(hp,quote->split.healing,true,multiplier);
        if(hp>0)wound.event(hp,-std::min(quote->split.remaining,hp+1));
        return hp;
    }
    bool commit_reversal_life_steal_transfer_locked(skills::effects::Scope source,skills::effects::Scope target,
            unsigned amount,std::uint64_t now){
        if(!amount||amount>1000000||source==target)return false;
        auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||!caster->second.resources||
           caster->second.precise_health<=0||now<caster->second.health_updated||now<caster->second.energy_updated)return false;
        auto player=players_.find(target.actor);auto live=npc_combat_.find(target.actor);auto body=npcs_.find(target.actor);
        const bool npc=player==players_.end();
        skills::effects::Entry removed{};skills::effects::ReversalFortuneQuote quote{};
        std::optional<skills::effects::Scope> healing_caster;
        combat::Health before{},after_healing{};
        if(!npc){
            auto& p=player->second;
            if(p.effect_scope!=target||!p.resources||!p.effect_registry)return false;
            const double reduced=std::max(0.,double(amount)-shielding_reduction_locked(p,now));
            if(!preview_player_reversal_life_steal_locked(p,reduced,now))return false;
            const auto prepared=skills::effects::prepare_reversal_fortune(*p.effect_registry,target,skills::effects::ReversalInput::life_steal,reduced,now);
            if(!prepared)return false;quote=*prepared;
            const auto ended=skills::effects::consume_reversal_fortune(*p.effect_registry,quote,now);if(!ended)return false;
            removed=*ended;healing_caster=reversal_caster_scope_locked(target,removed);
            before={p.precise_health,p.resources->max_health};health_event_locked(p,quote.split.healing,now,true);
            after_healing={p.precise_health,p.resources->max_health};
        }else{
            if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=target)return false;
            const auto life=live->second.lifetime();const auto registry=live->second.owned_effect_snapshot(life);
            const auto active=registry?registry->newest_skill(target,307,now):std::nullopt;
            const auto reduction=live->second.life_steal_reduction(life,now);
            if(!active||!reduction||!valid_reversal_owner_locked(target,*active))return false;
            healing_caster=reversal_caster_scope_locked(target,*active);
            const auto receipt=live->second.begin_reversal_life_steal(life,std::max(0.,double(amount)-*reduction),now);
            if(!receipt)return false;
            removed=receipt->removed;quote=receipt->quote;before=receipt->before_healing;after_healing=receipt->after_healing;
        }
        const auto healer=healing_caster?reversal_healing_source_locked(*healing_caster).actor:std::nullopt;
        std::vector<Packet> prefix;const auto gained=after_healing.current-before.current;
        if(gained>0&&healer)prefix.push_back(skills::particles::health_change_2022(target.actor,*healer,gained,after_healing.maximum));
        prefix.push_back(combat::detail::fraction(target.actor,after_healing));
        for(auto&[id,peer]:players_)peer.outbox->enqueue(prefix);
        if(!npc)remove_health_effect_locked(player->second,removed.instance,now);
        else{
            npc_simple_enchantment_credit_.erase({target.actor,removed.instance});
            for(auto&[id,peer]:players_)peer.outbox->enqueue({health_effect_remove(target.actor,removed.instance),
                emotes::effects_2022(target.actor,live->second.actor(live->second.lifetime())->effects)});
        }
        if(after_healing.current<=0){
            if(!npc)kill_locked(player->second,now);
            else record_npc_death_locked(caster->second,body->second,live->second,now);
            return true;
        }
        // Conversion precedes the documented remaining-Health-plus-one cap.
        // Combined overkill ordering is an explicit retail evidence gap.
        const auto transfer=skills::effects::steal_life_reduced(quote.split.remaining,
            {caster->second.precise_health,caster->second.resources->max_health},after_healing);
        if(!transfer)throw std::logic_error("admitted conversion transfer lost Health authority");
        const auto source_before=caster->second.precise_health;
        if(transfer->gained>0)health_event_locked(caster->second,transfer->gained,now);
        const auto source_gain=std::max(0.,caster->second.precise_health-source_before);
        if(source_gain>0)for(auto&[id,peer]:players_)peer.outbox->enqueue({skills::particles::health_change_2022(
            source.actor,source.actor,source_gain,caster->second.resources->max_health)});
        publish_health_locked(caster->second);
        // Source gain/capacity reactions must not restore a stale recipient.
        combat::Health after{};bool killed=false;
        if(!npc){
            player=players_.find(target.actor);
            if(player==players_.end()||player->second.effect_scope!=target||!player->second.resources||player->second.precise_health<=0)return true;
            if(transfer->stolen>0)health_event_locked(player->second,-transfer->stolen,now);
            after={player->second.precise_health,player->second.resources->max_health};killed=after.current<=0;
            if(transfer->stolen>0&&player->second.passive_recovery)player->second.passive_recovery->record(recovery::Event::health_loss,now);
        }else{
            live=npc_combat_.find(target.actor);body=npcs_.find(target.actor);
            if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=target)return true;
            const auto* current=live->second.actor(live->second.lifetime());if(!current||!current->health||current->health->current<=0)return true;
            const auto settled=live->second.apply_life_steal_loss(live->second.lifetime(),transfer->stolen,now);
            if(!settled)throw std::logic_error("admitted conversion recipient loss failed");after=*settled;killed=after.current<=0;
        }
        std::vector<Packet> loss;
        if(transfer->stolen>0)loss.push_back(skills::particles::health_change_2022(target.actor,source.actor,-transfer->stolen,after.maximum));
        loss.push_back(combat::detail::fraction(target.actor,after));
        if(npc&&killed)loss.push_back(emotes::effects_2022(target.actor,live->second.actor(live->second.lifetime())->effects));
        for(auto&[id,peer]:players_)peer.outbox->enqueue(loss);
        if(killed){if(npc)record_npc_death_locked(caster->second,body->second,live->second,now);else kill_locked(player->second,now);}
        return true;
    }
