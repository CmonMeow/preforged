// Included inside Instance. Runtime remains unbound until all cast producers
// can place this completion before their sacrifice/effect publication phase.
private:
    std::map<unsigned,SpellActivationCursor> ether_renewal_completions_;
    void invalidate_player_ether_renewal_on_death_locked(const Player& player,std::uint64_t now){
        const auto admitted=spell_activation_cursors_.find(player.id);
        if(admitted==spell_activation_cursors_.end()||admitted->second.actor!=player.effect_scope)return;
        auto barrier=admitted->second;barrier.time=std::max(barrier.time,now);barrier.skill=0;
        const auto completed=ether_renewal_completions_.find(player.id);
        if(completed!=ether_renewal_completions_.end()&&completed->second.actor==player.effect_scope)
            barrier.event=std::max(barrier.event,completed->second.event);
        // Preserve the high-water event across resurrection. Clearing the
        // admission cursor would allow the old event to be admitted again.
        ether_renewal_completions_.insert_or_assign(player.id,barrier);
    }
    enum class NpcSpellTerminal {unavailable,reaction_stopped,completed};
    NpcSpellTerminal complete_npc_spell_terminal_locked(
            const combat::npcs::control::Token& token,std::uint64_t now,bool reward=true){
        auto live=npc_combat_.find(token.actor.actor);
        if(!token.skill||now<token.completes||live==npc_combat_.end()||live->second.hex_scope()!=token.actor)
            return NpcSpellTerminal::unavailable;
        const auto life=live->second.lifetime();auto preview=live->second;
        if(!(reward?complete_npc_ether_terminal_locked(preview,token,now,false):preview.complete_action(life,token,now)))return NpcSpellTerminal::unavailable;
        const auto cursor=spell_activation_cursors_.find(token.actor.actor);
        const auto event=cursor==spell_activation_cursors_.end()?1:
            cursor->second.event==UINT64_MAX?0:cursor->second.event+1;
        if(!event)return NpcSpellTerminal::unavailable;
        const bool survived=before_spell_effect_locked(token.actor,token.skill->skill,event,now);
        live=npc_combat_.find(token.actor.actor);
        if(!survived||live==npc_combat_.end()||!live->second.matches(life)||
           !live->second.actor(life)->health||live->second.actor(life)->health->current<=0){
            if(live!=npc_combat_.end()&&live->second.matches(life))live->second.complete_action(life,token,now);
            return NpcSpellTerminal::reaction_stopped;
        }
        if(!(reward?complete_npc_ether_terminal_locked(live->second,token,now):live->second.complete_action(life,token,now))){
            live->second.complete_action(life,token,now);
            return NpcSpellTerminal::reaction_stopped;
        }
        return NpcSpellTerminal::completed;
    }
    bool complete_npc_ether_terminal_locked(combat::npcs::LiveActor& actor,
            const combat::npcs::control::Token& token,std::uint64_t now,bool publish=true){
        const auto life=actor.lifetime();
        const auto* row=token.skill?skills::find_skill(token.skill->skill):nullptr;
        const auto registry=actor.owned_effect_snapshot(life);
        if(token.kind!=combat::npcs::control::Action::spell||!row||!skills::effects::is_spell(*row)||
           !registry||!registry->newest_skill(actor.hex_scope(),181,now,2860))
            return actor.complete_action(life,token,now);
        const auto units=actor.precise_energy_units(life);
        const auto receipt=actor.complete_ether_renewal_action(life,token,now);
        if(!receipt)return false;
        if(!publish)return true; // Caller owns a speculative actor copy.
        const auto scope=actor.hex_scope();const auto after_units=actor.precise_energy_units(life);
        std::vector<Packet> packets;
        if(receipt->after.current>receipt->before.current){Packet delta{162,0};
            u32(delta,55);u32(delta,scope.actor);u32(delta,scope.actor);
            f32(delta,float((receipt->after.current-receipt->before.current)/receipt->after.maximum));
            packets.push_back(std::move(delta));}
        const auto absolute=combat::health_packets_2022(*actor.actor(life));
        packets.insert(packets.end(),absolute.begin(),absolute.end());
        for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
        if(units&&after_units)observe_npc_mind_wrack_energy_locked(scope,
            {receipt->energy_before.current,receipt->energy_before.maximum,0,unsigned(*units%300000)},now,
            skills::effects::MindWrackEnergy{receipt->energy_after.current,receipt->energy_after.maximum,0,unsigned(*after_units%300000)});
        return true;
    }
    bool install_npc_ether_renewal_locked(combat::npcs::LiveActor& actor,
            const skills::effects::EtherRenewalPlan& plan,unsigned effect,std::uint64_t now){
        const auto scope=actor.hex_scope();const auto life=actor.lifetime();
        const auto before=actor.owned_effect_snapshot(life);
        auto preview=actor;if(!before||!preview.apply_ether_renewal(life,plan,effect,now))return false;
        const auto after=preview.owned_effect_snapshot(life);const auto entry=after?after->lookup(scope,effect):std::nullopt;
        if(!entry)return false;
        std::vector<Packet> packets;
        for(const auto& old:before->active(scope,now))if(old.skill==181||old.skill==2860){
            packets.push_back(health_effect_remove(scope.actor,old.instance));
            npc_simple_enchantment_credit_.erase({scope.actor,old.instance});
        }
        actor=std::move(preview);
        npc_simple_enchantment_credit_[{scope.actor,effect}]=scope;
        packets.push_back(health_effect_add(*entry,scope.actor,now));
        packets.push_back(emotes::effects_2022(scope.actor,actor.actor(life)->effects));
        for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
        commit_applications_locked(scope,now);return true;
    }
