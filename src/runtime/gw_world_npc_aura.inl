// Included inside Instance. Owned self-enchantment and completed-spell healing.
private:
    struct NpcSelfEnchantmentCast {combat::npcs::control::Token token;skills::effects::AuraRestorationPlan plan;unsigned effect;
        std::optional<skills::effects::EtherRenewalPlan> renewal;};
    struct NpcAuraCompletion {combat::npcs::control::Token token;unsigned effect,rank;std::uint64_t at;};
    std::map<unsigned,NpcSelfEnchantmentCast> npc_self_enchantment_casts_;
    std::set<unsigned> npc_self_enchantment_in_progress_;
    std::map<std::pair<unsigned,unsigned>,skills::effects::Scope> npc_aura_credit_;
    std::map<skills::effects::Scope,std::uint64_t> npc_aura_completed_;
    void publish_npc_aura_ends_locked(combat::npcs::LiveActor& live){
        const auto scope=live.hex_scope();
        for(auto it=npc_aura_credit_.lower_bound({scope.actor,0});it!=npc_aura_credit_.end()&&it->first.first==scope.actor;){
            if(it->second!=scope){it=npc_aura_credit_.erase(it);continue;}
            if(live.hex_effect(live.lifetime(),it->first.second)){++it;continue;}
            for(auto&[id,p]:players_)p.outbox->enqueue({health_effect_remove(scope.actor,it->first.second)});
            it=npc_aura_credit_.erase(it);
        }
    }
    std::optional<NpcAuraCompletion> prepare_npc_aura_completion_locked(const combat::npcs::control::Token& token,std::uint64_t now){
        const auto live=npc_combat_.find(token.actor.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=token.actor||!token.skill||
           token.kind!=combat::npcs::control::Action::spell||now<token.completes||
           !live->second.owns_spell_action(live->second.lifetime(),token))return {};
        const auto aura=live->second.aura_restoration_entry(live->second.lifetime(),now);
        if(!aura)return {};
        return NpcAuraCompletion{token,aura->instance,aura->rank,now};
    }
    bool finish_npc_aura_completion_locked(const std::optional<NpcAuraCompletion>& quote,std::uint64_t now){
        if(!quote)return true;
        const auto& token=quote->token;const auto scope=token.actor;
        if(now!=quote->at||!token.skill||!token.sequence)return false;
        const auto previous=npc_aura_completed_.find(scope);
        if(previous!=npc_aura_completed_.end()&&token.sequence<=previous->second)return false;
        auto live=npc_combat_.find(scope.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=scope)return false;
        const auto life=live->second.lifetime();
        if(now<live->second.health_clock(life).value_or(now)||!settle_mind_wrack_caster_locked(scope,now))return false;
        advance_npc_conditions_locked(scope.actor,live->second,now);
        live=npc_combat_.find(scope.actor);
        if(live==npc_combat_.end()||!live->second.matches(life)||live->second.control_epoch(life)!=token.epoch||
           live->second.owns_spell_action(life,token)||live->second.healing_allowed(life)!=std::optional(true))return false;
        const auto aura=live->second.aura_restoration_entry(life,now);
        if(!aura||(aura->instance!=quote->effect&&!(token.skill->skill==180&&aura->applied==now)))return false;
        const auto* spell=skills::find_skill(token.skill->skill);
        const auto amount=spell?skills::effects::aura_restoration(*spell,quote->rank):std::nullopt;
        const auto* state=live->second.actor(life);
        if(!amount||!state||!state->health||state->health->current<=0)return false;
        const auto before=*state->health;const auto energy=live->second.energy(life);const auto units=live->second.precise_energy_units(life);
        auto proposed=live->second;
        if(!proposed.heal(life,amount->healing,now)||(amount->energy&&!proposed.gain_energy(life,amount->energy)))return false;
        const auto after=*proposed.actor(life)->health;
        const auto after_energy=proposed.energy(life);const auto after_units=proposed.precise_energy_units(life);
        live->second=std::move(proposed);npc_aura_completed_[scope]=token.sequence;
        std::vector<Packet> packets;
        if(after.current!=before.current){Packet delta{162,0};u32(delta,55);u32(delta,scope.actor);u32(delta,scope.actor);
            f32(delta,float((after.current-before.current)/after.maximum));packets.push_back(std::move(delta));}
        packets.push_back(combat::detail::fraction(scope.actor,after));
        for(auto&[id,p]:players_)p.outbox->enqueue(packets);
        // No actor assignment follows this observer: owned reactions remain
        // authoritative even if they end effects or kill the source.
        if(amount->energy&&energy&&units&&after_energy&&after_units)
            observe_npc_mind_wrack_energy_locked(scope,{energy->current,energy->maximum,0,unsigned(*units%300000)},now,
                skills::effects::MindWrackEnergy{after_energy->current,after_energy->maximum,0,unsigned(*after_units%300000)});
        return true;
    }
    std::uint64_t npc_self_enchantment_deadline_locked(const combat::npcs::control::Token& token)const{
        auto at=token.completes;const auto live=npc_combat_.find(token.actor.actor);
        if(live!=npc_combat_.end()&&live->second.hex_scope()==token.actor){const auto life=live->second.lifetime();
            at=std::max({at,live->second.health_clock(life).value_or(at),live->second.energy_clock(life).value_or(at)});}
        return at;
    }
    std::optional<std::uint64_t> next_npc_self_enchantment_deadline_locked()const{
        std::optional<std::uint64_t> next;
        for(const auto&[id,cast]:npc_self_enchantment_casts_)if(!npc_self_enchantment_in_progress_.contains(id)){
            const auto at=npc_self_enchantment_deadline_locked(cast.token);if(!next||at<*next)next=at;}
        return next;
    }
    std::optional<combat::npcs::control::Token> begin_npc_self_enchantment_locked(skills::effects::Scope source,
            const skills::effects::AuraRestorationPlan& plan,std::uint64_t now,
            std::optional<skills::effects::EtherRenewalPlan> renewal={}){
        const auto canonical=skills::effects::aura_restoration_plan(plan.rank);
        const auto duration=renewal?renewal->duration:plan.duration;
        if((renewal?skills::effects::ether_renewal(renewal->skill,renewal->rank)!=renewal:
                !canonical||*canonical!=plan)||now>UINT64_MAX-duration-60000)return {};
        const auto effect=renewal?std::max<std::uint64_t>(next_npc_hex_,next_mending_effect_):next_npc_hex_;
        if(!effect||effect>=UINT32_MAX)return {};
        auto live=npc_combat_.find(source.actor);
        if(live==npc_combat_.end()||!npcs_.contains(source.actor)||live->second.hex_scope()!=source)return {};
        const auto life=live->second.lifetime();
        if(const auto old=npc_self_enchantment_casts_.find(source.actor);old!=npc_self_enchantment_casts_.end()){
            if(live->second.owns_spell_action(life,old->second.token))return {};
            npc_self_enchantment_casts_.erase(old);
        }
        if(!settle_mind_wrack_caster_locked(source,now))return {};
        advance_npc_conditions_locked(source.actor,live->second,now);
        live=npc_combat_.find(source.actor);if(live==npc_combat_.end()||!live->second.matches(life))return {};
        // A paid Renewal loadout must own both resource authorities before it
        // can install an effect that observes subsequent spell completions.
        if(renewal&&(!live->second.healing_allowed(life).has_value()||
                !live->second.precise_energy_units(life)))return {};
        auto preview=live->second;if(!(renewal?preview.apply_ether_renewal(life,*renewal,unsigned(effect),now):
                preview.apply_aura_restoration(life,plan,unsigned(effect),now)))return {};
        const combat::ai::SpellTarget self{source,0,true,false,true,true};
        const combat::ai::Spellbook::Availability available=[&](skills::Slot slot,std::uint64_t at){return skill_disabled_slot_locked(source,slot,at);};
        const auto before=live->second.energy(life);const auto units=live->second.precise_energy_units(life);
        const auto spell=renewal?renewal->skill:180u;
        const auto token=live->second.begin_spell(life,{static_cast<std::uint16_t>(spell),0},self,now,available);if(!token)return {};
        next_npc_hex_=unsigned(effect)+1;
        npc_self_enchantment_casts_.insert_or_assign(source.actor,NpcSelfEnchantmentCast{*token,plan,unsigned(effect),renewal});
        if(before)observe_npc_mind_wrack_energy_locked(source,{before->current,before->maximum,0,units?unsigned(*units%300000):0},now);
        live=npc_combat_.find(source.actor);
        if(live==npc_combat_.end()||!live->second.matches(life)||!live->second.actor(life)->health||live->second.actor(life)->health->current<=0){npc_self_enchantment_casts_.erase(source.actor);return token;}
        if(quest_npc_motion_.erase(source.actor)){const auto& body=npcs_.at(source.actor);
            for(auto&[id,p]:players_)p.outbox->enqueue({position_packet(source.actor,body.x,body.y,body.plane),stop_packet(source.actor)});}
        npc_flare_event_locked(60,source.actor,spell);return token; // genuinely untargeted
    }
    bool complete_npc_self_enchantment_locked(const combat::npcs::control::Token& token,std::uint64_t now){
        const auto pending=npc_self_enchantment_casts_.find(token.actor.actor);
        if(pending==npc_self_enchantment_casts_.end()||pending->second.token!=token||now<token.completes)return false;
        if(!npc_self_enchantment_in_progress_.insert(token.actor.actor).second)return false;
        struct Reset{std::set<unsigned>& set;unsigned actor;~Reset(){set.erase(actor);}} reset{npc_self_enchantment_in_progress_,token.actor.actor};
        auto live=npc_combat_.find(token.actor.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=token.actor){npc_self_enchantment_casts_.erase(pending);return false;}
        const auto life=live->second.lifetime();
        if(!live->second.owns_spell_action(life,token)||live->second.control_epoch(life)!=token.epoch){npc_self_enchantment_casts_.erase(pending);npc_flare_event_locked(59,token.actor.actor);return false;}
        if(now<live->second.health_clock(life).value_or(now))return false;
        advance_npc_conditions_locked(token.actor.actor,live->second,now);
        live=npc_combat_.find(token.actor.actor);
        if(live==npc_combat_.end()||!live->second.matches(life)||live->second.control_epoch(life)!=token.epoch||
           !live->second.actor(life)->health||live->second.actor(life)->health->current<=0){npc_self_enchantment_casts_.erase(token.actor.actor);return false;}
        const auto cast=pending->second;auto preview=live->second;
        if(!preview.complete_action(life,token,now)||!(cast.renewal?
                preview.apply_ether_renewal(life,*cast.renewal,cast.effect,now):
                preview.apply_aura_restoration(life,cast.plan,cast.effect,now))){
            // Effect capacity may change during a paid windup. Fail closed
            // once instead of retaining a due action that stalls World time.
            live->second.cancel_action(life,now);npc_self_enchantment_casts_.erase(pending);npc_flare_event_locked(59,token.actor.actor);return false;
        }
        const auto quote=prepare_npc_aura_completion_locked(token,now);
        const auto terminal=complete_npc_spell_terminal_locked(token,now);
        if(terminal==NpcSpellTerminal::unavailable)return false;
        npc_self_enchantment_casts_.erase(token.actor.actor);npc_flare_event_locked(58,token.actor.actor);
        if(terminal==NpcSpellTerminal::reaction_stopped)return true;
        live=npc_combat_.find(token.actor.actor);if(live==npc_combat_.end()||!live->second.matches(life))return true;
        const auto old=live->second.aura_restoration_entry(life,now);
        if(well_blocks_npc_enchantment_locked(token.actor.actor,now)){
            finish_npc_aura_completion_locked(quote,now);
            if(!succor_npc_spell_completed_locked(token,now))throw std::logic_error("NPC suppressed Aura Succor completion failed");
            completed_skill_action_locked(token.actor,token.skill->skill,now);
            return true;
        }
        if(cast.renewal){
            if(!install_npc_ether_renewal_locked(live->second,*cast.renewal,cast.effect,now))return true;
            finish_npc_aura_completion_locked(quote,now);
            if(!succor_npc_spell_completed_locked(token,now))throw std::logic_error("NPC Renewal Succor completion failed");
            completed_skill_action_locked(token.actor,token.skill->skill,now);return true;
        }
        if(!live->second.apply_aura_restoration(life,cast.plan,cast.effect,now))return true;
        std::vector<Packet> packets;
        if(old){packets.push_back(health_effect_remove(token.actor.actor,old->instance));npc_aura_credit_.erase({token.actor.actor,old->instance});}
        const auto entry=*live->second.aura_restoration_entry(life,now);
        npc_aura_credit_[{token.actor.actor,entry.instance}]=token.actor;
        packets.push_back(health_effect_add(entry,token.actor.actor,now));packets.push_back(emotes::effects_2022(token.actor.actor,live->second.actor(life)->effects));
        for(auto&[id,p]:players_)p.outbox->enqueue(packets);
        commit_applications_locked(token.actor,now);finish_npc_aura_completion_locked(quote,now);
        if(!succor_npc_spell_completed_locked(token,now))throw std::logic_error("NPC Aura Succor completion failed");
        completed_skill_action_locked(token.actor,180,now);return true;
    }
    bool gate_npc_owned_caster_locked(NpcElementalController& ai,Npc& body,combat::npcs::LiveActor& actor,std::uint64_t now){
        auto& binding=*ai.owned_quest;if(binding.retired)return false;
        const auto life=actor.lifetime();if(now<actor.health_clock(life).value_or(now))return false;
        const auto owner=players_.find(binding.owner.actor);const auto assignment=quest_npc_owners_.find(body.id);
        bool valid=owner!=players_.end()&&owner->second.effect_scope==binding.owner&&assignment!=quest_npc_owners_.end()&&
            assignment->second==std::pair{binding.owner,binding.quest};
        if(valid){const auto objective=owner->second.quest_kills.find(binding.quest);
            valid=objective!=owner->second.quest_kills.end()&&objective->second.cycle==binding.cycle&&objective->second.remaining.contains(ai.motion.policy.source);}
        const auto state=actor.actor(life);
        if(!valid||!state||!state->health||state->health->current<=0){
            if(ai.ledger.pending())ai.ledger.cancel(ai.ledger.pending()->token,now);
            if(actor.active_action(life,now))actor.cancel_action(life,now);
            npc_self_enchantment_casts_.erase(body.id);retire_npc_patrol_locked(ai.motion.policy.source);
            binding.retired=true;ai.motion.engaged=false;return false;
        }
        if(!ai.motion.returning&&npc_elemental_player_clock(owner->second)>now)return false;
        const auto loss=*actor.health_loss_revision(life);
        if(loss!=binding.health_loss_revision||state->health->current<binding.health)binding.triggered=true;
        binding.health_loss_revision=loss;
        binding.health=state->health->current;
        // Only validated, unprovoked quest actors may idle. A rejected gate
        // also represents stale ownership or future clocks, never idle leave.
        if(!binding.triggered)idle_npc_wander_locked(ai.motion,body,now);
        return binding.triggered;
    }
    bool select_npc_owned_caster_locked(NpcElementalController& ai,Npc& body,combat::npcs::LiveActor& actor,std::uint64_t now){
        auto& binding=*ai.owned_quest;auto& motion=ai.motion;
        const auto owner=players_.find(binding.owner.actor);if(owner==players_.end()||owner->second.effect_scope!=binding.owner)return false;
        if(motion.returning){
            if(move_npc_flare_locked(motion,body,{motion.home_x,motion.home_y},1,now)){motion.returning=false;binding.triggered=false;}
            return false;
        }
        const auto point=healing_position_locked(owner->second,now);
        const bool target=point&&owner->second.precise_health>0&&owner->second.npc_publication_ready&&owner->second.surface.plane==body.plane&&
            std::hypot(point->x-body.x,point->z-body.y)<=motion.policy.follow;
        if(!target){if(motion.engaged){motion.engaged=false;motion.returning=true;suspend_busy_npc_wander_locked(motion,now);}return false;}
        if(actor.knocked_down(actor.lifetime(),now))return false;
        motion.policy.target=binding.owner;motion.engaged=true;suspend_busy_npc_wander_locked(motion,now);return true;
    }
    bool dispatch_npc_self_enchantment_locked(std::uint64_t now){
        std::erase_if(npc_aura_completed_,[&](const auto& entry){const auto live=npc_combat_.find(entry.first.actor);
            return live==npc_combat_.end()||live->second.hex_scope()!=entry.first;});
        std::erase_if(npc_aura_credit_,[&](const auto& entry){const auto live=npc_combat_.find(entry.second.actor);
            return live==npc_combat_.end()||live->second.hex_scope()!=entry.second;});
        std::vector<combat::npcs::control::Token> due;
        for(const auto&[id,cast]:npc_self_enchantment_casts_)if(!npc_self_enchantment_in_progress_.contains(id)&&npc_self_enchantment_deadline_locked(cast.token)<=now)due.push_back(cast.token);
        for(const auto& token:due){complete_npc_self_enchantment_locked(token,npc_self_enchantment_deadline_locked(token));
            const auto pending=npc_self_enchantment_casts_.find(token.actor.actor);if(pending!=npc_self_enchantment_casts_.end()&&pending->second.token==token)return false;}
        return true;
    }
public:
    std::optional<combat::npcs::control::Token> begin_npc_ether_renewal(skills::effects::Scope source,
            const skills::effects::EtherRenewalPlan& plan,std::uint64_t now){
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);return begin_npc_self_enchantment_locked(source,{},now,plan);
    }
    bool complete_npc_ether_renewal(const combat::npcs::control::Token& token,std::uint64_t now){
        if(!token.skill||(token.skill->skill!=181&&token.skill->skill!=2860))return false;
        std::lock_guard lock(mutex_);const auto at=npc_self_enchantment_deadline_locked(token);
        return now>=at&&complete_npc_self_enchantment_locked(token,at);
    }
    bool arm_brood_caster(skills::effects::Scope owner,unsigned cycle,std::uint64_t now){
        if(!cycle)return false;std::lock_guard lock(mutex_);
        const auto player=players_.find(owner.actor);if(player==players_.end()||player->second.effect_scope!=owner)return false;
        const auto objective=player->second.quest_kills.find(49);
        if(objective==player->second.quest_kills.end()||objective->second.cycle!=cycle)return false;
        std::vector<skills::effects::Scope> additions;
        for(const auto source:objective->second.remaining){
            const auto live=npc_combat_.find(source.actor);const auto body=npcs_.find(source.actor);const auto assignment=quest_npc_owners_.find(source.actor);
            if(live==npc_combat_.end()||body==npcs_.end()||assignment==quest_npc_owners_.end()||assignment->second!=std::pair{owner,49u}||
               live->second.hex_scope()!=source||live->second.lifetime().map!=161||body->second.definition.level!=4||
               !live->second.hostile_to_players(live->second.lifetime()))return false;
            const auto& definition=body->second.combat_definition;
            if(!definition||!definition->spawn_health||definition->spawn_health->maximum!=100||!definition->spawn_energy||
               definition->spawn_energy->maximum!=40||definition->energy_pips!=std::optional(5u)||definition->healing_allowed!=std::optional(true))return false;
            if(const auto old=npc_elemental_ai_.find(source.actor);old!=npc_elemental_ai_.end()){
                if(old->second.motion.policy.source!=source||!old->second.owned_quest||old->second.owned_quest->owner!=owner||old->second.owned_quest->cycle!=cycle||old->second.owned_quest->quest!=49)return false;
                continue;
            }
            if(npc_flare_ai_.contains(source.actor)||npc_ordinary_ai_.contains(source.actor)||quest_npc_motion_.contains(source.actor))return false;
            additions.push_back(source);
        }
        if(additions.empty())return true;
        auto actors=npc_combat_;auto controllers=npc_elemental_ai_;
        for(const auto source:additions){
            auto& actor=actors.at(source.actor);const auto& body=npcs_.at(source.actor);
            combat::ai::FlarePolicy policy;policy.source=source;policy.target=owner;policy.level=4;
            policy.speed=220;policy.acquire=1000;policy.follow=1500;policy.leash=2000;policy.flight={1200,32};
            policy.evidence="data/gw/quests/domination-magic-ai-development-policy.txt";
            const auto spear=*skills::effects::water_projectile(214,4);
            combat::ai::elemental::Ledger ledger(source,4,{{{214,0},spear,policy.evidence}},policy.flight,4);
            auto book=ledger.reviewed_spells();book.insert(book.begin(),{{180,0},0,policy.evidence,combat::ai::SpellTargetClass::self});
            if(!actor.configure_spellbook(actor.lifetime(),std::move(book))||!actor.observe_overcast(actor.lifetime(),0,now,policy.evidence))return false;
            NpcElementalController controller{{policy,body.x,body.y,body.plane,now},std::move(ledger)};
            controller.owned_quest=NpcElementalController::OwnedQuest{owner,49,cycle,100};
            controllers.emplace(source.actor,std::move(controller));
        }
        npc_combat_.swap(actors);npc_elemental_ai_.swap(controllers);return true;
    }
    std::optional<combat::npcs::control::Token> begin_npc_aura(skills::effects::Scope source,const skills::effects::AuraRestorationPlan& plan,std::uint64_t now){
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);return begin_npc_self_enchantment_locked(source,plan,now);
    }
    bool complete_npc_aura(const combat::npcs::control::Token& token,std::uint64_t now){
        if(!token.skill||token.skill->skill!=180)return false;
        std::lock_guard lock(mutex_);const auto at=npc_self_enchantment_deadline_locked(token);
        return now>=at&&complete_npc_self_enchantment_locked(token,at);
    }
    std::optional<skills::effects::Entry> npc_aura_snapshot(skills::effects::Scope scope,std::uint64_t now)const{
        std::lock_guard lock(mutex_);const auto live=npc_combat_.find(scope.actor);
        return live!=npc_combat_.end()&&live->second.hex_scope()==scope?live->second.aura_restoration_entry(live->second.lifetime(),now):std::nullopt;
    }
