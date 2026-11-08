// Trusted Aneurysm effect. Normal client casting remains gated until Runtime
// payment, target revalidation and 2022 publication ordering are connected.
public:
    bool stage_aneurysm(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::AneurysmPlan& plan,unsigned level,std::uint64_t now,
            std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        if(skills::effects::aneurysm(plan.rank)!=plan||!level||level>255||source==target)return false;
        std::lock_guard lock(mutex_);
        settle_damage_fields_locked(now);
        const auto caster_it=players_.find(source.actor);
        if(caster_it==players_.end()||caster_it->second.effect_scope!=source||
           !caster_it->second.resources||!caster_it->second.team_token||
           !advance_resource_event_locked(caster_it->second,now)||
           caster_it->second.precise_health<=0||caster_it->second.knocked_down_until>now)return false;
        auto& caster=caster_it->second;
        const auto origin=healing_position_locked(caster,now);
        if(!origin)return false;

        auto primary_player=players_.find(target.actor);
        auto primary_npc=npc_combat_.find(target.actor);
        auto primary_place=npcs_.find(target.actor);
        std::optional<navigation::Point> center;
        unsigned plane=0,current_energy=0,effective_max=0,base_max=0;
        combat::Actor actor;
        if(primary_player!=players_.end()){
            auto& p=primary_player->second;
            if(p.effect_scope!=target||!p.resources||!p.team_token||
               p.team_token==caster.team_token||p.energy_debt||
               !advance_resource_event_locked(p,now)||p.precise_health<=0||
               !settle_mind_wrack_caster_locked(target,now))return false;
            center=healing_position_locked(p,now);plane=p.surface.plane;
            current_energy=p.resources->energy;base_max=p.resources->max_energy;
            const auto ceiling=p.overcast_curve?
                p.overcast_curve->integer_ceiling(base_max):std::int64_t(base_max);
            effective_max=static_cast<unsigned>(std::clamp<std::int64_t>(ceiling,0,base_max));
            actor=incoming_spell_actor_locked(p,{p.id,target.generation,
                combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true},now);
        }else{
            if(primary_npc==npc_combat_.end()||primary_place==npcs_.end()||
               primary_npc->second.hex_scope()!=target||
               !primary_npc->second.hostile_to_players(primary_npc->second.lifetime()))return false;
            auto& live=primary_npc->second;const auto life=live.lifetime();
            if(now<live.health_clock(life).value_or(now)||
               !settle_mind_wrack_caster_locked(target,now))return false;
            advance_npc_conditions_locked(target.actor,live,now);
            refresh_npc_movement_locked(target.actor,now);
            if(!live.advance_energy(life,now))return false;
            const auto energy=live.energy(life);const auto* body=live.actor(life);
            if(!energy||!body||!body->health||body->health->current<=0)return false;
            center=navigation::Point{primary_place->second.x,primary_place->second.y};
            plane=primary_place->second.plane;
            current_energy=energy->current;effective_max=base_max=energy->maximum;
            actor=*body;
        }
        if(!center||plane!=caster.surface.plane||
           std::hypot(center->x-origin->x,center->z-origin->z)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(caster.surface,*origin,*center);
            if(!line||line->plane!=plane)return false;}

        std::vector<skills::effects::AneurysmNearby> samples;
        for(auto& [id,p]:players_){
            if(id==source.actor||id==target.actor||!p.team_token||
               p.team_token==caster.team_token||p.surface.plane!=plane)continue;
            if(!p.resources||!advance_resource_event_locked(p,now)||p.precise_health<=0)return false;
            const auto point=healing_position_locked(p,now);
            if(!point)return false;
            const auto distance=std::hypot(point->x-center->x,point->z-center->z);
            if(distance>plan.adjacent_radius)continue;
            if(!settle_mind_wrack_caster_locked(p.effect_scope,now))return false;
            samples.push_back({{p.id,p.effect_scope.generation,
                combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true},
                float(distance),true,p.resources->energy});
        }
        for(auto& [id,live]:npc_combat_){
            if(id==target.actor)continue;
            const auto place=npcs_.find(id);
            const auto life=live.lifetime();
            if(place==npcs_.end()||place->second.plane!=plane||
               !live.hostile_to_players(life))continue;
            refresh_npc_movement_locked(id,now);
            const auto distance=std::hypot(place->second.x-center->x,place->second.y-center->z);
            if(distance>plan.adjacent_radius)continue;
            if(now<live.health_clock(life).value_or(now)||
               !settle_mind_wrack_caster_locked(live.hex_scope(),now))return false;
            advance_npc_conditions_locked(id,live,now);
            if(!live.advance_energy(life,now))return false;
            const auto* body=live.actor(life);const auto energy=live.energy(life);
            if(!body||!body->health||body->health->current<=0)continue;
            samples.push_back({*body,float(distance),true,
                energy?std::optional<unsigned>(energy->current):std::nullopt});
        }
        const auto proposal=skills::effects::aneurysm_area(plan,actor,current_energy,
            effective_max,base_max,level,samples);
        if(!proposal)return false;
        if(primary_player!=players_.end()&&proposal->primary.hit&&
           !prepare_player_reactive_seed_locked(primary_player->second,*proposal->primary.hit,now))return false;
        // Preflight the mutable NPC copies and every Overcast player loss.
        if(primary_npc!=npc_combat_.end()&&primary_player==players_.end()){
            auto copy=primary_npc->second;const auto life=copy.lifetime();
            if(!copy.gain_energy(life,proposal->primary.restored_energy))return false;
            if(proposal->primary.hit&&copy.apply_damage(life,
                {combat::Kind::armor_ignoring,double(proposal->primary.damage_energy)*plan.damage_per_energy,
                 level,2055},now).reason!=combat::Reason::accepted)return false;
        }
        if(primary_player!=players_.end()&&primary_player->second.overcast_curve){
            auto& p=primary_player->second;
            if(!overcast::prepare_gain(*p.overcast_curve,p.effect_scope,*p.resources,
                    p.energy_regeneration,p.energy_debt,proposal->primary.restored_energy,
                    now,p.energy_pips,p.energy_fractional_tail))return false;
        }
        for(const auto& loss:proposal->adjacent){
            if(auto p=players_.find(loss.actor);p!=players_.end()){
                if(p->second.effect_scope.generation!=loss.generation||
                   !p->second.resources||p->second.energy_debt)return false;
                if(p->second.overcast_curve&&
                   !overcast::prepare_loss(*p->second.overcast_curve,p->second.effect_scope,
                       *p->second.resources,p->second.energy_regeneration,
                       p->second.energy_debt,loss.energy,now,p->second.energy_pips,
                       overcast::LossRule::drain_to_zero,p->second.energy_fractional_tail))return false;
            }else{
                const auto n=npc_combat_.find(loss.actor);
                if(n==npc_combat_.end()||n->second.hex_scope().generation!=loss.generation)return false;
                auto copy=n->second;
                if(!copy.drain_energy(copy.lifetime(),loss.energy))return false;
            }
        }

        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,p]:players_)if(seen.insert(p.outbox.get()).second)
            queues.emplace_back(p.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        publish_target_body_locked(target.actor,2055,caster.id);
        if(primary_player!=players_.end()){
            auto& p=primary_player->second;
            const auto before=mind_wrack_energy_locked(p);
            gain_energy_locked(p,proposal->primary.restored_energy,now);
            p.outbox->enqueue(owner_vitals(p));
            observe_mind_wrack_energy_locked(p,before,now);
            if(proposal->primary.hit){
                if(p.effect_scope==target&&p.precise_health>0){
                    auto current=incoming_spell_actor_locked(p,{p.id,target.generation,
                        combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true},now);
                    const auto hit=combat::apply(current,{combat::Kind::armor_ignoring,
                        double(proposal->primary.damage_energy)*plan.damage_per_energy,level,2055},true);
                    if(hit.reason!=combat::Reason::accepted||
                       !commit_player_reactive_hit_locked(p,hit,caster.id,now,false,nullptr,false,false))
                        throw std::logic_error("preflighted Aneurysm player damage failed");
                }
            }
        }else{
            auto& live=primary_npc->second;const auto life=live.lifetime();
            const auto energy=*live.energy(life);
            const auto units=live.precise_energy_units(life);
            if(!live.gain_energy(life,proposal->primary.restored_energy))
                throw std::logic_error("preflighted Aneurysm NPC Energy gain failed");
            observe_npc_mind_wrack_energy_locked(target,{energy.current,energy.maximum,0,
                units?unsigned(*units%300000):0},now);
            if(proposal->primary.hit){
                const auto hit=damage_npc_locked(caster,primary_place->second,live,
                    {combat::Kind::armor_ignoring,
                     double(proposal->primary.damage_energy)*plan.damage_per_energy,level,2055},now);
                if(hit.reason!=combat::Reason::accepted)
                    throw std::logic_error("preflighted Aneurysm NPC damage failed");
            }
        }
        for(const auto& loss:proposal->adjacent){
            if(auto p=players_.find(loss.actor);p!=players_.end()){
                auto& victim=p->second;
                if(victim.precise_health<=0)continue;
                const auto before=mind_wrack_energy_locked(victim);
                drain_energy_locked(victim,loss.energy,now);
                victim.outbox->enqueue(owner_vitals(victim));
                observe_mind_wrack_energy_locked(victim,before,now);
            }else{
                auto& live=npc_combat_.at(loss.actor);const auto life=live.lifetime();
                if(!live.actor(life)->health||live.actor(life)->health->current<=0)continue;
                const auto energy=*live.energy(life);const auto units=live.precise_energy_units(life);
                if(!live.drain_energy(life,loss.energy))
                    throw std::logic_error("preflighted Aneurysm adjacent NPC drain failed");
                observe_npc_mind_wrack_energy_locked(live.hex_scope(),
                    {energy.current,energy.maximum,0,units?unsigned(*units%300000):0},now);
            }
        }
        if(caster.passive_recovery)caster.passive_recovery->record(
            recovery::Event::target_enemy_spell_or_signet,now);
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));
        return true;
    }
