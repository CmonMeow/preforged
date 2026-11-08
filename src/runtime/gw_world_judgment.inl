// Included inside Instance. This handler uses the shared server AttackOutcomes
// proof;one bounded journal per qualified contact avoids imposing release order
// on projectile impacts. No network packet can call these private adapters.
private:
    std::map<std::pair<skills::effects::Scope,unsigned>,BackfireCredit> judgment_credit_;
    std::map<std::uint64_t,skills::effects::ShieldJudgmentContacts> judgment_contacts_;
    std::uint64_t judgment_failures_=0;
    struct RetiredJudgmentCaster {std::uint64_t team;std::uint64_t at;};
    std::map<skills::effects::Scope,RetiredJudgmentCaster> retired_judgment_casters_;
    std::map<skills::effects::Scope,unsigned> judgment_sources_in_flight_;
    void prune_retired_judgment_casters_locked(){
        std::erase_if(retired_judgment_casters_,[&](const auto& item){
            if(judgment_sources_in_flight_.contains(item.first))return false;
            for(const auto& [key,credit]:judgment_credit_)if(credit.caster==item.first)return false;
            for(const auto& [event,contact]:judgment_contacts_)if(contact.references_caster(item.first))return false;
            return true;
        });
    }
    void release_judgment_credit_locked(skills::effects::Scope target,unsigned effect){
        if(judgment_credit_.erase({target,effect}))prune_retired_judgment_casters_locked();
    }
    void prune_judgment_references_locked(){
        std::erase_if(judgment_credit_,[&](const auto& item){const auto& credit=item.second;std::optional<skills::effects::Entry> entry;
            if(const auto p=players_.find(credit.target.actor);p!=players_.end()&&p->second.effect_scope==credit.target&&p->second.effect_registry)entry=p->second.effect_registry->lookup(credit.target,credit.effect);
            else if(const auto n=npc_combat_.find(credit.target.actor);n!=npc_combat_.end()&&n->second.hex_scope()==credit.target)entry=n->second.hex_effect(n->second.lifetime(),credit.effect);
            // Expiry belongs to that recipient's clock/end receipt. Never
            // expire another actor's provenance using this caller's time.
            return !entry||entry->skill!=262||entry->cause!=credit.caster.actor;
        });
        prune_retired_judgment_casters_locked();
    }
    void retire_judgment_caster_locked(skills::effects::Scope scope,std::uint64_t now){
        prune_judgment_references_locked();const auto source=players_.find(scope.actor);
        if(source==players_.end()||source->second.effect_scope!=scope||!source->second.team_token)return;
        for(const auto& [key,credit]:judgment_credit_)if(credit.caster==scope){retired_judgment_casters_[scope]={source->second.team_token,now};return;}
    }
    std::optional<std::uint64_t> judgment_caster_team_locked(skills::effects::Scope caster,std::uint64_t now)const{
        if(const auto source=players_.find(caster.actor);source!=players_.end()&&source->second.effect_scope==caster&&source->second.team_token)return source->second.team_token;
        const auto retired=retired_judgment_casters_.find(caster);return retired!=retired_judgment_casters_.end()&&now>=retired->second.at?std::optional(retired->second.team):std::nullopt;
    }

    bool judgment_ready_locked(const skills::effects::ShieldJudgmentReaction& reaction){
        if(!judgment_caster_team_locked(reaction.caster,reaction.at))return false;
        if(const auto player=players_.find(reaction.attacker.actor);player!=players_.end()){
            auto& p=player->second;if(p.effect_scope!=reaction.attacker||!p.resources||reaction.at<p.health_updated||!advance_health_locked(p,reaction.at)||p.precise_health<=0)return false;
            auto actor=incoming_spell_actor_locked(p,{p.id,reaction.attacker.generation,combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true},reaction.at);
            const auto hit=combat::apply(actor,reaction.damage,true);
            if(hit.reason!=combat::Reason::accepted||!prepare_player_reactive_seed_locked(p,hit,reaction.at))return false;
            const auto current=players_.find(reaction.attacker.actor);
            return current!=players_.end()&&current->second.effect_scope==reaction.attacker&&current->second.precise_health>0&&
                (current->second.knocked_down_until>reaction.at||current->second.control_epoch!=UINT64_MAX);
        }
        const auto live=npc_combat_.find(reaction.attacker.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=reaction.attacker)return false;
        auto preview=live->second;const auto life=preview.lifetime();
        using K=combat::npcs::control::Knockdown;const auto knock=preview.knock_down(life,reaction.at,reaction.knockdown_ms);
        if(knock!=K::accepted&&knock!=K::immune&&knock!=K::already_down)return false;
        return reaction.damage.amount==0||preflight_npc_reversal_or_damage_locked(preview,reaction.damage,reaction.at);
    }
    bool prepare_judgment_contact_locked(const combat::weapon::QualifiedHit& hit){
        prune_judgment_references_locked();
        std::optional<skills::effects::Entry> entry;
        if(const auto p=players_.find(hit.target.actor);p!=players_.end()){
            if(p->second.effect_scope!=hit.target||!p->second.effect_registry)return true;
            entry=p->second.effect_registry->newest_skill(hit.target,262,hit.at);
        }else if(const auto n=npc_combat_.find(hit.target.actor);n!=npc_combat_.end()&&n->second.hex_scope()==hit.target){
            const auto registry=n->second.owned_effect_snapshot(n->second.lifetime());if(registry)entry=registry->newest_skill(hit.target,262,hit.at);
        }
        if(!entry)return true;
        const auto credit=judgment_credit_.find({hit.target,entry->instance});if(credit==judgment_credit_.end()||entry->cause!=credit->second.caster.actor)return false;
        const auto team=judgment_caster_team_locked(credit->second.caster,hit.at);if(!team)return false;
        std::optional<bool> hostile;skills::effects::HolyTraits traits;unsigned bonus=0;
        if(const auto p=players_.find(hit.attacker.actor);p!=players_.end()){
            if(p->second.effect_scope!=hit.attacker||!p->second.team_token||!p->second.reviewed_holy_traits||!p->second.reviewed_holy_bonus)return false;
            hostile=p->second.team_token!=*team;traits=*p->second.reviewed_holy_traits;bonus=*p->second.reviewed_holy_bonus;
        }else{const auto n=npc_combat_.find(hit.attacker.actor);const auto body=npcs_.find(hit.attacker.actor);
            if(n==npc_combat_.end()||body==npcs_.end()||n->second.hex_scope()!=hit.attacker||!body->second.combat_definition)return false;
            const auto relation=body->second.combat_definition->relation;if(relation==combat::npcs::Relation::unknown)return false;
            hostile=relation==combat::npcs::Relation::hostile_to_players;
            if(const auto known=n->second.holy_traits(n->second.lifetime()))traits=*known;
        }
        skills::effects::ShieldJudgmentContacts journal(hit.target);
        if(!journal.release(hit.target,hit.attacker,hit.event,hit.released,hit.delivery)||!journal.prepare(hit.target,credit->second.caster,*entry,hit,hostile,traits,credit->second.level))return false;
        const combat::weapon::ResolvedAttack terminal{hit.attacker,hit.target,hit.event,hit.released,hit.at,AttackOutcome::hit,hit.at,hit.delivery};
        auto reaction=skills::effects::shield_judgment_reaction(credit->second.caster,hit.target,*entry,terminal,hostile,traits,credit->second.level);if(!reaction)return false;
        if(*hostile)reaction->damage.amount+=bonus;
        if(!judgment_ready_locked(*reaction)||judgment_contacts_.size()>=4096)return false;
        // Equipment bonus is checked again at terminal;the journal owns the
        // canonical reaction,not a synthetic second attack event.
        return judgment_contacts_.emplace(hit.event,std::move(journal)).second;
    }
    void finish_judgment_contact_locked(const combat::weapon::ResolvedAttack& hit){
        const auto found=judgment_contacts_.find(hit.event);if(found==judgment_contacts_.end())return;
        auto journal=std::move(found->second);judgment_contacts_.erase(found);
        auto reaction=journal.consume(hit.target,hit);if(!reaction){prune_retired_judgment_casters_locked();return;}
        // The journal is consumed before callbacks. Keep only its exact source
        // metadata leased while nested effect endings prune their ownership.
        ++judgment_sources_in_flight_[reaction->caster];
        struct SourceLease {Instance& world;skills::effects::Scope caster;~SourceLease(){
            const auto held=world.judgment_sources_in_flight_.find(caster);
            if(held!=world.judgment_sources_in_flight_.end()&&--held->second==0)world.judgment_sources_in_flight_.erase(held);
            world.prune_retired_judgment_casters_locked();
        }} source_lease{*this,reaction->caster};
        auto source=players_.find(reaction->caster.actor);if(source!=players_.end()&&source->second.effect_scope!=reaction->caster)source=players_.end();
        if(!judgment_caster_team_locked(reaction->caster,hit.at)){++judgment_failures_;return;}
        auto player=players_.find(reaction->attacker.actor);
        if(player!=players_.end()){
            auto& p=player->second;if(p.effect_scope!=reaction->attacker||p.precise_health<=0)return;
            if(!p.reviewed_holy_bonus){++judgment_failures_;return;}if(reaction->damage.amount)reaction->damage.amount+=*p.reviewed_holy_bonus;
        }else{const auto n=npc_combat_.find(reaction->attacker.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=reaction->attacker||!n->second.actor(n->second.lifetime())->health||n->second.actor(n->second.lifetime())->health->current<=0)return;}
        if(!judgment_ready_locked(*reaction)){++judgment_failures_;return;}
        // Reactive admission settles nearby actors. Its source metadata lease
        // does not keep live map iterators valid across those callbacks.
        source=players_.find(reaction->caster.actor);
        if(source!=players_.end()&&source->second.effect_scope!=reaction->caster)source=players_.end();
        player=players_.find(reaction->attacker.actor);
        if(player!=players_.end()){
            if(player->second.effect_scope!=reaction->attacker||player->second.precise_health<=0)return;
        }else{
            const auto current=npc_combat_.find(reaction->attacker.actor);
            if(current==npc_combat_.end()||current->second.hex_scope()!=reaction->attacker)return;
        }
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::SubCapture>> captures;for(auto& [box,queue]:recipients)captures.push_back(std::make_unique<Outbox::SubCapture>(*box,queue));
        if(player!=players_.end()){
            auto& p=player->second;
            if(p.knocked_down_until<=hit.at&&!knockdown_player_locked(p,reaction->knockdown_ms,hit.at))throw std::logic_error("preflighted Judgment knockdown failed");
            if(reaction->damage.amount){auto actor=incoming_spell_actor_locked(p,{p.id,reaction->attacker.generation,combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true},hit.at);
                const auto damage=combat::apply(actor,reaction->damage,true);if(damage.reason!=combat::Reason::accepted)throw std::logic_error("preflighted Judgment damage failed");
                const auto cause=source!=players_.end()?std::optional<unsigned>(reaction->caster.actor):std::nullopt;
                if(!commit_player_reactive_hit_locked(p,damage,cause,hit.at,false,nullptr,false,false))
                    throw std::logic_error("preflighted Judgment protection commit failed");}
        }else{
            auto& n=npc_combat_.at(reaction->attacker.actor);const auto life=n.lifetime();auto preview=n;
            using K=combat::npcs::control::Knockdown;const auto knock=preview.knock_down(life,hit.at,reaction->knockdown_ms);
            if(knock==K::accepted&&knockdown_npc_locked(reaction->attacker,reaction->knockdown_ms,hit.at)!=K::accepted)throw std::logic_error("preflighted Judgment NPC knockdown failed");
            if(reaction->damage.amount){
                if(!commit_npc_reversal_or_damage_locked(source!=players_.end()?&source->second:nullptr,
                        npcs_.at(reaction->attacker.actor),n,reaction->damage,hit.at))
                    throw std::logic_error("preflighted Judgment NPC protection commit failed");
            }
        }
        captures.clear();for(auto& [box,queue]:recipients)box->enqueue(std::move(queue.packets));
    }
public:
    bool shield_judgment_caster_ready(skills::effects::Scope caster,std::uint64_t now)const{
        std::lock_guard lock(mutex_);const auto source=players_.find(caster.actor);
        return source!=players_.end()&&source->second.effect_scope==caster&&source->second.ordinary_level>=1&&source->second.ordinary_level<=255&&now>=source->second.health_updated;
    }
    // Trusted effect stage for a player's exact owned quest guardian. Normal
    // ally casts must keep their stricter allegiance/admission path.
    bool stage_guardian_shield_judgment(skills::effects::Scope caster,skills::effects::Scope target,
            const skills::effects::ShieldJudgmentPlan& plan,std::uint64_t now){
        const auto canonical=skills::effects::shield_judgment(plan.rank);if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);const auto source=players_.find(caster.actor);auto npc=npc_combat_.find(target.actor);
        if(source==players_.end()||source->second.effect_scope!=caster||source->second.ordinary_level<1||source->second.ordinary_level>255||
           npc==npc_combat_.end()||npc->second.hex_scope()!=target||next_npc_hex_==UINT32_MAX)return false;
        if(std::none_of(source->second.quest_waves.begin(),source->second.quest_waves.end(),[&](const auto& item){return item.second.guardian()==target;}))return false;
        advance_npc_conditions_locked(target.actor,npc->second,now);npc=npc_combat_.find(target.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=target)return false;
        auto proposed=npc->second;const auto previous=proposed.owned_effect_snapshot(proposed.lifetime());if(!previous)return false;
        const auto effect=next_npc_hex_;if(!proposed.apply_shield_judgment(proposed.lifetime(),caster,plan,effect,now))return false;
        std::vector<Packet> packets;for(const auto& old:previous->active(target,now))if(old.skill==262){packets.push_back(health_effect_remove(target.actor,old.instance));npc_simple_enchantment_credit_.erase({target.actor,old.instance});}
        npc->second=std::move(proposed);++next_npc_hex_;std::erase_if(judgment_credit_,[&](const auto& item){return item.second.target==target;});
        judgment_credit_[{target,effect}]={target,caster,effect,source->second.ordinary_level};prune_retired_judgment_casters_locked();npc_simple_enchantment_credit_[{target.actor,effect}]=target;
        packets.push_back(health_effect_add(*npc->second.hex_effect(npc->second.lifetime(),effect),target.actor,now));for(auto& [id,peer]:players_)peer.outbox->enqueue(packets);return true;
    }
    std::pair<std::size_t,std::size_t> shield_judgment_ownership()const{std::lock_guard lock(mutex_);return {judgment_credit_.size(),retired_judgment_casters_.size()};}
    std::size_t shield_judgment_pending_contacts()const{std::lock_guard lock(mutex_);return judgment_contacts_.size();}
    std::uint64_t shield_judgment_failures()const{std::lock_guard lock(mutex_);return judgment_failures_;}
