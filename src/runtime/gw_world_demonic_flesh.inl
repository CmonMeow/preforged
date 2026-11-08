// Included inside Instance. The self effect and skill-use replay authority
// stay in World; clients cannot supply a successful-use or allegiance fact.
public:
    bool demonic_flesh(skills::effects::Scope owner,unsigned effect,const skills::effects::DemonicFleshPlan& plan,
            std::uint64_t now,std::vector<Packet>* visuals=nullptr){
        if(visuals)visuals->clear();const auto canonical=skills::effects::demonic_flesh(plan.rank);
        if(!canonical||plan!=*canonical||!effect||effect>=0x80000000u||now>UINT64_MAX-plan.duration)return false;
        std::lock_guard lock(mutex_);const auto found=players_.find(owner.actor);
        if(found==players_.end()||found->second.effect_scope!=owner)return false;auto& p=found->second;
        if(!p.resources||!p.effect_registry||!advance_health_locked(p,now)||p.precise_health<=0||!p.health_regeneration||p.effect_registry->lookup(owner,effect))return false;
        if(well_blocks_player_enchantment_locked(p,now)){
            const Packet body=combat::detail::integer(21,owner.actor,240);
            if(visuals)visuals->push_back(body);
            else for(auto&[id,peer]:players_)peer.outbox->enqueue({body});
            return true;
        }
        const skills::effects::Entry entry{effect,owner.actor,130,plan.rank,skills::effects::Category::enchantment,now,now+plan.duration};
        auto registry=*p.effect_registry;auto timers=*p.health_regeneration;
        const auto changed=registry.apply(owner,entry,skills::effects::Replace::same_skill);if(!changed.accepted)return false;
        for(const auto& old:changed.removed)timers.remove(owner,old.instance);
        if(!timers.apply(owner,{effect,owner.actor,130,0,now,*entry.expires}))return false;
        *p.effect_registry=std::move(registry);*p.health_regeneration=std::move(timers);
        for(const auto& old:changed.removed){p.health_effects.erase(old.instance);for(auto&[id,peer]:players_)peer.outbox->enqueue({health_effect_remove(owner.actor,old.instance)});}
        p.health_effects.emplace(effect,entry);publish_status_locked(p,now);
        std::vector<Packet> out{combat::detail::integer(21,owner.actor,240),health_effect_add(entry,owner.actor,now)};
        if(visuals)*visuals=std::move(out);else for(auto&[id,peer]:players_)peer.outbox->enqueue(out);
        commit_applications_locked(owner,now);return true;
    }
private:
    struct PendingDemonicUse {
        struct Recipient {skills::effects::Scope scope;std::uint64_t death_sequence=0;};
        skills::effects::DemonicFleshReservation receipt;
        skills::effects::DemonicFleshPlan plan;
        unsigned level=0;std::uint64_t at=0;bool committed=false;
        std::vector<Recipient> remaining;
    };
    std::map<std::uint64_t,PendingDemonicUse> pending_demonic_uses_;
    std::uint64_t next_demonic_reservation_=1;
    void retry_demonic_flesh_uses_locked(std::uint64_t now){
        using namespace skills::effects;
        for(auto it=pending_demonic_uses_.begin();it!=pending_demonic_uses_.end();){
            auto& use=it->second;
            if(!use.committed||now<use.at){++it;continue;}
            const auto source=players_.find(use.receipt.caster.actor);
            if(source==players_.end()||source->second.effect_scope!=use.receipt.caster){it=pending_demonic_uses_.erase(it);continue;}
            // The successful use is already committed. Same-generation caster
            // death does not undo it; departure retires its attribution.
            std::erase_if(use.remaining,[&](const PendingDemonicUse::Recipient& recipient){
                const auto scope=recipient.scope;
                if(const auto found=players_.find(scope.actor);found!=players_.end()){
                    auto& p=found->second;
                    if(p.effect_scope!=scope||p.death_sequence!=recipient.death_sequence||!p.resources||p.precise_health<=0)return true;
                    if(p.health_updated>now||p.energy_updated>now)return false;
                    if(!advance_health_locked(p,now))return false;
                    if(p.precise_health<=0)return true;
                    auto actor=incoming_spell_actor_locked(p,{p.id,scope.generation,combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true},now);
                    const auto hit=demonic_flesh_hit(use.plan,actor,use.level);if(!hit)return false;
                    // A committed use may retry later; consume its recipient
                    // only once the current protection/reaction receipt commits.
                    return commit_player_reactive_hit_locked(p,*hit,use.receipt.caster.actor,now);
                }
                const auto live=npc_combat_.find(scope.actor);const auto placed=npcs_.find(scope.actor);
                if(live==npc_combat_.end()||placed==npcs_.end()||live->second.hex_scope()!=scope)return true;
                const auto life=live->second.lifetime();const auto actor=live->second.actor(life);
                if(!actor||!actor->damageable||!actor->health||actor->health->current<=0)return true;
                if(live->second.health_clock(life).value_or(now)>now)return false;
                advance_npc_conditions_locked(scope.actor,live->second,now);
                const auto current=npc_combat_.find(scope.actor);const auto position=npcs_.find(scope.actor);
                const auto credited=players_.find(use.receipt.caster.actor);
                if(current==npc_combat_.end()||position==npcs_.end()||current->second.hex_scope()!=scope||
                   credited==players_.end()||credited->second.effect_scope!=use.receipt.caster)return true;
                const auto refreshed_actor=current->second.actor(current->second.lifetime());
                if(!refreshed_actor||!refreshed_actor->health||refreshed_actor->health->current<=0)return true;
                const combat::Damage damage{combat::Kind::armor_ignoring,double(use.plan.damage),use.level,130};
                return commit_npc_reversal_or_damage_locked(credited->second,position->second,current->second,damage,now);
            });
            if(use.remaining.empty())it=pending_demonic_uses_.erase(it);else ++it;
        }
    }
public:
    std::optional<skills::effects::DemonicFleshReservation> prepare_demonic_flesh_use(const skills::effects::DemonicFleshUse& use,unsigned level){
        using namespace skills::effects;
        if(!demonic_scope(use.caster)||!demonic_scope(use.target)||!use.event||!level||level>255)return {};
        std::lock_guard lock(mutex_);const auto found=players_.find(use.caster.actor);
        if(found==players_.end()||found->second.effect_scope!=use.caster)return {};auto& caster=found->second;
        if(!caster.resources||!caster.effect_registry||use.event<=caster.last_demonic_use||use.at<caster.demonic_use_time||caster.precise_health<=0)return {};
        auto observed=use;observed.success=true;observed.foe=false;
        if(const auto p=players_.find(use.target.actor);p!=players_.end()){
            if(p->second.effect_scope!=use.target)return {};
            observed.foe=caster.team_token&&p->second.team_token&&caster.team_token!=p->second.team_token;
        }else{
            const auto npc=npc_combat_.find(use.target.actor);
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=use.target)return {};
            observed.foe=npc->second.hostile_to_players(npc->second.lifetime());
        }
        const auto trigger=demonic_flesh_trigger(*caster.effect_registry,observed);if(!trigger.accepted)return {};
        const auto admitted=[&](std::uint64_t id){caster.last_demonic_use=use.event;caster.demonic_use_time=use.at;return DemonicFleshReservation{use.caster,use.event,id};};
        if(!trigger.plan)return admitted(0);
        const auto origin=healing_position_locked(caster,use.at);if(!origin)return {};
        std::vector<DemonicFleshRecipient> candidates;
        for(auto&[id,p]:players_){
            if(p.effect_scope==use.caster||p.effect_scope==use.target||!caster.team_token||!p.resources||p.precise_health<=0||!p.team_token||p.team_token==caster.team_token)continue;
            if(p.surface.plane!=caster.surface.plane)continue;const auto point=healing_position_locked(p,use.at);if(!point)return {};
            if(std::hypot(point->x-origin->x,point->z-origin->z)>trigger.plan->radius)continue;
            candidates.push_back({p.effect_scope,point->x,point->z,static_cast<std::uint16_t>(p.surface.plane),true,true});
        }
        for(const auto&[id,live]:npc_combat_){
            const auto scope=live.hex_scope();const auto placed=npcs_.find(id);
            if(placed==npcs_.end()||scope==use.target||!live.hostile_to_players(live.lifetime())||placed->second.plane!=caster.surface.plane)continue;
            // Route mutation is not part of reservation. Refuse a snapshot
            // behind its authoritative movement cursor rather than rewind it.
            if(const auto motion=quest_npc_motion_.find(id);motion!=quest_npc_motion_.end()&&motion->second.updated>use.at)return {};
            refresh_npc_movement_locked(id,use.at);
            if(std::hypot(placed->second.x-origin->x,placed->second.y-origin->z)>trigger.plan->radius)continue;
            const auto actor=live.actor(live.lifetime());if(!actor||!actor->damageable)continue;if(!actor->health)return {};
            if(actor->health->current<=0)continue;
            candidates.push_back({scope,placed->second.x,placed->second.y,placed->second.plane,true,true});
        }
        const auto selected=demonic_flesh_recipients(*trigger.plan,observed,origin->x,origin->z,static_cast<std::uint16_t>(caster.surface.plane),candidates);
        if(!selected||selected->size()>4096)return {};
        if(selected->empty())return admitted(0);
        if(pending_demonic_uses_.size()>=4096||!next_demonic_reservation_)return {};
        std::vector<PendingDemonicUse::Recipient> recipients;recipients.reserve(selected->size());
        for(const auto scope:*selected){
            const auto player=players_.find(scope.actor);
            recipients.push_back({scope,player==players_.end()?0:player->second.death_sequence});
        }
        const auto receipt=admitted(next_demonic_reservation_++);
        pending_demonic_uses_.emplace(receipt.id,PendingDemonicUse{receipt,*trigger.plan,level,use.at,false,std::move(recipients)});
        return receipt;
    }
    bool commit_demonic_flesh_use(const skills::effects::DemonicFleshReservation& receipt){
        using namespace skills::effects;
        if(!demonic_scope(receipt.caster)||!receipt.event)return false;
        if(!receipt.id)return true;
        std::lock_guard lock(mutex_);const auto found=pending_demonic_uses_.find(receipt.id);
        if(found==pending_demonic_uses_.end()||found->second.receipt!=receipt)return false;
        found->second.committed=true;return true;
    }
    bool cancel_demonic_flesh_use(const skills::effects::DemonicFleshReservation& receipt){
        using namespace skills::effects;
        if(!demonic_scope(receipt.caster)||!receipt.event)return false;
        if(!receipt.id)return true;
        std::lock_guard lock(mutex_);const auto found=pending_demonic_uses_.find(receipt.id);
        if(found==pending_demonic_uses_.end()||found->second.receipt!=receipt||found->second.committed)return false;
        pending_demonic_uses_.erase(found);return true;
    }
    bool demonic_flesh_skill_used(const skills::effects::DemonicFleshUse& use,unsigned level){
        const auto receipt=prepare_demonic_flesh_use(use,level);if(!receipt)return false;
        if(!use.success)return cancel_demonic_flesh_use(*receipt);
        if(!commit_demonic_flesh_use(*receipt))return false;
        std::lock_guard lock(mutex_);retry_demonic_flesh_uses_locked(use.at);return true;
    }
