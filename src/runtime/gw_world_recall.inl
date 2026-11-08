private:
    std::map<std::pair<skills::effects::Scope,unsigned>,skills::effects::RecallAnchor> recall_anchors_;
    skills::effects::RecallEndQueue recall_ends_;
    std::set<skills::effects::Scope> recall_retiring_sources_;
    bool dispatching_recall_ends_=false;
    struct RecallPublication {skills::effects::RecallFeedbackNotice notice;std::vector<Packet> prefix,suffix;std::uint64_t reservation=0;};
    std::map<skills::effects::Scope,std::deque<RecallPublication>> recall_publications_;
    void publish_recall_removal_locked(const skills::effects::RecallAnchor& anchor){
        const skills::effects::Entry entry{anchor.effect,anchor.caster.actor,925,0,
            skills::effects::Category::enchantment,anchor.applied,{}};
        const auto wire=skills::effects::recall_end_wire(anchor,entry);
        if(!wire)throw std::logic_error("Recall removal lost original ownership");
        const auto owner=players_.find(anchor.caster.actor);
        if(owner==players_.end()||owner->second.effect_scope!=anchor.caster)return;
        owner->second.outbox->enqueue({wire->upkeep});
        for(auto&[id,peer]:players_)peer.outbox->enqueue({wire->effect});
    }
    void end_recall_monitor_locked(Player& player,const skills::effects::Entry& entry,std::uint64_t now){
        const auto found=recall_anchors_.find({player.effect_scope,entry.instance});
        if(found==recall_anchors_.end())throw std::logic_error("Recall end missing original ally anchor");
        const auto wire=skills::effects::recall_end_wire(found->second,entry);
        if(!wire||!recall_ends_.enqueue(found->second,now)||!mending_ownership_.ended(player.effect_scope,entry.instance,now))
            throw std::logic_error("Recall end ownership/chronology mismatch");
        recall_anchors_.erase(found);sync_mending_curve_locked(player.effect_scope,now);
        // Removal wire belongs to the dispatcher,after successful return trails
        // or the confirmed no-return outcome. Registry retirement stays here.
    }
    bool dispatch_recall_ends_locked(std::uint64_t horizon){
        if(dispatching_recall_ends_)return true;
        struct Guard {bool& flag;Guard(bool& f):flag(f){flag=true;}~Guard(){flag=false;}} guard(dispatching_recall_ends_);
        while(const auto pending=recall_ends_.front()){
            if(pending->at>horizon)break;
            const auto source=players_.find(pending->anchor.caster.actor);
            if(source==players_.end()||source->second.effect_scope!=pending->anchor.caster||source->second.precise_health<=0||
               recall_retiring_sources_.contains(pending->anchor.caster)){
                publish_recall_removal_locked(pending->anchor);
                if(!recall_ends_.acknowledge(pending->event))return false;continue;
            }
            auto& player=source->second;
            // Dead/retired targets end upkeep without return or new disables.
            const auto target_alive=[&]{
                if(const auto target=players_.find(pending->anchor.target.actor);target!=players_.end())
                    return target->second.effect_scope==pending->anchor.target&&target->second.precise_health>0;
                if(const auto target=npc_combat_.find(pending->anchor.target.actor);target!=npc_combat_.end())
                    return target->second.hex_scope()==pending->anchor.target&&target->second.healable_ally(target->second.lifetime());
                return false;
            };
            if(!target_alive()){
                publish_recall_removal_locked(pending->anchor);
                if(!recall_ends_.acknowledge(pending->event))return false;continue;
            }
            if(!player.cooldown_authority)return false;
            const auto bar=player.cooldown_authority->snapshot(player.effect_scope);if(!bar)return false;
            auto receipt=prepare_recall_step_locked(pending->anchor,pending->at,{});
            if(!receipt){
                // Preparation settles linked health events. A death discovered
                // there is a completed no-return end,not a navigation retry.
                const auto current=players_.find(pending->anchor.caster.actor);
                if(current==players_.end()||current->second.effect_scope!=pending->anchor.caster||
                   current->second.precise_health<=0||recall_retiring_sources_.contains(pending->anchor.caster)||!target_alive()){
                    publish_recall_removal_locked(pending->anchor);
                    if(!recall_ends_.acknowledge(pending->event))return false;continue;
                }
                return false;
            }
            const auto old=recall_disables_.find(player.effect_scope);
            const auto disables=(old==recall_disables_.end()?skills::effects::RecallDisables(player.effect_scope):old->second)
                .propose(player.effect_scope,*bar,pending->event,pending->at);
            if(!disables)return false;
            auto visuals=skills::particles::shadow_step_trail_2022(player.id,receipt->origin.point.x,receipt->origin.point.z,
                static_cast<std::uint16_t>(receipt->origin.surface.plane),receipt->destination.point.x,receipt->destination.point.z,
                static_cast<std::uint16_t>(receipt->destination.surface.plane));
            auto& publications=recall_publications_[player.effect_scope];if(publications.size()>=128)return false;
            const skills::effects::Entry entry{pending->anchor.effect,player.id,925,0,
                skills::effects::Category::enchantment,pending->anchor.applied,{}};
            const auto wire=skills::effects::recall_end_wire(pending->anchor,entry);if(!wire)return false;
            if(!receipt->bundle_drop)return false;
            auto owner_prefix=visuals;
            if(receipt->bundle_drop->transition){
                owner_prefix={receipt->bundle_drop->owner_clear};
                owner_prefix.insert(owner_prefix.end(),receipt->bundle_drop->shared.begin(),receipt->bundle_drop->shared.end());
                owner_prefix.insert(owner_prefix.end(),visuals.begin(),visuals.end());
                auto peer_prefix=receipt->bundle_drop->shared;peer_prefix.insert(peer_prefix.end(),visuals.begin(),visuals.end());
                visuals=std::move(peer_prefix);
            }
            RecallPublication publication{{player.effect_scope,pending->event,pending->at,bar->bar},std::move(owner_prefix),
                {wire->upkeep,wire->effect,receipt->position}};
            const auto reserved=player.outbox->reserve_publication();if(!reserved)return false;publication.reservation=*reserved;
            if(!commit_bundle_drop_locked(*receipt->bundle_drop,false)){
                player.outbox->resolve_publication(*reserved,{});return false;
            }
            if(!receipt->bundle_drop->owner_energy.empty()){
                const auto offset=1+receipt->bundle_drop->shared.size();
                publication.prefix.insert(publication.prefix.begin()+offset,receipt->bundle_drop->owner_energy.begin(),receipt->bundle_drop->owner_energy.end());
            }
            recall_disables_.insert_or_assign(player.effect_scope,*disables);
            apply_shadow_position_locked(player,receipt->destination,pending->at);
            publications.push_back(std::move(publication));
            visuals.push_back(wire->effect);visuals.push_back(receipt->position);
            for(auto&[id,peer]:players_)if(peer.effect_scope!=player.effect_scope)peer.outbox->enqueue(visuals);
            if(!recall_ends_.acknowledge(pending->event))throw std::logic_error("Recall end event lost");
        }
        return true;
    }
