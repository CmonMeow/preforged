// Included inside Instance. Trusted staged APIs,not a paid307 cast binding.
private:
    // The simple-enchantment map owns recipient publication, not caster
    // identity. Retain external caster generations independently of actor IDs.
    std::map<std::pair<skills::effects::Scope,unsigned>,skills::effects::Scope> reversal_caster_credit_;
    bool valid_reversal_owner_locked(skills::effects::Scope target,const skills::effects::Entry& entry)const{
        if(const auto player=players_.find(target.actor);player!=players_.end()){
            if(player->second.effect_scope!=target||!player->second.effect_registry||!player->second.effect_registry->matches(target))return false;
        }else{
            const auto owner=npc_simple_enchantment_credit_.find({target.actor,entry.instance});
            if(owner==npc_simple_enchantment_credit_.end()||owner->second!=target)return false;
        }
        const auto caster=reversal_caster_credit_.find({target,entry.instance});
        return caster!=reversal_caster_credit_.end()?
            caster->second.instance==target.instance&&caster->second.actor==entry.cause:entry.cause==target.actor;
    }
    std::optional<skills::effects::Scope> reversal_caster_scope_locked(skills::effects::Scope target,const skills::effects::Entry& entry)const{
        const auto recorded=reversal_caster_credit_.find({target,entry.instance});
        const auto caster=recorded==reversal_caster_credit_.end()?target:recorded->second;
        return caster.actor==entry.cause?std::optional(caster):std::nullopt;
    }
    combat::npcs::ReversalHealingSource reversal_healing_source_locked(skills::effects::Scope caster)const{
        if(const auto player=players_.find(caster.actor);player!=players_.end())
            return {player->second.effect_scope==caster?std::optional<unsigned>(caster.actor):std::nullopt};
        if(const auto npc=npc_combat_.find(caster.actor);npc!=npc_combat_.end())
            return {npc->second.hex_scope()==caster?std::optional<unsigned>(caster.actor):std::nullopt};
        return {};
    }
    combat::npcs::ReversalHealingSource reversal_healing_source_locked(skills::effects::Scope target,const skills::effects::Entry& entry)const{
        const auto caster=reversal_caster_scope_locked(target,entry);
        return caster?reversal_healing_source_locked(*caster):combat::npcs::ReversalHealingSource{};
    }
    void prune_reversal_caster_credit_locked(std::uint64_t now){
        std::erase_if(reversal_caster_credit_,[&](const auto& credit){
            const auto& [target,effect]=credit.first;
            if(const auto player=players_.find(target.actor);player!=players_.end()){
                if(player->second.effect_scope!=target||!player->second.effect_registry)return true;
                const auto entry=player->second.effect_registry->lookup(target,effect);
                return !entry||entry->skill!=307||entry->cause!=credit.second.actor||!entry->expires||now>=*entry->expires;
            }
            const auto current=npc_combat_.find(target.actor);
            if(current==npc_combat_.end()||current->second.hex_scope()!=target)return true;
            const auto entry=current->second.hex_effect(current->second.lifetime(),effect);
            return !entry||entry->skill!=307||entry->cause!=credit.second.actor||!entry->expires||now>=*entry->expires;
        });
    }
    bool validate_npc_reversal_receipts_locked(skills::effects::Scope target,
            const std::vector<std::optional<skills::effects::ReversalDamageReceipt>>& receipts)const{
        for(const auto& receipt:receipts)if(receipt){
            if(receipt->quote.target!=target||!valid_reversal_owner_locked(target,receipt->removed))return false;
        }
        return true;
    }
    struct NpcWeaponHit {
        combat::npcs::LiveActor actor;
        std::optional<combat::Hit> actual;
        std::optional<combat::npcs::LiveActor::ReversalDamage> reversal;
    };
    std::optional<NpcWeaponHit> prepare_npc_weapon_hit_locked(const combat::npcs::LiveActor& current,
            combat::npcs::LiveActor proposed,const combat::Hit& incoming,std::uint64_t now){
        const auto life=current.lifetime();const auto scope=current.hex_scope();
        const auto registry=current.owned_effect_snapshot(life);
        if(incoming.reason!=combat::Reason::accepted||incoming.target!=scope.actor||incoming.generation!=scope.generation)return {};
        if(incoming.damage>0&&registry&&registry->newest_skill(scope,307,now)){
            auto accepted_incoming=std::move(proposed);proposed=current;const auto receipt=proposed.apply_reversal_weapon_hit(life,incoming,now);
            if(!receipt||!validate_npc_reversal_receipts_locked(scope,{receipt})||!proposed.adopt_absorption_hit(current,accepted_incoming,life,now))return {};
            return NpcWeaponHit{std::move(proposed),receipt->damage,receipt};
        }
        return NpcWeaponHit{std::move(proposed),incoming,{}};
    }
    void publish_npc_weapon_hit_locked(Player& source,combat::npcs::LiveActor& live,const NpcWeaponHit& result,
            bool critical,bool armor_respecting,std::uint64_t now){
        const auto scope=live.hex_scope();std::vector<Packet> packets;
        if(result.reversal){
            npc_simple_enchantment_credit_.erase({scope.actor,result.reversal->removed.instance});
            packets=combat::npcs::reversal_weapon_packets_2022(scope.actor,source.id,*result.reversal,critical,reversal_healing_source_locked(scope,result.reversal->removed));
            packets.push_back(emotes::effects_2022(scope.actor,live.actor(live.lifetime())->effects));
        }else if(result.actual){
            const auto& hit=*result.actual;
            packets=hit.skill?combat::damage_packets_2022(source.id,hit,critical):combat::weapon::packets_2022(source.id,hit,critical);
        }
        if(result.actual)consume_watch_yourself_npc_hit_locked(live,*result.actual,armor_respecting,now);
        for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
        if(result.actual&&result.actual->applied>0&&!result.actual->killed)provoke_npc_locked(scope,source.effect_scope,now);
        if(result.actual&&result.actual->applied>0)bonds_npc_caster_damage_locked(scope,result.actual->applied,now,nullptr);
        // Contact owner records death after its other landed-hit reactions.
    }
    void publish_npc_elemental_hits_locked(Player& source,combat::npcs::LiveActor& live,
            const skills::effects::ElementalResult& result,std::uint64_t now){
        const auto scope=live.hex_scope();const auto source_id=source.id;const auto source_scope=source.effect_scope;
        for(std::size_t index=0;index<result.hits.size();++index){
            const auto current=npc_combat_.find(scope.actor);
            if(current==npc_combat_.end()||current->second.hex_scope()!=scope)break;
            auto& recipient=current->second;const auto life=recipient.lifetime();
            const auto& hit=result.hits[index];
            const auto* receipt=index<result.reversals.size()&&result.reversals[index]?&*result.reversals[index]:nullptr;
            if(receipt){
                npc_simple_enchantment_credit_.erase({scope.actor,receipt->removed.instance});
                const auto packets=combat::npcs::reversal_damage_packets_2022(scope.actor,source_id,*receipt,reversal_healing_source_locked(scope,receipt->removed));
                for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
                if(receipt->damage){
                    consume_watch_yourself_npc_hit_locked(recipient,*receipt->damage,hit.kind!=combat::Kind::armor_ignoring,now);
                    if(receipt->damage->applied>0&&!receipt->damage->killed)provoke_npc_locked(scope,source_scope,now);
                    if(receipt->damage->applied>0)bonds_npc_caster_damage_locked(scope,receipt->damage->applied,now,nullptr);
                }else for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(scope.actor,recipient.actor(life)->effects)});
            }else{
                consume_watch_yourself_npc_hit_locked(recipient,hit,hit.kind!=combat::Kind::armor_ignoring,now);
                if(hit.applied>0&&!hit.killed)provoke_npc_locked(scope,source_scope,now);
                for(auto&[id,peer]:players_)peer.outbox->enqueue(combat::damage_packets_2022(source_id,hit));
            }
        }
    }
    bool preflight_npc_reversal_or_damage_locked(combat::npcs::LiveActor& proposal,
            const combat::Damage& damage,std::uint64_t now,const skills::effects::EndingHexContext* ending=nullptr,std::optional<combat::Hit>* actual=nullptr,
            std::optional<combat::npcs::LiveActor::ReversalDamage>* conversion=nullptr,
            const skills::effects::EndingStonefleshContext* ending_stoneflesh=nullptr,bool opposed_owned=false){
        if(conversion)*conversion={};if(actual)*actual={};const auto scope=proposal.hex_scope();const auto life=proposal.lifetime();
        const auto registry=proposal.owned_effect_snapshot(life);
        if(registry&&registry->newest_skill(scope,307,now)){
            auto raw=proposal;const auto hit=opposed_owned?raw.apply_opposed_owned_damage(life,damage,now):raw.apply_damage(life,damage,now,ending,ending_stoneflesh);
            if(hit.reason!=combat::Reason::accepted)return false;
            if(hit.damage<=0){proposal=std::move(raw);if(actual)*actual=hit;return true;}
            const auto receipt=proposal.apply_reversal_damage(life,damage,now,true,ending,ending_stoneflesh);
            if(!receipt||!validate_npc_reversal_receipts_locked(scope,{receipt}))return false;
            if(actual)*actual=receipt->damage;if(conversion)*conversion=receipt;return true;
        }
        const auto hit=opposed_owned?proposal.apply_opposed_owned_damage(life,damage,now):proposal.apply_damage(life,damage,now,ending,ending_stoneflesh);if(hit.reason!=combat::Reason::accepted)return false;
        if(actual)*actual=hit;return true;
    }
    bool commit_npc_reversal_or_damage_locked(Player& source,const Npc& npc,combat::npcs::LiveActor& live,
            const combat::Damage& damage,std::uint64_t now,std::vector<Packet>* deferred=nullptr,const skills::effects::EndingHexContext* ending=nullptr,
            std::optional<combat::Hit>* actual=nullptr,bool defer_death=false,std::optional<skills::effects::Scope> actor_cause={},const skills::effects::EndingStonefleshContext* ending_stoneflesh=nullptr,bool opposed_owned=false){
        return commit_npc_reversal_or_damage_locked(&source,npc,live,damage,now,deferred,ending,actual,defer_death,actor_cause,ending_stoneflesh,opposed_owned);
    }
    bool commit_npc_reversal_or_damage_locked(Player* source,const Npc& npc,combat::npcs::LiveActor& live,
            const combat::Damage& damage,std::uint64_t now,std::vector<Packet>* deferred=nullptr,const skills::effects::EndingHexContext* ending=nullptr,
            std::optional<combat::Hit>* actual=nullptr,bool defer_death=false,std::optional<skills::effects::Scope> actor_cause={},const skills::effects::EndingStonefleshContext* ending_stoneflesh=nullptr,bool opposed_owned=false){
        if(actual)*actual={};
        const auto registry=live.owned_effect_snapshot(live.lifetime());
        if(registry&&registry->newest_skill(live.hex_scope(),307,now)){
            auto raw=live;const auto hit=opposed_owned?raw.apply_opposed_owned_damage(live.lifetime(),damage,now):raw.apply_damage(live.lifetime(),damage,now,ending,ending_stoneflesh);
            if(hit.reason!=combat::Reason::accepted)return false;
            if(hit.damage>0){
                std::optional<combat::npcs::LiveActor::ReversalDamage> receipt;
                const bool accepted=commit_npc_reversal_damage_locked(source,npc,live,damage,now,&receipt,deferred,ending,defer_death,actor_cause,ending_stoneflesh,opposed_owned);
                if(actual&&receipt)*actual=receipt->damage;
                return accepted;
            }
        }
        const auto hit=damage_npc_locked(source,npc,live,damage,now,deferred,ending,opposed_owned,defer_death,actor_cause,ending_stoneflesh);
        if(actual&&hit.reason==combat::Reason::accepted)*actual=hit;
        return hit.reason==combat::Reason::accepted;
    }
    bool commit_npc_reversal_damage_locked(Player& source,const Npc& npc,combat::npcs::LiveActor& live,
            const combat::Damage& damage,std::uint64_t now,
            std::optional<combat::npcs::LiveActor::ReversalDamage>* completion=nullptr,std::vector<Packet>* deferred=nullptr,const skills::effects::EndingHexContext* ending=nullptr,bool defer_death=false,std::optional<skills::effects::Scope> actor_cause={},const skills::effects::EndingStonefleshContext* ending_stoneflesh=nullptr,bool opposed_owned=false){
        return commit_npc_reversal_damage_locked(&source,npc,live,damage,now,completion,deferred,ending,defer_death,actor_cause,ending_stoneflesh,opposed_owned);
    }
    bool commit_npc_reversal_damage_locked(Player* source,const Npc& npc,combat::npcs::LiveActor& live,
            const combat::Damage& damage,std::uint64_t now,
            std::optional<combat::npcs::LiveActor::ReversalDamage>* completion=nullptr,std::vector<Packet>* deferred=nullptr,const skills::effects::EndingHexContext* ending=nullptr,bool defer_death=false,std::optional<skills::effects::Scope> actor_cause={},const skills::effects::EndingStonefleshContext* ending_stoneflesh=nullptr,bool opposed_owned=false){
        if(completion)*completion={};const auto target=live.hex_scope();const auto life=live.lifetime();
        if(target.actor!=npc.id||(!live.hostile_to_players(life)&&!(opposed_owned&&live.healable_ally(life))))return false;
        const auto registry=live.owned_effect_snapshot(life);const auto active=registry?registry->newest_skill(target,307,now):std::nullopt;
        if(!active)return false;
        if(!valid_reversal_owner_locked(target,*active))return false;
        auto proposed=live;const auto receipt=proposed.apply_reversal_damage(life,damage,now,true,ending,ending_stoneflesh);if(!receipt)return false;
        const auto cause=source?std::optional<unsigned>(source->id):npc_damage_attribution_locked(actor_cause);
        const auto packets=combat::npcs::reversal_damage_packets_2022(target.actor,cause,*receipt,reversal_healing_source_locked(target,receipt->removed));
        live=std::move(proposed);npc_simple_enchantment_credit_.erase({target.actor,receipt->removed.instance});
        if(deferred)deferred->insert(deferred->end(),packets.begin(),packets.end());
        else for(auto&[id,p]:players_)p.outbox->enqueue(packets);
        if(receipt->damage){
            consume_watch_yourself_npc_hit_locked(live,*receipt->damage,damage.kind!=combat::Kind::armor_ignoring&&!damage.ignores_armor,now,deferred);
            if(!receipt->damage->killed){const auto status=emotes::effects_2022(target.actor,live.actor(life)->effects);
                if(deferred)deferred->push_back(status);else for(auto&[id,p]:players_)p.outbox->enqueue({status});}
            if(receipt->damage->applied>0&&source&&!receipt->damage->killed)provoke_npc_locked(target,source->effect_scope,now);
            if(receipt->damage->applied>0)bonds_npc_caster_damage_locked(target,receipt->damage->applied,now,deferred);
        }else{const auto status=emotes::effects_2022(target.actor,live.actor(life)->effects);
            if(deferred)deferred->push_back(status);else for(auto&[id,p]:players_)p.outbox->enqueue({status});}
        // A deferred batch reacquires its scoped recipient for final cleanup.
        // Bonds callbacks may already have retired this actor reference.
        if(!defer_death){const auto current=npc_combat_.find(target.actor);const auto placed=npcs_.find(target.actor);
            if(current!=npc_combat_.end()&&placed!=npcs_.end()&&current->second.hex_scope()==target){
                const auto actor=current->second.actor(life);
                if(actor&&actor->health&&actor->health->current<=0)record_npc_death_locked(source,placed->second,current->second,now);
            }
        }
        if(completion)*completion=receipt;return true;
    }

