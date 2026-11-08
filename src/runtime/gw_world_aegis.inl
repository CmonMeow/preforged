public:
    // PvE party enchantment completion. Casting/payment remains caller-owned.
    bool complete_aegis(skills::effects::Scope source,const skills::effects::AegisPlan& plan,
            unsigned divine,std::uint64_t now,const skills::effects::EnchantmentDuration* adjusted=nullptr,
            std::optional<std::uint64_t> completion_event={}){
        const auto canonical=skills::effects::aegis(plan.rank);
        const auto duration=adjusted?adjusted->for_enchantment(257,plan.duration,source,now):std::optional(plan.duration);
        if(!canonical||*canonical!=plan||divine>20||!duration||now>UINT64_MAX-*duration)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto owner=players_.find(source.actor);
        if(owner==players_.end()||owner->second.effect_scope!=source||!owner->second.resources||
           !resurrection_clock_valid(owner->second,now)||!advance_health_locked(owner->second,now)||
           owner->second.precise_health<=0||owner->second.knocked_down_until>now||
           (divine&&!monk_primary(owner->second)))return false;
        auto& caster=owner->second;
        if(completion_event&&!complete_ether_renewal_locked(source,257,*completion_event,now,true,false))return false;
        if(adjusted&&caster.effect_registry&&caster.effect_registry->newest_skill(source,256,now))return false;
        const auto origin=healing_position_locked(caster,now);const auto group=parties_.group(static_cast<std::uint16_t>(source.actor));
        if(!origin||!group)return false;
        std::vector<skills::effects::AegisMember> members;
        for(auto id:parties_.members(group)){
            const auto found=players_.find(id);if(found==players_.end()||!found->second.resources)continue;
            auto& player=found->second;
            if(!resurrection_clock_valid(player,now)||!advance_health_locked(player,now))return false;
            const auto point=healing_position_locked(player,now);if(!point)return false;
            members.push_back({player.effect_scope,std::hypot(point->x-origin->x,point->z-origin->z),player.precise_health>0,true});
        }
        // Settling another member may end a linked effect or kill an earlier
        // member. Refresh liveness after all mutation-capable advancement.
        if(caster.effect_scope!=source||caster.precise_health<=0)return false;
        for(auto& member:members){const auto found=players_.find(member.scope.actor);
            if(found==players_.end()||found->second.effect_scope!=member.scope)return false;
            member.living=found->second.precise_health>0;
        }
        const auto selected=skills::effects::aegis_members(plan,source,members);if(!selected)return false;
        struct Proposal {Player* player;skills::effects::Registry registry;skills::regeneration::State regeneration;skills::effects::Entry entry;};
        std::vector<Proposal> proposals;std::vector<Packet> statuses;
        const auto prepare=[&]()->bool{
        proposals.clear();statuses.clear();
        for(const auto scope:*selected){
            auto& player=players_.at(scope.actor);
            // A party spell still completes when one member stands in a
            // hostile Profane field; only that recipient loses admission.
            if(well_blocks_player_enchantment_locked(player,now))continue;
            if(player.effect_scope!=scope||!player.effect_registry||!player.health_regeneration||player.next_health_effect>UINT32_MAX)return false;
            auto registry=*player.effect_registry;auto regeneration=*player.health_regeneration;
            const auto id=static_cast<unsigned>(player.next_health_effect);
            skills::effects::Entry entry{id,source.actor,257,plan.rank,skills::effects::Category::enchantment,now,now+*duration};
            const auto changed=registry.apply(scope,entry,skills::effects::Replace::same_skill);
            if(!changed.accepted||!regeneration.apply(scope,{id,source.actor,257,0,now,now+*duration}))return false;
            for(const auto& old:changed.removed)statuses.push_back(health_effect_remove(scope.actor,old.instance));
            statuses.push_back(health_effect_add(entry,scope.actor,now));
            proposals.push_back({&player,std::move(registry),std::move(regeneration),entry});
        }
        return true;
        };
        if(!prepare())return false;
        if(completion_event){
            if(!complete_ether_renewal_locked(source,257,*completion_event,now,true))return false;
            if(caster.precise_health<=0){
                for(auto&[id,peer]:players_)peer.outbox->enqueue({combat::detail::integer(58,source.actor,0),combat::detail::integer(21,source.actor,469)});
                return true;
            }
            // Source reward settlement must not be overwritten by a copied
            // caster registry prepared before that settlement.
            if(!prepare())throw std::logic_error("validated Aegis cohort changed during Renewal settlement");
        }
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;std::set<const Outbox*> seen;
        for(auto&[id,peer]:players_)if(seen.insert(peer.outbox.get()).second)queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto&[box,queue]:queues)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        for(auto& proposal:proposals){auto& player=*proposal.player;
            *player.effect_registry=std::move(proposal.registry);*player.health_regeneration=std::move(proposal.regeneration);
            ++player.next_health_effect;std::erase_if(player.health_effects,[](const auto& item){return item.second.skill==257;});
            player.health_effects.emplace(proposal.entry.instance,proposal.entry);
        }
        for(auto&[id,peer]:players_)peer.outbox->enqueue(statuses);
        if(divine){const auto before=caster.precise_health;health_event_locked(caster,unsigned(std::lround(3.2*divine)),now,true);
            if(caster.precise_health>before)for(auto&[id,peer]:players_)peer.outbox->enqueue({skills::particles::health_change_2022(source.actor,source.actor,caster.precise_health-before,caster.resources->max_health)});
            publish_health_locked(caster);
        }
        for(auto& proposal:proposals)publish_status_locked(*proposal.player,now);
        captures.clear();
        for(auto&[box,queue]:queues){std::vector<Packet> batch{combat::detail::integer(58,source.actor,0),combat::detail::integer(21,source.actor,469)};
            batch.insert(batch.end(),std::make_move_iterator(queue.packets.begin()),std::make_move_iterator(queue.packets.end()));box->enqueue(std::move(batch));}
        return true;
    }
