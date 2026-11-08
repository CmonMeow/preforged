// Included inside Instance. Trusted effect stage;Runtime owns shout payment.
public:
    bool stage_charge_allies(skills::effects::Scope source,const skills::effects::ChargePlan& plan,
            std::uint64_t now,std::function<void()>* publication=nullptr){
        if(publication)*publication={};const auto canonical=skills::effects::charge(plan.rank);
        if(!canonical||*canonical!=plan||now>UINT64_MAX-plan.duration)return false;
        std::lock_guard lock(mutex_);if(!settle_timed_effects_locked(now))return false;
        const auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||!caster->second.team_token||!caster->second.npc_publication_ready||
           !advance_resource_event_locked(caster->second,now)||caster->second.precise_health<=0)return false;
        const auto origin=healing_position_locked(caster->second,now);if(!origin)return false;
        struct PlayerChange {Player* player;skills::effects::Entry entry;};std::vector<PlayerChange> player_changes;
        for(auto& [id,p]:players_){
            if(p.team_token!=caster->second.team_token||!p.npc_publication_ready||!p.resources||p.precise_health<=0)continue;
            const auto point=healing_position_locked(p,now);if(!point)return false;
            if(std::hypot(point->x-origin->x,point->z-origin->z)>plan.range)continue;
            if(!advance_resource_event_locked(p,now))return false;if(p.precise_health<=0)continue;
            if(!p.effect_registry||!p.health_regeneration||p.next_health_effect>UINT32_MAX)return false;
            const auto entry=skills::effects::prepare_charge(plan,source,p.effect_scope,unsigned(p.next_health_effect),now);if(!entry)return false;
            auto registry=*p.effect_registry;auto timer=*p.health_regeneration;
            const auto change=registry.apply(p.effect_scope,*entry,skills::effects::Replace::same_skill);if(!change.accepted)return false;
            for(const auto& old:change.removed)timer.remove(p.effect_scope,old.instance);
            if(!timer.apply(p.effect_scope,{entry->instance,source.actor,364,0,now,*entry->expires}))return false;
            player_changes.push_back({&p,*entry});
        }
        struct NpcChange {unsigned id,effect;combat::npcs::LiveActor proposed;skills::effects::Applied change;};std::vector<NpcChange> npc_changes;
        auto next=next_npc_hex_;
        for(auto& [id,live]:npc_combat_){
            const auto body=npcs_.find(id);const auto life=live.lifetime();
            if(body==npcs_.end()||!live.healable_ally(life))continue;
            refresh_npc_movement_locked(id,now);
            if(std::hypot(body->second.x-origin->x,body->second.y-origin->z)>plan.range)continue;
            if(now<live.health_clock(life).value_or(now))return false;
            advance_npc_conditions_locked(id,live,now);if(!live.healable_ally(life))continue;
            if(next==UINT32_MAX)return false;auto proposed=live;const auto conditions=proposed.condition_snapshot(life);if(!conditions)return false;
            if(conditions->has(combat::conditions::Kind::crippled,now)&&!proposed.remove_condition(life,combat::conditions::Kind::crippled,now))return false;
            skills::effects::Applied changed;changed.accepted=true;
            if(proposed.actor(life)->health->current>0){const auto applied=proposed.apply_charge(life,source,plan,next,now);if(!applied)return false;changed=*applied;}
            npc_changes.push_back({id,next++,std::move(proposed),std::move(changed)});
        }
        // Every selected recipient has passed preflight. Defer all publication
        // until the caller can prepend the real shout notification/body event.
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;for(auto& [box,queue]:queues)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        for(auto& change:player_changes){auto& p=*change.player;
            advance_movement_effects_locked(p,now);
            if(p.conditions.has(combat::conditions::Kind::crippled,now))remove_condition_locked(p,combat::conditions::Kind::crippled,now);
            if(p.precise_health<=0)continue; // Fragility can kill on the cleanse.
            std::vector<unsigned> old;for(const auto& e:p.effect_registry->active(p.effect_scope,now))if(e.skill==364)old.push_back(e.instance);
            for(auto effect:old)remove_health_effect_locked(p,effect,now);
            auto entry=change.entry;entry.instance=unsigned(p.next_health_effect);
            if(!p.effect_registry->apply(p.effect_scope,entry,skills::effects::Replace::same_skill).accepted||
               !p.health_regeneration->apply(p.effect_scope,{entry.instance,source.actor,364,0,now,*entry.expires}))throw std::logic_error("preflighted Charge player application changed");
            ++p.next_health_effect;p.health_effects.emplace(entry.instance,entry);p.registered_movement=true;p.movement_effects.apply(entry);advance_movement_effects_locked(p,now);
            for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_add(entry,p.id,now)});
        }
        next_npc_hex_=next;
        for(auto& change:npc_changes){auto& live=npc_combat_.at(change.id);live=std::move(change.proposed);const auto scope=live.hex_scope();
            for(const auto& old:change.change.removed){npc_simple_enchantment_credit_.erase({change.id,old.instance});for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_remove(change.id,old.instance)});}
            if(const auto entry=live.hex_effect(live.lifetime(),change.effect)){
                npc_simple_enchantment_credit_[{change.id,change.effect}]=scope;for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_add(*entry,change.id,now)});
            }
            for(auto& [id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(change.id,live.actor(live.lifetime())->effects)});
            publish_npc_fragility_locked(change.id,live);publish_npc_simple_enchantment_ends_locked(live);
            refresh_npc_movement_locked(change.id,now);
        }
        dispatch_fevered_boundary_locked(now);captures.clear();auto publish=[queues=std::move(queues)]()mutable{for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));queues.clear();};
        defer_or_publish_completion_locked(publication,std::move(publish));return true;
    }
