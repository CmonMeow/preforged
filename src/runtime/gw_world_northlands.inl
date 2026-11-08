// Shared Instance-owned lever/door state; never a per-session portal shortcut.
private:
    std::optional<northlands::Controller> northlands_gate_;
    skills::effects::Scope northlands_door_{};
    std::unique_ptr<navigation::LayeredMesh> northlands_alternate_nav_;
    bool northlands_graph_open_=false,northlands_advancing_=false;
    struct NorthlandsPending {skills::effects::Scope lever;std::uint64_t operation,deadline;};
    std::map<skills::effects::Scope,NorthlandsPending> northlands_pending_;
    std::uint64_t northlands_operation_=0;
    std::uint64_t northlands_clock_locked()const{return northlands_gate_?northlands_gate_->snapshot()[0].at:0;}
    bool northlands_overdue_locked(std::uint64_t now)const{
        return !northlands_advancing_&&northlands_gate_&&northlands_gate_->closes_at()&&now>=*northlands_gate_->closes_at();
    }
    bool northlands_lever_actor_locked(unsigned actor)const{
        return northlands_gate_&&northlands_gate_->scene()==northlands::Scene::south146&&northlands_gate_->lever().actor==actor;
    }
    void append_northlands_snapshot_locked(std::vector<Packet>&)const{
        // Do not inject the captured nonliving lever/door records into the
        // 37130 client yet. Its join crash resolves actor 2 / model 1279 and
        // faults while indexing the model asset (missing file 0x0801f2).
        // Keep the authoritative gate controller, but withhold its actor and
        // scene-state packets until this native record is reproduced exactly.
    }
    void advance_player_positions_locked(std::uint64_t now){
        for(auto& [id,player]:players_)if(!player.steering||now>=player.steering_tick)advance_steering(player,now);
        for (auto& [id, player] : players_) if (player.moving) {
            if(now<player.started||now<player.movement_effect_clock)continue;
            std::vector<Packet> batch;
            bool segment_changed=false;
            bool plane_changed=false;
            bool direction_changed=false;
            // Consume all elapsed segments. Starting each segment at 'now'
            // added one network tick of artificial latency per nav boundary.
            while(player.moving && now >= player.ends) {
                plane_changed|=player.surface.plane!=player.target_surface.plane;
                player.x=player.tx;player.y=player.ty;player.surface=player.target_surface;
                while(!player.route.empty()&&std::hypot(player.route.front().point.x-player.x,player.route.front().point.z-player.y)<.01f){
                    plane_changed|=player.surface.plane!=player.route.front().surface.plane;
                    player.surface=player.route.front().surface;player.route.pop_front();
                }
                if(player.route.empty()){
                    player.moving=false;
                    batch.push_back(position_packet(id,player.x,player.y,static_cast<std::uint16_t>(player.surface.plane)));
                    if(player.ward_click_published_factor!=1.f){
                        player.ward_click_published_factor=1.f;
                        const auto speed=movement::ward_foes_player_speed_2022(id,
                            player.movement_effects.at_with_external(now,1.f),1.f,0);
                        batch.insert(batch.end(),speed.begin(),speed.end());
                    }
                    batch.push_back(stop_packet(id));
                }
                else {
                    const auto target=player.route.front();player.route.pop_front();
                    const auto ix=player.tx-player.sx,iy=player.ty-player.sy;
                    const auto ox=target.point.x-player.x,oy=target.point.z-player.y;
                    const double lengths=std::hypot(ix,iy)*std::hypot(ox,oy);
                    if(lengths>.0001)
                        direction_changed|=double(ix)*ox+double(iy)*oy<=0||
                            std::abs(double(ix)*oy-double(iy)*ox)>lengths*.0001;
                    player.sx=player.x;player.sy=player.y;player.tx=target.point.x;player.ty=target.point.z;player.target_surface=target.surface;
                    player.started=player.ends;player.ends=player.started+click_segment_duration_locked(player,player.started);
                    segment_changed=true;
                }
            }
            if(player.moving){
                materialize_click_position(player,now);
                const float factor=ward_foes_multiplier_locked(player.effect_scope,now).value_or(1.f);
                if(factor!=player.ward_click_published_factor){
                    player.ward_click_published_factor=factor;
                    batch.push_back(position_packet(id,player.x,player.y,
                        static_cast<std::uint16_t>(player.surface.plane)));
                    const auto speed=movement::ward_foes_player_speed_2022(id,
                        player.movement_effects.at_with_external(now,factor),1.f,0);
                    batch.insert(batch.end(),speed.begin(),speed.end());
                    batch.push_back(destination_packet(id,player.tx,player.ty,
                        static_cast<std::uint16_t>(player.target_surface.plane),
                        static_cast<std::uint16_t>(player.surface.plane)));
                }
                if(segment_changed){
                    const auto dx=player.tx-player.sx,dy=player.ty-player.sy;
                    if(std::hypot(dx,dy)>.01f){player.heading=std::atan2(dy,dx);player.turn_rate=0;player.turn_updated=now;
                    }
                    // Rebind the native actor to the reached surface before a
                    // bridge/stair continuation. A destination's plane alone
                    // can change the client's layer while its interpolation is
                    // still approaching the previous surface's boundary.
                    // WORLD41 rebases its velocity from the native actor's
                    // current interpolated point, not our reached waypoint.
                    // At a turn that point may still be behind the corner;
                    // the resulting chord can cross blocked ground. Rebind at
                    // actual direction changes as well as layer transitions.
                    if(plane_changed||direction_changed)batch.push_back(position_packet(id,player.x,player.y,static_cast<std::uint16_t>(player.surface.plane)));
                    batch.push_back(destination_packet(id,player.tx,player.ty,
                        static_cast<std::uint16_t>(player.target_surface.plane),static_cast<std::uint16_t>(player.surface.plane)));
                }
            }
            for (auto& [peer_id, peer] : players_) peer.outbox->enqueue(batch);
        }

    }
    void advance_quest_positions_locked(std::uint64_t now){
        for(auto it=quest_npc_motion_.begin();it!=quest_npc_motion_.end();){
            auto& motion=it->second;const auto npc=npcs_.find(it->first);const auto live=npc_combat_.find(it->first);
            const bool valid=valid_npc_motion_locked(it->first,motion);
            if(!valid){for(auto& [id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue({stop_packet(it->first)});
                it=quest_npc_motion_.erase(it);continue;}
            if(now<motion.updated){++it;continue;}
            if(live->second.health_clock(live->second.lifetime()).value_or(now)>now){++it;continue;}
            if(advance_npc_motion_locked(it->first,motion,npc->second,live->second,now)){it=quest_npc_motion_.erase(it);continue;}
            ++it;
        }

    }
    void settle_northlands_motion_locked(std::uint64_t at){
        // Global reactions can retire actors. Carry ownership across those
        // callbacks, never a container iterator or an actor reference.
        std::vector<skills::effects::Scope> owners,bodies;
        for(const auto&[id,p]:players_)owners.push_back(p.effect_scope);
        for(const auto&[id,live]:npc_combat_)bodies.push_back(live.hex_scope());
        for(const auto scope:owners){
            const auto p=players_.find(scope.actor);
            if(p!=players_.end()&&p->second.effect_scope==scope&&p->second.movement_effect_clock<=at)
                advance_movement_effects_locked(p->second,at);
        }
        for(const auto scope:bodies){
            const auto live=npc_combat_.find(scope.actor);
            if(live==npc_combat_.end()||live->second.hex_scope()!=scope||!npcs_.contains(scope.actor))continue;
            if(live->second.health_clock(live->second.lifetime()).value_or(at)>at)continue;
            advance_npc_conditions_locked(scope.actor,live->second,at);
        }
        std::vector<skills::effects::Scope> intents;for(const auto&[id,motion]:npc_ai_motion_)intents.push_back(motion.source);
        append_pet_layered_sources_locked(intents);
        for(const auto scope:intents){
            const auto motion=npc_ai_motion_.find(scope.actor);
            const auto live=npc_combat_.find(scope.actor);
            if(live==npc_combat_.end()||live->second.hex_scope()!=scope)continue;
            if(live->second.health_clock(live->second.lifetime()).value_or(at)>at)continue;
            if(motion!=npc_ai_motion_.end()&&(motion->second.source!=scope||motion->second.updated>at))continue;
            // Missing historical observations do not revoke a valid intent.
            // The new graph independently validates its remaining geometry.
            advance_npc_ai_motion_locked(scope,at);
        }
        advance_quest_positions_locked(at);advance_player_positions_locked(at);
    }
    bool northlands_path_legal_locked(navigation::Surface from,navigation::Point point,navigation::Surface to,navigation::Point goal)const{
        return layered_->straight_route(from,point,to,goal).has_value();
    }
    void retire_northlands_paths_locked(std::uint64_t at){
        for(auto&[id,p]:players_)if(p.moving){
            navigation::Surface from=p.surface;navigation::Point point{p.x,p.y};
            bool legal=northlands_path_legal_locked(from,point,p.target_surface,{p.tx,p.ty});
            from=p.target_surface;point={p.tx,p.ty};
            for(const auto& next:p.route){if(!legal)break;legal=northlands_path_legal_locked(from,point,next.surface,next.point);from=next.surface;point=next.point;}
            if(!legal){const auto frontier=std::max({at,p.started,p.movement_effect_clock});
                p.moving=false;p.route.clear();p.following=0;p.sx=p.tx=p.x;p.sy=p.ty=p.y;p.started=p.ends=frontier;
                cancel_npc_interaction(p);for(auto&[peer_id,peer]:players_)peer.outbox->enqueue({position_packet(id,p.x,p.y,static_cast<std::uint16_t>(p.surface.plane)),stop_packet(id)});}
        }
        for(auto it=quest_npc_motion_.begin();it!=quest_npc_motion_.end();){
            const auto body=npcs_.find(it->first);bool legal=body!=npcs_.end();
            navigation::Point point=legal?navigation::Point{body->second.x,body->second.y}:navigation::Point{};
            auto cell=legal?navigation_->locate_cell(body->second.plane,point):std::optional<std::size_t>{};legal&=cell.has_value();
            navigation::Surface from{legal?body->second.plane:0u,cell.value_or(0)};
            for(const auto& next:it->second.route){if(!legal)break;legal=northlands_path_legal_locked(from,point,next.surface,next.point);from=next.surface;point=next.point;}
            if(legal){++it;continue;}
            if(body!=npcs_.end())for(auto&[id,p]:players_)if(p.npc_publication_ready)p.outbox->enqueue({position_packet(it->first,body->second.x,body->second.y,body->second.plane),stop_packet(it->first)});
            it=quest_npc_motion_.erase(it);
        }
        for(auto it=npc_ai_motion_.begin();it!=npc_ai_motion_.end();){
            const auto body=npcs_.find(it->first);bool legal=body!=npcs_.end();
            navigation::Point point=legal?navigation::Point{body->second.x,body->second.y}:navigation::Point{};
            auto cell=legal?navigation_->locate_cell(body->second.plane,point):std::optional<std::size_t>{};legal&=cell.has_value();
            navigation::Surface from{legal?body->second.plane:0u,cell.value_or(0)};
            for(const auto& next:it->second.route){if(!legal)break;const navigation::Point goal{float(next.x),float(next.y)};
                const auto reached=layered_->trace(from,point,goal);legal=reached&&reached->plane==from.plane;if(reached)from=*reached;point=goal;}
            if(legal){++it;continue;}
            if(body!=npcs_.end())for(auto&[id,p]:players_)if(p.npc_publication_ready)p.outbox->enqueue({position_packet(it->first,body->second.x,body->second.y,body->second.plane),stop_packet(it->first)});
            it=npc_ai_motion_.erase(it);
        }
    }
    void publish_northlands_events_locked(const std::vector<northlands::Event>& events){
        for(const auto& event:events){
            if(event.changes_collision&&event.collision_open!=northlands_graph_open_){
                std::map<unsigned,std::vector<Packet>> held;
                for(const auto&[id,p]:players_)if(p.steering)held.emplace(id,steering_ahead(p,true));
                layered_.swap(northlands_alternate_nav_);northlands_graph_open_=event.collision_open;
                retire_northlands_paths_locked(event.at);
                retire_pet_layered_routes_locked(event.at);
                for(auto&[id,p]:players_)if(p.steering){
                    const auto ahead=steering_ahead(p,true);
                    if(held.at(id)!=ahead){
                        std::vector<Packet> packets{stop_packet(id),position_packet(id,p.x,p.y,static_cast<std::uint16_t>(p.surface.plane))};
                        packets.insert(packets.end(),ahead.begin(),ahead.end());
                        for(auto&[peer_id,peer]:players_)peer.outbox->enqueue(packets);
                        p.steering_clipped=p.steering_blocked=false;p.steering_server_guided=true;
                    }
                }
            }
            const auto packet=northlands::wire_2022(event);for(auto&[id,p]:players_)if(p.npc_publication_ready)p.outbox->enqueue({packet});
        }
    }
    void advance_northlands_locked(std::uint64_t now){
        if(!northlands_gate_||northlands_advancing_||now<northlands_clock_locked())return;
        northlands_advancing_=true;struct Reset {bool& value;~Reset(){value=false;}}reset{northlands_advancing_};
        while(const auto boundary=northlands_gate_->next_boundary()){
            if(*boundary>now)break;
            if(northlands_gate_->closes_at()==boundary)settle_northlands_motion_locked(*boundary);
            auto result=northlands_gate_->advance(*boundary);publish_northlands_events_locked(result.events);
        }
        auto result=northlands_gate_->advance(now);publish_northlands_events_locked(result.events);
    }
    bool northlands_ready_locked(const Player& p)const{
        if(!northlands_gate_||!p.npc_publication_ready||!layered_||
            std::hypot(double(p.x)-northlands::lever_position.x,double(p.y)-northlands::lever_position.z)>160)return false;
        const auto cell=navigation_->locate_cell(northlands::lever_plane,northlands::lever_position);if(!cell)return false;
        return layered_->reachable_within(p.surface,{p.x,p.y},{northlands::lever_plane,*cell},northlands::lever_position,160)==true;
    }
    bool execute_northlands_pull_locked(skills::effects::Scope owner,NorthlandsPending operation,std::uint64_t now){
        settle_northlands_motion_locked(now);
        const auto actor=players_.find(owner.actor);if(actor==players_.end()||actor->second.effect_scope!=owner)return false;
        auto& p=actor->second;
        northlands::Pull proof{owner,p.effect_scope,operation.lever,{p.x,p.y},operation.operation,p.npc_publication_ready,
            (!p.resources||p.resources->health>0)&&p.knocked_down_until<=now,northlands_ready_locked(p)};
        auto result=northlands_gate_->pull(proof,now);publish_northlands_events_locked(result.events);
        return result.status==northlands::Status::opened||result.status==northlands::Status::closed;
    }
    void drive_northlands_pending_locked(std::uint64_t now){
        if(northlands_pending_.empty())return;
        const auto pending=northlands_pending_;
        for(const auto&[owner,operation]:pending){
            if(!northlands_pending_.contains(owner))continue;
            const auto actor=players_.find(owner.actor);
            if(actor==players_.end()||actor->second.effect_scope!=owner||now>operation.deadline){northlands_pending_.erase(owner);continue;}
            auto& p=actor->second;if(!northlands_ready_locked(p))continue;
            northlands_pending_.erase(owner);stop_npc_approach_locked(p,now);
            execute_northlands_pull_locked(owner,operation,now);
        }
    }
public:
    // The client indexes these dynamic map actors in a compact per-instance
    // actor table. Large server-reserved IDs (62000/62001) pass the wire
    // decoder but crash the client when the portal models are registered.
    // Player identity is 1 in this host, so keep these captured portal actors
    // compact and distinct from it.
    void configure_northlands(northlands::Scene scene,unsigned lever_actor=2,unsigned door_actor=3){
        std::lock_guard lock(mutex_);
        if(northlands_gate_||!players_.empty()||!navigation_||next_actor_generation_>UINT64_MAX-3||
            lever_actor<2||door_actor<2||lever_actor>65535||door_actor>65535||lever_actor==door_actor||
            npcs_.contains(lever_actor)||npcs_.contains(door_actor)||ground_items_.used_actor(lever_actor)||ground_items_.used_actor(door_actor))
            throw std::invalid_argument("Northlands gate configuration unavailable");
        auto generation=next_actor_generation_;
        const northlands::Lease lease{instance_scope_,generation++};
        const skills::effects::Scope lever=scene==northlands::Scene::south146?skills::effects::Scope{instance_scope_,lever_actor,generation++}:skills::effects::Scope{};
        const skills::effects::Scope door{instance_scope_,door_actor,generation++};
        northlands::Controller controller(scene,lease,lever);
        auto initial=northlands::navigation_view(*navigation_,scene,controller.collision_open());
        auto alternate=scene==northlands::Scene::south146?northlands::navigation_view(*navigation_,scene,true):nullptr;
        auto reserved=retired_quest_actors_;if(lever.actor)reserved.insert(lever.actor);reserved.insert(door.actor);
        northlands_gate_.emplace(std::move(controller));northlands_door_=door;next_actor_generation_=generation;
        layered_=std::move(initial);northlands_alternate_nav_=std::move(alternate);northlands_graph_open_=northlands_gate_->collision_open();retired_quest_actors_=std::move(reserved);
    }
    void synchronize_northlands(std::uint64_t now){std::lock_guard lock(mutex_);advance_northlands_locked(now);}
    bool northlands_installed()const{std::lock_guard lock(mutex_);return northlands_gate_.has_value();}
    std::optional<skills::effects::Scope> northlands_lever_scope(unsigned actor)const{
        std::lock_guard lock(mutex_);return northlands_lever_actor_locked(actor)?std::optional{northlands_gate_->lever()}:std::nullopt;
    }
    bool northlands_open(std::uint64_t now){std::lock_guard lock(mutex_);advance_northlands_locked(now);return northlands_gate_&&northlands_gate_->collision_open();}
    bool interact_northlands(skills::effects::Scope owner,skills::effects::Scope target,std::uint64_t now,unsigned call=0){
        std::lock_guard lock(mutex_);advance_northlands_locked(now);
        const auto actor=players_.find(owner.actor);
        if(call>1||!northlands_gate_||northlands_gate_->scene()!=northlands::Scene::south146||target!=northlands_gate_->lever()||
            actor==players_.end()||actor->second.effect_scope!=owner||!actor->second.npc_publication_ready||
            (actor->second.resources&&!actor->second.resources->health)||actor->second.knocked_down_until>now||northlands_operation_==UINT64_MAX)return false;
        settle_northlands_motion_locked(now);auto& p=actor->second;cancel_npc_interaction(p);
        const NorthlandsPending operation{target,++northlands_operation_,now};
        if(northlands_ready_locked(p)){stop_npc_approach_locked(p,now);return execute_northlands_pull_locked(owner,operation,now);}
        try{move_locked(p,northlands::lever_position.x,northlands::lever_position.z,now,northlands::lever_plane);}
        catch(const std::runtime_error& e){if(std::string_view(e.what())=="movement outside navigable route")return false;throw;}
        p.following=0;northlands_pending_.insert_or_assign(owner,NorthlandsPending{target,operation.operation,movement_deadline_locked(p)});return true;
    }
