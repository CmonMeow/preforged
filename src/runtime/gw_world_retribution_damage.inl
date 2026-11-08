private:
    struct RetributionContact {
        skills::effects::RetributionContacts journal;
        skills::effects::RetributionLease lease;
        unsigned level;
    };
    std::map<std::uint64_t,RetributionContact> retribution_contacts_;
    std::uint64_t retribution_damage_failures_=0;
    void fail_retribution_damage_locked(){if(retribution_damage_failures_!=UINT64_MAX)++retribution_damage_failures_;}
    struct RetributionFacts {bool alive=false,hostile=false;skills::effects::HolyTraits traits;unsigned holy_bonus=0;};
    std::optional<RetributionFacts> retribution_attacker_locked(skills::effects::Scope caster,skills::effects::Scope attacker){
        const auto source=players_.find(caster.actor);
        const auto npc_side=source==players_.end()?retribution_npc_side_locked(caster):std::optional<bool>{};
        if(source!=players_.end()){if(source->second.effect_scope!=caster||!source->second.team_token)return {};}
        else if(!npc_side||!retribution_caster_level_locked(caster))return {};
        if(const auto p=players_.find(attacker.actor);p!=players_.end()){
            if(p->second.effect_scope!=attacker||!p->second.resources)return {};
            if(p->second.precise_health<=0)return RetributionFacts{};
            if(!p->second.team_token||!p->second.reviewed_holy_traits||!p->second.reviewed_holy_traits->light_sensitive||!p->second.reviewed_holy_bonus)return {};
            return RetributionFacts{true,source!=players_.end()?p->second.team_token!=source->second.team_token:*npc_side,*p->second.reviewed_holy_traits,*p->second.reviewed_holy_bonus};
        }
        const auto n=npc_combat_.find(attacker.actor);const auto body=npcs_.find(attacker.actor);
        if(n==npc_combat_.end()||body==npcs_.end()||n->second.hex_scope()!=attacker||!body->second.combat_definition)return {};
        const auto actor=n->second.actor(n->second.lifetime());if(!actor||!actor->health)return {};
        if(actor->health->current<=0)return RetributionFacts{};
        const auto traits=n->second.holy_traits(n->second.lifetime());const auto relation=body->second.combat_definition->relation;
        if(!traits||!traits->light_sensitive||relation==combat::npcs::Relation::unknown)return {};
        if(source==players_.end()){const auto attacker_side=retribution_npc_side_locked(attacker);if(!attacker_side)return {};
            return RetributionFacts{true,*attacker_side!=*npc_side,*traits,0};}
        return RetributionFacts{true,relation==combat::npcs::Relation::hostile_to_players,*traits,0};
    }
    bool prepare_retribution_contact_locked(const combat::weapon::QualifiedHit& hit){
        const auto target=players_.find(hit.target.actor);std::optional<skills::effects::Registry> npc_registry;
        const skills::effects::Registry* registry=nullptr;
        if(target!=players_.end()){
            if(target->second.effect_scope!=hit.target||!target->second.effect_registry)return false;registry=&*target->second.effect_registry;
        }else{
            const auto npc=npc_combat_.find(hit.target.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=hit.target)return false;
            npc_registry=npc->second.owned_effect_snapshot(npc->second.lifetime());if(!npc_registry)return false;registry=&*npc_registry;
        }
        const auto entry=registry->newest_skill(hit.target,248,hit.at);if(!entry)return true;
        const auto lease=retribution_leases_.find({hit.target,entry->instance});if(lease==retribution_leases_.end())return false;
        const auto level=retribution_caster_level_locked(lease->second.caster);
        if(!level||
           !retribution_attacker_locked(lease->second.caster,hit.attacker)||retribution_contacts_.size()>=4096)return false;
        skills::effects::RetributionContacts journal(hit.target);
        if(!journal.release(hit.target,hit.attacker,hit.event,hit.released,hit.delivery)||
           !journal.prepare(hit.target,lease->second,hit,*registry,mending_ownership_))return false;
        return retribution_contacts_.emplace(hit.event,RetributionContact{std::move(journal),lease->second,*level}).second;
    }
    // Hold base-hit publication locally, including inside an existing AoE frame.
    // Return-damage notifications precede the held attack notification, matching
    // the bounded retail receipt, while Health still commits through its normal
    // authoritative base-hit and reactive protection pipelines.
    class RetributionPublication {
        struct Row {std::shared_ptr<Outbox> box;Outbox::DeferredQueue base,reaction;};
        std::vector<Row> rows_;
        std::vector<std::unique_ptr<Outbox::SubCapture>> captures_;
    public:
        RetributionPublication(Instance& world,bool active){
            if(!active)return;std::set<const Outbox*> seen;
            for(auto&[id,peer]:world.players_)if(seen.insert(peer.outbox.get()).second)rows_.push_back({peer.outbox,{},{}});
            for(auto& row:rows_)captures_.push_back(std::make_unique<Outbox::SubCapture>(*row.box,row.base));
        }
        void reaction(){captures_.clear();for(auto& row:rows_)captures_.push_back(std::make_unique<Outbox::SubCapture>(*row.box,row.reaction));}
        void flush(){
            captures_.clear();for(auto& row:rows_){auto batch=std::move(row.reaction.packets);
                batch.insert(batch.end(),std::make_move_iterator(row.base.packets.begin()),std::make_move_iterator(row.base.packets.end()));row.box->enqueue(std::move(batch));}
            rows_.clear();
        }
    };
    void commit_retribution_damage_locked(const combat::weapon::QualifiedHit& qualified,const std::optional<combat::Hit>& actual){
        const auto found=retribution_contacts_.find(qualified.event);if(found==retribution_contacts_.end())return;
        auto contact=std::move(found->second);retribution_contacts_.erase(found); // Consume before all callbacks.
        if(!actual)return; // Conversion healing killed the recipient; no attack damage was dealt.
        const auto owner=attack_outcomes_.find(qualified.attacker);
        const auto receipt=owner==attack_outcomes_.end()?std::optional<combat::weapon::CommittedAttackDamage>{}:
            owner->second.commit_damage(qualified.attacker,qualified.target,qualified.event,*actual,qualified.at);
        if(!receipt){fail_retribution_damage_locked();return;}
        auto facts=retribution_attacker_locked(contact.lease.caster,qualified.attacker);
        if(!facts){fail_retribution_damage_locked();return;}if(!facts->alive)return;
        auto reaction=contact.journal.consume_damage(qualified.target,*receipt,facts->hostile,facts->traits,contact.level);
        if(!reaction)return;
        reaction->damage.amount+=facts->holy_bonus;
        const auto target=players_.find(qualified.attacker.actor);
        if(target!=players_.end()){
            if(target->second.effect_scope!=qualified.attacker||!advance_health_locked(target->second,qualified.at)||target->second.precise_health<=0)return;
            auto& p=target->second;
            auto actor=incoming_spell_actor_locked(p,{p.id,p.effect_scope.generation,combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true},qualified.at);
            auto hit=combat::apply(actor,reaction->damage,true);
            const auto seed=hit.reason==combat::Reason::accepted?prepare_player_reactive_seed_locked(p,hit,qualified.at):std::nullopt;
            if(!seed){fail_retribution_damage_locked();return;}
            const auto current=players_.find(qualified.attacker.actor);
            if(current==players_.end()||current->second.effect_scope!=qualified.attacker||current->second.precise_health<=0||!retribution_caster_level_locked(contact.lease.caster))return;
            const auto fresh=retribution_attacker_locked(contact.lease.caster,qualified.attacker);
            const auto proposal=fresh&&fresh->alive?skills::effects::retribution_reaction(contact.lease,receipt->attack(),receipt->hit(),fresh->hostile,fresh->traits,contact.level):std::nullopt;
            if(!proposal||proposal->damage.amount+fresh->holy_bonus!=reaction->damage.amount){fail_retribution_damage_locked();return;}
            auto& recipient=current->second;
            actor=incoming_spell_actor_locked(recipient,{recipient.id,qualified.attacker.generation,combat::Health{recipient.precise_health,recipient.resources->max_health},{},recipient.effects,true},qualified.at);
            hit=combat::apply(actor,reaction->damage,true);
            if(hit.reason!=combat::Reason::accepted||!commit_player_reactive_hit_locked(recipient,hit,contact.lease.caster.actor,qualified.at,false,&*seed,false,false,nullptr,contact.lease.caster))fail_retribution_damage_locked();
        }else{
            auto live=npc_combat_.find(qualified.attacker.actor);
            if(live==npc_combat_.end()||live->second.hex_scope()!=qualified.attacker)return;
            advance_npc_conditions_locked(qualified.attacker.actor,live->second,qualified.at);
            live=npc_combat_.find(qualified.attacker.actor);if(live==npc_combat_.end()||live->second.hex_scope()!=qualified.attacker)return;
            auto preview=live->second;
            const bool opposed=!live->second.hostile_to_players(live->second.lifetime());
            std::optional<combat::Hit> projected;
            if(!preflight_npc_reversal_or_damage_locked(preview,reaction->damage,qualified.at,nullptr,&projected,nullptr,nullptr,opposed)){
                fail_retribution_damage_locked();return;
            }
            // Seed responds to authoritative remaining loss after conversion,
            // rather than the raw holy amount or unrelated Health changes.
            const double cohort_loss=projected?projected->applied:0.;
            const auto cohort_registry=live->second.owned_effect_snapshot(live->second.lifetime());
            const auto cohort_effect=cohort_registry?cohort_registry->newest_skill(qualified.attacker,274,qualified.at):std::nullopt;
            const auto seed=prepare_npc_healing_seed_direct_locked(qualified.attacker,cohort_loss,qualified.at);
            if(!seed){fail_retribution_damage_locked();return;}
            auto body=npcs_.end();
            const auto refresh_return=[&](){
                live=npc_combat_.find(qualified.attacker.actor);body=npcs_.find(qualified.attacker.actor);
                if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=qualified.attacker||
                   !retribution_caster_level_locked(contact.lease.caster))return false;
                const auto fresh=retribution_attacker_locked(contact.lease.caster,qualified.attacker);
                const auto proposal=fresh&&fresh->alive?skills::effects::retribution_reaction(contact.lease,receipt->attack(),receipt->hit(),fresh->hostile,fresh->traits,contact.level):std::nullopt;
                return proposal&&proposal->damage.amount+fresh->holy_bonus==reaction->damage.amount&&
                    opposed==!live->second.hostile_to_players(live->second.lifetime());
            };
            if(!refresh_return())return;
            // Replan even an empty/no-effect cohort: a zero->positive change
            // must not bypass the original Seed eligibility preflight.
            auto current_preview=live->second;
            if(!preflight_npc_reversal_or_damage_locked(current_preview,reaction->damage,qualified.at,nullptr,&projected,nullptr,nullptr,opposed)){
                fail_retribution_damage_locked();return;
            }
            // Keep no iterator/reference across either fallible preflight.
            if(!refresh_return())return;
            const double current_loss=projected?projected->applied:0.;
            if(current_loss!=cohort_loss){fail_retribution_damage_locked();return;}
            if(current_loss>0){
                const auto registry=live->second.owned_effect_snapshot(live->second.lifetime());
                const auto active=registry?registry->newest_skill(qualified.attacker,274,qualified.at):std::nullopt;
                if(seed->effect){
                    if(!active||active->instance!=seed->effect||active->cause!=seed->cause){fail_retribution_damage_locked();return;}
                }else{
                    // An already-stale Seed caster legitimately yields no
                    // cohort. A newly added/replaced effect needs new admission.
                    if(bool(active)!=bool(cohort_effect)||
                       (active&&(active->instance!=cohort_effect->instance||active->cause!=cohort_effect->cause||
                                 active->rank!=cohort_effect->rank||active->applied!=cohort_effect->applied))){
                        fail_retribution_damage_locked();return;
                    }
                }
            }
            const auto caster=players_.find(contact.lease.caster.actor);
            Player* credit=caster!=players_.end()&&caster->second.effect_scope==contact.lease.caster?&caster->second:nullptr;
            std::optional<combat::Hit> actual;
            if(!commit_npc_reversal_or_damage_locked(credit,body->second,live->second,reaction->damage,qualified.at,nullptr,nullptr,&actual,true,contact.lease.caster,nullptr,opposed)){
                fail_retribution_damage_locked();return;
            }
            // A lethal recipient cannot be revived, but its adjacent Seed
            // cohort still receives the positive-loss trigger before cleanup.
            if(actual&&actual->applied>0&&seed->effect)publish_healing_seed_direct_locked(*seed,qualified.at);
            live=npc_combat_.find(qualified.attacker.actor);body=npcs_.find(qualified.attacker.actor);
            if(live!=npc_combat_.end()&&body!=npcs_.end()&&live->second.hex_scope()==qualified.attacker){
                const auto* actor=live->second.actor(live->second.lifetime());
                if(actor&&actor->health&&actor->health->current<=0){
                    const auto current_caster=players_.find(contact.lease.caster.actor);
                    Player* current_credit=current_caster!=players_.end()&&current_caster->second.effect_scope==contact.lease.caster?&current_caster->second:nullptr;
                    record_npc_death_locked(current_credit,body->second,live->second,qualified.at);
                }
            }
        }
    }
    void finish_retribution_contact_locked(const combat::weapon::ResolvedAttack& terminal){
        if(retribution_contacts_.erase(terminal.event)&&terminal.outcome==AttackOutcome::hit&&terminal.before_damage)fail_retribution_damage_locked();
    }
public:
    std::uint64_t retribution_damage_failures()const{std::lock_guard lock(mutex_);return retribution_damage_failures_;}
    std::size_t retribution_pending_damage_contacts()const{std::lock_guard lock(mutex_);return retribution_contacts_.size();}
