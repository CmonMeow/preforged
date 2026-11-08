// Included inside Instance. One shared preparation contact authority.
    std::optional<combat::weapon::OwnedPreparationContact> weapon_preparation_locked(const Player& source,const combat::weapon::Owned& weapon,std::uint64_t now)const{
        return source.effect_registry?combat::weapon::preparation_contact_snapshot(*source.effect_registry,source.effect_scope,weapon,now):std::nullopt;
    }
    bool preparation_contact_ready_locked(skills::effects::Scope target,const combat::weapon::PreparationDefinition& plan,unsigned level,std::uint64_t now,skills::effects::Scope source)const{
        if(now>UINT64_MAX-plan.condition_duration)return false;
        if(plan.kind==combat::weapon::PreparationContactKind::wager||plan.kind==combat::weapon::PreparationContactKind::accuracy)return true;
        if(plan.kind!=combat::weapon::PreparationContactKind::kindle){
            if(const auto p=players_.find(target.actor);p!=players_.end())return p->second.effect_scope==target&&p->second.resources&&
                p->second.precise_health>0&&now>=p->second.health_updated&&p->second.next_health_effect<=UINT32_MAX-2;
            return true;
        }
        if(const auto p=players_.find(target.actor);p!=players_.end()){
            if(p->second.effect_scope!=target||!p->second.resources||now<p->second.health_updated)return false;
            auto actor=incoming_spell_actor_locked(p->second,{p->second.id,p->second.effect_scope.generation,
                combat::Health{p->second.precise_health,p->second.resources->max_health},{},p->second.effects,true,{},p->second.reviewed_fire_armor},now,true);
            return combat::apply(actor,{combat::Kind::fire,double(plan.bonus),level,plan.skill},true).reason==combat::Reason::accepted;
        }
        const auto npc=npc_combat_.find(target.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=target)return false;
        auto preview=npc->second;
        const auto caster=players_.find(source.actor);if(caster==players_.end()||caster->second.effect_scope!=source)return false;
        const auto foe=player_npc_attack_relation_locked(caster->second,preview);if(!foe||!*foe)return false;
        const combat::Damage damage{combat::Kind::fire,double(plan.bonus),level,plan.skill};
        return (preview.hostile_to_players(preview.lifetime())?preview.apply_damage(preview.lifetime(),damage,now):preview.apply_opposed_owned_damage(preview.lifetime(),damage,now)).reason==combat::Reason::accepted;
    }
    std::optional<bool> preparation_enchanted_locked(skills::effects::Scope target,std::uint64_t now)const{
        if(const auto p=players_.find(target.actor);p!=players_.end()){
            if(p->second.effect_scope!=target||!p->second.effect_registry)return {};
            const auto count=p->second.effect_registry->count(target,skills::effects::Category::enchantment,now);
            if(!count&&(p->second.effects&0x80))return {};return count!=0;
        }
        const auto npc=npc_combat_.find(target.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=target)return {};
        const auto count=npc->second.enchantment_count(npc->second.lifetime(),now);return count?std::optional(*count!=0):std::nullopt;
    }
    // The primary contact owns the one attack event and death cleanup. The
    // separate armor-affected Fire hit is not another attack/critical event.
    bool preparation_fire_locked(Player& source,skills::effects::Scope target,const combat::weapon::PreparationDefinition& plan,unsigned level,std::uint64_t now){
        if(plan.kind!=combat::weapon::PreparationContactKind::kindle)return false;
        if(const auto p=players_.find(target.actor);p!=players_.end()){
            auto& victim=p->second;if(victim.effect_scope!=target||!victim.resources||victim.precise_health<=0)return false;
            auto actor=incoming_spell_actor_locked(victim,{victim.id,target.generation,
                combat::Health{victim.precise_health,victim.resources->max_health},{},victim.effects,true,{},victim.reviewed_fire_armor},now,true);
            const auto hit=combat::apply(actor,{combat::Kind::fire,double(plan.bonus),level,plan.skill},true);
            if(hit.reason!=combat::Reason::accepted)throw std::logic_error("preflighted preparation Fire hit rejected");
            if(!commit_player_reactive_hit_locked(victim,hit,source.id,now,true))throw std::logic_error("Preparation Fire commit failed");
            return victim.precise_health<=0;
        }
        const auto npc=npc_combat_.find(target.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=target)return false;
        auto& live=npc->second;const auto* body=live.actor(live.lifetime());if(!body||!body->health||body->health->current<=0)return false;
        const auto foe=player_npc_attack_relation_locked(source,live);if(!foe||!*foe)return false;
        const combat::Damage damage{combat::Kind::fire,double(plan.bonus),level,plan.skill};
        const auto hit=live.hostile_to_players(live.lifetime())?live.apply_damage(live.lifetime(),damage,now):live.apply_opposed_owned_damage(live.lifetime(),damage,now);
        if(hit.reason!=combat::Reason::accepted)throw std::logic_error("preflighted NPC preparation Fire hit rejected");
        for(auto&[id,peer]:players_)peer.outbox->enqueue(combat::damage_packets_2022(source.id,hit));return hit.killed;
    }
    // Apply Poison explicitly covers the attack skill's own condition. This
    // shared commit runs after those contact effects, never on miss/block/IW.
    void preparation_condition_locked(Player& source,skills::effects::Scope target,const combat::weapon::PreparationDefinition& plan,std::uint64_t now){
        using K=combat::weapon::PreparationContactKind;if(plan.kind!=K::poison&&plan.kind!=K::melandru)return;
        const auto kind=plan.kind==K::poison?combat::conditions::Kind::poison:combat::conditions::Kind::bleeding;
        if(const auto p=players_.find(target.actor);p!=players_.end()){
            auto& victim=p->second;if(victim.effect_scope!=target||!victim.resources||victim.precise_health<=0)return;
            auto conditions=victim.conditions;if(conditions.apply(kind,source.id,now,plan.condition_duration))commit_conditions_locked(victim,conditions,{kind},now);
        }else if(const auto npc=npc_combat_.find(target.actor);npc!=npc_combat_.end()&&npc->second.hex_scope()==target){
            auto& live=npc->second;const auto life=live.lifetime();const auto* body=live.actor(life);
            if(!body||!body->health||body->health->current<=0||!live.apply_condition(life,kind,source.id,now,plan.condition_duration))return;
            npc_condition_credit_[{target.actor,source.id}]={target,source.effect_scope};if(publish_npc_fragility_locked(target.actor,live))return;
            for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,live.actor(life)->effects)});
            commit_applications_locked(target,now);
        }
    }