public:
    // Trusted installation for reviewed NPC loadouts/tests; not a paid cast.
    bool apply_npc_ether_renewal(skills::effects::Scope scope,
            const skills::effects::EtherRenewalPlan& plan,std::uint64_t now){
        if(skills::effects::ether_renewal(plan.skill,plan.rank)!=plan)return false;
        std::lock_guard lock(mutex_);auto found=npc_combat_.find(scope.actor);
        if(found==npc_combat_.end()||found->second.hex_scope()!=scope||!npcs_.contains(scope.actor)||
           now<found->second.health_clock(found->second.lifetime()).value_or(now))return false;
        advance_npc_conditions_locked(scope.actor,found->second,now);
        found=npc_combat_.find(scope.actor);if(found==npc_combat_.end()||found->second.hex_scope()!=scope)return false;
        if(well_blocks_npc_enchantment_locked(scope.actor,now))return true;
        const auto next=std::max<std::uint64_t>(next_npc_hex_,next_mending_effect_);
        if(!next||next>=UINT32_MAX)return false;
        // Reserve before publication: application observers may reenter and
        // allocate another effect while this exact instance is being installed.
        next_npc_hex_=static_cast<unsigned>(next)+1;
        return install_npc_ether_renewal_locked(found->second,plan,static_cast<unsigned>(next),now);
    }
private:
    // Trusted staged installation only; this does not enable a paid cast.
    // World owns the zero-pip timer and every end, including death. Runtime
    // has no181/2860producer/cache while their required consumers remain gated.
    bool install_player_ether_renewal_locked(Player& player,
            const skills::effects::EtherRenewalPlan& plan,unsigned effect,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted){
        const auto scope=player.effect_scope;
        const auto duration=adjusted?adjusted->for_enchantment(plan.skill,plan.duration,scope,now):std::optional(plan.duration);
        if(skills::effects::ether_renewal(plan.skill,plan.rank)!=plan||!effect||effect>=0x80000000u||
           !duration||now>UINT64_MAX-*duration||!player.effect_registry||!player.health_regeneration||
           now<player.health_updated||now<player.energy_updated||
           !advance_resource_event_locked(player,now)||player.precise_health<=0)return false;
        if(player.effect_registry->lookup(scope,effect)||player.health_effects.contains(effect))return false;
        if(well_blocks_player_enchantment_locked(player,now))return true;
        auto registry=*player.effect_registry;auto timers=*player.health_regeneration;
        std::vector<skills::effects::Entry> removed;
        for(const auto& old:registry.active(scope,now))if(old.skill==181||old.skill==2860){
            removed.push_back(old);registry.remove(scope,old.instance);timers.remove(scope,old.instance);
        }
        const skills::effects::Entry entry{effect,scope.actor,plan.skill,plan.rank,
            skills::effects::Category::enchantment,now,now+*duration};
        if(!registry.apply(scope,entry,skills::effects::Replace::same_skill).accepted||
           !timers.apply(scope,{effect,scope.actor,plan.skill,0,now,*entry.expires}))return false;
        *player.effect_registry=std::move(registry);*player.health_regeneration=std::move(timers);
        for(const auto& old:removed){
            player.health_effects.erase(old.instance);
            for(auto&[id,peer]:players_)peer.outbox->enqueue({health_effect_remove(scope.actor,old.instance)});
        }
        player.health_effects.emplace(effect,entry);
        commit_applications_locked(scope,now);publish_status_locked(player,now);return true;
    }
    // Caller owns the World lock and has validated the spell's own target and
    // effect prerequisites. An absent enchantment still consumes a terminal
    // when used by an atomic spell producer, preventing duplicate sacrifices.
    bool complete_ether_renewal_locked(skills::effects::Scope scope,unsigned skill,
            std::uint64_t event,std::uint64_t now,bool allow_absent=false,bool commit=true){
        const auto* row=skills::find_skill(skill);
        if(!row||!skills::effects::is_spell(*row))return false;
        const auto found=players_.find(scope.actor);
        const auto activation=spell_activation_cursors_.find(scope.actor);
        // Reaction admission and effect commit may span World ticks. Keep
        // identity exact, but require monotonic time rather than equality.
        if(!event||found==players_.end()||found->second.effect_scope!=scope||
           activation==spell_activation_cursors_.end()||activation->second.actor!=scope||
           activation->second.event!=event||activation->second.skill!=skill||activation->second.time>now)return false;
        const auto previous=ether_renewal_completions_.find(scope.actor);
        if(previous!=ether_renewal_completions_.end()&&previous->second.actor==scope&&
           event<=previous->second.event)return false;
        auto& player=found->second;
        if(!player.resources||!player.effect_registry||now<player.health_updated||
           now<player.energy_updated||!advance_resource_event_locked(player,now)||player.precise_health<=0)return false;
        const auto reward=skills::effects::ether_renewal_reward(*player.effect_registry,scope,skill,now);
        if(!reward){
            if(!allow_absent||player.effect_registry->newest_skill(scope,181,now,2860))return false;
            if(commit)ether_renewal_completions_.insert_or_assign(scope.actor,SpellActivationCursor{scope,event,now,skill});
            return true;
        }
        // Check the fallible Energy timeline before either resource changes.
        if(player.overcast_curve&&!overcast::prepare_gain(*player.overcast_curve,scope,*player.resources,
                player.energy_regeneration,player.energy_debt,reward->energy,now,player.energy_pips,player.energy_fractional_tail))return false;
        if(!commit)return true; // Real due-resource settlement, but no terminal/reward consumption.
        ether_renewal_completions_.insert_or_assign(scope.actor,SpellActivationCursor{scope,event,now,skill});
        const auto before=player.precise_health;
        health_event_locked(player,reward->healing,now,true);
        const auto healed=player.precise_health-before;
        if(healed>0){Packet delta{162,0};u32(delta,55);u32(delta,scope.actor);u32(delta,scope.actor);
            f32(delta,float(healed/player.resources->max_health));
            for(auto&[id,peer]:players_)peer.outbox->enqueue({delta});}
        if(player.precise_health<=0){kill_locked(player,now);return true;}
        gain_energy_locked(player,reward->energy,now);
        player.outbox->enqueue(owner_vitals(player));publish_health_locked(player);
        return true;
    }
public:
    // Trusted host completion, not a client packet. A matching admitted spell
    // reaction event is necessary; the caller must additionally establish that
    // the spell completed and choose the reviewed pre-sacrifice boundary.
    bool ether_renewal_completed(skills::effects::Scope scope,unsigned skill,
            std::uint64_t event,std::uint64_t now){
        std::lock_guard lock(mutex_);return complete_ether_renewal_locked(scope,skill,event,now);
    }
