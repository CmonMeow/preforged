// Shared upkeep/resource chronology,normal player casting and owned cancellation.
private:
    skills::effects::MaintainedEnchantments mending_ownership_;
    std::uint64_t next_mending_effect_=1;
    std::optional<std::pair<std::uint64_t,std::uint64_t>> maintained_caster_clocks_locked(skills::effects::Scope source)const{
        if(const auto p=players_.find(source.actor);p!=players_.end())return p->second.effect_scope==source?
            std::optional(std::pair{p->second.health_updated,p->second.energy_updated}):std::nullopt;
        const auto n=npc_combat_.find(source.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=source)return {};
        const auto life=n->second.lifetime();const auto health=n->second.health_clock(life),energy=n->second.energy_clock(life);
        if(!health||!energy)return {};return std::pair{*health,*energy};
    }
    std::optional<skills::effects::MaintainedEnergy> maintained_energy_proposal_locked(skills::effects::Scope source,std::uint64_t now)const{
        if(const auto found=players_.find(source.actor);found!=players_.end()){
            const auto& p=found->second;if(p.effect_scope!=source||!p.resources||!p.overcast_curve||p.energy_updated>now||!p.resources->health)return {};
            return skills::effects::prepare_maintained_energy(mending_ownership_,*p.overcast_curve,source,*p.resources,p.energy_regeneration,p.energy_debt,now,p.energy_pips,p.energy_fractional_tail);
        }
        const auto n=npc_combat_.find(source.actor);return n!=npc_combat_.end()&&n->second.hex_scope()==source?
            n->second.maintained_energy(n->second.lifetime(),mending_ownership_,now):std::nullopt;
    }
    std::optional<skills::effects::Entry> maintained_entry_locked(skills::effects::Scope target,unsigned effect)const{
        if(const auto p=players_.find(target.actor);p!=players_.end()){
            if(p->second.effect_scope!=target)return {};
            const auto e=p->second.health_effects.find(effect);
            return e==p->second.health_effects.end()?std::nullopt:std::optional(e->second);
        }
        const auto n=npc_combat_.find(target.actor);
        return n!=npc_combat_.end()&&n->second.hex_scope()==target?n->second.hex_effect(n->second.lifetime(),effect):std::nullopt;
    }
    std::optional<std::uint64_t> maintained_recipient_clock_locked(skills::effects::Scope target)const{
        if(const auto p=players_.find(target.actor);p!=players_.end())
            return p->second.effect_scope==target?std::optional(p->second.health_updated):std::nullopt;
        const auto n=npc_combat_.find(target.actor);
        return n!=npc_combat_.end()&&n->second.hex_scope()==target?n->second.health_clock(n->second.lifetime()):std::nullopt;
    }
    std::uint64_t maintained_retirement_frontier_locked(skills::effects::Scope scope)const{
        std::uint64_t at=0;
        for(const auto& link:mending_ownership_.pending())if(link.caster==scope||link.target==scope){
            at=std::max(at,maintained_recipient_clock_locked(link.target).value_or(0));
            if(const auto p=players_.find(link.caster.actor);p!=players_.end()&&p->second.effect_scope==link.caster)
                at=std::max({at,p->second.health_updated,p->second.energy_updated});
            else if(const auto clocks=maintained_caster_clocks_locked(link.caster))at=std::max({at,clocks->first,clocks->second});
        }
        return at;
    }
    bool preflight_maintained_recipient_end_locked(skills::effects::Scope source,skills::effects::Scope target,unsigned effect,std::uint64_t now){
        if(const auto p=players_.find(target.actor);p!=players_.end()){
            if(p->second.effect_scope!=target||!p->second.effect_registry||!p->second.health_regeneration)return false;
            const auto anchor=recall_anchors_.find({source,effect});
            if(anchor!=recall_anchors_.end())return bool(skills::effects::prepare_recall_retirement(*skills::effects::recall_shadow(),anchor->second,
                source,anchor->second.target,true,true,now,*p->second.effect_registry,mending_ownership_));
            if(const auto lease=retribution_leases_.find({target,effect});lease!=retribution_leases_.end())
                return lease->second.caster==source&&bool(skills::effects::prepare_retribution_end(lease->second,now,*p->second.effect_registry,mending_ownership_));
            return bool(skills::effects::prepare_mending_end(source,target,effect,now,*p->second.effect_registry,*p->second.health_regeneration,mending_ownership_));
        }
        const auto n=npc_combat_.find(target.actor);
        if(const auto lease=retribution_leases_.find({target,effect});lease!=retribution_leases_.end()){
            if(n==npc_combat_.end()||n->second.hex_scope()!=target||lease->second.caster!=source)return false;
            const auto registry=n->second.owned_effect_snapshot(n->second.lifetime());
            if(!registry||!skills::effects::prepare_retribution_end(lease->second,now,*registry,mending_ownership_))return false;
            auto preview=n->second;return preview.remove_maintained_retribution(preview.lifetime(),effect,now);
        }
        const auto lease=succor_leases_.find({target,effect});
        if(n==npc_combat_.end()||n->second.hex_scope()!=target||lease==succor_leases_.end()||lease->second.caster!=source||
           mending_ownership_.current_maintainer(target,effect)!=std::optional(source))return false;
        auto preview=n->second;return preview.remove_reviewed_enchantment(preview.lifetime(),effect,now);
    }
    // The NPC resource primitive owns the registry/rate end. World owns the
    // matching full-generation upkeep lease and its one private/public pair.
    bool end_npc_succor_monitor_locked(skills::effects::Scope target,const skills::effects::Entry& entry,std::uint64_t now){
        const auto key=std::pair{target,entry.instance};const auto found=succor_leases_.find(key);
        if(found==succor_leases_.end())return false;
        const auto lease=found->second;
        if(entry.skill!=308||entry.cause!=lease.caster.actor||entry.applied!=lease.applied)
            throw std::logic_error("NPC Succor end generation mismatch");
        const auto caster=players_.find(lease.caster.actor);
        if(caster!=players_.end()&&caster->second.effect_scope==lease.caster&&
           !advance_energy_locked_for_succor(caster->second,now))throw std::logic_error("NPC Succor end crossed Energy");
        succor_consumers_.erase(key);succor_leases_.erase(key);
        end_mending_monitor_locked(target,entry,now);return true;
    }
    bool remove_maintained_recipient_locked(skills::effects::Scope target,unsigned effect,std::uint64_t now,bool settled=false){
        if(const auto p=players_.find(target.actor);p!=players_.end()){
            if(p->second.effect_scope!=target||!p->second.health_effects.contains(effect))return false;
            remove_health_effect_locked(p->second,effect,now,settled);return true;
        }
        const auto n=npc_combat_.find(target.actor);
        if(n!=npc_combat_.end()&&n->second.hex_scope()==target&&retribution_leases_.contains({target,effect})){
            if(!n->second.remove_maintained_retribution(n->second.lifetime(),effect,now))return false;
            publish_npc_enchantment_ends_locked(n->second);publish_npc_aura_ends_locked(n->second);return true;
        }
        return n!=npc_combat_.end()&&n->second.hex_scope()==target&&
            remove_npc_enchantment_locked(n->second,n->second.lifetime(),effect,now);
    }
    std::set<skills::effects::Scope> mending_casters_locked()const{
        std::set<skills::effects::Scope> result;for(const auto& link:mending_ownership_.pending())result.insert(link.caster);return result;
    }
    std::set<skills::effects::Scope> maintained_resource_participants_locked()const{
        std::set<skills::effects::Scope> result;
        for(const auto& link:mending_ownership_.pending()){
            result.insert(link.caster);result.insert(link.target);
        }
        return result;
    }
    std::set<skills::effects::Scope> maintained_participants_locked()const{
        auto result=maintained_resource_participants_locked();
        for(const auto& link:mending_ownership_.pending())
            if(const auto anchor=recall_anchors_.find({link.caster,link.effect});anchor!=recall_anchors_.end())
                result.insert(anchor->second.target);
        return result;
    }
    void sync_mending_curve_locked(skills::effects::Scope owner,std::uint64_t now){
        const auto caster=players_.find(owner.actor);
        if(caster==players_.end()){
            const auto npc=npc_combat_.find(owner.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=owner)return;
            const auto count=mending_ownership_.count(owner,now);
            if(!count||!npc->second.set_maintained_upkeep(npc->second.lifetime(),*count,now))throw std::logic_error("NPC maintained upkeep crossed Energy");
            return; // NPCs have no owner connection/private maintained bar.
        }
        if(caster->second.effect_scope!=owner)return;
        auto& p=caster->second;const auto count=mending_ownership_.count(owner,now);if(!count)throw std::logic_error("Mending ownership clock mismatch");
        if(!p.overcast_curve){if(*count)throw std::logic_error("Mending without Energy authority");return;}
        if(!p.overcast_curve->set_maintained_upkeep(owner,*count,now))throw std::logic_error("Mending end crossed unsettled Energy");
        p.outbox->enqueue(owner_vitals(p));
    }
    void retire_mending_actor_locked(skills::effects::Scope actor,std::uint64_t now){
        if(const auto p=players_.find(actor.actor);p!=players_.end()&&p->second.effect_scope==actor)
            recall_retiring_sources_.insert(actor);
        for(const auto& link:mending_ownership_.pending())if(link.caster==actor||link.target==actor){
            if(maintained_entry_locked(link.target,link.effect)){
                const auto clock=maintained_recipient_clock_locked(link.target);
                if(!clock||*clock>now||!remove_maintained_recipient_locked(link.target,link.effect,now,true))
                    throw std::logic_error("Maintained recipient crossed source retirement");
            }else if(const auto lease=retribution_leases_.find({link.target,link.effect});lease!=retribution_leases_.end()){
                const auto entry=lease->second.entry;
                if(end_npc_retribution_monitor_locked(link.target,entry,now))for(auto&[id,peer]:players_)peer.outbox->enqueue({health_effect_remove(link.target.actor,link.effect)});
            }else if(const auto lease=succor_leases_.find({link.target,link.effect});lease!=succor_leases_.end()){
                const skills::effects::Entry entry{link.effect,lease->second.caster.actor,308,0,skills::effects::Category::enchantment,lease->second.applied,{}};
                if(end_npc_succor_monitor_locked(link.target,entry,now))for(auto&[id,peer]:players_)peer.outbox->enqueue({health_effect_remove(link.target.actor,link.effect)});
            }else if(mending_ownership_.ended(link.target,link.effect,now))sync_mending_curve_locked(link.caster,now);
        }
    }
    struct MendingRangeEnd {skills::effects::Scope caster,target;unsigned effect;std::uint64_t at;};
    bool mending_boundary_locked(std::uint64_t& boundary,std::uint64_t horizon,std::vector<MendingRangeEnd>& ranges){
        if(!mending_ownership_.size())return true;
        const auto participants=maintained_participants_locked();
        const auto resource_participants=maintained_resource_participants_locked();
        for(const auto scope:participants){const auto p=players_.find(scope.actor);
            if(p!=players_.end()&&p->second.effect_scope==scope)boundary=std::min(boundary,next_parasitic_health_boundary_locked(p->second,boundary));
            else if(const auto n=npc_combat_.find(scope.actor);n!=npc_combat_.end()&&n->second.hex_scope()==scope){
                // A Recall tether observes the NPC's position/Health, not its
                // Energy. Unknown Energy remains a gate for actual recipients
                // and maintainers; known Energy still joins zero forecasting.
                if(resource_participants.contains(scope)||n->second.energy(n->second.lifetime())){
                    const auto forecast=n->second.forecast_energy_zero(n->second.lifetime(),boundary);if(!forecast)return false;
                    if(forecast->first_zero)boundary=std::min(boundary,*forecast->first_zero);
                }
                auto preview=n->second;const auto health=preview.advance_conditions(preview.lifetime(),boundary);if(!health)return false;
                if(health->killed)boundary=std::min(boundary,health->death_time);
            }}
        for(const auto scope:mending_casters_locked()){
            const auto clocks=maintained_caster_clocks_locked(scope);if(!clocks||clocks->second>boundary)return false;
            const auto count=mending_ownership_.count(scope,clocks->second);if(!count)return false;
            if(const auto p=players_.find(scope.actor);p!=players_.end()){
                if(!p->second.overcast_curve||p->second.overcast_curve->maintained_upkeep()!=*count)return false;
            }else{const auto n=npc_combat_.find(scope.actor);if(n==npc_combat_.end()||n->second.maintained_upkeep(n->second.lifetime())!=count)return false;}
            const auto forecast=maintained_energy_proposal_locked(scope,boundary);if(!forecast)return false;
            if(!forecast->ended.empty())boundary=std::min(boundary,forecast->ended.front().at);
        }
        for(const auto& link:mending_ownership_.pending()){
            auto range_target=link.target;
            if(link.caster==link.target){
                const auto anchor=recall_anchors_.find({link.caster,link.effect});
                if(anchor==recall_anchors_.end())continue; // Ordinary self upkeep has no range tether.
                range_target=anchor->second.target;
                const auto ally=players_.find(range_target.actor);
                // Dead/departed anchors retain upkeep,including NPC anchors.
                if(ally!=players_.end()){
                    if(ally->second.effect_scope!=range_target||ally->second.precise_health<=0)continue;
                }else{
                    const auto npc=npc_combat_.find(range_target.actor);
                    if(npc==npc_combat_.end()||npc->second.hex_scope()!=range_target||!npc->second.healable_ally(npc->second.lifetime()))continue;
                }
            }
            const auto source=players_.find(link.caster.actor),target=players_.find(range_target.actor);
            const auto source_clocks=maintained_caster_clocks_locked(link.caster);if(!source_clocks)return false;
            std::uint64_t target_clock=0;
            if(target!=players_.end()){
                if(target->second.effect_scope!=range_target)return false;target_clock=target->second.health_updated;
            }else{
                const auto npc=npc_combat_.find(range_target.actor);
                if(npc==npc_combat_.end()||npc->second.hex_scope()!=range_target)return false;
                target_clock=npc->second.health_clock(npc->second.lifetime()).value_or(0);
            }
            const auto entry=maintained_entry_locked(link.target,link.effect);if(!entry)return false;
            const auto from=std::max({entry->applied,source_clocks->first,source_clocks->second,target_clock});
            if(from>horizon)return false;
            // Always project to the outer drain's original horizon. Shorter
            // resource boundaries must not repartition midpoint turn motion.
            const auto a=maintained_range_path_locked(link.caster,from,horizon),b=maintained_range_path_locked(range_target,from,horizon);
            if(!a||!b)return false;
            const auto exit=maintained_exact_range_exit_locked(link.caster,range_target,*a,*b,from,horizon);if(!exit)return false;
            if(exit->first_outside){const auto first=*exit->first_outside;
                ranges.push_back({link.caster,link.target,link.effect,first});boundary=std::min(boundary,first);}

        }
        return true;
    }
    // The ordinary AI cadence must not defer an exact maintained-resource or
    // range deadline. Forecast only here; the normal timed drain owns commits.
    bool maintained_deadline_due_locked(std::uint64_t now){
        if(!mending_ownership_.size())return false;
        auto boundary=now;std::vector<MendingRangeEnd> ranges;
        if(!mending_boundary_locked(boundary,now,ranges))return true;
        if(boundary<now||!ranges.empty())return true;
        for(const auto scope:mending_casters_locked()){
            const auto forecast=maintained_energy_proposal_locked(scope,now);
            if(!forecast||!forecast->ended.empty())return true;
        }
        return false;
    }
    bool dispatch_mending_resources_locked(std::uint64_t boundary,std::span<const MendingRangeEnd> ranges={}){
        if(!mending_ownership_.size())return true;
        const auto participants=maintained_participants_locked();
        std::vector<std::pair<skills::effects::Scope,skills::effects::MaintainedEnergy>> proposals;
        std::vector<std::pair<skills::effects::Scope,skills::effects::MindWrackTransition>> zeros;
        for(const auto scope:mending_casters_locked()){
            auto proposal=maintained_energy_proposal_locked(scope,boundary);if(!proposal)return false;
            for(const auto& end:proposal->ended){const auto clock=maintained_recipient_clock_locked(end.release.target);
                if(end.at!=boundary||!clock||*clock>boundary||!maintained_entry_locked(end.release.target,end.release.effect)||
                   !preflight_maintained_recipient_end_locked(scope,end.release.target,end.release.effect,boundary))return false;}
            if(proposal->energy.first_zero){
                // Only the precise before/after snapshots are carried here;
                // the shared observer allocates its own real event identity.
                const auto zero=skills::effects::energy_zero_observation(proposal->energy,scope,1);
                if(!zero||zero->at!=boundary)return false;zeros.emplace_back(scope,*zero);
            }
            proposals.emplace_back(scope,std::move(*proposal));
        }
        for(const auto& end:ranges)if(end.at==boundary){
            if(!preflight_maintained_recipient_end_locked(end.caster,end.target,end.effect,boundary))return false;
        }
        // All Energy authorities reach this frontier before a removal/status
        // callback or zero observer can cause a cross-actor death.
        for(auto& [scope,proposal]:proposals){
            if(const auto p=players_.find(scope.actor);p!=players_.end())commit_overcast_energy_locked(p->second,std::move(proposal.energy));
            else{const auto n=npc_combat_.find(scope.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=scope||!n->second.commit_maintained_energy(n->second.lifetime(),proposal.energy))return false;}
        }
        for(const auto& [scope,proposal]:proposals){
            for(const auto& end:proposal.ended)if(maintained_entry_locked(end.release.target,end.release.effect)&&
                !remove_maintained_recipient_locked(end.release.target,end.release.effect,boundary,true))return false;
            if(const auto p=players_.find(scope.actor);p!=players_.end())p->second.outbox->enqueue(owner_vitals(p->second));
        }
        for(const auto& end:ranges)if(end.at==boundary){
            // The same boundary can already have removed this link for zero
            // Energy. Generic owned removal publishes exactly one63/67 pair.
            if(maintained_entry_locked(end.target,end.effect)&&!remove_maintained_recipient_locked(end.target,end.effect,boundary,true))return false;
        }
        for(const auto scope:participants){const auto p=players_.find(scope.actor);
            if(p!=players_.end()){if(p->second.effect_scope==scope&&!advance_health_locked(p->second,boundary))return false;}
            else if(const auto n=npc_combat_.find(scope.actor);n!=npc_combat_.end()&&n->second.hex_scope()==scope){
                advance_npc_conditions_locked(scope.actor,n->second,boundary);
                if(n->second.health_clock(n->second.lifetime()).value_or(boundary)!=boundary)return false;
            }}
        for(const auto& [scope,event]:zeros){const auto target=players_.find(scope.actor);if(target!=players_.end()&&target->second.effect_scope==scope&&target->second.precise_health>0)observe_mind_wrack_energy_locked(target->second,event.before,boundary,event.after);}
        return true;
    }
    bool settling_mending_policy_=false;
    bool settle_mending_policy_locked(Player& player,std::uint64_t now){
        if(!mending_ownership_.size()||!player.resources||player.precise_health<=0)return true;
        const auto count=mending_ownership_.count(player.effect_scope,now);if(!count)return false;if(!*count)return true;
        if(!player.overcast_curve||player.energy_updated!=now)return false;
        const auto raw=player.overcast_curve->uncapped_energy_pips(player.energy_pips,now);
        const auto current=(std::int64_t(player.resources->energy)-player.energy_debt)*overcast::energy_unit+std::int64_t(player.energy_regeneration.remainder())*100+player.energy_fractional_tail;
        if(raw>=-10&&(current>0||raw>=0))return true;
        if(settling_mending_policy_)return true;
        struct Guard {bool& flag;Guard(bool& value):flag(value){flag=true;}~Guard(){flag=false;}} guard(settling_mending_policy_);
        if(!timed_effect_boundary_)return settle_timed_effects_locked(now);
        return now==*timed_effect_boundary_&&dispatch_mending_resources_locked(now);
    }
    void end_mending_monitor_locked(skills::effects::Scope target,const skills::effects::Entry& entry,std::uint64_t now){
        const auto owner=mending_ownership_.current_maintainer(target,entry.instance);
        const auto wire=owner?skills::effects::mending_end_wire(*owner,target,entry):std::nullopt;
        if(!owner||!wire||!mending_ownership_.ended(target,entry.instance,now))throw std::logic_error("invalid owned Mending end");
        sync_mending_curve_locked(*owner,now);
        const auto caster=players_.find(owner->actor);
        if(caster!=players_.end()&&caster->second.effect_scope==*owner)caster->second.outbox->enqueue({wire->upkeep});
        // Generic health-effect removal publishes the recipient's opcode67.
    }
    std::optional<std::vector<skills::effects::MaintainedRangePoint>> maintained_steering_path_locked(const Player& p,std::uint64_t from,std::uint64_t through)const{
        if(from<p.steering_tick||from>through)return {};
        using Point=skills::effects::MaintainedRangePoint;
        std::vector<Point> path;
        const auto append=[&](Point point){if(point.at<from)return;if(!path.empty()&&path.back().at==point.at)path.back()=point;else path.push_back(point);};
        auto at=p.steering_tick;auto surface=p.surface;navigation::Point position{p.x,p.y};float vx=p.velocity_x,vy=p.velocity_y;
        const auto elapsed=through-at;
        const auto step=p.steering_turn_rate?std::max<std::uint64_t>(20,elapsed/64+(elapsed%64!=0)):elapsed;
        if(from==at)append({at,position.x,position.z});
        while(at<through){
            const auto duration=std::min(step,through-at),end=at+duration;
            const float half=p.steering_turn_rate*float(duration)*.0005f;rotate_velocity(vx,vy,half);
            std::vector<navigation::MotionPoint> trace;
            const auto motion=steering_motion_locked(surface,position,vx,vy,float(duration)/1000.f,&trace);
            if(!motion){
                if(path.empty())append({from,position.x,position.z});append({through,position.x,position.z});return path;
            }
            // Preserve the actual collision/contact trajectory. Include both
            // integer times surrounding each fractional contact so no turn
            // can disappear between the integer samples used by range expiry.
            std::set<std::uint64_t> samples{std::max(from,at),end};
            for(const auto& vertex:trace){if(vertex.fraction>=1){samples.insert(end);continue;}const double offset=double(vertex.fraction)*double(duration);
                samples.insert(at+std::min(duration,static_cast<std::uint64_t>(std::floor(offset))));
                samples.insert(at+std::min(duration,static_cast<std::uint64_t>(std::ceil(offset))));}
            for(const auto sample:samples){
                if(sample<from||sample>end)continue;
                const double fraction=double(sample-at)/double(duration);std::size_t index=0;
                while(index+1<trace.size()&&trace[index+1].fraction<=fraction)++index;
                auto point=trace[index].position.point;
                if(index+1<trace.size()){const auto& next=trace[index+1];const double span=next.fraction-trace[index].fraction;
                    const double t=span>0?(fraction-trace[index].fraction)/span:1.;
                    point={static_cast<float>(std::lerp(double(point.x),double(next.position.point.x),t)),static_cast<float>(std::lerp(double(point.z),double(next.position.point.z),t))};}
                append({sample,point.x,point.z});
            }
            position=motion->end.point;surface=motion->end.surface;at=end;rotate_velocity(vx,vy,half);
        }
        if(path.empty())append({from,position.x,position.z});return path;
    }
    // Piecewise nominal paths retain controller corners and speed boundaries.
    // Float storage can still put an intermediate integer observation outside
    // an exactly-edge chord, even when neither nominal endpoint exits. Refine
    // only uncertain spans, in earliest-first order, against real authority.
    std::optional<skills::effects::MaintainedRangeExit> maintained_exact_range_exit_locked(
            skills::effects::Scope caster,skills::effects::Scope target,
            const std::vector<skills::effects::MaintainedRangePoint>& a,
            const std::vector<skills::effects::MaintainedRangePoint>& b,
            std::uint64_t from,std::uint64_t through)const{
        using Point=skills::effects::MaintainedRangePoint;using Exit=skills::effects::MaintainedRangeExit;
        // Reuse the pure validator, but do not let its endpoint-only answer
        // suppress the exact oracle for nominally edge-parallel trajectories.
        if(!skills::effects::maintained_range_exit(a,b,from,through))return {};
        const auto interpolate=[](const auto& path,std::uint64_t at)->Point{
            const auto found=std::lower_bound(path.begin(),path.end(),at,[](const auto& p,auto time){return p.at<time;});
            if(found==path.end())return path.back();if(found->at==at)return *found;
            const auto& prior=*std::prev(found);const double t=double(at-prior.at)/double(found->at-prior.at);
            return {at,std::lerp(prior.x,found->x,t),std::lerp(prior.y,found->y,t)};
        };
        const auto position=[&](skills::effects::Scope scope,const auto& path,std::uint64_t at)->std::optional<Point>{
            if(const auto player=players_.find(scope.actor);player!=players_.end()){
                if(player->second.effect_scope!=scope)return {};
                // Turning steering is authored against the outer horizon's
                // midpoint subdivisions, not repartitioned for oracle queries.
                if(player->second.steering)return interpolate(path,at);
                const auto point=healing_position_locked(player->second,at);
                if(!point)return {};return Point{at,point->x,point->z};
            }
            const auto point=maintained_npc_path_locked(scope,at,at);
            if(!point||point->empty())return {};return point->front();
        };
        // Absolute error scale includes retained travel BEFORE this interval:
        // small final coordinates do not bound cancellation in a long detour's
        // accumulated budget. The shared Ward routine bounds route/ward work;
        // use a deliberately larger double-operation budget than that full run.
        const auto route_scale=[&](skills::effects::Scope scope,const auto& path){
            double scale=1.;for(const auto& point:path)scale=std::max({scale,std::abs(point.x),std::abs(point.y)});
            const auto placed=npcs_.find(scope.actor);if(placed==npcs_.end())return scale;
            double distance=0,x=placed->second.x,y=placed->second.y;
            const auto append=[&](double next_x,double next_y){distance+=std::hypot(next_x-x,next_y-y);
                scale=std::max({scale,std::abs(next_x),std::abs(next_y)});x=next_x;y=next_y;};
            if(const auto motion=npc_ai_motion_.find(scope.actor);motion!=npc_ai_motion_.end())
                for(const auto& point:motion->second.route)append(point.x,point.y);
            if(const auto motion=quest_npc_motion_.find(scope.actor);motion!=quest_npc_motion_.end())
                for(const auto& point:motion->second.route)append(point.point.x,point.point.z);
            return std::max(scale,distance);
        };
        const double operation_scale=route_scale(caster,a)+route_scale(target,b);
        constexpr double operation_count=4294967296.; // exceeds262144 bounded Ward work +4096 route legs
        constexpr double accumulated=operation_count*std::numeric_limits<double>::epsilon();
        constexpr double gamma=accumulated/(1.-accumulated);
        // Absolute double clocks are exact integer milliseconds through2^53-1.
        // Include subtraction/division error at the shared maximum route rate;
        // this also widens refinement for large epochs rather than assuming a
        // short uptime to justify a small geometric envelope.
        const double arithmetic_error=gamma*operation_scale+
            16.*std::numeric_limits<double>::epsilon()*double(through)*1000000./1000.;
        std::set<std::uint64_t> cuts{from,through};
        for(const auto& point:a)if(point.at>from&&point.at<through)cuts.insert(point.at);
        for(const auto& point:b)if(point.at>from&&point.at<through)cuts.insert(point.at);
        std::function<std::optional<Exit>(std::uint64_t,std::uint64_t)> search;
        search=[&](std::uint64_t low,std::uint64_t high)->std::optional<Exit>{
            const auto a0=interpolate(a,low),a1=interpolate(a,high),b0=interpolate(b,low),b1=interpolate(b,high);
            const double scale=std::max({1.,std::abs(a0.x),std::abs(a0.y),std::abs(a1.x),std::abs(a1.y),
                std::abs(b0.x),std::abs(b0.y),std::abs(b1.x),std::abs(b1.y)});
            // Each planar span is affine before float storage. Cover endpoint
            // interpolation, independently rounded x/y for BOTH participants,
            // and shared double-integrator arithmetic with a deliberately loose
            // float-ULP envelope. Near-edge cases get exact observations instead
            // of relying on tolerance to classify a strict >5000 predicate.
            const double error=16.*std::numeric_limits<float>::epsilon()*scale+arithmetic_error;
            const double maximum=std::max(std::hypot(a0.x-b0.x,a0.y-b0.y),std::hypot(a1.x-b1.x,a1.y-b1.y));
            // Convexity proves the whole affine relative chord lies in this
            // disk. Subtract the error envelope before declaring it safe.
            if(maximum+error<=5000.)return Exit{};
#ifdef FW_GW_MAINTAINED_RANGE_ORACLE_OBSERVER
            FW_GW_MAINTAINED_RANGE_ORACLE_OBSERVER(low);
#endif
            const auto exact_a=position(caster,a,low),exact_b=position(target,b,low);
            if(!exact_a||!exact_b)return {};
            if(std::hypot(exact_a->x-exact_b->x,exact_a->y-exact_b->y)>5000.)return Exit{low};
            if(low==high)return Exit{};
            const auto middle=low+(high-low)/2;
            const auto left=search(low,middle);if(!left||left->first_outside)return left;
            return search(middle+1,high);
        };
        auto previous=from;
        for(const auto end:cuts){
            if(end<previous)continue;
            const auto result=search(previous,end);if(!result||result->first_outside)return result;
            if(end==through)break;previous=end+1;
        }
        return Exit{};
    }
    // Forecast only already-authored forward motion. Every observation starts
    // from the same baseline: advancing one cloned object incrementally would
    // introduce extra float roundings absent from authoritative materialization.
    std::optional<std::vector<skills::effects::MaintainedRangePoint>> maintained_controller_path_locked(
            skills::effects::Scope scope,std::uint64_t from,std::uint64_t through)const{
        using Point=skills::effects::MaintainedRangePoint;
        const auto placed=npcs_.find(scope.actor);const auto live=npc_combat_.find(scope.actor);
        if(from>through||placed==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=scope||
           live->second.health_clock(live->second.lifetime()).value_or(from)>from)return {};
        const combat::ai::FlareController* controller=nullptr;
        if(const auto f=npc_flare_ai_.find(scope.actor);f!=npc_flare_ai_.end()){
            if(f->second.policy.source!=scope||f->second.updated>from)return {};
            controller=&f->second;
        }
        if(const auto e=npc_elemental_ai_.find(scope.actor);e!=npc_elemental_ai_.end()){
            if(controller||e->second.motion.policy.source!=scope||e->second.motion.updated>from)return {};
            controller=&e->second.motion;
        }
        if(const auto ordinary=npc_ordinary_ai_.find(scope.actor);ordinary!=npc_ordinary_ai_.end()&&ordinary->second.source!=scope)return {};
        if(const auto wander=npc_wander_.find(scope.actor);wander!=npc_wander_.end()&&wander->second.source!=scope)return {};
        const auto intent=npc_ai_motion_.find(scope.actor);
        if(intent==npc_ai_motion_.end()){
            // Driver decisions author future intent at their actual frontier;
            // they cannot retroactively move an actor without retained intent.
            std::vector<Point> result{{from,placed->second.x,placed->second.y}};
            if(through>from)result.push_back({through,placed->second.x,placed->second.y});return result;
        }
        if(!controller||intent->second.source!=scope||intent->second.updated>from||
           intent->second.committed>intent->second.updated||placed->second.plane!=intent->second.plane||
           !std::isfinite(intent->second.speed)||intent->second.speed<=0)return {};
        if(intent->second.stopped||intent->second.route.empty()){
            std::vector<Point> result{{from,placed->second.x,placed->second.y}};
            if(through>from)result.push_back({through,placed->second.x,placed->second.y});return result;
        }
        struct Sample {Point point;double travelled=0;bool stopped=false;};
        std::map<std::uint64_t,Sample> observations;
        const auto sample=[&](std::uint64_t at)->std::optional<Sample>{
            if(const auto saved=observations.find(at);saved!=observations.end())return saved->second;
            auto forecast_controller=*controller;auto forecast_body=placed->second;auto forecast_intent=intent->second;
            const auto outcome=integrate_npc_ai_motion_locked(forecast_controller,forecast_body,forecast_intent,at,false);
            using Kind=combat::ai::motion::OutcomeKind;using Deferral=combat::ai::motion::Deferral;
            if(outcome.kind==Kind::invalid_request||outcome.kind==Kind::route_unavailable||
               (outcome.kind==Kind::deferred&&outcome.deferral==Deferral::observation)||
               !std::isfinite(forecast_body.x)||!std::isfinite(forecast_body.y)||!std::isfinite(outcome.travelled))return {};
            // At the retained baseline last_movement may describe a PRIOR
            // interval. Its distance must not masquerade as new route travel.
            const Sample value{{at,forecast_body.x,forecast_body.y},at==intent->second.updated?0:outcome.travelled,forecast_intent.stopped};
            observations.emplace(at,value);return value;
        };
        const auto first=sample(from),last=sample(through);if(!first||!last)return {};
        const auto& route=intent->second;
        const auto eligible=eligible_foes_wards_locked(scope,route.updated,through);
        const auto history=live->second.historical_movement_boundaries(live->second.lifetime(),route.updated,through);
        if(!eligible||!history)return {};
        std::set<std::uint64_t> times{from,through};
        const auto add=[&](std::uint64_t at){if(at>=from&&at<=through)times.insert(at);};
        for(const auto at:*history)add(at);
        for(const auto& ward:*eligible){add(ward.created);add(ward.expires);}
        if(route.expires)add(*route.expires);
        // Route geometry only supplies cuts. Travel, wards, leash clipping,
        // knockdown, stop inset and float storage all remain in the integrator.
        std::vector<double> cuts;double distance=0;float x=placed->second.x,y=placed->second.y;
        for(const auto& endpoint:route.route){
            const auto length=std::hypot(endpoint.x-x,endpoint.y-y);
            if(!std::isfinite(length))return {};
            const skills::effects::WardFoeRecipient recipient{scope,placed->second.plane,x,y,true,false,true};
            for(const auto& ward:*eligible){
                const auto interval=skills::effects::ward_foes_segment_interval(ward,recipient,float(endpoint.x),float(endpoint.y),ward.created);
                if(interval){cuts.push_back(distance+length*interval->enter);cuts.push_back(distance+length*interval->leave);}
            }
            distance+=length;cuts.push_back(distance);x=float(endpoint.x);y=float(endpoint.y);
        }
        for(const auto cut:cuts){
            if(cut<=0||cut>last->travelled)continue;
            auto low=route.updated,high=through;
            while(high-low>1){const auto middle=low+(high-low)/2;const auto observed=sample(middle);if(!observed)return {};
                if(observed->travelled>=cut)high=middle;else low=middle;}
            add(low);add(high);
        }
        if(last->stopped&&!route.stopped){
            auto low=route.updated,high=through;
            while(high-low>1){const auto middle=low+(high-low)/2;const auto observed=sample(middle);if(!observed)return {};
                if(observed->stopped)high=middle;else low=middle;}
            add(low);add(high);
        }
        std::vector<Point> result;result.reserve(times.size());
        for(const auto at:times){const auto observed=sample(at);if(!observed)return {};result.push_back(observed->point);}
        // The adaptive range consumer uses single-time samples from this
        // adapter as its exact integer oracle, including float-only exits.
        return result;
    }
    std::optional<std::vector<skills::effects::MaintainedRangePoint>> maintained_npc_path_locked(
            skills::effects::Scope scope,std::uint64_t from,std::uint64_t through)const{
        const auto placed=npcs_.find(scope.actor);const auto found=npc_combat_.find(scope.actor);
        if(from>through||placed==npcs_.end()||found==npc_combat_.end()||found->second.hex_scope()!=scope)return {};
        const auto& live=found->second;const auto life=live.lifetime();
        if(live.health_clock(life).value_or(from)>from)return {};
        // Layered pet motion has a separate producer, not a stationary NPC
        // placement and not this planar controller's retained route.
        if(const auto pet=pet_ai_.find(scope.actor);pet!=pet_ai_.end()&&pet->second.layered_motion&&
           !pet->second.layered_motion->route.empty())return {};
        using Point=skills::effects::MaintainedRangePoint;
        const auto motion=quest_npc_motion_.find(scope.actor);
        if(motion==quest_npc_motion_.end()){
            if(npc_flare_ai_.contains(scope.actor)||npc_elemental_ai_.contains(scope.actor)||npc_ordinary_ai_.contains(scope.actor)||
               npc_ai_motion_.contains(scope.actor)||npc_wander_.contains(scope.actor))
                return maintained_controller_path_locked(scope,from,through);
            std::vector<Point> path{{from,placed->second.x,placed->second.y}};
            if(through>from)path.push_back({through,placed->second.x,placed->second.y});return path;
        }
        const auto& route=motion->second;
        if(route.scope!=scope||route.updated>from||!valid_npc_motion_locked(scope.actor,route)||
           !std::isfinite(route.speed)||route.speed<=0)return {};
        const auto eligible=eligible_foes_wards_locked(scope,route.updated,through);
        const auto boundaries=live.historical_movement_boundaries(life,route.updated,through);
        if(!eligible||!boundaries)return {};
        std::vector<skills::effects::WardFoeRouteLeg> legs;legs.reserve(route.route.size());
        float route_x=placed->second.x,route_y=placed->second.y;unsigned plane=placed->second.plane;
        for(const auto& goal:route.route){
            legs.push_back({route_x,route_y,goal.point.x,goal.point.z,plane});
            route_x=goal.point.x;route_y=goal.point.z;plane=static_cast<unsigned>(goal.surface.plane);
        }
        const skills::effects::WardFoeRecipient recipient{scope,placed->second.plane,
            placed->second.x,placed->second.y,true,false,true};
        const auto budget=[&](std::uint64_t at)->std::optional<double>{
            const auto travel=skills::effects::ward_foes_npc_route_travel(recipient,*eligible,legs,
                live,life,route.updated,at,route.speed);
            return travel?std::optional(travel->distance):std::nullopt;
        };
        const auto position=[&](std::uint64_t at)->std::optional<Point>{
            const auto travelled=budget(at);if(!travelled)return {};
            double remaining=*travelled;navigation::Point point{placed->second.x,placed->second.y};
            for(const auto& goal:route.route){
                const double dx=double(goal.point.x)-point.x,dy=double(goal.point.z)-point.z,distance=std::hypot(dx,dy);
                if(distance<=remaining){point=goal.point;remaining-=distance;}
                else {if(distance>0){point.x+=float(dx*remaining/distance);point.z+=float(dy*remaining/distance);}break;}
            }
            return Point{at,point.x,point.z};
        };
        std::set<std::uint64_t> samples{from,through};
        for(const auto at:*boundaries)if(at>=from)samples.insert(at);
        for(const auto& ward:*eligible)for(const auto at:{ward.created,ward.expires})
            if(at>=from&&at<=through)samples.insert(at);
        // Include both integer samples bracketing each route corner. Speed and
        // knockdown boundaries alone would join distant corners with a chord.
        double distance=0;std::vector<double> cuts;
        const auto total=budget(through);if(!total)return {};
        for(const auto& leg:legs){
            const double length=std::hypot(double(leg.end_x)-leg.x,double(leg.end_y)-leg.y);
            auto sample=recipient;sample.x=leg.x;sample.y=leg.y;sample.plane=leg.plane;
            for(const auto& ward:*eligible){
                const auto interval=skills::effects::ward_foes_segment_interval(ward,sample,leg.end_x,leg.end_y,ward.created);
                if(interval){cuts.push_back(distance+length*interval->enter);cuts.push_back(distance+length*interval->leave);}
            }
            distance+=length;cuts.push_back(distance);
        }
        // Field edges are speed boundaries too. Bracket their arrival times
        // just like navigation corners, retaining the maintained-range adapter's
        // integer-millisecond trajectory contract.
        for(const auto cut:cuts){
            if(cut>*total)continue;
            auto low=route.updated,high=through;
            while(high-low>1){const auto middle=low+(high-low)/2;const auto spent=budget(middle);
                if(!spent)return {};if(*spent>=cut)high=middle;else low=middle;}
            if(low>=from)samples.insert(low);if(high>=from)samples.insert(high);
        }
        std::vector<Point> result;result.reserve(samples.size());for(const auto at:samples){
            const auto point=position(at);if(!point)return {};result.push_back(*point);
        }return result;
    }
    // Read-only staged trajectory adapter. Click paths were validated by the
    // navigation authority when accepted. Keyboard paths use the same midpoint
    // integration and collision-displacement calculation as actual movement.
    std::optional<std::vector<skills::effects::MaintainedRangePoint>> maintained_range_path_locked(
            skills::effects::Scope source,std::uint64_t from,std::uint64_t through)const{
        const auto found=players_.find(source.actor);
        if(found==players_.end())return maintained_npc_path_locked(source,from,through);
        if(found->second.effect_scope!=source||from>through)return {};
        const auto& p=found->second;
        if(from<p.last_move||(p.moving&&from<p.started))return {};
        if(p.steering)return maintained_steering_path_locked(p,from,through);
        const auto first=healing_position_locked(p,from),last=healing_position_locked(p,through);if(!first||!last)return {};
        using Point=skills::effects::MaintainedRangePoint;
        std::vector<Point> path{{from,first->x,first->z}};
        if(from==through)return path;
        if(p.moving){
            if(p.ends<=p.started)return {};
            // The first click leg moves at the authoritative run speed until
            // its final millisecond. Its rounded-up end clock includes a
            // short terminal remainder; a single endpoint interpolation
            // would make range expiry one millisecond late.
            if(p.ends>from+1&&p.ends<through){
                const auto before_end=healing_position_locked(p,p.ends-1);if(!before_end)return {};
                path.push_back({p.ends-1,before_end->x,before_end->z});
            }
            auto at=p.ends;navigation::Point point{p.tx,p.ty};
            if(at>from&&at<through)path.push_back({at,point.x,point.z});
            for(const auto& segment:p.route){
                if(at>=through)break;
                const auto distance=std::hypot(segment.point.x-point.x,segment.point.z-point.z);
                if(distance<.01f){point=segment.point;continue;}
                const auto duration=std::max<std::uint64_t>(1,static_cast<std::uint64_t>(std::ceil(distance/(movement::player_run_speed*p.movement_multiplier)*1000)));
                if(duration>UINT64_MAX-at)return {};
                at+=duration;point=segment.point;
                if(at>from&&at<through)path.push_back({at,point.x,point.z});
            }
        }
        path.push_back({through,last->x,last->z});return path;
    }
public:
    std::optional<std::vector<skills::effects::MaintainedRangePoint>> maintained_range_path(skills::effects::Scope source,std::uint64_t from,std::uint64_t through)const{
        std::lock_guard lock(mutex_);return maintained_range_path_locked(source,from,through);
    }
    std::optional<unsigned> maintained_enchantment_count(skills::effects::Scope caster,std::uint64_t now)const{
        std::lock_guard lock(mutex_);const auto found=players_.find(caster.actor);
        if(found!=players_.end())return found->second.effect_scope==caster?mending_ownership_.count(caster,now):std::nullopt;
        const auto npc=npc_combat_.find(caster.actor);
        return npc!=npc_combat_.end()&&npc->second.hex_scope()==caster?mending_ownership_.count(caster,now):std::nullopt;
    }
    bool mending_target_valid(skills::effects::Scope source,skills::effects::Scope target,unsigned divine,std::uint64_t now){
        std::lock_guard lock(mutex_);return players_.contains(target.actor)&&valid_ally_enchantment_target_locked(source,target,divine,now);
    }
    bool complete_mending(skills::effects::Scope source,skills::effects::Scope target,const skills::effects::MendingPlan& plan,unsigned divine,std::uint64_t now){
        return stage_mending(source,target,plan,now,divine,true);
    }
    bool stage_mending(skills::effects::Scope source,skills::effects::Scope target,const skills::effects::MendingPlan& plan,std::uint64_t now,unsigned divine=0,bool completion=false){
        const auto canonical=skills::effects::mending(plan.rank);if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);
        const auto selected=players_.find(target.actor);if(selected==players_.end()||!valid_ally_enchantment_target_locked(source,target,divine,now))return false;
        const auto visual=target==source?skills::particles::target_body_2022(target.actor,290):skills::particles::targeted_body_2022(target.actor,source.actor,290);if(completion&&!visual)return false;
        auto& caster=players_.at(source.actor);
        if(!advance_resource_event_locked(caster,now))return false;
        auto& recipient=selected->second;
        if(!recipient.effect_registry||!recipient.health_regeneration)return false;
        const auto allocated=std::max(next_mending_effect_,recipient.next_health_effect);if(allocated>UINT32_MAX)return false;
        const auto effect=static_cast<unsigned>(allocated);
        auto proposed=skills::effects::prepare_mending(plan,source,target,effect,now,*recipient.effect_registry,*recipient.health_regeneration,mending_ownership_);if(!proposed)return false;
        const auto wire=skills::effects::mending_wire(source,target,proposed->entry);if(!wire)return false;
        std::vector<std::pair<skills::effects::Scope,skills::effects::MendingWire>> old;
        for(const auto& entry:proposed->removed){const auto owner=mending_ownership_.current_maintainer(target,entry.instance);const auto ended=owner?skills::effects::mending_end_wire(*owner,target,entry):std::nullopt;if(!owner||!ended)return false;old.emplace_back(*owner,*ended);}
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;for(auto& [box,queue]:recipients)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        if(!caster.overcast_curve)caster.overcast_curve.emplace(source,now,33);
        *recipient.effect_registry=std::move(proposed->registry);*recipient.health_regeneration=std::move(proposed->regeneration);mending_ownership_=std::move(proposed->ownership);
        sync_mending_curve_locked(source,now);for(const auto& [owner,unused]:old)if(owner!=source)sync_mending_curve_locked(owner,now);
        for(const auto& entry:proposed->removed)recipient.health_effects.erase(entry.instance);
        recipient.health_effects.emplace(effect,proposed->entry);recipient.next_health_effect=next_mending_effect_=allocated+1;
        for(const auto& [owner,ended]:old){const auto caster=players_.find(owner.actor);if(caster!=players_.end()&&caster->second.effect_scope==owner)caster->second.outbox->enqueue({ended.upkeep});for(auto& [id,peer]:players_)peer.outbox->enqueue({ended.effect});}
        players_.at(source.actor).outbox->enqueue({wire->upkeep});for(auto& [id,peer]:players_)peer.outbox->enqueue({wire->effect});
        commit_applications_locked(target,now);
        if(divine&&recipient.precise_health>0){const auto before=recipient.precise_health;health_event_locked(recipient,static_cast<unsigned>(std::lround(3.2*divine)),now,true);const auto gained=recipient.precise_health-before;
            if(gained>0)for(auto& [id,peer]:players_)peer.outbox->enqueue({skills::particles::health_change_2022(target.actor,source.actor,gained,recipient.resources->max_health)});publish_health_locked(recipient);}
        publish_status_locked(recipient,now);
        if(!settle_timed_effects_locked(now))throw std::logic_error("Mending application crossed resource boundary");
        captures.clear();for(auto& [box,queue]:recipients){std::vector<Packet> batch;if(completion)batch={combat::detail::integer(58,source.actor,0),*visual};batch.insert(batch.end(),std::make_move_iterator(queue.packets.begin()),std::make_move_iterator(queue.packets.end()));box->enqueue(std::move(batch));}return true;
    }
    bool cancel_maintained_enchantment(skills::effects::Scope source,unsigned effect,std::uint64_t now){
        std::lock_guard lock(mutex_);const auto caster=players_.find(source.actor);
        if(caster!=players_.end()){if(caster->second.effect_scope!=source)return false;}
        else{const auto n=npc_combat_.find(source.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=source)return false;}
        if(!settle_timed_effects_locked(now))return false;
        const auto target=mending_ownership_.maintained_target(source,effect,now);if(!target)return false;
        if(!players_.contains(target->actor))return preflight_maintained_recipient_end_locked(source,*target,effect,now)&&
            remove_maintained_recipient_locked(*target,effect,now);
        const auto found=players_.find(target->actor);if(found==players_.end()||found->second.effect_scope!=*target||!found->second.effect_registry||!found->second.health_regeneration||!advance_health_locked(found->second,now))return false;
        auto& recipient=found->second;
        if(const auto entry=recipient.effect_registry->lookup(*target,effect);entry&&entry->skill==248){
            if(!preflight_maintained_recipient_end_locked(source,*target,effect,now))return false;
            remove_health_effect_locked(recipient,effect,now);return true;
        }
        if(const auto entry=recipient.effect_registry->lookup(*target,effect);entry&&entry->skill==925){
            if(*target!=source||!recall_anchors_.contains({source,effect}))return false;
            remove_health_effect_locked(recipient,effect,now);return true;
        }
        const auto proposed=skills::effects::prepare_mending_end(source,*target,effect,now,*recipient.effect_registry,*recipient.health_regeneration,mending_ownership_);if(!proposed)return false;
        // Generic removal owns monitor retirement and recipient/status packets.
        remove_health_effect_locked(recipient,effect,now);return true;
    }
    bool blessed_signet_caster_ready(skills::effects::Scope source,std::uint64_t now)const{
        std::lock_guard lock(mutex_);const auto p=players_.find(source.actor);
        return p!=players_.end()&&p->second.effect_scope==source&&p->second.resources&&p->second.resources->max_energy&&
            p->second.precise_health>0&&p->second.knocked_down_until<=now&&now>=p->second.health_updated&&now>=p->second.energy_updated;
    }
    // Runtime owns admission,interruption,once-per-cast completion and recharge.
    bool complete_blessed_signet(skills::effects::Scope source,const skills::effects::BlessedSignetPlan& plan,std::uint64_t now){
        const auto canonical=skills::effects::blessed_signet(plan.rank);if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);const auto found=players_.find(source.actor);
        if(found==players_.end()||found->second.effect_scope!=source)return false;
        auto& caster=found->second;
        if(!caster.resources||!caster.resources->max_energy||now<caster.health_updated||now<caster.energy_updated||
           !advance_resource_event_locked(caster,now)||caster.precise_health<=0||caster.knocked_down_until>now)return false;
        const auto amount=skills::effects::blessed_signet_gain(plan,source,mending_ownership_,now);if(!amount)return false;
        // All linked expiry/death/upkeep events have settled before counting.
        // The shared Energy authority handles debt and reduced ceilings.
        const auto gain=gain_energy_locked(caster,*amount,now);
        const auto wire=skills::effects::blessed_signet_wire(source.actor,gain.precise_units,caster.resources->max_energy,overcast::energy_unit);
        if(gain.changed()&&!wire)throw std::logic_error("Blessed Signet gain could not serialize");
        for(auto& [id,peer]:players_){
            std::vector<Packet> packets{combat::detail::integer(58,source.actor,0),combat::detail::integer(21,source.actor,523)};
            // Energy is owner-private here;the frozen self-cast window does
            // not independently establish retail observer routing.
            if(peer.effect_scope==source){
                if(wire){packets.push_back(wire->delta);if(wire->popup)packets.push_back(*wire->popup);}
                auto vitals=owner_vitals(caster);packets.insert(packets.end(),std::make_move_iterator(vitals.begin()),std::make_move_iterator(vitals.end()));
            }
            peer.outbox->enqueue(std::move(packets));
        }
        return true;
    }