public:
    bool stage_npc_reversal(skills::effects::Scope target,unsigned rank,std::uint64_t now){
        return stage_npc_reversal_from(target,target,rank,now);
    }
    // Trusted integration fixture; paid ally-target admission remains separate.
    bool stage_npc_reversal_from(skills::effects::Scope caster,skills::effects::Scope target,unsigned rank,std::uint64_t now){
        std::lock_guard lock(mutex_);auto found=npc_combat_.find(target.actor);
        if(found==npc_combat_.end()||found->second.hex_scope()!=target||next_npc_hex_==UINT32_MAX)return false;
        const auto valid_caster=[&]{
            if(const auto player=players_.find(caster.actor);player!=players_.end())
                return player->second.effect_scope==caster&&player->second.resources&&player->second.precise_health>0&&
                    now>=player->second.health_updated&&now>=player->second.energy_updated;
            const auto npc=npc_combat_.find(caster.actor);
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=caster)return false;
            const auto actor=npc->second.actor(npc->second.lifetime());
            return actor&&actor->health&&actor->health->current>0&&now>=npc->second.health_clock(npc->second.lifetime()).value_or(now);
        };
        if(!valid_caster())return false;
        prune_reversal_caster_credit_locked(now);
        if(caster!=target&&reversal_caster_credit_.size()>=4096)return false;
        if(now<found->second.health_clock(found->second.lifetime()).value_or(now))return false;
        advance_npc_conditions_locked(target.actor,found->second,now);
        found=npc_combat_.find(target.actor);
        if(found==npc_combat_.end()||found->second.hex_scope()!=target||!valid_caster())return false;
        auto& live=found->second;const auto life=live.lifetime();
        if(now<live.health_clock(life).value_or(now))return false;
        const auto prior=live.owned_effect_snapshot(life);if(!prior)return false;
        const auto old=prior->newest_skill(target,307,now);const auto effect=next_npc_hex_;
        if(!live.apply_reversal_fortune(life,rank,caster.actor,effect,now))return false;
        ++next_npc_hex_;std::vector<Packet> packets;
        if(old){npc_simple_enchantment_credit_.erase({target.actor,old->instance});reversal_caster_credit_.erase({target,old->instance});packets.push_back(health_effect_remove(target.actor,old->instance));}
        npc_simple_enchantment_credit_[{target.actor,effect}]=target;
        if(caster!=target)reversal_caster_credit_[{target,effect}]=caster;
        const auto entry=live.hex_effect(life,effect);if(!entry)throw std::logic_error("staged NPC Reversal missing owned entry");
        packets.push_back(health_effect_add(*entry,target.actor,now));
        if(const auto body=skills::particles::target_body_2022(target.actor,307))packets.push_back(*body);
        packets.push_back(emotes::effects_2022(target.actor,live.actor(life)->effects));
        for(auto&[id,p]:players_)p.outbox->enqueue(packets);
        commit_applications_locked(target,now);return true;
    }
    // Trusted testing/integration primitive; no paid1399 producer is enabled.
    bool stage_npc_shield_absorption(skills::effects::Scope target,unsigned rank,std::uint64_t now){
        std::lock_guard lock(mutex_);auto found=npc_combat_.find(target.actor);
        if(found==npc_combat_.end()||found->second.hex_scope()!=target||next_npc_hex_==UINT32_MAX)return false;
        auto& live=found->second;const auto life=live.lifetime();
        if(now<live.health_clock(life).value_or(now))return false;
        advance_npc_conditions_locked(target.actor,live,now);
        const auto prior=live.owned_effect_snapshot(life);if(!prior)return false;
        const auto old=prior->newest_skill(target,1399,now);const auto effect=next_npc_hex_;
        if(!live.stage_shield_absorption(life,rank,target.actor,effect,now))return false;
        ++next_npc_hex_;std::vector<Packet> packets;
        if(old){npc_simple_enchantment_credit_.erase({target.actor,old->instance});packets.push_back(health_effect_remove(target.actor,old->instance));}
        npc_simple_enchantment_credit_[{target.actor,effect}]=target;
        const auto entry=live.hex_effect(life,effect);if(!entry)throw std::logic_error("staged NPC Absorption missing owned entry");
        packets.push_back(health_effect_add(*entry,target.actor,now));
        if(const auto body=skills::particles::target_body_2022(target.actor,1399))packets.push_back(*body);
        packets.push_back(emotes::effects_2022(target.actor,live.actor(life)->effects));
        for(auto&[id,p]:players_)p.outbox->enqueue(packets);
        commit_applications_locked(target,now);return true;
    }
    // A separate completion receipt preserves healing death without inventing
    // a damaging Hit. Remaining NPC producers must migrate explicitly.
    bool npc_reversal_damage(skills::effects::Scope source,skills::effects::Scope target,
            const combat::Damage& damage,std::uint64_t now,
            std::optional<combat::npcs::LiveActor::ReversalDamage>* completion=nullptr){
        if(completion)*completion={};std::lock_guard lock(mutex_);
        auto caster=players_.find(source.actor);auto found=npc_combat_.find(target.actor);const auto placed=npcs_.find(target.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||found==npc_combat_.end()||
           placed==npcs_.end()||found->second.hex_scope()!=target||!caster->second.resources||
           placed->second.plane!=caster->second.surface.plane||!advance_resource_event_locked(caster->second,now)||
           caster->second.precise_health<=0)return false;
        auto& live=found->second;const auto life=live.lifetime();
        if(!live.hostile_to_players(life)||now<live.health_clock(life).value_or(now))return false;
        const auto origin=healing_position_locked(caster->second,now);refresh_npc_movement_locked(target.actor,now);
        if(!origin||std::hypot(placed->second.x-origin->x,placed->second.y-origin->z)>1248)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,*origin,{placed->second.x,placed->second.y});if(!line||line->plane!=placed->second.plane)return false;}
        advance_npc_conditions_locked(target.actor,live,now);
        return commit_npc_reversal_damage_locked(caster->second,placed->second,live,damage,now,completion);
    }
    // Trusted recipient-only operation,not a complete life-steal skill cast.
private:
    // Typed recipient conversion used by compound effects already holding the
    // World lock. It never applies caster payout or re-selects geometry.
    bool commit_npc_reversal_life_steal_locked(Player& caster,const Npc& placed,combat::npcs::LiveActor& live,
            double reduced_amount,std::uint64_t now,
            std::optional<skills::effects::ReversalLifeStealReceipt>* completion=nullptr){
        if(completion)*completion={};
        const auto source=caster.effect_scope,target=live.hex_scope();const auto life=live.lifetime();
        if(!std::isfinite(reduced_amount)||reduced_amount<=0||reduced_amount>16000000||
           target.actor!=placed.id||!live.hostile_to_players(life)||
           now<live.health_clock(life).value_or(now))return false;
        const auto registry=live.owned_effect_snapshot(life);const auto active=registry?registry->newest_skill(target,307,now):std::nullopt;
        if(!active)return false;
        if(!valid_reversal_owner_locked(target,*active))return false;
        auto proposal=live;const auto receipt=proposal.apply_reversal_life_steal(life,reduced_amount,now);if(!receipt)return false;
        auto stages=combat::npcs::reversal_life_steal_recipient_packets_2022(target.actor,source.actor,*receipt,reversal_healing_source_locked(target,receipt->removed));
        live=std::move(proposal);npc_simple_enchantment_credit_.erase({target.actor,receipt->removed.instance});
        const auto status=emotes::effects_2022(target.actor,live.actor(life)->effects);
        const bool remaining_death=receipt->after_healing.current>0&&receipt->after_loss.current<=0;
        if(!remaining_death)stages.before_transfer.push_back(status);
        stages.before_transfer.insert(stages.before_transfer.end(),stages.remaining_loss.begin(),stages.remaining_loss.end());
        if(remaining_death)stages.before_transfer.push_back(status);
        for(auto& [id,p]:players_)p.outbox->enqueue(stages.before_transfer);
        if(live.actor(life)->health->current<=0)record_npc_death_locked(caster,placed,live,now);
        if(completion)*completion=receipt;return true;
    }
public:
    // The caller owns reduced-amount/overkill policy and any caster payout.
    bool npc_reversal_life_steal_recipient(skills::effects::Scope source,skills::effects::Scope target,
            double reduced_amount,std::uint64_t now,std::optional<skills::effects::ReversalLifeStealReceipt>* completion=nullptr){
        if(completion)*completion={};
        if(!std::isfinite(reduced_amount)||reduced_amount<=0||reduced_amount>16000000)return false;
        std::lock_guard lock(mutex_);
        auto caster=players_.find(source.actor);auto found=npc_combat_.find(target.actor);const auto placed=npcs_.find(target.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||!caster->second.resources||found==npc_combat_.end()||
           placed==npcs_.end()||found->second.hex_scope()!=target||placed->second.plane!=caster->second.surface.plane||
           !advance_resource_event_locked(caster->second,now)||caster->second.precise_health<=0)return false;
        auto& live=found->second;const auto life=live.lifetime();
        if(!live.hostile_to_players(life)||now<live.health_clock(life).value_or(now))return false;
        const auto origin=healing_position_locked(caster->second,now);refresh_npc_movement_locked(target.actor,now);
        if(!origin||std::hypot(placed->second.x-origin->x,placed->second.y-origin->z)>1248)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,*origin,{placed->second.x,placed->second.y});if(!line||line->plane!=placed->second.plane)return false;}
        advance_npc_conditions_locked(target.actor,live,now);
        return commit_npc_reversal_life_steal_locked(caster->second,placed->second,live,reduced_amount,now,completion);
    }

private:
