// Included inside Instance. Recipient-owned weapon-spell transactions.
private:
    std::map<skills::effects::Scope,skills::effects::WeaponSpellChargeLedger> charged_weapon_spells_;
    bool weapon_spell_target_locked(skills::effects::Scope source,skills::effects::Scope target,const skills::effects::WeaponSpellPlan&plan,std::uint64_t now){
        if(!skills::effects::valid_weapon_spell(plan))return false;
        // Reject a future recipient before settling the caster.
        if(const auto p=players_.find(target.actor);p!=players_.end()){
            if(p->second.effect_scope!=target||p->second.health_updated>now)return false;
        }else{
            const auto n=npc_combat_.find(target.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=target||n->second.health_clock(n->second.lifetime()).value_or(0)>now)return false;
            const auto type=n->second.creature_type(n->second.lifetime());if(!type||*type=="Spirit"||*type=="spirit")return false;
        }
        return valid_ally_enchantment_target_locked(source,target,0,now);
    }
public:
    bool valid_weapon_spell_target(skills::effects::Scope source,skills::effects::Scope target,const skills::effects::WeaponSpellPlan&plan,std::uint64_t now){
        std::lock_guard lock(mutex_);return weapon_spell_target_locked(source,target,plan,now);
    }
    bool valid_weapon_spell_target(skills::effects::Scope source,std::uint32_t actor,const skills::effects::WeaponSpellPlan&plan,std::uint64_t now){
        std::lock_guard lock(mutex_);skills::effects::Scope target;
        if(const auto p=players_.find(actor);p!=players_.end())target=p->second.effect_scope;
        else if(const auto n=npc_combat_.find(actor);n!=npc_combat_.end())target=n->second.hex_scope();else return false;
        return weapon_spell_target_locked(source,target,plan,now);
    }
    bool apply_weapon_spell(skills::effects::Scope source,skills::effects::Scope target,const skills::effects::WeaponSpellPlan&plan,std::uint64_t now,std::vector<Packet>*visuals=nullptr,std::function<void()>*publication=nullptr){
        std::lock_guard lock(mutex_);if(visuals)visuals->clear();if(publication)*publication={};
        if(!skills::effects::valid_weapon_spell(plan)||now>UINT64_MAX-plan.duration||!weapon_spell_target_locked(source,target,plan,now))return false;
        const auto source_player=players_.find(source.actor);if(source_player==players_.end()||source_player->second.effect_scope!=source)return false;
        struct Publication{bool sent=false;std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>>recipients;
            void send(){if(std::exchange(sent,true))return;for(auto&[box,queue]:recipients)box->enqueue(std::move(queue.packets));}};
        std::shared_ptr<Publication>pending;std::vector<std::unique_ptr<Outbox::Capture>>captures;
        if(publication){pending=std::make_shared<Publication>();std::set<const Outbox*>seen;
            for(auto&[id,peer]:players_)if(seen.insert(peer.outbox.get()).second)pending->recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
            for(auto&[box,queue]:pending->recipients)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));}
        auto charged=charged_weapon_spells_;
        const auto prepare_charges=[&](const skills::effects::Registry&registry,const skills::effects::Entry&entry){
            const auto current=charged.find(target);
            // Finite-charge admission is added only together with its consumers.
            if(plan.skill==792||plan.skill==2868){
                const auto canonical=skills::effects::charged_weapon_spell(plan.skill,plan.rank,plan.primary,plan.spawning);if(!canonical)return false;
                auto slot=charged.try_emplace(target,target).first;
                return slot->second.attach({source,target,entry,*canonical},registry,now);
            }
            return current==charged.end()||current->second.detach(target,now);
        };
        unsigned effect=0;std::vector<unsigned>removed;
        if(const auto p=players_.find(target.actor);p!=players_.end()){
            auto&recipient=p->second;if(!recipient.effect_registry||!recipient.health_regeneration||recipient.next_health_effect>=UINT32_MAX)return false;
            effect=static_cast<unsigned>(recipient.next_health_effect);
            const skills::effects::Entry entry{effect,source.actor,plan.skill,plan.rank,skills::effects::Category::weapon_spell,now,now+plan.duration};
            auto registry=*recipient.effect_registry;auto timers=*recipient.health_regeneration;
            const auto changed=registry.apply(target,entry,skills::effects::Replace::category);if(!changed.accepted)return false;
            for(const auto&old:changed.removed){timers.remove(target,old.instance);removed.push_back(old.instance);}
            if(!timers.apply(target,{effect,source.actor,plan.skill,0,now,now+plan.duration},false)||!prepare_charges(registry,entry))return false;
            for(const auto id:removed)recipient.health_effects.erase(id);
            *recipient.effect_registry=std::move(registry);recipient.health_regeneration=std::move(timers);recipient.health_effects.emplace(effect,entry);
            recipient.weapon_spell=skills::effects::OwnedWeaponSpell{source,target,entry,plan};++recipient.next_health_effect;
            recipient.effects=recipient.effect_registry->presentation(target,recipient.effects,now);
            commit_applications_locked(target,now);publish_status_locked(recipient,now);
        }else{
            auto n=npc_combat_.find(target.actor);if(n==npc_combat_.end()||next_npc_hex_==UINT32_MAX)return false;
            const auto registry=n->second.owned_effect_snapshot(n->second.lifetime());if(!registry)return false;
            for(const auto&e:registry->active(target,now))if(e.kind==skills::effects::Category::weapon_spell)removed.push_back(e.instance);
            auto proposal=n->second;effect=next_npc_hex_;
            if(!proposal.apply_weapon_spell(proposal.lifetime(),source,plan,effect,now))return false;
            const auto proposed_registry=proposal.owned_effect_snapshot(proposal.lifetime());
            const auto proposed_spell=proposal.weapon_spell_snapshot(proposal.lifetime(),now);
            if(!proposed_registry||!proposed_spell||!prepare_charges(*proposed_registry,proposed_spell->entry))return false;
            n->second=std::move(proposal);++next_npc_hex_;commit_applications_locked(target,now);
            for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,n->second.actor(n->second.lifetime())->effects)});
        }
        charged_weapon_spells_.swap(charged);
        std::vector<Packet>packets;for(unsigned old:removed)packets.push_back(health_effect_remove(target.actor,old));
        const auto body=[](unsigned actor,unsigned resource){Packet p;u16(p,158);u32(p,21);u32(p,actor);u32(p,resource);return p;};
        const auto*r=skills::find_skill(plan.skill);if(r->caster_body_resource!=2077)packets.push_back(body(source.actor,r->caster_body_resource));
        if(r->target_body_resource!=2077)packets.push_back(body(target.actor,r->target_body_resource));packets.push_back(body(target.actor,1155));
        packets.push_back(health_effect_add({effect,source.actor,plan.skill,plan.rank,skills::effects::Category::weapon_spell,now,now+plan.duration},target.actor,now));
        if(visuals)*visuals=packets;else for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
        captures.clear();
        if(publication)defer_or_publish_completion_locked(publication,[pending]{pending->send();});return true;
    }
private:
