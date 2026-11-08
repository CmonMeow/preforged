// Included inside Instance. Explicit friendly direct-heal casts only;
// no production encounter or autonomous target priority is enabled here.
private:
    using FriendlyHealProfile=combat::ai::friendly::HealProfile;
    struct FriendlyHealCast {combat::ai::friendly::HealQuote quote;combat::npcs::control::Token token;};
    struct FriendlyHealerBinding {skills::effects::Scope source;FriendlyHealProfile profile;};
    std::map<unsigned,FriendlyHealerBinding> npc_friendly_healers_;
    std::map<unsigned,FriendlyHealCast> npc_friendly_heals_;
    std::set<unsigned> npc_friendly_heals_in_progress_;
    std::uint64_t npc_friendly_heal_deadline_locked(const FriendlyHealCast& cast)const{
        auto at=npc_self_enchantment_deadline_locked(cast.token);
        const auto player=players_.find(cast.quote.target.actor);
        if(player!=players_.end()&&player->second.effect_scope==cast.quote.target)
            at=std::max(at,npc_elemental_player_clock(player->second));
        else if(const auto live=npc_combat_.find(cast.quote.target.actor);live!=npc_combat_.end()&&live->second.hex_scope()==cast.quote.target)
            at=std::max(at,live->second.health_clock(live->second.lifetime()).value_or(at));
        return at;
    }
    std::optional<std::uint64_t> next_npc_friendly_heal_deadline_locked()const{
        std::optional<std::uint64_t> next;
        for(const auto&[id,cast]:npc_friendly_heals_)if(!npc_friendly_heals_in_progress_.contains(id)){
            const auto at=npc_friendly_heal_deadline_locked(cast);if(!next||at<*next)next=at;}
        return next;
    }
    std::optional<combat::ai::friendly::Healer> npc_friendly_healer_snapshot_locked(skills::effects::Scope scope)const{
        const auto live=npc_combat_.find(scope.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=scope)return {};
        const auto life=live->second.lifetime();const auto* actor=live->second.actor(life);const auto energy=live->second.energy(life);
        if(!actor||!actor->health||!energy)return {};
        return combat::ai::friendly::Healer{scope,*actor->health,energy->current,
            live->second.health_clock(life).value_or(0),live->second.energy_clock(life).value_or(0),live->second.healable_ally(life)};
    }
    std::optional<combat::ai::friendly::Recipient> npc_friendly_recipient_locked(skills::effects::Scope source,
            skills::effects::Scope target,std::uint64_t now){
        if(!npcs_.contains(source.actor))return {};
        combat::ai::friendly::Recipient facts;facts.scope=target;
        const auto player=players_.find(target.actor);
        float x=0,y=0;std::size_t plane=0;
        if(player!=players_.end()){
            // Enrollment is restricted to explicit protected/allied-to-players
            // metadata. Readiness is visibility, not an inferred PvP team rule.
            auto& p=player->second;if(p.effect_scope!=target||!p.resources||npc_elemental_player_clock(p)>now||!advance_health_locked(p,now))return {};
            const auto point=healing_position_locked(p,now);if(!point)return {};
            facts.health={p.precise_health,p.resources->max_health};facts.health_clock=p.health_updated;
            facts.allied=p.npc_publication_ready;facts.healing_allowed=true;x=point->x;y=point->z;plane=p.surface.plane;
        }else{
            auto live=npc_combat_.find(target.actor);auto body=npcs_.find(target.actor);
            if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=target||
               live->second.health_clock(live->second.lifetime()).value_or(now)>now)return {};
            advance_npc_conditions_locked(target.actor,live->second,now);refresh_npc_movement_locked(target.actor,now);
            live=npc_combat_.find(target.actor);body=npcs_.find(target.actor);
            if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=target)return {};
            const auto life=live->second.lifetime();const auto* actor=live->second.actor(life);if(!actor||!actor->health)return {};
            facts.health=*actor->health;facts.health_clock=live->second.health_clock(life).value_or(now);
            facts.allied=live->second.healable_ally(life);facts.healing_allowed=live->second.healing_allowed(life)==std::optional(true);
            x=body->second.x;y=body->second.y;plane=body->second.plane;
        }
        const auto source_body=npcs_.find(source.actor);if(source_body==npcs_.end())return {};
        facts.distance=std::hypot(x-source_body->second.x,y-source_body->second.y);
        facts.same_plane=plane==source_body->second.plane;facts.line_of_sight=facts.same_plane;
        if(navigation_&&facts.line_of_sight){const auto cell=navigation_->locate_cell(source_body->second.plane,{source_body->second.x,source_body->second.y});
            const auto trace=cell?layered_->trace({source_body->second.plane,*cell},{source_body->second.x,source_body->second.y},{x,y}):std::nullopt;
            facts.line_of_sight=trace&&trace->plane==plane;}
        return facts;
    }
    bool complete_npc_friendly_heal_locked(const combat::npcs::control::Token& token,std::uint64_t now){
        const auto found=npc_friendly_heals_.find(token.actor.actor);
        if(found==npc_friendly_heals_.end()||found->second.token!=token||now<token.completes||
           !npc_friendly_heals_in_progress_.insert(token.actor.actor).second)return false;
        struct Reset{std::set<unsigned>& active;unsigned id;~Reset(){active.erase(id);}} reset{npc_friendly_heals_in_progress_,token.actor.actor};
        const auto cast=found->second;auto live=npc_combat_.find(token.actor.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=token.actor){npc_friendly_heals_.erase(found);return false;}
        const auto life=live->second.lifetime();advance_npc_conditions_locked(token.actor.actor,live->second,now);
        live=npc_combat_.find(token.actor.actor);
        if(live==npc_combat_.end()||!live->second.matches(life)||!live->second.owns_spell_action(life,token)||
           live->second.control_epoch(life)!=token.epoch||!live->second.healable_ally(life)){
            npc_friendly_heals_.erase(token.actor.actor);
            if(live!=npc_combat_.end()&&live->second.matches(life))npc_flare_event_locked(59,token.actor.actor);
            return false;}
        const auto aura=prepare_npc_aura_completion_locked(token,now);
        const auto pre_source=npc_friendly_healer_snapshot_locked(token.actor);
        const auto pre_target=npc_friendly_recipient_locked(token.actor,cast.quote.target,now);
        const auto pre_profile=npc_friendly_healers_.find(token.actor.actor);
        if(!pre_source||!pre_target||pre_profile==npc_friendly_healers_.end()||
           !combat::ai::friendly::completion(cast.quote,token,*pre_source,*pre_target,now,true,pre_profile->second.profile)){
            live=npc_combat_.find(token.actor.actor);
            if(live==npc_combat_.end()||!live->second.matches(life)||!live->second.fail_target_action(life,token,now))return false;
            npc_friendly_heals_.erase(token.actor.actor);npc_flare_event_locked(59,token.actor.actor);return true;
        }
        const auto terminal=complete_npc_spell_terminal_locked(token,now);
        if(terminal==NpcSpellTerminal::unavailable)return false;
        npc_friendly_heals_.erase(token.actor.actor);npc_flare_event_locked(58,token.actor.actor);
        if(terminal==NpcSpellTerminal::reaction_stopped)return true;
        const auto source=npc_friendly_healer_snapshot_locked(token.actor);
        const auto target=npc_friendly_recipient_locked(token.actor,cast.quote.target,now);
        const auto profile=npc_friendly_healers_.find(token.actor.actor);
        bool succeeded=false;
        if(source&&target&&profile!=npc_friendly_healers_.end()&&profile->second.source==token.actor){
            const auto amount=combat::ai::friendly::completion(cast.quote,token,*source,*target,now,true,profile->second.profile);
            succeeded=amount.has_value();
            if(amount)if(const auto body=skills::particles::targeted_body_2022(cast.quote.target.actor,token.actor.actor,cast.quote.profile.skill))
                for(auto&[id,p]:players_)p.outbox->enqueue({*body});
            if(amount&&*amount){
                if(auto player=players_.find(cast.quote.target.actor);player!=players_.end()&&player->second.effect_scope==cast.quote.target){
                    const auto before=player->second.precise_health;
                    health_event_locked(player->second,*amount,now,true);
                    player->second.resources->health=static_cast<unsigned>(std::ceil(player->second.precise_health));publish_health_locked(player->second);
                    publish_npc_friendly_healing_gain_locked(token.actor.actor,cast.quote.target.actor,
                        player->second.precise_health-before,player->second.resources->max_health);
                }else if(auto recipient=npc_combat_.find(cast.quote.target.actor);recipient!=npc_combat_.end()&&recipient->second.hex_scope()==cast.quote.target){
                    const auto recipient_life=recipient->second.lifetime();
                    const auto before=*recipient->second.actor(recipient_life)->health;
                    if(recipient->second.heal(recipient_life,*amount,now)){
                        const auto after=*recipient->second.actor(recipient_life)->health;
                        for(auto&[id,p]:players_)p.outbox->enqueue(combat::health_packets_2022(*recipient->second.actor(recipient_life)));
                        publish_npc_friendly_healing_gain_locked(token.actor.actor,cast.quote.target.actor,after.current-before.current,after.maximum);
                    }
                }
            }
        }
        finish_npc_aura_completion_locked(aura,now);
        if(succeeded&&!succor_npc_spell_completed_locked(token,now))throw std::logic_error("NPC healing Succor completion failed");
        completed_skill_action_locked(token.actor,cast.quote.profile.skill,now);return true;
    }
    bool dispatch_npc_friendly_heals_locked(std::uint64_t now){
        std::erase_if(npc_friendly_healers_,[&](const auto& item){const auto live=npc_combat_.find(item.first);
            return live==npc_combat_.end()||live->second.hex_scope()!=item.second.source;});
        std::vector<combat::npcs::control::Token> due;
        for(const auto&[id,cast]:npc_friendly_heals_)if(!npc_friendly_heals_in_progress_.contains(id)&&npc_friendly_heal_deadline_locked(cast)<=now)due.push_back(cast.token);
        for(const auto& token:due){complete_npc_friendly_heal_locked(token,now);
            if(const auto pending=npc_friendly_heals_.find(token.actor.actor);pending!=npc_friendly_heals_.end()&&pending->second.token==token)return false;}
        return true;
    }
    void publish_npc_friendly_healing_gain_locked(unsigned source,unsigned target,double gain,double maximum){
        if(gain<=0||maximum<=0)return;
        const auto packet=skills::particles::health_change_2022(target,source,gain,maximum);
        for(auto&[id,p]:players_)p.outbox->enqueue({packet});
    }
public:
    bool configure_npc_friendly_healer(skills::effects::Scope source,const FriendlyHealProfile& profile){
        std::lock_guard lock(mutex_);auto live=npc_combat_.find(source.actor);
        const auto body=npcs_.find(source.actor);
        if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=source||npc_friendly_healers_.contains(source.actor)||
           (profile.divine&&body->second.definition.primary!=3))return false;
        // One existing book/control/Energy owner is used.
        if(!live->second.configure_friendly_spellbook(live->second.lifetime(),profile))return false;
        npc_friendly_healers_.emplace(source.actor,FriendlyHealerBinding{source,profile});return true;
    }
    std::optional<combat::npcs::control::Token> begin_npc_friendly_heal(skills::effects::Scope source,
            skills::effects::Scope target,std::uint64_t now){
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        return begin_npc_friendly_heal_locked(source,target,now);
    }
private:
    std::optional<combat::npcs::control::Token> begin_npc_friendly_heal_locked(skills::effects::Scope source,
            skills::effects::Scope target,std::uint64_t now){
        const auto profile=npc_friendly_healers_.find(source.actor);auto live=npc_combat_.find(source.actor);
        if(profile==npc_friendly_healers_.end()||profile->second.source!=source||live==npc_combat_.end()||
           live->second.hex_scope()!=source)return {};
        const auto life=live->second.lifetime();
        if(const auto old=npc_friendly_heals_.find(source.actor);old!=npc_friendly_heals_.end()){
            if(live->second.owns_spell_action(life,old->second.token))return {};
            npc_friendly_heals_.erase(old);
        }
        if(!settle_mind_wrack_caster_locked(source,now))return {};
        advance_npc_conditions_locked(source.actor,live->second,now);refresh_npc_movement_locked(source.actor,now);
        live=npc_combat_.find(source.actor);if(live==npc_combat_.end()||!live->second.matches(life)||!live->second.advance_energy(life,now))return {};
        const auto healer=npc_friendly_healer_snapshot_locked(source);const auto recipient=npc_friendly_recipient_locked(source,target,now);
        if(!healer||!recipient)return {};
        const auto proposed=combat::ai::friendly::prepare(profile->second.profile,*healer,*recipient,now);if(!proposed)return {};
        const combat::ai::SpellTarget facts{target,recipient->distance,true,false,recipient->same_plane,recipient->line_of_sight};
        const combat::ai::Spellbook::Availability available=[&](skills::Slot slot,std::uint64_t at){return skill_disabled_slot_locked(source,slot,at);};
        live=npc_combat_.find(source.actor);if(live==npc_combat_.end()||!live->second.matches(life))return {};
        const auto energy=live->second.energy(life);const auto units=live->second.precise_energy_units(life);
        const skills::Slot slot{profile->second.profile.skill,profile->second.profile.copy};
        const auto token=live->second.begin_spell(life,slot,facts,now,available);if(!token)return {};
        const auto paid=combat::ai::friendly::bind(*proposed,*token);if(!paid)throw std::logic_error("admitted friendly heal token changed");
        npc_friendly_heals_.emplace(source.actor,FriendlyHealCast{*paid,*token});
        if(energy)observe_npc_mind_wrack_energy_locked(source,{energy->current,energy->maximum,0,units?unsigned(*units%300000):0},now);
        live=npc_combat_.find(source.actor);
        if(live==npc_combat_.end()||!live->second.matches(life)||!live->second.healable_ally(life)){
            npc_friendly_heals_.erase(source.actor);return token;}
        if(quest_npc_motion_.erase(source.actor)){const auto& body=npcs_.at(source.actor);
            for(auto&[id,p]:players_)p.outbox->enqueue({position_packet(source.actor,body.x,body.y,body.plane),stop_packet(source.actor)});}
        npc_flare_event_locked(60,source.actor,slot.skill,target.actor);return token;
    }

public:
