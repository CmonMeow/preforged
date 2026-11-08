// Included inside Instance. Ground wards are stationary environmental effects;
// they do not follow an actor or manufacture enchantment/Health authorities.
private:
    std::vector<skills::effects::DefensiveGroundWard> defensive_wards_;
    bool ward_knockdown_protection_locked(skills::effects::Scope scope,std::uint64_t now)const{
        const auto state=ward_recipient_locked(scope,now);if(!state)return false;
        const auto& r=state->recipient;
        const skills::effects::DefensiveWardRecipient recipient{scope,state->team,r.plane,r.x,r.y,true,false};
        return std::any_of(defensive_wards_.begin(),defensive_wards_.end(),[&](const auto& ward){
            return skills::effects::defensive_ward_prevents_knockdown(ward,recipient,now);
        });
    }
    void expire_defensive_wards_locked(std::uint64_t now){
        for(const auto& ward:defensive_wards_)if(ward.expires<=now){
            const auto source=players_.find(ward.caster.actor);
            if(source!=players_.end()&&source->second.effect_scope==ward.caster)
                for(auto&[id,peer]:players_)peer.outbox->enqueue({health_effect_remove(ward.caster.actor,ward.effect)});
        }
        std::erase_if(defensive_wards_,[&](const auto& ward){return ward.expires<=now;});
    }
    struct OwnedElementWard {skills::effects::WardAgainstElements ward;skills::effects::Scope source;std::uint64_t team=0;unsigned plane=0,effect=0;};
    std::vector<OwnedElementWard> elemental_wards_;
    struct WardRecipientState {skills::effects::WardMeleeRecipient recipient;std::uint64_t team=0;};
    std::optional<WardRecipientState> ward_recipient_locked(skills::effects::Scope scope,std::uint64_t now)const{
        float x=0,y=0;unsigned plane=0;std::uint64_t team=0;bool npc_ally=false;
        if(const auto p=players_.find(scope.actor);p!=players_.end()){
            if(p->second.effect_scope!=scope||!p->second.resources||p->second.precise_health<=0)return {};
            const auto position=healing_position_locked(p->second,now);if(!position)return {};
            x=position->x;y=position->z;plane=p->second.surface.plane;team=p->second.team_token;
        }else{
            const auto n=npc_combat_.find(scope.actor);const auto placed=npcs_.find(scope.actor);
            if(n==npc_combat_.end()||placed==npcs_.end()||n->second.hex_scope()!=scope)return {};
            const auto life=n->second.lifetime();const auto* actor=n->second.actor(life);const auto type=n->second.creature_type(life);
            if(!actor||!actor->health||actor->health->current<=0||!type||*type=="Spirit"||*type=="spirit")return {};
            npc_ally=n->second.healable_ally(life);if(!npc_ally)return {};
            if(const auto owned=npc_opposed_allies_.find(scope);owned!=npc_opposed_allies_.end()){
                const auto owner=players_.find(owned->second.owner.actor);
                if(owner==players_.end()||owner->second.effect_scope!=owned->second.owner)return {};
                team=owner->second.team_token;
            }else{
                // Global allied-to-players metadata has no PvP team. Grant
                // only an unambiguous current player side; never guess which
                // opposing player's ward belongs to this unowned NPC.
                for(const auto&[id,player]:players_)if(player.team_token){
                    if(team&&team!=player.team_token)return {};team=player.team_token;
                }
            }
            x=placed->second.x;y=placed->second.y;plane=placed->second.plane;
        }
        return WardRecipientState{{scope,plane,x,y,true,false,true},team};
    }
    unsigned ward_elemental_bonus_locked(skills::effects::Scope scope,std::uint64_t now)const{
        const auto state=ward_recipient_locked(scope,now);if(!state)return 0;
        const auto& p=state->recipient;
        for(const auto& owned:elemental_wards_)if(owned.plane==p.plane&&
            skills::effects::ward_covers(owned.ward,instance_scope_,p.x,p.y,state->team&&state->team==owned.team,false,now))return 24;
        return 0;
    }
    struct OwnedMeleeWard {skills::effects::WardAgainstMelee ward;std::uint64_t team=0;};
    std::vector<OwnedMeleeWard> melee_wards_;
    struct OwnedFoesWard {skills::effects::WardAgainstFoes ward;std::uint64_t team=0;bool removal_published=false;};
    std::vector<OwnedFoesWard> foes_wards_;
    // This is a consumed-history watermark, never the wall clock. World may
    // advance it only after every player/NPC movement producer has consumed
    // its interval, including inactive routes and pending sparse ticks.
    std::uint64_t foes_ward_history_retired_through_=0;
    void retire_foes_ward_history_locked(std::uint64_t through){
        foes_ward_history_retired_through_=std::max(foes_ward_history_retired_through_,through);
        std::erase_if(foes_wards_,[&](const auto& owned){return owned.removal_published&&
            owned.ward.expires<=foes_ward_history_retired_through_;});
    }
    std::optional<std::vector<skills::effects::WardAgainstFoes>> eligible_foes_wards_locked(
            skills::effects::Scope scope,std::uint64_t from,std::uint64_t through)const{
        if(through<from||from<foes_ward_history_retired_through_)return {};
        std::vector<skills::effects::WardAgainstFoes> result;
        for(const auto& owned:foes_wards_){
            if(owned.ward.expires<=from||owned.ward.created>through)continue;
            // Position is immaterial here: the route adapter owns historical
            // positions and plane boundaries; World supplies authority only.
            if(ward_foe_recipient_locked(scope,through,owned.team))result.push_back(owned.ward);
        }
        return result;
    }
    std::optional<skills::effects::WardFoeRecipient> ward_foe_recipient_locked(
            skills::effects::Scope scope,std::uint64_t now,std::uint64_t caster_team)const{
        if(!caster_team)return {};
        if(const auto p=players_.find(scope.actor);p!=players_.end()){
            if(p->second.effect_scope!=scope||!p->second.resources||p->second.precise_health<=0||
               !p->second.team_token||p->second.team_token==caster_team)return {};
            const auto position=healing_position_locked(p->second,now);if(!position)return {};
            return skills::effects::WardFoeRecipient{scope,static_cast<unsigned>(p->second.surface.plane),
                position->x,position->z,true,false,true};
        }
        const auto n=npc_combat_.find(scope.actor);
        const auto placed=npcs_.find(scope.actor);
        if(n==npc_combat_.end()||placed==npcs_.end()||n->second.hex_scope()!=scope)return {};
        const auto life=n->second.lifetime();
        const auto actor=n->second.actor(life);
        const auto type=n->second.creature_type(life);
        if(!actor||!actor->health||actor->health->current<=0||!type||*type=="Spirit"||*type=="spirit"||
           !n->second.hostile_to_players(life))return {};
        return skills::effects::WardFoeRecipient{scope,placed->second.plane,
            placed->second.x,placed->second.y,true,false,true};
    }
    std::optional<float> ward_foes_multiplier_locked(skills::effects::Scope scope,std::uint64_t now)const{
        // Multiple overlapping wards confer one slow. World movement will
        // consume this point query at every boundary once its route adapter is
        // integrated; merely storing a field must not claim a live slow.
        for(const auto& owned:foes_wards_){
            const auto recipient=ward_foe_recipient_locked(scope,now,owned.team);
            if(recipient)if(const auto factor=skills::effects::ward_foes_movement_multiplier(
                    owned.ward,*recipient,now))return factor;
        }
        return {};
    }
    std::optional<skills::effects::BlockSource> ward_melee_source_locked(skills::effects::Scope scope,std::uint64_t now)const{
        const auto state=ward_recipient_locked(scope,now);if(!state||!state->team)return {};
        // Provisional private policy: one Ward176, earliest insertion first;
        // environmental source follows personal block effects. No retail
        // overlap/attribution ordering is asserted. Scope namespaces its ID.
        for(const auto& owned:melee_wards_)if(owned.team==state->team)
            if(const auto source=skills::effects::ward_melee_block_source(owned.ward,state->recipient,combat::weapon::AttackDelivery::melee,now))return source;
        return {};
    }
    void expire_melee_wards_locked(std::uint64_t now){
        for(const auto& owned:melee_wards_)if(owned.ward.expires<=now){const auto p=players_.find(owned.ward.caster.actor);
            if(p!=players_.end()&&p->second.effect_scope==owned.ward.caster)for(auto&[id,peer]:players_)peer.outbox->enqueue({health_effect_remove(owned.ward.caster.actor,owned.ward.effect)});}
        std::erase_if(melee_wards_,[&](const auto& owned){return owned.ward.expires<=now;});
    }
    void expire_foes_wards_locked(std::uint64_t now){
        for(auto& owned:foes_wards_)if(owned.ward.expires<=now&&!owned.removal_published){
            const auto p=players_.find(owned.ward.caster.actor);
            if(p!=players_.end()&&p->second.effect_scope==owned.ward.caster)
                for(auto&[id,peer]:players_)peer.outbox->enqueue({health_effect_remove(
                    owned.ward.caster.actor,owned.ward.effect)});
            owned.removal_published=true;
        }
        // Expiry ends publication; sparse movement may still query an earlier
        // interval. A stalled consumer retains history and the 4096 cast cap
        // fails closed instead of evicting fields it has not consumed.
        retire_foes_ward_history_locked(foes_ward_history_retired_through_);
    }
    void bind_npc_ward_queries_locked(){
        // LiveActor's contract already forbids escaping the Instance mutex.
        // Copies used for preflight retain this read-only current World query.
        for(auto&[id,actor]:npc_combat_){
            actor.environmental_elemental_armor([this](auto scope,auto at){return ward_elemental_bonus_locked(scope,at);});
            actor.environmental_knockdown_protection([this](auto scope,auto at){return ward_knockdown_protection_locked(scope,at);});
        }
    }
    void expire_elemental_wards_locked(std::uint64_t now){
        for(const auto& owned:elemental_wards_)if(owned.ward.expires<=now){const auto p=players_.find(owned.source.actor);
            if(p!=players_.end()&&p->second.effect_scope==owned.source)for(auto&[id,peer]:players_)peer.outbox->enqueue({health_effect_remove(owned.source.actor,owned.effect)});}
        std::erase_if(elemental_wards_,[&](const auto& owned){return owned.ward.expires<=now;});
    }
public:
    bool ward_knockdown_protection(skills::effects::Scope scope,std::uint64_t now){
        std::lock_guard lock(mutex_);
        // Public event-time observation materializes a retained NPC route,
        // like position(id,now). The LiveActor callback remains read-only;
        // control/preflight callers settle movement before invoking it.
        if(const auto npc=npc_combat_.find(scope.actor);npc!=npc_combat_.end()&&npc->second.hex_scope()==scope)
            refresh_npc_movement_locked(scope.actor,now);
        return ward_knockdown_protection_locked(scope,now);
    }
    bool complete_defensive_ward(skills::effects::Scope source,const skills::effects::DefensiveWardPlan& plan,
            std::uint64_t event,std::uint64_t now,std::vector<Packet>* visuals=nullptr){
        if(visuals)visuals->clear();if(!event)return false;
        return apply_defensive_ward(source,plan,now,visuals,event);
    }
    // Direct field staging remains available for native policy fixtures. Normal
    // completion must supply its admitted event through complete_defensive_ward.
    bool apply_defensive_ward(skills::effects::Scope source,const skills::effects::DefensiveWardPlan& plan,
            std::uint64_t now,std::vector<Packet>* visuals=nullptr,std::optional<std::uint64_t> event={}){
        if(visuals)visuals->clear();
        const auto canonical=skills::effects::defensive_ground_ward_plan(plan.skill,plan.rank);
        if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);if(!settle_timed_effects_locked(now))return false;
        if(event&&!complete_ether_renewal_locked(source,plan.skill,*event,now,true,false))return false;
        std::optional<skills::effects::DefensiveGroundWard> ward;
        const auto prepare=[&]()->bool{
            ward.reset();auto p=players_.find(source.actor);
            if(p==players_.end()||p->second.effect_scope!=source||!p->second.team_token||!p->second.resources||
               p->second.health_updated>now||!advance_health_locked(p->second,now)||p->second.precise_health<=0||
               p->second.knocked_down_until>now||p->second.next_health_effect>=UINT32_MAX||
               p->second.surface.plane>65535||defensive_wards_.size()>=4096)return false;
            // Reactive resource settlement may change the owning lifetime.
            p=players_.find(source.actor);if(p==players_.end()||p->second.effect_scope!=source)return false;
            const auto position=healing_position_locked(p->second,now);if(!position)return false;
            ward=skills::effects::defensive_ground_ward(source,static_cast<unsigned>(p->second.next_health_effect),
                p->second.team_token,static_cast<unsigned>(p->second.surface.plane),position->x,position->z,plan,now);
            return bool(ward);
        };
        if(!prepare())return false;
        if(event&&!complete_ether_renewal_locked(source,plan.skill,*event,now,true))return false;
        if(event&&!prepare())return true; // Reward consumed the completion but killed/retired its owner.
        const auto& field=*ward;
        const skills::effects::Entry timer{field.effect,source.actor,plan.skill,plan.rank,skills::effects::Category::other,now,field.expires};
        std::vector<Packet> packets{skills::particles::ward_stability_ground_2022(field.x,field.y,
            static_cast<std::uint16_t>(field.plane)),health_effect_add(timer,source.actor,now)};
        // Same fixed field policy as Ward175/176: caster departure/death does
        // not move or erase it. Timer cleanup still requires the old scope.
        defensive_wards_.push_back(field);++players_.at(source.actor).next_health_effect;
        if(visuals)*visuals=std::move(packets);else for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
        return true;
    }
    std::optional<std::vector<skills::effects::WardAgainstFoes>> ward_foes_history(
            skills::effects::Scope scope,std::uint64_t from,std::uint64_t through)const{
        std::lock_guard lock(mutex_);return eligible_foes_wards_locked(scope,from,through);
    }
    std::optional<float> ward_foes_multiplier(skills::effects::Scope scope,std::uint64_t now)const{
        std::lock_guard lock(mutex_);return ward_foes_multiplier_locked(scope,now);
    }
    bool apply_ward_foes(skills::effects::Scope source,const skills::effects::WardFoesPlan& plan,
            std::uint64_t now,std::vector<Packet>* visuals=nullptr){
        if(visuals)visuals->clear();const auto canonical=skills::effects::ward_foes_plan(plan.rank);
        if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);if(!settle_timed_effects_locked(now))return false;
        const auto p=players_.find(source.actor);
        if(p==players_.end()||p->second.effect_scope!=source||!p->second.team_token||!p->second.resources||
           p->second.health_updated>now||!advance_health_locked(p->second,now)||
           p->second.precise_health<=0||p->second.knocked_down_until>now||
           p->second.next_health_effect>=UINT32_MAX||p->second.surface.plane>65535||
           foes_wards_.size()>=4096)return false;
        auto& caster=p->second;const auto position=healing_position_locked(caster,now);if(!position)return false;
        const auto effect=static_cast<unsigned>(caster.next_health_effect);
        const auto ward=skills::effects::ward_against_foes(source,effect,plan.rank,
            static_cast<unsigned>(caster.surface.plane),position->x,position->z,now);
        if(!ward)return false;
        const skills::effects::Entry timer{effect,source.actor,177,plan.rank,
            skills::effects::Category::other,now,ward->expires};
        std::vector<Packet> packets{skills::particles::positional_effect_2022(
            source.actor,900,position->x,position->z,static_cast<std::uint16_t>(caster.surface.plane)),
            health_effect_add(timer,source.actor,now)};
        // A field can appear during an already scheduled click leg. Settle the
        // old path before publication, then schedule its remaining distance
        // against the new stationary boundary; the old ETA is no longer valid.
        std::vector<std::uint32_t> moving_foes;
        for(auto& [id,peer]:players_)if(peer.moving&&now>=peer.started&&now<peer.ends&&
            peer.team_token&&peer.team_token!=caster.team_token){
            materialize_click_position(peer,now);moving_foes.push_back(id);
        }
        std::vector<std::uint32_t> steering_foes;
        for(auto&[id,peer]:players_)if(peer.steering&&peer.team_token&&peer.team_token!=caster.team_token){
            // Motion before the cast belongs to the old field set. Do not
            // let a later sparse held-input update retroactively slow it.
            advance_steering(peer,now);steering_foes.push_back(id);
        }
        foes_wards_.push_back({*ward,caster.team_token});++caster.next_health_effect;
        for(const auto id:moving_foes){
            auto& peer=players_.at(id);peer.sx=peer.x;peer.sy=peer.y;peer.started=now;
            peer.ends=now+click_segment_duration_locked(peer,now);
            const auto factor=ward_foes_multiplier_locked(peer.effect_scope,now).value_or(1.f);
            peer.ward_click_published_factor=factor;
            packets.push_back(position_packet(id,peer.x,peer.y,static_cast<std::uint16_t>(peer.surface.plane)));
            const auto speed=movement::ward_foes_player_speed_2022(id,
                peer.movement_effects.at_with_external(now,factor),1.f,0);
            packets.insert(packets.end(),speed.begin(),speed.end());
            packets.push_back(destination_packet(id,peer.tx,peer.ty,
                static_cast<std::uint16_t>(peer.target_surface.plane),static_cast<std::uint16_t>(peer.surface.plane)));
        }
        for(const auto id:steering_foes){
            auto& peer=players_.at(id);
            const auto factor=ward_foes_multiplier_locked(peer.effect_scope,now).value_or(1.f);
            if(factor==peer.ward_steering_published_factor)continue;
            peer.ward_steering_published_factor=factor;
            packets.push_back(position_packet(id,peer.x,peer.y,
                static_cast<std::uint16_t>(peer.surface.plane)));
            const float gait=std::clamp(std::hypot(peer.velocity_x,peer.velocity_y)/
                (movement::player_run_speed*peer.movement_multiplier),.01f,1.f);
            const auto speed=movement::ward_foes_player_speed_2022(id,
                peer.movement_effects.at_with_external(now,factor),gait,
                static_cast<std::uint8_t>(peer.steering_direction));
            packets.insert(packets.end(),speed.begin(),speed.end());
            const auto ahead=steering_ahead(peer,peer.steering_clipped);
            if(ahead.size()>1)packets.push_back(ahead.back());
        }
        if(visuals)*visuals=std::move(packets);else for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
        return true;
    }
    std::optional<skills::effects::BlockSource> ward_melee_source(skills::effects::Scope scope,std::uint64_t now)const{
        std::lock_guard lock(mutex_);return ward_melee_source_locked(scope,now);
    }
    bool apply_ward_melee(skills::effects::Scope source,const skills::effects::WardMeleePlan&plan,std::uint64_t now,std::vector<Packet>* visuals=nullptr){
        if(visuals)visuals->clear();const auto canonical=skills::effects::ward_melee_plan(plan.rank);
        if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);if(!settle_timed_effects_locked(now))return false;
        const auto p=players_.find(source.actor);
        if(p==players_.end()||p->second.effect_scope!=source||!p->second.team_token||!p->second.resources||p->second.health_updated>now||
            !advance_health_locked(p->second,now)||p->second.precise_health<=0||p->second.knocked_down_until>now||p->second.next_health_effect>=UINT32_MAX||p->second.surface.plane>65535||melee_wards_.size()>=4096)return false;
        auto& caster=p->second;const auto position=healing_position_locked(caster,now);if(!position)return false;
        const auto effect=static_cast<unsigned>(caster.next_health_effect);
        const auto ward=skills::effects::ward_against_melee(source,effect,plan.rank,static_cast<unsigned>(caster.surface.plane),position->x,position->z,now);if(!ward)return false;
        const skills::effects::Entry timer{effect,source.actor,176,plan.rank,skills::effects::Category::other,now,ward->expires};
        std::vector<Packet> packets{skills::particles::positional_effect_2022(source.actor,900,position->x,position->z,static_cast<std::uint16_t>(caster.surface.plane)),health_effect_add(timer,source.actor,now)};
        // Same stationary retained-lifetime policy as Ward175, including source
        // departure/death; expiry is half-open and drained after due contacts.
        melee_wards_.push_back({*ward,caster.team_token});++caster.next_health_effect;
        if(visuals)*visuals=std::move(packets);else for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
        return true;
    }
    unsigned ward_elemental_armor(skills::effects::Scope scope,std::uint64_t now)const{
        std::lock_guard lock(mutex_);return ward_elemental_bonus_locked(scope,now);
    }
    bool apply_ward_elements(skills::effects::Scope source,const skills::effects::WardElementsPlan&plan,std::uint64_t now,std::vector<Packet>* visuals=nullptr){
        if(visuals)visuals->clear();const auto canonical=skills::effects::ward_elements_plan(plan.rank);
        if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);
        // Preserve historical footprints while any due damage consumer is
        // retained. Only the successful outer timed drain retires history.
        if(!settle_timed_effects_locked(now))return false;
        const auto p=players_.find(source.actor);
        if(p==players_.end()||p->second.effect_scope!=source||!p->second.team_token||!p->second.resources||p->second.health_updated>now||
            !advance_health_locked(p->second,now)||p->second.precise_health<=0||p->second.knocked_down_until>now||p->second.next_health_effect>=UINT32_MAX||p->second.surface.plane>65535)return false;
        auto& caster=p->second;const auto position=healing_position_locked(caster,now);if(!position)return false;
        const auto ward=skills::effects::ward_against_elements(instance_scope_,source.actor,source.generation,position->x,position->z,plan.rank,now);if(!ward)return false;
        if(elemental_wards_.size()>=4096)return false;
        const auto effect=static_cast<unsigned>(caster.next_health_effect);
        const skills::effects::Entry timer{effect,source.actor,175,plan.rank,skills::effects::Category::other,now,ward->expires};
        std::vector<Packet> packets{skills::particles::positional_effect_2022(source.actor,900,position->x,position->z,static_cast<std::uint16_t>(caster.surface.plane)),health_effect_add(timer,source.actor,now)};
        elemental_wards_.push_back({*ward,source,caster.team_token,static_cast<unsigned>(caster.surface.plane),effect});++caster.next_health_effect;
        if(visuals)*visuals=std::move(packets);else for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
        return true;
    }
private:


