// Target-centered completion slice. NPC summon identity is an explicit
// catalogue fact; an unknown identity cannot be silently treated as ordinary.
// The normal skill gateway remains unbound pending cast/visual verification.
public:
    bool stage_spiritual_pain(skills::effects::Scope source,
            skills::effects::Scope target,
            const skills::effects::SpiritualPainPlan& plan,unsigned level,
            std::uint64_t now,std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        if(skills::effects::spiritual_pain(plan.rank)!=plan||!level||level>255||
           source==target)return false;
        std::lock_guard lock(mutex_);
        settle_damage_fields_locked(now);
        const auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.resources||!caster->second.team_token||
           caster->second.knocked_down_until>now||
           !advance_health_locked(caster->second,now)||
           caster->second.precise_health<=0)return false;
        const auto origin=healing_position_locked(caster->second,now);
        if(!origin)return false;
        const auto hostile_npc=[&](std::uint32_t id,
                const combat::npcs::LiveActor& live){
            if(live.hostile_to_players(live.lifetime()))return true;
            const auto owned=quest_npc_owners_.find(id);
            if(owned==quest_npc_owners_.end())return false;
            const auto owner=players_.find(owned->second.first.actor);
            return owner!=players_.end()&&
                owner->second.effect_scope==owned->second.first&&
                owner->second.team_token&&
                owner->second.team_token!=caster->second.team_token;
        };

        auto selected_player=players_.find(target.actor);
        std::optional<navigation::Point> center;
        unsigned plane=0;
        if(selected_player!=players_.end()){
            auto& p=selected_player->second;
            if(p.effect_scope!=target||!p.resources||!p.team_token||
               p.team_token==caster->second.team_token||
               !advance_health_locked(p,now)||p.precise_health<=0)return false;
            center=healing_position_locked(p,now);
            plane=p.surface.plane;
        }else{
            auto placement=npcs_.find(target.actor);
            auto live=npc_combat_.find(target.actor);
            if(placement==npcs_.end()||live==npc_combat_.end()||
               live->second.hex_scope()!=target||
               !hostile_npc(target.actor,live->second))return false;
            refresh_npc_movement_locked(target.actor,now);
            const auto life=live->second.lifetime();
            const auto* actor=live->second.actor(life);
            const auto* traits=live->second.holy_traits(life);
            if(!actor||!actor->damageable||!actor->health||
               actor->health->current<=0||!traits||!traits->summoned)return false;
            center=navigation::Point{placement->second.x,placement->second.y};
            plane=placement->second.plane;
        }
        if(!center||plane!=caster->second.surface.plane||
           std::hypot(center->x-origin->x,center->z-origin->z)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,*origin,*center);
            if(!line||line->plane!=plane)return false;}

        std::vector<combat::Hit> player_hits;
        if(selected_player!=players_.end()){
            auto& p=selected_player->second;
            auto actor=incoming_spell_actor_locked(p,{p.id,p.effect_scope.generation,
                combat::Health{p.precise_health,p.resources->max_health},
                {},p.effects,true},now);
            const auto result=skills::effects::spiritual_pain_target(
                plan,actor,level,true,true,false,0);
            if(!result)return false;
            player_hits=*result;
            if(player_hits.size()!=1||!prepare_player_reactive_seed_locked(p,player_hits.front(),now))return false;
        }
        struct NpcHits{std::uint32_t id;std::vector<combat::Hit> hits;};
        std::vector<NpcHits> npc_hits;
        bool found_selected_npc=selected_player!=players_.end();
        for(auto& [id,live]:npc_combat_){
            auto placement=npcs_.find(id);
            const auto life=live.lifetime();
            if(placement==npcs_.end()||placement->second.plane!=plane||
               !hostile_npc(id,live))continue;
            refresh_npc_movement_locked(id,now);
            const bool selected=live.hex_scope()==target;
            const auto distance=std::hypot(placement->second.x-center->x,
                placement->second.y-center->z);
            if((!selected&&distance>plan.area)||
               now<live.health_clock(life).value_or(now))continue;
            advance_npc_conditions_locked(id,live,now);
            const auto* actor=live.actor(life);
            const auto* traits=live.holy_traits(life);
            if(!actor||!actor->damageable||!actor->health||
               actor->health->current<=0||!traits||!traits->summoned){
                if(selected)return false;
                continue;
            }
            if(!selected&&!*traits->summoned)continue;
            auto proposed=live;
            std::vector<combat::Hit> hits;
            const auto apply=[&](unsigned damage){
                const combat::Damage offer{combat::Kind::armor_ignoring,
                    double(damage),level,1336};
                const auto hit=live.hostile_to_players(life)?
                    proposed.apply_damage(life,offer,now):
                    proposed.apply_opposed_owned_damage(life,offer,now);
                if(hit.reason!=combat::Reason::accepted)return false;
                hits.push_back(hit);return true;
            };
            if(selected&&!apply(plan.primary_damage))return false;
            if(*traits->summoned&&!hits.empty()&&!hits.back().killed){
                if(!apply(plan.summoned_damage))return false;
            }else if(*traits->summoned&&!selected){
                if(!apply(plan.summoned_damage))return false;
            }
            if(selected)found_selected_npc=true;
            if(!hits.empty())npc_hits.push_back({id,std::move(hits)});
        }
        if(!found_selected_npc)return false;

        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,p]:players_)if(seen.insert(p.outbox.get()).second)
            queues.emplace_back(p.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        publish_target_body_locked(target.actor,1336,caster->second.id,nullptr);
        if(selected_player!=players_.end()){
            auto& p=selected_player->second;
            for(const auto& preview:player_hits){
                auto current=incoming_spell_actor_locked(p,{p.id,target.generation,
                    combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true},now);
                const auto hit=combat::apply(current,{combat::Kind::armor_ignoring,double(plan.primary_damage),level,1336},true);
                if(hit.reason!=combat::Reason::accepted||!commit_player_reactive_hit_locked(p,hit,source.actor,now,false,nullptr,false,false))
                    throw std::logic_error("preflighted Spiritual Pain player damage failed");
                if(p.precise_health<=0)break;
            }
        }
        for(const auto& entry:npc_hits){
            auto& live=npc_combat_.at(entry.id);
            const auto& placement=npcs_.at(entry.id);
            const bool selected=live.hex_scope()==target;
            for(std::size_t i=0;i<entry.hits.size();++i){
                const unsigned damage=selected&&i==0?
                    plan.primary_damage:plan.summoned_damage;
                const auto hit=damage_npc_locked(caster->second,placement,live,
                    {combat::Kind::armor_ignoring,double(damage),level,1336},now,
                    nullptr,nullptr,!live.hostile_to_players(live.lifetime()));
                if(hit.reason!=combat::Reason::accepted)
                    throw std::logic_error("preflighted Spiritual Pain NPC damage failed");
                if(hit.killed)break;
            }
        }
        if(caster->second.passive_recovery)
            caster->second.passive_recovery->record(
                recovery::Event::target_enemy_spell_or_signet,now);
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));
        return true;
    }
