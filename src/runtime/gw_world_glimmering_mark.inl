// Included inside Instance. Owned application, pulse combat and admitted
// completion used by the shared Runtime/gateway adapter.
public:
    struct GlimmeringMarkOwner {
        skills::effects::Scope caster,target;unsigned effect=0,level=0,team=0;
        skills::effects::GlimmeringMarkPlan plan;std::uint64_t applied=0;
    };
private:
    skills::effects::GlimmeringMarkSchedule glimmering_marks_;
    std::map<skills::effects::Scope,GlimmeringMarkOwner> glimmering_mark_owners_;
    std::map<skills::effects::Scope,std::pair<std::uint64_t,std::uint64_t>> glimmering_use_events_;
public:
    // Server completion observation, never a decoded client request. The
    // Runtime must retain both scopes and invoke only at the valid completion
    // frontier. Own event high-water prevents replay ending a newer mark.
    bool complete_glimmering_targeted_use(const skills::effects::GlimmeringTargetedUse& use){
        if(!use.event||!skills::effects::glimmering_direct_target_skill(use.skill)||
           !use.caster.instance||use.caster.instance!=use.target.instance||!use.caster.generation||
           !use.target.generation||!use.caster.actor||!use.target.actor||use.caster.actor>65535||use.target.actor>65535)return false;
        std::lock_guard lock(mutex_);const auto source=players_.find(use.caster.actor);
        if(source==players_.end()||source->second.effect_scope!=use.caster||!source->second.resources||
           source->second.precise_health<=0)return false;
        if(const auto old=glimmering_use_events_.find(use.caster);old!=glimmering_use_events_.end()&&
           (use.event<=old->second.first||use.at<old->second.second))return false;
        if(const auto p=players_.find(use.target.actor);p!=players_.end()){
            if(p->second.effect_scope!=use.target)return false;
        }else{const auto npc=npc_combat_.find(use.target.actor);
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=use.target)return false;}
        const auto accepted=[&]{glimmering_use_events_[use.caster]={use.event,use.at};return true;};
        // Observing an unrelated completed use must not settle or rewind
        // resources. A concurrent World tick can already be ahead of its
        // timestamp. Still consume the event so replay cannot end a new Mark.
        const auto before=glimmering_mark_owners_.find(use.target);
        if(before==glimmering_mark_owners_.end()||before->second.caster!=use.caster)return accepted();
        if(use.at<source->second.health_updated||!advance_health_locked(source->second,use.at)||source->second.precise_health<=0)return false;
        const auto found=glimmering_mark_owners_.find(use.target);
        if(found==glimmering_mark_owners_.end()||found->second.caster!=use.caster)return accepted();
        const auto owned=found->second;
        if(const auto p=players_.find(use.target.actor);p!=players_.end()){
            if(p->second.effect_scope!=use.target)return false;
            if(!advance_health_locked(p->second,use.at))return false;
            const auto current=p->second.effect_registry?p->second.effect_registry->lookup(use.target,owned.effect):std::nullopt;
            if(current&&current->skill==227&&current->cause==use.caster.actor)
                remove_health_effect_locked(p->second,owned.effect,use.at);
        }else{
            auto npc=npc_combat_.find(use.target.actor);
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=use.target)return false;
            if(npc->second.health_clock(npc->second.lifetime()).value_or(use.at)>use.at)return false;
            advance_npc_conditions_locked(use.target.actor,npc->second,use.at);
            npc=npc_combat_.find(use.target.actor);
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=use.target)return false;
            if(const auto entry=npc->second.hex_effect(npc->second.lifetime(),owned.effect);entry&&entry->skill==227){
                auto proposed=npc->second;
                if(!proposed.remove_hex_effect(proposed.lifetime(),owned.effect,use.at))return false;
                npc->second=std::move(proposed);
                for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(use.target.actor,
                    npc->second.actor(npc->second.lifetime())->effects)});
            }
        }
        end_glimmering_mark_locked(use.target,owned.effect,use.at);return accepted();
    }
    std::optional<GlimmeringMarkOwner> glimmering_mark_snapshot(skills::effects::Scope target,std::uint64_t now)const{
        std::lock_guard lock(mutex_);const auto owner=glimmering_mark_owners_.find(target);
        if(owner==glimmering_mark_owners_.end()||now<owner->second.applied||
           now-owner->second.applied>=owner->second.plan.duration)return {};
        std::optional<skills::effects::Entry> entry;
        if(const auto p=players_.find(target.actor);p!=players_.end()){
            if(p->second.effect_scope!=target||!p->second.effect_registry||p->second.precise_health<=0)return {};
            entry=p->second.effect_registry->lookup(target,owner->second.effect);
        }else{const auto n=npc_combat_.find(target.actor);
            if(n==npc_combat_.end()||n->second.hex_scope()!=target)return {};
            const auto actor=n->second.actor(n->second.lifetime());if(!actor||!actor->health||actor->health->current<=0)return {};
            entry=n->second.hex_effect(n->second.lifetime(),owner->second.effect);
        }
        const auto& owned=owner->second;
        if(!entry||entry->skill!=227||entry->cause!=owned.caster.actor||entry->rank!=owned.plan.rank||
           entry->applied!=owned.applied||entry->expires!=owned.applied+owned.plan.duration)return {};
        return owned;
    }
    bool complete_glimmering_mark(skills::effects::Scope caster,skills::effects::Scope target,
            const skills::effects::GlimmeringMarkPlan& plan,unsigned level,std::uint64_t event,
            std::uint64_t now,std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        if(!event)return false;
        return stage_glimmering_mark(caster,target,plan,level,now,event,publication);
    }
    bool stage_glimmering_mark(skills::effects::Scope caster,skills::effects::Scope target,
            const skills::effects::GlimmeringMarkPlan& plan,unsigned level,std::uint64_t now,
            std::optional<std::uint64_t> completion_event={},std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        const auto canonical=skills::effects::glimmering_mark(plan.rank);
        if(!canonical||*canonical!=plan||!level||level>255||now>UINT64_MAX-plan.duration-plan.blind_ms)return false;
        std::lock_guard lock(mutex_);
        if(completion_event&&!complete_ether_renewal_locked(caster,227,*completion_event,now,true,false))return false;
        DamagePublication pending;
        const auto perform=[&]()->bool{
        auto source=players_.find(caster.actor);
        unsigned effect=0;bool prevented=false;
        std::optional<combat::npcs::LiveActor> proposed_npc;
        std::optional<skills::effects::Registry> registry;
        std::optional<skills::regeneration::State> regeneration;
        skills::effects::Applied applied;
        auto player=players_.end();
        const auto prepare=[&]()->bool{
        source=players_.find(caster.actor);effect=0;prevented=false;proposed_npc.reset();registry.reset();regeneration.reset();
        if(source==players_.end()||source->second.effect_scope!=caster||!source->second.resources||
           !source->second.team_token||!source->second.health_regeneration||source->second.precise_health<=0||
           source->second.knocked_down_until>now||now<source->second.health_updated||
           !advance_health_locked(source->second,now)||source->second.precise_health<=0)return false;
        auto origin=healing_position_locked(source->second,now);if(!origin)return false;
        player=players_.find(target.actor);
        if(player!=players_.end()){
            auto& p=player->second;
            if(p.effect_scope!=target||!p.resources||!p.effect_registry||!p.health_regeneration||!p.team_token||
               p.team_token==source->second.team_token||p.precise_health<=0||now<p.health_updated||
               p.next_health_effect>UINT32_MAX||p.surface.plane!=source->second.surface.plane||
               !advance_health_locked(p,now)||p.precise_health<=0)return false;
            if(source->second.effect_scope!=caster||source->second.precise_health<=0||source->second.knocked_down_until>now)return false;
            origin=healing_position_locked(source->second,now);if(!origin)return false;
            const auto point=healing_position_locked(p,now);
            if(!point||std::hypot(point->x-origin->x,point->z-origin->z)>plan.range)return false;
            if(navigation_){const auto trace=layered_->trace(source->second.surface,*origin,*point);if(!trace||trace->plane!=p.surface.plane)return false;}
            const auto prevention=prevent_selected_hex_locked(source->second,p,227,now,false);if(!prevention)return false;
            prevented=*prevention;if(prevented)return true;
            effect=static_cast<unsigned>(p.next_health_effect);registry=*p.effect_registry;regeneration=*p.health_regeneration;
            applied=registry->apply(target,{effect,caster.actor,227,plan.rank,skills::effects::Category::hex,now,now+plan.duration},skills::effects::Replace::same_skill);
            if(!applied.accepted)return false;
            for(const auto& old:applied.removed)regeneration->remove(target,old.instance);
            if(!regeneration->apply(target,{effect,caster.actor,227,0,now,now+plan.duration}))return false;
        }else{
            auto npc=npc_combat_.find(target.actor);
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=target||next_npc_hex_==UINT32_MAX||
               npc->second.health_clock(npc->second.lifetime()).value_or(now)>now)return false;
            advance_npc_conditions_locked(target.actor,npc->second,now);refresh_npc_movement_locked(target.actor,now);
            npc=npc_combat_.find(target.actor);const auto body=npcs_.find(target.actor);
            origin=healing_position_locked(source->second,now);if(!origin)return false;
            if(source->second.effect_scope!=caster||source->second.precise_health<=0||source->second.knocked_down_until>now||
               npc==npc_combat_.end()||body==npcs_.end()||npc->second.hex_scope()!=target||
               body->second.plane!=source->second.surface.plane||
               std::hypot(body->second.x-origin->x,body->second.y-origin->z)>plan.range)return false;
            if(navigation_){const auto trace=layered_->trace(source->second.surface,*origin,{body->second.x,body->second.y});if(!trace||trace->plane!=body->second.plane)return false;}
            effect=next_npc_hex_;proposed_npc=npc->second;
            const auto result=proposed_npc->apply_glimmering_mark(proposed_npc->lifetime(),caster,plan,effect,now);
            if(!result)return false;applied=*result;
        }
        return true;
        };
        if(!prepare())return false;
        if(!prevented){auto checked=glimmering_marks_;if(!checked.add(caster,target,effect,level,plan,now))return false;}
        if(completion_event&&!complete_ether_renewal_locked(caster,227,*completion_event,now,true))return false;
        if(publication)begin_damage_publication_locked(pending);
        // Reward reactions can kill/retire or otherwise alter a recipient.
        // Completion remains consumed, but never restore a stale proposal.
        if(!prepare())return bool(completion_event);
        if(prevented){const auto result=prevent_selected_hex_locked(source->second,player->second,227,now);return result.has_value();}
        auto timeline=glimmering_marks_;
        if(!timeline.add(caster,target,effect,level,plan,now))return false;
        glimmering_marks_=std::move(timeline);
        glimmering_mark_owners_[target]={caster,target,effect,level,source->second.team_token,plan,now};
        if(proposed_npc){npc_combat_.at(target.actor)=std::move(*proposed_npc);++next_npc_hex_;
            npc_condition_credit_[{target.actor,caster.actor}]={target,caster};
            for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,npc_combat_.at(target.actor).actor(npc_combat_.at(target.actor).lifetime())->effects)});
        }else{auto& p=player->second;*p.effect_registry=std::move(*registry);*p.health_regeneration=std::move(*regeneration);++p.next_health_effect;
            std::vector<Packet> packets;for(const auto& old:applied.removed){p.health_effects.erase(old.instance);packets.push_back(health_effect_remove(p.id,old.instance));}
            const skills::effects::Entry entry{effect,caster.actor,227,plan.rank,skills::effects::Category::hex,now,now+plan.duration};
            p.health_effects.emplace(effect,entry);packets.push_back(health_effect_add(entry,p.id,now));
            for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);publish_status_locked(p,now);
        }
        commit_applications_locked(target,now);publish_target_body_locked(target.actor,227,caster.actor);return true;
        };
        const bool accepted=perform();
        auto release=finish_damage_publication_locked(pending);
        if(publication&&accepted)*publication=std::move(release);else if(release)release();
        return accepted;
    }
