// Included within Instance. Relocation component only: not a paid skill cast.
private:
    std::optional<navigation::SurfacePoint> shadow_position_locked(const Player& p,std::uint64_t now)const{
        if(!layered_||!resurrection_clock_valid(p,now))return {};
        if(p.steering){
            // Match advance_steering's rotating midpoint integration without
            // publishing intermediate goals or mutating the target player.
            navigation::SurfacePoint end{{p.x,p.y},p.surface};
            auto tick=p.steering_tick;float vx=p.velocity_x,vy=p.velocity_y;
            const auto elapsed=now-tick;
            const auto step=p.steering_turn_rate?std::max<std::uint64_t>(20,elapsed/64+(elapsed%64!=0)):elapsed;
            while(tick<now){
                const auto next=tick+std::min(step,now-tick);
                const float half=p.steering_turn_rate*float(next-tick)*.0005f;
                rotate_velocity(vx,vy,half);
                const auto motion=steering_motion_locked(end.surface,end.point,vx,vy,float(next-tick)/1000.f);
                if(!motion)break;
                end=motion->end;rotate_velocity(vx,vy,half);tick=next;
            }
            return end;
        }
        const auto point=healing_position_locked(p,now);if(!point)return {};
        auto surface=p.surface;
        navigation::Point previous{p.x,p.y};
        if(p.moving&&now>=p.ends){
            surface=p.target_surface;previous={p.tx,p.ty};auto starts=p.ends;
            for(const auto& next:p.route){
                const auto distance=std::hypot(next.point.x-previous.x,next.point.z-previous.z);
                if(distance<.01f){surface=next.surface;previous=next.point;continue;}
                const auto duration=std::max<std::uint64_t>(1,static_cast<std::uint64_t>(
                    std::ceil(distance/(movement::player_run_speed*p.movement_multiplier)*1000)));
                if(duration>UINT64_MAX-starts)return {};
                if(now<starts+duration)break;
                previous=next.point;surface=next.surface;starts+=duration;
            }
        }
        const auto final=layered_->trace(surface,previous,*point);
        if(!final||(navigation_plane_&&final->plane!=*navigation_plane_))return {};
        return navigation::SurfacePoint{*point,*final};
    }
public:
    struct ShadowStepReceipt {
        navigation::SurfacePoint origin,destination;
        bool clipped=false,blocked=false;
        Packet position;
        unsigned healing_quote=0;
        std::optional<BundleDropProposal> bundle_drop;
    };
    // Admission is read-only. Completion rechecks the live target, Health,
    // bundle and navigation under the same World authority after payment.
    bool death_charge_target_valid(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::DeathCharge& plan,std::uint64_t now)const{
        if(!skills::effects::death_charge(plan.rank)||*skills::effects::death_charge(plan.rank)!=plan)return false;
        std::lock_guard lock(mutex_);
        if(!navigation_||!layered_||source.instance!=target.instance||source.actor==target.actor)return false;
        const auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||!caster->second.resources||
           caster->second.precise_health<=0||caster->second.knocked_down_until>now)return false;
        const auto origin=shadow_position_locked(caster->second,now);if(!origin)return false;
        std::optional<navigation::SurfacePoint> goal;
        if(const auto foe=players_.find(target.actor);foe!=players_.end()){
            if(foe->second.effect_scope!=target||!foe->second.resources||foe->second.precise_health<=0||
               !caster->second.team_token||!foe->second.team_token||caster->second.team_token==foe->second.team_token)return false;
            goal=shadow_position_locked(foe->second,now);
        }else{
            const auto npc=npcs_.find(target.actor);
            const auto state=npc_combat_.find(target.actor);
            if(npc==npcs_.end()||state==npc_combat_.end()||state->second.hex_scope()!=target||
               !state->second.hostile_to_players(state->second.lifetime()))return false;
            const auto* live=state->second.actor(state->second.lifetime());
            if(!live||!live->health||live->health->current<=0)return false;
            const navigation::Point point{npc->second.x,npc->second.y};
            const auto cell=navigation_->locate_cell(npc->second.plane,point);if(!cell)return false;
            goal=navigation::SurfacePoint{point,{npc->second.plane,*cell}};
        }
        return goal&&std::hypot(double(goal->point.x)-origin->point.x,double(goal->point.z)-origin->point.z)<=plan.range&&
            bool(layered_->shadow_destination(origin->surface,origin->point,goal->surface,goal->point,plan.compass));
    }
    // Trusted completion component. Runtime combines its position packet with
    // the captured skill visuals and shared healing publication.
    std::optional<ShadowStepReceipt> death_charge_step(skills::effects::Scope scope,
            skills::effects::Scope target,const skills::effects::DeathCharge& plan,std::uint64_t now,
            std::span<const navigation::SurfaceLink> closed={}){
        std::lock_guard lock(mutex_);
        return death_charge_step_locked(scope,target,plan,now,closed);
    }
private:
    std::optional<ShadowStepReceipt> death_charge_step_locked(skills::effects::Scope scope,
            skills::effects::Scope target,const skills::effects::DeathCharge& plan,std::uint64_t now,
            std::span<const navigation::SurfaceLink> closed){
        const auto expected=skills::effects::death_charge(plan.rank);
        if(!expected||*expected!=plan)return {};
        if(!navigation_||!layered_)return {};
        settle_damage_fields_locked(now);
        const auto caster=players_.find(scope.actor);
        if(caster==players_.end()||caster->second.effect_scope!=scope||!caster->second.resources||
           !resurrection_clock_valid(caster->second,now)||caster->second.knocked_down_until>now||
           !advance_health_locked(caster->second,now)||caster->second.precise_health<=0)return {};
        auto& p=caster->second;
        if(holding_bundle_locked(scope)&&(!advance_resource_event_locked(p,now)||p.precise_health<=0))return {};
        const auto origin=shadow_position_locked(p,now);if(!origin)return {};
        std::optional<navigation::SurfacePoint> goal;
        std::optional<combat::Health> foe_health;
        if(const auto foe=players_.find(target.actor);foe!=players_.end()){
            auto& f=foe->second;
            if(f.effect_scope!=target||!f.resources||!p.team_token||!f.team_token||p.team_token==f.team_token||
               !resurrection_clock_valid(f,now)||!advance_health_locked(f,now)||f.precise_health<=0)return {};
            goal=shadow_position_locked(f,now);
            foe_health=combat::Health{f.precise_health,f.resources->max_health};
        }else{
            const auto npc=npcs_.find(target.actor);const auto state=npc_combat_.find(target.actor);
            if(npc==npcs_.end()||state==npc_combat_.end()||state->second.hex_scope()!=target||
               !state->second.hostile_to_players(state->second.lifetime())||
               state->second.health_clock(state->second.lifetime()).value_or(0)>now)return {};
            refresh_npc_movement_locked(target.actor,now);advance_npc_conditions_locked(target.actor,state->second,now);
            const auto* body=state->second.actor(state->second.lifetime());
            if(!body||!body->health||body->health->current<=0)return {};
            const navigation::Point point{npc->second.x,npc->second.y};
            const auto cell=navigation_->locate_cell(npc->second.plane,point);if(!cell)return {};
            goal=navigation::SurfacePoint{point,{npc->second.plane,*cell}};
            foe_health=*body->health;
        }
        if(!goal||std::hypot(double(goal->point.x)-origin->point.x,double(goal->point.z)-origin->point.z)>plan.range)return {};
        const auto heal=foe_health?skills::effects::death_charge_healing(plan,
            {p.precise_health,p.resources->max_health},*foe_health):std::nullopt;
        if(!heal)return {};
        const auto motion=layered_->shadow_destination(origin->surface,origin->point,goal->surface,goal->point,plan.compass,closed);
        if(!motion||motion->end.surface.plane>65535||(navigation_plane_&&motion->end.surface.plane!=*navigation_plane_))return {};
        ShadowStepReceipt result{*origin,motion->end,motion->clipped,motion->blocked,
            position_packet(p.id,motion->end.point.x,motion->end.point.z,static_cast<std::uint16_t>(motion->end.surface.plane)),*heal};
        result.bundle_drop=prepare_bundle_drop_locked(scope,now,bundles::Cause::shadow_step,*origin);
        if(!result.bundle_drop||!commit_bundle_drop_locked(*result.bundle_drop))return {};
        apply_shadow_position_locked(p,result.destination,now);
        return result;
    }
    void apply_shadow_position_locked(Player& p,const navigation::SurfacePoint& destination,std::uint64_t now){
        // Invalidate every source of pre-step movement and prediction credit.
        // Timed buffs, emotes, effect ownership and unrelated projectiles stay.
        p.heading=std::remainder(p.heading+p.turn_rate*float(now-p.turn_updated)/1000.f,6.283185307f);
        p.x=p.sx=p.tx=destination.point.x;p.y=p.sy=p.ty=destination.point.z;
        p.surface=p.target_surface=destination.surface;
        p.moving=p.steering=p.steering_clipped=p.steering_blocked=p.steering_server_guided=false;
        p.steering_turn_rate=p.turn_rate=p.velocity_x=p.velocity_y=0;
        p.started=p.ends=p.last_move=p.steering_tick=p.turn_updated=now;
        p.route.clear();p.following=0;cancel_npc_interaction(p);
        p.steering_checkpoint=destination;p.steering_checkpoint_active=false;
        p.steering_checkpoint_initialized=true;p.steering_checkpoint_budget=0;
    }
public:
    // Trusted relocation component only. Recall's owned end transaction must
    // retire upkeep and publish disable/visuals together before network binding.
    std::optional<ShadowStepReceipt> recall_step(const skills::effects::RecallAnchor& anchor,
            std::uint64_t now,std::span<const navigation::SurfaceLink> closed={}){
        std::lock_guard lock(mutex_);
        auto receipt=prepare_recall_step_locked(anchor,now,closed);
        if(receipt){
            if(!receipt->bundle_drop||!commit_bundle_drop_locked(*receipt->bundle_drop))return {};
            apply_shadow_position_locked(players_.at(anchor.caster.actor),receipt->destination,now);
        }
        return receipt;
    }
    std::optional<ShadowStepReceipt> recall_step_preview(const skills::effects::RecallAnchor& anchor,
            std::uint64_t now,std::span<const navigation::SurfaceLink> closed={}){
        std::lock_guard lock(mutex_);return prepare_recall_step_locked(anchor,now,closed);
    }
private:
    std::optional<ShadowStepReceipt> prepare_recall_step_locked(const skills::effects::RecallAnchor& anchor,
            std::uint64_t now,std::span<const navigation::SurfaceLink> closed){
        const auto plan=skills::effects::recall_shadow();
        if(!plan||!skills::effects::recall_scope(anchor.caster)||!skills::effects::recall_scope(anchor.target)||
           anchor.caster.instance!=anchor.target.instance||anchor.caster.actor==anchor.target.actor||
           !anchor.effect||anchor.applied>now)return {};
        if(!navigation_||!layered_)return {};
        settle_damage_fields_locked(now);
        const auto owner=players_.find(anchor.caster.actor);
        if(owner==players_.end()||owner->second.effect_scope!=anchor.caster||!owner->second.resources||
           !resurrection_clock_valid(owner->second,now)||!advance_health_locked(owner->second,now)||owner->second.precise_health<=0)return {};
        auto& p=owner->second;
        if(holding_bundle_locked(anchor.caster)&&(!advance_resource_event_locked(p,now)||p.precise_health<=0))return {};
        const auto origin=shadow_position_locked(p,now);if(!origin)return {};
        std::optional<navigation::SurfacePoint> goal;
        if(const auto ally=players_.find(anchor.target.actor);ally!=players_.end()){
            auto& target=ally->second;
            if(target.effect_scope!=anchor.target||!target.resources||!p.team_token||target.team_token!=p.team_token||
               !resurrection_clock_valid(target,now)||!advance_health_locked(target,now)||target.precise_health<=0)return {};
            goal=shadow_position_locked(target,now);
        }else{
            const auto npc_ally=npcs_.find(anchor.target.actor);const auto state=npc_combat_.find(anchor.target.actor);
            if(npc_ally==npcs_.end()||state==npc_combat_.end()||state->second.hex_scope()!=anchor.target||
               state->second.health_clock(state->second.lifetime()).value_or(0)>now)return {};
            refresh_npc_movement_locked(anchor.target.actor,now);advance_npc_conditions_locked(anchor.target.actor,state->second,now);
            if(!state->second.healable_ally(state->second.lifetime()))return {};
            const navigation::Point point{npc_ally->second.x,npc_ally->second.y};
            const auto cell=navigation_->locate_cell(npc_ally->second.plane,point);if(!cell)return {};
            goal=navigation::SurfacePoint{point,{npc_ally->second.plane,*cell}};
        }
        if(!goal||p.precise_health<=0||p.effect_scope!=anchor.caster)return {};
        // Recall explicitly returns when the tether crosses compass range.
        // Include the actual endpoint in the bounded topology query so the
        // first outside millisecond is not rejected as an ordinary cast target.
        const auto distance=std::hypot(double(goal->point.x)-origin->point.x,double(goal->point.z)-origin->point.z);
        const auto radius=std::nextafter(std::max(plan->compass,static_cast<float>(distance)),std::numeric_limits<float>::infinity());
        const auto motion=layered_->shadow_destination(origin->surface,origin->point,goal->surface,goal->point,radius,closed);
        if(!motion||motion->end.surface.plane>65535||(navigation_plane_&&motion->end.surface.plane!=*navigation_plane_))return {};
        ShadowStepReceipt result{*origin,motion->end,motion->clipped,motion->blocked,
            position_packet(p.id,motion->end.point.x,motion->end.point.z,static_cast<std::uint16_t>(motion->end.surface.plane)),0};
        result.bundle_drop=prepare_bundle_drop_locked(anchor.caster,now,bundles::Cause::shadow_step,*origin);
        if(!result.bundle_drop)return {};
        return result;
    }
