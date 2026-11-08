// Retained forward route intent. No target is consulted while integrating.
private:
    struct NpcAiMotionIntent {
        skills::effects::Scope source;
        std::vector<combat::ai::motion::Point> route;
        navigation::Point goal;float stop=0,speed=0;
        std::uint64_t committed=0,updated=0;
        unsigned plane=0;bool returning=false,layer_truncated=false,stopped=false;
        std::optional<std::uint64_t> expires; // Explicit authored idle-leg bound, not a chase timeout.
    };
    std::map<unsigned,NpcAiMotionIntent> npc_ai_motion_;
    combat::ai::motion::Outcome integrate_npc_ai_motion_locked(combat::ai::FlareController& ai,Npc& body,
            NpcAiMotionIntent& intent,std::uint64_t now,bool publish=true)const{
        using Outcome=combat::ai::motion::Outcome;using Kind=combat::ai::motion::OutcomeKind;
        using Deferral=combat::ai::motion::Deferral;
        if(intent.expires)now=std::min(now,*intent.expires);
        const auto observed=[&](Outcome result){ai.last_movement=combat::ai::FlareController::Movement{result,now};return result;};
        if(now<intent.updated)return Outcome::deferred(Deferral::observation);
        if(intent.stopped||now==intent.updated)return ai.last_movement?ai.last_movement->outcome:Outcome::deferred(Deferral::zero_budget);
        if(body.plane!=intent.plane)return observed({Kind::unsupported_layer});
        auto waypoints=intent.route;const auto goal=intent.goal;const float stop=intent.stop;
        const bool layer_truncated=intent.layer_truncated;
        if(waypoints.empty()){intent.stopped=true;return observed({layer_truncated?Kind::unsupported_layer:Kind::reached});}
        const auto& live=npc_combat_.at(body.id);
        const auto eligible=eligible_foes_wards_locked(live.hex_scope(),intent.updated,now);
        if(!eligible)return observed(Outcome::deferred(Deferral::observation));
        std::vector<skills::effects::WardFoeRouteLeg> legs;legs.reserve(waypoints.size());
        float route_x=body.x,route_y=body.y;
        for(const auto& point:waypoints){
            legs.push_back({route_x,route_y,float(point.x),float(point.y),body.plane});
            route_x=float(point.x);route_y=float(point.y);
        }
        const skills::effects::WardFoeRecipient recipient{live.hex_scope(),body.plane,body.x,body.y,true,false,true};
        const auto travel=skills::effects::ward_foes_npc_route_travel(recipient,*eligible,legs,
            live,live.lifetime(),intent.updated,now,intent.speed);
        if(!travel)return observed(Outcome::deferred(Deferral::observation));
        const double budget=travel->distance;
        const auto prior= combat::ai::motion::Point{body.x,body.y};
        const auto limit=intent.returning?std::optional(combat::ai::motion::LeashBudget{budget,false}):
            combat::ai::motion::leash_budget({body.x,body.y},waypoints,{ai.home_x,ai.home_y},ai.policy.leash,budget);
        if(!limit)return observed({Kind::invalid_request});
        // Motion integrates in double but publishes/stores float positions.
        // Stopping exactly at weapon/skill reach can round just outside it,
        // leaving every later correction smaller than one representable step.
        // Inset the movement stop by a conservative float quantization bound;
        // attack/cast admission and contact retain their strict actual reach.
        const double quantization=2.0*std::numeric_limits<float>::epsilon()*
            std::max({1.0,std::abs(double(goal.x))+stop,std::abs(double(goal.z))+stop});
        const double movement_stop=std::max(0.0,double(stop)-quantization);
        const auto moved=combat::ai::motion::advance({body.x,body.y},waypoints,{goal.x,goal.z},movement_stop,limit->budget);
        if(!moved)return observed({Kind::invalid_request});
        intent.updated=now;
        double consumed=0;auto cursor=prior;std::size_t completed=0;
        for(const auto next:waypoints){
            const double length=std::hypot(next.x-cursor.x,next.y-cursor.y);
            if(consumed+length>moved->spent+1e-8)break;
            consumed+=length;cursor=next;++completed;
        }
        intent.route.erase(intent.route.begin(),intent.route.begin()+completed);
        const bool stored_within_stop=std::hypot(double(goal.x)-float(moved->point.x),
            double(goal.z)-float(moved->point.y))<=double(stop);
        const bool pursuit_broken=limit->boundary&&!moved->reached&&!stored_within_stop&&moved->spent>=limit->budget;
        // Float storage can enter the nominal stop circle one millisecond
        // before the shared inset geometry reaches its canonical endpoint.
        // Do not retire the retained route on that rounded observation: later
        // integrations must still reach the same goal endpoint. The nominal
        // circle remains a policy guard against a false leash disengagement.
        if(moved->reached||pursuit_broken||(intent.expires&&now>=*intent.expires))intent.stopped=true;
        if(pursuit_broken){
            ai.engaged=false;ai.returning=true;
            if(ai.encounter)ai.encounter->clear();
        }
        if(moved->spent<=0){
            if(layer_truncated&&budget>0&&!pursuit_broken)return observed({Kind::unsupported_layer});
            return observed(combat::ai::motion::describe(*moved,pursuit_broken));
        }
        const bool turned=moved->facing&&std::abs(std::atan2(std::sin(*moved->facing-body.facing),
            std::cos(*moved->facing-body.facing)))>1e-5;
        body.x=float(moved->point.x);body.y=float(moved->point.y);
        if(moved->facing)body.facing=float(*moved->facing);
        // Keep all consumed route corners in wire order. The authoritative
        // endpoint and spent time/distance remain the full interval result;
        // no residual budget is dropped at a corner. A single endpoint packet
        // would instead publish a chord that can cross an unwalkable obstacle.
        std::vector<Packet> packets;packets.reserve(moved->path.size()+1);
        for(const auto& point:moved->path)
            packets.push_back(position_packet(body.id,float(point.x),float(point.y),body.plane));
        if(turned)packets.push_back(agent::facing_2022(body.id,body.facing));
        if(publish)for(const auto&[id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue(packets);
        return observed(combat::ai::motion::describe(*moved,pursuit_broken));
    }
    bool advance_npc_ai_motion_locked(skills::effects::Scope source,std::uint64_t now){
        const auto intent=npc_ai_motion_.find(source.actor);
        if(intent==npc_ai_motion_.end()||intent->second.source!=source)return advance_pet_layered_motion_locked(source,now);
        const auto body=npcs_.find(source.actor);const auto live=npc_combat_.find(source.actor);
        if(body==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=source){npc_ai_motion_.erase(intent);return true;}
        combat::ai::FlareController* motion=nullptr;
        if(const auto f=npc_flare_ai_.find(source.actor);f!=npc_flare_ai_.end()&&f->second.policy.source==source)motion=&f->second;
        if(const auto e=npc_elemental_ai_.find(source.actor);e!=npc_elemental_ai_.end()&&e->second.motion.policy.source==source)motion=&e->second.motion;
        if(!motion)motion=pet_motion_controller_locked(source);
        if(!motion){npc_ai_motion_.erase(intent);return true;}
        const auto result=integrate_npc_ai_motion_locked(*motion,body->second,intent->second,now);
        return result.kind!=combat::ai::motion::OutcomeKind::deferred||result.deferral!=combat::ai::motion::Deferral::observation;
    }
    // The current authoritative decision can replace pursuit with standing
    // combat. Prior route travel has already been materialized at now by the
    // driver frontier; erase only this exact source's non-homeward intent.
    void stop_npc_pursuit_locked(skills::effects::Scope source,std::uint64_t now){
        const auto intent=npc_ai_motion_.find(source.actor);
        const auto body=npcs_.find(source.actor);const auto live=npc_combat_.find(source.actor);
        if(intent==npc_ai_motion_.end()||intent->second.source!=source||intent->second.returning||
           intent->second.updated>now||body==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=source)return;
        npc_ai_motion_.erase(intent);
        // Match existing NPC route retirement publication. Do not assume an
        // attack/cast animation independently clears client movement state.
        for(auto&[id,peer]:players_)if(peer.npc_publication_ready)
            peer.outbox->enqueue({position_packet(source.actor,body->second.x,body->second.y,body->second.plane),
                                 stop_packet(source.actor)});
    }
    combat::ai::motion::Outcome move_npc_flare_locked(combat::ai::FlareController& ai,Npc& body,navigation::Point goal,float stop,std::uint64_t now){
        using Outcome=combat::ai::motion::Outcome;using Kind=combat::ai::motion::OutcomeKind;
        using Deferral=combat::ai::motion::Deferral;
        const auto observed=[&](Outcome outcome){ai.last_movement=combat::ai::FlareController::Movement{outcome,now};return outcome;};
        if(now<ai.updated||!advance_npc_ai_motion_locked(ai.policy.source,now))return Outcome::deferred(Deferral::observation);
        if(!std::isfinite(goal.x)||!std::isfinite(goal.z)||!std::isfinite(stop)||stop<0)return observed({Kind::invalid_request});
        if(body.plane!=ai.plane)return observed({Kind::unsupported_layer});
        const auto old=npc_ai_motion_.find(body.id);
        if(old!=npc_ai_motion_.end()&&old->second.source==ai.policy.source&&
           old->second.goal.x==goal.x&&old->second.goal.z==goal.z&&old->second.stop==stop&&
           old->second.returning==ai.returning&&!old->second.stopped)
            return ai.last_movement?ai.last_movement->outcome:Outcome::deferred(Deferral::zero_budget);
        const bool retired_route=old!=npc_ai_motion_.end()&&old->second.source==ai.policy.source;
        npc_ai_motion_.erase(body.id); // A changed/invalid route stops the old intent at now.
        const auto terminal=[&](Kind kind){
            // A replacement that cannot move must retire the client route as
            // well. Publish once for the erased intent, including home routes;
            // subsequent failed retries have no prior intent to stop.
            if(retired_route)for(auto&[id,peer]:players_)if(peer.npc_publication_ready)
                peer.outbox->enqueue({position_packet(body.id,body.x,body.y,body.plane),stop_packet(body.id)});
            return observed(Outcome{kind});
        };
        if(std::hypot(goal.x-body.x,goal.z-body.y)<=stop)return terminal(Kind::reached);
        std::vector<combat::ai::motion::Point> waypoints;
        bool layer_truncated=false;
        if(navigation_){
            const auto start=navigation_->locate_cell(body.plane,{body.x,body.y}),end=navigation_->locate_cell(body.plane,goal);
            if(!start||!end)return terminal(Kind::route_unavailable);
            const auto path=layered_->route({body.plane,*start},{body.x,body.y},{body.plane,*end},goal);
            if(!path||path->size()<2)return terminal(Kind::route_unavailable);
            // This controller owns planar movement only. Keep the existing
            // boundary at a layer transition rather than inventing a portal.
            for(std::size_t i=1;i<path->size();++i){
                if((*path)[i].surface.plane!=body.plane){layer_truncated=true;break;}
                waypoints.push_back({(*path)[i].point.x,(*path)[i].point.z});
            }
        }
        else waypoints.push_back({goal.x,goal.z});
        if(waypoints.empty())return terminal(layer_truncated?Kind::unsupported_layer:Kind::route_unavailable);
        npc_ai_motion_.insert_or_assign(body.id,NpcAiMotionIntent{ai.policy.source,std::move(waypoints),goal,stop,ai.policy.speed,
            now,now,body.plane,ai.returning,layer_truncated,false});
        // The newly observed goal owns only future travel. Never spend the
        // previous polling interval on a route that did not yet exist.
        return observed(Outcome::deferred(Deferral::zero_budget));
    }
