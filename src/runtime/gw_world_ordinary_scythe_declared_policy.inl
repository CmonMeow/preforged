// Detached declared-policy producer; no default Scythe geometry / normal enable.
public:
    struct OrdinaryScythePolicyFollowon {
        enum class Origin {unspecified,wielder,primary};
        enum class SecondaryOrder {unspecified,actor_id,nearest_wielder,nearest_primary};
        enum class ReactionOrder {unspecified,secondary_first_provisional};
        // Radial center is explicit; angular anchor is ALWAYS the wielder's
        // accepted materialized World facing, even for primary-centered radius.
        double reach=0,half_arc_radians=0;
        Origin radial_origin=Origin::unspecified;SecondaryOrder secondary_order=SecondaryOrder::unspecified;
        ReactionOrder reaction_order=ReactionOrder::unspecified;std::string evidence;
        bool valid()const{return std::isfinite(reach)&&reach>0&&reach<=10000&&std::isfinite(half_arc_radians)&&half_arc_radians>0&&half_arc_radians<=3.14159265358979323846&&
            (radial_origin==Origin::wielder||radial_origin==Origin::primary)&&(secondary_order==SecondaryOrder::actor_id||secondary_order==SecondaryOrder::nearest_wielder||secondary_order==SecondaryOrder::nearest_primary)&&
            reaction_order==ReactionOrder::secondary_first_provisional&&!evidence.empty()&&evidence.size()<=1024;}
    };
private:
    struct OrdinaryScytheEnrollmentFollowon {combat::weapon::Owned weapon;OrdinaryScythePolicyFollowon policy;};
    std::map<skills::effects::Scope,OrdinaryScytheEnrollmentFollowon> ordinary_scythe_policies_followon_;
    struct OrdinaryScytheCandidateFollowon {skills::effects::Scope scope;navigation::Point point;double wielder_distance=0,primary_distance=0;};
    // ONLY selection policy: no damage, RNG, observer consumption or invented
    // exact retail reach/arc/tie rule. Standing source limitation is explicit.
    std::optional<std::vector<skills::effects::Scope>> select_ordinary_scythe_followon_locked(Player& source,
            skills::effects::Scope primary,const OrdinaryScythePolicyFollowon& policy,std::uint64_t now){
        if(!policy.valid()||source.moving||source.steering||source.turn_updated>now)return {};
        const auto origin=healing_position_locked(source,now);if(!origin)return {};
        std::vector<OrdinaryScytheCandidateFollowon> facts;
        for(const auto&[id,target]:players_){
            if(target.effect_scope==source.effect_scope)continue;
            const auto snapshot=ordinary_snapshot_locked(source,target,now);
            if(!snapshot.alive||!snapshot.target_alive||!snapshot.hostile||!snapshot.line_of_sight)continue;
            const auto point=healing_position_locked(target,now);if(!point)return {};
            facts.push_back({target.effect_scope,*point});
        }
        for(const auto&[id,body]:npcs_){
            const auto live=npc_combat_.find(id);if(live==npc_combat_.end())continue;
            const auto snapshot=ordinary_npc_snapshot_locked(source,body,live->second,now);
            if(!snapshot.alive||!snapshot.target_alive||!snapshot.hostile||!snapshot.line_of_sight)continue;
            facts.push_back({live->second.hex_scope(),{body.x,body.y}});
        }
        const auto selected=std::find_if(facts.begin(),facts.end(),[&](const auto& value){return value.scope==primary;});if(selected==facts.end())return {};
        const auto primary_point=selected->point;
        const auto inside=[&](OrdinaryScytheCandidateFollowon& value){
            const auto dx=value.point.x-origin->x,dy=value.point.z-origin->z;
            value.wielder_distance=std::hypot(dx,dy);value.primary_distance=std::hypot(value.point.x-primary_point.x,value.point.z-primary_point.z);
            const auto radial=policy.radial_origin==OrdinaryScythePolicyFollowon::Origin::wielder?value.wielder_distance:value.primary_distance;
            const auto angle=std::remainder(std::atan2(dy,dx)-player_heading_at_locked(source,now),2*3.14159265358979323846);
            return radial<=policy.reach&&std::abs(angle)<=policy.half_arc_radians;
        };
        if(!inside(*selected))return {};
        std::vector<OrdinaryScytheCandidateFollowon> secondary;
        for(auto value:facts)if(value.scope!=primary&&inside(value))secondary.push_back(value);
        std::sort(secondary.begin(),secondary.end(),[&](const auto&a,const auto&b){
            if(policy.secondary_order==OrdinaryScythePolicyFollowon::SecondaryOrder::nearest_wielder&&a.wielder_distance!=b.wielder_distance)return a.wielder_distance<b.wielder_distance;
            if(policy.secondary_order==OrdinaryScythePolicyFollowon::SecondaryOrder::nearest_primary&&a.primary_distance!=b.primary_distance)return a.primary_distance<b.primary_distance;
            return a.scope.actor<b.scope.actor;
        });
        std::vector<skills::effects::Scope> result{primary};for(const auto& value:secondary){if(result.size()==3)break;result.push_back(value.scope);}return result;
    }
    bool resolve_ordinary_scythe_declared_followon_locked(Player& source,skills::effects::Scope primary,
            const combat::weapon::Owned& weapon,bool weakness,bool blind,std::uint64_t now){
        const auto owner=source.effect_scope;const auto enrollment=ordinary_scythe_policies_followon_.find(owner);
        if(enrollment==ordinary_scythe_policies_followon_.end()||!enrollment->second.policy.valid()||weapon.item_type!=35||weapon.caster_weapon||holding_bundle_locked(owner)||
           !source.ordinary_weapon||!combat::weapon::same_weapon(weapon,*source.ordinary_weapon)||!combat::weapon::same_weapon(weapon,enrollment->second.weapon)||!source.resources||source.precise_health<=0)return false;
        const auto control=source.control_epoch,interrupt=source.attack_interrupt_epoch;
        DamagePublication publication;begin_damage_publication_locked(publication);
        const auto perform=[&]()->bool{
            const auto selected=select_ordinary_scythe_followon_locked(source,primary,enrollment->second.policy,now);if(!selected||selected->empty()||selected->size()>3)return false;
            const auto illusionary=illusionary_attack_locked(source,weapon,now);
            for(const auto target:*selected)if(!player_ordinary_release_ready_locked(source,target,weapon,now))return false;
            if(!player_empathy_reaction_ready_locked(owner,now)||source.precise_health<=0||source.control_epoch!=control||source.attack_interrupt_epoch!=interrupt)return false;
            if(next_attack_outcome_>UINT64_MAX-selected->size()-1)return false;
            const auto existing=attack_outcomes_.find(owner);
            auto staged=existing==attack_outcomes_.end()?combat::weapon::AttackOutcomeDelivery(owner):existing->second;
            auto next=next_attack_outcome_;const auto action=staged.reserve_ordinary_action(owner,next++,now,control);if(!action)return false;
            std::vector<combat::weapon::AttackRelease> contacts;
            // Canonical IDs may keep primary first; observed publication order
            // is secondary before primary and never inferred from these IDs.
            for(const auto target:*selected){if(!staged.release(owner,target,next,now,combat::weapon::AttackDelivery::melee))return false;contacts.push_back({target,next++,combat::weapon::AttackDelivery::melee});}
            if(!staged.can_finish(owner,contacts.size()))return false;
            attack_outcomes_.insert_or_assign(owner,std::move(staged));next_attack_outcome_=next;
            AttackResolution source_action(*this,*action);bool made=false;std::set<std::uint64_t> closed;
            const auto finish=[&](const auto& contact,AttackOutcome result){if(closed.insert(contact.event).second)finish_attack_outcome_locked(owner,contact.target,contact.event,result,now);};
            const auto cancel_remaining=[&]{for(const auto& contact:contacts)finish(contact,AttackOutcome::cancelled);};
            if(!illusionary)for(const auto& contact:contacts){
                auto preview=attack_outcomes_.at(owner);const auto hit=preview.qualify_hit(owner,contact.target,contact.event,now);
                if(!hit||!prepare_attack_contact_contract_locked(*hit,false,true)){cancel_remaining();return false;}
            }
            if(source.precise_health<=0||source.control_epoch!=control||source.attack_interrupt_epoch!=interrupt){cancel_remaining();return false;}
            if(!source_action.make_action()){cancel_remaining();return false;}made=true;
            if(!source_action.action_once(8,[&]{return end_weapon_spell_attack_locked(owner,now);})||!source_action.release(false)){cancel_remaining();return made;}
            if(!source_action.action_once(16,[&]{observe_pet_attack_release_locked(owner,primary,now);return true;})){cancel_remaining();return made;}
            std::vector<std::size_t> order;for(std::size_t index=1;index<contacts.size();++index)order.push_back(index);order.push_back(0);
            if(!source_action.action_once(32,[&]{reserve_dodge_this_attack_locked(owner,contacts[order.front()].event,now);return true;})){cancel_remaining();return made;}
            const auto target_still_live=[&](skills::effects::Scope target){
                if(const auto player=players_.find(target.actor);player!=players_.end())return player->second.effect_scope==target&&player->second.resources&&player->second.precise_health>0;
                const auto npc=npc_combat_.find(target.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=target||!npcs_.contains(target.actor))return false;
                const auto actor=npc->second.actor(npc->second.lifetime());return actor&&actor->health&&actor->health->current>0&&actor->damageable;
            };
            // Declared provisional internal order. Replay receipts prove stored
            // secondary-before-primary DAMAGE, not the hidden reaction order.
            for(const auto index:order){const auto contact=contacts[index];
                if(source.precise_health<=0||source.control_epoch!=control||source.attack_interrupt_epoch!=interrupt){cancel_remaining();return made;}
                // Prior committed contact cohorts (Splinter, Mark of Pain,...)
                // may retire a later selected foe. Its reservation is NOT a
                // new actual contact observer against a corpse/new generation.
                if(!target_still_live(contact.target)){finish(contact,AttackOutcome::target_retired);continue;}
                if(!player_empathy_release_locked(owner,contact.target,contact.event,now)){cancel_remaining();return made;}
                if(!source_action.action_once(2,[&]{return vigorous_release_locked(owner,now,skills::effects::VigorousActionKind::attack);})){cancel_remaining();return made;}
                std::optional<AttackOutcome> result;
                if(const auto player=players_.find(contact.target.actor);player!=players_.end()){
                    if(player->second.effect_scope==contact.target)result=ordinary_hit_locked(source,player->second,weapon,source.ordinary_level,weakness,blind,now,0,{},contact.event,{},!illusionary);
                }else{
                    const auto body=npcs_.find(contact.target.actor);const auto live=npc_combat_.find(contact.target.actor);
                    if(body!=npcs_.end()&&live!=npc_combat_.end()&&live->second.hex_scope()==contact.target)result=ordinary_npc_hit_locked(source,body->second,live->second,weapon,source.ordinary_level,weakness,blind,now,0,{},contact.event,{},!illusionary);
                }
                // A late contact failure is an actual made attack with a
                // cancelled recipient, never a retroactive no-release refusal.
                finish(contact,result.value_or(AttackOutcome::cancelled));
            }
            // If all reserved recipients disappeared before contact, this is
            // still one made action; a living source gets its once-only Vigor.
            if(source.precise_health>0&&!source_action.action_once(2,[&]{return vigorous_release_locked(owner,now,skills::effects::VigorousActionKind::attack);})){cancel_remaining();return made;}
            source_action.action_once(4,[&]{ordinary_spiteful_release_locked(owner,now);return true;});return made;
        };
        try{const bool made=perform();auto release=finish_damage_publication_locked(publication);if(release)release();return made;}
        catch(...){auto release=finish_damage_publication_locked(publication);if(release)release();throw;}
    }
public:
    // Trusted host enrollment ONLY. There is no geometry default, catalog row,
    // Session binding or implicit enable for every item-type35 actor.
    bool configure_ordinary_scythe_policy_followon(skills::effects::Scope scope,OrdinaryScythePolicyFollowon policy,std::uint64_t now){
        if(!policy.valid())return false;std::lock_guard lock(mutex_);const auto found=players_.find(scope.actor);
        if(found==players_.end()||found->second.effect_scope!=scope||!found->second.ordinary_weapon||found->second.ordinary_weapon->item_type!=35||found->second.ordinary_weapon->caster_weapon||
           holding_bundle_locked(scope)||found->second.ordinary_attack.active()||now<found->second.health_updated)return false;
        ordinary_scythe_policies_followon_.insert_or_assign(scope,OrdinaryScytheEnrollmentFollowon{*found->second.ordinary_weapon,std::move(policy)});return true;
    }
private:
