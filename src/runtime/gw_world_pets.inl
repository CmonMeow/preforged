// Default policy supplied by gw_pet_default_ai.h included by World.
// Include inside Instance after NPC/action/motion authorities. Top-level header
// must include gw_pet_lifecycle.h and gw_pet_wire.h. Not production-bound alone.
private:
    struct PlayerPetSession {skills::effects::Scope owner;std::uint64_t revision=0;pets::Profile durable;std::optional<skills::effects::Scope> actor;unsigned owner_level=0;std::optional<bool> pvp;};
    std::map<unsigned,PlayerPetSession> player_pet_sessions_;
    struct PetOwnerCombat {skills::effects::Scope owner;unsigned level=0,mastery=0;std::uint64_t observed=0;};
    std::map<unsigned,PetOwnerCombat> pet_owner_combat_;

    std::map<skills::effects::Scope,combat::npcs::Binding> tameable_stalkers_;
    struct TameableStalkerCombat {combat::ai::FlarePolicy motion;combat::ai::ordinary::Reviewed attack;combat::ai::wander::Policy patrol;std::optional<combat::ai::FlareController> idle;};
    std::map<skills::effects::Scope,TameableStalkerCombat> tameable_stalker_combat_;
    pets::CharmLedger pet_charms_;
    struct PetLayeredMotion {
        navigation::Surface surface;std::vector<navigation::SurfacePoint> route;
        navigation::SurfacePoint goal;std::uint64_t updated=0;float stop=0;
    };
    struct PetAiController {
        skills::effects::Scope owner,source;
        ai::companion::Controller behavior{ai::companion::Profile::ranger_pet};
        ai::companion::Config config;
        combat::ai::FlareController motion;
        std::optional<ai::companion::Acquisition> acquisition;
        pets::wire::Behavior mode=pets::wire::Behavior::guard;
        std::optional<skills::effects::Scope> locked;
        std::optional<PetLayeredMotion> layered_motion;
        std::uint64_t offensive_completion=0;
    };
    std::map<unsigned,PetAiController> pet_ai_;
    std::map<skills::effects::Scope,std::uint64_t> pet_comfort_completed_;
    static std::u16string pet_encoded_name(const pets::Pet& pet){
        if(!pet.name.empty()){std::u16string name{u'\x108',u'\x107'};for(const auto c:pet.name)name.push_back(char16_t(c));name.push_back(u'\x1');return name;}
        return {u'\x108',u'\x10a',u'\xf0d',u'\xa74a',u'\xbbf9',u'\x249a',u'\x1'};
    }
    std::optional<PetAiController> prepare_default_pet_ai_locked(skills::effects::Scope owner,
            skills::effects::Scope pet,const pets::Pet& profile,const Npc& body,std::uint64_t now)const{
        const auto stats=pet_owner_combat_.find(owner.actor);const auto session=player_pet_sessions_.find(owner.actor);
        const auto policy=pets::default_stalker_ai(pet,profile);
        if(!policy||stats==pet_owner_combat_.end()||stats->second.owner!=owner||stats->second.observed>now||
           session==player_pet_sessions_.end()||session->second.owner!=owner||!session->second.pvp.has_value())return {};
        PetAiController ai;ai.owner=owner;ai.source=pet;ai.config=policy->behavior;
        ai.motion={policy->motion,body.x,body.y,body.plane,now};return ai;
    }
    static std::optional<Npc> owned_stalker_template(unsigned map,unsigned actor,unsigned definition,
            const pets::Pet& pet,std::uint32_t team,navigation::Point position,std::uint16_t plane){
        const auto stats=pets::base_stats(pet);if(!stats||!map||map>65535||!actor||actor>65535||!definition||definition>65535||!team)return {};
        Npc body;body.id=actor;body.definition.id=definition;body.definition.model_file=128406;body.definition.skin_file=0;
        body.definition.visual_adjustment={0,0,0,100};body.definition.appearance=0;body.definition.flags=13;
        body.definition.primary=2;body.definition.level=std::uint8_t(pet.level);
        body.definition.encoded_name={u'\xf0d',u'\xa74a',u'\xbbf9',u'\x249a'};
        body.model_files={129924};body.x=position.x;body.y=position.z;body.plane=plane;body.captured_name="Pet - Melandru's Stalker";
        if(!pet.name.empty())body.literal_display_name=std::u16string(pet.name.begin(),pet.name.end());
        combat::npcs::Definition metadata;metadata.binding={map,actor,definition,128406,0,0,13,std::uint8_t(pet.level)};
        metadata.relation=combat::npcs::Relation::allied_to_players;metadata.packed_allegiance=team;
        metadata.spawn_health=combat::Health{double(stats->maximum_health),stats->maximum_health};
        metadata.physical_armor=metadata.general_armor=metadata.earth_armor=metadata.fire_armor=metadata.cold_armor=metadata.lightning_armor=int(stats->armor);
        metadata.damageable=true;metadata.conditions_allowed=true;
        const auto traits=pets::reviewed_species_traits(pet.species);if(!traits)return {};
        metadata.fleshy=traits->fleshy;metadata.fleshy_evidence=traits->evidence;
        metadata.healing_allowed=true;metadata.healing_evidence="Comfort_Animal local wiki and four-profession healing31131";
        metadata.evidence={"owned persistent pet","Animal_companion441 and exactlevel5 max180 packet16408","Animal_companion441","private owner team token","animal companion combat conditions"};
        if(pet.level==5){metadata.spawn_energy=combat::npcs::Energy{30,30};metadata.energy_evidence="exactlevel5 pet181 packet16407";}
        body.combat_definition=std::move(metadata);return body;
    }
    void remove_world_pet_locked(PlayerPetSession& session,std::uint64_t now){
        if(!session.actor)return;const auto pet=*session.actor;
        if(valid_pet_owner_locked(session.owner,pet,false)){
            for(auto& [id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue({pets::wire::remove(pet.actor)});
            cancel_pet_pending_locked(pet,now);stop_pet_motion_locked(pet,now);retire_npc_ordinary_locked(pet,now);
            npc_friendly_ordinary_.erase(pet.actor);pet_ai_.erase(pet.actor);pet_charms_.retire(pet);
            // Pet actor is enrolled under quest0 solely to reuse the exact
            // scoped NPC retirement/credit/projectile/visibility machinery.
            retire_quest_npcs_locked(session.owner,0u,pet.actor);
        }
        session.actor.reset();
    }
    std::vector<Packet> pet_entry_packets_locked(const std::set<std::uint32_t>* only=nullptr)const{
        std::vector<Packet> packets;
        for(const auto& [id,session]:player_pet_sessions_){
            if(!session.actor||!session.durable.pet||(only&&!only->contains(session.actor->actor))||
               !valid_pet_owner_locked(session.owner,*session.actor,false))continue;
            const auto ai=pet_ai_.find(session.actor->actor);
            const auto mode=ai!=pet_ai_.end()&&ai->second.source==*session.actor?ai->second.mode:pets::wire::Behavior::guard;
            packets.push_back(pets::wire::add(session.actor->actor,session.owner.actor,pet_encoded_name(*session.durable.pet),128406,129924,mode));
            if(ai!=pet_ai_.end()&&ai->second.source==*session.actor&&ai->second.locked)
                packets.push_back(pets::wire::locked_target(session.actor->actor,ai->second.locked->actor));
        }
        return packets;
    }
    static ai::companion::Actor pet_actor_key(skills::effects::Scope s){return {s.instance,s.generation,s.actor};}
    static skills::effects::Scope pet_scope_key(ai::companion::Actor s){return {s.instance,s.id,s.generation};}
    bool valid_pet_owner_locked(skills::effects::Scope owner,skills::effects::Scope pet,bool alive=true)const{
        const auto player=players_.find(owner.actor);const auto session=player_pet_sessions_.find(owner.actor);
        const auto membership=pet_owners_.find(pet);const auto live=npc_combat_.find(pet.actor);
        if(player==players_.end()||player->second.effect_scope!=owner||session==player_pet_sessions_.end()||
           session->second.owner!=owner||session->second.actor!=pet||!session->second.durable.pet||
           membership==pet_owners_.end()||membership->second!=owner||live==npc_combat_.end()||live->second.hex_scope()!=pet)return false;
        const auto* actor=live->second.actor(live->second.lifetime());
        return actor&&actor->damageable&&actor->health&&(!alive||actor->health->current>0);
    }
    // Parent must call this only from the pet branch of the shared friendly
    // ordinary authority. Other NPC quest isolation retains its existing gates.
    std::optional<double> pet_ordinary_damage_locked(skills::effects::Scope pet,unsigned target_level,
            int armor,bool weakness,bool critical_immune,std::uint64_t now){
        const auto membership=pet_owners_.find(pet);if(membership==pet_owners_.end())return {};
        const auto stats=pet_owner_combat_.find(membership->second.actor);
        const auto session=player_pet_sessions_.find(membership->second.actor);
        const auto live=npc_combat_.find(pet.actor);
        if(stats==pet_owner_combat_.end()||stats->second.owner!=membership->second||stats->second.observed>now||
           session==player_pet_sessions_.end()||session->second.owner!=membership->second||!session->second.durable.pet||!session->second.pvp.has_value()||
           live==npc_combat_.end()||live->second.hex_scope()!=pet||
           live->second.owned_stalker_owner(live->second.lifetime())!=std::optional(membership->second))return {};
        // Reuse the reviewed community critical-chance approximation, with pet
        // level/BM, no item penalty or Assassin attribute. Pet damage itself
        // retains its variable roll rather than weapon maximum-critical rules.
        const auto chance=combat::weapon::critical_chance({2,0,session->second.durable.pet->level},target_level,stats->second.mastery,0.);
        if(!chance)return {};
        const bool critical=sample_attack_block_locked()<*chance;
        const pets::combat_model::Strike strike{{pet,membership->second,!*session->second.pvp},pet,*session->second.durable.pet,
            stats->second.level,stats->second.mastery,armor,sample_attack_block_locked(),critical,critical_immune,weakness};
        return pets::combat_model::ordinary(strike);
    }
    bool queue_pet_acquisition_locked(skills::effects::Scope owner,skills::effects::Scope target,
            ai::companion::Trigger trigger,std::uint64_t now){
        const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner||player->second.precise_health<=0)return false;

        const auto session=player_pet_sessions_.find(owner.actor);
        if(session==player_pet_sessions_.end()||session->second.owner!=owner||!session->second.actor)return false;
        const auto pet=*session->second.actor;const auto controller=pet_ai_.find(pet.actor);
        if(controller==pet_ai_.end()||controller->second.source!=pet||controller->second.owner!=owner||
           controller->second.mode==pets::wire::Behavior::avoid_combat||
           !valid_pet_ordinary_locked(pet,target))return false;
        // A released action may be delivered after this tick's pet decision.
        // Queue for the next decision; never rewind pet motion/action clocks.
        controller->second.acquisition=ai::companion::Acquisition{pet_actor_key(target),trigger,true};return true;
    }
    void observe_pet_attack_release_locked(skills::effects::Scope source,skills::effects::Scope target,std::uint64_t now){
        if(const auto owner=players_.find(source.actor);owner!=players_.end()&&owner->second.effect_scope==source)
            queue_pet_acquisition_locked(source,target,ai::companion::Trigger::owner_attack,now);
        const auto owner=players_.find(target.actor);const auto attacker=npc_combat_.find(source.actor);const auto body=npcs_.find(source.actor);
        if(owner==players_.end()||owner->second.effect_scope!=target||attacker==npc_combat_.end()||
           attacker->second.hex_scope()!=source||!attacker->second.hostile_to_players(attacker->second.lifetime())||body==npcs_.end()||
           owner->second.surface.plane!=body->second.plane)return;
        const auto position=healing_position_locked(owner->second,now);
        if(position&&std::hypot(position->x-body->second.x,position->z-body->second.y)<=166.)
            queue_pet_acquisition_locked(target,source,ai::companion::Trigger::adjacent_owner_threat,now);
    }
    // Owner death preserves ownership and the pet's retained combat. The
    // acquisition/command entry points independently require a living initiator.
    bool valid_pet_ordinary_locked(skills::effects::Scope pet,skills::effects::Scope target,bool require_alive=true)const{
        const auto member=pet_owners_.find(pet);if(member==pet_owners_.end())return false;
        const auto attributes=pet_owner_combat_.find(member->second.actor);
        if(attributes==pet_owner_combat_.end()||attributes->second.owner!=member->second)return false;

        const auto membership=pet_owners_.find(pet);if(membership==pet_owners_.end()||
           !valid_pet_owner_locked(membership->second,pet,require_alive))return false;
        const auto owner=players_.find(membership->second.actor);const auto victim=npc_combat_.find(target.actor);
        if(owner==players_.end()||!owner->second.npc_publication_ready||
           victim==npc_combat_.end()||victim->second.hex_scope()!=target||!victim->second.hostile_to_players(victim->second.lifetime()))return false;
        const auto* state=victim->second.actor(victim->second.lifetime());if(!state||!state->health||state->health->current<=0)return false;
        const auto assignment=quest_npc_owners_.find(target.actor);
        if(assignment==quest_npc_owners_.end())return !npc_opposed_allies_.contains(target);
        if(assignment->second.first!=membership->second)return false;
        const auto objective=owner->second.quest_kills.find(assignment->second.second);
        return objective!=owner->second.quest_kills.end()&&objective->second.remaining.contains(target);
    }
    bool valid_pet_enemy_ordinary_locked(skills::effects::Scope source,skills::effects::Scope pet,bool require_alive=true)const{
        const auto membership=pet_owners_.find(pet);const auto enemy=npc_combat_.find(source.actor);
        if(membership==pet_owners_.end()||!valid_pet_owner_locked(membership->second,pet)||enemy==npc_combat_.end()||
           enemy->second.hex_scope()!=source||!enemy->second.hostile_to_players(enemy->second.lifetime()))return false;
        const auto* state=enemy->second.actor(enemy->second.lifetime());
        if(!state||!state->health||(require_alive&&state->health->current<=0))return false;
        const auto assignment=quest_npc_owners_.find(source.actor);
        if(assignment==quest_npc_owners_.end())return !npc_opposed_allies_.contains(source);
        if(assignment->second.first!=membership->second)return false;
        const auto owner=players_.find(membership->second.actor);
        if(owner==players_.end()||owner->second.effect_scope!=membership->second)return false;
        const auto objective=owner->second.quest_kills.find(assignment->second.second);
        return objective!=owner->second.quest_kills.end()&&(!require_alive||objective->second.remaining.contains(source));
    }
    Player* pet_credit_owner_locked(skills::effects::Scope pet){
        const auto membership=pet_owners_.find(pet);if(membership==pet_owners_.end())return nullptr;
        const auto player=players_.find(membership->second.actor);
        return player!=players_.end()&&player->second.effect_scope==membership->second?&player->second:nullptr;
    }
    // Shared movement integrator must consult this after its hostile lookups.
    combat::ai::FlareController* pet_motion_controller_locked(skills::effects::Scope pet){
        const auto found=pet_ai_.find(pet.actor);
        if(found!=pet_ai_.end()&&found->second.source==pet)return &found->second.motion;
        const auto wild=tameable_stalker_combat_.find(pet);
        return wild!=tameable_stalker_combat_.end()&&wild->second.idle?&*wild->second.idle:nullptr;
    }
    void append_pet_layered_sources_locked(std::vector<skills::effects::Scope>& sources)const{
        for(const auto& [id,ai]:pet_ai_)if(ai.layered_motion&&!ai.layered_motion->route.empty())sources.push_back(ai.source);
    }
    bool advance_pet_layered_motion_locked(skills::effects::Scope source,std::uint64_t now){
        const auto found=pet_ai_.find(source.actor);
        if(found==pet_ai_.end()||found->second.source!=source||!found->second.layered_motion)return true;
        auto& ai=found->second;auto& motion=*ai.layered_motion;
        const auto body=npcs_.find(source.actor);const auto live=npc_combat_.find(source.actor);
        if(body==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=source){ai.layered_motion.reset();return true;}
        if(now<motion.updated||now<live->second.health_clock(live->second.lifetime()).value_or(now))return true;
        if(now==motion.updated||motion.route.empty())return true;
        const auto owner=players_.find(ai.owner.actor);
        if(owner==players_.end()||owner->second.effect_scope!=ai.owner||!owner->second.npc_publication_ready||
           !live->second.actor(live->second.lifetime())->health||live->second.actor(live->second.lifetime())->health->current<=0){motion.updated=now;return true;}
        const auto eligible=eligible_foes_wards_locked(source,motion.updated,now);if(!eligible)return false;
        std::vector<skills::effects::WardFoeRouteLeg> legs;float x=body->second.x,y=body->second.y;unsigned plane=body->second.plane;
        for(const auto& goal:motion.route){legs.push_back({x,y,goal.point.x,goal.point.z,plane});x=goal.point.x;y=goal.point.z;plane=unsigned(goal.surface.plane);}
        const skills::effects::WardFoeRecipient recipient{source,body->second.plane,body->second.x,body->second.y,true,false,true};
        const auto travel=skills::effects::ward_foes_npc_route_travel(recipient,*eligible,legs,live->second,live->second.lifetime(),motion.updated,now,ai.motion.policy.speed);
        if(!travel)return false;double remaining=travel->distance;motion.updated=now;std::vector<Packet> packets;
        while(remaining>0&&!motion.route.empty()){
            const auto next=motion.route.front();const double dx=next.point.x-body->second.x,dy=next.point.z-body->second.y,length=std::hypot(dx,dy);
            double step=std::min(length,remaining);bool reached_stop=false;
            if(motion.route.size()==1&&body->second.plane==motion.goal.surface.plane&&length>0){
                const combat::ai::motion::Point point{next.point.x,next.point.z};
                const auto moved=combat::ai::motion::advance({body->second.x,body->second.y},std::span(&point,1),point,motion.stop,remaining);
                if(!moved)return false;step=moved->spent;reached_stop=moved->reached;
            }
            if(length>0){body->second.facing=float(std::atan2(dy,dx));body->second.x+=float(dx*step/length);body->second.y+=float(dy*step/length);remaining-=step;}
            if(step>=length){body->second.x=next.point.x;body->second.y=next.point.z;body->second.plane=std::uint16_t(next.surface.plane);motion.surface=next.surface;motion.route.erase(motion.route.begin());}
            else if(navigation_){const auto cell=navigation_->locate_cell(body->second.plane,{body->second.x,body->second.y});if(cell)motion.surface={body->second.plane,*cell};}
            packets.push_back(position_packet(source.actor,body->second.x,body->second.y,body->second.plane));
            if(reached_stop){motion.route.clear();break;}
            if(step<length)break;
        }
        ai.motion.plane=body->second.plane;
        if(!packets.empty())packets.push_back(agent::facing_2022(source.actor,body->second.facing));
        if(motion.route.empty())packets.push_back(stop_packet(source.actor));
        if(!packets.empty())for(auto& [id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue(packets);
        return true;
    }
    void plan_pet_layered_follow_locked(PetAiController& ai,const Npc& body,navigation::SurfacePoint goal,std::uint64_t now){
        if(!navigation_||!layered_)return;
        if(ai.layered_motion&&ai.layered_motion->goal.surface==goal.surface&&
           ai.layered_motion->goal.point.x==goal.point.x&&ai.layered_motion->goal.point.z==goal.point.z&&!ai.layered_motion->route.empty())return;
        const auto cell=navigation_->locate_cell(body.plane,{body.x,body.y});if(!cell)return;
        const navigation::Surface start{body.plane,*cell};
        const auto route=layered_->route(start,{body.x,body.y},goal.surface,goal.point);
        if(!route||route->size()<2){stop_pet_motion_locked(ai.source,now);return;}
        npc_ai_motion_.erase(ai.source.actor);
        ai.layered_motion=PetLayeredMotion{start,std::vector<navigation::SurfacePoint>(route->begin()+1,route->end()),goal,now,0};
    }
    void retire_pet_layered_routes_locked(std::uint64_t now){
        for(auto& [id,ai]:pet_ai_){if(!ai.layered_motion||ai.layered_motion->updated>now)continue;
            const auto body=npcs_.find(id);const auto live=npc_combat_.find(id);
            if(body==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=ai.source)continue;
            if(now<live->second.health_clock(live->second.lifetime()).value_or(now))continue;
            auto surface=ai.layered_motion->surface;navigation::Point point{body->second.x,body->second.y};bool legal=bool(layered_);
            for(const auto& next:ai.layered_motion->route){if(!legal||!layered_->straight_route(surface,point,next.surface,next.point)){legal=false;break;}surface=next.surface;point=next.point;}
            if(!legal)stop_pet_motion_locked(ai.source,now);
        }
    }
    void drive_tameable_stalker_idle_locked(std::uint64_t now){
        for(auto& [scope,reviewed]:tameable_stalker_combat_){
            const auto body=npcs_.find(scope.actor);const auto live=npc_combat_.find(scope.actor);
            if(!reviewed.idle||body==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=scope||
               !live->second.neutral_guardian(live->second.lifetime())||now<reviewed.idle->updated||
               now<live->second.health_clock(live->second.lifetime()).value_or(now))continue;
            if(!advance_npc_ai_motion_locked(scope,now))continue;
            idle_npc_wander_locked(*reviewed.idle,body->second,now);reviewed.idle->updated=now;
        }
    }
    void stop_pet_motion_locked(skills::effects::Scope pet,std::uint64_t now){
        if(const auto ai=pet_ai_.find(pet.actor);ai!=pet_ai_.end()&&ai->second.source==pet&&ai->second.layered_motion&&ai->second.layered_motion->updated<=now){
            ai->second.layered_motion.reset();const auto body=npcs_.find(pet.actor);
            if(body!=npcs_.end())for(auto& [id,peer]:players_)if(peer.npc_publication_ready)
                peer.outbox->enqueue({position_packet(pet.actor,body->second.x,body->second.y,body->second.plane),stop_packet(pet.actor)});
        }

        const auto intent=npc_ai_motion_.find(pet.actor);const auto body=npcs_.find(pet.actor);
        if(intent==npc_ai_motion_.end()||intent->second.source!=pet||intent->second.updated>now||body==npcs_.end())return;
        npc_ai_motion_.erase(intent);
        for(auto& [id,peer]:players_)if(peer.npc_publication_ready)
            peer.outbox->enqueue({position_packet(pet.actor,body->second.x,body->second.y,body->second.plane),stop_packet(pet.actor)});
    }
    void cancel_pet_pending_locked(skills::effects::Scope pet,std::uint64_t now){
        const auto ordinary=npc_ordinary_ai_.find(pet.actor);const auto live=npc_combat_.find(pet.actor);
        if(ordinary==npc_ordinary_ai_.end()||ordinary->second.source!=pet||live==npc_combat_.end()||live->second.hex_scope()!=pet)return;
        if(ordinary->second.ledger.pending()){
            if(!live->second.cancel_action(live->second.lifetime(),now)||!ordinary->second.ledger.cancel(now))return;
            npc_ordinary_packet_locked(combat::weapon::stop_2022(pet.actor));
        }
    }
    ai::companion::Snapshot pet_ai_snapshot_locked(PetAiController& controller,std::uint64_t now){
        ai::companion::Snapshot snapshot;snapshot.self=pet_actor_key(controller.source);snapshot.owner=pet_actor_key(controller.owner);
        if(!valid_pet_owner_locked(controller.owner,controller.source))return snapshot;
        const auto owner=players_.find(controller.owner.actor);const auto pet=npcs_.find(controller.source.actor);
        if(owner==players_.end()||pet==npcs_.end()||!owner->second.npc_publication_ready)return snapshot;
        const auto position=healing_position_locked(owner->second,now);if(!position||owner->second.surface.plane!=pet->second.plane)return snapshot;
        snapshot.alive=true;snapshot.owner_distance=std::hypot(position->x-pet->second.x,position->z-pet->second.y);
        const auto ledger=npc_ordinary_ai_.find(controller.source.actor);
        if(ledger==npc_ordinary_ai_.end()||ledger->second.source!=controller.source)return snapshot;
        if(ledger->second.ledger.pending())snapshot.pending_action_target=pet_actor_key(ledger->second.ledger.pending()->target);
        const auto& profile=ledger->second.ledger.profile();
        for(const auto& [id,body]:npcs_){
            const auto live=npc_combat_.find(id);if(live==npc_combat_.end())continue;
            const auto target=live->second.hex_scope();if(!valid_pet_ordinary_locked(controller.source,target)||body.plane!=pet->second.plane)continue;
            const double distance=std::hypot(body.x-pet->second.x,body.y-pet->second.y);
            bool line=true;
            if(navigation_){const auto start=navigation_->locate_cell(pet->second.plane,{pet->second.x,pet->second.y});
                const auto trace=start?layered_->trace({pet->second.plane,*start},{pet->second.x,pet->second.y},{body.x,body.y}):std::nullopt;
                line=trace&&trace->plane==body.plane;}
            snapshot.targets.push_back({pet_actor_key(target),true,true,line&&distance<=profile.range,distance});
        }
        snapshot.acquisition=controller.acquisition;controller.acquisition.reset();return snapshot;
    }
    bool settle_pet_charm_locked(skills::effects::Scope owner,skills::effects::Scope target,std::uint64_t now){
        settle_damage_fields_locked(now);
        const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner||
           now<player->second.health_updated||now<player->second.energy_updated||
           !advance_resource_event_locked(player->second,now))return false;
        refresh_npc_movement_locked(target.actor,now);
        const auto live=npc_combat_.find(target.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=target||
           now<live->second.health_clock(live->second.lifetime()).value_or(now))return false;
        advance_npc_conditions_locked(target.actor,live->second,now);
        return true; // Caller reacquires every scope after settlement callbacks.
    }
    std::optional<pets::CharmFacts> pet_charm_facts_locked(skills::effects::Scope owner,
            skills::effects::Scope target,std::uint64_t now)const{
        const auto player=players_.find(owner.actor);const auto session=player_pet_sessions_.find(owner.actor);
        const auto attributes=pet_owner_combat_.find(owner.actor);
        const auto enrolled=tameable_stalkers_.find(target);const auto body=npcs_.find(target.actor);
        const auto live=npc_combat_.find(target.actor);
        if(player==players_.end()||player->second.effect_scope!=owner||session==player_pet_sessions_.end()||
           session->second.owner!=owner||attributes==pet_owner_combat_.end()||attributes->second.owner!=owner||enrolled==tameable_stalkers_.end()||body==npcs_.end()||
           live==npc_combat_.end()||live->second.hex_scope()!=target||!body->second.combat_definition||
           body->second.combat_definition->binding!=enrolled->second)return {};
        const auto* animal=live->second.actor(live->second.lifetime());
        const auto position=healing_position_locked(player->second,now);if(!animal||!position)return {};
        bool line=true;
        if(navigation_){const auto result=layered_->trace(player->second.surface,*position,{body->second.x,body->second.y});line=result&&result->plane==body->second.plane;}
        const auto clock=std::max({attributes->second.observed,player->second.health_updated,player->second.energy_updated,
            live->second.health_clock(live->second.lifetime()).value_or(0),live->second.energy_clock(live->second.lifetime()).value_or(0)});
        return pets::CharmFacts{owner,target,attributes->second.level,body->second.definition.level,
            player->second.precise_health>0,player->second.npc_publication_ready,animal->health&&animal->health->current>0,
            true,pet_owners_.contains(target),std::hypot(position->x-body->second.x,position->z-body->second.y)<=1248,
            player->second.surface.plane==body->second.plane,line,bool(session->second.durable.pet),clock};
    }
public:
    // Trusted map classification, immutable for this exact owner lifetime.
    bool pet_instance_mode(skills::effects::Scope owner,bool pvp,std::uint64_t now){
        std::lock_guard lock(mutex_);const auto player=players_.find(owner.actor);const auto session=player_pet_sessions_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner||session==player_pet_sessions_.end()||session->second.owner!=owner||
           now<player->second.health_updated||now<player->second.energy_updated||
           (session->second.pvp&&*session->second.pvp!=pvp))return false;
        session->second.pvp=pvp;return true;
    }
    bool reconcile_pet_bar_mode(skills::effects::Scope owner,unsigned map,bool combat_zone,bool pvp,
            std::span<const unsigned> bar,std::uint64_t now){
        return pet_instance_mode(owner,pvp,now)&&reconcile_pet_bar(owner,map,combat_zone,bar,now);
    }
    // Authenticated entry/bar adapter supplies actual combat-zone classification
    // and owned current skill IDs. Removing bar presence never releases storage.
    bool reconcile_pet_bar(skills::effects::Scope owner,unsigned map,bool combat_zone,
            std::span<const unsigned> bar,std::uint64_t now){
        std::lock_guard lock(mutex_);const auto player=players_.find(owner.actor);const auto session=player_pet_sessions_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner||session==player_pet_sessions_.end()||session->second.owner!=owner)return false;
        // Legacy mechanics convenience; production calls explicit _mode.
        if(!session->second.pvp)session->second.pvp=false;
        if(!combat_zone||!pets::brings_pet(bar)||!session->second.durable.pet){remove_world_pet_locked(session->second,now);return true;}
        if(session->second.actor)return valid_pet_owner_locked(owner,*session->second.actor,false);
        if(!player->second.team_token||npcs_.size()>=1024||next_actor_generation_==UINT64_MAX)return false;
        const auto position=healing_position_locked(player->second,now);if(!position)return false;
        const auto plane=static_cast<std::uint16_t>(player->second.surface.plane);
        if(navigation_&&!navigation_->locate_cell(plane,*position))return false;
        auto actor=next_quest_actor_;
        while(actor<=65535&&(players_.contains(actor)||npcs_.contains(actor)||ground_items_.used_actor(actor)||
              retired_quest_actors_.contains(actor)||std::any_of(npcs_.begin(),npcs_.end(),[&](const auto& pair){return pair.second.definition.id==actor;})))++actor;
        if(actor>65535)return false;
        auto body=owned_stalker_template(map,actor,actor,*session->second.durable.pet,player->second.team_token,*position,plane);if(!body)return false;
        Instance validation;try{validation.configure_npcs({*body});}catch(const std::exception&){return false;}
        auto packets=validation.npc_packets();const skills::effects::Scope scope{instance_scope_,actor,next_actor_generation_};
        packets.push_back(pets::wire::add(actor,owner.actor,pet_encoded_name(*session->second.durable.pet),128406,129924,pets::wire::Behavior::guard));
        auto bodies=npcs_;auto actors=npc_combat_;auto owners=pet_owners_;auto quests=quest_npc_owners_;auto reservations=corpse_reservations_;
        bodies.emplace(actor,*body);actors.emplace(actor,combat::npcs::LiveActor(*body->combat_definition,{instance_scope_,instance_scope_,next_actor_generation_,map,actor}));
        if(!owners.emplace(scope,owner).second||!quests.emplace(actor,std::pair{owner,0u}).second||!reservations.pre_exploit(scope))return false;
        auto& enrolled=actors.at(actor);
        if(!enrolled.enroll_owned_stalker(enrolled.lifetime(),body->combat_definition->binding,owner,player->second.team_token,!*session->second.pvp,now))return false;
        const auto prepared_ai=prepare_default_pet_ai_locked(owner,scope,*session->second.durable.pet,*body,now);
        if(!prepared_ai)return false;
        auto ai=pet_ai_;auto ordinary=npc_ordinary_ai_;auto friendly=npc_friendly_ordinary_;
        ai.emplace(actor,*prepared_ai);ordinary.emplace(actor,NpcOrdinaryController{scope,combat::ai::ordinary::Ledger(scope,pets::default_stalker_ai(scope,*session->second.durable.pet)->ordinary),{}});
        friendly.emplace(actor,FriendlyOrdinaryBinding{scope,{}, {},combat::ai::opposition::Side::allied});
        npcs_.swap(bodies);npc_combat_.swap(actors);pet_owners_.swap(owners);quest_npc_owners_.swap(quests);corpse_reservations_=std::move(reservations);
        pet_ai_.swap(ai);npc_ordinary_ai_.swap(ordinary);npc_friendly_ordinary_.swap(friendly);
        next_quest_actor_=actor+1;++next_actor_generation_;session->second.actor=scope;bind_npc_ward_queries_locked();
        publish_new_npcs_locked(packets);return true;
    }
    std::optional<skills::effects::Scope> owned_pet_scope(skills::effects::Scope owner)const{
        std::lock_guard lock(mutex_);const auto session=player_pet_sessions_.find(owner.actor);
        if(session==player_pet_sessions_.end()||session->second.owner!=owner||!session->second.actor||!valid_pet_owner_locked(owner,*session->second.actor,false))return {};
        return session->second.actor;
    }
    // Auth adapter supplies CURRENT effective owner level/Beast Mastery22.
    // Missing stats reject pet attack admission/contact rather than defaulting.
    bool pet_owner_combat(skills::effects::Scope owner,unsigned level,unsigned beast_mastery,std::uint64_t now){
        if(!level||level>20||beast_mastery>20)return false;
        std::lock_guard lock(mutex_);const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner)return false;
        const auto prior=pet_owner_combat_.find(owner.actor);
        if(prior!=pet_owner_combat_.end()&&prior->second.owner==owner&&prior->second.observed>now)return false;
        pet_owner_combat_[owner.actor]={owner,level,beast_mastery,now};return true;
    }
    bool pet_comfort_ready(skills::effects::Scope owner,skills::effects::Scope pet,std::uint64_t now)const{
        std::lock_guard lock(mutex_);return pet_comfort_ready_locked(owner,pet,now);
    }
    struct PetComfortResult {bool accepted=false,revived=false;double healed=0;std::vector<Packet> packets;std::function<void()> publication;};
    PetComfortResult complete_pet_comfort(skills::effects::Scope owner,skills::effects::Scope pet,
            unsigned rank,std::uint64_t cast_sequence,bool paid_action_current,std::uint64_t now,bool defer_publication=false){
        const auto plan=pets::comfort(rank);if(!plan||!cast_sequence||!paid_action_current)return {};
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto completed=pet_comfort_completed_.find(owner);
        if(completed!=pet_comfort_completed_.end()&&cast_sequence<=completed->second)return {};
        const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner||!advance_resource_event_locked(player->second,now))return {};
        refresh_npc_movement_locked(pet.actor,now);
        auto live=npc_combat_.find(pet.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=pet||now<live->second.health_clock(live->second.lifetime()).value_or(now))return {};
        // Shared corpse refresh retires owned effects/credit/weapon records and
        // drains pending reaction journals before the typed same-lifetime revive.
        advance_npc_conditions_locked(pet.actor,live->second,now);
        if(!pet_comfort_ready_locked(owner,pet,now))return {};
        live=npc_combat_.find(pet.actor);const auto body=npcs_.find(pet.actor);
        if(live==npc_combat_.end()||body==npcs_.end()||!body->second.combat_definition)return {};
        const auto life=live->second.lifetime();const auto before=*live->second.actor(life)->health;
        auto proposed=live->second;const bool resurrect=before.current<=0;
        if(resurrect){
            if(!proposed.revive_owned_stalker(life,body->second.combat_definition->binding,owner,plan->resurrect_percent,now))return {};
        }else if(!proposed.heal_ally(life,plan->healing,now))return {};
        const auto after=*proposed.actor(life)->health;
        const double actual=resurrect?0:std::max(0.,after.current-before.current);
        struct Publication {bool sent=false;std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;
            void send(){if(std::exchange(sent,true))return;for(auto& [box,queue]:recipients)box->enqueue(std::move(queue.packets));}};
        std::shared_ptr<Publication> deferred;std::vector<std::unique_ptr<Outbox::Capture>> holds;
        if(defer_publication){
            deferred=std::make_shared<Publication>();std::set<const Outbox*> seen;
            for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)deferred->recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
            for(auto& [box,queue]:deferred->recipients)holds.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        }
        pet_comfort_completed_[owner]=cast_sequence;live->second=std::move(proposed);
        publish_npc_enchantment_ends_locked(live->second);
        // Publishers may run owned end reactions. Reacquire exact lifetime before
        // aura/death publication and build final Health only after those callbacks.
        live=npc_combat_.find(pet.actor);
        if(live!=npc_combat_.end()&&live->second.matches(life))publish_npc_aura_ends_locked(live->second);
        prune_reversal_caster_credit_locked(now);
        live=npc_combat_.find(pet.actor);const auto fresh_body=npcs_.find(pet.actor);
        if(live!=npc_combat_.end()&&live->second.matches(life)&&fresh_body!=npcs_.end()&&
           !resurrect&&before.current>0&&live->second.actor(life)->health->current<=0)
            record_npc_death_locked(nullptr,fresh_body->second,live->second,now);
        std::vector<Packet> packets;live=npc_combat_.find(pet.actor);
        if(live!=npc_combat_.end()&&live->second.matches(life)&&live->second.actor(life)->health){
            packets=combat::health_packets_2022(*live->second.actor(life));
            packets.push_back(emotes::effects_2022(pet.actor,live->second.actor(life)->effects));
            if(actual>0)packets.push_back(skills::particles::health_change_2022(pet.actor,owner.actor,actual,after.maximum));
        }
        holds.clear();std::function<void()> publication;
        if(deferred)defer_or_publish_completion_locked(&publication,[deferred]{deferred->send();});
        return {true,resurrect,actual,std::move(packets),std::move(publication)};
    }
