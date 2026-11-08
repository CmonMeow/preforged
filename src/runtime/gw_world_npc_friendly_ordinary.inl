// Included inside Instance. Friendly sources share the existing ordinary ledger;
// explicit profiles/targets only, no implicit quest content activation.
public:
    struct FriendlyOrdinaryQuest {skills::effects::Scope owner;unsigned quest=0,cycle=0;};
    bool bind_npc_opposed_allies(FriendlyOrdinaryQuest quest,std::span<const skills::effects::Scope> allies){
        if(!quest.quest||!quest.cycle||allies.empty()||allies.size()>64)return false;
        std::lock_guard lock(mutex_);const auto owner=players_.find(quest.owner.actor);
        if(owner==players_.end()||owner->second.effect_scope!=quest.owner)return false;
        const auto objective=owner->second.quest_kills.find(quest.quest);
        if(objective==owner->second.quest_kills.end()||objective->second.cycle!=quest.cycle)return false;
        const combat::ai::opposition::QuestDomain domain{quest.owner,quest.quest,quest.cycle};
        for(const auto scope:allies){const auto actor=npc_combat_.find(scope.actor);
            if(actor==npc_combat_.end()||actor->second.hex_scope()!=scope||
               combat::ai::opposition::living_side(actor->second,actor->second.lifetime())!=combat::ai::opposition::Side::allied)return false;
            if(const auto prior=npc_opposed_allies_.find(scope);prior!=npc_opposed_allies_.end()&&prior->second!=domain){
                const auto player=players_.find(prior->second.owner.actor);
                if(player!=players_.end()&&player->second.effect_scope==prior->second.owner){const auto active=player->second.quest_kills.find(prior->second.quest);
                    if(active!=player->second.quest_kills.end()&&active->second.cycle==prior->second.cycle)return false;}}
        }
        for(const auto scope:allies)npc_opposed_allies_[scope]=domain;return true;
    }
