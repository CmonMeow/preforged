// Included inside Instance: real2058 participants join the shared event frontier.
private:
    bool putrid_has_enchantments_locked(const Player& player,std::uint64_t at)const{
        if(!player.effect_registry)return false;
        const auto active=player.effect_registry->active(player.effect_scope,at);
        return std::any_of(active.begin(),active.end(),[](const auto& entry){return entry.kind==skills::effects::Category::enchantment;});
    }
    bool settle_putrid_profane_entry_locked(Player& player,std::uint64_t at){
        if(well_fields_.empty()||player.precise_health<=0||!putrid_has_enchantments_locked(player,at))return true;
        if(at==UINT64_MAX)return false;
        const auto exposure=well_player_exposure_locked(player,at,at+1);if(!exposure)return false;
        for(const auto& segment:*exposure)if(segment.begins<=double(at)+.5&&double(at)+.5<segment.ends&&
            segment.effect.strip_enchantments){
            strip_well_enchantments_locked(player,at);break;
        }
        return true;
    }
    bool collect_putrid_timeline_locked(std::uint64_t horizon,
            std::vector<skills::effects::PutridBileLink>& links){
        using namespace skills::effects;
        for(const auto& link:putrid_bile_links_.pending()){
            const auto caster=players_.find(link.caster.actor);
            if(caster==players_.end()||caster->second.effect_scope!=link.caster){
                putrid_bile_links_.remove(link.target,link.effect);continue;
            }
            std::optional<Entry> hex;
            if(const auto p=players_.find(link.target.actor);p!=players_.end()){
                if(p->second.effect_scope!=link.target){putrid_bile_links_.remove(link.target,link.effect);continue;}
                const auto& player=p->second;
                if(std::max(player.health_updated,player.energy_updated)>horizon)return false;
                if(player.effect_registry)hex=player.effect_registry->lookup(link.target,link.effect);
            }else{
                const auto npc=npc_combat_.find(link.target.actor);
                if(npc==npc_combat_.end()||npc->second.hex_scope()!=link.target){
                    putrid_bile_links_.remove(link.target,link.effect);continue;
                }
                const auto life=npc->second.lifetime();
                if(npc->second.health_clock(life).value_or(horizon)>horizon||
                   npc->second.energy_clock(life).value_or(horizon)>horizon)return false;
                hex=npc->second.hex_effect(life,link.effect);
            }
            if(!hex||hex->kind!=Category::hex||hex->skill!=2058||hex->cause!=link.caster.actor||
               hex->rank!=link.rank||hex->applied!=link.applied||hex->expires!=link.expires){
                putrid_bile_links_.remove(link.target,link.effect);continue;
            }
            links.push_back(link);
        }
        return true;
    }
    bool putrid_health_boundary_locked(std::uint64_t& boundary){
        using namespace skills::effects;
        std::vector<PutridBileLink> links;
        if(!collect_putrid_timeline_locked(boundary,links))return false;
        for(const auto& link:links){
            const auto found=players_.find(link.target.actor);if(found==players_.end())continue;
            auto& p=found->second;auto limit=boundary;int well_pips=0;
            if(!well_fields_.empty()&&p.health_updated<limit){
                const auto exposure=well_player_exposure_locked(p,p.health_updated,limit);if(!exposure)return false;
                for(const auto& segment:*exposure){
                    for(const double edge:{segment.begins,segment.ends}){
                        const auto at=static_cast<std::uint64_t>(std::ceil(edge));
                        if(p.health_updated<at)limit=std::min(limit,at);
                    }
                    if(segment.begins<=double(p.health_updated)+.5&&double(p.health_updated)+.5<segment.ends){
                        well_pips=segment.effect.health_pips;
                        // Profane can remove regeneration/capacity sources before
                        // any positive-time Health integration. Settle that end
                        // at the current frontier, then forecast the new state.
                        if(segment.effect.strip_enchantments&&putrid_has_enchantments_locked(p,p.health_updated))
                            limit=p.health_updated;
                    }
                }
            }
            boundary=std::min(boundary,next_parasitic_health_boundary_locked(p,limit,well_pips));
        }
        const auto forecast=forecast_putrid_bile(links,boundary,[&](Scope scope)->std::optional<BileTimelineTarget>{
            if(const auto p=players_.find(scope.actor);p!=players_.end()){
                return BileTimelineTarget{p->second.effect_scope,{},p->second.effect_registry?
                    p->second.effect_registry->lookup(scope,putrid_bile_links_.lookup(scope)->effect):std::nullopt,{}};
            }
            const auto npc=npc_combat_.find(scope.actor);if(npc==npc_combat_.end())return {};
            return BileTimelineTarget{npc->second.hex_scope(),npc->second,{},npc_external_health_locked(scope.actor,npc->second,boundary).external};
        });
        if(forecast.reason!=BileForecastReason::okay)return false;
        if(forecast.next)boundary=std::min(boundary,forecast.next->at);
        return true;
    }
    bool dispatch_putrid_health_locked(std::uint64_t at){
        std::vector<skills::effects::PutridBileLink> links;
        if(!collect_putrid_timeline_locked(at,links))return false;
        for(const auto& link:links){
            if(!advance_npc_ai_motion_locked(link.target,at))return false;
            // Materialize already-owned quest routes while movement history is
            // still alive. Death cleanup can erase the route and control state.
            auto motion=quest_npc_motion_.find(link.target.actor);
            if(motion!=quest_npc_motion_.end()&&motion->second.scope==link.target&&
               valid_npc_motion_locked(link.target.actor,motion->second)){
                auto npc=npcs_.find(link.target.actor);auto live=npc_combat_.find(link.target.actor);
                if(npc!=npcs_.end()&&live!=npc_combat_.end()&&live->second.hex_scope()==link.target&&
                   advance_npc_motion_locked(link.target.actor,motion->second,npc->second,live->second,at))
                    quest_npc_motion_.erase(motion);
            }
        }
        for(const auto& link:links){
            // A prior participant's death can consume this link or lifetime.
            if(putrid_bile_links_.lookup(link.target)!=link)continue;
            if(const auto p=players_.find(link.target.actor);p!=players_.end()){
                if(p->second.effect_scope==link.target){
                    if(!advance_health_locked(p->second,at)||!settle_putrid_profane_entry_locked(p->second,at))return false;
                }
            }else if(const auto n=npc_combat_.find(link.target.actor);n!=npc_combat_.end()&&n->second.hex_scope()==link.target){
                advance_npc_conditions_locked(link.target.actor,n->second,at);
            }
            if(link.expires<=at)putrid_bile_links_.remove(link.target,link.effect);
            if(!dispatch_putrid_deaths_locked(at))return false;
        }
        return true;
    }
