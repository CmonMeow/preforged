// Included inside Instance. Group membership/route ownership is independent of
// the four actual kill targets and reuses normal NPC motion/navigation.
private:
    void retire_npc_opposed_domain_locked(skills::effects::Scope owner,std::optional<unsigned> quest={},std::optional<unsigned> cycle={}){
        const auto matches=[&](const combat::ai::opposition::QuestDomain& domain){return domain.owner==owner&&(!quest||domain.quest==*quest)&&(!cycle||domain.cycle==*cycle);};
        std::erase_if(npc_opposed_allies_,[&](const auto& item){return matches(item.second);});
        for(auto it=quest_npc_motion_.begin();it!=quest_npc_motion_.end();){
            if(!it->second.group_attempt||!matches(*it->second.group_attempt)){++it;continue;}
            const auto live=npc_combat_.find(it->first);
            if(live!=npc_combat_.end()&&live->second.hex_scope()==it->second.scope)for(auto&[id,p]:players_)p.outbox->enqueue({stop_packet(it->first)});
            it=quest_npc_motion_.erase(it);
        }
        std::vector<skills::effects::Scope> sources;
        for(const auto&[id,binding]:npc_friendly_ordinary_)if(binding.quest&&matches({binding.quest->owner,binding.quest->quest,binding.quest->cycle}))sources.push_back(binding.source);
        for(const auto source:sources){const auto ai=npc_ordinary_ai_.find(source.actor);const auto live=npc_combat_.find(source.actor);
            if(ai!=npc_ordinary_ai_.end()){auto at=ai->second.ledger.clock();
                if(live!=npc_combat_.end()&&live->second.hex_scope()==source){const auto life=live->second.lifetime();
                    at=std::max(at,live->second.health_clock(life).value_or(at));live->second.cancel_action(life,at);}
                retire_npc_ordinary_locked(source,at);}
            npc_friendly_ordinary_.erase(source.actor);
        }
    }
public:
    struct NpcOpposedGroup {
        FriendlyOrdinaryQuest domain;std::vector<skills::effects::Scope> allies;
        std::map<std::uint64_t,skills::effects::Scope> enemies;
    };
    std::optional<NpcOpposedGroup> npc_opposed_group(FriendlyOrdinaryQuest group)const{
        std::lock_guard lock(mutex_);const auto owner=players_.find(group.owner.actor);
        if(owner==players_.end()||owner->second.effect_scope!=group.owner)return {};
        const auto objective=owner->second.quest_kills.find(group.quest);
        if(objective==owner->second.quest_kills.end()||objective->second.cycle!=group.cycle)return {};
        NpcOpposedGroup result{group,{},{}};const combat::ai::opposition::QuestDomain domain{group.owner,group.quest,group.cycle};
        for(const auto&[scope,binding]:npc_opposed_allies_)if(binding==domain){const auto live=npc_combat_.find(scope.actor);
            if(live!=npc_combat_.end()&&live->second.hex_scope()==scope)result.allies.push_back(scope);}
        for(const auto&[scope,credit]:objective->second.credit_ids)if(objective->second.remaining.contains(scope)){
            const auto live=npc_combat_.find(scope.actor);if(live!=npc_combat_.end()&&live->second.hex_scope()==scope)result.enemies.emplace(credit,scope);}
        return result;
    }
    bool route_npc_opposed_ally(FriendlyOrdinaryQuest group,skills::effects::Scope source,
            float x,float y,unsigned plane,float speed,float stop_range,std::uint64_t now){
        if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(speed)||speed<100||speed>1000||
           !std::isfinite(stop_range)||stop_range<0||stop_range>1248)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto owner=players_.find(group.owner.actor);const auto membership=npc_opposed_allies_.find(source);
        const combat::ai::opposition::QuestDomain domain{group.owner,group.quest,group.cycle};
        if(owner==players_.end()||owner->second.effect_scope!=group.owner||membership==npc_opposed_allies_.end()||membership->second!=domain||
           npc_elemental_player_clock(owner->second)>now)return false;
        const auto objective=owner->second.quest_kills.find(group.quest);
        if(objective==owner->second.quest_kills.end()||objective->second.cycle!=group.cycle)return false;
        auto live=npc_combat_.find(source.actor);auto body=npcs_.find(source.actor);
        if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=source||
           live->second.health_clock(live->second.lifetime()).value_or(now)>now)return false;
        refresh_npc_movement_locked(source.actor,now);live=npc_combat_.find(source.actor);body=npcs_.find(source.actor);
        if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=source)return false;
        const auto life=live->second.lifetime();
        if(!live->second.healable_ally(life)||live->second.active_action(life,now)||!live->second.ordinary_action_ready(life,now)||live->second.knocked_down(life,now))return false;
        if(body->second.plane==plane&&std::hypot(body->second.x-x,body->second.y-y)<=stop_range){
            if(quest_npc_motion_.erase(source.actor))for(auto&[id,p]:players_)p.outbox->enqueue({position_packet(source.actor,body->second.x,body->second.y,body->second.plane),stop_packet(source.actor)});
            return false;
        }
        std::deque<navigation::SurfacePoint> route;
        if(navigation_){const navigation::Point from{body->second.x,body->second.y},to{x,y};
            const auto start=navigation_->locate_cell(body->second.plane,from),end=navigation_->locate_cell(plane,to);
            if(!start||!end)return false;
            const auto points=layered_->route({body->second.plane,*start},from,{plane,*end},to);if(!points)return false;
            for(std::size_t i=1;i<points->size();++i)route.push_back((*points)[i]);
        }else{if(body->second.plane!=plane)return false;route.push_back({{x,y},{plane,0}});}
        if(route.empty())return false;
        QuestNpcMotion motion{source,std::move(route),speed,now,group.owner};motion.group_attempt=domain;
        quest_npc_motion_.insert_or_assign(source.actor,std::move(motion));const auto& next=quest_npc_motion_.at(source.actor).route.front();
        for(auto&[id,p]:players_)p.outbox->enqueue({agent::movement_base_speed_2022(source.actor,speed),agent::movement_speed_2022(source.actor,1.f,1),
            destination_packet(source.actor,next.point.x,next.point.z,static_cast<std::uint16_t>(next.surface.plane),body->second.plane)});
        return true;
    }