private:
    struct FriendlyOrdinaryBinding {skills::effects::Scope source;std::optional<FriendlyOrdinaryQuest> quest;std::optional<skills::effects::Scope> target;combat::ai::opposition::Side side=combat::ai::opposition::Side::allied;};
    std::map<unsigned,FriendlyOrdinaryBinding> npc_friendly_ordinary_;
    std::map<skills::effects::Scope,combat::ai::opposition::QuestDomain> npc_opposed_allies_;
    bool valid_friendly_ordinary_locked(skills::effects::Scope source,skills::effects::Scope target,bool require_living_source=true)const{
        const auto binding=npc_friendly_ordinary_.find(source.actor);
        const auto attacker=npc_combat_.find(source.actor),victim=npc_combat_.find(target.actor);
        if(binding==npc_friendly_ordinary_.end()||binding->second.source!=source||attacker==npc_combat_.end()||
           attacker->second.hex_scope()!=source||victim==npc_combat_.end()||victim->second.hex_scope()!=target)return false;
        if(pet_owners_.contains(source))return valid_pet_ordinary_locked(source,target,require_living_source);
        if(pet_owners_.contains(target))return valid_pet_enemy_ordinary_locked(source,target,require_living_source);
        const auto recipient_side=combat::ai::opposition::living_side(victim->second,victim->second.lifetime());
        if(!recipient_side||*recipient_side==binding->second.side||
           (require_living_source&&combat::ai::opposition::living_side(attacker->second,attacker->second.lifetime())!=binding->second.side))return false;
        if(!binding->second.quest)return !quest_npc_owners_.contains(target.actor)&&!npc_opposed_allies_.contains(target);
        const auto& quest=*binding->second.quest;const auto owner=players_.find(quest.owner.actor);
        if(owner==players_.end()||owner->second.effect_scope!=quest.owner)return false;
        const auto objective=owner->second.quest_kills.find(quest.quest);
        if(objective==owner->second.quest_kills.end()||objective->second.cycle!=quest.cycle)return false;
        const auto source_assignment=quest_npc_owners_.find(source.actor);
        if(source_assignment!=quest_npc_owners_.end()&&source_assignment->second!=std::pair{quest.owner,quest.quest})return false;
        if(binding->second.side==combat::ai::opposition::Side::hostile&&
           (source_assignment==quest_npc_owners_.end()||(require_living_source&&!objective->second.remaining.contains(source))))return false;
        if(*recipient_side==combat::ai::opposition::Side::hostile){const auto recipient=quest_npc_owners_.find(target.actor);
            return recipient!=quest_npc_owners_.end()&&recipient->second==std::pair{quest.owner,quest.quest}&&objective->second.remaining.contains(target);}
        const auto ally=npc_opposed_allies_.find(target);
        return ally!=npc_opposed_allies_.end()&&ally->second==combat::ai::opposition::QuestDomain{quest.owner,quest.quest,quest.cycle};
    }
    std::optional<AttackOutcome> npc_ordinary_npc_contact_locked(NpcOrdinaryController& ai,
            const combat::ai::ordinary::Impact& impact,std::uint64_t event,std::uint64_t now){
        const auto target=impact.release.attack.target;const auto& profile=ai.ledger.profile();
        auto source=npc_combat_.find(ai.source.actor),victim=npc_combat_.find(target.actor);
        const auto body=npcs_.find(ai.source.actor),recipient=npcs_.find(target.actor);
        if(source==npc_combat_.end()||source->second.hex_scope()!=ai.source||victim==npc_combat_.end()||
           victim->second.hex_scope()!=target||body==npcs_.end()||recipient==npcs_.end())return AttackOutcome::target_retired;
        const auto a=source->second.lifetime(),b=victim->second.lifetime();
        // The existing release is immutable; target generation never changes.
        const auto facts=combat::ai::ordinary::opposed_npc_recipient(source->second,a,victim->second,b,recipient->second.x,recipient->second.y,
            recipient->second.plane,profile.damage_type,now,profile.delivery==combat::ai::ordinary::Delivery::ranged,true);
        if(!facts||!facts->pose.observed||facts->pose.clock>now||!facts->armor)return {};
        combat::weapon::Strike strike;strike.level=body->second.definition.level;strike.minimum=profile.minimum;strike.maximum=profile.maximum;
        strike.mastery=profile.mastery;strike.caster_weapon=profile.caster_weapon;strike.base_roll=.5;strike.miss_roll=.5;
        strike.location_armor=*facts->armor;strike.weakness=source->second.has_condition(a,combat::conditions::Kind::weakness,now);
        strike.blind=source->second.has_condition(a,combat::conditions::Kind::blind,now);sample_attack_miss_locked(strike,ai.source,now);
        const auto resolved=combat::weapon::resolve(strike);if(!resolved)return {};
        if(resolved->missed)return AttackOutcome::miss;
        const auto block=npc_block_contact_locked(victim->second,now,false,0,profile.delivery==combat::ai::ordinary::Delivery::melee);if(!block)return {};
        if(*block!=AttackOutcome::hit)return *block;
        double base_damage=resolved->damage;
        if(pet_owners_.contains(ai.source)){
            // Pet damage is not weapon damage: its critical does not force the
            // maximum roll. Shared miss/block/reactions/protection still apply.
            if(!victim->second.protect_stoneflesh_critical(b,strike,now))return {};
            const auto pet_damage=pet_ordinary_damage_locked(ai.source,recipient->second.definition.level,
                *facts->armor,strike.weakness,strike.critical_damage_suppressed||strike.critical_immune,now);
            if(!pet_damage)return {};base_damage=*pet_damage;
        }
        const auto damage=source->second.empathy_attack_damage(a,base_damage,now);if(!damage)return {};
        std::optional<combat::npcs::control::Token> dazed;
        if(!preflight_npc_dazed_hit_locked(victim->second,now,dazed))return {};
        const auto qualified=qualify_attack_hit_locked(ai.source,target,event,now);if(!qualified)return {};
        // Qualification owns shared Judgment/Tainted/Wailing/Insidious reactions.
        // Never assign an older whole-actor snapshot after these callbacks.
        victim=npc_combat_.find(target.actor);
        if(victim==npc_combat_.end()||!victim->second.matches(b))return AttackOutcome::target_retired;
        const auto* fresh=victim->second.actor(b);
        if(!fresh||!fresh->health||fresh->health->current<=0)return AttackOutcome::target_retired;
        // This delivery was already released by the source's real action.
        // Like the existing NPC->player path, a qualification reaction that
        // kills the source does not revoke that immutable released delivery.
        source=npc_combat_.find(ai.source.actor);
        if(source==npc_combat_.end()||!source->second.matches(a))return AttackOutcome::cancelled;
        // Preview the real post-protection weapon transaction, including307.
        // Seed responds only to remaining loss, never converted healing.
        const auto plan_hit=[&]()->std::optional<NpcWeaponHit>{
            if(victim==npc_combat_.end()||!victim->second.matches(b)||source==npc_combat_.end()||!source->second.matches(a))return {};
            auto proposal=victim->second;
            const auto raw=proposal.apply_opposed_npc_weapon_damage(b,source->second,a,*damage,profile.damage_type,now,true);
            if(!raw)return {};
            return prepare_npc_weapon_hit_locked(victim->second,std::move(proposal),*raw,now);
        };
        auto planned=plan_hit();if(!planned)return {};
        const auto preview_loss=planned->actual?planned->actual->applied:0.;
        auto seed=prepare_npc_healing_seed_direct_locked(target,preview_loss,now);if(!seed)return {};
        // Only an active positive-loss Seed preflight settles nearby actors.
        // Its callbacks require a fresh plan. The ordinary no-Seed path owns
        // the same World lock and avoids a second whole-actor copy.
        if(seed->effect){
            victim=npc_combat_.find(target.actor);source=npc_combat_.find(ai.source.actor);
            planned=plan_hit();if(!planned)return {};
            if(planned->actual&&planned->actual->applied>0){
                const auto registry=victim->second.owned_effect_snapshot(b);
                const auto active=registry?registry->newest_skill(target,274,now):std::nullopt;
                if(!active||active->instance!=seed->effect)return {};
            }
        }
        const auto loss=planned->actual?planned->actual->applied:0.;
        RetributionPublication retribution_publication(*this,retribution_contacts_.contains(event));
        victim->second=std::move(planned->actor);
        std::vector<Packet> packets;
        if(planned->reversal){
            npc_simple_enchantment_credit_.erase({target.actor,planned->reversal->removed.instance});
            packets=combat::npcs::reversal_weapon_packets_2022(target.actor,ai.source.actor,*planned->reversal,false,reversal_healing_source_locked(target,planned->reversal->removed));
            packets.push_back(emotes::effects_2022(target.actor,victim->second.actor(b)->effects));
        }else if(planned->actual)packets=combat::weapon::packets_2022(ai.source.actor,*planned->actual);
        for(const auto& packet:packets)npc_ordinary_packet_locked(packet);
        if(planned->actual)consume_watch_yourself_npc_hit_locked(victim->second,*planned->actual,true,now);
        retribution_publication.reaction();commit_retribution_damage_locked(*qualified,planned->actual);retribution_publication.flush();
        if(loss>0&&seed->effect)publish_healing_seed_direct_locked(*seed,now);
        mark_of_pain_reaction_locked(target,profile.damage_type,loss,now);
        victim=npc_combat_.find(target.actor);
        if(victim!=npc_combat_.end()&&victim->second.matches(b)&&victim->second.actor(b)->health->current>0)
            barbs_reaction_locked(target,profile.damage_type,loss,now);
        victim=npc_combat_.find(target.actor);const auto current=npcs_.find(target.actor);
        if(victim!=npc_combat_.end()&&victim->second.matches(b)&&current!=npcs_.end()){
            // Conversion healing can itself kill under healing-debt effects.
            if(victim->second.actor(b)->health->current<=0)record_npc_death_locked(pet_credit_owner_locked(ai.source),current->second,victim->second,now);
            else commit_npc_dazed_hit_locked(victim->second,dazed,now);
        }
        return AttackOutcome::hit;
    }
