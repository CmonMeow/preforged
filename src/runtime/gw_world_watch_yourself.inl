// Trusted single-recipient adapter. Party-wide shout/cost selection is separate.
private:
    void consume_watch_yourself_npc_hit_locked(combat::npcs::LiveActor& live,const combat::Hit& hit,
            bool armor_respecting,std::uint64_t now,std::vector<Packet>* deferred=nullptr){
        const auto result=live.record_watch_yourself_hit(live.lifetime(),hit,armor_respecting,now);
        if(!result||!result->ended)return;
        npc_simple_enchantment_credit_.erase({hit.target,result->owned.effect});
        const auto packet=health_effect_remove(hit.target,result->owned.effect);
        if(deferred)deferred->push_back(packet);else for(auto& [id,peer]:players_)peer.outbox->enqueue({packet});
    }
    combat::ArmorSequence watch_yourself_armor_sequence_locked(const Player& player,combat::Kind kind,std::uint64_t now)const{
        const auto rating=kind==combat::Kind::earth?player.reviewed_earth_armor:kind==combat::Kind::fire?player.reviewed_fire_armor:
            kind==combat::Kind::cold?player.reviewed_cold_armor:kind==combat::Kind::lightning?player.reviewed_lightning_armor:std::nullopt;
        if(!player.watch_yourself||!player.effect_registry||!rating||
           !skills::effects::watch_yourself_armor(*player.effect_registry,*player.watch_yourself,now))return {};
        const auto base=*rating,other=int(skills::effects::stacked_skill_armor(armor_effect_bonus_locked(player,now,false),ward_elemental_bonus_locked(player.effect_scope,now)));
        return [owned=*player.watch_yourself,registry=*player.effect_registry,base,other,now](const combat::Hit* previous)mutable->std::optional<int>{
            if(previous&&owned.remaining&&previous->reason==combat::Reason::accepted&&previous->kind!=combat::Kind::armor_ignoring&&previous->applied>0){
                if(owned.last_damage==UINT64_MAX)return {};
                const auto next=skills::effects::consume_watch_yourself(registry,owned,{owned.target,owned.last_damage+1,now,true,true,previous->applied});
                if(!next)return {};owned=next->owned;
            }
            const auto bonus=skills::effects::watch_yourself_armor(registry,owned,now);
            return base+int(skills::effects::stacked_skill_armor(unsigned(other),bonus));
        };
    }
    combat::ArmorSequence watch_yourself_earth_sequence_locked(const Player& player,std::uint64_t now)const{
        return watch_yourself_armor_sequence_locked(player,combat::Kind::earth,now);
    }
    void consume_watch_yourself_hit_locked(Player& player,double applied,bool armor_respecting,std::uint64_t now){
        if(!player.watch_yourself||!player.effect_registry||!armor_respecting||!std::isfinite(applied)||applied<=0)return;
        const auto& owned=*player.watch_yourself;
        if(owned.last_damage==UINT64_MAX)throw std::logic_error("Watch Yourself damage receipt exhausted");
        const auto next=skills::effects::consume_watch_yourself(*player.effect_registry,owned,
            {player.effect_scope,owned.last_damage+1,now,true,true,applied});
        if(!next)return;player.watch_yourself=next->owned;
        if(next->ended)remove_health_effect_locked(player,next->owned.effect,now);
    }
public:
    // Trusted recipient-only NPC intent. The controller must prove its party
    // selection and pay Adrenaline before exposing a complete NPC shout.
    bool stage_npc_watch_yourself_recipient(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::WatchYourselfPlan& plan,std::uint64_t now){
        const auto canonical=skills::effects::watch_yourself(plan.rank,plan.skill==2858);if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        auto caster=npc_combat_.find(source.actor),recipient=npc_combat_.find(target.actor);
        const auto a=npcs_.find(source.actor),b=npcs_.find(target.actor);
        if(caster==npc_combat_.end()||recipient==npc_combat_.end()||a==npcs_.end()||b==npcs_.end()||
           caster->second.hex_scope()!=source||recipient->second.hex_scope()!=target||next_npc_hex_==UINT32_MAX||
           !caster->second.hostile_to_players(caster->second.lifetime())||!recipient->second.hostile_to_players(recipient->second.lifetime()))return false;
        for(const auto live:{&caster->second,&recipient->second}){
            const auto life=live->lifetime();if(const auto clock=live->health_clock(life);clock&&now<*clock)return false;
            advance_npc_conditions_locked(life.actor,*live,now);refresh_npc_movement_locked(life.actor,now);
            const auto actor=live->actor(life);if(!actor||!actor->health||actor->health->current<=0)return false;
        }
        if(caster->second.knocked_down(caster->second.lifetime(),now)||std::hypot(a->second.x-b->second.x,a->second.y-b->second.y)>plan.range)return false;
        auto proposed=recipient->second;const auto changed=proposed.apply_watch_yourself(proposed.lifetime(),source,plan,next_npc_hex_,now);if(!changed)return false;
        const auto effect=next_npc_hex_++;recipient->second=std::move(proposed);
        for(const auto& old:changed->removed){npc_simple_enchantment_credit_.erase({target.actor,old.instance});for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_remove(target.actor,old.instance)});}
        npc_simple_enchantment_credit_[{target.actor,effect}]=target;
        const auto entry=recipient->second.hex_effect(recipient->second.lifetime(),effect);if(!entry)throw std::logic_error("NPC shout recipient status absent");
        for(auto& [id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue({health_effect_add(*entry,target.actor,now)});return true;
    }
    bool stage_watch_yourself_recipient(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::WatchYourselfPlan& plan,std::uint64_t now){
        const auto canonical=skills::effects::watch_yourself(plan.rank,plan.skill==2858);if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto owner=players_.find(source.actor);
        if(owner==players_.end()||owner->second.effect_scope!=source||owner->second.knocked_down_until>now||!advance_resource_event_locked(owner->second,now)||owner->second.precise_health<=0)return false;
        const auto& caster=owner->second;const auto origin=healing_position_locked(caster,now);if(!origin)return false;
        if(const auto player=players_.find(target.actor);player!=players_.end()){
            auto& recipient=player->second;
            const auto party=parties_.group(static_cast<std::uint16_t>(source.actor));
            if(recipient.effect_scope!=target||!party||party!=parties_.group(static_cast<std::uint16_t>(target.actor))||!advance_resource_event_locked(recipient,now)||recipient.precise_health<=0)return false;
            const auto point=healing_position_locked(recipient,now);
            if(!point||std::hypot(point->x-origin->x,point->z-origin->z)>plan.range||!recipient.effect_registry||!recipient.health_regeneration||recipient.next_health_effect>UINT32_MAX)return false;
            const auto effect=unsigned(recipient.next_health_effect);const auto owned=skills::effects::prepare_watch_yourself(plan,source,target,effect,now);if(!owned)return false;
            auto registry=*recipient.effect_registry;auto timer=*recipient.health_regeneration;
            if(recipient.watch_yourself){const auto old=recipient.watch_yourself->effect;registry.remove(target,old);timer.remove(target,old);}
            const skills::effects::Entry entry{effect,source.actor,plan.skill,plan.rank,skills::effects::Category::other,now,owned->ends};
            if(!registry.apply(target,entry,skills::effects::Replace::same_skill).accepted||!timer.apply(target,{effect,source.actor,plan.skill,0,now,owned->ends}))return false;
            if(recipient.watch_yourself)remove_health_effect_locked(recipient,recipient.watch_yourself->effect,now);
            *recipient.effect_registry=std::move(registry);*recipient.health_regeneration=std::move(timer);
            recipient.watch_yourself=*owned;++recipient.next_health_effect;recipient.health_effects.emplace(effect,entry);
            for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_add(entry,target.actor,now)});return true;
        }
        auto live=npc_combat_.find(target.actor);const auto body=npcs_.find(target.actor);
        if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=target||!live->second.healable_ally(live->second.lifetime())||next_npc_hex_==UINT32_MAX)return false;
        if(const auto clock=live->second.health_clock(live->second.lifetime());clock&&now<*clock)return false;
        advance_npc_conditions_locked(target.actor,live->second,now);refresh_npc_movement_locked(target.actor,now);
        if(std::hypot(body->second.x-origin->x,body->second.y-origin->z)>plan.range)return false;
        auto proposed=live->second;const auto changed=proposed.apply_watch_yourself(proposed.lifetime(),source,plan,next_npc_hex_,now);if(!changed)return false;
        const auto effect=next_npc_hex_++;live->second=std::move(proposed);
        for(const auto& old:changed->removed){npc_simple_enchantment_credit_.erase({target.actor,old.instance});for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_remove(target.actor,old.instance)});}
        npc_simple_enchantment_credit_[{target.actor,effect}]=target;
        const auto entry=live->second.hex_effect(live->second.lifetime(),effect);if(!entry)throw std::logic_error("Watch Yourself NPC status absent");
        for(auto& [id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue({health_effect_add(*entry,target.actor,now)});return true;
    }
    // Effect-only completion stage. Normal Runtime owns shout payment/events.
    // The current party authority contains players; friendly NPC relation alone
    // cannot establish NPC party membership.
    bool stage_watch_yourself_party(skills::effects::Scope source,
            const skills::effects::WatchYourselfPlan& plan,std::uint64_t now,std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        const auto canonical=skills::effects::watch_yourself(plan.rank,plan.skill==2858);if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto owner=players_.find(source.actor);
        if(owner==players_.end()||owner->second.effect_scope!=source||
           !advance_resource_event_locked(owner->second,now)||owner->second.precise_health<=0)return false;
        const auto group=parties_.group(static_cast<std::uint16_t>(source.actor));
        const auto origin=healing_position_locked(owner->second,now);if(!group||!origin)return false;
        std::vector<Player*> recipients;
        for(const auto id:parties_.members(group)){
            const auto found=players_.find(id);if(found==players_.end())return false;
            auto& recipient=found->second;const auto point=healing_position_locked(recipient,now);if(!point)return false;
            if(std::hypot(point->x-origin->x,point->z-origin->z)>plan.range)continue;
            if(!advance_resource_event_locked(recipient,now))return false;
            if(recipient.precise_health>0)recipients.push_back(&recipient);
        }
        if(owner->second.precise_health<=0)return false;
        struct Prepared{Player* player;skills::effects::Registry registry;skills::regeneration::State timer;
            skills::effects::WatchYourselfOwned owned;skills::effects::Entry entry;};
        std::vector<Prepared> prepared;
        // Finish every admission/copy before replacing any live recipient.
        for(auto* recipient:recipients){
            if(recipient->precise_health<=0)continue;
            if(!recipient->effect_registry||!recipient->health_regeneration||recipient->next_health_effect>UINT32_MAX)return false;
            const auto target=recipient->effect_scope;const auto effect=unsigned(recipient->next_health_effect);
            const auto owned=skills::effects::prepare_watch_yourself(plan,source,target,effect,now);if(!owned)return false;
            auto registry=*recipient->effect_registry;auto timer=*recipient->health_regeneration;
            if(recipient->watch_yourself){const auto old=recipient->watch_yourself->effect;registry.remove(target,old);timer.remove(target,old);}
            const skills::effects::Entry entry{effect,source.actor,plan.skill,plan.rank,skills::effects::Category::other,now,owned->ends};
            if(!registry.apply(target,entry,skills::effects::Replace::same_skill).accepted||!timer.apply(target,{effect,source.actor,plan.skill,0,now,owned->ends}))return false;
            prepared.push_back({recipient,std::move(registry),std::move(timer),*owned,entry});
        }
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;for(auto& [box,queue]:queues)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        for(auto& change:prepared){auto& recipient=*change.player;
            if(recipient.watch_yourself)remove_health_effect_locked(recipient,recipient.watch_yourself->effect,now);
            *recipient.effect_registry=std::move(change.registry);*recipient.health_regeneration=std::move(change.timer);
            recipient.watch_yourself=change.owned;++recipient.next_health_effect;recipient.health_effects.emplace(change.entry.instance,change.entry);
            for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_add(change.entry,recipient.id,now)});
        }
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));queues.clear();};
        defer_or_publish_completion_locked(publication,std::move(publish));
        return true;
    }
    std::optional<unsigned> watch_yourself_armor(skills::effects::Scope target,std::uint64_t now){
        std::lock_guard lock(mutex_);const auto player=players_.find(target.actor);
        if(player!=players_.end()){
            if(player->second.effect_scope!=target||!advance_resource_event_locked(player->second,now))return {};
            const auto& p=player->second;return p.precise_health>0&&p.watch_yourself?skills::effects::watch_yourself_armor(*p.effect_registry,*p.watch_yourself,now):0;
        }
        const auto npc=npc_combat_.find(target.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=target)return {};
        advance_npc_conditions_locked(target.actor,npc->second,now);return npc->second.watch_yourself_armor(npc->second.lifetime(),now);
    }
    // Trusted post-commit receipt only. Network clients cannot report damage.
    std::optional<skills::effects::WatchYourselfConsumption> consume_watch_yourself_damage(const skills::effects::WatchYourselfDamage& damage){
        std::lock_guard lock(mutex_);const auto player=players_.find(damage.target.actor);
        if(player!=players_.end()){
            auto& p=player->second;if(p.effect_scope!=damage.target||!p.watch_yourself||damage.at<p.health_updated||!advance_health_locked(p,damage.at))return {};
            const auto result=skills::effects::consume_watch_yourself(*p.effect_registry,*p.watch_yourself,damage);if(!result)return {};
            p.watch_yourself=result->owned;if(result->ended)remove_health_effect_locked(p,result->owned.effect,damage.at);return result;
        }
        const auto npc=npc_combat_.find(damage.target.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=damage.target)return {};
        const auto result=npc->second.consume_watch_yourself_damage(npc->second.lifetime(),damage);if(!result)return {};
        if(result->ended){npc_simple_enchantment_credit_.erase({damage.target.actor,result->owned.effect});for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_remove(damage.target.actor,result->owned.effect)});}return result;
    }

