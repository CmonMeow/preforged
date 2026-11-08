// Shared scoped holy damage and complete paid feedback publication.
public:
    bool holy_damage(skills::effects::Scope scope,skills::effects::Scope target,
                     const skills::effects::HolyPlan& plan,unsigned level,std::uint64_t now,std::vector<Packet>* visuals=nullptr,std::function<void()>* publication=nullptr){
        if(publication)*publication={};if(publication&&!visuals)return false;
        if(visuals)visuals->clear();
        std::lock_guard lock(mutex_);
        DamagePublication pending;if(publication)begin_damage_publication_locked(pending);
        const auto execute=[&]()->bool{
        settle_damage_fields_locked(now);
        const auto player_source=players_.find(scope.actor),player_target=players_.find(target.actor);
        if(player_source!=players_.end()&&player_target!=players_.end()){
            auto& source=player_source->second;auto& victim=player_target->second;
            const auto canonical=skills::effects::holy_damage(plan.skill,plan.rank);
            if(!canonical||*canonical!=plan||source.effect_scope!=scope||victim.effect_scope!=target||
               !source.resources||!victim.resources||!victim.reviewed_holy_traits||!victim.reviewed_holy_bonus||
               !source.team_token||!victim.team_token||source.team_token==victim.team_token||
               source.surface.plane!=victim.surface.plane||source.precise_health<=0||victim.precise_health<=0||
               source.knocked_down_until>now||!resurrection_clock_valid(source,now)||!resurrection_clock_valid(victim,now))return false;
            const auto origin=healing_position_locked(source,now),destination=healing_position_locked(victim,now);
            if(!origin||!destination||std::hypot(origin->x-destination->x,origin->z-destination->z)>plan.range)return false;
            if(navigation_){const auto line=layered_->trace(source.surface,*origin,*destination);if(!line||line->plane!=victim.surface.plane)return false;}
            if(plan.energy_if_attacking&&(!advance_resource_event_locked(source,now)||source.precise_health<=0))return false;
            if(!advance_health_locked(source,now)||!advance_health_locked(victim,now)||source.precise_health<=0||victim.precise_health<=0)return false;
            const auto resolved=plan.skill==1689?skills::effects::holy_damage(1689,plan.rank,
                static_cast<unsigned>(std::min<std::size_t>(20,source.effect_registry->count(scope,skills::effects::Category::enchantment,now)))):
                std::optional(plan);
            if(!resolved)return false;
            auto strikes=skills::effects::holy_strikes(*resolved,*victim.reviewed_holy_traits,level,victim.knocked_down_until>now);
            if(!strikes)return false;
            for(auto& damage:*strikes)damage.amount+=*victim.reviewed_holy_bonus;
            auto actor=incoming_spell_actor_locked(victim,{victim.id,target.generation,combat::Health{victim.precise_health,victim.resources->max_health},{},victim.effects,true},now);
            if(!admit_player_damage_sequence_locked(victim,*strikes,now,[&](const Player& current,combat::Health health){
                return incoming_spell_actor_locked(current,{current.id,target.generation,health,{},current.effects,true},now,true);
            }))return false;
            const auto admitted_target=players_.find(target.actor);
            if(admitted_target==players_.end()||admitted_target->second.effect_scope!=target||!admitted_target->second.resources||admitted_target->second.precise_health<=0)return false;
            auto& current_target=admitted_target->second;
            const bool attacking=current_target.ordinary_animation||now<current_target.melee_skill_until;
            if(plan.skill==296&&attacking&&current_target.control_epoch==UINT64_MAX)return false;
            if(plan.skill==296&&attacking)knockdown_player_locked(current_target,2000,now);
            for(const auto& damage:*strikes){
                const auto current=players_.find(target.actor);
                if(current==players_.end()||current->second.effect_scope!=target||!current->second.resources||current->second.precise_health<=0)break;
                auto& recipient=current->second;
                actor=incoming_spell_actor_locked(recipient,{recipient.id,target.generation,combat::Health{recipient.precise_health,recipient.resources->max_health},{},recipient.effects,true},now,true);
                const auto hit=combat::apply(actor,damage,true);
                if(hit.reason!=combat::Reason::accepted||!commit_player_reactive_hit_locked(recipient,hit,scope.actor,now,false,nullptr,false,true,nullptr,scope))
                    throw std::logic_error("admitted player holy damage failed");
                if(recipient.passive_recovery)recipient.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
            }
            if(plan.skill==312||plan.skill==2006||plan.skill==1689)publish_target_body_locked(target.actor,plan.skill,scope.actor,visuals);
            const auto current_source=players_.find(scope.actor);
            if(attacking&&plan.energy_if_attacking&&current_source!=players_.end()&&current_source->second.effect_scope==scope&&current_source->second.precise_health>0){
                const auto gain=gain_energy_locked(current_source->second,plan.energy_if_attacking,now);
                if(gain.changed())current_source->second.outbox->enqueue(owner_vitals(current_source->second));
            }
            return true;
        }
        const auto source_at=[&]()->Player*{const auto p=players_.find(scope.actor);return p!=players_.end()&&p->second.effect_scope==scope?&p->second:nullptr;};
        const auto live_at=[&]()->combat::npcs::LiveActor*{const auto n=npc_combat_.find(target.actor);return n!=npc_combat_.end()&&n->second.hex_scope()==target?&n->second:nullptr;};
        auto* source=source_at();auto* live=live_at();auto placed=npcs_.find(target.actor);
        const auto canonical=skills::effects::holy_damage(plan.skill,plan.rank);
        if(!canonical||*canonical!=plan||!source||!live||placed==npcs_.end()||!source->resources||source->precise_health<=0||
           source->knocked_down_until>now||now<source->health_updated||source->surface.plane!=placed->second.plane||!live->hostile_to_players(live->lifetime()))return false;
        if(now<live->health_clock(live->lifetime()).value_or(now))return false;
        advance_npc_conditions_locked(target.actor,*live,now);refresh_npc_movement_locked(target.actor,now);
        live=live_at();source=source_at();placed=npcs_.find(target.actor);
        if(!live||!source||placed==npcs_.end())return false;
        const auto actor=live->actor(live->lifetime());if(!actor||!actor->health||actor->health->current<=0)return false;
        const bool attacking=live->attacking(live->lifetime(),now);
        if(plan.skill==296&&attacking&&!live->knockdown_allowed(live->lifetime(),now).has_value())return false;
        const auto traits=live->holy_traits(live->lifetime());if(!traits)return false;
        if(plan.skill==1689&&!source->effect_registry)return false;
        const auto resolved=plan.skill==1689?skills::effects::holy_damage(1689,plan.rank,
            static_cast<unsigned>(std::min<std::size_t>(20,source->effect_registry->count(scope,skills::effects::Category::enchantment,now)))):std::optional(plan);
        if(!resolved)return false;
        const auto strikes=skills::effects::holy_strikes(*resolved,*traits,level,live->knocked_down(live->lifetime(),now));if(!strikes)return false;
        const auto origin=healing_position_locked(*source,now);if(!origin||std::hypot(origin->x-placed->second.x,origin->z-placed->second.y)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(source->surface,*origin,{placed->second.x,placed->second.y});if(!line||line->plane!=placed->second.plane)return false;}
        if(plan.energy_if_attacking&&(!advance_resource_event_locked(*source,now)||source->precise_health<=0))return false;
        if(!advance_health_locked(*source,now)||source->precise_health<=0)return false;
        live=live_at();source=source_at();if(!live||!source)return false;
        const auto current=live->actor(live->lifetime());if(!current||!current->health||current->health->current<=0)return false;
        const bool energy_qualified=plan.energy_if_attacking&&live->attacking(live->lifetime(),now);
        auto preview=*live;const auto life=preview.lifetime();
        for(const auto& strike:*strikes){if(!preflight_npc_reversal_or_damage_locked(preview,strike,now))return false;if(preview.actor(life)->health->current<=0)break;}
        bool knock=false;
        if(plan.skill==296&&attacking&&preview.actor(life)->health->current>0){
            using K=combat::npcs::control::Knockdown;const auto result=preview.knock_down(life,now,2000);
            if(result==K::unknown||result==K::invalid)return false;knock=result==K::accepted;
        }
        // Native Bane control precedes damage. Reacquire authority after it.
        if(knock)knockdown_npc_locked(target,2000,now);
        for(const auto& strike:*strikes){
            live=live_at();placed=npcs_.find(target.actor);if(!live||placed==npcs_.end())break;
            const auto recipient=live->actor(live->lifetime());if(!recipient||!recipient->health||recipient->health->current<=0)break;
            if(!commit_npc_reversal_or_damage_locked(source_at(),placed->second,*live,strike,now,nullptr,nullptr,nullptr,false,scope))
                throw std::logic_error("admitted NPC holy conversion failed");
        }
        if(live_at()&&(plan.skill==312||plan.skill==2006||plan.skill==1689))publish_target_body_locked(target.actor,plan.skill,scope.actor,visuals);
        source=source_at();
        if(energy_qualified&&source&&source->precise_health>0){
            const auto gain=gain_energy_locked(*source,plan.energy_if_attacking,now);if(gain.changed())source->outbox->enqueue(owner_vitals(*source));
        }
        return true;
        };
        const bool accepted=execute();auto release=finish_damage_publication_locked(pending);
        if(publication&&accepted)*publication=std::move(release);else if(release)release();
        return accepted;
    }