public:
    bool configure_npc_opposed_ordinary(skills::effects::Scope source,combat::ai::ordinary::Reviewed profile,
            std::uint64_t now,std::optional<FriendlyOrdinaryQuest> quest={}){
        if(!profile.valid()||(quest&&(!quest->quest||!quest->cycle)))return false;
        std::lock_guard lock(mutex_);
        if(quest){const auto owner=players_.find(quest->owner.actor);
            if(owner==players_.end()||owner->second.effect_scope!=quest->owner)return false;
            const auto objective=owner->second.quest_kills.find(quest->quest);
            if(objective==owner->second.quest_kills.end()||objective->second.cycle!=quest->cycle)return false;
        }
        const auto live=npc_combat_.find(source.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=source||!combat::ai::opposition::living_side(live->second,live->second.lifetime())||
           !live->second.ordinary_action_ready(live->second.lifetime(),now))return false;
        if(quest){const auto assignment=quest_npc_owners_.find(source.actor);
            if(assignment!=quest_npc_owners_.end()&&assignment->second!=std::pair{quest->owner,quest->quest})return false;
            if(combat::ai::opposition::living_side(live->second,live->second.lifetime())==combat::ai::opposition::Side::hostile&&assignment==quest_npc_owners_.end())return false;}
        if(const auto old=npc_ordinary_ai_.find(source.actor);old!=npc_ordinary_ai_.end()){
            const auto binding=npc_friendly_ordinary_.find(source.actor);
            if(old->second.source!=source||binding==npc_friendly_ordinary_.end()||binding->second.source!=source||
               old->second.ledger.pending()||old->second.ledger.in_flight()||!old->second.events.empty())return false;
            if(binding->second.quest){const auto& prior=*binding->second.quest;
                const auto owner=players_.find(prior.owner.actor);
                if(owner!=players_.end()&&owner->second.effect_scope==prior.owner){const auto objective=owner->second.quest_kills.find(prior.quest);
                    if(objective!=owner->second.quest_kills.end()&&objective->second.cycle==prior.cycle)return false;}
            }else return false;
        }
        npc_ordinary_ai_.insert_or_assign(source.actor,NpcOrdinaryController{source,combat::ai::ordinary::Ledger(source,std::move(profile)),{}});
        const auto side=*combat::ai::opposition::living_side(live->second,live->second.lifetime());
        npc_friendly_ordinary_.insert_or_assign(source.actor,FriendlyOrdinaryBinding{source,quest,{},side});
        if(quest&&side==combat::ai::opposition::Side::allied)npc_opposed_allies_[source]={quest->owner,quest->quest,quest->cycle};return true;
    }
    bool configure_npc_friendly_ordinary(skills::effects::Scope source,combat::ai::ordinary::Reviewed profile,
            std::uint64_t now,std::optional<FriendlyOrdinaryQuest> quest={}){
        {std::lock_guard lock(mutex_);const auto live=npc_combat_.find(source.actor);
            if(live==npc_combat_.end()||live->second.hex_scope()!=source||!live->second.healable_ally(live->second.lifetime()))return false;}
        return configure_npc_opposed_ordinary(source,std::move(profile),now,quest);
    }
    std::optional<combat::npcs::control::Token> begin_npc_friendly_ordinary(skills::effects::Scope source,
            skills::effects::Scope target,std::uint64_t now){
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        return begin_npc_friendly_ordinary_locked(source,target,now);
    }
    std::optional<combat::npcs::control::Token> begin_npc_opposed_ordinary(skills::effects::Scope source,
            skills::effects::Scope target,std::uint64_t now){return begin_npc_friendly_ordinary(source,target,now);}