public:
    std::optional<bool> recall_feedback_pending(skills::effects::Scope owner,skills::Slot slot,std::uint64_t now)const{
        std::lock_guard lock(mutex_);const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner||!skills::effects::cooldown_slot(slot))return {};
        if(const auto pending=recall_publications_.find(owner);pending!=recall_publications_.end())
            for(const auto& publication:pending->second){
                if(publication.notice.at>now)return {};
                if(std::find(publication.notice.bar.begin(),publication.notice.bar.end(),slot)!=publication.notice.bar.end())return true;
            }
        return false;
    }
    std::optional<std::vector<skills::effects::RecallFeedbackNotice>> pending_recall_feedback(skills::effects::Scope owner)const{
        std::lock_guard lock(mutex_);const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner)return {};
        std::vector<skills::effects::RecallFeedbackNotice> result;
        if(const auto found=recall_publications_.find(owner);found!=recall_publications_.end())
            for(const auto& publication:found->second)result.push_back(publication.notice);
        return result;
    }
    // Session-only trusted reply. No wire request exposes this transaction.
    bool publish_recall_feedback(const skills::effects::RecallFeedbackReply& reply){
        std::lock_guard lock(mutex_);const auto owner=reply.notice.owner;
        const auto player=players_.find(owner.actor);const auto pending=recall_publications_.find(owner);
        if(player==players_.end()||player->second.effect_scope!=owner||pending==recall_publications_.end()||pending->second.empty()||
           pending->second.front().notice!=reply.notice||reply.at<reply.notice.at||player->second.health_updated>reply.at||
           !player->second.cooldown_authority)return false;
        const auto current=player->second.cooldown_authority->snapshot(owner);
        if(!current||current->bar!=reply.current||current->published.at>reply.at)return false;
        std::size_t index=0,observations=0;
        for(const auto slot:reply.notice.bar)if(slot.skill&&std::find(reply.current.begin(),reply.current.end(),slot)!=reply.current.end()){
            if(observations>=reply.slots.size())return false;const auto& observation=reply.slots[observations++];
            if(observation.slot!=slot)return false;
            const auto until=skill_disabled_slot_locked(owner,slot,reply.at);if(!until)return false;
            if(observation.disabled_until<*until)return false;
            const auto before=std::max(observation.ordinary_until,observation.cached_until);
            const auto after=std::max(observation.ordinary_until,observation.disabled_until);
            if(after<=std::max(before,reply.at))continue;
            const auto remaining=after-reply.at;
            const auto seconds=static_cast<unsigned>(std::min<std::uint64_t>(UINT32_MAX,remaining/1000+(remaining%1000!=0)));
            if(index>=reply.packets.size()||reply.packets[index++]!=skills::effects::recall_feedback_packet(owner.actor,slot,seconds))return false;
        }
        if(index!=reply.packets.size()||observations!=reply.slots.size())return false;
        const auto& publication=pending->second.front();auto packets=publication.prefix;
        packets.insert(packets.end(),reply.packets.begin(),reply.packets.end());
        packets.insert(packets.end(),publication.suffix.begin(),publication.suffix.end());
        if(!player->second.outbox->resolve_publication(publication.reservation,std::move(packets)))return false;
        pending->second.pop_front();
        if(pending->second.empty())recall_publications_.erase(pending);return true;
    }
