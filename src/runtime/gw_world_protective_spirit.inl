// Included inside Instance. All publication happens before releasing its mutex.
private:
    std::map<std::pair<unsigned,unsigned>,skills::effects::Scope> npc_simple_enchantment_credit_;
    void publish_npc_simple_enchantment_ends_locked(combat::npcs::LiveActor& live){
        const auto scope=live.hex_scope();
        for(auto credit=npc_simple_enchantment_credit_.lower_bound({scope.actor,0});credit!=npc_simple_enchantment_credit_.end()&&credit->first.first==scope.actor;){
            if(credit->second!=scope){npc_seed_casters_.erase(credit->second);release_judgment_credit_locked(credit->second,credit->first.second);credit=npc_simple_enchantment_credit_.erase(credit);continue;}
            if(live.hex_effect(live.lifetime(),credit->first.second)){++credit;continue;}
            if(const auto seed=npc_seed_casters_.find(scope);seed!=npc_seed_casters_.end()&&seed->second.first==credit->first.second)npc_seed_casters_.erase(seed);
            const auto packet=health_effect_remove(scope.actor,credit->first.second);release_judgment_credit_locked(scope,credit->first.second);credit=npc_simple_enchantment_credit_.erase(credit);
            for(auto& [id,peer]:players_)peer.outbox->enqueue({packet});
        }
    }
public:
    // Trusted host effect application, like armor_earth; not a paid NPC cast.
    // The caller owns AI admission/payment. Never accept this from a client.
    bool stage_npc_shield_deflection(skills::effects::Scope owner,const skills::effects::ShieldDeflectionPlan& plan,std::uint64_t now){return stage_npc_simple_enchantment<259>(owner,plan,now);}
private:
    template<unsigned skill,class Plan>
    bool stage_npc_simple_enchantment(skills::effects::Scope owner,const Plan& plan,std::uint64_t now){
        static_assert(skill==259||skill==254);
        const auto canonical=[&]{if constexpr(skill==254)return skills::effects::vigorous_spirit(plan.rank);else return skills::effects::shield_deflection(plan.rank);}();if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);if(next_npc_hex_==UINT32_MAX)return false;
        auto live=npc_combat_.find(owner.actor);if(live==npc_combat_.end()||live->second.hex_scope()!=owner)return false;
        advance_npc_conditions_locked(owner.actor,live->second,now);
        live=npc_combat_.find(owner.actor);if(live==npc_combat_.end()||live->second.hex_scope()!=owner)return false;
        if(well_blocks_npc_enchantment_locked(owner.actor,now))return true;
        if constexpr(skill==254)if(!live->second.healing_allowed(live->second.lifetime()).has_value())return false;
        auto proposed=live->second;const auto previous=proposed.owned_effect_snapshot(proposed.lifetime());if(!previous)return false;
        const auto effect=next_npc_hex_;const bool applied=[&]{if constexpr(skill==254)return proposed.apply_vigorous_spirit(proposed.lifetime(),owner,plan,effect,now);else return proposed.apply_shield_deflection(proposed.lifetime(),owner,plan,effect,now);}();if(!applied)return false;
        const auto entry=proposed.hex_effect(proposed.lifetime(),effect);if(!entry)return false;
        std::vector<Packet> packets;
        for(const auto& old:previous->active(owner,now))if(old.skill==skill){packets.push_back(health_effect_remove(owner.actor,old.instance));npc_simple_enchantment_credit_.erase({owner.actor,old.instance});}
        live->second=std::move(proposed);++next_npc_hex_;npc_simple_enchantment_credit_[{owner.actor,effect}]=owner;
        packets.push_back(health_effect_add(*entry,owner.actor,now));
        packets.push_back(emotes::effects_2022(owner.actor,live->second.actor(live->second.lifetime())->effects));
        for(auto& [id,peer]:players_)peer.outbox->enqueue(packets);return true;
    }
