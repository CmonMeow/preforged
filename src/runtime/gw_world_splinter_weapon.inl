// Included inside Instance. Finite-charge secondary contact transactions.
private:
    struct SplinterContact {
        skills::effects::OwnedWeaponSpell spell;
        navigation::Point center;unsigned plane=0;skills::effects::Scope primary;std::uint64_t at=0;bool deferred=false;
    };
    std::map<std::uint64_t,SplinterContact> splinter_contacts_;
    bool splinter_retrying_=false;
    std::uint64_t splinter_late_refusals_=0;
    bool splinter_foe_locked(skills::effects::Scope source,skills::effects::Scope target)const{
        if(source==target)return false;
        const auto player=players_.find(source.actor),recipient=players_.find(target.actor);
        if(player!=players_.end()&&player->second.effect_scope==source){
            if(recipient!=players_.end())return recipient->second.effect_scope==target&&player->second.team_token&&recipient->second.team_token&&player->second.team_token!=recipient->second.team_token;
            const auto npc=npc_combat_.find(target.actor);return npc!=npc_combat_.end()&&npc->second.hex_scope()==target&&player_npc_attack_relation_locked(player->second,npc->second).value_or(false);
        }
        const auto npc=npc_combat_.find(source.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=source)return false;
        if(recipient!=players_.end())return recipient->second.effect_scope==target&&npc->second.hostile_to_players(npc->second.lifetime());
        return valid_friendly_ordinary_locked(source,target);
    }
    std::uint64_t splinter_attack_observer_frontier_locked(skills::effects::Scope source,skills::effects::Scope primary,std::uint64_t observed)const{
        const auto spell=owned_weapon_spell_locked(source,observed);
        if(!spell||(spell->plan.skill!=792&&spell->plan.skill!=2868))return observed;
        navigation::Point center;unsigned plane=0;bool moving_center=false;
        if(const auto p=players_.find(primary.actor);p!=players_.end()){
            if(p->second.effect_scope!=primary)return observed;
            const auto point=healing_position_locked(p->second,observed);if(!point)return observed;
            center=*point;plane=p->second.surface.plane;
        }else{const auto n=npc_combat_.find(primary.actor);const auto placed=npcs_.find(primary.actor);
            if(n==npc_combat_.end()||n->second.hex_scope()!=primary||placed==npcs_.end())return observed;
            center={placed->second.x,placed->second.y};plane=placed->second.plane;
            const auto route=quest_npc_motion_.find(primary.actor);moving_center=route!=quest_npc_motion_.end()&&!route->second.route.empty();}
        auto frontier=observed;
        for(const auto&[id,p]:players_){
            if(p.effect_scope==primary||(!moving_center&&p.surface.plane!=plane)||!splinter_foe_locked(source,p.effect_scope))continue;
            const auto motion=std::max(p.last_move,p.steering?p.steering_tick:0);
            // A newer motion record has no retained historical position. Do
            // not infer prior nonadjacency from the latest far-away point.
            if(motion>observed){frontier=std::max({frontier,motion,p.health_updated,p.energy_updated});continue;}
            const auto point=healing_position_locked(p,observed);
            if(moving_center||(point&&std::hypot(point->x-center.x,point->z-center.z)<=166))frontier=std::max({frontier,p.health_updated,p.energy_updated});
        }
        for(const auto&[id,n]:npc_combat_){
            const auto scope=n.hex_scope();const auto placed=npcs_.find(id);
            if(scope==primary||placed==npcs_.end()||(!moving_center&&placed->second.plane!=plane)||!splinter_foe_locked(source,scope))continue;
            const auto motion=quest_npc_motion_.find(id);
            const bool moving=motion!=quest_npc_motion_.end()&&!motion->second.route.empty();
            const bool future=motion!=quest_npc_motion_.end()&&motion->second.updated>observed;
            if(moving_center||moving||future||std::hypot(placed->second.x-center.x,placed->second.y-center.z)<=166){
                const auto life=n.lifetime();frontier=std::max({frontier,n.health_clock(life).value_or(0),n.energy_clock(life).value_or(0),motion==quest_npc_motion_.end()?0:motion->second.updated});
            }
        }
        return frontier;
    }
    template<class Controller,class Release>
    std::uint64_t npc_ordinary_observer_frontier_locked(const Controller&ai,const Release&shot)const{
        auto at=std::max(shot.arrives,ai.ledger.clock());
        const auto observe=[&](skills::effects::Scope scope){
            if(const auto p=players_.find(scope.actor);p!=players_.end()&&p->second.effect_scope==scope){
                at=std::max({at,p->second.health_updated,p->second.energy_updated,p->second.last_move,p->second.steering?p->second.steering_tick:0});
            }else if(const auto n=npc_combat_.find(scope.actor);n!=npc_combat_.end()&&n->second.hex_scope()==scope){
                const auto life=n->second.lifetime();at=std::max({at,n->second.health_clock(life).value_or(0),n->second.energy_clock(life).value_or(0)});
                if(const auto motion=quest_npc_motion_.find(scope.actor);motion!=quest_npc_motion_.end())at=std::max(at,motion->second.updated);
            }
        };
        observe(ai.source);observe(shot.attack.target);
        if(const auto ledger=attack_outcomes_.find(ai.source);ledger!=attack_outcomes_.end())at=std::max(at,ledger->second.clock());
        return splinter_attack_observer_frontier_locked(ai.source,shot.attack.target,at);
    }
    std::optional<skills::effects::ChargedHealthFact> splinter_health_locked(skills::effects::Scope scope)const{
        if(const auto p=players_.find(scope.actor);p!=players_.end()){
            if(p->second.effect_scope!=scope||!p->second.resources)return {};
            // Charge admission anchors the attacker/primary observation only.
            // Shared damage commits separately own Health and protection state.
            return skills::effects::ChargedHealthFact{p->second.precise_health,double(p->second.resources->max_health),p->second.health_updated,1};
        }
        const auto n=npc_combat_.find(scope.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=scope)return {};
        const auto life=n->second.lifetime();const auto*body=n->second.actor(life);const auto revision=n->second.health_loss_revision(life);
        if(!body||!body->health||!revision||*revision==UINT64_MAX)return {};
        return skills::effects::ChargedHealthFact{body->health->current,double(body->health->maximum),n->second.health_clock(life).value_or(0),*revision+1};
    }
    std::optional<std::vector<skills::effects::Scope>> splinter_candidates_locked(const SplinterContact& contact,skills::effects::Scope primary,std::uint64_t now){
        std::vector<skills::effects::Scope> candidates;
        for(auto&[id,p]:players_){
            if(p.effect_scope==primary||!p.resources||p.surface.plane!=contact.plane||!splinter_foe_locked(contact.spell.target,p.effect_scope))continue;
            const auto point=healing_position_locked(p,now);if(!point)return {};
            if(std::hypot(point->x-contact.center.x,point->z-contact.center.z)>166)continue;
            if(p.health_updated>now||p.energy_updated>now||!advance_health_locked(p,now))return {};
            if(p.precise_health>0)candidates.push_back(p.effect_scope);
        }
        std::vector<unsigned> npc_ids;for(const auto& [id,n]:npc_combat_)npc_ids.push_back(id);
        for(const auto id:npc_ids){
            auto found=npc_combat_.find(id);auto placed=npcs_.find(id);
            if(found==npc_combat_.end())continue;const auto scope=found->second.hex_scope();
            if(scope==primary||placed==npcs_.end()||placed->second.plane!=contact.plane||!splinter_foe_locked(contact.spell.target,scope))continue;
            refresh_npc_movement_locked(id,now);
            placed=npcs_.find(id);found=npc_combat_.find(id);if(placed==npcs_.end()||found==npc_combat_.end()||found->second.hex_scope()!=scope)continue;
            if(std::hypot(placed->second.x-contact.center.x,placed->second.y-contact.center.z)>166)continue;
            const auto life=found->second.lifetime();if(found->second.health_clock(life).value_or(0)>now||found->second.energy_clock(life).value_or(0)>now)return {};
            advance_npc_conditions_locked(id,found->second,now);found=npc_combat_.find(id);
            if(found==npc_combat_.end()||found->second.hex_scope()!=scope)continue;
            const auto*body=found->second.actor(life);
            if(body&&body->damageable&&body->health&&body->health->current>0)candidates.push_back(scope);
        }
        // Explicit deterministic project policy; captures prove four recipients,
        // not the retail tie-break order among more than four adjacent foes.
        std::sort(candidates.begin(),candidates.end());if(candidates.size()>4)candidates.resize(4);return candidates;
    }
    bool preflight_splinter_damage_locked(skills::effects::Scope source,skills::effects::Scope scope,const combat::Damage& damage,std::uint64_t now){
        if(const auto found=players_.find(scope.actor);found!=players_.end()){
            auto& p=found->second;if(p.effect_scope!=scope||!p.resources||p.precise_health<=0)return true;
            auto actor=incoming_spell_actor_locked(p,{scope.actor,scope.generation,combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true},now);
            actor.incoming_reduction+=p.absorption;
            const auto hit=combat::apply(actor,damage,true);
            return hit.reason==combat::Reason::accepted&&bool(prepare_player_reactive_seed_locked(p,hit,now));
        }
        const auto found=npc_combat_.find(scope.actor);
        if(found==npc_combat_.end()||found->second.hex_scope()!=scope)return true;
        const auto* actor=found->second.actor(found->second.lifetime());if(!actor||!actor->health||actor->health->current<=0)return true;
        auto preview=found->second;const bool owned=!preview.hostile_to_players(preview.lifetime());
        if(owned){const auto caster=players_.find(source.actor);if(caster==players_.end()||caster->second.effect_scope!=source||!player_npc_attack_relation_locked(caster->second,preview).value_or(false))return false;}
        return preflight_npc_reversal_or_damage_locked(preview,damage,now,nullptr,nullptr,nullptr,nullptr,owned);
    }
    bool preflight_splinter_payload_locked(const SplinterContact&contact,const combat::weapon::QualifiedHit&hit){
        const auto plan=skills::effects::charged_weapon_spell(contact.spell.plan.skill,contact.spell.plan.rank,contact.spell.plan.primary,contact.spell.plan.spawning);
        const auto ledger=charged_weapon_spells_.find(hit.attacker);if(!plan||ledger==charged_weapon_spells_.end())return false;
        skills::effects::ChargedSpellSnapshot snapshot(hit.attacker);snapshot.ledger=ledger->second;
        if(const auto source=players_.find(hit.attacker.actor);source!=players_.end()){
            const auto&player=source->second;if(!player.effect_registry||!player.health_regeneration)return false;snapshot.registry=*player.effect_registry;
            const auto effect=player.health_effects.find(contact.spell.entry.instance);const auto timer=player.health_regeneration->lookup(hit.attacker,contact.spell.entry.instance);
            if(effect==player.health_effects.end()||!skills::effects::charged_entry_equal(effect->second,contact.spell.entry)||!timer||
                timer->skill!=plan->skill||timer->cause!=contact.spell.source.actor||timer->pips||timer->begins!=contact.spell.entry.applied||timer->ends!=contact.spell.entry.expires)return false;
        }else{const auto npc_source=npc_combat_.find(hit.attacker.actor);if(npc_source==npc_combat_.end())return false;
            const auto registry=npc_source->second.owned_effect_snapshot(npc_source->second.lifetime());if(!registry)return false;snapshot.registry=*registry;}
        for(const auto scope:{hit.attacker,hit.target}){const auto fact=splinter_health_locked(scope);if(!fact||!fact->valid()||fact->at>hit.at)return false;snapshot.health.emplace(scope,*fact);}
        const skills::effects::OwnedChargedWeaponSpell owned{contact.spell.source,hit.attacker,contact.spell.entry,*plan};
        const combat::weapon::ResolvedAttack event{hit.attacker,hit.target,hit.event,hit.released,hit.at,AttackOutcome::hit,hit.at,hit.delivery};
        if(!skills::effects::ChargeProposal::prepare(snapshot,owned,event,skills::effects::ChargeConsumption::consume,{}))return false;
        unsigned level=0;if(const auto source=players_.find(hit.attacker.actor);source!=players_.end())level=source->second.ordinary_level;
        else if(const auto source=npcs_.find(hit.attacker.actor);source!=npcs_.end())level=source->second.definition.level;
        if(!level||level>255)return false;
        const combat::Damage damage{combat::Kind::armor_ignoring,double(plan->amount),level,plan->skill};
        const auto candidates=splinter_candidates_locked(contact,hit.target,hit.at);if(!candidates)return false;
        for(const auto scope:*candidates){
            if(!preflight_splinter_damage_locked(hit.attacker,scope,damage,hit.at))return false;
        }
        return true;
    }
    bool prepare_splinter_contact_locked(const combat::weapon::QualifiedHit&hit){
        const auto spell=owned_weapon_spell_locked(hit.attacker,hit.at);
        if(!spell||(spell->plan.skill!=792&&spell->plan.skill!=2868))return true;
        const auto ledger=charged_weapon_spells_.find(hit.attacker);
        if(ledger==charged_weapon_spells_.end()||!ledger->second.remaining()||ledger->second.receipts()>=4096||splinter_contacts_.size()>=4096||splinter_contacts_.contains(hit.event))return false;
        SplinterContact contact;contact.spell=*spell;contact.primary=hit.target;contact.at=hit.at;
        if(const auto target=players_.find(hit.target.actor);target!=players_.end()){
            if(target->second.effect_scope!=hit.target)return false;const auto point=healing_position_locked(target->second,hit.at);if(!point)return false;
            contact.center=*point;contact.plane=target->second.surface.plane;
        }else{
            const auto npc_target=npc_combat_.find(hit.target.actor);const auto placed=npcs_.find(hit.target.actor);
            if(npc_target==npc_combat_.end()||npc_target->second.hex_scope()!=hit.target||placed==npcs_.end())return false;
            refresh_npc_movement_locked(hit.target.actor,hit.at);contact.center={placed->second.x,placed->second.y};contact.plane=placed->second.plane;
        }
        if(!preflight_splinter_payload_locked(contact,hit))return false;
        return splinter_contacts_.emplace(hit.event,std::move(contact)).second;
    }
    // false means retained delivery, never acknowledgement. The adapter must
    // retry buffered outcomes before sealing their consumer completion frontier.
    bool finish_splinter_contact_locked(const combat::weapon::ResolvedAttack&event){
        const auto receipt=splinter_contacts_.find(event.event);if(receipt==splinter_contacts_.end())return true;
        const auto contact=receipt->second;
        const auto no_effect=[&]{splinter_contacts_.erase(event.event);return true;};
        if(event.outcome!=AttackOutcome::hit||!event.before_damage||event.attacker!=contact.spell.target||event.target!=contact.primary||event.at!=contact.at)return no_effect();
        const auto current=owned_weapon_spell_locked(event.attacker,event.at);
        if(!current||current->source!=contact.spell.source||!skills::effects::charged_entry_equal(current->entry,contact.spell.entry)||current->plan!=contact.spell.plan)return no_effect();
        const auto plan=skills::effects::charged_weapon_spell(current->plan.skill,current->plan.rank,current->plan.primary,current->plan.spawning);if(!plan)return false;
        unsigned level=0;
        if(const auto source=players_.find(event.attacker.actor);source!=players_.end())level=source->second.ordinary_level;
        else if(const auto source=npcs_.find(event.attacker.actor);source!=npcs_.end())level=source->second.definition.level;
        if(!level||level>255)return false;
        const combat::Damage damage{combat::Kind::armor_ignoring,double(plan->amount),level,plan->skill};
        const auto candidates=splinter_candidates_locked(contact,event.target,event.at);if(!candidates)return false;
        struct Recipient {skills::effects::Scope scope;bool npc;};std::vector<Recipient> recipients;
        for(const auto scope:*candidates){
            if(!preflight_splinter_damage_locked(event.attacker,scope,damage,event.at))return false;
            recipients.push_back({scope,!players_.contains(scope.actor)});
        }
        // Keep the established player-before-NPC base order. Shared reactions
        // may change subsequent recipients, which are always resolved fresh.
        std::stable_partition(recipients.begin(),recipients.end(),[](const auto& r){return !r.npc;});
        const auto refreshed=owned_weapon_spell_locked(event.attacker,event.at);
        if(!refreshed||refreshed->source!=current->source||!skills::effects::charged_entry_equal(refreshed->entry,current->entry)||refreshed->plan!=current->plan)return no_effect();
        const auto actual=charged_weapon_spells_.find(event.attacker);if(actual==charged_weapon_spells_.end())return false;
        skills::effects::ChargedSpellSnapshot snapshot(event.attacker);snapshot.ledger=actual->second;
        if(const auto source=players_.find(event.attacker.actor);source!=players_.end()){
            if(source->second.effect_scope!=event.attacker||!source->second.effect_registry)return false;snapshot.registry=*source->second.effect_registry;
        }else{const auto npc_source=npc_combat_.find(event.attacker.actor);if(npc_source==npc_combat_.end()||npc_source->second.hex_scope()!=event.attacker)return false;
            const auto registry=npc_source->second.owned_effect_snapshot(npc_source->second.lifetime());if(!registry)return false;snapshot.registry=*registry;}
        for(const auto scope:{event.attacker,event.target}){const auto fact=splinter_health_locked(scope);if(!fact||!fact->valid()||fact->at>event.at)return false;snapshot.health.emplace(scope,*fact);}
        const skills::effects::OwnedChargedWeaponSpell owned{current->source,event.attacker,current->entry,*plan};
        // Charge/event/final-removal authority only. Damage and protection
        // counters belong to shared commits, never aggregate Health snapshots.
        auto proposal=skills::effects::ChargeProposal::prepare(snapshot,owned,event,skills::effects::ChargeConsumption::consume,{});if(!proposal)return false;
        // Preflight exact final-charge removal on copies, including the actual
        // zero-pip timer and owned Health-effect mirror. Do not invoke generic
        // end callbacks after committing a secondary Health burst.
        Player*ending_player=nullptr;combat::npcs::LiveActor*ending_npc=nullptr;
        std::optional<skills::regeneration::State>ending_timers;
        std::map<unsigned,skills::effects::Entry>ending_health_effects;
        std::optional<combat::npcs::LiveActor::PreparedWeaponSpellRemoval>ending_actor;unsigned ending_effects=0;
        std::vector<Packet>ending_packets;
        const bool final_charge=actual->second.remaining()==1;
        auto ending_registry=snapshot.registry;
        if(final_charge){
            if(!ending_registry.remove(event.attacker,current->entry.instance))return false;
            ending_packets.push_back(health_effect_remove(event.attacker.actor,current->entry.instance));
            if(const auto p=players_.find(event.attacker.actor);p!=players_.end()){
                auto&source=p->second;if(!source.health_regeneration||!source.effect_registry)return false;
                const auto effect=source.health_effects.find(current->entry.instance);
                const auto timer=source.health_regeneration->lookup(event.attacker,current->entry.instance);
                if(effect==source.health_effects.end()||!skills::effects::charged_entry_equal(effect->second,current->entry)||!timer||
                   timer->skill!=plan->skill||timer->cause!=current->source.actor||timer->pips||timer->begins!=current->entry.applied||timer->ends!=current->entry.expires)return false;
                ending_timers=*source.health_regeneration;if(!ending_timers->remove(event.attacker,current->entry.instance))return false;
                ending_health_effects=source.health_effects;ending_health_effects.erase(current->entry.instance);
                ending_effects=ending_registry.presentation(event.attacker,source.effects,event.at);ending_player=&source;
            }else{
                const auto source=npc_combat_.find(event.attacker.actor);if(source==npc_combat_.end())return false;
                ending_actor=source->second.prepare_weapon_spell_removal(source->second.lifetime(),current->entry.instance,event.at);if(!ending_actor||!source->second.can_commit_weapon_spell_removal(*ending_actor))return false;
                ending_npc=&source->second;ending_effects=ending_actor->effects();
            }
            ending_packets.push_back(emotes::effects_2022(event.attacker.actor,ending_effects));
        }
        if(!proposal->commit(snapshot,event.at))return false; // Transaction-local copies only.
        // No callbacks between the charge/removal primitives below.
        actual->second.swap(snapshot.ledger);splinter_contacts_.erase(event.event);
        if(ending_player){
            using std::swap;static_assert(std::is_nothrow_swappable_v<skills::regeneration::State>);
            swap(*ending_player->effect_registry,snapshot.registry);swap(*ending_player->health_regeneration,*ending_timers);
            ending_player->health_effects.swap(ending_health_effects);ending_player->weapon_spell.reset();ending_player->effects=ending_effects;
        }
        if(ending_npc&&!ending_npc->commit_weapon_spell_removal(*ending_actor))throw std::logic_error("preflighted Splinter removal commit failed");
        for(auto&[id,peer]:players_)if(!ending_packets.empty())peer.outbox->enqueue(ending_packets);
        const auto cause=[&]()->std::optional<unsigned>{
            const auto source=players_.find(event.attacker.actor);
            if(source!=players_.end()&&source->second.effect_scope==event.attacker)return event.attacker.actor;
            return npc_damage_attribution_locked(event.attacker);
        };
        const auto fx=[&](unsigned target){Packet packet{159,0};combat::detail::put(packet,20);combat::detail::put(packet,target);combat::detail::put(packet,cause().value_or(0));combat::detail::put(packet,1152);for(auto&[id,peer]:players_)peer.outbox->enqueue({packet});};
        fx(event.target.actor);
        for(const auto& recipient:recipients){
            const auto scope=recipient.scope;std::optional<combat::Hit> hit;
            if(!recipient.npc){
                const auto found=players_.find(scope.actor);
                if(found==players_.end()||found->second.effect_scope!=scope||!found->second.resources||found->second.precise_health<=0)continue;
                auto& target=found->second;
                auto actor=incoming_spell_actor_locked(target,{scope.actor,scope.generation,combat::Health{target.precise_health,target.resources->max_health},{},target.effects,true},event.at);
                actor.incoming_reduction+=target.absorption;
                const auto incoming=combat::apply(actor,damage,true);
                if(incoming.reason!=combat::Reason::accepted||!commit_player_reactive_hit_locked(target,incoming,cause(),event.at,true,nullptr,false,false,&hit,event.attacker)){
                    ++splinter_late_refusals_;std::cerr<<"Splinter late damage refusal event="<<event.event<<" actor="<<scope.actor<<" reason=player_protection_commit"<<std::endl;continue;
                }
            }else{
                const auto found=npc_combat_.find(scope.actor);const auto placed=npcs_.find(scope.actor);
                if(found==npc_combat_.end()||placed==npcs_.end()||found->second.hex_scope()!=scope)continue;
                const auto* body=found->second.actor(found->second.lifetime());if(!body||!body->health||body->health->current<=0)continue;
                const auto source=players_.find(event.attacker.actor);
                Player* credit=source!=players_.end()&&source->second.effect_scope==event.attacker?&source->second:nullptr;
                const bool owned=!found->second.hostile_to_players(found->second.lifetime());
                if(owned&&(!credit||!player_npc_attack_relation_locked(*credit,found->second).value_or(false)))continue;
                if(!commit_npc_reversal_or_damage_locked(credit,placed->second,found->second,damage,event.at,nullptr,nullptr,&hit,true,event.attacker,nullptr,owned)){
                    ++splinter_late_refusals_;std::cerr<<"Splinter late damage refusal event="<<event.event<<" actor="<<scope.actor<<" reason=npc_protection_commit"<<std::endl;continue;
                }
            }
            fx(scope.actor);
            if(hit&&hit->applied>0){
                mark_of_pain_reaction_locked(scope,0u,hit->applied,event.at);
                if(const auto p=players_.find(scope.actor);p!=players_.end()&&p->second.effect_scope==scope){
                    if(p->second.precise_health>0)barbs_reaction_locked(scope,0u,hit->applied,event.at);
                }else if(const auto n=npc_combat_.find(scope.actor);n!=npc_combat_.end()&&n->second.hex_scope()==scope&&n->second.actor(n->second.lifetime())->health->current>0)
                    barbs_reaction_locked(scope,0u,hit->applied,event.at);
            }
            if(!recipient.npc){
                const auto found=players_.find(scope.actor);
                if(found!=players_.end()&&found->second.effect_scope==scope&&found->second.precise_health<=0)kill_locked(found->second,event.at);
            }else{
                const auto found=npc_combat_.find(scope.actor);const auto placed=npcs_.find(scope.actor);const auto source=players_.find(event.attacker.actor);
                if(found!=npc_combat_.end()&&placed!=npcs_.end()&&found->second.hex_scope()==scope&&found->second.actor(found->second.lifetime())->health->current<=0)
                    record_npc_death_locked(source!=players_.end()&&source->second.effect_scope==event.attacker?&source->second:nullptr,placed->second,found->second,event.at);
            }
        }
        // The charge/contact is committed before callbacks. A late recipient
        // refusal completes this delivery; it must never replay other hits.
        return true;
    }

    void retire_splinter_receipts_locked(skills::effects::Scope source){
        const auto ledger=charged_weapon_spells_.find(source);const auto delivery=attack_outcomes_.find(source);
        if(ledger!=charged_weapon_spells_.end()&&delivery!=attack_outcomes_.end())
            if(const auto frontier=delivery->second.completion_frontier(source))
                if(!ledger->second.retire_receipts(*frontier))throw std::logic_error("Splinter consumer frontier rejected");
    }
    void retry_splinter_contacts_locked(std::uint64_t now){
        if(splinter_retrying_)return;
        struct Reset{bool&active;~Reset(){active=false;}}reset{splinter_retrying_};splinter_retrying_=true;
        std::vector<std::pair<skills::effects::Scope,std::uint64_t>>pending;
        for(const auto&[event,contact]:splinter_contacts_)if(contact.deferred&&contact.at<=now)pending.emplace_back(contact.spell.target,event);
        for(const auto&[source,id]:pending){
            const auto owner=attack_outcomes_.find(source);if(owner==attack_outcomes_.end()){splinter_contacts_.erase(id);continue;}
            const auto buffered=owner->second.buffered(source);if(!buffered)continue;
            const auto event=std::find_if(buffered->begin(),buffered->end(),[&](const auto&event){return event.event==id;});
            if(event==buffered->end()){splinter_contacts_.erase(id);continue;}
            if(!finish_splinter_contact_locked(*event))continue;
            if(!owner->second.acknowledge(source,*event))throw std::logic_error("Splinter deferred acknowledgement failed");
            retire_splinter_receipts_locked(source);
        }
    }
    void retire_splinter_scope_locked(skills::effects::Scope retired){
        std::erase_if(splinter_contacts_,[&](const auto&entry){return entry.second.spell.target==retired||entry.second.primary==retired;});
        for(auto&[source,delivery]:attack_outcomes_){
            if(source==retired)continue;const auto buffered=delivery.buffered(source);if(!buffered)continue;
            for(const auto&event:*buffered)if(event.target==retired){
                if(!delivery.acknowledge(source,event))throw std::logic_error("retired Splinter target acknowledgement failed");
            }
            retire_splinter_receipts_locked(source);
        }
    }

public:
    std::uint64_t splinter_late_refusals()const{std::lock_guard lock(mutex_);return splinter_late_refusals_;}
private:
