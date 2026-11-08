// Included inside Instance. Staged trusted host services, not client commands.
private:
    struct VigorousReceipts {
        skills::effects::Scope owner;
        std::array<std::optional<skills::effects::VigorousAction>,3> last;
    };
    // At most three terminal receipts per actor; a new generation replaces the
    // old ledger. Player departure releases its entry as well.
    std::map<unsigned,VigorousReceipts> vigorous_healing_receipts_;
    bool vigorous_release_locked(skills::effects::Scope owner,std::uint64_t now,
            skills::effects::VigorousActionKind kind,unsigned skill=0){
        // This callback runs at an accepted attack-skill release, after its
        // activation interval. The frozen Heket/Triple Shot sequence removes
        // the stance on this boundary, before the first projectile launch.
        const auto end_hekets=[&]()->bool{
            if(kind!=skills::effects::VigorousActionKind::attack||!skill)return true;
            const auto* row=skills::find_skill(skill);
            if(!row||row->type!=14)return true;
            if(const auto p=players_.find(owner.actor);p!=players_.end()&&p->second.effect_scope==owner){
                if(!p->second.effect_registry)return false;
                const auto stance=skills::effects::active_hekets_rampage(*p->second.effect_registry,owner,now);
                if(!stance)return true;
                if(!p->second.effect_registry->remove(owner,stance->instance))return false;
                const auto packet=health_effect_remove(owner.actor,stance->instance);
                for(auto&[id,peer]:players_)peer.outbox->enqueue({packet});
                publish_status_locked(p->second,now);
                return true;
            }
            const auto n=npc_combat_.find(owner.actor);
            if(n==npc_combat_.end()||n->second.hex_scope()!=owner)return false;
            const auto active=n->second.newest_stance(n->second.lifetime(),now);
            if(!active||active->skill!=1728)return true;
            if(!n->second.remove_stance(n->second.lifetime(),active->instance,now))return false;
            const auto packet=health_effect_remove(owner.actor,active->instance);
            for(auto&[id,peer]:players_)peer.outbox->enqueue({packet});
            return true;
        };
        // Producers call only at a genuine release/completion boundary. Do not
        // allocate receipts or advance unrelated actors when no254is present.
        bool active=false;
        if(const auto p=players_.find(owner.actor);p!=players_.end()&&p->second.effect_scope==owner)
            active=p->second.effect_registry&&p->second.effect_registry->newest_skill(owner,254,now).has_value();
        else if(const auto n=npc_combat_.find(owner.actor);n!=npc_combat_.end()&&n->second.hex_scope()==owner){
            const auto registry=n->second.owned_effect_snapshot(n->second.lifetime());
            active=registry&&registry->newest_skill(owner,254,now).has_value();
        }else return false;
        if(!active)return end_hekets();
        const auto index=static_cast<unsigned>(kind);if(index>=3)return false;
        std::uint64_t event=1;
        if(const auto ledger=vigorous_healing_receipts_.find(owner.actor);ledger!=vigorous_healing_receipts_.end()&&ledger->second.owner==owner)
            if(const auto& last=ledger->second.last[index]){if(last->event==UINT64_MAX)return false;event=last->event+1;}
        return vigorous_action_locked({owner,event,now,kind,skill})&&end_hekets();
    }
    bool vigorous_action_locked(const skills::effects::VigorousAction& action){
        using namespace skills::effects;
        const auto owner=action.actor;const auto now=action.at;
        const auto kind=static_cast<unsigned>(action.kind);if(kind>=3||!action.event)return false;
        auto player=players_.find(owner.actor);auto npc=npc_combat_.find(owner.actor);
        const bool is_player=player!=players_.end();
        if(is_player?player->second.effect_scope!=owner:(npc==npc_combat_.end()||npc->second.hex_scope()!=owner))return false;
        const auto seen=vigorous_healing_receipts_.find(owner.actor);
        if(seen!=vigorous_healing_receipts_.end()&&seen->second.owner==owner){
            const auto& last=seen->second.last[kind];
            if(last){if(action.event<=last->event)return action==*last;if(now<last->at)return false;}
        }
        std::optional<Registry> snapshot;
        if(is_player){auto& p=player->second;
            if(!p.resources||now<p.health_updated||now<p.energy_updated||!advance_resource_event_locked(p,now)||p.precise_health<=0||!p.effect_registry)return false;
            snapshot=*p.effect_registry;
        }else{
            const auto clock=npc->second.health_clock(npc->second.lifetime());if(clock&&now<*clock)return false;
            advance_npc_conditions_locked(owner.actor,npc->second,now);
            npc=npc_combat_.find(owner.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=owner)return false;
            snapshot=npc->second.owned_effect_snapshot(npc->second.lifetime());
            const auto* actor=npc->second.actor(npc->second.lifetime());
            if(!actor||!actor->health||actor->health->current<=0)return false;
        }
        if(!snapshot)return false;
        const auto selected=vigorous_spirit_healing(*snapshot,action);if(!selected)return false;
        if(selected->amount){
            const auto entry=snapshot->lookup(owner,selected->effect);if(!entry)return false;
            if(is_player){auto& p=player->second;const auto before=p.precise_health;
                health_event_locked(p,selected->amount,now,true);const auto gained=p.precise_health-before;
                publish_health_locked(p);
                if(gained>0)for(auto& [id,peer]:players_)peer.outbox->enqueue({skills::particles::health_change_2022(owner.actor,entry->cause,gained,p.resources->max_health)});
                if(p.precise_health<=0)kill_locked(p,now);
            }else{
                const auto life=npc->second.lifetime();const auto allowed=npc->second.healing_allowed(life);if(!allowed)return false;
                if(*allowed){auto proposed=npc->second;const auto before=proposed.actor(life)->health->current;
                    if(!proposed.heal(life,selected->amount,now))return false;
                    const auto health=*proposed.actor(life)->health;const auto gained=health.current-before;npc->second=std::move(proposed);
                    for(auto& [id,peer]:players_){std::vector<Packet> packets{combat::detail::fraction(owner.actor,health)};
                        if(gained>0)packets.push_back(skills::particles::health_change_2022(owner.actor,entry->cause,gained,health.maximum));peer.outbox->enqueue(std::move(packets));}
                }
            }
        }
        auto& ledger=vigorous_healing_receipts_[owner.actor];if(ledger.owner!=owner)ledger=VigorousReceipts{owner,{}};
        ledger.last[kind]=action;return true;
    }
public:
    // Trusted self-buff stage; AI admission/payment and a real cast are separate.
    bool stage_npc_vigorous_spirit(skills::effects::Scope owner,const skills::effects::VigorousSpiritPlan& plan,std::uint64_t now){return stage_npc_simple_enchantment<254>(owner,plan,now);}
    bool complete_vigorous_spirit(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::VigorousSpiritPlan& plan,unsigned divine,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr,std::optional<std::uint64_t> completion_event={}){
        return complete_simple_enchantment<254>(source,target,plan,divine,now,adjusted,completion_event);
    }
    // Producers must use monotonically ordered actual-action IDs per kind.
    // Recipient impacts/hits are not action IDs. Replays are idempotent; older
    // or conflicting receipts reject. Ordinary-player and completed-spell
    // producers, current melee/bow/area skill attacks and ordinary/quest NPC
    // releases are wired. New attack families must supply actual release counts.
    bool vigorous_action(const skills::effects::VigorousAction& action){
        std::lock_guard lock(mutex_);return vigorous_action_locked(action);
    }
