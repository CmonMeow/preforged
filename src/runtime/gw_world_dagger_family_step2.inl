// Un-included step2 candidate, inside Instance after first-step contact and
// dual-group helpers. All actual damage remains the existing weapon pipeline.
private:
    struct DaggerFamilyPreparedDraft {
        combat::weapon::DetachedDaggerFamilyPlan plan;
        combat::weapon::DetachedDaggerContactFacts facts;
        skills::effects::Scope source,target;
        skills::effects::DaggerChain admitted;
        std::uint64_t at=0;
        std::optional<AreaDamageRecipients> collateral;
        combat::Damage collateral_damage;
    };
    class DaggerFamilyCommitDraft {
        friend class Instance;
        DaggerFamilyPreparedDraft prepared_;
        combat::weapon::QualifiedHit qualified_;
        DaggerFamilyCommitDraft(DaggerFamilyPreparedDraft prepared,combat::weapon::QualifiedHit qualified):
            prepared_(std::move(prepared)),qualified_(qualified){}
    public:
        ~DaggerFamilyCommitDraft()=default;
        DaggerFamilyCommitDraft(const DaggerFamilyCommitDraft&)=delete;
        DaggerFamilyCommitDraft& operator=(const DaggerFamilyCommitDraft&)=delete;
    };
    // Per-source synchronous melee events are monotonic. Retain through death
    // (same Scope may resurrect), retire only with the source lifetime.
    std::map<skills::effects::Scope,std::uint64_t> dagger_family_commit_watermark_;
    std::unique_ptr<DaggerFamilyCommitDraft> own_dagger_family_commit_draft_locked(
            DaggerFamilyPreparedDraft prepared,const combat::weapon::QualifiedHit& qualified){
        if(prepared.source!=qualified.attacker||prepared.target!=qualified.target||prepared.at!=qualified.at||
           !qualified.event||qualified.delivery!=combat::weapon::AttackDelivery::melee||qualified.released>qualified.at||
           !dagger_family_alive_draft_locked(qualified.attacker)||!dagger_family_alive_draft_locked(qualified.target))return {};
        const auto previous=dagger_family_commit_watermark_.find(qualified.attacker);
        if(previous!=dagger_family_commit_watermark_.end()&&qualified.event<=previous->second)return {};
        if(prepared.plan.strikes==2&&(!dagger_dual_world_draft_||
            !dagger_dual_world_draft_->group.owns_event(qualified.event)))return {};
        dagger_family_commit_watermark_.insert_or_assign(qualified.attacker,qualified.event);
        return std::unique_ptr<DaggerFamilyCommitDraft>(new DaggerFamilyCommitDraft(std::move(prepared),qualified));
    }
    bool dagger_family_alive_draft_locked(skills::effects::Scope scope)const{
        if(const auto p=players_.find(scope.actor);p!=players_.end())return p->second.effect_scope==scope&&p->second.resources&&p->second.precise_health>0;
        const auto n=npc_combat_.find(scope.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=scope)return false;
        const auto* actor=n->second.actor(n->second.lifetime());return actor&&actor->health&&actor->health->current>0;
    }
    std::optional<AreaDamageCentre> dagger_family_centre_draft_locked(skills::effects::Scope scope,std::uint64_t at){
        if(const auto p=players_.find(scope.actor);p!=players_.end()){
            if(p->second.effect_scope!=scope)return {};const auto point=healing_position_locked(p->second,at);if(!point)return {};
            return AreaDamageCentre{*point,p->second.surface.plane};
        }
        refresh_npc_movement_locked(scope.actor,at);
        const auto p=npcs_.find(scope.actor);const auto n=npc_combat_.find(scope.actor);
        if(p==npcs_.end()||n==npc_combat_.end()||n->second.hex_scope()!=scope)return {};
        return AreaDamageCentre{{p->second.x,p->second.y},p->second.plane};
    }
    std::optional<bool> dagger_family_adjacent_ally_draft_locked(skills::effects::Scope target,std::uint64_t at){
        const auto centre=dagger_family_centre_draft_locked(target,at);if(!centre)return {};
        bool ally=false;
        const auto inspect=[&](skills::effects::Scope candidate,const AreaDamageCentre& point)->bool{
            if(candidate==target||point.plane!=centre->plane||std::hypot(point.point.x-centre->point.x,point.point.z-centre->point.z)>geometry::adjacent_center_community)return true;
            if(!dagger_family_alive_draft_locked(candidate))return true;
            const auto relation=subversion_spell_target_locked(target,candidate);
            if(!relation||relation->relation==skills::effects::SpellTargetRelation::unknown)return false;
            ally=ally||relation->relation==skills::effects::SpellTargetRelation::ally;return true;
        };
        std::vector<skills::effects::Scope> candidates;
        for(const auto&[id,p]:players_)candidates.push_back(p.effect_scope);
        for(const auto&[id,n]:npc_combat_)candidates.push_back(n.hex_scope());
        for(const auto candidate:candidates){const auto point=dagger_family_centre_draft_locked(candidate,at);
            if(!point||!inspect(candidate,*point))return {};
        }
        return ally;
    }
    std::optional<DaggerFamilyPreparedDraft> prepare_dagger_family_draft_locked(skills::effects::Scope source,
            skills::effects::Scope target,const combat::weapon::DetachedDaggerFamilyPlan& plan,std::uint64_t at,bool illusionary,const skills::effects::DaggerChain* admission_preview=nullptr){
        if(combat::weapon::detached_dagger_family_plan(plan.skill,plan.rank)!=plan||
           !dagger_family_alive_draft_locked(source)||!dagger_family_alive_draft_locked(target)||at>UINT64_MAX-15000)return {};
        const auto caster=players_.find(source.actor);if(caster==players_.end()||caster->second.effect_scope!=source)return {};
        DaggerFamilyPreparedDraft prepared;prepared.plan=plan;prepared.source=source;prepared.target=target;prepared.at=at;
        prepared.admitted=caster->second.dagger_chain;
        if(plan.strikes==2){
            if(admission_preview)prepared.admitted=*admission_preview;
            else{
                if(!dagger_dual_allowed_draft_locked(source,target,plan.skill,at)||!dagger_dual_world_draft_)return {};
                prepared.admitted=dagger_dual_world_draft_->group.admitted_chain();
            }
            if(prepared.admitted.current(target,at)!=2)return {};
        }
        const auto p=players_.find(target.actor);
        if(p!=players_.end()){
            prepared.facts.target_health=combat::Health{p->second.precise_health,p->second.resources->max_health};
            prepared.facts.target_knocked_down=p->second.knocked_down_until>at;
            if(!p->second.effect_registry)return {};
            prepared.facts.target_hexed=p->second.effect_registry->has(target,skills::effects::Category::hex,at);
        }else{
            const auto n=npc_combat_.find(target.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=target)return {};
            const auto life=n->second.lifetime();const auto* actor=n->second.actor(life);
            const auto hex=n->second.hex_registry_snapshot(life,at);if(!actor||!actor->health||!hex)return {};
            prepared.facts.target_health=*actor->health;prepared.facts.target_hexed=hex->has(target,skills::effects::Category::hex,at);
            prepared.facts.target_knocked_down=n->second.knocked_down(life,at);
        }
        if(combat::weapon::dagger_role(plan.skill)==976&&prepared.admitted.current(target,at)!=2)return {};
        if(combat::weapon::dagger_role(plan.skill)==778&&(!prepared.facts.target_knocked_down||!*prepared.facts.target_knocked_down))return {};
        // IW replaces successful in-sequence attacks; it never applies the
        // physical-hit conditions, Energy, KD, collateral or chain marks.
        if(illusionary)return prepared;
        if(combat::weapon::dagger_role(plan.skill)==777){const auto adjacent=dagger_family_adjacent_ally_draft_locked(target,at);if(!adjacent)return {};
            prepared.facts.target_has_adjacent_ally=*adjacent;
            if(!*adjacent){
                if(p!=players_.end()){if(p->second.knocked_down_until<=at&&p->second.control_epoch==UINT64_MAX)return {};}
                else{const auto n=npc_combat_.find(target.actor);auto probe=n->second;
                    const auto result=probe.knock_down(probe.lifetime(),at,2000);using K=combat::npcs::control::Knockdown;
                    if(result==K::invalid||result==K::unknown)return {};
                }
            }
        }
        if(combat::weapon::dagger_role(plan.skill)==776||combat::weapon::dagger_role(plan.skill)==778){
            const auto kind=combat::weapon::dagger_role(plan.skill)==776?combat::conditions::Kind::bleeding:combat::conditions::Kind::poison;
            if(p!=players_.end()){auto probe=p->second.conditions;
                if(p->second.next_health_effect>UINT32_MAX-2||!probe.apply(kind,source.actor,at,plan.duration_ms)||
                   (combat::weapon::dagger_role(plan.skill)==776&&!probe.apply(combat::conditions::Kind::deep_wound,source.actor,at,plan.duration_ms)))return {};
            }else{
                const auto n=npc_combat_.find(target.actor);const auto life=n->second.lifetime();
                if(n->second.allows_conditions(life)){const auto flesh=combat::conditions::flesh_eligibility(kind,n->second.fleshy(life));
                    if(!flesh)return {};if(*flesh){auto probe=n->second;
                        if(!probe.apply_condition(life,kind,source.actor,at,plan.duration_ms)||
                           (combat::weapon::dagger_role(plan.skill)==776&&!probe.apply_condition(life,combat::conditions::Kind::deep_wound,source.actor,at,plan.duration_ms)))return {};
                        for(const auto& reaction:probe.take_fragility_hits(life))if(reaction.reversal&&!validate_npc_reversal_receipts_locked(target,{reaction.reversal}))return {};
                    }
                }
            }
        }
        if(combat::weapon::dagger_role(plan.skill)==775){const auto centre=dagger_family_centre_draft_locked(target,at);if(!centre||!caster->second.team_token)return {};
            prepared.collateral_damage={combat::Kind::armor_ignoring,double(plan.bonus),caster->second.ordinary_level,plan.skill};
            // Candidate shared-area API gains a defaulted excluded Scope so
            // primary exclusion happens BEFORE any collateral cohort preflight.
            prepared.collateral=admit_area_damage_pulse_locked(source,prepared.collateral_damage,*centre,geometry::adjacent_center_community,at,
                caster->second.team_token,std::nullopt,{}, {},target);
            if(!prepared.collateral)return {};
        }
        if(!dagger_family_alive_draft_locked(source)||!dagger_family_alive_draft_locked(target))return {};
        return prepared;
    }
    unsigned dagger_family_extra_bonus_draft(const DaggerFamilyPreparedDraft& prepared)const{
        const auto health=prepared.facts.target_health;
        return combat::weapon::dagger_role(prepared.plan.skill)==783&&health&&health->current*10>double(health->maximum)*9?prepared.plan.extra:0;
    }
    // Last callback in each real physical strike, after original terminal and
    // preparation/resource consumers. No caller actor references used afterward.
    bool commit_dagger_family_draft_locked(std::unique_ptr<DaggerFamilyCommitDraft> owned,DaggerContactPublicationDraft& publication){
        if(!owned)return false;
        auto prepared=std::move(owned->prepared_);const auto qualified=owned->qualified_;
        const auto effect=combat::weapon::detached_dagger_family_effect(prepared.plan,prepared.admitted,qualified,prepared.facts);
        if(!effect)return false;
        const auto source=prepared.source,target=prepared.target;const auto at=prepared.at;
        if(dagger_family_alive_draft_locked(source)&&effect->energy_gain){const auto caster=players_.find(source.actor);
            if(caster==players_.end()||caster->second.effect_scope!=source)return false;
            gain_energy_locked(caster->second,effect->energy_gain,at);caster->second.outbox->enqueue(owner_vitals(caster->second));
        }
        // On-hit collateral can survive primary death; its captured centre and
        // exact scoped cohort remain independent of a reused raw target ID.
        if(prepared.collateral&&dagger_family_alive_draft_locked(source))
            commit_area_damage_pulse_locked(source,prepared.collateral_damage,at,*prepared.collateral,std::nullopt);
        for(unsigned index=0;index<effect->condition_count;++index){
            if(!dagger_family_alive_draft_locked(source)||!dagger_family_alive_draft_locked(target))break;
            const auto request=effect->conditions[index];
            if(const auto p=players_.find(target.actor);p!=players_.end()){
                if(p->second.effect_scope!=target)return false;auto next=p->second.conditions;
                if(!next.apply(request.kind,source.actor,at,request.base_ms))throw std::logic_error("admitted dagger condition failed");
                commit_conditions_locked(p->second,next,{request.kind},at);
            }else{
                const auto n=npc_combat_.find(target.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=target)break;
                const auto life=n->second.lifetime();if(!n->second.allows_conditions(life))continue;
                const auto flesh=combat::conditions::flesh_eligibility(request.kind,n->second.fleshy(life));
                if(!flesh)throw std::logic_error("admitted dagger flesh became unknown");if(!*flesh)continue;
                auto proposed=n->second;if(!proposed.apply_condition(life,request.kind,source.actor,at,request.base_ms))throw std::logic_error("admitted NPC dagger condition failed");
                auto check=proposed;for(const auto& reaction:check.take_fragility_hits(life))
                    if(reaction.reversal&&!validate_npc_reversal_receipts_locked(target,{reaction.reversal}))throw std::logic_error("dagger condition lost Reversal lease");
                n->second=std::move(proposed);npc_condition_credit_[{target.actor,source.actor}]={target,source};
                for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,n->second.actor(life)->effects)});
                const bool killed=publish_npc_fragility_locked(target.actor,n->second);
                const auto current=npc_combat_.find(target.actor);
                if(!killed&&current!=npc_combat_.end()&&current->second.hex_scope()==target)commit_applications_locked(target,at);
            }
        }
        if(effect->knockdown_ms&&dagger_family_alive_draft_locked(source)&&dagger_family_alive_draft_locked(target)){
            if(const auto p=players_.find(target.actor);p!=players_.end()){
                if(p->second.effect_scope==target&&p->second.knocked_down_until<=at&&!knockdown_player_locked(p->second,effect->knockdown_ms,at))throw std::logic_error("admitted dagger KD failed");
            }else{using K=combat::npcs::control::Knockdown;const auto result=knockdown_npc_locked(target,effect->knockdown_ms,at);
                if(result==K::invalid||result==K::unknown)throw std::logic_error("admitted NPC dagger KD failed");}
        }
        if(!dagger_family_alive_draft_locked(source)||!dagger_family_alive_draft_locked(target))return true;
        const auto caster=players_.find(source.actor);if(caster==players_.end()||caster->second.effect_scope!=source)return true;
        auto& chain=caster->second.dagger_chain;
        // Do not restore state reset by death or replace an unrelated chain.
        if(prepared.plan.strikes==2){
            const auto stage=chain.current(target,at);if(stage!=2&&stage!=3)return true;
        }
        const bool changed=chain.current(target,at)!=effect->result_stage;
        // Repeating976 adopts this existing15s refresh convention provisionally;
        // fixed repeated-hit receipts do not establish hidden expiry refresh.
        chain={target,effect->result_stage,at+15000};caster->second.iron_palm_wire_expiries[target]=chain.expires;
        if(changed)publication.stage(skills::effects::iron_palm_chain_2022(source.actor,target.actor,effect->result_stage));
        return true;
    }
// Current candidate policies still requiring retail contrasts before binding:
// primary-excluded156 collateral, per-strike Energy/conditions/KD, base condition
// duration scope, simultaneous two contacts, survivor-only chain and source-death
// effect suppression. These remain explicit owned simulation policies, not
// inferred completion from skill names or captured failed dual attempts.
