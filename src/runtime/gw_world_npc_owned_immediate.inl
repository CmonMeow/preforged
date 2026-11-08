// Included inside Instance after the immediate and wander helpers.
// Owned quest actors reuse typed activation; no proximity acquisition or stats.
private:
    struct NpcOwnedImmediate {
        skills::effects::Scope source,owner;
        unsigned quest=0,cycle=0;
        std::uint64_t loss_revision=0;
        bool triggered=false,retired=false;
    };
    std::map<unsigned,NpcOwnedImmediate> npc_owned_immediate_;
    bool npc_owned_immediate_valid_locked(const NpcOwnedImmediate& binding)const{
        const auto owner=players_.find(binding.owner.actor);
        const auto assignment=quest_npc_owners_.find(binding.source.actor);
        if(binding.retired||owner==players_.end()||owner->second.effect_scope!=binding.owner||
           assignment==quest_npc_owners_.end()||assignment->second!=std::pair{binding.owner,binding.quest})return false;
        const auto objective=owner->second.quest_kills.find(binding.quest);
        return objective!=owner->second.quest_kills.end()&&objective->second.cycle==binding.cycle&&
            objective->second.remaining.contains(binding.source);
    }
    bool configure_npc_owned_immediate_locked(skills::effects::Scope owner,unsigned quest,unsigned cycle,
            combat::ai::FlarePolicy policy,std::vector<combat::ai::activated::Reviewed> book,
            combat::ai::wander::Policy patrol,std::uint64_t now){
        NpcOwnedImmediate binding{policy.source,owner,quest,cycle};
        if(!quest||!cycle||policy.target!=owner||!policy.valid()||!patrol.valid(policy.leash)||
           book.empty()||book.size()>8||!npc_owned_immediate_valid_locked(binding)||
           std::any_of(book.begin(),book.end(),[](const auto& entry){return !entry.valid();}))return false;
        const auto placed=npcs_.find(policy.source.actor);const auto live=npc_combat_.find(policy.source.actor);
        if(placed==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=policy.source||
           placed->second.definition.level!=policy.level)return false;
        const auto life=live->second.lifetime();const auto state=live->second.actor(life);
        if(!state||!state->health||state->health->current<=0||!live->second.hostile_to_players(life)||
           !live->second.energy(life)||now<live->second.health_clock(life).value_or(now)||
           now<live->second.energy_clock(life).value_or(now))return false;
        const auto old=npc_owned_immediate_.find(policy.source.actor);
        if(old!=npc_owned_immediate_.end()){
            const auto motion=npc_flare_ai_.find(policy.source.actor);const auto immediate=npc_immediate_ai_.find(policy.source.actor);
            return !old->second.retired&&old->second.source==policy.source&&old->second.owner==owner&&
                old->second.quest==quest&&old->second.cycle==cycle&&motion!=npc_flare_ai_.end()&&
                motion->second.policy.source==policy.source&&immediate!=npc_immediate_ai_.end()&&immediate->second.source==policy.source;
        }
        if(npc_flare_ai_.contains(policy.source.actor)||npc_elemental_ai_.contains(policy.source.actor)||
           npc_ordinary_ai_.contains(policy.source.actor)||quest_npc_motion_.contains(policy.source.actor)||
           live->second.active_action(life,now)||!live->second.ordinary_action_ready(life,now))return false;
        auto proposed=live->second;std::vector<combat::ai::ReviewedSpell> reviewed;
        for(const auto& item:book)reviewed.push_back({item.slot,item.effect_range(),item.evidence,
            combat::ai::SpellTargetClass::reviewed,*combat::ai::activated::kind(item.slot.skill)});
        if(!proposed.advance_energy(life,now)||!proposed.configure_activated_skillbook(life,std::move(reviewed)))return false;
        binding.loss_revision=proposed.health_loss_revision(life).value_or(0);
        binding.triggered=binding.loss_revision!=0; // Damage before the host enrollment poll still retaliates.
        live->second=std::move(proposed);
        npc_immediate_ai_.emplace(policy.source.actor,NpcImmediateController{policy.source,std::move(book),{}});
        npc_flare_ai_.emplace(policy.source.actor,combat::ai::FlareController{policy,placed->second.x,placed->second.y,placed->second.plane,now});
        npc_wander_.insert_or_assign(policy.source.actor,NpcWander{policy.source,combat::ai::wander::State(std::move(patrol),now)});
        npc_owned_immediate_.emplace(policy.source.actor,binding);return true;
    }
    // Intercept before generic immediate completion/acquisition. The caller
    // must not touch actor aliases after this hook: paid reactions can retire.
    bool drive_npc_owned_immediate_locked(combat::ai::FlareController& motion,std::uint64_t now){
        const auto found=npc_owned_immediate_.find(motion.policy.source.actor);
        if(found==npc_owned_immediate_.end()||found->second.source!=motion.policy.source)return false;
        auto& binding=found->second;const auto source=binding.source;
        const auto live=npc_combat_.find(source.actor);const auto placed=npcs_.find(source.actor);
        if(binding.retired){motion.updated=now;return true;}
        if(live==npc_combat_.end()||live->second.hex_scope()!=source||placed==npcs_.end())return true;
        auto& actor=live->second;auto& body=placed->second;const auto life=actor.lifetime();
        // Identity/quest invalidation precedes former-owner observation clocks.
        if(!npc_owned_immediate_valid_locked(binding)){
            if(actor.active_action(life,now)&&!actor.cancel_action(life,now))return true;
            if(motion.cast)npc_flare_event_locked(59,source.actor);
            motion.cast.reset();motion.engaged=false;motion.returning=false;motion.encounter.reset();
            binding.retired=true;retire_npc_patrol_locked(source);motion.updated=now;return true;
        }
        if(now<actor.health_clock(life).value_or(now)||now<actor.energy_clock(life).value_or(now))return true;
        const auto state=actor.actor(life);
        if(!state||!state->health||state->health->current<=0){
            binding.retired=true;motion.engaged=false;motion.cast.reset();retire_npc_patrol_locked(source);motion.updated=now;return true;
        }
        const auto loss=actor.health_loss_revision(life).value_or(binding.loss_revision);
        if(loss!=binding.loss_revision)binding.triggered=true;
        binding.loss_revision=loss;
        // Completion retains existing paid skill/interrupt/Seed authority.
        if(finish_npc_immediate_locked(motion,now)){motion.updated=now;return true;}
        if(actor.knocked_down(life,now)||actor.active_action(life,now)){motion.updated=now;return true;}
        if(motion.returning){
            if(move_npc_flare_locked(motion,body,{motion.home_x,motion.home_y},1,now)){
                motion.returning=false;motion.engaged=false;binding.triggered=false;
            }
            motion.updated=now;return true;
        }
        const auto owner=players_.find(binding.owner.actor);
        if(npc_elemental_player_clock(owner->second)>now){motion.updated=now;return true;}
        if(!binding.triggered){idle_npc_wander_locked(motion,body,now);motion.updated=now;return true;}
        const auto point=healing_position_locked(owner->second,now);
        const bool target=point&&owner->second.precise_health>0&&owner->second.npc_publication_ready&&
            owner->second.surface.plane==body.plane&&std::hypot(point->x-body.x,point->z-body.y)<=motion.policy.follow&&
            std::hypot(body.x-motion.home_x,body.y-motion.home_y)<=motion.policy.leash;
        if(!target){motion.engaged=false;motion.returning=true;suspend_busy_npc_wander_locked(motion,now);motion.updated=now;return true;}
        motion.engaged=true;motion.policy.target=binding.owner;suspend_busy_npc_wander_locked(motion,now);
        // Keep previous update time available to routed movement budgeting.
        begin_npc_immediate_locked(motion,body,owner->second,now);motion.updated=now;return true;
    }
public:
    bool configure_npc_owned_immediate(skills::effects::Scope owner,unsigned quest,unsigned cycle,
            combat::ai::FlarePolicy policy,std::vector<combat::ai::activated::Reviewed> book,
            combat::ai::wander::Policy patrol,std::uint64_t now){
        std::lock_guard lock(mutex_);
        return configure_npc_owned_immediate_locked(owner,quest,cycle,std::move(policy),std::move(book),std::move(patrol),now);
    }
    struct NpcOwnedImmediateEnrollment {std::size_t remaining=0,armed=0,unsupported=0;bool owner_valid=false;};
    NpcOwnedImmediateEnrollment arm_blood_fanatic_immediate(skills::effects::Scope owner,unsigned cycle,std::uint64_t now){
        std::lock_guard lock(mutex_);NpcOwnedImmediateEnrollment report;
        const auto player=players_.find(owner.actor);if(!cycle||player==players_.end()||player->second.effect_scope!=owner)return report;
        const auto objective=player->second.quest_kills.find(43);
        if(objective==player->second.quest_kills.end()||objective->second.cycle!=cycle)return report;
        report.owner_valid=true;report.remaining=objective->second.remaining.size();
        for(const auto source:objective->second.remaining){
            const auto placed=npcs_.find(source.actor);const auto live=npc_combat_.find(source.actor);
            bool supported=placed!=npcs_.end()&&live!=npc_combat_.end()&&live->second.hex_scope()==source&&live->second.lifetime().map==145;
            if(supported){const auto& d=placed->second.combat_definition;
                // spawn_kill_encounter remaps template definition70 to the
                // scoped runtime actor ID; source catalog checks stay in host.
                supported=d&&d->binding.definition==source.actor&&d->binding.model_file==116378&&d->binding.level==2&&
                    d->spawn_health&&d->spawn_health->current==120&&d->spawn_health->maximum==120&&
                    d->spawn_energy&&d->spawn_energy->current==40&&d->spawn_energy->maximum==40&&
                    d->energy_pips==std::optional(4u);
            }
            if(!supported){++report.unsupported;continue;}
            combat::ai::FlarePolicy policy;policy.source=source;policy.target=owner;policy.level=2;
            policy.speed=220;policy.acquire=1000;policy.follow=1500;policy.leash=2000;policy.flight={1200,32};
            policy.evidence="data/gw/quests/power-of-blood-immediate-development-policy.txt";
            const auto plan=skills::effects::shadow_damage(155,0);
            if(plan&&configure_npc_owned_immediate_locked(owner,43,cycle,policy,{{{155,0},*plan,policy.evidence}},
                {{{80,0},{0,80},{-80,0},{0,-80}},1200,6000,policy.evidence},now))++report.armed;
            else ++report.unsupported;
        }
        return report;
    }
