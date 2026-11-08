// Included inside Instance. Explicit hostile-NPC Imbue Health casts.
// Encounter enrollment and autonomous ally selection remain content work.
private:
    struct NpcImbueCast {
        combat::npcs::control::Token token;
        skills::effects::Scope target;
        skills::effects::ImbueHealthPlan plan;
    };
    std::map<unsigned,NpcImbueCast> npc_imbue_casts_;
    struct NpcImbueAi {skills::effects::Scope source;unsigned rank;double below_fraction;};
    std::map<unsigned,NpcImbueAi> npc_imbue_ai_;
    std::optional<std::uint64_t> next_npc_imbue_deadline_locked()const{
        std::optional<std::uint64_t> next;
        for(const auto& [id,cast]:npc_imbue_casts_)
            if(!next||cast.token.completes<*next)next=cast.token.completes;
        return next;
    }
    bool npc_imbue_target_locked(skills::effects::Scope source,
                                  skills::effects::Scope target)const{
        if(source==target)return false;
        const auto caster=npc_combat_.find(source.actor),ally=npc_combat_.find(target.actor);
        const auto from=npcs_.find(source.actor),to=npcs_.find(target.actor);
        if(caster==npc_combat_.end()||ally==npc_combat_.end()||
           from==npcs_.end()||to==npcs_.end()||
           caster->second.hex_scope()!=source||ally->second.hex_scope()!=target||
           from->second.plane!=to->second.plane||
           std::hypot(from->second.x-to->second.x,from->second.y-to->second.y)>1248)return false;
        const auto cl=caster->second.lifetime(),al=ally->second.lifetime();
        const auto* ca=caster->second.actor(cl);const auto* aa=ally->second.actor(al);
        const auto allegiance=caster->second.packed_allegiance(cl);
        if(!caster->second.hostile_to_players(cl)||!ally->second.hostile_to_players(al)||
           !allegiance||!*allegiance||ally->second.packed_allegiance(al)!=allegiance||
           ally->second.healing_allowed(al)==std::optional(false)||
           !ca||!ca->health||ca->health->current<=0||!aa||!aa->health||aa->health->current<=0)return false;
        if(navigation_){
            const auto cell=navigation_->locate_cell(from->second.plane,{from->second.x,from->second.y});
            const auto trace=cell?layered_->trace({from->second.plane,*cell},
                {from->second.x,from->second.y},{to->second.x,to->second.y}):std::nullopt;
            if(!trace||trace->plane!=to->second.plane)return false;
        }
        return true;
    }
    bool complete_npc_imbue_locked(const combat::npcs::control::Token& token,std::uint64_t now){
        const auto pending=npc_imbue_casts_.find(token.actor.actor);
        if(pending==npc_imbue_casts_.end()||pending->second.token!=token||now<token.completes)return false;
        const auto cast=pending->second;
        auto live=npc_combat_.find(token.actor.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=token.actor){npc_imbue_casts_.erase(pending);return false;}
        const auto life=live->second.lifetime();
        if(!live->second.owns_spell_action(life,token)||live->second.control_epoch(life)!=token.epoch){
            npc_imbue_casts_.erase(pending);return false;
        }
        advance_npc_conditions_locked(token.actor.actor,live->second,token.completes);
        live=npc_combat_.find(token.actor.actor);
        if(live==npc_combat_.end()||!live->second.matches(life)||
           !live->second.owns_spell_action(life,token)||live->second.control_epoch(life)!=token.epoch){
            npc_imbue_casts_.erase(token.actor.actor);return false;
        }
        if(!npc_imbue_target_locked(token.actor,cast.target)){
            if(!live->second.fail_target_action(life,token,token.completes))return false;
            npc_imbue_casts_.erase(token.actor.actor);npc_flare_event_locked(59,token.actor.actor);
            return true;
        }
        const auto aura=prepare_npc_aura_completion_locked(token,token.completes);
        const auto terminal=complete_npc_spell_terminal_locked(token,token.completes);
        if(terminal==NpcSpellTerminal::unavailable)return false;
        npc_imbue_casts_.erase(token.actor.actor);npc_flare_event_locked(58,token.actor.actor);
        if(terminal==NpcSpellTerminal::reaction_stopped)return true;
        bool succeeded=false;
        {
            if(npc_imbue_target_locked(token.actor,cast.target)&&
               npc_imbue_health_locked(token.actor,cast.target,cast.plan,token.completes)){
                succeeded=true;
                // The captured 1400 body is on the caster; the skill row's
                // target body is a sentinel and must not produce a target FX.
                for(auto& [id,peer]:players_)if(peer.npc_publication_ready)
                    peer.outbox->enqueue({combat::detail::integer(21,token.actor.actor,1400)});
            }
        }
        finish_npc_aura_completion_locked(aura,token.completes);
        if(succeeded&&!succor_npc_spell_completed_locked(token,token.completes))throw std::logic_error("NPC Imbue Succor completion failed");
        completed_skill_action_locked(token.actor,1526,token.completes);
        return true;
    }
    bool dispatch_npc_imbue_locked(std::uint64_t now){
        std::vector<combat::npcs::control::Token> due;
        for(const auto& [id,cast]:npc_imbue_casts_)
            if(cast.token.completes<=now)due.push_back(cast.token);
        for(const auto& token:due){
            complete_npc_imbue_locked(token,token.completes);
            const auto pending=npc_imbue_casts_.find(token.actor.actor);
            if(pending!=npc_imbue_casts_.end()&&pending->second.token==token)return false;
        }
        return true;
    }
    void drive_npc_imbue_ai_locked(std::uint64_t now){
        // Private-server policy for explicitly enrolled, stationary healers:
        // choose the most wounded in-range NPC of the same observed allegiance.
        // Captures do not establish ArenaNet's priority formula.
        std::erase_if(npc_imbue_ai_,[&](const auto& entry){
            const auto live=npc_combat_.find(entry.first);
            return live==npc_combat_.end()||live->second.hex_scope()!=entry.second.source;
        });
        for(const auto& [id,binding]:npc_imbue_ai_){
            auto caster=npc_combat_.find(id);
            if(caster==npc_combat_.end())continue;
            const auto life=caster->second.lifetime();
            if(now<caster->second.health_clock(life).value_or(now)||
               now<caster->second.energy_clock(life).value_or(now))continue;
            advance_npc_conditions_locked(id,caster->second,now);
            caster=npc_combat_.find(id);
            if(caster==npc_combat_.end()||!caster->second.matches(life)||
               caster->second.active_action(life,now)||!caster->second.advance_energy(life,now))continue;
            std::optional<skills::effects::Scope> choice;
            double lowest=binding.below_fraction;
            for(const auto& [target_id,recipient]:npc_combat_){
                const auto target=recipient.hex_scope();
                if(now<recipient.health_clock(recipient.lifetime()).value_or(now)||
                   !npc_imbue_target_locked(binding.source,target))continue;
                const auto* actor=recipient.actor(recipient.lifetime());
                if(!actor||!actor->health||!actor->health->maximum)continue;
                const auto fraction=actor->health->current/actor->health->maximum;
                if(fraction>=binding.below_fraction||
                   (choice&&(fraction>lowest||
                       (fraction==lowest&&target.actor>=choice->actor))))continue;
                choice=target;lowest=fraction;
            }
            if(choice)begin_npc_imbue_health_locked(binding.source,*choice,binding.rank,now);
        }
    }
