// Included inside Instance after both hostile controller definitions.
private:
    struct NpcWander {skills::effects::Scope source;combat::ai::wander::State state;};
    std::map<unsigned,NpcWander> npc_wander_;
    std::set<skills::effects::Scope> npc_hostile_ordinary_;
    void retire_npc_patrol_locked(skills::effects::Scope source){
        const auto motion=npc_ai_motion_.find(source.actor);
        if(motion!=npc_ai_motion_.end()&&motion->second.source==source)npc_ai_motion_.erase(motion);
        const auto owned=npc_owned_immediate_.find(source.actor);
        if(owned!=npc_owned_immediate_.end()&&owned->second.source==source)owned->second.retired=true;
        const auto immediate=npc_immediate_ai_.find(source.actor);
        if(immediate!=npc_immediate_ai_.end()&&immediate->second.source==source)npc_immediate_ai_.erase(immediate);
        const auto found=npc_wander_.find(source.actor);
        if(found!=npc_wander_.end()&&found->second.source==source)npc_wander_.erase(found);
        npc_hostile_ordinary_.erase(source);
    }
    void suspend_busy_npc_wander_locked(const combat::ai::FlareController& motion,std::uint64_t now){
        if(!motion.engaged&&!motion.returning&&!motion.cast&&(!motion.encounter||!motion.encounter->size()))return;
        const auto found=npc_wander_.find(motion.policy.source.actor);
        if(found!=npc_wander_.end()&&found->second.source==motion.policy.source)found->second.state.suspend(now);
        // Combat owns future movement from this decision onward. Suspending
        // only the patrol state leaves its retained route alive after a cast
        // or swing completes (including a coarse next poll). Chase and home
        // routes have no idle expiry and must retain their own chronology.
        const auto intent=npc_ai_motion_.find(motion.policy.source.actor);
        if(intent!=npc_ai_motion_.end()&&intent->second.source==motion.policy.source&&intent->second.expires)
            stop_npc_pursuit_locked(motion.policy.source,now);
    }
    // Called only after positive committed damage. Radius acquisition and
    // direct provocation are separate events; neither cancels a leash return.
    bool provoke_npc_locked(skills::effects::Scope source,skills::effects::Scope attacker,std::uint64_t now){
        const auto owned=npc_owned_immediate_.find(source.actor);
        if(owned!=npc_owned_immediate_.end()&&owned->second.source==source)return false;
        combat::ai::FlareController* motion=nullptr;
        if(const auto f=npc_flare_ai_.find(source.actor);f!=npc_flare_ai_.end()&&f->second.policy.source==source&&!f->second.melee_quest)motion=&f->second;
        if(const auto e=npc_elemental_ai_.find(source.actor);e!=npc_elemental_ai_.end()&&e->second.motion.policy.source==source&&!e->second.owned_quest){
            if(motion)return false;motion=&e->second.motion;
        }
        const auto live=npc_combat_.find(source.actor);const auto body=npcs_.find(source.actor);const auto player=players_.find(attacker.actor);
        if(!motion||motion->returning||now<motion->updated||live==npc_combat_.end()||body==npcs_.end()||
           live->second.hex_scope()!=source||player==players_.end()||player->second.effect_scope!=attacker||
           !player->second.npc_publication_ready||player->second.precise_health<=0||player->second.surface.plane!=body->second.plane||
           npc_elemental_player_clock(player->second)>now)return false;
        const auto actor=live->second.actor(live->second.lifetime());const auto point=healing_position_locked(player->second,now);
        if(!actor||!actor->health||actor->health->current<=0||!point||
           !live->second.hostile_to_players(live->second.lifetime())||
           std::hypot(body->second.x-motion->home_x,body->second.y-motion->home_y)>motion->policy.leash||
           std::hypot(point->x-body->second.x,point->z-body->second.y)>motion->policy.follow)return false;
        if(!motion->encounter)motion->encounter.emplace(source);
        return motion->encounter->provoke(attacker);
    }
    void idle_npc_wander_locked(combat::ai::FlareController& motion,Npc& body,std::uint64_t now){
        const auto found=npc_wander_.find(body.id);
        if(found==npc_wander_.end()||found->second.source!=motion.policy.source)return;
        auto& state=found->second.state;
        const auto live=npc_combat_.find(body.id);
        if(live==npc_combat_.end()||live->second.hex_scope()!=motion.policy.source)return;
        const auto life=live->second.lifetime();const auto actor=live->second.actor(life);
        if(motion.engaged||motion.returning||motion.cast||
           (motion.encounter&&motion.encounter->size())||!actor||!actor->health||actor->health->current<=0||
           live->second.active_action(life,now)||live->second.knocked_down(life,now)){
            state.suspend(now);return;
        }
        const auto goal=state.destination({motion.home_x,motion.home_y},now);
        if(!goal){
            // Paused/expired idle state must not leave a previous route live.
            const auto intent=npc_ai_motion_.find(body.id);
            if(intent!=npc_ai_motion_.end()&&intent->second.source==motion.policy.source&&intent->second.expires)
                stop_npc_pursuit_locked(motion.policy.source,now);
            return;
        }
        // The motion frontier already materialized arrival. Publish retirement
        // before move_npc_flare_locked can discard the completed intent; the
        // shared helper is exact-scope, clock-guarded and emits only once.
        if(std::hypot(goal->x-body.x,goal->y-body.y)<=1){
            stop_npc_pursuit_locked(motion.policy.source,now);state.finish(now);return;
        }
        const auto movement=move_npc_flare_locked(motion,body,{float(goal->x),float(goal->y)},1,now);
        using Kind=combat::ai::motion::OutcomeKind;
        // An authoritative route failure will not improve by asking for the
        // same idle waypoint every tick. Retire this leg and try the next one
        // after the configured pause. Deferrals and progressing routes retain
        // their original deadline; missing observations are not obstructions.
        if(movement||movement.kind==Kind::route_unavailable||movement.kind==Kind::unsupported_layer||
           movement.kind==Kind::invalid_request){
            stop_npc_pursuit_locked(motion.policy.source,now);state.finish(now);
        }
        else if(const auto intent=npc_ai_motion_.find(body.id);
                intent!=npc_ai_motion_.end()&&intent->second.source==motion.policy.source)
            intent->second.expires=state.leg_deadline();
    }
public:
    struct NpcAiCoverage {std::size_t placed=0,reviewed_hostiles=0,active_hostiles=0,unknown_health=0,wandering=0;};
    NpcAiCoverage npc_ai_coverage()const{
        std::lock_guard lock(mutex_);NpcAiCoverage result;result.placed=npcs_.size();
        for(const auto&[id,body]:npcs_){
            const auto live=npc_combat_.find(id);
            if(live==npc_combat_.end()){++result.unknown_health;continue;}
            const auto life=live->second.lifetime();const auto actor=live->second.actor(life);
            if(!actor||!actor->health){++result.unknown_health;continue;}
            if(!live->second.hostile_to_players(life))continue;
            ++result.reviewed_hostiles;const auto scope=live->second.hex_scope();
            const auto f=npc_flare_ai_.find(id);const auto e=npc_elemental_ai_.find(id);
            if(actor->health->current>0&&((f!=npc_flare_ai_.end()&&f->second.policy.source==scope)||(e!=npc_elemental_ai_.end()&&e->second.motion.policy.source==scope)))++result.active_hostiles;
            const auto wander=npc_wander_.find(id);if(wander!=npc_wander_.end()&&wander->second.source==scope)++result.wandering;
        }
        return result;
    }
    // Production's existing quest75 roster only: same owner/cycle, same exact
    // lifetimes, unchanged retaliation trigger. No duplicate creature spawns.
    // Offsets/timing are explicitly authored development playability policy.
    std::size_t configure_npc_owned_ordinary_wander(skills::effects::Scope owner,unsigned quest,unsigned cycle,
            combat::ai::wander::Policy policy,std::uint64_t now){
        if(!quest||!cycle)return 0;std::lock_guard lock(mutex_);
        const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner)return 0;
        const auto objective=player->second.quest_kills.find(quest);
        if(objective==player->second.quest_kills.end()||objective->second.cycle!=cycle)return 0;
        std::size_t enrolled=0;
        for(auto&[id,motion]:npc_flare_ai_){
            if(!motion.melee_quest||motion.melee_quest->owner!=owner||motion.melee_quest->cycle!=cycle||
               motion.melee_quest->quest!=quest||motion.melee_quest->retired||now<motion.updated||
               !objective->second.remaining.contains(motion.policy.source)||!policy.valid(motion.policy.leash))continue;
            const auto live=npc_combat_.find(id);const auto assignment=quest_npc_owners_.find(id);
            if(live==npc_combat_.end()||!npcs_.contains(id)||live->second.hex_scope()!=motion.policy.source||
               assignment==quest_npc_owners_.end()||assignment->second!=std::pair{owner,quest})continue;
            const auto life=live->second.lifetime();const auto actor=live->second.actor(life);
            if(!actor||!actor->health||actor->health->current<=0||now<live->second.health_clock(life).value_or(now))continue;
            const auto old=npc_wander_.find(id);
            if(old!=npc_wander_.end()&&old->second.source==motion.policy.source){++enrolled;continue;}
            npc_wander_.insert_or_assign(id,NpcWander{motion.policy.source,combat::ai::wander::State(policy,now)});++enrolled;
        }
        return enrolled;
    }
    std::size_t arm_grawl_wander(skills::effects::Scope owner,unsigned cycle,std::uint64_t now){
        return configure_npc_owned_ordinary_wander(owner,75,cycle,{{{80,0},{0,80},{-80,0},{0,-80}},1200,6000,
            "data/gw/ai/hostile-patrol-development-policy.txt"},now);
    }
    std::size_t arm_rogue_bull_wander(skills::effects::Scope owner,unsigned cycle,std::uint64_t now){
        return configure_npc_owned_ordinary_wander(owner,71,cycle,{{{80,0},{0,80},{-80,0},{0,-80}},1200,6000,
            "data/gw/quests/rogue-bull-melee-development-policy.txt"},now);
    }
    // Explicit opt-in for the existing reviewed quest49 controller only.
    // Enrollment never spawns actors or supplies missing combat definitions.
    std::size_t arm_brood_wander(skills::effects::Scope owner,unsigned cycle,std::uint64_t now){
        if(!cycle)return 0;std::lock_guard lock(mutex_);
        const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner)return 0;
        const auto objective=player->second.quest_kills.find(49);
        if(objective==player->second.quest_kills.end()||objective->second.cycle!=cycle)return 0;
        std::size_t enrolled=0;
        for(auto&[id,ai]:npc_elemental_ai_){
            auto& motion=ai.motion;
            if(!ai.owned_quest||ai.owned_quest->owner!=owner||ai.owned_quest->quest!=49||
               ai.owned_quest->cycle!=cycle||ai.owned_quest->retired||now<motion.updated||
               !objective->second.remaining.contains(motion.policy.source))continue;
            const auto live=npc_combat_.find(id);const auto assignment=quest_npc_owners_.find(id);
            if(live==npc_combat_.end()||!npcs_.contains(id)||live->second.hex_scope()!=motion.policy.source||
               assignment==quest_npc_owners_.end()||assignment->second!=std::pair{owner,49u})continue;
            const auto life=live->second.lifetime();const auto actor=live->second.actor(life);
            if(!actor||!actor->health||actor->health->current<=0||now<live->second.health_clock(life).value_or(now))continue;
            const auto old=npc_wander_.find(id);
            if(old!=npc_wander_.end()&&old->second.source==motion.policy.source){++enrolled;continue;}
            combat::ai::wander::Policy policy{{{80,0},{0,80},{-80,0},{0,-80}},1200,6000,
                "data/gw/ai/hostile-patrol-development-policy.txt"};
            if(!policy.valid(motion.policy.leash))continue;
            npc_wander_.insert_or_assign(id,NpcWander{motion.policy.source,combat::ai::wander::State(std::move(policy),now)});++enrolled;
        }
        return enrolled;
    }
    // Generic ordinary-only enemy: deliberately no synthetic spellbook or
    // Energy requirement. Existing combat content remains authoritative.
    bool configure_npc_hostile_ordinary(combat::ai::FlarePolicy policy,
            combat::ai::ordinary::Reviewed attack,std::uint64_t now){
        if(!policy.valid()||!attack.valid())return false;
        std::lock_guard lock(mutex_);
        const auto body=npcs_.find(policy.source.actor);const auto live=npc_combat_.find(policy.source.actor);
        if(body==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=policy.source||
           body->second.definition.level!=policy.level||
           quest_npc_owners_.contains(policy.source.actor)||quest_npc_motion_.contains(policy.source.actor)||
           npc_flare_ai_.contains(policy.source.actor)||npc_elemental_ai_.contains(policy.source.actor)||
           npc_ordinary_ai_.contains(policy.source.actor))return false;
        const auto life=live->second.lifetime();const auto actor=live->second.actor(life);
        if(!actor||!actor->health||actor->health->current<=0||!live->second.hostile_to_players(life)||
           now<live->second.health_clock(life).value_or(now)||live->second.active_action(life,now)||
           !live->second.ordinary_action_ready(life,now))return false;
        npc_flare_ai_.emplace(policy.source.actor,combat::ai::FlareController{policy,body->second.x,body->second.y,body->second.plane,now});
        npc_ordinary_ai_.emplace(policy.source.actor,NpcOrdinaryController{policy.source,combat::ai::ordinary::Ledger(policy.source,std::move(attack)),{}});
        npc_hostile_ordinary_.insert(policy.source);return true;
    }
    // A caller must first enroll a reviewed hostile combat controller. This
    // adds no health, armor, Energy, skillbook or allegiance defaults.
    bool configure_npc_wander(skills::effects::Scope source,combat::ai::wander::Policy policy,std::uint64_t now){
        std::lock_guard lock(mutex_);
        combat::ai::FlareController* motion=nullptr;
        if(const auto f=npc_flare_ai_.find(source.actor);f!=npc_flare_ai_.end()&&f->second.policy.source==source&&!f->second.melee_quest)motion=&f->second;
        if(const auto e=npc_elemental_ai_.find(source.actor);e!=npc_elemental_ai_.end()&&e->second.motion.policy.source==source&&!e->second.owned_quest){
            if(motion)return false;motion=&e->second.motion;
        }
        const auto live=npc_combat_.find(source.actor);
        if(!motion||!policy.valid(motion->policy.leash)||live==npc_combat_.end()||live->second.hex_scope()!=source||
           !live->second.hostile_to_players(live->second.lifetime())||now<motion->updated||
           quest_npc_owners_.contains(source.actor)||quest_npc_motion_.contains(source.actor))return false;
        npc_wander_.insert_or_assign(source.actor,NpcWander{source,combat::ai::wander::State(std::move(policy),now)});return true;
    }