private:
    std::optional<combat::npcs::control::Token> begin_npc_friendly_ordinary_locked(skills::effects::Scope source,
            skills::effects::Scope target,std::uint64_t now){
        if(!valid_friendly_ordinary_locked(source,target))return {};
        auto live=npc_combat_.find(source.actor),victim=npc_combat_.find(target.actor);
        const auto body=npcs_.find(source.actor),recipient=npcs_.find(target.actor);const auto ledger=npc_ordinary_ai_.find(source.actor);
        if(body==npcs_.end()||recipient==npcs_.end()||ledger==npc_ordinary_ai_.end()||ledger->second.source!=source)return {};
        const auto a=live->second.lifetime(),b=victim->second.lifetime();
        advance_npc_conditions_locked(source.actor,live->second,now);advance_npc_conditions_locked(target.actor,victim->second,now);
        live=npc_combat_.find(source.actor);victim=npc_combat_.find(target.actor);
        if(live==npc_combat_.end()||!live->second.matches(a)||victim==npc_combat_.end()||!victim->second.matches(b)||
           !valid_friendly_ordinary_locked(source,target)||!ledger->second.ledger.ready(now)||live->second.knocked_down(a,now))return {};
        const auto& profile=ledger->second.ledger.profile();
        const auto facts=combat::ai::ordinary::opposed_npc_recipient(live->second,a,victim->second,b,recipient->second.x,recipient->second.y,recipient->second.plane,profile.damage_type,now,profile.delivery==combat::ai::ordinary::Delivery::ranged);
        if(!facts||!facts->pose.alive||!facts->pose.observed||!facts->armor||facts->pose.plane!=body->second.plane||
           std::hypot(facts->pose.x-body->second.x,facts->pose.y-body->second.y)>profile.range)return {};
        if(navigation_){const navigation::Point from{body->second.x,body->second.y};
            const auto cell=navigation_->locate_cell(body->second.plane,from);if(!cell)return {};
            const auto trace=layered_->trace({body->second.plane,*cell},from,{facts->pose.x,facts->pose.y});
            if(!trace||trace->plane!=facts->pose.plane)return {};}
        const auto duration=live->second.attack_duration(a,profile.windup_ms,now),period=live->second.attack_duration(a,profile.period_ms,now);
        if(!duration||!period||now>UINT64_MAX-*period)return {};
        const auto token=live->second.begin_action(a,combat::npcs::control::Action::attack,now,now+*duration);if(!token)return {};
        if(quest_npc_motion_.erase(source.actor))for(auto&[id,p]:players_)
            p.outbox->enqueue({position_packet(source.actor,body->second.x,body->second.y,body->second.plane),stop_packet(source.actor)});
        npc_ordinary_packet_locked(combat::weapon::timing_2022(source.actor,float(*period)/1000.f,1));
        npc_ordinary_packet_locked(combat::weapon::start_2022(source.actor,target.actor));
        const bool killed=publish_npc_attack_hexes_locked(source.actor,live->second);
        if(killed||!npc_empathy_reaction_locked(source,live->second,body->second,now)||live->second.control_epoch(a)!=token->epoch){
            live->second.cancel_action(a,now);npc_ordinary_packet_locked(combat::weapon::stop_2022(source.actor));return {};}
        if(!ledger->second.ledger.accept(target,*token,*period))throw std::logic_error("friendly ordinary admission changed");
        npc_friendly_ordinary_.at(source.actor).target=target;return token;
    }
    void drive_npc_friendly_ordinary_locked(std::uint64_t now){
        std::vector<skills::effects::Scope> sources;
        for(const auto&[id,binding]:npc_friendly_ordinary_)sources.push_back(binding.source);
        for(const auto scope:sources){
            auto live=npc_combat_.find(scope.actor);const auto body=npcs_.find(scope.actor);
            if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=scope){
                retire_npc_ordinary_locked(scope,now);npc_friendly_ordinary_.erase(scope.actor);continue;}
            if(drive_npc_ordinary_locked(scope,body->second,live->second,now))continue;
            const auto binding=npc_friendly_ordinary_.find(scope.actor);if(binding==npc_friendly_ordinary_.end()||!binding->second.target)continue;
            if(!valid_friendly_ordinary_locked(scope,*binding->second.target)){binding->second.target.reset();continue;}
            begin_npc_friendly_ordinary_locked(scope,*binding->second.target,now);
        }
    }
public:
