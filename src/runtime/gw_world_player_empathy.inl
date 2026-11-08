// Staged common World consumer. No Session-owned damage or raw event identity.
private:
    std::map<skills::effects::Scope,BackfireCredit> player_empathy_credit_;
    // A refusal here is not an attack made: validate the existing reactive
    // protection/Seed cohort before consuming the release observer. Its existing
    // preflight may settle causal resource frontiers but commits no attack loss.
    bool player_empathy_reaction_ready_locked(skills::effects::Scope attacker,std::uint64_t now){
        const auto found=players_.find(attacker.actor);if(found==players_.end())return true;
        auto& player=found->second;if(player.effect_scope!=attacker)return false;
        if(!player.effect_registry||player.precise_health<=0)return true;
        const auto entry=player.effect_registry->newest_skill(attacker,26,now,3151);if(!entry)return true;
        const auto credit=player_empathy_credit_.find(attacker);
        if(credit==player_empathy_credit_.end()||credit->second.target!=attacker||credit->second.effect!=entry->instance)return true;
        if(credit->second.caster.instance!=attacker.instance||!credit->second.caster.actor||!credit->second.caster.generation||!credit->second.level||credit->second.level>255)return false;
        const auto plan=skills::effects::empathy(entry->skill,entry->rank);
        if(!plan||!player.resources||now<player.health_updated)return false;
        auto actor=incoming_spell_actor_locked(player,{player.id,attacker.generation,combat::Health{player.precise_health,player.resources->max_health},{},player.effects,true},now);
        const auto hit=combat::apply(actor,{combat::Kind::armor_ignoring,double(plan->damage),credit->second.level,entry->skill},true);
        return hit.reason==combat::Reason::accepted&&preflight_player_reversal_locked(player,hit,now)&&bool(prepare_player_reactive_seed_locked(player,hit,now));
    }
    bool player_empathy_release_locked(skills::effects::Scope attacker,skills::effects::Scope target,std::uint64_t event,std::uint64_t now,bool react=true){
        const auto found=players_.find(attacker.actor);if(found==players_.end())return true; // existing NPC consumer owns NPC reactions
        auto& player=found->second;
        if(player.effect_scope!=attacker||!event)return false;
        const auto ledger=attack_outcomes_.find(attacker);if(ledger==attack_outcomes_.end())return false;
        constexpr unsigned empathy_observer=1;
        const auto observed=ledger->second.release_observed(attacker,target,event,now,empathy_observer);
        if(!observed)return false;if(*observed)return true;
        if(react&&!player_empathy_reaction_ready_locked(attacker,now))return false;
        const auto first=ledger->second.observe_release_once(attacker,target,event,now,empathy_observer);
        if(!first)return false;
        if(*first)mark_dagger_physical_release_draft_locked(attacker,target,event,now);
        if(!*first||!react)return true;
        return commit_player_empathy_reaction_locked(attacker,now);
    }
    // Called only AFTER the canonical contact/action observer is consumed.
    bool commit_player_empathy_reaction_locked(skills::effects::Scope attacker,std::uint64_t now){
        const auto found=players_.find(attacker.actor);if(found==players_.end()||found->second.effect_scope!=attacker)return false;
        auto& player=found->second;
        if(player.precise_health<=0||!player.effect_registry)return true;
        const auto entry=player.effect_registry->newest_skill(attacker,26,now,3151);if(!entry)return true;
        if(!player.resources||now<player.health_updated)return false;
        const auto credit=player_empathy_credit_.find(attacker);
        if(credit==player_empathy_credit_.end()||credit->second.target!=attacker||credit->second.effect!=entry->instance)return true;
        const auto plan=skills::effects::empathy(entry->skill,entry->rank);if(!plan)return false;
        auto actor=incoming_spell_actor_locked(player,{player.id,attacker.generation,combat::Health{player.precise_health,player.resources->max_health},{},player.effects,true},now);
        const auto hit=combat::apply(actor,{combat::Kind::armor_ignoring,double(plan->damage),credit->second.level,entry->skill},true);
        const auto seed=hit.reason==combat::Reason::accepted?prepare_player_reactive_seed_locked(player,hit,now):std::nullopt;
        if(!seed||!commit_player_reactive_hit_locked(player,hit,{},now,false,&*seed,npc_damage_attribution_locked(credit->second.caster).has_value(),true,nullptr,credit->second.caster))return false;
        return player.precise_health>0;
    }
    bool player_ordinary_release_ready_locked(Player& source,skills::effects::Scope target,const combat::weapon::Owned& weapon,std::uint64_t now){
        const auto owned=weapon_preparation_locked(source,weapon,now);
        const auto contact=owned?combat::weapon::preparation_contact_plan(*owned,source.effect_scope,weapon):std::nullopt;
        if(owned&&!contact)return false;
        if(contact&&!preparation_contact_ready_locked(target,*contact,source.ordinary_level,now,source.effect_scope))return false;
        const auto effective=combat::weapon::preparation_weapon_damage(weapon,contact);
        if(illusionary_attack_locked(source,effective,now))return true;
        if(const auto p=players_.find(target.actor);p!=players_.end())return p->second.effect_scope==target&&known_weapon_armor_locked(p->second,effective);
        const auto n=npc_combat_.find(target.actor);return n!=npc_combat_.end()&&n->second.hex_scope()==target&&bool(npc_weapon_armor_locked(n->second,effective,now));
    }
    double player_empathy_damage_locked(const Player& attacker,double damage,std::uint64_t now)const{
        if(!attacker.effect_registry)return damage;
        return skills::effects::empathy_attack_damage(*attacker.effect_registry,attacker.effect_scope,now,damage).value_or(damage);
    }
    void commit_empathy_player_registration_followon_locked(Player& victim,skills::effects::Registry registry,
            const skills::effects::Entry& entry,const std::vector<skills::effects::Entry>& removed,
            skills::effects::Scope caster,unsigned level,std::uint64_t now,bool npc_source=false){
        *victim.effect_registry=std::move(registry);++victim.next_health_effect;std::vector<Packet> packets;
        for(const auto& old:removed){victim.health_effects.erase(old.instance);packets.push_back(health_effect_remove(victim.id,old.instance));}
        victim.health_effects.emplace(entry.instance,entry);player_empathy_credit_[victim.effect_scope]={victim.effect_scope,caster,entry.instance,level};
        packets.push_back(health_effect_add(entry,victim.id,now));for(auto&[id,peer]:players_)if(!npc_source||peer.npc_publication_ready)peer.outbox->enqueue(packets);
        commit_applications_locked(victim.effect_scope,now);publish_status_locked(victim,now);publish_target_body_locked(victim.id,entry.skill,npc_source?0:caster.actor);
        if(victim.passive_recovery)victim.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
    }
public:
    bool empathy(skills::effects::Scope caster,skills::effects::Scope target,unsigned skill,unsigned rank,unsigned level,std::uint64_t now){
        const auto plan=skills::effects::empathy(skill,rank);
        if(!plan||!level||level>255||now>UINT64_MAX-plan->duration)return false;
        std::unique_lock lock(mutex_);
        if(!players_.contains(target.actor)){lock.unlock();return empathy_npc(caster,target,skill,rank,now);}
        const auto source=players_.find(caster.actor),found=players_.find(target.actor);
        if(source==players_.end())return apply_npc_empathy_player_followon_locked(caster,target,*plan,now,true,level);
        if(source==players_.end()||source->second.effect_scope!=caster||!source->second.resources||
           source->second.precise_health<=0||source->second.knocked_down_until>now||
           !advance_resource_event_locked(source->second,now)||source->second.precise_health<=0)return false;
        auto& victim=found->second;
        if(victim.effect_scope!=target||!victim.resources||!victim.effect_registry||!source->second.team_token||
           !victim.team_token||source->second.team_token==victim.team_token||victim.surface.plane!=source->second.surface.plane||
           victim.next_health_effect>=UINT32_MAX||!advance_health_locked(victim,now)||victim.precise_health<=0)return false;
        const auto a=healing_position_locked(source->second,now),b=healing_position_locked(victim,now);
        if(!a||!b||std::hypot(a->x-b->x,a->z-b->z)>1248)return false;
        if(navigation_){const auto line=layered_->trace(source->second.surface,*a,*b);if(!line||line->plane!=victim.surface.plane)return false;}
        const auto prevented=prevent_selected_hex_locked(source->second,victim,skill,now);if(!prevented)return false;if(*prevented)return true;
        auto registry=*victim.effect_registry;std::vector<skills::effects::Entry> removed;
        for(const auto& entry:registry.active(target,now))if(entry.skill==26||entry.skill==3151){if(!registry.remove(target,entry.instance))return false;removed.push_back(entry);}
        const skills::effects::Entry entry{static_cast<unsigned>(victim.next_health_effect),caster.actor,skill,rank,skills::effects::Category::hex,now,now+plan->duration};
        if(!registry.apply(target,entry,skills::effects::Replace::same_skill).accepted)return false;
        commit_empathy_player_registration_followon_locked(victim,std::move(registry),entry,removed,caster,level,now);
        if(source->second.passive_recovery)source->second.passive_recovery->record(recovery::Event::target_enemy_spell_or_signet,now);
        return true;
    }
private:
