// Included inside Instance. No decoded packet can call this detached effect.
public:
    std::optional<std::vector<skills::effects::Scope>> stage_healing_ribbon(
            skills::effects::Scope source,skills::effects::Scope primary,
            const skills::effects::HealingRibbonPlan& plan,unsigned divine_rank,
            std::uint64_t now,std::function<void()>* publication=nullptr,
            std::optional<std::uint64_t> completion_event={}){
        if(publication)*publication={};
        if(skills::effects::healing_ribbon(plan.rank)!=plan||divine_rank>20||
           source==primary)return {};
        std::lock_guard lock(mutex_);
        const auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.resources||caster->second.precise_health<=0||
           caster->second.knocked_down_until>now||
           (divine_rank&&!monk_primary(caster->second))||
           !settle_timed_effects_locked(now))return {};
        const auto origin=healing_position_locked(caster->second,now);
        if(!origin)return {};
        const auto primary_player=players_.find(primary.actor);
        const auto primary_npc=npc_combat_.find(primary.actor);
        navigation::Point center{};
        std::uint16_t plane=0;
        if(primary_player!=players_.end()){
            if(primary_player->second.effect_scope!=primary||
               !live_heal_target_locked(caster->second,primary_player->second,
                   std::numeric_limits<float>::max(),false))return {};
            const auto position=healing_position_locked(primary_player->second,now);
            if(!position)return {};
            center=*position;plane=primary_player->second.surface.plane;
        }else{
            if(primary_npc==npc_combat_.end()||primary_npc->second.hex_scope()!=primary||
               !primary_npc->second.healable_ally(primary_npc->second.lifetime())||
               primary_npc->second.healing_allowed(primary_npc->second.lifetime())!=std::optional(true)||
               now<primary_npc->second.health_clock(primary_npc->second.lifetime()).value_or(now))return {};
            refresh_npc_movement_locked(primary.actor,now);
            const auto position=npcs_.find(primary.actor);
            if(position==npcs_.end())return {};
            center={position->second.x,position->second.y};plane=position->second.plane;
        }
        if(plane!=caster->second.surface.plane||
           std::hypot(origin->x-center.x,origin->z-center.z)>plan.range)return {};
        if(navigation_){const auto line=layered_->trace(caster->second.surface,*origin,center);
            if(!line||line->plane!=plane)return {};}
        if(!advance_resource_event_locked(caster->second,now)||
           caster->second.precise_health<=0)return {};
        // Timed conditions must be settled before a recipient's health is
        // quoted; never infer a dead/hostile NPC from its name or model.
        if(primary_npc!=npc_combat_.end()){
            advance_npc_conditions_locked(primary.actor,primary_npc->second,now);
            const auto live=npc_combat_.find(primary.actor);
            if(live==npc_combat_.end()||live->second.hex_scope()!=primary||
               !live->second.healable_ally(live->second.lifetime()))return {};
        }
        std::vector<skills::effects::HealingRibbonCandidate> candidates;
        for(auto& [id,player]:players_){
            if(!live_heal_target_locked(caster->second,player,
                   std::numeric_limits<float>::max(),true))continue;
            const auto position=healing_position_locked(player,now);
            if(!position||player.surface.plane!=plane)continue;
            const auto distance=std::hypot(position->x-center.x,position->z-center.z);
            if(distance>plan.near&&player.effect_scope!=primary)continue;
            if(!advance_health_locked(player,now)||player.precise_health<=0)continue;
            candidates.push_back({player.effect_scope,distance,true,true});
        }
        for(auto& [id,live]:npc_combat_){
            const auto scope=live.hex_scope();const auto life=live.lifetime();
            if(!live.healable_ally(life)||
               live.healing_allowed(life)!=std::optional(true)||
               now<live.health_clock(life).value_or(now))continue;
            refresh_npc_movement_locked(id,now);
            const auto position=npcs_.find(id);
            if(position==npcs_.end()||position->second.plane!=plane)continue;
            const auto distance=std::hypot(position->second.x-center.x,
                                           position->second.y-center.z);
            if(distance>plan.near&&scope!=primary)continue;
            advance_npc_conditions_locked(id,live,now);
            const auto current=npc_combat_.find(id);
            if(current==npc_combat_.end()||current->second.hex_scope()!=scope||
               !current->second.healable_ally(current->second.lifetime())||
               current->second.healing_allowed(current->second.lifetime())!=std::optional(true))continue;
            candidates.push_back({scope,distance,true,true});
        }
        const auto recipients=skills::effects::healing_ribbon_recipients(
            plan,source,primary,std::move(candidates));
        if(!recipients)return {};
        struct NpcProposal {std::uint32_t id;combat::npcs::LiveActor live;double gained;unsigned maximum;};
        std::vector<NpcProposal> npc_proposals;
        for(const auto& recipient:*recipients){
            if(players_.contains(recipient.scope.actor))continue;
            const auto found=npc_combat_.find(recipient.scope.actor);
            if(found==npc_combat_.end()||found->second.hex_scope()!=recipient.scope)return {};
            auto proposed=found->second;const auto life=proposed.lifetime();
            const auto* before=proposed.actor(life);
            if(!before||!before->health||before->health->current<=0)return {};
            const auto old=before->health->current;
            const auto amount=recipient.base_heal+
                (recipient.divine_favor?static_cast<unsigned>(std::lround(3.2*divine_rank)):0u);
            if(!proposed.heal_ally(life,amount,now))return {};
            const auto* after=proposed.actor(life);
            if(!after||!after->health)return {};
            const auto gained=after->health->current-old;
            const auto maximum=after->health->maximum;
            npc_proposals.push_back({recipient.scope.actor,std::move(proposed),
                                     gained,maximum});
        }
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        if(completion_event&&!complete_ether_renewal_locked(source,2062,*completion_event,now,true))return {};
        for(const auto& recipient:*recipients){
            if(auto player=players_.find(recipient.scope.actor);player!=players_.end()){
                auto& ally=player->second;const auto before=ally.precise_health;
                const auto amount=recipient.base_heal+
                    (recipient.divine_favor?static_cast<unsigned>(std::lround(3.2*divine_rank)):0u);
                health_event_locked(ally,amount,now,true);
                publish_health_locked(ally);
                const auto gained=ally.precise_health-before;
                if(gained>0)for(auto& [id,peer]:players_)peer.outbox->enqueue({
                    skills::particles::health_change_2022(recipient.scope.actor,source.actor,
                        gained,ally.resources->max_health)});
                if(ally.precise_health<=0)kill_locked(ally,now);
            }else{
                const auto item=std::find_if(npc_proposals.begin(),npc_proposals.end(),
                    [&](const auto& n){return n.id==recipient.scope.actor;});
                if(item==npc_proposals.end())return {};
                npc_combat_.at(item->id)=std::move(item->live);
                const auto& health=*npc_combat_.at(item->id).actor(
                    npc_combat_.at(item->id).lifetime())->health;
                for(auto& [id,peer]:players_){
                    std::vector<Packet> packets{combat::detail::fraction(item->id,health)};
                    if(item->gained>0)packets.push_back(skills::particles::health_change_2022(
                        item->id,source.actor,item->gained,item->maximum));
                    peer.outbox->enqueue(packets);
                }
            }
        }
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));
        std::vector<skills::effects::Scope> scopes;
        for(const auto& recipient:*recipients)scopes.push_back(recipient.scope);
        return scopes;
    }
