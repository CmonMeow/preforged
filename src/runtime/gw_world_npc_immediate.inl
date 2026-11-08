// Included inside Instance. Shared hostile motion lives in npc_flare_ai_.
private:
    struct NpcImmediateController {
        skills::effects::Scope source;
        std::vector<combat::ai::activated::Reviewed> book;
        struct Pending {combat::npcs::control::Token token;skills::effects::Scope target;combat::ai::activated::Reviewed reviewed;};
        std::optional<Pending> pending;
    };
    std::map<unsigned,NpcImmediateController> npc_immediate_ai_;
    bool finish_npc_immediate_locked(combat::ai::FlareController& motion,std::uint64_t now){
        const auto controller=npc_immediate_ai_.find(motion.policy.source.actor);
        if(controller==npc_immediate_ai_.end()||controller->second.source!=motion.policy.source||!controller->second.pending)return false;
        const auto pending=*controller->second.pending;const auto source=pending.token.actor;
        const auto live=npc_combat_.find(source.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=source||
           live->second.control_epoch(live->second.lifetime())!=pending.token.epoch||
           !live->second.owns_activated_skill_action(live->second.lifetime(),pending.token)){
            controller->second.pending.reset();motion.cast.reset();npc_flare_event_locked(59,source.actor);return true;
        }
        if(now<pending.token.completes)return true;
        DamagePublication publication;begin_damage_publication_locked(publication);
        const auto result=complete_npc_immediate_damage_locked(pending.token,pending.target,pending.reviewed,now);
        auto release=finish_damage_publication_locked(publication);
        if(result==combat::ai::activated::Completion::deferred){if(release)release();return true;}
        const auto current=npc_immediate_ai_.find(source.actor);
        if(current!=npc_immediate_ai_.end()&&current->second.source==source&&current->second.pending&&
           current->second.pending->token==pending.token)current->second.pending.reset();
        motion.cast.reset();
        if(result==combat::ai::activated::Completion::stopped&&release){release();release={};}
        npc_flare_event_locked(result==combat::ai::activated::Completion::completed?58:59,source.actor);
        if(release)release();
        return true;
    }
    bool begin_npc_immediate_locked(combat::ai::FlareController& motion,Npc& body,Player& target,std::uint64_t now){
        const auto source=motion.policy.source;
        const auto target_scope=target.effect_scope;
        if(const auto old=npc_immediate_ai_.find(source.actor);
           old==npc_immediate_ai_.end()||old->second.source!=source)return false;
        if(!settle_mind_wrack_caster_locked(source,now))return true;
        // Caster Health settlement can kill a linked NPC and retire its book.
        // Reacquire the controller and exact participants before retaining aliases.
        const auto controller=npc_immediate_ai_.find(source.actor);
        auto live=npc_combat_.find(source.actor);const auto placed=npcs_.find(source.actor);
        const auto victim=players_.find(target_scope.actor);
        if(controller==npc_immediate_ai_.end()||controller->second.source!=source||
           live==npc_combat_.end()||live->second.hex_scope()!=source||placed==npcs_.end()||
           victim==players_.end()||victim->second.effect_scope!=target_scope)return true;
        auto& actor=live->second;const auto life=actor.lifetime();
        const auto alive=actor.actor(life);
        if(!alive||!alive->health||alive->health->current<=0)return true;
        const auto snapshot=ordinary_npc_snapshot_locked(target,body,actor,now);
        const auto point=healing_position_locked(target,now);if(!point)return true;
        combat::ai::SpellTarget facts{target.effect_scope,float(snapshot.distance),target.precise_health>0,true,
            target.surface.plane==body.plane,snapshot.line_of_sight};
        const combat::ai::Spellbook::Availability ready=[&](skills::Slot slot,std::uint64_t at){return skill_disabled_slot_locked(source,slot,at);};
        const auto selected=actor.choose_activated_skill(life,facts,now,ready);
        if(!selected){
            // An explicitly configured ordinary profile remains usable while
            // the paid skill is recharging or unaffordable. This path owns its
            // attack/movement decision and never falls through to Flare194.
            if(begin_npc_ordinary_locked(motion,body,actor,target,now))return true;
            auto approach=facts;approach.distance=0;approach.line_of_sight=true;
            const auto next=actor.choose_activated_skill(life,approach,now,ready);
            if(next){const auto entry=std::find_if(controller->second.book.begin(),controller->second.book.end(),[&](const auto& item){return item.slot==*next;});
                if(entry!=controller->second.book.end())move_npc_flare_locked(motion,body,*point,snapshot.line_of_sight?entry->effect_range():0,now);
            }else if(!controller->second.book.empty()){
                // Recharge/low Energy need not freeze pursuit. This is only
                // geometry; LiveActor still gates payment, and its movement
                // history excludes windup/aftercast from the interval budget.
                move_npc_flare_locked(motion,body,*point,snapshot.line_of_sight?controller->second.book.front().effect_range():0,now);
            }
            return true;
        }
        const auto entry=std::find_if(controller->second.book.begin(),controller->second.book.end(),[&](const auto& item){return item.slot==*selected;});
        if(entry==controller->second.book.end())return true;
        stop_npc_pursuit_locked(source,now);
        const auto reviewed=*entry;const auto before=actor.energy(life);const auto units=actor.precise_energy_units(life);
        const auto token=actor.begin_activated_skill(life,*selected,facts,now,ready);if(!token)return true;
        controller->second.pending=NpcImmediateController::Pending{*token,target.effect_scope,reviewed};motion.cast=*token;
        if(before)observe_npc_mind_wrack_energy_locked(source,{before->current,before->maximum,0,units?unsigned(*units%300000):0},now);
        live=npc_combat_.find(source.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=source||live->second.control_epoch(life)!=token->epoch){
            const auto current=npc_immediate_ai_.find(source.actor);
            if(current!=npc_immediate_ai_.end()&&current->second.source==source&&current->second.pending&&
               current->second.pending->token==*token)current->second.pending.reset();
            motion.cast.reset();return true;
        }
        npc_flare_event_locked(60,source.actor,selected->skill,target.id);return true;
    }
public:
    bool configure_npc_immediate_ai(combat::ai::FlarePolicy policy,
            std::vector<combat::ai::activated::Reviewed> book,std::uint64_t now){
        if(!policy.valid()||book.empty()||book.size()>8||std::any_of(book.begin(),book.end(),[](const auto& item){return !item.valid();}))return false;
        std::lock_guard lock(mutex_);const auto placed=npcs_.find(policy.source.actor);const auto live=npc_combat_.find(policy.source.actor);
        if(placed==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=policy.source||
           placed->second.definition.level!=policy.level||quest_npc_owners_.contains(policy.source.actor)||
           quest_npc_motion_.contains(policy.source.actor)||npc_flare_ai_.contains(policy.source.actor)||
           npc_elemental_ai_.contains(policy.source.actor)||npc_ordinary_ai_.contains(policy.source.actor))return false;
        const auto life=live->second.lifetime();const auto body=live->second.actor(life);
        if(!body||!body->health||body->health->current<=0||!live->second.hostile_to_players(life)||
           !live->second.energy(life)||now<live->second.health_clock(life).value_or(now)||
           !live->second.ordinary_action_ready(life,now)||live->second.active_action(life,now))return false;
        auto proposed=live->second;std::vector<combat::ai::ReviewedSpell> reviewed;
        for(const auto& item:book)reviewed.push_back({item.slot,item.effect_range(),item.evidence,
            combat::ai::SpellTargetClass::reviewed,*combat::ai::activated::kind(item.slot.skill)});
        if(!proposed.advance_energy(life,now)||!proposed.configure_activated_skillbook(life,std::move(reviewed)))return false;
        live->second=std::move(proposed);
        npc_immediate_ai_.insert_or_assign(policy.source.actor,NpcImmediateController{policy.source,std::move(book),{}});
        npc_flare_ai_.emplace(policy.source.actor,combat::ai::FlareController{policy,placed->second.x,placed->second.y,placed->second.plane,now});return true;
    }
