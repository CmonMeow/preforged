// UNINCLUDED INTEGRATION DRAFT. Include inside Instance after Player/Outbox
// definitions; the translation unit must include gw_dagger_attack_policy.h.
// Existing paid weapon transaction retains all damage/retaliation authority.
private:
    // Run BEFORE allocating/releasing AttackResolution as well as after
    // qualification: unknown secondary facts must not consume release/hit
    // reactions before returning a refusal. This probe mutates only copies.
    bool dagger_secondary_ready_draft_locked(skills::effects::Scope source,
            skills::effects::Scope target,const combat::weapon::DaggerAttackPolicy& plan,std::uint64_t now){
        if(!combat::weapon::canonical(plan))return false;
        if(!plan.bleeding_ms)return true;
        if(const auto player=players_.find(target.actor);player!=players_.end()){
            if(player->second.effect_scope!=target||player->second.precise_health<=0||player->second.next_health_effect>UINT32_MAX)return false;
            auto conditions=player->second.conditions;
            return conditions.apply(combat::conditions::Kind::bleeding,source.actor,now,plan.bleeding_ms);
        }
        const auto npc=npc_combat_.find(target.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=target)return false;
        const auto life=npc->second.lifetime();const auto* actor=npc->second.actor(life);
        if(!actor||!actor->health||actor->health->current<=0)return false;
        if(!npc->second.allows_conditions(life))return true;
        const auto flesh=combat::conditions::flesh_eligibility(combat::conditions::Kind::bleeding,npc->second.fleshy(life));
        if(!flesh)return false;if(!*flesh)return true;
        auto probe=npc->second;if(!probe.apply_condition(life,combat::conditions::Kind::bleeding,source.actor,now,plan.bleeding_ms))return false;
        for(const auto& reaction:probe.take_fragility_hits(life))
            if(reaction.reversal&&!validate_npc_reversal_receipts_locked(target,{reaction.reversal}))return false;
        return true;
    }
    class DaggerContactCommitDraft {
        friend class Instance;
        combat::weapon::DaggerContactProposal proposal_;
        unsigned item_=0;std::uint64_t revision_=0;
        bool bleed_=false;
        DaggerContactCommitDraft(combat::weapon::DaggerContactProposal p,unsigned item,
                std::uint64_t revision,bool bleed):proposal_(std::move(p)),item_(item),revision_(revision),bleed_(bleed){}
    public:
        ~DaggerContactCommitDraft()=default;
        DaggerContactCommitDraft(const DaggerContactCommitDraft&)=delete;
        DaggerContactCommitDraft& operator=(const DaggerContactCommitDraft&)=delete;
    };
    // Start immediately AFTER outcome.qualify_hit and its reaction callbacks,
    // BEFORE any primary damage publication. Nested SubCapture preserves outer
    // Runtime publication. Failed exits still flush all authoritative feedback.
    class DaggerContactPublicationDraft {
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues_;
        std::vector<std::unique_ptr<Outbox::SubCapture>> holds_;
        std::optional<Packet> stage_;
    public:
        explicit DaggerContactPublicationDraft(Instance& world){
            std::set<const Outbox*> seen;
            for(auto&[id,peer]:world.players_)if(seen.insert(peer.outbox.get()).second)
                queues_.emplace_back(peer.outbox,Outbox::DeferredQueue{});
            for(auto&[box,queue]:queues_)holds_.push_back(std::make_unique<Outbox::SubCapture>(*box,queue));
        }
        DaggerContactPublicationDraft(const DaggerContactPublicationDraft&)=delete;
        void stage(Packet packet){stage_=std::move(packet);}
        ~DaggerContactPublicationDraft(){
            holds_.clear();
            for(auto&[box,queue]:queues_){
                if(stage_)queue.packets.insert(queue.packets.begin(),*stage_);
                box->enqueue(std::move(queue.packets));
            }
        }
    };
    // Caller holds mutex_, provides the ACTUAL one-time QualifiedHit from the
    // existing AttackResolution and calls only after accuracy/block. No new
    // public API accepts receipts. Source/target/equipment are reacquired after
    // qualify_hit's Insidious/Endurance/reaction preparation.
    std::unique_ptr<DaggerContactCommitDraft> prepare_dagger_contact_draft_locked(
            const combat::weapon::QualifiedHit& qualified,
            const combat::weapon::DaggerAttackPolicy& plan){
        const auto source=players_.find(qualified.attacker.actor);
        if(source==players_.end()||source->second.effect_scope!=qualified.attacker||
           !source->second.resources||source->second.precise_health<=0||!source->second.ordinary_weapon||
           source->second.knocked_down_until>qualified.at||qualified.at<source->second.health_updated)return {};
        const auto& weapon=*source->second.ordinary_weapon;
        const auto proposal=combat::weapon::dagger_contact_proposal(plan,source->second.dagger_chain,
            qualified,weapon.item_type,weapon.caster_weapon);
        if(!proposal)return {};
        bool bleed=proposal->bleeding_ms!=0;
        const auto player=players_.find(qualified.target.actor);
        if(player!=players_.end()){
            if(player->second.effect_scope!=qualified.target||!player->second.resources||player->second.precise_health<=0||
               qualified.at<player->second.health_updated)return {};
            const auto snapshot=ordinary_snapshot_locked(source->second,player->second,qualified.at);
            if(!snapshot.alive||!snapshot.target_alive||!snapshot.hostile||!snapshot.line_of_sight||snapshot.distance>128)return {};
            if(bleed){auto conditions=player->second.conditions;
                if(player->second.next_health_effect>UINT32_MAX||
                   !conditions.apply(combat::conditions::Kind::bleeding,qualified.attacker.actor,qualified.at,proposal->bleeding_ms))return {};
            }
        }else{
            const auto npc=npc_combat_.find(qualified.target.actor);
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=qualified.target)return {};
            const auto life=npc->second.lifetime();const auto* actor=npc->second.actor(life);
            if(!actor||!actor->health||actor->health->current<=0||qualified.at<npc->second.health_clock(life).value_or(qualified.at))return {};
            const auto placed=npcs_.find(qualified.target.actor);if(placed==npcs_.end())return {};
            const auto snapshot=ordinary_npc_snapshot_locked(source->second,placed->second,npc->second,qualified.at);
            if(!snapshot.alive||!snapshot.target_alive||!snapshot.hostile||!snapshot.line_of_sight||snapshot.distance>128)return {};
            bleed=bleed&&npc->second.allows_conditions(life);
            if(bleed){const auto flesh=combat::conditions::flesh_eligibility(combat::conditions::Kind::bleeding,npc->second.fleshy(life));
                if(!flesh)return {};bleed=*flesh;
            }
            if(bleed){auto probe=npc->second;
                if(!probe.apply_condition(life,combat::conditions::Kind::bleeding,qualified.attacker.actor,qualified.at,proposal->bleeding_ms))return {};
                for(const auto& hit:probe.take_fragility_hits(life))
                    if(hit.reversal&&!validate_npc_reversal_receipts_locked(qualified.target,{hit.reversal}))return {};
            }
        }
        return std::unique_ptr<DaggerContactCommitDraft>(new DaggerContactCommitDraft(*proposal,weapon.item,weapon.revision,bleed));
    }
    // Call once only AFTER original paid branch has committed primary damage
    // and its full owned reaction cohort, outcome.resolve(hit) and preparation
    // consumers. No caller references may be used after this helper. Taking
    // the unique_ptr consumes the local capability even on refusal/death.
    // Successful-primary evidence is supplied by that original branch, never
    // inferred from a standalone damage modifier or Runtime cast completion.
    bool commit_dagger_contact_draft_locked(std::unique_ptr<DaggerContactCommitDraft> commit,
            DaggerContactPublicationDraft& publication){
        if(!commit)return false;
        const auto p=commit->proposal_;const auto now=p.at;
        auto source=players_.find(p.source.actor);
        if(source==players_.end()||source->second.effect_scope!=p.source||source->second.precise_health<=0||
           !source->second.ordinary_weapon||source->second.ordinary_weapon->item!=commit->item_||
           source->second.ordinary_weapon->revision!=commit->revision_||
           !combat::weapon::dagger_chain_matches(source->second.dagger_chain,p.before))return true;
        const auto alive=[&](){
            if(const auto victim=players_.find(p.target.actor);victim!=players_.end())
                return victim->second.effect_scope==p.target&&victim->second.precise_health>0;
            const auto npc=npc_combat_.find(p.target.actor);
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=p.target)return false;
            const auto* actor=npc->second.actor(npc->second.lifetime());return actor&&actor->health&&actor->health->current>0;
        };
        if(!alive())return true;
        if(commit->bleed_){
            const auto player=players_.find(p.target.actor);
            if(player!=players_.end()){
                auto conditions=player->second.conditions;
                if(!conditions.apply(combat::conditions::Kind::bleeding,p.source.actor,now,p.bleeding_ms))
                    throw std::logic_error("preflighted Jagged player Bleeding failed");
                commit_conditions_locked(player->second,conditions,{combat::conditions::Kind::bleeding},now);
            }else{
                auto npc=npc_combat_.find(p.target.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=p.target)return true;
                const auto life=npc->second.lifetime();auto proposed=npc->second;
                if(!proposed.apply_condition(life,combat::conditions::Kind::bleeding,p.source.actor,now,p.bleeding_ms))
                    throw std::logic_error("preflighted Jagged NPC Bleeding failed");
                auto check=proposed;
                for(const auto& hit:check.take_fragility_hits(life))
                    if(hit.reversal&&!validate_npc_reversal_receipts_locked(p.target,{hit.reversal}))
                        throw std::logic_error("Jagged NPC condition lost owned Reversal receipt");
                npc->second=std::move(proposed);npc_condition_credit_[{p.target.actor,p.source.actor}]={p.target,p.source};
                const auto* actor=npc->second.actor(life);
                for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(p.target.actor,actor->effects),
                    combat::detail::fraction(p.target.actor,*actor->health)});
                const bool killed=publish_npc_fragility_locked(p.target.actor,npc->second);
                const auto current=npc_combat_.find(p.target.actor);
                if(!killed&&current!=npc_combat_.end()&&current->second.hex_scope()==p.target)
                    commit_applications_locked(p.target,now);
            }
        }
        source=players_.find(p.source.actor);
        if(!alive()||source==players_.end()||source->second.effect_scope!=p.source||source->second.precise_health<=0||
           !source->second.ordinary_weapon||source->second.ordinary_weapon->item!=commit->item_||
           source->second.ordinary_weapon->revision!=commit->revision_||
           !combat::weapon::dagger_chain_matches(source->second.dagger_chain,p.before))return true;
        // Current survivor-only private policy; lethal retail stage remains open.
        source->second.dagger_chain=p.after;
        source->second.iron_palm_wire_expiries[p.target]=p.after.expires;
        if(p.publish_stage)publication.stage(skills::effects::iron_palm_chain_2022(p.source.actor,p.target.actor,p.after.stage));
        return true;
    }
// Required actual caller patch (not applied here):
// 1 canonical AttackPlan/support/rank/condition metadata integration; admission
//   and contact eligibility; Fox unblockable at BOTH player/NPC block gates.
// 2 after each actual outcome.qualify_hit, construct DaggerContactPublicationDraft
//   and prepare token; on refusal return cancellation, preserving callback wires.
// 3 execute unchanged primary weapon damage and all existing reactions.
// 4 commit token after finish(hit), preparation and resource consumers, as the
//   last action before successful return; no old actor references used afterward.
//   Do NOT also add782 to generic condition application or it applies twice.
// 5 scope publication until after finish, and prove kind46/91/damage native order.
// This draft intentionally does not call public apply_condition (it locks again),
// does not fabricate a Hit, does not own cost/recharge or bypass attack ledger.
// Bleeding gear modifiers are not yet supported by shared equipment helper;
// this draft retains current attack-family base-duration scope, requiring review.
