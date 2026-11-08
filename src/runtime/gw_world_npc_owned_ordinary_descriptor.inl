// Include after owned ordinary and wander helpers. Descriptor header is included
// at namespace scope by World. Never creates NPCs or changes quest objectives.
private:
    std::size_t arm_reviewed_owned_ordinary_locked(skills::effects::Scope owner,unsigned quest,unsigned cycle,std::uint64_t now){
        namespace reviewed=combat::ai::owned_ordinary;
        const auto descriptor=reviewed::captured_tads(quest);if(!descriptor)return 0;
        const auto& d=*descriptor;const auto& p=d.profile;
        const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner)return 0;
        const auto found=player->second.quest_kills.find(quest);
        if(found==player->second.quest_kills.end())return 0;
        const auto& objective=found->second;
        // Loot/wave/timed/partial-credit objectives must not silently acquire
        // ordinary kill-all semantics through this descriptor.
        if(player->second.quest_loot.contains(quest)||player->second.quest_waves.contains(quest)||
           objective.timed||objective.required||objective.restored_credits)return 0;
        reviewed::Objective observation{owner,quest,objective.cycle,d.map,
            objective.any_death?reviewed::ObjectiveKind::kill_any:reviewed::ObjectiveKind::kill_all,
            objective.assigned,objective.remaining};
        std::vector<reviewed::Member> facts;facts.reserve(objective.remaining.size());
        combat::ai::wander::Policy patrol{{{float(p.patrol_radius),0},{0,float(p.patrol_radius)},
            {-float(p.patrol_radius),0},{0,-float(p.patrol_radius)}},p.pause,p.leg_timeout,p.evidence};
        for(const auto source:objective.remaining){
            const auto placed=npcs_.find(source.actor);const auto live=npc_combat_.find(source.actor);
            const auto assignment=quest_npc_owners_.find(source.actor);
            if(placed==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=source||
               assignment==quest_npc_owners_.end()||!placed->second.combat_definition)return 0;
            const auto& def=*placed->second.combat_definition;const auto life=live->second.lifetime();
            const auto actor=live->second.actor(life);if(!actor||!actor->health)return 0;
            if(def.binding.actor!=source.actor||def.binding.level!=placed->second.definition.level||
               def.binding.model_file!=placed->second.definition.model_file||
               def.binding.definition_flags!=placed->second.definition.flags||def.binding.skin_file||def.binding.appearance||
               !def.spawn_health||def.spawn_health->maximum!=d.maximum_health)return 0;
            reviewed::Member m;m.source=source;m.assigned_owner=assignment->second.first;m.assigned_quest=assignment->second.second;
            m.assigned_cycle=objective.cycle;m.map=life.map;m.actor=def.binding.actor;m.definition=def.binding.definition;
            m.model=def.binding.model_file;m.flags=def.binding.definition_flags;m.level=def.binding.level;
            m.maximum_health=actor->health->maximum;m.current_health=actor->health->current;
            m.armor={def.physical_armor,def.earth_armor,def.fire_armor,def.cold_armor,def.lightning_armor,def.general_armor};
            m.hostile=live->second.hostile_to_players(life);m.health_clock=live->second.health_clock(life).value_or(now);
            const auto motion=npc_flare_ai_.find(source.actor);const auto ordinary=npc_ordinary_ai_.find(source.actor);
            const auto wandering=npc_wander_.find(source.actor);
            if(npc_elemental_ai_.contains(source.actor)||npc_immediate_ai_.contains(source.actor)||quest_npc_motion_.contains(source.actor))return 0;
            if(motion!=npc_flare_ai_.end()){
                const auto& ai=motion->second;const auto& binding=ai.melee_quest;
                if(!binding||binding->retired||binding->acquisition!=combat::ai::FlareController::OwnedAcquisition::damage_only||
                   ordinary==npc_ordinary_ai_.end()||ordinary->second.source!=source)return 0;
                const auto& a=ordinary->second.ledger.profile();const auto& q=ai.policy;
                if(q.target!=owner||q.level!=d.level||q.fire_magic||q.max_flights!=8||q.flight.speed!=100||q.flight.hit_radius!=32||
                   a.minimum!=a.maximum||a.caster_weapon||a.delivery!=combat::ai::ordinary::Delivery::melee||
                   a.flight.speed!=100||a.flight.hit_radius!=32||a.evidence!=q.evidence)return 0;
                m.controller_present=m.controller_owned=true;m.controller_source=q.source;m.controller_owner=binding->owner;
                m.controller_quest=binding->quest;m.controller_cycle=binding->cycle;m.controller_clock=ai.updated;
                reviewed::Profile installed;installed.damage=a.minimum;installed.mastery=a.mastery;installed.damage_type=a.damage_type;
                installed.windup=a.windup_ms;installed.period=a.period_ms;installed.reach=a.range;
                installed.speed=q.speed;installed.acquire=q.acquire;installed.follow=q.follow;installed.leash=q.leash;installed.evidence=q.evidence;
                m.installed_profile=installed;
            }else if(ordinary!=npc_ordinary_ai_.end()||live->second.active_action(life,now)||!live->second.ordinary_action_ready(life,now))return 0;
            if(wandering!=npc_wander_.end()){
                if(wandering->second.source!=source)return 0;const auto& old=wandering->second.state.policy();
                if(old.pause_ms!=patrol.pause_ms||old.leg_timeout_ms!=patrol.leg_timeout_ms||old.evidence!=patrol.evidence||
                   old.offsets.size()!=patrol.offsets.size())return 0;
                for(std::size_t i=0;i<old.offsets.size();++i)if(old.offsets[i].x!=patrol.offsets[i].x||old.offsets[i].y!=patrol.offsets[i].y)return 0;
            }
            facts.push_back(std::move(m));
        }
        if(!reviewed::validate(d,observation,owner,cycle,facts,now))return 0;
        // All refusing gates are checked above under the same lock; no callbacks
        // occur while installing controllers. Existing actions/Health stay intact.
        for(const auto& m:facts){
            combat::ai::FlarePolicy policy;policy.source=m.source;policy.target=owner;policy.level=d.level;
            policy.speed=float(p.speed);policy.acquire=float(p.acquire);policy.follow=float(p.follow);policy.leash=float(p.leash);
            policy.flight={100,32};policy.evidence=p.evidence;
            combat::ai::ordinary::Reviewed attack{p.damage,p.damage,p.mastery,p.damage_type,false,combat::ai::ordinary::Delivery::melee,
                p.windup,p.period,p.reach,{100,32},p.evidence};
            if(!configure_npc_owned_ordinary_locked(owner,quest,cycle,std::move(policy),std::move(attack),
                combat::ai::FlareController::OwnedAcquisition::damage_only,now))throw std::logic_error("owned ordinary admission changed without callback");
            if(!npc_wander_.contains(m.source.actor))npc_wander_.emplace(m.source.actor,NpcWander{m.source,combat::ai::wander::State(patrol,now)});
        }
        return facts.size();
    }
public:
    std::size_t arm_reviewed_owned_ordinary(skills::effects::Scope owner,unsigned quest,unsigned cycle,std::uint64_t now){
        std::lock_guard lock(mutex_);return arm_reviewed_owned_ordinary_locked(owner,quest,cycle,now);
    }
