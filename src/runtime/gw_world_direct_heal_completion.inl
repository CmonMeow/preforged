// Included inside Instance. Paid spell admission belongs to Runtime; this
// producer owns target validation, resource completion and conditional healing.
private:
    bool direct_heal_target_locked(skills::effects::Scope caller,skills::effects::Scope target,
            const skills::effects::DirectHeal& plan,std::uint64_t now)const{
        const auto source=players_.find(caller.actor);
        if(source==players_.end()||source->second.effect_scope!=caller||!source->second.resources||
           source->second.precise_health<=0)return false;
        now=std::max({now,source->second.health_updated,source->second.energy_updated});
        const auto origin=healing_position_locked(source->second,now);if(!origin)return false;
        if(const auto recipient=players_.find(target.actor);recipient!=players_.end()){
            const auto& p=recipient->second;
            now=std::max(now,p.health_updated);
            if(source->second.knocked_down_until>now||p.effect_scope!=target||
               !live_heal_target_locked(source->second,p,std::numeric_limits<float>::max(),plan.self))return false;
            const auto position=healing_position_locked(p,now);
            return position&&std::hypot(origin->x-position->x,origin->z-position->z)<=plan.range;
        }
        const auto body=npcs_.find(target.actor);const auto live=npc_combat_.find(target.actor);
        if(live!=npc_combat_.end())now=std::max(now,live->second.health_clock(live->second.lifetime()).value_or(now));
        return body!=npcs_.end()&&live!=npc_combat_.end()&&live->second.hex_scope()==target&&
            source->second.knocked_down_until<=now&&
            live->second.healable_ally(live->second.lifetime())&&body->second.plane==source->second.surface.plane&&
            now>=live->second.health_clock(live->second.lifetime()).value_or(now)&&
            std::hypot(origin->x-body->second.x,origin->z-body->second.y)<=plan.range;
    }
public:
    bool valid_direct_heal_target(skills::effects::Scope caller,skills::effects::Scope target,
            unsigned skill,std::uint64_t now)const{
        const auto plan=skills::effects::direct_heal(skill);const auto* row=skills::find_skill(skill);
        if(!plan||!row||!skills::effects::is_spell(*row))return false;
        std::lock_guard lock(mutex_);return direct_heal_target_locked(caller,target,*plan,now);
    }
    bool complete_direct_heal(skills::effects::Scope caller,skills::effects::Scope target,
            unsigned skill,unsigned rank,unsigned divine,std::uint64_t event,std::uint64_t now){
        const auto plan=skills::effects::direct_heal(skill);const auto* row=skills::find_skill(skill);
        if(!event||!plan||!row||!skills::effects::is_spell(*row)||rank>20||divine>20)return false;
        std::lock_guard lock(mutex_);
        // A different session may tick World after Runtime admits the spell.
        // Settle against current authoritative clocks, never rewind them.
        const auto source=players_.find(caller.actor);
        if(source==players_.end()||source->second.effect_scope!=caller)return false;
        now=std::max({now,source->second.health_updated,source->second.energy_updated});
        if(const auto p=players_.find(target.actor);p!=players_.end())now=std::max(now,p->second.health_updated);
        else if(const auto n=npc_combat_.find(target.actor);n!=npc_combat_.end())
            now=std::max(now,n->second.health_clock(n->second.lifetime()).value_or(now));
        if(!direct_heal_target_locked(caller,target,*plan,now))return false;
        auto& caster=players_.at(caller.actor);
        if((divine&&!monk_primary(caster))||!advance_resource_event_locked(caster,now)||caster.precise_health<=0)return false;
        auto reward=[&](bool enchanted,bool below_half){
            if(caster.precise_health<=0)return;
            const unsigned energy=skill==867&&enchanted?skills::effects::heal_scale(row->bonus0,row->bonus15,rank):
                skill==1687&&below_half?7u:0u;
            if(energy){const auto gained=gain_energy_locked(caster,energy,now);
                if(gained.changed())caster.outbox->enqueue(owner_vitals(caster));}
        };
        if(const auto found=players_.find(target.actor);found!=players_.end()){
            auto& recipient=found->second;
            if(!advance_health_locked(recipient,now)||recipient.precise_health<=0)return false;
            if(skill==867&&!recipient.effect_registry)return false;
            if(!skills::effects::direct_heal_amount_precise(skill,rank,divine,recipient.precise_health,recipient.resources->max_health))return false;
            if(!complete_ether_renewal_locked(caller,skill,event,now,true))return false;
            if(caster.precise_health<=0||recipient.precise_health<=0)return true;
            // In particular, self-targeted Word/Zealous must inspect Health
            // after Renewal's healing and before this spell's own healing.
            const auto amount=skills::effects::direct_heal_amount_precise(skill,rank,divine,
                recipient.precise_health,recipient.resources->max_health);
            const bool below_half=recipient.precise_health*2<recipient.resources->max_health;
            const bool enchanted=skill==867&&recipient.effect_registry->has(target,skills::effects::Category::enchantment,now);
            const auto before=recipient.precise_health;health_event_locked(recipient,*amount,now,true);
            publish_health_locked(recipient);
            if(recipient.precise_health!=before)for(auto&[id,peer]:players_)
                peer.outbox->enqueue({skills::particles::health_change_2022(target.actor,caller.actor,
                    recipient.precise_health-before,recipient.resources->max_health)});
            if(recipient.precise_health<=0)kill_locked(recipient,now);
            reward(enchanted,below_half);return true;
        }
        auto found=npc_combat_.find(target.actor);
        if(found==npc_combat_.end()||found->second.hex_scope()!=target)return false;
        advance_npc_conditions_locked(target.actor,found->second,now);
        found=npc_combat_.find(target.actor);
        if(found==npc_combat_.end()||found->second.hex_scope()!=target)return false;
        const auto life=found->second.lifetime();const auto* state=found->second.actor(life);
        if(!state||!state->health||state->health->current<=0)return false;
        const auto count=skill==867?found->second.enchantment_count(life,now):std::optional<unsigned>(0);
        const auto amount=skills::effects::direct_heal_amount_precise(skill,rank,divine,
            state->health->current,state->health->maximum);
        if(!amount||!count)return false;
        auto preview=found->second;if(!preview.heal_ally(life,*amount,now))return false;
        if(!complete_ether_renewal_locked(caller,skill,event,now,true))return false;
        if(caster.precise_health<=0||!found->second.healable_ally(life))return true;
        const auto before=*found->second.actor(life)->health;
        const bool below_half=before.current*2<before.maximum;
        if(!found->second.heal_ally(life,*amount,now))throw std::logic_error("preflighted direct NPC heal failed");
        const auto after=*found->second.actor(life)->health;
        for(auto&[id,peer]:players_){peer.outbox->enqueue({combat::detail::fraction(target.actor,after)});
            if(after.current!=before.current)peer.outbox->enqueue({skills::particles::health_change_2022(
                target.actor,caller.actor,after.current-before.current,after.maximum)});}
        reward(*count!=0,below_half);return true;
    }
