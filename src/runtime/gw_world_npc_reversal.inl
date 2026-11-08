// Explicit friendly NPC -> player casts. No inferred loadout, rank, Divine
// Favor or autonomous target priority. Player Runtime307 remains unbound.
private:
    struct NpcReversalProfile {skills::effects::Scope source;unsigned rank=0,divine=0;};
    struct NpcReversalCast {combat::npcs::control::Token token;skills::effects::Scope target;NpcReversalProfile profile;};
    std::map<unsigned,NpcReversalProfile> npc_reversal_profiles_;
    std::map<unsigned,NpcReversalCast> npc_reversal_casts_;
    std::set<unsigned> npc_reversal_in_progress_;
    std::uint64_t npc_reversal_deadline_locked(const NpcReversalCast& cast)const{
        auto at=npc_self_enchantment_deadline_locked(cast.token);
        if(const auto target=players_.find(cast.target.actor);target!=players_.end()&&target->second.effect_scope==cast.target)
            at=std::max(at,npc_elemental_player_clock(target->second));
        return at;
    }
    std::optional<std::uint64_t> next_npc_reversal_deadline_locked()const{
        std::optional<std::uint64_t> result;
        for(const auto&[id,cast]:npc_reversal_casts_)if(!npc_reversal_in_progress_.contains(id)){
            const auto at=npc_reversal_deadline_locked(cast);if(!result||at<*result)result=at;}
        return result;
    }
    bool npc_reversal_target_locked(skills::effects::Scope source,skills::effects::Scope target,std::uint64_t now){
        const auto player=players_.find(target.actor);auto live=npc_combat_.find(source.actor);
        if(player==players_.end()||player->second.effect_scope!=target||!player->second.resources||
           !player->second.effect_registry||!player->second.health_regeneration||
           live==npc_combat_.end()||live->second.hex_scope()!=source||!live->second.healable_ally(live->second.lifetime()))return false;
        const auto facts=npc_friendly_recipient_locked(source,target,now);
        return facts&&facts->allied&&facts->health.current>0&&facts->same_plane&&facts->line_of_sight&&facts->distance<=1248;
    }
    struct NpcReversalApplication {
        skills::effects::Registry registry;skills::regeneration::State regeneration;
        skills::effects::Entry entry;std::vector<skills::effects::Entry> removed;
    };
    std::optional<NpcReversalApplication> prepare_npc_reversal_application_locked(const NpcReversalCast& cast,std::uint64_t now){
        const auto plan=skills::effects::reversal_fortune(cast.profile.rank);
        if(!plan||now>UINT64_MAX-plan->duration||!npc_reversal_target_locked(cast.token.actor,cast.target,now))return {};
        prune_reversal_caster_credit_locked(now);
        if(reversal_caster_credit_.size()>=4096)return {};
        auto& p=players_.at(cast.target.actor);
        if(p.next_health_effect>UINT32_MAX)return {};
        auto registry=*p.effect_registry;auto regeneration=*p.health_regeneration;
        const auto effect=static_cast<unsigned>(p.next_health_effect);
        const skills::effects::Entry entry{effect,cast.token.actor.actor,307,cast.profile.rank,
            skills::effects::Category::enchantment,now,now+plan->duration};
        auto applied=registry.apply(cast.target,entry,skills::effects::Replace::same_skill);
        if(!applied.accepted||!regeneration.apply(cast.target,{effect,entry.cause,307,0,now,*entry.expires}))return {};
        return NpcReversalApplication{std::move(registry),std::move(regeneration),entry,std::move(applied.removed)};
    }
    bool complete_npc_reversal_locked(const combat::npcs::control::Token& token,std::uint64_t now){
        const auto found=npc_reversal_casts_.find(token.actor.actor);
        if(found==npc_reversal_casts_.end()||found->second.token!=token||now<token.completes||
           !npc_reversal_in_progress_.insert(token.actor.actor).second)return false;
        struct Reset {std::set<unsigned>& active;unsigned id;~Reset(){active.erase(id);}} reset{npc_reversal_in_progress_,token.actor.actor};
        const auto cast=found->second;auto live=npc_combat_.find(token.actor.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=token.actor){npc_reversal_casts_.erase(found);return false;}
        const auto life=live->second.lifetime();advance_npc_conditions_locked(token.actor.actor,live->second,now);
        live=npc_combat_.find(token.actor.actor);
        if(live==npc_combat_.end()||!live->second.matches(life)||!live->second.owns_spell_action(life,token)||
           live->second.control_epoch(life)!=token.epoch||!live->second.healable_ally(life)){
            npc_reversal_casts_.erase(token.actor.actor);return false;}
        const bool target_valid=npc_reversal_target_locked(token.actor,cast.target,now);
        const bool blocked=target_valid&&well_blocks_player_enchantment_locked(players_.at(cast.target.actor),now);
        if(!target_valid||(!blocked&&!prepare_npc_reversal_application_locked(cast,now))){
            live=npc_combat_.find(token.actor.actor);
            if(live==npc_combat_.end()||!live->second.matches(life)||!live->second.fail_target_action(life,token,now))return false;
            npc_reversal_casts_.erase(token.actor.actor);npc_flare_event_locked(59,token.actor.actor);return true;
        }
        const auto aura=prepare_npc_aura_completion_locked(token,now);
        const auto terminal=complete_npc_spell_terminal_locked(token,now);
        if(terminal==NpcSpellTerminal::unavailable)return false;
        npc_reversal_casts_.erase(token.actor.actor);npc_flare_event_locked(58,token.actor.actor);
        if(terminal==NpcSpellTerminal::reaction_stopped)return true;
        live=npc_combat_.find(token.actor.actor);
        if(live==npc_combat_.end()||!live->second.matches(life)||!live->second.healable_ally(life))return true;
        if(npc_reversal_target_locked(token.actor,cast.target,now)&&well_blocks_player_enchantment_locked(players_.at(cast.target.actor),now)){
            if(const auto visual=skills::particles::targeted_body_2022(cast.target.actor,token.actor.actor,307))
                for(auto&[id,p]:players_)if(p.npc_publication_ready)p.outbox->enqueue({*visual});
            finish_npc_aura_completion_locked(aura,now);
            if(!succor_npc_spell_completed_locked(token,now))throw std::logic_error("NPC blocked Reversal Succor completion failed");
            completed_skill_action_locked(token.actor,307,now);return true;
        }
        auto application=prepare_npc_reversal_application_locked(cast,now);
        if(!application)return true; // Completion reactions can retire a target.
        const auto visual=skills::particles::targeted_body_2022(cast.target.actor,token.actor.actor,307);
        if(!visual)return true;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;
        std::set<const Outbox*> seen;
        for(auto&[id,p]:players_)if(seen.insert(p.outbox.get()).second)recipients.emplace_back(p.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto&[box,queue]:recipients)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        auto& p=players_.at(cast.target.actor);std::vector<Packet> statuses{*visual};
        for(const auto& old:application->removed){
            p.health_effects.erase(old.instance);reversal_caster_credit_.erase({cast.target,old.instance});
            statuses.push_back(health_effect_remove(cast.target.actor,old.instance));
        }
        *p.effect_registry=std::move(application->registry);*p.health_regeneration=std::move(application->regeneration);++p.next_health_effect;
        p.health_effects.emplace(application->entry.instance,application->entry);
        reversal_caster_credit_[{cast.target,application->entry.instance}]=token.actor;
        statuses.push_back(health_effect_add(application->entry,cast.target.actor,now));
        for(auto&[id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue(statuses);
        commit_applications_locked(cast.target,now);
        if(const auto current=players_.find(cast.target.actor);current!=players_.end()&&current->second.effect_scope==cast.target){
            auto& recipient=current->second;
            if(cast.profile.divine&&recipient.precise_health>0){
                const auto before=recipient.precise_health;
                health_event_locked(recipient,std::lround(3.2*cast.profile.divine),now,true);
                const auto gained=recipient.precise_health-before;
                if(gained>0)for(auto&[id,peer]:players_)if(peer.npc_publication_ready)
                    peer.outbox->enqueue({skills::particles::health_change_2022(cast.target.actor,token.actor.actor,gained,recipient.resources->max_health)});
                publish_health_locked(recipient);
            }
            if(recipient.precise_health<=0)kill_locked(recipient,now);
            publish_status_locked(recipient,now);
        }
        captures.clear();for(auto&[box,queue]:recipients)box->enqueue(queue.packets);
        finish_npc_aura_completion_locked(aura,now);
        if(!succor_npc_spell_completed_locked(token,now))throw std::logic_error("NPC Reversal Succor completion failed");
        completed_skill_action_locked(token.actor,307,now);return true;
    }
    bool dispatch_npc_reversal_locked(std::uint64_t now){
        std::erase_if(npc_reversal_profiles_,[&](const auto& item){const auto live=npc_combat_.find(item.first);return live==npc_combat_.end()||live->second.hex_scope()!=item.second.source;});
        std::vector<combat::npcs::control::Token> due;
        for(const auto&[id,cast]:npc_reversal_casts_)if(!npc_reversal_in_progress_.contains(id)&&npc_reversal_deadline_locked(cast)<=now)due.push_back(cast.token);
        for(const auto& token:due){complete_npc_reversal_locked(token,now);
            if(const auto pending=npc_reversal_casts_.find(token.actor.actor);pending!=npc_reversal_casts_.end()&&pending->second.token==token)return false;}
        return true;
    }
public:
    bool configure_npc_reversal_caster(skills::effects::Scope source,unsigned rank,unsigned divine,std::string evidence){
        if(!skills::effects::reversal_fortune(rank)||divine>20)return false;
        std::lock_guard lock(mutex_);auto live=npc_combat_.find(source.actor);const auto body=npcs_.find(source.actor);
        if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=source||npc_reversal_profiles_.contains(source.actor)||
           (divine&&body->second.definition.primary!=3))return false;
        if(!live->second.configure_friendly_reversal_spellbook(live->second.lifetime(),std::move(evidence)))return false;
        npc_reversal_profiles_.emplace(source.actor,NpcReversalProfile{source,rank,divine});return true;
    }
    std::optional<combat::npcs::control::Token> begin_npc_reversal(skills::effects::Scope source,skills::effects::Scope target,std::uint64_t now){
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto profile=npc_reversal_profiles_.find(source.actor);auto live=npc_combat_.find(source.actor);
        if(profile==npc_reversal_profiles_.end()||profile->second.source!=source||live==npc_combat_.end()||live->second.hex_scope()!=source)return {};
        const auto life=live->second.lifetime();
        if(const auto old=npc_reversal_casts_.find(source.actor);old!=npc_reversal_casts_.end()){
            if(live->second.owns_spell_action(life,old->second.token))return {};
            npc_reversal_casts_.erase(old);
        }
        if(!settle_mind_wrack_caster_locked(source,now))return {};
        advance_npc_conditions_locked(source.actor,live->second,now);refresh_npc_movement_locked(source.actor,now);
        live=npc_combat_.find(source.actor);
        if(live==npc_combat_.end()||!live->second.matches(life)||!live->second.advance_energy(life,now)||!npc_reversal_target_locked(source,target,now))return {};
        const auto recipient=npc_friendly_recipient_locked(source,target,now);if(!recipient)return {};
        const combat::ai::SpellTarget facts{target,recipient->distance,true,false,recipient->same_plane,recipient->line_of_sight};
        const combat::ai::Spellbook::Availability available=[&](skills::Slot slot,std::uint64_t at){return skill_disabled_slot_locked(source,slot,at);};
        live=npc_combat_.find(source.actor);if(live==npc_combat_.end()||!live->second.matches(life))return {};
        const auto energy=live->second.energy(life);const auto units=live->second.precise_energy_units(life);
        const auto token=live->second.begin_spell(life,{307,0},facts,now,available);if(!token)return {};
        npc_reversal_casts_.emplace(source.actor,NpcReversalCast{*token,target,profile->second});
        if(energy)observe_npc_mind_wrack_energy_locked(source,{energy->current,energy->maximum,0,units?unsigned(*units%300000):0},now);
        live=npc_combat_.find(source.actor);const auto body=npcs_.find(source.actor);
        if(live==npc_combat_.end()||body==npcs_.end()||!live->second.matches(life)||!live->second.healable_ally(life)){
            npc_reversal_casts_.erase(source.actor);return token;}
        if(quest_npc_motion_.erase(source.actor))for(auto&[id,p]:players_)if(p.npc_publication_ready)
            p.outbox->enqueue({position_packet(source.actor,body->second.x,body->second.y,body->second.plane),stop_packet(source.actor)});
        npc_flare_event_locked(60,source.actor,307,target.actor);return token;
    }
    bool complete_npc_reversal(const combat::npcs::control::Token& token,std::uint64_t now){
        std::lock_guard lock(mutex_);if(now<token.completes)return false;
        const auto found=npc_reversal_casts_.find(token.actor.actor);
        if(found==npc_reversal_casts_.end()||found->second.token!=token)return false;
        return complete_npc_reversal_locked(token,std::max(now,npc_reversal_deadline_locked(found->second)));
    }
