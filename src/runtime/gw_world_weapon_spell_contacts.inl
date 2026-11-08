// Included inside Instance after owned preparation helpers. Contact receipts
// preserve the qualified attack event and pre-damage attacking target facts.
private:
    struct WailingContact{skills::effects::Scope source,target;std::uint64_t at=0,epoch=0;unsigned effect=0;
        std::optional<combat::npcs::control::Token> npc_action;bool protected_action=false;};
    std::map<std::uint64_t,WailingContact>wailing_contacts_;
    std::optional<skills::effects::OwnedWeaponSpell> owned_weapon_spell_locked(skills::effects::Scope owner,std::uint64_t now)const{
        if(const auto p=players_.find(owner.actor);p!=players_.end()){
            const auto&a=p->second;if(a.effect_scope!=owner||!a.resources||a.precise_health<=0||!a.weapon_spell||!a.effect_registry||!a.weapon_spell->active(*a.effect_registry,owner,now))return {};
            return a.weapon_spell;
        }
        const auto n=npc_combat_.find(owner.actor);return n!=npc_combat_.end()&&n->second.hex_scope()==owner?n->second.weapon_spell_snapshot(n->second.lifetime(),now):std::nullopt;
    }
    static bool weapon_target_attacking(const combat::weapon::AttackActionFacts&action){
        if(action.kind==combat::weapon::AttackTargetAction::attack)return true;
        const auto*r=action.skill?skills::find_skill(action.skill->skill):nullptr;return r&&r->type==14;
    }
    bool prepare_wailing_contact_locked(const combat::weapon::QualifiedHit&hit,bool independent_interrupt=false,bool synchronous_reservation=false){
        const auto spell=owned_weapon_spell_locked(hit.attacker,hit.at);
        if(!spell||spell->plan.kind!=skills::effects::WeaponSpellKind::wailing||independent_interrupt)return true;
        // A synchronous AttackResolution owns its provisional receipt until
        // its stack terminal (hit/miss/block/cancel). World's mutex serializes
        // these producers, so one scoped extra slot avoids refusing a possible
        // independent critical interrupt solely for a persistent-cache limit.
        // Persistent/asynchronous producers retain the existing4096 budget.
        const std::size_t limit=4096+(synchronous_reservation?1u:0u);
        if(wailing_contacts_.size()>=limit||wailing_contacts_.contains(hit.event))return false;
        WailingContact receipt{hit.attacker,hit.target,hit.at,0,spell->entry.instance};
        if(const auto p=players_.find(hit.target.actor);p!=players_.end()){
            auto&target=p->second;if(target.effect_scope!=hit.target||target.health_updated>hit.at||!target.resources||target.precise_health<=0)return false;
            if(!weapon_target_attacking(player_attack_action_locked(target,hit.at)))return true;
            if(target.control_epoch==UINT64_MAX||target.attack_interrupt_epoch==UINT64_MAX||!preflight_interrupt_protection_locked(target,hit.at))return false;
            receipt.epoch=target.control_epoch;
        }else{
            const auto n=npc_combat_.find(hit.target.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=hit.target)return false;
            auto&target=n->second;const auto action=target.active_action(target.lifetime(),hit.at);if(!action)return true;
            combat::weapon::AttackActionFacts facts;facts.kind=action->kind==combat::npcs::control::Action::attack?combat::weapon::AttackTargetAction::attack:combat::weapon::AttackTargetAction::skill;facts.skill=action->skill;
            if(!weapon_target_attacking(facts))return true;
            const auto protect=prevent_npc_interrupt_locked(target,hit.at,false);if(!protect)return false;
            auto probe=target;if(!probe.cancel_action(probe.lifetime(),hit.at))return false;
            receipt.npc_action=action;receipt.protected_action=*protect;
        }
        return wailing_contacts_.emplace(hit.event,receipt).second;
    }
    void finish_wailing_contact_locked(const combat::weapon::ResolvedAttack&outcome){
        const auto found=wailing_contacts_.find(outcome.event);if(found==wailing_contacts_.end())return;
        const auto receipt=found->second;wailing_contacts_.erase(found);
        if(outcome.outcome!=AttackOutcome::hit||outcome.attacker!=receipt.source||outcome.target!=receipt.target||outcome.at!=receipt.at)return;
        const auto spell=owned_weapon_spell_locked(receipt.source,outcome.at);if(!spell||spell->entry.instance!=receipt.effect)return;
        if(const auto p=players_.find(receipt.target.actor);p!=players_.end()){
            auto&target=p->second;if(target.effect_scope!=receipt.target||target.precise_health<=0||target.control_epoch!=receipt.epoch)return;
            if(!prevent_interrupt_locked(target,outcome.at)){record_control(target,outcome.at);++target.attack_interrupt_epoch;target.casting_skill=0;target.melee_skill_until=0;cancel_ordinary_locked(target,combat::weapon::Cancel::explicit_input);}
        }else{
            const auto n=npc_combat_.find(receipt.target.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=receipt.target||!receipt.npc_action)return;
            auto&target=n->second;const auto*body=target.actor(target.lifetime());const auto current=target.active_action(target.lifetime(),outcome.at);
            if(!body||!body->health||body->health->current<=0||!current||*current!=*receipt.npc_action)return;
            if(receipt.protected_action){if(!prevent_npc_interrupt_locked(target,outcome.at,true))throw std::logic_error("preflighted Wailing protection failed");return;}
            if(!target.cancel_action(target.lifetime(),outcome.at))throw std::logic_error("preflighted Wailing interruption failed");
            Packet stopped;if(current->kind==combat::npcs::control::Action::attack)stopped=combat::weapon::stop_2022(receipt.target.actor);
            else if(current->skill){const auto*r=skills::find_skill(current->skill->skill);u16(stopped,158);u32(stopped,r&&r->type==14?49:59);u32(stopped,receipt.target.actor);u32(stopped,0);}
            if(!stopped.empty())for(auto&[id,peer]:players_)peer.outbox->enqueue({stopped});
        }
    }
    bool end_weapon_spell_attack_locked(skills::effects::Scope owner,std::uint64_t now){
        const auto spell=owned_weapon_spell_locked(owner,now);if(!spell||!spell->plan.ends_on_attack)return true;
        if(const auto p=players_.find(owner.actor);p!=players_.end()){
            auto&target=p->second;if(now<target.health_updated||!advance_health_locked(target,now)||target.precise_health<=0)return false;
            remove_health_effect_locked(target,spell->entry.instance,now);target.weapon_spell.reset();publish_status_locked(target,now);return true;
        }
        const auto n=npc_combat_.find(owner.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=owner)return false;
        auto&target=n->second;if(!target.end_weapon_spell_on_attack(target.lifetime(),now))return false;
        for(auto&[id,peer]:players_)peer.outbox->enqueue({health_effect_remove(owner.actor,spell->entry.instance),emotes::effects_2022(owner.actor,target.actor(target.lifetime())->effects)});return true;
    }
