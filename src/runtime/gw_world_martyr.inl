// Included inside Instance. Trusted completion;normal298casting remains staged.
public:
    bool complete_martyr(skills::effects::Scope owner,const skills::effects::MartyrPlan& plan,unsigned divine,std::uint64_t now){
        const auto canonical=skills::effects::martyr();if(!canonical||*canonical!=plan||divine>20)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);settle_parasitic_returns_locked(now);
        const auto source=players_.find(owner.actor);
        if(source==players_.end()||source->second.effect_scope!=owner||!source->second.resources||source->second.precise_health<=0||
           source->second.knocked_down_until>now||now<source->second.health_updated||(divine&&!monk_primary(source->second))||
           !advance_health_locked(source->second,now)||source->second.precise_health<=0)return false;
        auto& caster=source->second;const auto origin=healing_position_locked(caster,now);if(!origin)return false;
        std::vector<skills::effects::MartyrAlly> observed;
        for(auto& [id,ally]:players_){
            if(id==owner.actor||!ally.resources||!caster.team_token||ally.team_token!=caster.team_token)continue;
            const auto point=healing_position_locked(ally,now);if(!point)return false;
            const auto distance=float(std::hypot(point->x-origin->x,point->z-origin->z));if(distance>plan.range)continue;
            if(now<ally.health_updated||!advance_health_locked(ally,now))return false;
            observed.push_back({ally.effect_scope,distance,ally.precise_health>0,true,ally.conditions});
        }
        for(auto& [id,live]:npc_combat_){
            const auto life=live.lifetime();if(!live.healable_ally(life))continue;
            const auto body=npcs_.find(id);if(body==npcs_.end())return false;
            refresh_npc_movement_locked(id,now);
            const auto distance=float(std::hypot(body->second.x-origin->x,body->second.y-origin->z));if(distance>plan.range)continue;
            if(const auto clock=live.health_clock(life);clock&&now<*clock)return false;
            advance_npc_conditions_locked(id,live,now);const auto conditions=live.condition_snapshot(life);if(!conditions)return false;
            observed.push_back({live.hex_scope(),distance,live.healable_ally(life),true,*conditions});
        }
        if(caster.precise_health<=0)return false;
        const auto proposal=skills::effects::prepare_martyr(plan,owner,caster.conditions,observed,now);if(!proposal)return false;
        unsigned count=0;for(const auto& ally:proposal->allies)count+=unsigned(ally.removed.size());
        if(caster.next_health_effect>UINT32_MAX-count)return false;
        std::optional<skills::Slot> dazed_interrupt;
        if(!preflight_new_dazed_locked(caster,proposal->conditions,now,dazed_interrupt))return false;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;
        std::set<const Outbox*> seen;for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;for(auto& [box,queue]:recipients)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        // Freeze recipients and exact original condition receipts before shared
        // callbacks. Never assign a copied final state over reaction effects.
        for(const auto& transfer:proposal->allies){
            const auto snapshot=std::find_if(observed.begin(),observed.end(),[&](const auto& a){return a.scope==transfer.scope;});
            for(const auto kind:transfer.removed){
                if(caster.precise_health<=0)break;
                const auto original=snapshot->conditions.lookup(kind);if(!original||original->expires<=now)continue;
                bool removed=false;
                if(const auto donor=players_.find(transfer.scope.actor);donor!=players_.end()){
                    if(donor->second.effect_scope!=transfer.scope||donor->second.precise_health<=0)continue;
                    const auto current=donor->second.conditions.lookup(kind);
                    if(current&&current->order==original->order&&current->expires==original->expires){remove_condition_locked(donor->second,kind,now);removed=true;}
                }else{
                    const auto npc_donor=npc_combat_.find(transfer.scope.actor);
                    if(npc_donor==npc_combat_.end()||npc_donor->second.hex_scope()!=transfer.scope)continue;
                    auto& live=npc_donor->second;const auto life=live.lifetime();const auto current=live.condition_snapshot(life)->lookup(kind);
                    if(current&&current->order==original->order&&current->expires==original->expires){
                        removed=live.remove_condition(life,kind,now);publish_npc_fragility_locked(transfer.scope.actor,live);
                        refresh_npc_movement_locked(transfer.scope.actor,now);
                        const auto actor=live.actor(life);for(auto& [id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(transfer.scope.actor,actor->effects),combat::detail::fraction(transfer.scope.actor,*actor->health)});
                        commit_applications_locked(transfer.scope,now);
                    }
                }
                if(removed&&caster.precise_health>0){auto next=caster.conditions;
                    if(!next.apply(kind,owner.actor,now,original->expires-now))throw std::logic_error("preflighted Martyr transfer failed");
                    commit_conditions_locked(caster,next,{kind},now,dazed_interrupt);
                }
            }
        }
        if(divine&&caster.precise_health>0){const auto before=caster.precise_health;health_event_locked(caster,std::lround(3.2*divine),now,true);publish_health_locked(caster);
            if(caster.precise_health>before){const auto feedback=skills::particles::health_change_2022(owner.actor,owner.actor,caster.precise_health-before,caster.resources->max_health);for(auto& [id,peer]:players_)peer.outbox->enqueue({feedback});}}
        dispatch_fevered_boundary_locked(now);captures.clear();
        const auto body=skills::particles::target_body_2022(owner.actor,298);if(!body)throw std::logic_error("Martyr body unavailable");
        for(auto& [box,queue]:recipients){std::vector<Packet> batch{combat::detail::integer(58,owner.actor,0),*body};batch.insert(batch.end(),queue.packets.begin(),queue.packets.end());box->enqueue(std::move(batch));}
        return true;
    }