public:
    bool configure_npc_imbue_ai(skills::effects::Scope source,unsigned rank,double below_fraction){
        if(!skills::effects::imbue_health(rank)||!std::isfinite(below_fraction)||
           below_fraction<=0||below_fraction>1)return false;
        std::lock_guard lock(mutex_);
        const auto live=npc_combat_.find(source.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=source||npc_imbue_ai_.contains(source.actor))return false;
        const auto life=live->second.lifetime();
        if(!live->second.hostile_to_players(life)||
           !live->second.spell_recharge_until(life,{1526,0},0))return false;
        npc_imbue_ai_.emplace(source.actor,NpcImbueAi{source,rank,below_fraction});
        return true;
    }
    std::optional<combat::npcs::control::Token> begin_npc_imbue_health(
            skills::effects::Scope source,skills::effects::Scope target,unsigned rank,std::uint64_t now){
        const auto plan=skills::effects::imbue_health(rank);if(!plan)return {};
        std::lock_guard lock(mutex_);
        return begin_npc_imbue_health_locked(source,target,rank,now);
    }
private:
    std::optional<combat::npcs::control::Token> begin_npc_imbue_health_locked(
            skills::effects::Scope source,skills::effects::Scope target,unsigned rank,std::uint64_t now){
        const auto plan=skills::effects::imbue_health(rank);if(!plan)return {};
        if(!settle_timed_effects_locked(now)||!settle_mind_wrack_caster_locked(source,now))return {};
        auto live=npc_combat_.find(source.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=source)return {};
        const auto life=live->second.lifetime();
        if(const auto old=npc_imbue_casts_.find(source.actor);old!=npc_imbue_casts_.end()){
            if(live->second.owns_spell_action(life,old->second.token))return {};
            npc_imbue_casts_.erase(old);
        }
        advance_npc_conditions_locked(source.actor,live->second,now);
        refresh_npc_movement_locked(source.actor,now);
        auto recipient=npc_combat_.find(target.actor);
        if(recipient!=npc_combat_.end()&&recipient->second.hex_scope()==target)
            advance_npc_conditions_locked(target.actor,recipient->second,now);
        if(!npc_imbue_target_locked(source,target))return {};
        live=npc_combat_.find(source.actor);if(live==npc_combat_.end()||!live->second.matches(life))return {};
        const auto& from=npcs_.at(source.actor),&to=npcs_.at(target.actor);
        const combat::ai::SpellTarget facts{target,float(std::hypot(from.x-to.x,from.y-to.y)),
            true,false,true,true};
        const combat::ai::Spellbook::Availability available=[&](skills::Slot slot,std::uint64_t at){
            return skill_disabled_slot_locked(source,slot,at);
        };
        const auto choice=live->second.choose_spell(life,facts,now,available);
        if(!choice||choice->skill!=1526)return {};
        const auto energy=live->second.energy(life);
        const auto units=live->second.precise_energy_units(life);
        const auto token=live->second.begin_spell(life,*choice,facts,now,available);
        if(!token)return {};
        npc_imbue_casts_[source.actor]={*token,target,*plan};
        publish_npc_enchantment_ends_locked(live->second);
        if(energy)observe_npc_mind_wrack_energy_locked(source,
            {energy->current,energy->maximum,0,units?unsigned(*units%300000):0},now);
        if(quest_npc_motion_.erase(source.actor))
            for(auto& [id,peer]:players_)if(peer.npc_publication_ready)
                peer.outbox->enqueue({position_packet(source.actor,from.x,from.y,from.plane),stop_packet(source.actor)});
        npc_flare_event_locked(60,source.actor,1526,target.actor);
        return token;
    }
public:
    bool complete_npc_imbue_health(const combat::npcs::control::Token& token,std::uint64_t now){
        if(!token.skill||token.skill->skill!=1526||now<token.completes)return false;
        std::lock_guard lock(mutex_);
        return complete_npc_imbue_locked(token,token.completes);
    }

