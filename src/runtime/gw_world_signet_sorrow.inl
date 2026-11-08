private:
    // Read the selected foe and corpse positions from this World instance.
    // Pet ownership is supplied by a trusted spawn binding, never inferred
    // from an NPC model or the location of an arbitrary corpse.
    std::optional<skills::effects::SignetSorrowFacts> signet_sorrow_facts_locked(
            skills::effects::Scope source,skills::effects::Scope selected,
            std::uint64_t now)const{
        const auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.resources||!caster->second.team_token||
           caster->second.precise_health<=0||
           caster->second.knocked_down_until>now)return {};
        const auto origin=healing_position_locked(caster->second,now);
        if(!origin)return {};
        float x=0,z=0;std::uint16_t plane=0;
        if(const auto target=players_.find(selected.actor);target!=players_.end()){
            if(target->second.effect_scope!=selected||!target->second.resources||
               target->second.precise_health<=0||!target->second.team_token||
               target->second.team_token==caster->second.team_token)return {};
            const auto point=healing_position_locked(target->second,now);
            if(!point)return {};
            x=point->x;z=point->z;plane=target->second.surface.plane;
        }else{
            const auto body=npcs_.find(selected.actor);
            const auto live=npc_combat_.find(selected.actor);
            if(body==npcs_.end()||live==npc_combat_.end()||
               live->second.hex_scope()!=selected||
               !live->second.hostile_to_players(live->second.lifetime()))return {};
            const auto* actor=live->second.actor(live->second.lifetime());
            if(!actor||!actor->damageable||!actor->health||actor->health->current<=0)return {};
            x=body->second.x;z=body->second.y;plane=body->second.plane;
        }
        const float distance=std::hypot(origin->x-x,origin->z-z);
        if(plane!=caster->second.surface.plane||distance>1248)return {};
        if(navigation_){const auto line=layered_->trace(caster->second.surface,*origin,{x,z});
            if(!line||line->plane!=plane)return {};}
        skills::effects::SignetSorrowFacts facts{true,true,false,distance,{}};
        for(const auto& [id,body]:npcs_){
            if(body.plane!=plane)continue;
            const auto live=npc_combat_.find(id);
            if(live==npc_combat_.end())continue;
            const auto life=live->second.lifetime();
            const auto* actor=live->second.actor(life);
            if(!actor||!actor->damageable||!actor->health||actor->health->current!=0||
               live->second.fleshy(life)!=true)continue;
            const skills::effects::Scope corpse{instance_scope_,id,life.actor_generation};
            if(!corpse_reservations_.available(corpse))continue;
            const float nearby=std::hypot(x-body.x,z-body.y);
            if(!facts.nearest_corpse_distance||nearby<*facts.nearest_corpse_distance)
                facts.nearest_corpse_distance=nearby;
        }
        for(const auto& [id,player]:players_){
            if(player.surface.plane!=plane||player.precise_health>0||
               !(player.effects&0x10)||!player.died_at||*player.died_at>now||
               !player.resources||!corpse_reservations_.available(player.effect_scope))continue;
            const auto position=healing_position_locked(player,now);
            if(!position)continue;
            const float nearby=std::hypot(x-position->x,z-position->z);
            if(!facts.nearest_corpse_distance||nearby<*facts.nearest_corpse_distance)
                facts.nearest_corpse_distance=nearby;
        }
        for(const auto& [pet,owner]:pet_owners_){
            if(owner!=selected)continue;
            const auto live=npc_combat_.find(pet.actor);
            if(live==npc_combat_.end()||live->second.hex_scope()!=pet)continue;
            const auto* actor=live->second.actor(live->second.lifetime());
            if(actor&&actor->damageable&&actor->health&&actor->health->current==0){
                facts.dead_pet=true;break;
            }
        }
        return facts;
    }
public:
    // Only the authoritative companion-spawn path may call this. It is not
    // exposed to client packets; both current generations must be live.
    bool bind_pet_owner(skills::effects::Scope owner,skills::effects::Scope pet){
        std::lock_guard lock(mutex_);
        if(!owner.instance||owner.instance!=instance_scope_||pet.instance!=instance_scope_||
           owner==pet||pet_owners_.size()>=1024)return false;
        const auto player=players_.find(owner.actor);
        if(player!=players_.end()){
            if(player->second.effect_scope!=owner||player->second.precise_health<=0)return false;
        }else{
            const auto npc=npc_combat_.find(owner.actor);
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=owner)return false;
            const auto* actor=npc->second.actor(npc->second.lifetime());
            if(!actor||!actor->health||actor->health->current<=0)return false;
        }
        const auto companion=npc_combat_.find(pet.actor);
        if(companion==npc_combat_.end()||companion->second.hex_scope()!=pet)return false;
        const auto* actor=companion->second.actor(companion->second.lifetime());
        if(!actor||!actor->damageable||!actor->health||actor->health->current<=0)return false;
        if(const auto existing=pet_owners_.find(pet);existing!=pet_owners_.end()&&existing->second!=owner)return false;
        if(!corpse_reservations_.can_pre_exploit(pet))return false;
        const auto [it,inserted]=pet_owners_.emplace(pet,owner);
        if(!inserted&&it->second!=owner)return false;
        if(!corpse_reservations_.pre_exploit(pet))throw std::logic_error("validated pet corpse policy changed");
        return true;
    }
    struct SignetSorrowCompletion {
        bool accepted=false;
        unsigned recharge_ms=30000;
        std::vector<Packet> visuals;
        std::function<void()> publication;
    };
    SignetSorrowCompletion complete_signet_sorrow(
            skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::SignetSorrowPlan& plan,unsigned level,
            std::uint64_t now){
        if(skills::effects::signet_sorrow(plan.rank)!=plan)return {};
        std::lock_guard lock(mutex_);
        settle_damage_fields_locked(now);
        const auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !resurrection_clock_valid(caster->second,now)||
           !advance_health_locked(caster->second,now))return {};
        auto facts=signet_sorrow_facts_locked(source,target,now);
        if(!facts)return {};
        auto prepared=skills::effects::prepare_signet_sorrow(plan,level,*facts);
        if(!prepared)return {};
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        const auto begin_publication=[&]{
            std::set<const Outbox*> seen;
            for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
                queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
            for(auto& [box,queue]:queues)
                captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        };
        if(const auto victim=players_.find(target.actor);victim!=players_.end()){
            auto& player=victim->second;
            if(!resurrection_clock_valid(player,now)||!advance_health_locked(player,now)||
               player.precise_health<=0)return {};
            auto actor=incoming_spell_actor_locked(player,{player.id,target.generation,
                combat::Health{player.precise_health,player.resources->max_health},
                {},player.effects,true},now);
            const auto hit=combat::apply(actor,prepared->damage,true);
            if(hit.reason!=combat::Reason::accepted||!preflight_player_reversal_locked(player,hit,now))return {};
            const auto seed=prepare_player_reactive_seed_locked(player,hit,now);
            if(!seed)return {};
            begin_publication();
            // Generic integer42 is maximum Health, not an animation resource.
            for(auto& [id,peer]:players_)peer.outbox->enqueue({combat::detail::integer(42,target.actor,player.resources->max_health)});
            if(!commit_player_reactive_hit_locked(player,hit,source.actor,now,false,&*seed))
                throw std::logic_error("preflighted Signet of Sorrow player damage failed");
            if(player.passive_recovery)player.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
        }else{
            const auto body=npcs_.find(target.actor);
            const auto found=npc_combat_.find(target.actor);
            if(body==npcs_.end()||found==npc_combat_.end()||
               found->second.hex_scope()!=target||
               found->second.health_clock(found->second.lifetime()).value_or(0)>now)return {};
            auto preview=found->second;
            if(!preflight_npc_reversal_or_damage_locked(preview,prepared->damage,now))return {};
            const auto maximum=preview.actor(preview.lifetime())->health->maximum;
            begin_publication();
            for(auto& [id,peer]:players_)peer.outbox->enqueue({combat::detail::integer(42,target.actor,maximum)});
            if(!commit_npc_reversal_or_damage_locked(caster->second,body->second,found->second,prepared->damage,now))
                throw std::logic_error("preflighted Signet of Sorrow damage failed");
        }
        if(caster->second.passive_recovery)
            caster->second.passive_recovery->record(recovery::Event::target_enemy_spell_or_signet,now);
        captures.clear();
        SignetSorrowCompletion result{true,prepared->recharge_ms,{}, {}};
        defer_or_publish_completion_locked(&result.publication,[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        });
        return result;
    }
    std::optional<skills::effects::SignetSorrowFacts> signet_sorrow_facts(
            skills::effects::Scope source,skills::effects::Scope selected,
            std::uint64_t now)const{
        std::lock_guard lock(mutex_);
        return signet_sorrow_facts_locked(source,selected,now);
    }
