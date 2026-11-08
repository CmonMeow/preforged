// Included inside Instance. A lifetime-filtered census of current scoped NPCs.
private:
    bool spirit_census_lifetime_active_locked(skills::effects::Scope spirit,std::uint64_t now)const{
        // Registry find(now) hides expired records; find(0) distinguishes an
        // expired controlled actor awaiting tick cleanup from a static NPC.
        if(const auto record=controlled_spirits_.find(spirit,0);record&&record->expires<=now)return false;
        if(const auto found=quickening_spirits_.find(spirit);found!=quickening_spirits_.end())
            if(found->second.begins>now||found->second.expires<=now)return false;
        return true;
    }
    skills::effects::SpiritProximity spirit_light_proximity_locked(
            skills::effects::Scope caster,std::uint64_t now){
        using skills::effects::SpiritProximity;
        const auto found=players_.find(caster.actor);
        if(found==players_.end()||found->second.effect_scope!=caster||
           !found->second.resources||found->second.precise_health<=0)return SpiritProximity::unknown;
        const auto origin=healing_position_locked(found->second,now);
        const auto plan=skills::effects::spirit_light(0);
        if(!origin||!plan)return SpiritProximity::unknown;
        const auto plane=found->second.surface.plane;
        std::vector<skills::effects::Scope> scopes;
        for(const auto& [id,live]:npc_combat_)scopes.push_back(live.hex_scope());
        // Movement refresh may drain reactions and retire earlier witnesses.
        // Finish every refresh before reading any witness Health/type.
        for(const auto scope:scopes){
            const auto current=npc_combat_.find(scope.actor);
            if(current!=npc_combat_.end()&&current->second.hex_scope()==scope&&
               spirit_census_lifetime_active_locked(scope,now))refresh_npc_movement_locked(scope.actor,now);
        }
        bool unresolved=false,present=false;
        for(const auto scope:scopes){
            if(!spirit_census_lifetime_active_locked(scope,now))continue;
            const auto live=npc_combat_.find(scope.actor);
            if(live==npc_combat_.end()||live->second.hex_scope()!=scope)continue;
            const auto placed=npcs_.find(scope.actor);
            if(placed==npcs_.end())return SpiritProximity::unknown;
            if(placed->second.plane!=plane||std::hypot(double(placed->second.x)-origin->x,
                double(placed->second.y)-origin->z)>plan->earshot)continue;
            const auto life=live->second.lifetime();const auto actor=live->second.actor(life);
            if(!actor||!actor->health||actor->health->current<=0)continue;
            const auto type=live->second.creature_type(life);
            if(!type)unresolved=true;else if(*type=="spirit")present=true;
        }
        const auto source=players_.find(caster.actor);
        if(source==players_.end()||source->second.effect_scope!=caster||!source->second.resources||
           source->second.precise_health<=0)return SpiritProximity::unknown;
        return present?SpiritProximity::present:unresolved?SpiritProximity::unknown:SpiritProximity::absent;
    }
    // Direct spell completion may precede World::tick. Settle nearby stationary
    // actors as well as movers before reading Health; their condition deaths
    // and reactions use the existing NPC authority. Caller must reacquire its
    // selected target after this method, since those reactions can retire it.
    skills::effects::SpiritProximity settled_spirit_light_proximity_locked(
            skills::effects::Scope caster,std::uint64_t now){
        using skills::effects::SpiritProximity;
        if(!settle_timed_effects_locked(now))return SpiritProximity::unknown;
        const auto source=players_.find(caster.actor);
        if(source==players_.end()||source->second.effect_scope!=caster||!source->second.resources||
           source->second.precise_health<=0)return SpiritProximity::unknown;
        const auto origin=healing_position_locked(source->second,now);
        const auto plan=skills::effects::spirit_light(0);
        if(!origin||!plan)return SpiritProximity::unknown;
        const auto plane=source->second.surface.plane;
        std::vector<skills::effects::Scope> scopes;
        for(const auto& [id,live]:npc_combat_)scopes.push_back(live.hex_scope());
        for(const auto scope:scopes){
            const auto current=npc_combat_.find(scope.actor);
            if(current==npc_combat_.end()||current->second.hex_scope()!=scope||
               !spirit_census_lifetime_active_locked(scope,now))continue;
            refresh_npc_movement_locked(scope.actor,now);
            auto live=npc_combat_.find(scope.actor);
            if(live==npc_combat_.end()||live->second.hex_scope()!=scope)continue;
            const auto placed=npcs_.find(scope.actor);
            if(placed==npcs_.end())return SpiritProximity::unknown;
            if(placed->second.plane!=plane||std::hypot(double(placed->second.x)-origin->x,
                double(placed->second.y)-origin->z)>plan->earshot)continue;
            const auto clock=live->second.health_clock(live->second.lifetime());
            if(clock&&*clock>now)return SpiritProximity::unknown;
            // Avoid a second zero-time reaction flush at an active frontier.
            if(!clock||*clock<now)advance_npc_conditions_locked(scope.actor,live->second,now);
            live=npc_combat_.find(scope.actor);
            if(live==npc_combat_.end()||live->second.hex_scope()!=scope)continue;
            const auto life=live->second.lifetime();const auto actor=live->second.actor(life);
            // Untouched NPCs have no Health timeline: condition/hex/external
            // Health admission starts it. advance_conditions intentionally does
            // nothing in that state, so a missing clock is not failed settlement.
            const auto settled=live->second.health_clock(life);
            if(actor&&actor->health&&actor->health->current>0&&
               ((clock&&!settled)||(settled&&*settled!=now)))
                return SpiritProximity::unknown;
        }
        return spirit_light_proximity_locked(caster,now);
    }
public:
    skills::effects::SpiritProximity spirit_light_proximity(
            skills::effects::Scope caster,std::uint64_t now){
        std::lock_guard lock(mutex_);
        const auto admitted=players_.find(caster.actor);
        if(admitted==players_.end()||admitted->second.effect_scope!=caster)
            return skills::effects::SpiritProximity::unknown;
        if(!settle_timed_effects_locked(now))return skills::effects::SpiritProximity::unknown;
        return spirit_light_proximity_locked(caster,now);
    }
    // Completion-time count for Mend Body and Soul. A nearby actor with no
    // reviewed creature type makes the count unknown; treating it as zero
    // would silently under-cleanse the recipient.
private:
    std::optional<unsigned> mend_body_soul_spirit_count_locked(
            skills::effects::Scope target,std::uint64_t now){
        navigation::Point center{};
        unsigned plane=0;
        if(const auto player=players_.find(target.actor);player!=players_.end()){
            if(player->second.effect_scope!=target||!player->second.resources||
               player->second.precise_health<=0)return {};
            const auto point=healing_position_locked(player->second,now);
            if(!point)return {};
            center=*point;plane=player->second.surface.plane;
        }else{
            const auto placed=npcs_.find(target.actor);
            const auto live=npc_combat_.find(target.actor);
            if(placed==npcs_.end()||live==npc_combat_.end()||
               live->second.hex_scope()!=target)return {};
            const auto* actor=live->second.actor(live->second.lifetime());
            if(!actor||!actor->health||actor->health->current<=0)return {};
            center={placed->second.x,placed->second.y};plane=placed->second.plane;
        }
        const auto plan=skills::effects::mend_body_soul(0);
        if(!plan)return {};
        unsigned count=0;
        for(auto& [id,live]:npc_combat_){
            refresh_npc_movement_locked(id,now);
            const auto placed=npcs_.find(id);
            if(placed==npcs_.end())return {};
            if(placed->second.plane!=plane||
               std::hypot(double(placed->second.x)-center.x,
                          double(placed->second.y)-center.z)>plan->earshot)continue;
            const auto life=live.lifetime();
            const auto* actor=live.actor(life);
            if(!actor||!actor->health||actor->health->current<=0)continue;
            const auto type=live.creature_type(life);
            if(!type)return {};
            if(*type=="spirit")++count;
        }
        return count;
    }
public:
    std::optional<unsigned> mend_body_soul_spirit_count(
            skills::effects::Scope target,std::uint64_t now){
        std::lock_guard lock(mutex_);
        if(!settle_timed_effects_locked(now))return {};
        return mend_body_soul_spirit_count_locked(target,now);
    }
    bool valid_mend_body_soul_target(skills::effects::Scope source,
            skills::effects::Scope target,std::uint64_t now)const{
        std::lock_guard lock(mutex_);
        const auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.resources||caster->second.precise_health<=0)return false;
        const auto origin=healing_position_locked(caster->second,now);
        if(!origin)return false;
        const auto player=players_.find(target.actor);
        if(player!=players_.end()){
            if(player->second.effect_scope!=target||
               !live_heal_target_locked(caster->second,player->second,1248.f,true))return false;
            const auto destination=healing_position_locked(player->second,now);
            return destination&&std::hypot(origin->x-destination->x,
                origin->z-destination->z)<=1248.f;
        }
        const auto npc=npcs_.find(target.actor);
        const auto live=npc_combat_.find(target.actor);
        if(npc==npcs_.end()||live==npc_combat_.end()||
           live->second.hex_scope()!=target||
           !live->second.healable_ally(live->second.lifetime())||
           live->second.healing_allowed(live->second.lifetime())!=std::optional(true)||
           npc->second.plane!=caster->second.surface.plane||
           std::hypot(origin->x-npc->second.x,origin->z-npc->second.y)>1248.f)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,
                *origin,{npc->second.x,npc->second.y});
            if(!line||line->plane!=npc->second.plane)return false;}
        return true;
    }
    // A trusted completion path. Packet publication is deferred behind the
    // cast-finish prefix; condition selection remains a provisional policy.
    bool stage_mend_body_soul_player(
            skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::MendBodySoulPlan& plan,std::uint64_t now,
            std::function<void()>* publication=nullptr,
            std::optional<std::uint64_t> completion_event={}){
        if(publication)*publication={};
        if(skills::effects::mend_body_soul(plan.rank)!=plan)return false;
        std::lock_guard lock(mutex_);
        const auto caster=players_.find(source.actor),ally=players_.find(target.actor);
        if(caster==players_.end()||ally==players_.end()||
           caster->second.effect_scope!=source||ally->second.effect_scope!=target||
           !caster->second.resources||!resurrection_clock_valid(caster->second,now)||
           !live_heal_target_locked(caster->second,ally->second,1248.f,true))return false;
        if(!settle_timed_effects_locked(now))return false;
        const auto origin=healing_position_locked(caster->second,now);
        const auto destination=healing_position_locked(ally->second,now);
        if(!origin||!destination||caster->second.surface.plane!=ally->second.surface.plane||
           std::hypot(origin->x-destination->x,origin->z-destination->z)>1248.f||
           caster->second.knocked_down_until>now)return false;
        const auto spirits=mend_body_soul_spirit_count_locked(target,now);
        if(!spirits)return false;
        if(!advance_resource_event_locked(caster->second,now)||
           !advance_health_locked(ally->second,now)||
           caster->second.precise_health<=0||ally->second.precise_health<=0)return false;
        const auto quote=skills::effects::quote_mend_body_soul(
            plan,spirits,ally->second.conditions.count(now));
        if(!quote)return false;
        std::vector<combat::conditions::Kind> removals;
        auto proposed=ally->second.conditions;
        for(unsigned i=0;i<quote->conditions_to_remove;++i){
            const auto kind=proposed.newest(now);
            if(!kind||!proposed.remove(*kind))return false;
            removals.push_back(*kind);
        }
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        if(completion_event&&!complete_ether_renewal_locked(source,1234,*completion_event,now,true))return false;
        auto& recipient=ally->second;
        const auto before=recipient.precise_health;
        health_event_locked(recipient,quote->heal,now,true);
        publish_health_locked(recipient);
        const auto gained=recipient.precise_health-before;
        if(gained>0)for(auto& [id,peer]:players_)peer.outbox->enqueue({
            skills::particles::health_change_2022(target.actor,source.actor,gained,
                recipient.resources->max_health)});
        for(const auto kind:removals){
            if(recipient.precise_health<=0)break;
            if(recipient.conditions.has(kind,now))remove_condition_locked(recipient,kind,now);
        }
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));
        return true;
    }
    bool stage_mend_body_soul_npc(
            skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::MendBodySoulPlan& plan,std::uint64_t now,
            std::function<void()>* publication=nullptr,
            std::optional<std::uint64_t> completion_event={}){
        if(publication)*publication={};
        if(skills::effects::mend_body_soul(plan.rank)!=plan||source==target)return false;
        std::lock_guard lock(mutex_);
        const auto caster=players_.find(source.actor);
        auto live=npc_combat_.find(target.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.resources||!resurrection_clock_valid(caster->second,now)||
           live==npc_combat_.end()||live->second.hex_scope()!=target)return false;
        const auto life=live->second.lifetime();
        if(!live->second.healable_ally(life)||
           live->second.healing_allowed(life)!=std::optional(true)||
           now<live->second.health_clock(life).value_or(now))return false;
        if(!settle_timed_effects_locked(now))return false;
        refresh_npc_movement_locked(target.actor,now);
        live=npc_combat_.find(target.actor);
        const auto placed=npcs_.find(target.actor);
        if(live==npc_combat_.end()||placed==npcs_.end()||
           !live->second.matches(life)||!live->second.healable_ally(life)||
           live->second.healing_allowed(life)!=std::optional(true)||
           placed->second.plane!=caster->second.surface.plane||
           caster->second.knocked_down_until>now)return false;
        const auto origin=healing_position_locked(caster->second,now);
        if(!origin||std::hypot(origin->x-placed->second.x,
                              origin->z-placed->second.y)>1248.f)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,
                *origin,{placed->second.x,placed->second.y});
            if(!line||line->plane!=placed->second.plane)return false;}
        const auto spirits=mend_body_soul_spirit_count_locked(target,now);
        if(!spirits)return false;
        if(!advance_resource_event_locked(caster->second,now)||
           caster->second.precise_health<=0)return false;
        advance_npc_conditions_locked(target.actor,live->second,now);
        live=npc_combat_.find(target.actor);
        if(live==npc_combat_.end()||!live->second.matches(life)||
           !live->second.healable_ally(life)||
           live->second.healing_allowed(life)!=std::optional(true))return false;
        const auto conditions=live->second.condition_snapshot(life);
        if(!conditions)return false;
        const auto quote=skills::effects::quote_mend_body_soul(
            plan,spirits,conditions->count(now));
        if(!quote)return false;
        auto proposed=live->second;
        const auto before=proposed.actor(life)->health->current;
        if(!proposed.heal_ally(life,quote->heal,now))return false;
        const auto healed=proposed.actor(life)->health->current-before;
        for(unsigned i=0;i<quote->conditions_to_remove;++i){
            if(proposed.actor(life)->health->current<=0)break;
            const auto current=proposed.condition_snapshot(life);
            if(!current)return false;
            const auto kind=current->newest(now);
            if(!kind||!proposed.remove_condition(life,*kind,now))return false;
        }
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        if(completion_event&&!complete_ether_renewal_locked(source,1234,*completion_event,now,true))return false;
        live->second=std::move(proposed);
        auto& committed=live->second;
        if(healed>0)for(auto& [id,peer]:players_)peer.outbox->enqueue({
            skills::particles::health_change_2022(target.actor,source.actor,
                healed,committed.actor(life)->health->maximum)});
        publish_npc_fragility_locked(target.actor,committed);
        for(auto& [id,peer]:players_)peer.outbox->enqueue({
            combat::detail::fraction(target.actor,*committed.actor(life)->health),
            emotes::effects_2022(target.actor,committed.actor(life)->effects)});
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));
        return true;
    }
    // The World owns the completion-time target, spirit and Health decision.
    // Normal casting must first pay Energy and supply this trusted actor scope;
    // this stage deliberately does not expose a decoded-network entrypoint.
    std::optional<skills::sacrifice::Loss> stage_spirit_light_player(
            skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::SpiritLightPlan& plan,std::uint64_t now,
            std::function<void()>* publication=nullptr,
            std::optional<std::uint64_t> completion_event={}){
        if(publication)*publication={};
        if(skills::effects::spirit_light(plan.rank)!=plan||source==target)return {};
        std::lock_guard lock(mutex_);
        const auto caster=players_.find(source.actor),ally=players_.find(target.actor);
        if(caster==players_.end()||ally==players_.end()||
           caster->second.effect_scope!=source||ally->second.effect_scope!=target||
           !caster->second.resources||!resurrection_clock_valid(caster->second,now)||
           !live_heal_target_locked(caster->second,ally->second,1248.f,false))return {};
        if(!settle_timed_effects_locked(now))return {};
        const auto origin=healing_position_locked(caster->second,now);
        const auto destination=healing_position_locked(ally->second,now);
        if(!origin||!destination||caster->second.surface.plane!=ally->second.surface.plane||
           std::hypot(origin->x-destination->x,origin->z-destination->z)>1248.f||
           caster->second.knocked_down_until>now)return {};
        if(!advance_resource_event_locked(caster->second,now)||
           !advance_health_locked(ally->second,now)||
           caster->second.precise_health<=0||ally->second.precise_health<=0)return {};
        const auto proximity=spirit_light_proximity_locked(source,now);
        auto quote=skills::effects::quote_spirit_light(plan,proximity,
            caster->second.precise_health,caster->second.resources->max_health);
        if(!quote)return {};
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        if(completion_event&&!complete_ether_renewal_locked(source,915,*completion_event,now,true))return {};
        quote=skills::effects::quote_spirit_light(plan,proximity,
            caster->second.precise_health,caster->second.resources->max_health);
        if(!quote)return {};
        const auto before=ally->second.precise_health;
        health_event_locked(ally->second,quote->raw_heal,now,true);
        publish_health_locked(ally->second);
        const auto gained=ally->second.precise_health-before;
        if(gained>0)for(auto& [id,peer]:players_)peer.outbox->enqueue({
            skills::particles::health_change_2022(target.actor,source.actor,gained,
                ally->second.resources->max_health)});
        auto loss=quote->sacrifice.value_or(skills::sacrifice::Loss{
            0,0,caster->second.precise_health,false});
        if(quote->sacrifice){
            health_event_locked(caster->second,
                loss.remaining-caster->second.precise_health,now);
            loss.remaining=caster->second.precise_health;
            loss.killed=loss.remaining<=0;
            if(caster->second.passive_recovery&&loss.lost>0)
                caster->second.passive_recovery->record(recovery::Event::health_loss,now);
            if(loss.killed)kill_locked(caster->second,now);
            else{publish_health_locked(caster->second);
                 caster->second.outbox->enqueue(owner_vitals(caster->second));}
        }
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));
        return loss;
    }
    std::optional<skills::sacrifice::Loss> stage_spirit_light_npc(
            skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::SpiritLightPlan& plan,std::uint64_t now,
            std::function<void()>* publication=nullptr,
            std::optional<std::uint64_t> completion_event={}){
        if(publication)*publication={};
        if(skills::effects::spirit_light(plan.rank)!=plan||source==target)return {};
        std::lock_guard lock(mutex_);
        const auto caster=players_.find(source.actor);
        const auto initial=npc_combat_.find(target.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.resources||!resurrection_clock_valid(caster->second,now)||
           initial==npc_combat_.end()||initial->second.hex_scope()!=target)return {};
        const auto life=initial->second.lifetime();
        if(!initial->second.healable_ally(life)||
           initial->second.healing_allowed(life)!=std::optional(true)||
           now<initial->second.health_clock(life).value_or(now))return {};
        if(!settle_timed_effects_locked(now))return {};
        refresh_npc_movement_locked(target.actor,now);
        auto body=npc_combat_.find(target.actor);
        auto placed=npcs_.find(target.actor);
        if(body==npc_combat_.end()||placed==npcs_.end()||
           !body->second.matches(life)||
           placed->second.plane!=caster->second.surface.plane||
           caster->second.knocked_down_until>now)return {};
        const auto origin=healing_position_locked(caster->second,now);
        if(!origin||std::hypot(origin->x-placed->second.x,
                origin->z-placed->second.y)>1248.f)return {};
        if(navigation_){const auto line=layered_->trace(caster->second.surface,
                *origin,{placed->second.x,placed->second.y});
            if(!line||line->plane!=placed->second.plane)return {};}
        if(!advance_resource_event_locked(caster->second,now)||
           caster->second.precise_health<=0)return {};
        advance_npc_conditions_locked(target.actor,body->second,now);
        body=npc_combat_.find(target.actor);
        if(body==npc_combat_.end()||!body->second.matches(life)||
           !body->second.healable_ally(life)||
           body->second.healing_allowed(life)!=std::optional(true))return {};
        const auto proximity=spirit_light_proximity_locked(source,now);
        auto quote=skills::effects::quote_spirit_light(plan,proximity,
            caster->second.precise_health,caster->second.resources->max_health);
        if(!quote)return {};
        auto proposed=body->second;
        const auto before=proposed.actor(life)->health->current;
        if(!proposed.heal_ally(life,quote->raw_heal,now))return {};
        const auto health=*proposed.actor(life)->health;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        if(completion_event&&!complete_ether_renewal_locked(source,915,*completion_event,now,true))return {};
        quote=skills::effects::quote_spirit_light(plan,proximity,
            caster->second.precise_health,caster->second.resources->max_health);
        if(!quote)return {};
        body->second=std::move(proposed);
        for(auto& [id,peer]:players_){
            std::vector<Packet> packets{combat::detail::fraction(target.actor,health)};
            if(health.current>before)packets.push_back(
                skills::particles::health_change_2022(target.actor,source.actor,
                    health.current-before,health.maximum));
            peer.outbox->enqueue(packets);
        }
        auto loss=quote->sacrifice.value_or(skills::sacrifice::Loss{
            0,0,caster->second.precise_health,false});
        if(quote->sacrifice){
            health_event_locked(caster->second,
                loss.remaining-caster->second.precise_health,now);
            loss.remaining=caster->second.precise_health;
            loss.killed=loss.remaining<=0;
            if(caster->second.passive_recovery&&loss.lost>0)
                caster->second.passive_recovery->record(recovery::Event::health_loss,now);
            if(loss.killed)kill_locked(caster->second,now);
            else{publish_health_locked(caster->second);
                 caster->second.outbox->enqueue(owner_vitals(caster->second));}
        }
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));
        return loss;
    }