private:
    void retire_recall_publications_locked(skills::effects::Scope owner){
        const auto pending=recall_publications_.find(owner);if(pending==recall_publications_.end())return;
        const auto player=players_.find(owner.actor);
        if(player!=players_.end()&&player->second.effect_scope==owner)
            for(const auto& publication:pending->second)player->second.outbox->resolve_publication(publication.reservation,{});
        recall_publications_.erase(pending);
    }
public:
    bool recall_target_valid(skills::effects::Scope source,skills::effects::Scope target,std::uint64_t now)const{
        std::lock_guard lock(mutex_);return recall_target_valid_locked(source,target,now);
    }
    // Trusted staged producer. No Runtime/gateway request exposes925 yet.
    bool stage_recall(skills::effects::Scope source,skills::effects::Scope target,std::uint64_t now){
        std::lock_guard lock(mutex_);return apply_recall_locked(source,target,now,false);
    }
    bool complete_recall(skills::effects::Scope source,skills::effects::Scope target,std::uint64_t now){
        std::lock_guard lock(mutex_);return apply_recall_locked(source,target,now,true);
    }
private:
    bool recall_target_valid_locked(skills::effects::Scope source,skills::effects::Scope target,std::uint64_t now)const{
        const auto plan=skills::effects::recall_shadow();const auto owner=players_.find(source.actor);
        if(!plan||!navigation_||!layered_||source==target||source.instance!=target.instance||owner==players_.end()||
           owner->second.effect_scope!=source||!owner->second.resources||owner->second.precise_health<=0||
           !resurrection_clock_valid(owner->second,now)||now<owner->second.energy_updated||
           owner->second.knocked_down_until>now||!owner->second.cooldown_authority||!owner->second.cooldown_authority->snapshot(source)||
           !owner->second.effect_registry||owner->second.effect_registry->newest_skill(source,925,now))return false;
        const auto origin=healing_position_locked(owner->second,now);if(!origin)return false;
        std::optional<navigation::Point> destination;
        if(const auto ally=players_.find(target.actor);ally!=players_.end()){
            const auto& player=ally->second;
            if(player.effect_scope!=target||!player.resources||player.precise_health<=0||!owner->second.team_token||
               player.team_token!=owner->second.team_token||!resurrection_clock_valid(player,now))return false;
            destination=healing_position_locked(player,now);
        }else{
            const auto live=npc_combat_.find(target.actor);
            if(live==npc_combat_.end()||live->second.hex_scope()!=target||!live->second.healable_ally(live->second.lifetime()))return false;
            const auto path=maintained_npc_path_locked(target,now,now);if(!path||path->empty())return false;
            destination=navigation::Point{static_cast<float>(path->front().x),static_cast<float>(path->front().y)};
        }
        return destination&&std::hypot(destination->x-origin->x,destination->z-origin->z)<=plan->range;
    }
    bool apply_recall_locked(skills::effects::Scope source,skills::effects::Scope target,std::uint64_t now,bool completion){
        const auto plan=skills::effects::recall_shadow();if(!plan||source.actor==target.actor)return false;
        if(!recall_target_valid_locked(source,target,now))return false;
        const auto owner=players_.find(source.actor);
        if(!navigation_||!layered_||owner==players_.end()||owner->second.effect_scope!=source||!owner->second.resources||
           !resurrection_clock_valid(owner->second,now)||owner->second.knocked_down_until>now)return false;
        auto& caster=owner->second;
        if(!caster.cooldown_authority||!caster.cooldown_authority->snapshot(source)||
           !advance_resource_event_locked(caster,now)||!caster.effect_registry||!caster.health_regeneration)return false;
        if(caster.precise_health<=0)return false;
        const auto origin=healing_position_locked(caster,now);if(!origin)return false;
        std::optional<navigation::Point> destination;
        if(const auto ally=players_.find(target.actor);ally!=players_.end()){
            auto& player=ally->second;
            if(player.effect_scope!=target||!player.resources||!caster.team_token||player.team_token!=caster.team_token||
               !resurrection_clock_valid(player,now)||!advance_health_locked(player,now)||player.precise_health<=0)return false;
            destination=healing_position_locked(player,now);
        }else{
            const auto body=npcs_.find(target.actor);const auto live=npc_combat_.find(target.actor);
            if(body==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=target||
               live->second.health_clock(live->second.lifetime()).value_or(0)>now)return false;
            refresh_npc_movement_locked(target.actor,now);advance_npc_conditions_locked(target.actor,live->second,now);
            if(!live->second.healable_ally(live->second.lifetime())||
               !live->second.begin_external_health(live->second.lifetime(),now))return false;
            destination=navigation::Point{body->second.x,body->second.y};
        }
        if(!destination||caster.precise_health<=0||std::hypot(destination->x-origin->x,destination->z-origin->z)>plan->range)return false;
        const auto allocated=std::max(next_mending_effect_,caster.next_health_effect);if(allocated>UINT32_MAX)return false;
        const auto proposal=skills::effects::prepare_recall_application(*plan,source,target,static_cast<unsigned>(allocated),now,
            true,true,true,0,*caster.effect_registry,mending_ownership_);if(!proposal)return false;
        const auto wire=skills::effects::recall_wire(proposal->anchor,proposal->entry);if(!wire)return false;
        auto regeneration=*caster.health_regeneration;
        if(!regeneration.apply(source,{proposal->entry.instance,source.actor,925,0,now,{}},true))return false;
        const auto visual=skills::particles::targeted_body_2022(target.actor,source.actor,925);if(completion&&!visual)return false;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        if(completion){
            std::set<const Outbox*> seen;for(auto&[id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
                recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
            for(auto&[box,queue]:recipients)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
            // Queue the prefix before a zero-Energy removal can reserve its
            // own publication inside this capture and materialize earlier work.
            for(auto&[id,peer]:players_)peer.outbox->enqueue({combat::detail::integer(58,source.actor,0),*visual});
        }
        if(!caster.overcast_curve)caster.overcast_curve.emplace(source,now,33);
        *caster.effect_registry=proposal->registry;*caster.health_regeneration=std::move(regeneration);mending_ownership_=proposal->ownership;
        caster.health_effects.emplace(proposal->entry.instance,proposal->entry);
        recall_anchors_.emplace(std::pair{source,proposal->entry.instance},proposal->anchor);
        recall_retiring_sources_.erase(source);
        caster.next_health_effect=next_mending_effect_=allocated+1;sync_mending_curve_locked(source,now);
        caster.outbox->enqueue({wire->upkeep});for(auto&[id,peer]:players_)peer.outbox->enqueue({wire->effect});
        commit_applications_locked(source,now);publish_status_locked(caster,now);
        if(completion&&!settle_timed_effects_locked(now))throw std::logic_error("Recall application crossed resource boundary");
        captures.clear();for(auto&[box,queue]:recipients)box->enqueue(std::move(queue.packets));return true;
    }
public:



