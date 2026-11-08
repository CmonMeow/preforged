// Trusted mixed-area effect stage. Normal Runtime admission is still separate.
public:
    bool complete_signet_judgment(skills::effects::Scope source,skills::effects::Scope selected,
            const skills::effects::SignetJudgmentPlan& plan,unsigned level,std::uint64_t now){
        return stage_signet_judgment(source,selected,plan,level,now,true);
    }
    bool stage_signet_judgment(skills::effects::Scope source,skills::effects::Scope selected,
            const skills::effects::SignetJudgmentPlan& plan,unsigned level,std::uint64_t now,bool publish_completion=false){
        const auto canonical=skills::effects::signet_judgment(plan.rank,plan.skill==2887);
        if(!canonical||*canonical!=plan||!level||level>255||now>UINT64_MAX-plan.knockdown_ms)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        if(!settle_timed_effects_locked(now))return false;
        const auto source_player=players_.find(source.actor);
        if(source_player==players_.end()||source_player->second.effect_scope!=source)return false;
        auto& caster=source_player->second;
        if(!caster.resources||!caster.team_token||now<caster.health_updated||!advance_health_locked(caster,now)||caster.precise_health<=0||caster.knocked_down_until>now)return false;
        const auto origin=healing_position_locked(caster,now);if(!origin)return false;
        double center_x=0,center_z=0;unsigned plane=0;
        if(const auto p=players_.find(selected.actor);p!=players_.end()){
            auto& target=p->second;
            if(target.effect_scope!=selected||!target.resources||!target.team_token||target.team_token==caster.team_token||now<target.health_updated||!advance_health_locked(target,now)||target.precise_health<=0)return false;
            const auto point=healing_position_locked(target,now);if(!point)return false;
            center_x=point->x;center_z=point->z;plane=target.surface.plane;
        }else{
            auto n=npc_combat_.find(selected.actor);auto body=npcs_.find(selected.actor);
            if(n==npc_combat_.end()||body==npcs_.end()||n->second.hex_scope()!=selected||!n->second.hostile_to_players(n->second.lifetime()))return false;
            advance_npc_conditions_locked(selected.actor,n->second,now);refresh_npc_movement_locked(selected.actor,now);
            n=npc_combat_.find(selected.actor);body=npcs_.find(selected.actor);
            if(n==npc_combat_.end()||body==npcs_.end()||n->second.hex_scope()!=selected||!n->second.actor(n->second.lifetime())->health||n->second.actor(n->second.lifetime())->health->current<=0)return false;
            center_x=body->second.x;center_z=body->second.y;plane=body->second.plane;
        }
        const auto range=std::hypot(origin->x-center_x,origin->z-center_z);
        if(plane!=caster.surface.plane||range>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(caster.surface,*origin,{float(center_x),float(center_z)});if(!line||line->plane!=plane)return false;}
        std::vector<skills::effects::SignetJudgmentRecipient> inputs;
        for(auto& [id,p]:players_){
            if(!p.resources||!p.team_token||p.team_token==caster.team_token||p.surface.plane!=plane)continue;
            const auto point=healing_position_locked(p,now);if(!point)return false;
            const auto distance=std::hypot(point->x-center_x,point->z-center_z);if(distance>plan.radius)continue;
            if(now<p.health_updated||!advance_health_locked(p,now))return false;if(p.precise_health<=0)continue;
            if(!p.reviewed_holy_traits||!p.reviewed_holy_bonus)return false;
            auto actor=incoming_spell_actor_locked(p,{id,p.effect_scope.generation,combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true},now);
            const bool immune=p.effect_registry&&(skills::effects::active_dolyak(*p.effect_registry,p.effect_scope,now)||skills::effects::balanced_stance_active(*p.effect_registry,p.effect_scope,now));
            inputs.push_back({p.effect_scope,actor,*p.reviewed_holy_traits,distance,plane,!immune,p.knocked_down_until>now,*p.reviewed_holy_bonus,player_damage_projection_locked(p,now)});
        }
        std::vector<unsigned> npc_ids;for(const auto& [id,npc]:npcs_)npc_ids.push_back(id);
        for(auto id:npc_ids){auto n=npc_combat_.find(id);auto body=npcs_.find(id);
            if(n==npc_combat_.end()||body==npcs_.end()||!n->second.hostile_to_players(n->second.lifetime()))continue;
            if(id!=selected.actor){advance_npc_conditions_locked(id,n->second,now);refresh_npc_movement_locked(id,now);}
            n=npc_combat_.find(id);body=npcs_.find(id);if(n==npc_combat_.end()||body==npcs_.end()||body->second.plane!=plane)continue;
            const auto distance=std::hypot(body->second.x-center_x,body->second.y-center_z);if(distance>plan.radius)continue;
            const auto life=n->second.lifetime();const auto* actor=n->second.actor(life);if(actor&&actor->health&&actor->health->current<=0)continue;
            const auto traits=n->second.holy_traits(life);if(!traits)return false;
            const auto snapshot=n->second.holy_damage_snapshot(life,now);if(!snapshot)return false;
            combat::DamageProjection projection;const auto effects=n->second.owned_effect_snapshot(life);
            if(effects&&effects->newest_skill(n->second.hex_scope(),307,now))projection=[this,copy=n->second,now](combat::Actor& actor,const combat::Damage& damage)mutable->std::optional<combat::DamageStep>{
                auto raw=copy;const auto hit=raw.apply_damage(raw.lifetime(),damage,now);if(hit.reason!=combat::Reason::accepted)return {};
                std::optional<combat::Hit> actual;if(!preflight_npc_reversal_or_damage_locked(copy,damage,now,nullptr,&actual))return {};
                const auto* after=copy.actor(copy.lifetime());if(!after||!after->health)return {};actor=*after;return combat::DamageStep{hit,actual};
            };
            inputs.push_back({n->second.hex_scope(),*snapshot,*traits,distance,plane,n->second.knockdown_allowed(life,now),n->second.knocked_down(life,now),0,std::move(projection)});
        }
        // Primary-first is deterministic private ordering;the single-target
        // frozen window does not establish retail adjacent-recipient order.
        std::sort(inputs.begin(),inputs.end(),[&](const auto& a,const auto& b){if((a.owner==selected)!=(b.owner==selected))return a.owner==selected;return a.owner.actor<b.owner.actor;});
        const auto proposal=skills::effects::prepare_signet_judgment(plan,source,selected,level,range,plane,inputs);if(!proposal)return false;
        for(const auto& result:*proposal){
            if(const auto p=players_.find(result.target.actor);p!=players_.end()){if(!prepare_player_reactive_seed_locked(p->second,result.hit,now)||(result.knockdown_ms&&p->second.control_epoch==UINT64_MAX))return false;}
            else{auto preview=npc_combat_.at(result.target.actor);const auto life=preview.lifetime();
                const auto input=std::find_if(inputs.begin(),inputs.end(),[&](const auto& i){return i.owner==result.target;});
                auto damage=skills::effects::holy_strike(*skills::effects::holy_damage(plan.skill,plan.rank),input->traits,level);if(!damage)return false;damage->amount+=input->holy_bonus;
                if(!preflight_npc_reversal_or_damage_locked(preview,*damage,now)||(result.knockdown_ms&&preview.knock_down(life,now,result.knockdown_ms)!=combat::npcs::control::Knockdown::accepted))return false;
            }
        }
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;for(auto& [box,queue]:recipients)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        for(const auto& result:*proposal){
            if(const auto p=players_.find(result.target.actor);p!=players_.end()){
                auto& target=p->second;if(target.effect_scope!=result.target||target.precise_health<=0)continue;
                if(!target.reviewed_holy_traits||!target.reviewed_holy_bonus)throw std::logic_error("Signet Judgment lost reviewed player facts");
                auto damage=skills::effects::holy_strike(*skills::effects::holy_damage(plan.skill,plan.rank),*target.reviewed_holy_traits,level);if(!damage)throw std::logic_error("Signet Judgment lost holy traits");damage->amount+=*target.reviewed_holy_bonus;
                auto actor=incoming_spell_actor_locked(target,{target.id,result.target.generation,combat::Health{target.precise_health,target.resources->max_health},{},target.effects,true},now);
                const auto hit=combat::apply(actor,*damage,true);if(hit.reason!=combat::Reason::accepted)throw std::logic_error("preflighted Signet Judgment player damage failed");
                if(!commit_player_reactive_hit_locked(target,hit,source.actor,now))throw std::logic_error("preflighted Signet Judgment player conversion failed");
                if(target.precise_health>0&&result.knockdown_ms&&target.knocked_down_until<=now&&!knockdown_player_locked(target,plan.knockdown_ms,now))throw std::logic_error("preflighted Signet Judgment player control failed");
            }else{
                auto n=npc_combat_.find(result.target.actor);const auto body=npcs_.find(result.target.actor);
                if(n==npc_combat_.end()||body==npcs_.end()||n->second.hex_scope()!=result.target||!n->second.actor(n->second.lifetime())->health||n->second.actor(n->second.lifetime())->health->current<=0)continue;
                const auto* traits=n->second.holy_traits(n->second.lifetime());if(!traits)throw std::logic_error("Signet Judgment lost NPC holy facts");
                const auto damage=skills::effects::holy_strike(*skills::effects::holy_damage(plan.skill,plan.rank),*traits,level);if(!damage)throw std::logic_error("Signet Judgment lost NPC sensitivity");
                if(!commit_npc_reversal_or_damage_locked(caster,body->second,n->second,*damage,now))throw std::logic_error("preflighted Signet Judgment NPC damage failed");
                if(n->second.actor(n->second.lifetime())->health->current>0&&result.knockdown_ms){using K=combat::npcs::control::Knockdown;const auto knock=knockdown_npc_locked(result.target,plan.knockdown_ms,now);if(knock!=K::accepted&&knock!=K::immune&&knock!=K::already_down)throw std::logic_error("preflighted Signet Judgment NPC control failed");}
            }
        }
        captures.clear();
        for(auto& [box,queue]:recipients){std::vector<Packet> packets;
            if(publish_completion)packets={combat::detail::integer(58,source.actor,0),combat::detail::integer(21,source.actor,519)};
            packets.insert(packets.end(),std::make_move_iterator(queue.packets.begin()),std::make_move_iterator(queue.packets.end()));box->enqueue(std::move(packets));}
        return true;
    }
