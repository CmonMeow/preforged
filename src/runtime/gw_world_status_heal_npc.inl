// Included inside Instance after shared direct-heal target validation.
public:
    bool valid_status_heal_target(skills::effects::Scope source,skills::effects::Scope target,
            unsigned skill,std::uint64_t now)const{
        const auto plan=skills::effects::status_heal(skill,0,0);if(!plan)return false;
        std::lock_guard lock(mutex_);
        return direct_heal_target_locked(source,target,{skill,plan->range,plan->self,false,1},now);
    }
private:
    bool status_heal_npc_locked(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::StatusHealPlan& plan,std::uint64_t now,
            std::optional<std::uint64_t> event){
        const auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           (plan.divine_heal&&!monk_primary(caster->second))||
           !direct_heal_target_locked(source,target,{plan.skill,plan.range,plan.self,false,1},now))return false;
        auto found=npc_combat_.find(target.actor);if(found==npc_combat_.end())return false;
        advance_npc_conditions_locked(target.actor,found->second,now);
        found=npc_combat_.find(target.actor);
        if(found==npc_combat_.end()||found->second.hex_scope()!=target)return false;
        const auto life=found->second.lifetime();const auto* actor=found->second.actor(life);
        const auto conditions=found->second.condition_snapshot(life);
        if(!actor||!actor->health||actor->health->current<=0||!conditions)return false;
        unsigned enchantments=0,hexes=0;
        if(plan.skill==283){
            const auto count=found->second.enchantment_count(life,now);
            const auto registry=found->second.hex_registry_snapshot(life,now);
            if(!count||!registry)return false;
            enchantments=*count;hexes=registry->count(target,skills::effects::Category::hex,now);
            if(!hexes&&(actor->effects&0x800))return false;
        }
        const auto amount=skills::effects::status_heal_amount(plan,enchantments,hexes,conditions->count(now));
        if(!amount)return false;
        auto proposed=found->second;const auto before=actor->health->current;
        if(!proposed.heal_ally(life,*amount,now))return false;
        if(event&&!complete_ether_renewal_locked(source,plan.skill,*event,now,true))return false;
        found->second=std::move(proposed);const auto health=*found->second.actor(life)->health;
        for(auto&[id,peer]:players_){peer.outbox->enqueue({combat::detail::fraction(target.actor,health)});
            if(health.current!=before)peer.outbox->enqueue({skills::particles::health_change_2022(
                target.actor,source.actor,health.current-before,health.maximum)});}
        return true;
    }
public:
