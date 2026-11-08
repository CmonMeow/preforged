// Explicit declared ordinary dagger driver ONLY; no default random/timing enable.
public:
    struct OrdinaryDaggerPolicyFollowon {
        enum class Timing {unspecified,empirical_delayed_declared};
        enum class Grain {unspecified,action_once_declared,physical_contacts_declared};
        combat::weapon::OrdinaryDaggerChancePolicyFollowon chance;
        unsigned second_contact_base_ms=0;Timing timing=Timing::unspecified;Grain empathy=Grain::unspecified,vigor=Grain::unspecified;
        std::string evidence;
        bool valid()const{return chance.valid()&&timing==Timing::empirical_delayed_declared&&second_contact_base_ms==500&&
            (empathy==Grain::action_once_declared||empathy==Grain::physical_contacts_declared)&&
            (vigor==Grain::action_once_declared||vigor==Grain::physical_contacts_declared)&&!evidence.empty()&&evidence.size()<=1024;}
    };
private:
    struct OrdinaryDaggerEnrollmentFollowon {combat::weapon::Owned weapon;OrdinaryDaggerPolicyFollowon policy;std::function<double()> draw;};
    std::map<skills::effects::Scope,OrdinaryDaggerEnrollmentFollowon> ordinary_dagger_policies_followon_;
    struct OrdinaryDaggerSecondFollowon {
        combat::weapon::AttackMade made;skills::effects::Scope target;combat::weapon::Owned weapon;
        OrdinaryDaggerPolicyFollowon policy;std::uint64_t due,control,interrupt,intent_epoch;unsigned level;
    };
    std::map<skills::effects::Scope,OrdinaryDaggerSecondFollowon> ordinary_dagger_seconds_followon_;
    std::uint64_t ordinary_dagger_second_frontier_followon_locked(const OrdinaryDaggerSecondFollowon& phase)const{
        auto at=attack_participant_observer_frontier_locked(phase.made.owner(),phase.target,phase.due);
        // Read actual retained producer clocks; never rewind an already
        // materialized controller or replace its motion with a guessed chord.
        if(const auto motion=npc_ai_motion_.find(phase.target.actor);motion!=npc_ai_motion_.end()&&motion->second.source==phase.target)at=std::max(at,motion->second.updated);
        if(const auto pet=pet_ai_.find(phase.target.actor);pet!=pet_ai_.end()&&pet->second.source==phase.target&&pet->second.layered_motion)at=std::max(at,pet->second.layered_motion->updated);
        return splinter_attack_observer_frontier_locked(phase.made.owner(),phase.target,at);
    }
    std::optional<std::uint64_t> next_ordinary_dagger_second_followon_locked()const{
        std::optional<std::uint64_t> result;
        for(const auto&[owner,phase]:ordinary_dagger_seconds_followon_){const auto at=ordinary_dagger_second_frontier_followon_locked(phase);if(!result||at<*result)result=at;}
        return result;
    }
    bool resolve_ordinary_dagger_declared_followon_locked(Player& source,skills::effects::Scope target,
            const combat::weapon::Owned& weapon,bool weakness,bool blind,std::uint64_t now){
        const auto owner=source.effect_scope;
        // A faster NEW ordinary instance can release before the cancelled old
        // instance's nominal second deadline. Retire its held parent now rather
        // than incorrectly refusing the new action because that map slot exists.
        if(const auto held=ordinary_dagger_seconds_followon_.find(owner);held!=ordinary_dagger_seconds_followon_.end()&&
           (held->second.intent_epoch!=source.ordinary_attack.epoch()||held->second.control!=source.control_epoch||held->second.interrupt!=source.attack_interrupt_epoch)){
            auto cancelled=std::move(held->second);ordinary_dagger_seconds_followon_.erase(held);close_ordinary_dagger_action_followon_locked(cancelled,now);
        }
        const auto enrollment=ordinary_dagger_policies_followon_.find(owner);
        if(ordinary_dagger_seconds_followon_.contains(owner)||enrollment==ordinary_dagger_policies_followon_.end()||!enrollment->second.policy.valid()||!enrollment->second.draw||weapon.item_type!=32||weapon.caster_weapon||
           holding_bundle_locked(owner)||!source.ordinary_weapon||!combat::weapon::same_weapon(weapon,*source.ordinary_weapon)||!combat::weapon::same_weapon(weapon,enrollment->second.weapon)||!source.resources||source.precise_health<=0)return false;
        const auto policy=enrollment->second.policy;const auto control=source.control_epoch,interrupt=source.attack_interrupt_epoch,intent_epoch=source.ordinary_attack.epoch();
        DamagePublication publication;begin_damage_publication_locked(publication);
        const auto perform=[&]()->bool{
            if(!player_ordinary_release_ready_locked(source,target,weapon,now)||!player_empathy_reaction_ready_locked(owner,now)||source.precise_health<=0||source.control_epoch!=control||source.attack_interrupt_epoch!=interrupt)return false;
            // Injected trusted draw only. Locust's Fury effect wiring is NOT enabled.
            const auto selected=combat::weapon::select_ordinary_dagger_contacts_followon(enrollment->second.policy.chance,32,weapon.mastery,false,enrollment->second.draw());
            if(!selected||next_attack_outcome_>UINT64_MAX-2)return false;
            const auto registry=source.effect_registry?source.effect_registry->active(owner,now):std::vector<skills::effects::Entry>{};
            // Empirical~500ms*factor, EXPLICIT authored policy. Reuse shared IAS
            // rounding once; capture the accepted first-contact speed snapshot.
            const auto gap=skills::effects::reviewed_attack_timing({policy.second_contact_base_ms,policy.second_contact_base_ms,128},registry,now,false,false);
            if(!gap||!gap->period_ms||gap->period_ms>=ordinary_timing_locked(source,weapon,now).period_ms||now>UINT64_MAX-gap->period_ms)return false;
            const auto second_due=now+gap->period_ms;
            const bool illusionary=illusionary_attack_locked(source,weapon,now).has_value();
            const auto existing=attack_outcomes_.find(owner);
            auto staged=existing==attack_outcomes_.end()?combat::weapon::AttackOutcomeDelivery(owner):existing->second;
            auto next=next_attack_outcome_;const auto reservation=staged.reserve_ordinary_action(owner,next++,now,control);if(!reservation)return false;
            const auto made=staged.make_action(owner,*reservation);if(!made||!staged.declare_ordinary_contacts(owner,*made,selected->contacts,control))return false;
            std::vector<combat::weapon::AttackRelease> contacts;
            for(unsigned ordinal=0;ordinal<1;++ordinal){
                if(!staged.release_action_contact(owner,*made,target,next,now,ordinal,control))return false;
                contacts.push_back({target,next++,combat::weapon::AttackDelivery::melee});
            }
            if(!staged.can_finish(owner,contacts.size()))return false;
            // Shared preparation adapters reserve event-keyed contracts only.
            // Fresh global IDs have no prior contact owner. Refused staging must
            // retire these reservations before those unpublished IDs can recur.
            struct PreparationGuard {std::function<void()> cleanup;~PreparationGuard(){if(cleanup)cleanup();}};
            PreparationGuard prepared{[&]{for(const auto& contact:contacts){
                bulls_charge_contacts_.erase(contact.event);splinter_contacts_.erase(contact.event);
                judgment_contacts_.erase(contact.event);tainted_contacts_.erase(contact.event);
                wailing_contacts_.erase(contact.event);retribution_contacts_.erase(contact.event);
            }}};

            // Preflight uses a copy of the SAME canonical contacts, before the
            // staged made action becomes authoritative or source observers run.
            if(!illusionary)for(const auto& contact:contacts){auto preview=staged;const auto qualified=preview.qualify_hit(owner,target,contact.event,now);
                if(!qualified||!prepare_attack_contact_contract_locked(*qualified,false,true))return false;}
            if(source.precise_health<=0||source.control_epoch!=control||source.attack_interrupt_epoch!=interrupt)return false;
            attack_outcomes_.insert_or_assign(owner,std::move(staged));next_attack_outcome_=next;prepared.cleanup={};
            AttackResolution action(*this,*reservation);action.made_=*made; // exact private capability from now-adopted canonical staged journal
            std::set<std::uint64_t> closed;
            const auto finish=[&](const auto& contact,AttackOutcome result){if(closed.insert(contact.event).second)finish_attack_outcome_locked(owner,target,contact.event,result,now);};
            const auto cancel_remaining=[&]{for(const auto& contact:contacts)finish(contact,AttackOutcome::cancelled);};
            // An actual made action still owns Spiteful when a first contact's
            // nonlethal retaliation changes control and cancels its second.
            // Preserve existing single-ordinary release ordering: after the
            // completed/cancelled contact terminal(s), never per physical hit.
            const auto finish_source_action=[&]{if(source.effect_scope==owner&&source.precise_health>0)
                action.action_once(4,[&]{ordinary_spiteful_release_locked(owner,now);return true;});};

            if(!action.action_once(8,[&]{return end_weapon_spell_attack_locked(owner,now);})||!action.release(policy.empathy==OrdinaryDaggerPolicyFollowon::Grain::action_once_declared)){cancel_remaining();finish_source_action();return true;}
            if(!action.action_once(16,[&]{observe_pet_attack_release_locked(owner,target,now);return true;})){cancel_remaining();finish_source_action();return true;}
            if(!action.action_once(32,[&]{reserve_dodge_this_attack_locked(owner,contacts.front().event,now);return true;})){cancel_remaining();finish_source_action();return true;}
            const auto target_live=[&]{
                if(const auto p=players_.find(target.actor);p!=players_.end())return p->second.effect_scope==target&&p->second.resources&&p->second.precise_health>0;
                const auto n=npc_combat_.find(target.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=target||!npcs_.contains(target.actor))return false;
                const auto body=n->second.actor(n->second.lifetime());return body&&body->health&&body->health->current>0&&body->damageable;
            };
            for(const auto& contact:contacts){
                if(source.precise_health<=0||source.control_epoch!=control||source.attack_interrupt_epoch!=interrupt){cancel_remaining();finish_source_action();return true;}
                if(!target_live()){finish(contact,AttackOutcome::target_retired);continue;}
                if(!player_empathy_release_locked(owner,target,contact.event,now,policy.empathy==OrdinaryDaggerPolicyFollowon::Grain::physical_contacts_declared)){cancel_remaining();finish_source_action();return true;}
                const auto vigor=[&]{return vigorous_release_locked(owner,now,skills::effects::VigorousActionKind::attack);};
                if(!(policy.vigor==OrdinaryDaggerPolicyFollowon::Grain::physical_contacts_declared?vigor():action.action_once(2,vigor))){cancel_remaining();finish_source_action();return true;}
                std::optional<AttackOutcome> result;
                if(const auto p=players_.find(target.actor);p!=players_.end()){
                    if(p->second.effect_scope==target)result=ordinary_hit_locked(source,p->second,weapon,source.ordinary_level,weakness,blind,now,0,{},contact.event,{},!illusionary);
                }else{const auto body=npcs_.find(target.actor);const auto live=npc_combat_.find(target.actor);
                    if(body!=npcs_.end()&&live!=npc_combat_.end()&&live->second.hex_scope()==target)result=ordinary_npc_hit_locked(source,body->second,live->second,weapon,source.ordinary_level,weakness,blind,now,0,{},contact.event,{},!illusionary);}
                finish(contact,result.value_or(AttackOutcome::cancelled));
            }
            finish_source_action();
            if(selected->contacts==2&&source.precise_health>0&&source.control_epoch==control&&source.attack_interrupt_epoch==interrupt&&
               source.ordinary_attack.epoch()==intent_epoch&&source.ordinary_attack.active()&&target_live()){
                const OrdinaryDaggerSecondFollowon phase{*made,target,weapon,policy,second_due,control,interrupt,intent_epoch,source.ordinary_level};
                if(!ordinary_dagger_seconds_followon_.emplace(owner,phase).second)throw std::logic_error("ordinary dagger phase overwrote current action");
                if(!action.retain_action())throw std::logic_error("ordinary dagger lost canonical retained action");
            }
            return true;
        };
        try{const bool made=perform();auto release=finish_damage_publication_locked(publication);if(release)release();return made;}
        catch(...){auto release=finish_damage_publication_locked(publication);if(release)release();throw;}
    }
    void close_ordinary_dagger_action_followon_locked(const OrdinaryDaggerSecondFollowon& phase,std::uint64_t at){
        const auto owner=phase.made.owner();const auto ledger=attack_outcomes_.find(owner);
        if(ledger==attack_outcomes_.end()||!ledger->second.action_observed(owner,phase.made,1))return;
        if(!ledger->second.finish_action(owner,phase.made,at))throw std::logic_error("ordinary dagger retained parent lost terminal");
        retire_splinter_receipts_locked(owner);
    }
    void resolve_ordinary_dagger_second_followon_locked(OrdinaryDaggerSecondFollowon phase,std::uint64_t at){
        const auto owner=phase.made.owner();
        const auto close=[&]{close_ordinary_dagger_action_followon_locked(phase,at);};
        auto source=players_.find(owner.actor);
        if(source==players_.end()||source->second.effect_scope!=owner){close();return;}
        if(!advance_resource_event_locked(source->second,at)){close();return;}
        if(const auto target=players_.find(phase.target.actor);target!=players_.end()&&target->second.effect_scope==phase.target)advance_health_locked(target->second,at);
        else if(const auto target=npc_combat_.find(phase.target.actor);target!=npc_combat_.end()&&target->second.hex_scope()==phase.target)advance_npc_conditions_locked(phase.target.actor,target->second,at);
        if(!players_.contains(phase.target.actor))refresh_npc_movement_locked(phase.target.actor,at);
        // No map iterators from target settlement are retained across reactions.
        source=players_.find(owner.actor);
        if(source==players_.end()||source->second.effect_scope!=owner){close();return;}
        auto& actor=source->second;
        const auto current=[&]{return actor.resources&&actor.precise_health>0&&actor.control_epoch==phase.control&&actor.attack_interrupt_epoch==phase.interrupt&&
            actor.ordinary_attack.active()&&actor.ordinary_attack.epoch()==phase.intent_epoch&&actor.ordinary_attack.target()==std::optional(phase.target)&&
            actor.ordinary_level==phase.level&&actor.ordinary_weapon&&combat::weapon::same_weapon(*actor.ordinary_weapon,phase.weapon)&&!holding_bundle_locked(owner);};
        if(!current()){close();return;}
        combat::weapon::Snapshot snapshot;
        if(const auto target=players_.find(phase.target.actor);target!=players_.end()&&target->second.effect_scope==phase.target)snapshot=ordinary_snapshot_locked(actor,target->second,at);
        else if(const auto body=npcs_.find(phase.target.actor);body!=npcs_.end()){
            const auto live=npc_combat_.find(phase.target.actor);if(live!=npc_combat_.end()&&live->second.hex_scope()==phase.target)snapshot=ordinary_npc_snapshot_locked(actor,body->second,live->second,at);
        }
        if(!snapshot.alive||!snapshot.target_alive||!snapshot.hostile||!snapshot.line_of_sight||snapshot.distance>ordinary_timing_locked(actor,phase.weapon,at).range||
           !player_ordinary_release_ready_locked(actor,phase.target,phase.weapon,at)||(phase.policy.empathy==OrdinaryDaggerPolicyFollowon::Grain::physical_contacts_declared&&!player_empathy_reaction_ready_locked(owner,at))||!current()){close();return;}
        const auto actual=attack_outcomes_.find(owner);
        if(actual==attack_outcomes_.end()||next_attack_outcome_==UINT64_MAX){close();return;}
        auto staged=actual->second;const auto event=next_attack_outcome_;
        if(!staged.release_action_contact(owner,phase.made,phase.target,event,at,1,phase.control)||!staged.can_finish(owner)){close();return;}
        const bool illusionary=illusionary_attack_locked(actor,phase.weapon,at).has_value();
        struct PreparationGuard {std::function<void()> cleanup;~PreparationGuard(){if(cleanup)cleanup();}};
        PreparationGuard prepared{[&]{bulls_charge_contacts_.erase(event);splinter_contacts_.erase(event);judgment_contacts_.erase(event);tainted_contacts_.erase(event);wailing_contacts_.erase(event);retribution_contacts_.erase(event);}};
        if(!illusionary){auto preview=staged;const auto qualified=preview.qualify_hit(owner,phase.target,event,at);
            if(!qualified||!prepare_attack_contact_contract_locked(*qualified,false,true)){close();return;}}
        if(!current()){close();return;}
        attack_outcomes_.insert_or_assign(owner,std::move(staged));++next_attack_outcome_;prepared.cleanup={};
        const auto finish=[&](AttackOutcome result){finish_attack_outcome_locked(owner,phase.target,event,result,at);close();};
        if(!player_empathy_release_locked(owner,phase.target,event,at,phase.policy.empathy==OrdinaryDaggerPolicyFollowon::Grain::physical_contacts_declared)){finish(AttackOutcome::cancelled);return;}
        if(phase.policy.vigor==OrdinaryDaggerPolicyFollowon::Grain::physical_contacts_declared&&
           !vigorous_release_locked(owner,at,skills::effects::VigorousActionKind::attack)){finish(AttackOutcome::cancelled);return;}
        const bool weakness=actor.conditions.has(combat::conditions::Kind::weakness,at),blind=actor.conditions.has(combat::conditions::Kind::blind,at);
        std::optional<AttackOutcome> result;
        if(const auto target=players_.find(phase.target.actor);target!=players_.end()&&target->second.effect_scope==phase.target)
            result=ordinary_hit_locked(actor,target->second,phase.weapon,phase.level,weakness,blind,at,0,{},event,{},!illusionary);
        else{const auto body=npcs_.find(phase.target.actor);const auto live=npc_combat_.find(phase.target.actor);
            if(body!=npcs_.end()&&live!=npc_combat_.end()&&live->second.hex_scope()==phase.target)
                result=ordinary_npc_hit_locked(actor,body->second,live->second,phase.weapon,phase.level,weakness,blind,at,0,{},event,{},!illusionary);}
        finish(result.value_or(AttackOutcome::cancelled));
    }
    void resolve_ordinary_dagger_seconds_followon_locked(std::uint64_t at){
        std::vector<skills::effects::Scope> due;
        for(const auto&[owner,phase]:ordinary_dagger_seconds_followon_)if(ordinary_dagger_second_frontier_followon_locked(phase)<=at)due.push_back(owner);
        for(const auto owner:due){const auto found=ordinary_dagger_seconds_followon_.find(owner);
            if(found==ordinary_dagger_seconds_followon_.end()||ordinary_dagger_second_frontier_followon_locked(found->second)>at)continue;
            auto phase=std::move(found->second);ordinary_dagger_seconds_followon_.erase(found); // consume BEFORE callbacks
            resolve_ordinary_dagger_second_followon_locked(std::move(phase),at);
        }
    }
public:
    bool configure_ordinary_dagger_policy_followon(skills::effects::Scope scope,OrdinaryDaggerPolicyFollowon policy,
            std::function<double()> draw,std::uint64_t now){
        if(!policy.valid()||!draw)return false;std::lock_guard lock(mutex_);const auto found=players_.find(scope.actor);
        if(found==players_.end()||found->second.effect_scope!=scope||!found->second.ordinary_weapon||found->second.ordinary_weapon->item_type!=32||found->second.ordinary_weapon->caster_weapon||
           holding_bundle_locked(scope)||found->second.ordinary_attack.active()||now<found->second.health_updated)return false;
        ordinary_dagger_policies_followon_.insert_or_assign(scope,OrdinaryDaggerEnrollmentFollowon{*found->second.ordinary_weapon,std::move(policy),std::move(draw)});return true;
    }
private:
