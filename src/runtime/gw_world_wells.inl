private:
    std::vector<skills::effects::WellField> well_fields_;
    std::uint64_t well_field_history_retired_through_=0;
    bool stationary_well_npc_locked(std::uint32_t id)const{
        return !quest_npc_motion_.contains(id)&&!npc_flare_ai_.contains(id)&&
               !npc_elemental_ai_.contains(id);
    }
    // Shared allied-NPC environmental/Health-gain team authority. A generic
    // relation is not an invented PvP team; owned scopes must remain current.
    std::optional<std::uint64_t> npc_ally_team_locked(skills::effects::Scope scope)const{
        const auto live=npc_combat_.find(scope.actor);const auto body=npcs_.find(scope.actor);
        if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=scope||!body->second.combat_definition)return {};
        const auto relation=body->second.combat_definition->relation;
        if(relation!=combat::npcs::Relation::allied_to_players&&relation!=combat::npcs::Relation::protected_ally)return {};
        const auto owner_team=[&](skills::effects::Scope owner)->std::optional<std::uint64_t>{
            const auto player=players_.find(owner.actor);
            if(player==players_.end()||player->second.effect_scope!=owner||!player->second.team_token)return {};
            return player->second.team_token;
        };
        if(const auto pet=pet_owners_.find(scope);pet!=pet_owners_.end())return owner_team(pet->second);
        if(const auto owned=npc_opposed_allies_.find(scope);owned!=npc_opposed_allies_.end()){
            const auto team=owner_team(owned->second.owner);if(!team)return {};
            const auto& player=players_.at(owned->second.owner.actor);
            const auto objective=player.quest_kills.find(owned->second.quest);
            return objective!=player.quest_kills.end()&&objective->second.cycle==owned->second.cycle?team:std::nullopt;
        }
        if(const auto owned=quest_npc_owners_.find(scope.actor);owned!=quest_npc_owners_.end()){
            const auto team=owner_team(owned->second.first);if(!team)return {};
            return players_.at(owned->second.first.actor).quest_kills.contains(owned->second.second)?team:std::nullopt;
        }
        std::optional<std::uint64_t> team;
        for(const auto& [id,player]:players_)if(player.team_token){
            if(team&&*team!=player.team_token)return {};team=player.team_token;
        }
        return team;
    }
    // Shared mixed player/NPC relation. Unowned globally-friendly bodies keep
    // their established domain; an explicit current owner proves one team only.
    std::optional<bool> npc_player_alliance_locked(skills::effects::Scope npc_scope,
            skills::effects::Scope player_scope)const{
        const auto npc=npc_combat_.find(npc_scope.actor);const auto player=players_.find(player_scope.actor);
        if(npc_scope.instance!=player_scope.instance||npc==npc_combat_.end()||
           npc->second.hex_scope()!=npc_scope||player==players_.end()||player->second.effect_scope!=player_scope||
           !player->second.resources||player->second.precise_health<=0)return {};
        const auto life=npc->second.lifetime();const auto actor=npc->second.actor(life);
        if(!actor||!actor->health||actor->health->current<=0)return {};
        if(npc->second.hostile_to_players(life))return false;
        if(!npc->second.healable_ally(life))return {};
        const bool owned=pet_owners_.contains(npc_scope)||npc_opposed_allies_.contains(npc_scope)||
            quest_npc_owners_.contains(npc_scope.actor);
        if(!owned)return true;
        const auto team=npc_ally_team_locked(npc_scope);
        if(!team||!player->second.team_token)return {};
        return *team==player->second.team_token;
    }
    bool well_affects_npc_locked(const skills::effects::WellField& field,const Npc& npc)const{
        if(!npc.combat_definition||!npc.combat_definition->damageable||npc.plane!=field.plane||
           std::hypot(double(npc.x)-field.x,double(npc.y)-field.z)>field.plan.radius)return false;
        const auto live=npc_combat_.find(npc.id);
        if(live==npc_combat_.end())return false;
        const auto scope=live->second.hex_scope();
        const auto relation=npc.combat_definition->relation;
        const bool ally=relation==combat::npcs::Relation::allied_to_players||relation==combat::npcs::Relation::protected_ally;
        const bool hostile=relation==combat::npcs::Relation::hostile_to_players;
        if(!ally&&!hostile)return false;
        if(ally){
            const auto team=npc_ally_team_locked(scope);
            return team&&(field.plan.affects_allies?*team==field.team:*team!=field.team);
        }
        const auto owner_team=[&](skills::effects::Scope owner)->std::optional<std::uint64_t>{
            const auto player=players_.find(owner.actor);
            if(player==players_.end()||player->second.effect_scope!=owner||!player->second.team_token)return {};
            return player->second.team_token;
        };
        std::optional<std::uint64_t> team;
        if(const auto pet=pet_owners_.find(scope);pet!=pet_owners_.end()){
            team=owner_team(pet->second);if(!team)return false;
        }else if(const auto owned=npc_opposed_allies_.find(scope);owned!=npc_opposed_allies_.end()){
            team=owner_team(owned->second.owner);if(!team)return false;
            const auto& player=players_.at(owned->second.owner.actor);
            const auto objective=player.quest_kills.find(owned->second.quest);
            if(objective==player.quest_kills.end()||objective->second.cycle!=owned->second.cycle)return false;
        }else if(const auto owned=quest_npc_owners_.find(npc.id);owned!=quest_npc_owners_.end()){
            const auto current=owner_team(owned->second.first);if(!current)return false;
            const auto& player=players_.at(owned->second.first.actor);
            if(!player.quest_kills.contains(owned->second.second))return false;
            if(hostile){
                // A quest foe is proved opposed to its current owner only.
                // An unrelated PvP side is not automatically that foe's ally.
                return field.team==*current&&!field.plan.affects_allies;
            }
            team=*current;
        }
        if(team)return field.plan.affects_allies?*team==field.team:*team!=field.team;
        // Allied recipients already returned through the shared resolver.
        return !field.plan.affects_allies;
    }
    void strip_well_enchantments_locked(Player& player,std::uint64_t now){
        if(!player.effect_registry||player.precise_health<=0)return;
        std::vector<std::uint32_t> enchanted;
        for(const auto& entry:player.effect_registry->active(player.effect_scope,now))
            if(entry.kind==skills::effects::Category::enchantment)
                enchanted.push_back(entry.instance);
        for(const auto effect:enchanted)remove_health_effect_locked(player,effect,now);
        // A real end-effect gain/heal can settle lethal wound debt. Complete
        // death cleanup after the strip cohort so pending effect IDs are not
        // removed twice by a nested death retirement.
        if(player.precise_health<=0)kill_locked(player,now);
    }
    bool well_blocks_player_enchantment_locked(const Player& player,std::uint64_t now)const{
        if(!player.team_token)return false;
        const auto point=healing_position_locked(player,now);
        if(!point)return false;
        for(const auto& field:well_fields_)
            if(skills::effects::well_field_effect(field,player.surface.plane,
                point->x,point->z,player.team_token,now).block_enchantments)return true;
        return false;
    }
    bool well_blocks_npc_enchantment_locked(std::uint32_t id,std::uint64_t now)const{
        const auto placed=npcs_.find(id);
        if(placed==npcs_.end())return false;
        for(const auto& field:well_fields_)
            if(field.plan.block_enchantments&&field.begins<=now&&now<field.expires&&
               well_affects_npc_locked(field,placed->second))return true;
        return false;
    }
public:
    // A completed Well cast selects a corpse from authoritative World state.
    // The caller publishes the paid cast and field inside this same lock;
    // returning false leaves the corpse available for another cast.
    bool claim_well_corpse(skills::effects::Scope caster,
                           const skills::effects::WellCastPlan& plan,
                           std::uint64_t attempt,std::uint64_t now,float range,
                           const std::function<bool(const skills::effects::WellCorpseClaim&)>& commit){
        if(!commit||!attempt||!std::isfinite(range)||range<=0||range>1248||!now||
           skills::effects::well_cast_plan(plan.skill,plan.rank)!=plan)return false;
        std::lock_guard lock(mutex_);
        const auto source=players_.find(caster.actor);
        if(source==players_.end()||source->second.effect_scope!=caster||
           !source->second.resources||source->second.precise_health<=0||
           !source->second.team_token||source->second.knocked_down_until>now)return false;
        // The live World clock cannot run a new cast before a previously
        // published field; retire expired fields before applying the cap.
        for(const auto& field:well_fields_)if(field.begins>now)return false;
        std::erase_if(well_fields_,[&](const auto& field){
            if(field.expires>now)return false;
            // Delayed Health/Energy ticks must retain the entire field until
            // every current player has settled past its end. NPC consumers
            // will join this watermark when their Well effects are wired.
            for(const auto& [id,player]:players_)if(player.resources&&
                (player.health_updated<field.expires||player.energy_updated<field.expires))return false;
            for(const auto& [id,live]:npc_combat_){
                const auto placed=npcs_.find(id);if(placed==npcs_.end()||!stationary_well_npc_locked(id)||
                    !well_affects_npc_locked(field,placed->second))continue;
                const auto life=live.lifetime();
                const auto* actor=live.actor(life);
                if(actor&&actor->health&&actor->health->current>0&&
                   live.health_clock(life).value_or(0)<field.expires)return false;
            }
            well_field_history_retired_through_=std::max(well_field_history_retired_through_,field.expires);
            return true;
        });
        if(well_fields_.size()>=4096)return false;
        const auto origin=healing_position_locked(source->second,now);
        if(!origin)return false;
        std::optional<skills::effects::WellCorpseClaim> selected;
        float best=range;
        for(const auto& [id,body]:npcs_){
            if(body.plane!=source->second.surface.plane)continue;
            const auto live=npc_combat_.find(id);
            if(live==npc_combat_.end())continue;
            const auto life=live->second.lifetime();
            const auto* actor=live->second.actor(life);
            if(!actor||!actor->damageable||!actor->health||actor->health->current!=0||
               live->second.fleshy(life)!=true)continue;
            const skills::effects::Scope corpse{instance_scope_,id,life.actor_generation};
            if(!corpse_reservations_.available(corpse))continue;
            const float distance=std::hypot(origin->x-body.x,origin->z-body.y);
            if(distance>best)continue;
            const skills::effects::WellCorpseFact fact{corpse,1,body.plane,body.x,body.y,true,true,false};
            const auto claim=skills::effects::prepare_well_corpse_claim(plan,caster,fact,true,true,now);
            if(!claim)continue;
            if(selected&&distance==best&&id>selected->corpse.actor)continue;
            best=distance;selected=claim;
        }
        if(!selected)return false;
        const auto field=skills::effects::create_well_field(*selected,source->second.team_token);
        if(!field)return false;
        well_fields_.reserve(well_fields_.size()+1);
        if(!corpse_reservations_.reserve_well(selected->corpse,caster,plan.skill,attempt))return false;
        bool published=false;
        try{published=commit(*selected);}catch(...){
            corpse_reservations_.release(selected->corpse,{caster,0,0,plan.skill,attempt});
            throw;
        }
        if(!published){
            corpse_reservations_.release(selected->corpse,{caster,0,0,plan.skill,attempt});
            return false;
        }
        // Settle the old timeline before this field begins. In particular a
        // timed NPC Energy curve must not gain Well pips retroactively.
        for(auto& [id,live]:npc_combat_){
            const auto placed=npcs_.find(id);
            if(placed!=npcs_.end()&&stationary_well_npc_locked(id)&&
               well_affects_npc_locked(*field,placed->second)){
                advance_npc_conditions_locked(id,live,now);
                live.advance_energy(live.lifetime(),now);
                live.begin_external_health(live.lifetime(),now);
            }
        }
        // Commit corpse ownership before publishing the field or stripping any
        // enchantments. A failed reservation must not leave an active Well.
        if(!corpse_reservations_.consume_well(selected->corpse,caster,plan.skill,attempt))return false;
        // Appending the already-validated field cannot allocate after reserve().
        well_fields_.push_back(*field);
        if(field->plan.strip_enchantments){
            for(auto& [id,player]:players_){
                if(!player.effect_registry||!player.resources||player.precise_health<=0)continue;
                const auto position=healing_position_locked(player,now);
                if(!position||!skills::effects::well_field_effect(*field,player.surface.plane,
                    position->x,position->z,player.team_token,now).strip_enchantments)continue;
                if(!advance_health_locked(player,now)||player.precise_health<=0)continue;
                strip_well_enchantments_locked(player,now);
            }
            for(auto& [id,live]:npc_combat_){
                const auto placed=npcs_.find(id);
                if(placed==npcs_.end()||!stationary_well_npc_locked(id)||
                   !well_affects_npc_locked(*field,placed->second))continue;
                const auto life=live.lifetime();
                const auto actor=live.actor(life);
                const auto registry=live.owned_effect_snapshot(life);
                if(!actor||!actor->health||actor->health->current<=0||!registry)continue;
                for(const auto& entry:registry->active(live.hex_scope(),now))
                    if(entry.kind==skills::effects::Category::enchantment&&
                       combat::npcs::LiveActor::reviewed_enchantment_end_skill(entry.skill))
                        remove_npc_enchantment_locked(live,life,entry.instance,now);
            }
        }
        return true;
    }
    // The verified 2022 ground trio is available for Wells91..94.
    // Build it while the corpse is exclusively reserved, then expose it to
    // Runtime only after World has consumed the corpse and installed the field.
    enum class WellCastOutcome {rejected,failed_roll,created};
    struct WellCastResult {
        WellCastOutcome outcome=WellCastOutcome::rejected;
        std::vector<Packet> visuals;
        bool completed()const{return outcome!=WellCastOutcome::rejected;}
    };
    WellCastResult complete_well_cast(skills::effects::Scope caster,
                                      const skills::effects::WellCastPlan& plan,
                                      std::uint64_t attempt,std::uint64_t now,float range,
                                      std::optional<double> roll={}){
        const auto field_plan=skills::effects::well_field_plan(plan.skill,plan.rank);
        if(!field_plan||field_plan->cast!=plan||(roll&&(!std::isfinite(*roll)||*roll<0||*roll>=1))||
           (field_plan->low_rank_failure_percent&&!roll))return {};
        std::vector<Packet> staged;
        bool failed_roll=false;
        const bool accepted=claim_well_corpse(caster,plan,attempt,now,range,
            [&](const skills::effects::WellCorpseClaim& corpse){
                if(field_plan->low_rank_failure_percent&&
                   *roll<double(field_plan->low_rank_failure_percent)/100.){
                    failed_roll=true;return false;
                }
                staged=skills::particles::well_ground_2022(plan.skill,caster.actor,
                    corpse.x,corpse.z,static_cast<std::uint16_t>(corpse.plane));
                return staged.size()==3;
            });
        if(failed_roll)return {WellCastOutcome::failed_roll,{}};
        if(!accepted)return {};
        return {WellCastOutcome::created,std::move(staged)};
    }
    std::optional<skills::effects::WellFieldEffect> well_field_effect_at(
            unsigned plane,float x,float z,std::uint64_t team,std::uint64_t now)const{
        if(!team||!std::isfinite(x)||!std::isfinite(z))return {};
        std::lock_guard lock(mutex_);
        skills::effects::WellFieldEffect total;
        for(const auto& field:well_fields_){
            const auto current=skills::effects::well_field_effect(field,plane,x,z,team,now);
            total.health_pips+=current.health_pips;
            total.energy_pips+=current.energy_pips;
            total.strip_enchantments|=current.strip_enchantments;
            total.block_enchantments|=current.block_enchantments;
        }
        return total;
    }
    struct WellExposure {
        double begins=0,ends=0; // Authoritative World clock, milliseconds.
        skills::effects::WellFieldEffect effect;
    };
private:
    std::optional<std::vector<WellExposure>> well_player_exposure_locked(
            const Player& p,std::uint64_t from,std::uint64_t through)const{
        if(from>=through||through>9007199254740991ULL)return {};
        if(from<well_field_history_retired_through_)return {};
        if(!p.resources||!p.team_token||p.steering||from<p.last_move||
           (p.moving&&!foes_wards_.empty())||p.surface.plane!=p.target_surface.plane)return {};
        for(const auto& segment:p.route)if(segment.surface.plane!=p.surface.plane)return {};
        std::vector<std::uint64_t> cuts{from,through};
        const auto cut=[&](std::uint64_t at){if(from<at&&at<through)cuts.push_back(at);};
        if(p.moving){
            cut(p.started);cut(p.ends);
            auto at=p.ends;navigation::Point previous{p.tx,p.ty};
            for(const auto& segment:p.route){
                const auto distance=std::hypot(segment.point.x-previous.x,segment.point.z-previous.z);
                const auto duration=distance<.01f?0:std::max<std::uint64_t>(1,
                    static_cast<std::uint64_t>(std::ceil(distance/
                        (movement::player_run_speed*p.movement_multiplier)*1000)));
                if(duration>UINT64_MAX-at)return {};
                at+=duration;cut(at);previous=segment.point;
            }
        }
        std::sort(cuts.begin(),cuts.end());cuts.erase(std::unique(cuts.begin(),cuts.end()),cuts.end());
        std::vector<WellExposure> result;
        for(std::size_t i=1;i<cuts.size();++i){
            const auto a=cuts[i-1],b=cuts[i];
            const auto start=healing_position_locked(p,a),end=healing_position_locked(p,b);
            if(!start||!end)return {};
            const skills::effects::WellRouteLeg leg{static_cast<unsigned>(p.surface.plane),
                start->x,start->z,end->x,end->z,a,b};
            for(const auto& segment:skills::effects::well_route_segments(well_fields_,leg,p.team_token)){
                const double base=static_cast<double>(a),duration=static_cast<double>(b-a);
                result.push_back({base+segment.enter*duration,
                                  base+segment.leave*duration,segment.effect});
            }
        }
        return result;
    }
public:
    std::optional<std::vector<WellExposure>> well_player_exposure(
            skills::effects::Scope actor,std::uint64_t from,std::uint64_t through)const{
        std::lock_guard lock(mutex_);
        const auto found=players_.find(actor.actor);
        if(found==players_.end()||found->second.effect_scope!=actor)return {};
        return well_player_exposure_locked(found->second,from,through);
    }