public:
    // Staged completion: comparison uses current absolute Health at the
    // completion transaction, before shared healing modifiers/caps. Isolated
    // retail comparison-unit/timing verification is still outstanding.
    // The Runtime checks the target again before this completion transaction.
    bool death_charge(skills::effects::Scope scope,skills::effects::Scope target,
            const skills::effects::DeathCharge& plan,std::uint64_t now,
            std::vector<Packet>* visuals,std::function<void()>* publication,
            std::span<const navigation::SurfaceLink> closed={}){
        if(visuals)visuals->clear();if(publication)*publication={};
        if(!visuals||!publication)return false;
        std::lock_guard lock(mutex_);
        const auto step=death_charge_step_locked(scope,target,plan,now,closed);if(!step)return false;
        auto& p=players_.at(scope.actor);
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        std::set<const Outbox*> seen;
        for(auto&[id,peer]:players_)if(seen.insert(peer.outbox.get()).second)queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        for(auto&[box,queue]:queues)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        auto trail=skills::particles::death_charge_trail_2022(scope.actor,
            step->origin.point.x,step->origin.point.z,static_cast<std::uint16_t>(step->origin.surface.plane),
            step->destination.point.x,step->destination.point.z,static_cast<std::uint16_t>(step->destination.surface.plane));
        visuals->push_back(combat::detail::integer(21,scope.actor,951));
        visuals->insert(visuals->end(),trail.begin(),trail.end());visuals->push_back(step->position);
        if(step->healing_quote){
            const auto before=p.precise_health;
            health_event_locked(p,step->healing_quote,now,true);publish_health_locked(p);
            const auto gained=p.precise_health-before;
            if(gained>0)for(auto&[id,peer]:players_)peer.outbox->enqueue({
                skills::particles::health_change_2022(scope.actor,scope.actor,gained,p.resources->max_health)});
            if(p.precise_health<=0)kill_locked(p,now);
        }
        captures.clear();
        defer_or_publish_completion_locked(publication,[queues=std::move(queues)]()mutable{
            for(auto&[box,queue]:queues)box->enqueue(std::move(queue.packets));queues.clear();
        });
        return true;
    }