private:
    bool pet_comfort_ready_locked(skills::effects::Scope owner,skills::effects::Scope pet,std::uint64_t now)const{
        if(!valid_pet_owner_locked(owner,pet,false))return false;
        const auto player=players_.find(owner.actor);const auto body=npcs_.find(pet.actor);const auto live=npc_combat_.find(pet.actor);
        if(player==players_.end()||body==npcs_.end()||live==npc_combat_.end()||!player->second.npc_publication_ready||
           player->second.precise_health<=0||
           live->second.healing_allowed(live->second.lifetime())!=std::optional(true)||
           now<player->second.health_updated||now<player->second.energy_updated||
           now<live->second.health_clock(live->second.lifetime()).value_or(now))return false;
        const auto position=healing_position_locked(player->second,now);
        return position&&std::hypot(position->x-body->second.x,position->z-body->second.y)<=5000;
    }
public:
    bool configure_pet_ai(skills::effects::Scope owner,skills::effects::Scope pet,
            combat::ai::FlarePolicy motion,ai::companion::Config behavior,
            combat::ai::ordinary::Reviewed attack,std::uint64_t now){
        if(motion.source!=pet||motion.target.instance||motion.target.actor||motion.target.generation||
           !motion.valid()||!attack.valid()||attack.delivery!=combat::ai::ordinary::Delivery::melee||attack.damage_type!=2||
           !behavior.follow_start||!behavior.follow_stop||!behavior.pet_guard_combat_break||!behavior.pet_attack_lock_range)return false;
        for(auto distance:{*behavior.follow_start,*behavior.follow_stop,*behavior.pet_guard_combat_break,*behavior.pet_attack_lock_range})
            if(!std::isfinite(distance)||distance<0||distance>10000)return false;
        if(*behavior.follow_start<*behavior.follow_stop)return false;
        std::lock_guard lock(mutex_);if(!valid_pet_owner_locked(owner,pet)||pet_ai_.contains(pet.actor))return false;
        const auto body=npcs_.find(pet.actor);const auto live=npc_combat_.find(pet.actor);
        if(body==npcs_.end()||live==npc_combat_.end()||motion.level!=body->second.definition.level||
           !live->second.healable_ally(live->second.lifetime())||live->second.healing_allowed(live->second.lifetime())!=std::optional(true)||
           npc_ordinary_ai_.contains(pet.actor)||npc_flare_ai_.contains(pet.actor)||npc_elemental_ai_.contains(pet.actor)||
           !live->second.ordinary_action_ready(live->second.lifetime(),now))return false;
        npc_ordinary_ai_.emplace(pet.actor,NpcOrdinaryController{pet,combat::ai::ordinary::Ledger(pet,std::move(attack)),{}});
        npc_friendly_ordinary_.emplace(pet.actor,FriendlyOrdinaryBinding{pet,{}, {},combat::ai::opposition::Side::allied});
        PetAiController controller;controller.owner=owner;controller.source=pet;controller.config=behavior;
        controller.motion={motion,body->second.x,body->second.y,body->second.plane,now};
        pet_ai_.emplace(pet.actor,std::move(controller));return true;
    }
    // Successful targeted effects only. Runtime retains the exact scopes and
    // a nonwrapping completion sequence; no selection or cast-start event enters.
    void pet_offensive_completed(skills::effects::Scope owner,skills::effects::Scope target,
            unsigned skill,std::uint64_t sequence,std::uint64_t now){
        if(!sequence||!skill||skill>3432)return;
        std::lock_guard lock(mutex_);const auto session=player_pet_sessions_.find(owner.actor);
        if(session==player_pet_sessions_.end()||session->second.owner!=owner||!session->second.actor)return;
        const auto controller=pet_ai_.find(session->second.actor->actor);
        if(controller==pet_ai_.end()||controller->second.owner!=owner||controller->second.source!=*session->second.actor||
           sequence<=controller->second.offensive_completion)return;
        controller->second.offensive_completion=sequence;
        queue_pet_acquisition_locked(owner,target,ai::companion::Trigger::owner_attack,now);
    }
    // Hooks forward only committed owner attacks/current adjacent threats.
    // Host policy supplies threat qualification; proximity alone never provokes
    // a Guard pet. Pet attack-lock retains its original current scoped target.
    bool provoke_pet(skills::effects::Scope owner,skills::effects::Scope target,
            ai::companion::Trigger trigger,std::uint64_t now){
        std::lock_guard lock(mutex_);const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner||player->second.precise_health<=0)return false;
        const auto session=player_pet_sessions_.find(owner.actor);
        if(session==player_pet_sessions_.end()||session->second.owner!=owner||!session->second.actor)return false;
        const auto pet=*session->second.actor;const auto found=pet_ai_.find(pet.actor);
        if(found==pet_ai_.end()||found->second.source!=pet||found->second.owner!=owner||
           (trigger!=ai::companion::Trigger::owner_attack&&trigger!=ai::companion::Trigger::adjacent_owner_threat)||
           !valid_pet_ordinary_locked(pet,target)||found->second.motion.updated>now)return false;
        found->second.acquisition=ai::companion::Acquisition{pet_actor_key(target),trigger,true};return true;
    }
    std::optional<std::vector<Packet>> command_pet(skills::effects::Scope owner,
            const pets::wire::Command& command,std::uint64_t now){
        std::lock_guard lock(mutex_);const auto found=pet_ai_.find(command.pet);
        if(found==pet_ai_.end()||found->second.owner!=owner||!valid_pet_owner_locked(owner,found->second.source)||
           found->second.motion.updated>now)return {};
        auto& controller=found->second;refresh_npc_movement_locked(command.pet,now);
        const auto player=players_.find(owner.actor);
        if(player==players_.end()||now<player->second.health_updated||now<player->second.energy_updated)return {};
        if(player->second.precise_health<=0&&((command.kind==pets::wire::Command::Kind::target&&command.value)||
           (command.kind==pets::wire::Command::Kind::behavior&&command.value==0)))return {};
        auto snapshot=pet_ai_snapshot_locked(controller,now);if(!snapshot.alive)return {};
        // Bind the detached policy to the live pair before accepting commands.
        controller.behavior.tick(snapshot,controller.config);
        auto proposed=controller.behavior;std::optional<skills::effects::Scope> locked=controller.locked;
        auto mode=controller.mode;std::vector<Packet> packets;
        if(command.kind==pets::wire::Command::Kind::target){
            if(command.value){const auto live=npc_combat_.find(command.value);if(live==npc_combat_.end())return {};
                const auto target=live->second.hex_scope();if(!valid_pet_ordinary_locked(controller.source,target))return {};
                if(!proposed.command({ai::companion::PetMode::attack,pet_actor_key(target)},snapshot))return {};
                locked=target;
            }else{if(!proposed.command({mode==pets::wire::Behavior::avoid_combat?ai::companion::PetMode::heel:ai::companion::PetMode::guard,{}},snapshot))return {};locked.reset();}
            packets.push_back(pets::wire::locked_target(command.pet,command.value));
        }else{
            if(command.value>2)return {};mode=pets::wire::Behavior(command.value);
            const auto pet_mode=mode==pets::wire::Behavior::avoid_combat?ai::companion::PetMode::heel:
                mode==pets::wire::Behavior::fight&&locked?ai::companion::PetMode::attack:ai::companion::PetMode::guard;
            if(!proposed.command({pet_mode,locked?std::optional(pet_actor_key(*locked)):std::nullopt},snapshot))return {};
            if(mode!=pets::wire::Behavior::fight){locked.reset();packets.push_back(pets::wire::locked_target(command.pet,0));}
            packets.push_back(pets::wire::behavior(command.pet,mode));
        }
        controller.behavior=std::move(proposed);controller.mode=mode;controller.locked=locked;
        controller.motion.updated=now;
        cancel_pet_pending_locked(controller.source,now);stop_pet_motion_locked(controller.source,now);
        if(const auto ordinary=npc_friendly_ordinary_.find(command.pet);ordinary!=npc_friendly_ordinary_.end()&&ordinary->second.source==controller.source)ordinary->second.target.reset();
        return packets;
    }
private:
    void drive_pet_ai_locked(std::uint64_t now){
        std::vector<skills::effects::Scope> pet_sources;for(const auto& [id,controller]:pet_ai_)pet_sources.push_back(controller.source);
        for(const auto scope:pet_sources){
            const auto found=pet_ai_.find(scope.actor);if(found==pet_ai_.end()||found->second.source!=scope)continue;
            auto& controller=found->second;
            if(now<controller.motion.updated)continue;
            if(!valid_pet_owner_locked(controller.owner,scope)){
                cancel_pet_pending_locked(scope,now);stop_pet_motion_locked(scope,now);
                if(const auto ordinary=npc_friendly_ordinary_.find(scope.actor);ordinary!=npc_friendly_ordinary_.end()&&ordinary->second.source==scope)ordinary->second.target.reset();
                continue;
            }
            const auto owner=players_.find(controller.owner.actor);
            if(owner==players_.end()||!owner->second.npc_publication_ready){cancel_pet_pending_locked(scope,now);stop_pet_motion_locked(scope,now);continue;}
            if(now<owner->second.health_updated||now<owner->second.energy_updated)continue;
            if(!advance_npc_ai_motion_locked(scope,now))continue;
            const auto pet_body=npcs_.find(scope.actor);if(pet_body==npcs_.end())continue;
            if(pet_body->second.plane!=owner->second.surface.plane){
                cancel_pet_pending_locked(scope,now);
                if(const auto ordinary=npc_friendly_ordinary_.find(scope.actor);ordinary!=npc_friendly_ordinary_.end())ordinary->second.target.reset();
                const auto position=healing_position_locked(owner->second,now);
                if(position)plan_pet_layered_follow_locked(controller,pet_body->second,{*position,owner->second.surface},now);
                controller.motion.updated=now;continue;
            }
            if(controller.layered_motion)stop_pet_motion_locked(scope,now);
            if(const auto selected=controller.behavior.target();selected){
                const auto target=npc_combat_.find(selected->id);
                if(target!=npc_combat_.end()&&target->second.hex_scope()==pet_scope_key(*selected)&&
                   now<target->second.health_clock(target->second.lifetime()).value_or(now))continue;
            }
            if(now<controller.motion.updated||!advance_npc_ai_motion_locked(scope,now))continue;
            controller.motion.updated=now;
            auto snapshot=pet_ai_snapshot_locked(controller,now);
            if(controller.motion.last_movement&&controller.motion.last_movement->at==now&&
               controller.motion.last_movement->outcome.kind==combat::ai::motion::OutcomeKind::leash_boundary){
                controller.behavior.command({controller.mode==pets::wire::Behavior::avoid_combat?
                    ai::companion::PetMode::heel:ai::companion::PetMode::guard,{}},snapshot);
                if(controller.locked){controller.locked.reset();for(auto& [id,peer]:players_)if(peer.npc_publication_ready)
                    peer.outbox->enqueue({pets::wire::locked_target(scope.actor,0)});}
                snapshot.acquisition.reset(); // A leash return owns this decision.
            }
            const auto decision=controller.behavior.tick(snapshot,controller.config);
            if(decision.cancel_pending_action)cancel_pet_pending_locked(scope,now);
            const auto ordinary=npc_friendly_ordinary_.find(scope.actor);const auto body=npcs_.find(scope.actor);
            if(ordinary==npc_friendly_ordinary_.end()||ordinary->second.source!=scope||body==npcs_.end())continue;
            if(controller.locked&&!controller.behavior.target()){
                controller.locked.reset();for(auto& [id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue({pets::wire::locked_target(scope.actor,0)});
            }
            using Intent=ai::companion::Intent;
            if(decision.intent==Intent::engage_target&&decision.target){
                stop_pet_motion_locked(scope,now);ordinary->second.target=pet_scope_key(*decision.target);
                begin_npc_friendly_ordinary_locked(scope,*ordinary->second.target,now);continue;
            }
            ordinary->second.target.reset();
            const auto live=npc_combat_.find(scope.actor);
            if(live==npc_combat_.end()||live->second.hex_scope()!=scope||live->second.active_action(live->second.lifetime(),now))continue;
            std::optional<navigation::Point> goal;float stop=0;controller.motion.returning=false;
            const auto owner_position=healing_position_locked(owner->second,now);
            if(owner_position){controller.motion.home_x=owner_position->x;controller.motion.home_y=owner_position->z;}
            if(decision.intent==Intent::follow_owner&&owner_position&&owner->second.surface.plane==body->second.plane){
                goal=*owner_position;stop=float(*controller.config.follow_stop);controller.motion.returning=true;
            }else if(decision.intent==Intent::pursue_target&&decision.target){
                const auto target=npcs_.find(decision.target->id);if(target!=npcs_.end()&&target->second.plane==body->second.plane){
                    goal=navigation::Point{target->second.x,target->second.y};
                    bool line=true;
                    if(navigation_){const auto start=navigation_->locate_cell(body->second.plane,{body->second.x,body->second.y});
                        const auto trace=start?layered_->trace({body->second.plane,*start},{body->second.x,body->second.y},*goal):std::nullopt;
                        line=trace&&trace->plane==target->second.plane;}
                    // Euclidean weapon reach across an obstruction is not an
                    // arrival. Follow the valid route until direct contact is
                    // possible, as the generic ordinary pursuit driver does.
                    stop=line?float(npc_ordinary_ai_.at(scope.actor).ledger.profile().range):0;}
            }
            if(goal)move_npc_flare_locked(controller.motion,body->second,*goal,stop,now);else stop_pet_motion_locked(scope,now);
        }
    }
public:
    // Called from the authenticated entry adapter with the latest storage read.
    // Replacement while a pet/paid charm is present is refused. Empty state is
    // explicit (revision0 for no storage row), never inferred from no World pet.
    bool restore_pet_profile(skills::effects::Scope owner,std::uint64_t revision,const pets::Profile& profile,unsigned owner_level){
        try{pets::validate(profile);}catch(const std::invalid_argument&){return false;}
        if(revision>=INT64_MAX||(profile.pet&&!revision)||owner_level<1||owner_level>20)return false;
        std::lock_guard lock(mutex_);const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner)return false;
        const auto old=player_pet_sessions_.find(owner.actor);
        if(old!=player_pet_sessions_.end())return old->second.owner==owner&&old->second.revision==revision&&old->second.durable==profile;
        player_pet_sessions_.emplace(owner.actor,PlayerPetSession{owner,revision,profile,{},owner_level});return true;
    }
    // Explicit exact-content enrollment. This does not add a spawn, infer
    // charmability from model/name, or supply missing Health/armor/AI tuning.
    bool configure_tameable_stalker(skills::effects::Scope scope,const combat::npcs::Binding& expected){
        std::lock_guard lock(mutex_);const auto body=npcs_.find(scope.actor);const auto live=npc_combat_.find(scope.actor);
        if(body==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=scope||
           !body->second.combat_definition||body->second.combat_definition->binding!=expected||
           expected.map!=162||expected.model_file!=128406||expected.skin_file||expected.appearance||
           expected.definition_flags!=13||expected.level!=5||body->second.definition.primary!=2||
           body->second.model_files!=std::vector<std::uint32_t>{129924}||pet_owners_.contains(scope)||
           quest_npc_owners_.contains(scope.actor)||!live->second.neutral_guardian(live->second.lifetime()))return false;
        const auto& metadata=*body->second.combat_definition;
        if(metadata.healing_allowed!=std::optional(true)||!metadata.conditions_allowed||!metadata.damageable||
           !metadata.spawn_health||metadata.spawn_health->maximum!=180||
           metadata.physical_armor!=35||metadata.general_armor!=35||metadata.earth_armor!=35||
           metadata.fire_armor!=35||metadata.cold_armor!=35||metadata.lightning_armor!=35)return false;
        const auto [it,inserted]=tameable_stalkers_.emplace(scope,expected);return inserted||it->second==expected;
    }
    bool configure_tameable_stalker_combat(skills::effects::Scope scope,combat::ai::FlarePolicy motion,
            combat::ai::ordinary::Reviewed attack,combat::ai::wander::Policy patrol,std::uint64_t now=0){
        if(motion.source!=scope||motion.target.instance||motion.target.actor||motion.target.generation||
           motion.proximity_acquisition||!motion.valid()||motion.level!=5||motion.follow<1248||
           !attack.valid()||attack.damage_type!=2||attack.delivery!=combat::ai::ordinary::Delivery::melee||
           attack.period_ms!=2000||!patrol.valid(motion.leash))return false;
        std::lock_guard lock(mutex_);const auto body=npcs_.find(scope.actor);
        if(!tameable_stalkers_.contains(scope)||tameable_stalker_combat_.contains(scope)||body==npcs_.end()||npc_wander_.contains(scope.actor))return false;
        combat::ai::FlareController idle{motion,body->second.x,body->second.y,body->second.plane,now};
        npc_wander_.emplace(scope.actor,NpcWander{scope,combat::ai::wander::State(patrol,now)});
        tameable_stalker_combat_.emplace(scope,TameableStalkerCombat{std::move(motion),std::move(attack),std::move(patrol),std::move(idle)});return true;
    }
    // Reserve before normal Runtime payment. Payment may reenter World; it
    // MUST run outside our mutex, and Runtime RAII cancels on every early exit.
    std::optional<pets::CharmReservation> reserve_pet_charm(skills::effects::Scope owner,
            skills::effects::Scope target,std::uint64_t now){
        std::lock_guard lock(mutex_);if(!settle_pet_charm_locked(owner,target,now))return {};
        const auto facts=pet_charm_facts_locked(owner,target,now);if(!facts)return {};
        const auto live=npc_combat_.find(target.actor);const auto reviewed=tameable_stalker_combat_.find(target);
        if(live==npc_combat_.end()||reviewed==tameable_stalker_combat_.end()||
           (!live->second.neutral_actor(live->second.lifetime())&&!live->second.hostile_to_players(live->second.lifetime())))return {};
        const auto motion=npc_flare_ai_.find(target.actor);const auto ordinary=npc_ordinary_ai_.find(target.actor);
        if((motion==npc_flare_ai_.end())!=(ordinary==npc_ordinary_ai_.end()))return {};
        if(motion!=npc_flare_ai_.end()&&(motion->second.policy.source!=target||ordinary->second.source!=target||!npc_hostile_ordinary_.contains(target)))return {};
        const auto session=player_pet_sessions_.find(owner.actor);
        const auto stats=pet_owner_combat_.find(owner.actor);
        if(!session->second.pvp.has_value()||stats==pet_owner_combat_.end()||stats->second.owner!=owner||stats->second.observed>now)return {};
        return pet_charms_.reserve(*facts,session->second.durable,session->second.revision,now);
    }
    // Exact cancellation deliberately needs no current player/generation lookup.
    bool cancel_pet_charm(const pets::CharmReservation& receipt){std::lock_guard lock(mutex_);return pet_charms_.cancel(receipt);}
    std::optional<pets::CharmAttempt> accept_pet_charm(const pets::CharmReservation& receipt,
            std::uint64_t cast_sequence,std::uint64_t starts,std::uint64_t duration){
        std::lock_guard lock(mutex_);
        try{
            auto result=accept_pet_charm_locked(receipt,cast_sequence,starts,duration);
            if(!result)pet_charms_.cancel(receipt);
            return result;
        }catch(...){pet_charms_.cancel(receipt);throw;}
    }
    // Trusted convenience for existing mechanics callers. Paid Runtime must use
    // reserve/payment/accept and preserve genuine reactive costs on rejection.
    std::optional<pets::CharmAttempt> begin_pet_charm(skills::effects::Scope owner,
            skills::effects::Scope target,std::uint64_t cast_sequence,std::uint64_t now,std::uint64_t duration){
        {std::lock_guard lock(mutex_);const auto session=player_pet_sessions_.find(owner.actor);
            if(session!=player_pet_sessions_.end()&&session->second.owner==owner&&!session->second.pvp)session->second.pvp=false;}
        const auto reserved=reserve_pet_charm(owner,target,now);
        return reserved?accept_pet_charm(*reserved,cast_sequence,now,duration):std::nullopt;
    }
private:
    std::optional<pets::CharmAttempt> accept_pet_charm_locked(const pets::CharmReservation& receipt,
            std::uint64_t cast_sequence,std::uint64_t now,std::uint64_t duration){
        const auto owner=receipt.owner,target=receipt.target;
        if(!settle_pet_charm_locked(owner,target,now))return {};
        const auto facts=pet_charm_facts_locked(owner,target,now);if(!facts)return {};
        const auto session=player_pet_sessions_.find(owner.actor);auto live=npc_combat_.find(target.actor);
        const auto reviewed=tameable_stalker_combat_.find(target);
        if(live==npc_combat_.end()||reviewed==tameable_stalker_combat_.end())return {};
        auto proposed=live->second;
        if(proposed.neutral_actor(proposed.lifetime())){
            if(!proposed.transition_stalker_charm(proposed.lifetime(),tameable_stalkers_.at(target),
                combat::npcs::LiveActor::StalkerCharmTransition::hostility,now))return {};
        }else if(!proposed.hostile_to_players(proposed.lifetime()))return {};
        auto motions=npc_flare_ai_;auto ordinary=npc_ordinary_ai_;auto wander=npc_wander_;auto hostile=npc_hostile_ordinary_;
        if(!motions.contains(target.actor)&&!ordinary.contains(target.actor)){
            auto policy=reviewed->second.motion;policy.target=owner;
            combat::ai::FlareController controller{policy,npcs_.at(target.actor).x,npcs_.at(target.actor).y,npcs_.at(target.actor).plane,now};
            controller.encounter.emplace(target);controller.encounter->provoke(owner);controller.engaged=true;
            motions.emplace(target.actor,std::move(controller));ordinary.emplace(target.actor,NpcOrdinaryController{target,combat::ai::ordinary::Ledger(target,reviewed->second.attack),{}});
            hostile.insert(target);wander.emplace(target.actor,NpcWander{target,combat::ai::wander::State(reviewed->second.patrol,now)});
        }else if(!motions.contains(target.actor)||!ordinary.contains(target.actor)||
                 motions.at(target.actor).policy.source!=target||ordinary.at(target.actor).source!=target||!hostile.contains(target))return {};
        else{
            auto& motion=motions.at(target.actor);if(motion.updated>now)return {};
            if(!motion.encounter)motion.encounter.emplace(target);motion.encounter->provoke(owner);
        }
        const auto attempt=pet_charms_.accept(receipt,*facts,session->second.durable,session->second.revision,cast_sequence,now,duration);
        if(!attempt)return {};
        live->second=std::move(proposed);npc_flare_ai_.swap(motions);npc_ordinary_ai_.swap(ordinary);npc_wander_.swap(wander);npc_hostile_ordinary_.swap(hostile);
        stop_pet_motion_locked(target,now);if(const auto patrol=npc_wander_.find(target.actor);patrol!=npc_wander_.end()&&patrol->second.source==target)patrol->second.state.suspend(now);
        return attempt;
    }
public:
    bool cancel_pet_charm(const pets::CharmAttempt& receipt){std::lock_guard lock(mutex_);return pet_charms_.cancel(receipt);}
    struct PetAdoptionResult {pets::CharmCompletion status=pets::CharmCompletion::invalid;std::vector<Packet> packets;};
    PetAdoptionResult complete_pet_charm(const pets::CharmAttempt& receipt,bool paid_action_current,
            const pets::AdoptionCommit& commit,std::uint64_t now){
        std::lock_guard lock(mutex_);if(!settle_pet_charm_locked(receipt.owner,receipt.target,now)){pet_charms_.cancel(receipt);return {};}
        const auto facts=pet_charm_facts_locked(receipt.owner,receipt.target,now);
        if(!facts){pet_charms_.cancel(receipt);return {};}
        const auto live=npc_combat_.find(receipt.target.actor);const auto body=npcs_.find(receipt.target.actor);
        const auto player=players_.find(receipt.owner.actor);const auto session=player_pet_sessions_.find(receipt.owner.actor);
        if(live==npc_combat_.end()||body==npcs_.end()||player==players_.end()||session==player_pet_sessions_.end())return {};
        auto prepared=live->second;
        const auto change=prepared.transition_stalker_charm(prepared.lifetime(),tameable_stalkers_.at(receipt.target),
            combat::npcs::LiveActor::StalkerCharmTransition::adoption,now);
        const bool changed=bool(change)&&session->second.pvp.has_value()&&prepared.enroll_owned_stalker(prepared.lifetime(),
            tameable_stalkers_.at(receipt.target),receipt.owner,player->second.team_token,session->second.pvp?!*session->second.pvp:false,now);
        auto owners=pet_owners_;auto reservations=corpse_reservations_;auto sessions=player_pet_sessions_;auto quests=quest_npc_owners_;
        const bool owned=owners.emplace(receipt.target,receipt.owner).second;
        const bool retirement_owned=quests.emplace(receipt.target.actor,std::pair{receipt.owner,0u}).second;
        const bool corpse=reservations.pre_exploit(receipt.target);
        auto& next=sessions.at(receipt.owner.actor);next.durable=receipt.write.replacement;
        next.revision=receipt.write.expected_revision+1;next.actor=receipt.target;
        // Default captured pet-name form, not a display-name-derived species.
        std::u16string name{u'\x108',u'\x10a',u'\xf0d',u'\xa74a',u'\xbbf9',u'\x249a',u'\x1'};
        std::vector<Packet> packets{pets::wire::add(receipt.target.actor,receipt.owner.actor,name,128406,129924,pets::wire::Behavior::guard)};
        if(change&&change->cancelled)packets.insert(packets.begin(),combat::weapon::stop_2022(receipt.target.actor));
        const auto prepared_ai=prepare_default_pet_ai_locked(receipt.owner,receipt.target,*receipt.write.replacement.pet,body->second,now);
        std::map<unsigned,PetAiController> ai_node;
        std::map<unsigned,NpcOrdinaryController> ordinary_node;
        std::map<unsigned,FriendlyOrdinaryBinding> friendly_node;
        if(prepared_ai){
            ai_node.emplace(receipt.target.actor,*prepared_ai);
            ordinary_node.emplace(receipt.target.actor,NpcOrdinaryController{receipt.target,combat::ai::ordinary::Ledger(receipt.target,pets::default_stalker_ai(receipt.target,*receipt.write.replacement.pet)->ordinary),{}});
            friendly_node.emplace(receipt.target.actor,FriendlyOrdinaryBinding{receipt.target,{}, {},combat::ai::opposition::Side::allied});
        }
        const auto status=pet_charms_.finish(receipt,*facts,paid_action_current,changed&&owned&&corpse&&retirement_owned&&bool(prepared_ai),commit,now);
        if(status!=pets::CharmCompletion::adopted)return {status,{}};
        // All allocations/proposals precede durable CAS. These are prepared
        // state moves; no user callback may run between adoption and this swap.
        live->second=std::move(prepared);pet_owners_.swap(owners);corpse_reservations_=std::move(reservations);player_pet_sessions_.swap(sessions);quest_npc_owners_.swap(quests);
        retire_npc_ordinary_locked(receipt.target,now);stop_pet_motion_locked(receipt.target,now);retire_npc_patrol_locked(receipt.target);
        if(const auto motion=npc_flare_ai_.find(receipt.target.actor);motion!=npc_flare_ai_.end()&&motion->second.policy.source==receipt.target)npc_flare_ai_.erase(motion);
        pet_ai_.insert(ai_node.extract(receipt.target.actor));npc_ordinary_ai_.insert(ordinary_node.extract(receipt.target.actor));
        npc_friendly_ordinary_.insert(friendly_node.extract(receipt.target.actor));
        tameable_stalkers_.erase(receipt.target);tameable_stalker_combat_.erase(receipt.target);return {status,std::move(packets)};
    }
private:
    // Called on exact owner/actor departure before ID reuse. Character's durable
    // pet remains stored; new entry reads storage and creates fresh World scope.
    void retire_pet_scope_locked(skills::effects::Scope scope,std::uint64_t now){
        if(const auto session=player_pet_sessions_.find(scope.actor);session!=player_pet_sessions_.end()&&session->second.owner==scope)
            remove_world_pet_locked(session->second,now);
        if(const auto stats=pet_owner_combat_.find(scope.actor);stats!=pet_owner_combat_.end()&&stats->second.owner==scope)pet_owner_combat_.erase(stats);
        pet_charms_.retire(scope);tameable_stalkers_.erase(scope);tameable_stalker_combat_.erase(scope);pet_comfort_completed_.erase(scope);
        if(const auto controller=pet_ai_.find(scope.actor);controller!=pet_ai_.end()&&controller->second.source==scope)pet_ai_.erase(controller);
        for(auto& [id,session]:player_pet_sessions_)if(session.actor==scope)session.actor.reset();
        std::erase_if(player_pet_sessions_,[&](const auto& pair){return pair.second.owner==scope;});
    }