private:
    template<unsigned skill,class Plan>
    bool complete_simple_enchantment(skills::effects::Scope source,skills::effects::Scope target,
            const Plan& plan,unsigned divine_rank,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr,std::optional<std::uint64_t> completion_event={}){
        static_assert(skill==113||skill==245||skill==259||skill==261||skill==262||skill==310||skill==254||skill==274||skill==307);
        if constexpr(skill==310)if(source!=target)return false;
        if constexpr(skill==274)if(source==target)return false;
        const auto canonical=[&]{if constexpr(skill==113)return skills::effects::tainted_flesh(plan.rank);else if constexpr(skill==245)return skills::effects::protective_spirit(plan.rank);else if constexpr(skill==259)return skills::effects::shield_deflection(plan.rank);else if constexpr(skill==261)return skills::effects::shield_regeneration(plan.rank);else if constexpr(skill==262)return skills::effects::shield_judgment(plan.rank);else if constexpr(skill==254)return skills::effects::vigorous_spirit(plan.rank);else if constexpr(skill==274)return skills::effects::healing_seed(plan.rank);else if constexpr(skill==307)return skills::effects::reversal_fortune(plan.rank);else return skills::effects::divine_spirit(plan.rank);}();
        const auto duration=adjusted?adjusted->for_enchantment(skill,plan.duration,source,now):std::optional(plan.duration);
        if(!canonical||*canonical!=plan||!duration||now>UINT64_MAX-*duration)return false;
        std::lock_guard lock(mutex_);
        if(!valid_ally_enchantment_target_locked(source,target,divine_rank,now))return false;
        if constexpr(skill==307){
            prune_reversal_caster_credit_locked(now);
            if(source!=target&&reversal_caster_credit_.size()>=4096)return false;
        }
        auto& caster=players_.at(source.actor);
        if(completion_event&&!complete_ether_renewal_locked(source,skill,*completion_event,now,true,false))return false;
        if constexpr(skill==262)if(caster.ordinary_level<1||caster.ordinary_level>255)return false;
        if constexpr(skill!=113)if(adjusted&&caster.effect_registry&&caster.effect_registry->newest_skill(source,256,now))return false;
        const auto visual=skill==310?std::optional<Packet>{combat::detail::integer(21,source.actor,553)}:((skill==113||skill==259||skill==261||skill==262||skill==254||skill==307)&&target==source)?skills::particles::target_body_2022(target.actor,skill):skills::particles::targeted_body_2022(target.actor,source.actor,skill);if(!visual)return false;
        const auto player=players_.find(target.actor);auto npc=npc_combat_.find(target.actor);
        // Profane prevents a new enchantment, but the paid spell still
        // completes. Publish its ordinary completion/visual without creating
        // an effect, Divine Favor heal, or a phantom status-add packet.
        if((player!=players_.end()&&well_blocks_player_enchantment_locked(player->second,now))||
           (npc!=npc_combat_.end()&&well_blocks_npc_enchantment_locked(target.actor,now))){
            if(completion_event&&!complete_ether_renewal_locked(source,skill,*completion_event,now,true))return false;
            const std::vector<Packet> prefix{combat::detail::integer(58,source.actor,0),*visual};
            for(auto& [id,peer]:players_)peer.outbox->enqueue(prefix);
            return true;
        }
        unsigned effect=0;std::vector<unsigned> previous_effects;std::vector<Packet> statuses;std::optional<combat::npcs::LiveActor> npc_proposal;
        std::optional<skills::effects::Registry> registry;std::optional<skills::regeneration::State> regeneration;
        skills::effects::Entry entry{};
        const auto prepare=[&]()->bool{
        previous_effects.clear();statuses.clear();npc_proposal.reset();registry.reset();regeneration.reset();
        if(player!=players_.end()){
            auto& recipient=player->second;
            if(!recipient.effect_registry||!recipient.health_regeneration||recipient.next_health_effect>UINT32_MAX)return false;
            effect=static_cast<unsigned>(recipient.next_health_effect);registry=*recipient.effect_registry;regeneration=*recipient.health_regeneration;
            entry={effect,source.actor,skill,plan.rank,skills::effects::Category::enchantment,now,now+*duration};
            const auto changed=registry->apply(target,entry,skills::effects::Replace::same_skill);
            const auto pips=[&]{if constexpr(skill==261)return int(plan.pips);else return 0;}();
            if(!changed.accepted||!regeneration->apply(target,{effect,source.actor,skill,pips,now,now+*duration}))return false;
            for(const auto& old:changed.removed){statuses.push_back(health_effect_remove(target.actor,old.instance));previous_effects.push_back(old.instance);}
        }else{
            if(npc==npc_combat_.end()||next_npc_hex_==UINT32_MAX)return false;
            if constexpr(skill==254||skill==274||skill==307)if(!npc->second.healing_allowed(npc->second.lifetime()).has_value())return false;
            effect=next_npc_hex_;npc_proposal=npc->second;const auto previous=npc_proposal->owned_effect_snapshot(npc_proposal->lifetime());if(!previous)return false;
            const bool applied=[&]{if constexpr(skill==310)return false;else if constexpr(skill==113)return npc_proposal->apply_tainted_flesh(npc_proposal->lifetime(),source,plan,effect,now,adjusted).has_value();else if constexpr(skill==307)return npc_proposal->apply_reversal_fortune(npc_proposal->lifetime(),source,plan,effect,now,adjusted);else if constexpr(skill==274)return npc_proposal->apply_healing_seed(npc_proposal->lifetime(),source,plan,effect,now,adjusted);else if constexpr(skill==254)return npc_proposal->apply_vigorous_spirit(npc_proposal->lifetime(),source,plan,effect,now,adjusted);else if constexpr(skill==245)return npc_proposal->apply_protective_spirit(npc_proposal->lifetime(),source,plan,effect,now,adjusted);else if constexpr(skill==259)return npc_proposal->apply_shield_deflection(npc_proposal->lifetime(),source,plan,effect,now,adjusted);else if constexpr(skill==261)return npc_proposal->apply_shield_regeneration(npc_proposal->lifetime(),source,plan,effect,now,adjusted);else return npc_proposal->apply_shield_judgment(npc_proposal->lifetime(),source,plan,effect,now,adjusted);}();
            if(!applied)return false;
            if(divine_rank){const auto allowed=npc_proposal->healing_allowed(npc_proposal->lifetime());
                if(!allowed)return false;
                if(*allowed){auto preview=*npc_proposal;if(!preview.heal_ally(preview.lifetime(),std::lround(3.2*divine_rank),now))return false;}}
            entry=*npc_proposal->hex_effect(npc_proposal->lifetime(),effect);
            for(const auto& old:previous->active(target,now))if(old.skill==skill){statuses.push_back(health_effect_remove(target.actor,old.instance));previous_effects.push_back(old.instance);}
        }
        return true;
        };
        if(!prepare())return false;
        if(completion_event){
            if(!complete_ether_renewal_locked(source,skill,*completion_event,now,true))return false;
            if(caster.precise_health<=0){
                const std::vector<Packet> prefix{combat::detail::integer(58,source.actor,0),*visual};
                for(auto& [id,peer]:players_)peer.outbox->enqueue(prefix);
                return true;
            }
            // Source healing can end effects. Never restore its pre-reward
            // registry or regeneration snapshot when enchanting itself.
            if constexpr(skill==113||skill==307){
                // Source reward reactions can kill/retire another recipient.
                // Revalidate the exact lifetime and rebuild after those reactions.
                if(!valid_ally_enchantment_target_locked(source,target,0,now)){
                    for(auto&[id,peer]:players_)peer.outbox->enqueue({combat::detail::integer(58,source.actor,0),*visual});
                    return true;
                }
                npc=npc_combat_.find(target.actor);
                if(!prepare())throw std::logic_error("validated Tainted Flesh changed during Renewal settlement");
            }else if(source==target&&!prepare())throw std::logic_error("validated self enchantment changed during Renewal settlement");
        }
        statuses.push_back(health_effect_add(entry,target.actor,now));
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;recipients.reserve(players_.size());
        std::set<const Outbox*> seen;for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;for(auto& [box,queue]:recipients)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        if(player!=players_.end()){
            auto& recipient=player->second;*recipient.effect_registry=std::move(*registry);*recipient.health_regeneration=std::move(*regeneration);++recipient.next_health_effect;
            std::erase_if(recipient.health_effects,[](const auto& item){return item.second.skill==skill;});recipient.health_effects.emplace(effect,entry);
        }else{
            npc->second=std::move(*npc_proposal);++next_npc_hex_;
            for(const auto& old:previous_effects)npc_simple_enchantment_credit_.erase({target.actor,old});
            npc_simple_enchantment_credit_.emplace(std::pair{target.actor,effect},target);
        }
        if constexpr(skill==307){
            for(const auto old:previous_effects)reversal_caster_credit_.erase({target,old});
            if(source!=target)reversal_caster_credit_[{target,effect}]=source;
        }
        if constexpr(skill==274)if(npc!=npc_combat_.end())npc_seed_casters_[target]={effect,source};
        if constexpr(skill==262){std::erase_if(judgment_credit_,[&](const auto& item){return item.second.target==target;});judgment_credit_[{target,effect}]={target,source,effect,caster.ordinary_level};prune_retired_judgment_casters_locked();}
        for(auto& [id,peer]:players_)peer.outbox->enqueue(statuses);
        if constexpr(skill==113){
            tainted_credit_[target.actor]={target,source,effect,caster.ordinary_level};
            if(player!=players_.end()){
                if(player->second.conditions.has(combat::conditions::Kind::disease,now))remove_condition_locked(player->second,combat::conditions::Kind::disease,now);
            }else publish_npc_fragility_locked(target.actor,npc->second);
        }
        commit_applications_locked(target,now);
        if(player!=players_.end()){
            auto& recipient=player->second;
            if(divine_rank&&recipient.precise_health>0){const auto before=recipient.precise_health;
                health_event_locked(recipient,static_cast<unsigned>(std::lround(3.2*divine_rank)),now,true);
                const auto gained=recipient.precise_health-before;if(gained>0)for(auto& [id,peer]:players_)
                    peer.outbox->enqueue({skills::particles::health_change_2022(target.actor,source.actor,gained,recipient.resources->max_health)});
                publish_health_locked(recipient);}
            if constexpr(skill==307)if(recipient.precise_health<=0)kill_locked(recipient,now);
            publish_status_locked(recipient,now);
        }else{
            npc=npc_combat_.find(target.actor);
            if(npc!=npc_combat_.end()&&npc->second.hex_scope()==target){const auto life=npc->second.lifetime();const auto* actor=npc->second.actor(life);
                if(divine_rank&&actor&&actor->health&&actor->health->current>0){const auto allowed=npc->second.healing_allowed(life);
                    if(allowed&&*allowed){const auto before=actor->health->current;
                        if(!npc->second.heal_ally(life,std::lround(3.2*divine_rank),now))throw std::logic_error("validated protection enchantment Divine Favor failed");
                        const auto& health=*npc->second.actor(life)->health;const auto gained=health.current-before;
                        for(auto& [id,peer]:players_){std::vector<Packet> packets{combat::detail::fraction(target.actor,health)};
                            if(gained>0)packets.push_back(skills::particles::health_change_2022(target.actor,source.actor,gained,health.maximum));peer.outbox->enqueue(packets);}}}
                if constexpr(skill==307){
                    // A real healing event can settle lethal Deep Wound debt.
                    // Retire its lifetime now; do not invent a damaging Hit or
                    // award the healer a kill. Death reactions may retire actors,
                    // so reacquire the scoped recipient before publication.
                    const auto* current=npc->second.actor(life);
                    if(current&&current->health&&current->health->current<=0){
                        const auto placed=npcs_.find(target.actor);
                        if(placed!=npcs_.end())record_npc_death_locked(static_cast<Player*>(nullptr),placed->second,npc->second,now);
                        npc=npc_combat_.find(target.actor);
                        if(npc!=npc_combat_.end()&&npc->second.hex_scope()==target){
                            npc->second.refresh_hexes(life,now);
                            publish_npc_simple_enchantment_ends_locked(npc->second);
                            prune_reversal_caster_credit_locked(now);
                        }
                    }
                }
                npc=npc_combat_.find(target.actor);
                if(npc!=npc_combat_.end()&&npc->second.hex_scope()==target)
                    for(auto& [id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,npc->second.actor(life)->effects)});
            }
        }
        captures.clear();
        const std::vector<Packet> prefix{combat::detail::integer(58,source.actor,0),*visual};
        for(auto& [box,queue]:recipients){
            auto batch=prefix;if constexpr(skill==113)batch.pop_back();
            batch.insert(batch.end(),std::make_move_iterator(queue.packets.begin()),std::make_move_iterator(queue.packets.end()));
            if constexpr(skill==113)batch.push_back(*visual); // Captured58 -> status -> body214.
            box->enqueue(std::move(batch));
        }
        return true;
    }
public:
    // Paid staging admits only reviewed healing recipients. An unknown/immune
    // NPC must fail before payment, even for a secondary Monk with no DF.
    bool valid_reversal_target(skills::effects::Scope source,skills::effects::Scope target,
            unsigned divine,std::uint64_t now){
        std::lock_guard lock(mutex_);
        if(!valid_ally_enchantment_target_locked(source,target,divine,now))return false;
        const auto npc=npc_combat_.find(target.actor);
        return npc==npc_combat_.end()||npc->second.healing_allowed(npc->second.lifetime()).value_or(false);
    }
    // Normal gateway remains disabled pending remaining consumer acceptance.
    bool complete_reversal_fortune(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::ReversalFortunePlan& plan,unsigned divine,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr,std::optional<std::uint64_t> completion_event={}){
        return complete_simple_enchantment<307>(source,target,plan,divine,now,adjusted,completion_event);
    }
    bool complete_tainted_flesh(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::TaintedFleshPlan& plan,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr,std::optional<std::uint64_t> completion_event={}){
        return complete_simple_enchantment<113>(source,target,plan,0,now,adjusted,completion_event);
    }
    bool valid_healing_seed_target(skills::effects::Scope source,skills::effects::Scope target,
            unsigned divine,std::uint64_t now){
        std::lock_guard lock(mutex_);
        if(source==target||!valid_ally_enchantment_target_locked(source,target,divine,now))return false;
        const auto npc=npc_combat_.find(target.actor);
        return npc==npc_combat_.end()||npc->second.healing_allowed(npc->second.lifetime()).has_value();
    }
    // Shared application stage; paid Runtime/gateway own costs and cast admission.
    bool complete_healing_seed(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::HealingSeedPlan& plan,unsigned divine,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr,std::optional<std::uint64_t> completion_event={}){
        return complete_simple_enchantment<274>(source,target,plan,divine,now,adjusted,completion_event);
    }
    bool complete_divine_spirit(skills::effects::Scope source,const skills::effects::DivineSpiritPlan& plan,unsigned divine,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr,std::optional<std::uint64_t> completion_event={}){return complete_simple_enchantment<310>(source,source,plan,divine,now,adjusted,completion_event);}
    // Staged cost service. Normal casting must still establish modifier order
    // and call this before payment with its actual scoped cost authority.
    std::optional<unsigned> divine_spirit_spell_cost(skills::effects::Scope source,unsigned skill,unsigned cost,std::uint64_t now){
        const auto* row=skills::find_skill(skill);if(!row)return {};
        std::lock_guard lock(mutex_);const auto p=players_.find(source.actor);
        if(p==players_.end()||p->second.effect_scope!=source||!p->second.effect_registry||now<p->second.health_updated||now<p->second.energy_updated||
           !advance_resource_event_locked(p->second,now)||p->second.precise_health<=0)return {};
        return skills::effects::divine_spirit_cost(*row,cost,*p->second.effect_registry,source,now);
    }
    // Trusted staged payment boundary. The caller must establish the order of
    // other modifiers before supplying cost;normal casting is not wired yet.
    // No quote survives unlocking:expiry/strip and debit share one authority.
    bool consume_divine_spirit_spell_energy(skills::effects::Scope source,unsigned skill,unsigned cost,std::uint64_t now){
        const auto* row=skills::find_skill(skill);if(!row||!row->id)return false;
        std::lock_guard lock(mutex_);const auto found=players_.find(source.actor);
        if(found==players_.end()||found->second.effect_scope!=source||!found->second.effect_registry||
           now<found->second.health_updated||now<found->second.energy_updated||
           !advance_resource_event_locked(found->second,now)||found->second.precise_health<=0)return false;
        const auto adjusted=skills::effects::divine_spirit_cost(*row,cost,*found->second.effect_registry,source,now);
        return adjusted&&consume_energy_locked(found->second,*adjusted,now);
    }

    bool complete_protective_spirit(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::ProtectiveSpiritPlan& plan,unsigned divine,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr,std::optional<std::uint64_t> completion_event={}){return complete_simple_enchantment<245>(source,target,plan,divine,now,adjusted,completion_event);}
    bool complete_shield_deflection(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::ShieldDeflectionPlan& plan,unsigned divine,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr,std::optional<std::uint64_t> completion_event={}){return complete_simple_enchantment<259>(source,target,plan,divine,now,adjusted,completion_event);}
    bool complete_shield_regeneration(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::ShieldRegenerationPlan& plan,unsigned divine,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr,std::optional<std::uint64_t> completion_event={}){return complete_simple_enchantment<261>(source,target,plan,divine,now,adjusted,completion_event);}
    bool complete_shield_judgment(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::ShieldJudgmentPlan& plan,unsigned divine,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr,std::optional<std::uint64_t> completion_event={}){return complete_simple_enchantment<262>(source,target,plan,divine,now,adjusted,completion_event);}
