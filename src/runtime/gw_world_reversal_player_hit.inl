// Included inside Instance. This commit path is opt-in per reviewed damage
// producer. Skill307 cast admission stays unbound until all producers agree.
    enum class ReversalHitCommit {absent,committed,invalid};
    bool player_absorption_hit_locked(Player& p,const combat::Hit& hit,std::uint64_t now,bool commit){
        const bool active=p.effect_registry&&p.effect_registry->newest_skill(p.effect_scope,1399,now).has_value();
        if(!hit.cumulative_reduction)return !active;
        if(!active||hit.reason!=combat::Reason::accepted||hit.target!=p.id||hit.generation!=p.effect_scope.generation)return false;
        auto counter=p.shield_absorption_hits.value_or(skills::effects::ShieldAbsorption(p.effect_scope));
        if(!counter.commit_receipt(*p.effect_registry,p.effect_scope,*hit.cumulative_reduction,now))return false;
        if(commit)p.shield_absorption_hits=std::move(counter);return true;
    }

    // Weapon contacts retain their own attribution, critical popup and physical
    // reaction rules. A zero skill is admitted only through this typed path.
    struct PlayerWeaponHit {bool critical=false,elemental=false,armor_respecting=true,npc_visibility=false;};
    bool preflight_player_reversal_locked(const Player& p,const combat::Hit& incoming,std::uint64_t now,bool weapon_damage=false)const{
        const auto active=p.effect_registry?p.effect_registry->newest_skill(p.effect_scope,307,now):std::nullopt;
        if(incoming.damage<=0||!active)return true;
        if(!valid_reversal_owner_locked(p.effect_scope,*active))return false;
        if(!p.resources||p.precise_health<=0||now<p.health_updated||now<p.energy_updated||
           incoming.reason!=combat::Reason::accepted||incoming.target!=p.id||incoming.generation!=p.effect_scope.generation||
           !std::isfinite(incoming.damage)||incoming.damage>16000000||(!incoming.skill&&!weapon_damage)||incoming.skill>3432||
           static_cast<unsigned>(incoming.kind)>static_cast<unsigned>(combat::Kind::holy))return false;
        const auto quote=skills::effects::prepare_reversal_fortune(*p.effect_registry,p.effect_scope,
            skills::effects::ReversalInput::damage,incoming.damage,now);
        const auto multiplier=skills::effects::healing_hex_multiplier(p.effect_registry->active(p.effect_scope,now),now);
        return quote&&std::isfinite(multiplier)&&multiplier>0&&multiplier<=1;
    }
    combat::DamageProjection player_damage_projection_locked(const Player& p,std::uint64_t now)const{
        if(!p.effect_registry||!p.effect_registry->newest_skill(p.effect_scope,307,now))return {};
        return [registry=*p.effect_registry,wound=p.deep_wound,scope=p.effect_scope,now,
                clock_valid=now>=p.health_updated&&now>=p.energy_updated&&valid_reversal_owner_locked(p.effect_scope,*p.effect_registry->newest_skill(p.effect_scope,307,now))](combat::Actor& actor,const combat::Damage& damage)mutable->std::optional<combat::DamageStep>{
            if(!clock_valid||actor.id!=scope.actor||actor.generation!=scope.generation)return {};
            const auto before=actor;const auto incoming=combat::apply(actor,damage,true);
            if(incoming.reason!=combat::Reason::accepted)return {};
            if(incoming.damage<=0||!registry.newest_skill(scope,307,now))return combat::DamageStep{incoming,incoming};
            const auto quote=skills::effects::prepare_reversal_fortune(registry,scope,skills::effects::ReversalInput::damage,incoming.damage,now);
            if(!quote||!skills::effects::consume_reversal_fortune(registry,*quote,now))return {};
            auto hp=before.health->current;
            const auto multiplier=skills::effects::healing_hex_multiplier(registry.active(scope,now),now);
            if(!std::isfinite(multiplier)||multiplier<=0||multiplier>1)return {};
            wound.event(hp,quote->split.healing,true,multiplier);actor=before;
            actor.effects=registry.presentation(scope,actor.effects&~0x10u,now);
            if(hp<=0){actor.health->current=0;actor.effects|=0x10;return combat::DamageStep{incoming,{}};}
            auto hit=incoming;hit.damage=quote->split.remaining;hit.before={hp,before.health->maximum};hit.after=hit.before;
            hit.applied=std::min(hit.damage,hp);wound.event(hp,-hit.damage);hit.after.current=hp;hit.killed=hp<=0&&hit.applied>0;
            hit.effects=actor.effects|(hit.killed?0x10u:0u);actor.health=hit.after;actor.effects=hit.effects;
            return combat::DamageStep{incoming,hit};
        };
    }
    void commit_player_remaining_hit_locked(Player& p,const combat::Hit& incoming,double amount,
            std::optional<std::uint32_t> cause,std::uint64_t now,bool armor_respecting=true,std::optional<combat::Hit>* actual=nullptr,std::vector<Packet>* deferred=nullptr,bool defer_damage_death=false,const PlayerWeaponHit* weapon=nullptr,bool npc_visibility=false){
        if(actual)*actual={};if(p.precise_health<=0)return;
        auto hit=incoming;hit.damage=amount;
        const auto refresh=[&]{hit.health_settlement.reset();hit.before={p.precise_health,p.resources->max_health};hit.after=hit.before;
            hit.applied=std::min(hit.damage,hit.before.current);
            hit.after.current=std::max(0.,hit.before.current-hit.damage);
            hit.killed=hit.applied>0&&hit.after.current==0;hit.effects=p.effects|(hit.killed?0x10u:0u);
            // Rebuild sealed evidence from current capacity after callbacks.
            // This projection never consumes the real pending Health event.
            auto capacity=p.deep_wound;
            if(!combat::settle_damage_health(hit,capacity))throw std::logic_error("invalid player damage Health settlement");};
        refresh();
        if(!weapon||weapon->elemental)record_hit_locked(p,hit,now,armor_respecting);
        else {consume_watch_yourself_hit_locked(p,hit.applied,armor_respecting,now);record_adrenaline_damage_locked(p,hit.damage,now);}
        if(p.precise_health<=0)return;refresh();
        auto packets=weapon&&cause?(hit.skill?combat::damage_packets_2022(*cause,hit,weapon->critical):combat::weapon::packets_2022(*cause,hit,weapon->critical)):
            cause?combat::damage_packets_2022(*cause,hit):combat::health_packets_2022(
            {p.id,p.effect_scope.generation,hit.after,{},hit.effects,true});
        if(deferred)deferred->insert(deferred->end(),packets.begin(),packets.end());
        else for(auto&[id,peer]:players_)if((!npc_visibility&&(!weapon||!weapon->npc_visibility))||peer.npc_publication_ready)peer.outbox->enqueue(packets);
        health_event_locked(p,-hit.damage,now);
        if(actual)*actual=hit;
        if(hit.applied>0&&p.passive_recovery)p.passive_recovery->record(recovery::Event::health_loss,now);
        if(p.precise_health<=0&&!defer_damage_death)kill_locked(p,now);
    }
    ReversalHitCommit commit_reversal_player_hit_locked(Player& p,const combat::Hit& incoming,
            std::optional<std::uint32_t> cause,std::uint64_t now,bool armor_respecting=true,std::optional<combat::Hit>* actual=nullptr,std::vector<Packet>* deferred=nullptr,bool defer_damage_death=false,const PlayerWeaponHit* weapon=nullptr,bool npc_visibility=false){
        if(actual)*actual={};
        if(!p.effect_registry||incoming.damage<=0||!p.effect_registry->newest_skill(p.effect_scope,307,now))
            return ReversalHitCommit::absent;
        if((cause&&(!*cause||*cause>65535||*cause==p.id))||!preflight_player_reversal_locked(p,incoming,now,weapon!=nullptr))return ReversalHitCommit::invalid;
        const auto quote=*skills::effects::prepare_reversal_fortune(*p.effect_registry,p.effect_scope,
            skills::effects::ReversalInput::damage,incoming.damage,now);
        const auto removed=skills::effects::consume_reversal_fortune(*p.effect_registry,quote,now);
        if(!removed)return ReversalHitCommit::invalid;
        const auto healing_caster=reversal_caster_scope_locked(p.effect_scope,*removed);
        // Reuse the owned timer/status cleanup;the quoted307 application is
        // already retired,so nested reactions cannot consume it a second time.
        remove_health_effect_locked(p,removed->instance,now);
        const auto before=p.precise_health;
        health_event_locked(p,quote.split.healing,now,true);
        const auto gained=p.precise_health-before;
        const auto healer=healing_caster?reversal_healing_source_locked(*healing_caster).actor:std::nullopt;
        if(gained>0&&healer)for(auto&[id,peer]:players_)if(!npc_visibility||peer.npc_publication_ready)peer.outbox->enqueue({skills::particles::health_change_2022(
            p.id,*healer,gained,p.resources->max_health)});
        publish_health_locked(p);
        if(p.precise_health<=0){if(!defer_damage_death)kill_locked(p,now);return ReversalHitCommit::committed;}
        commit_player_remaining_hit_locked(p,incoming,quote.split.remaining,cause,now,armor_respecting,actual,deferred,defer_damage_death,weapon,npc_visibility);
        return ReversalHitCommit::committed;
    }
    bool commit_player_resolved_hit_locked(Player& p,const combat::Hit& hit,std::optional<std::uint32_t> cause,std::uint64_t now,
            std::optional<combat::Hit>* actual=nullptr,std::vector<Packet>* deferred=nullptr,bool defer_damage_death=false,bool npc_visibility=false,bool armor_respecting=true){
        const auto absorption_before=p.shield_absorption_hits;
        if(!player_absorption_hit_locked(p,hit,now,true))return false;
        const auto reversal=commit_reversal_player_hit_locked(p,hit,cause,now,armor_respecting,actual,deferred,defer_damage_death,nullptr,npc_visibility);
        if(reversal==ReversalHitCommit::invalid){p.shield_absorption_hits=absorption_before;return false;}
        if(reversal==ReversalHitCommit::absent)commit_player_remaining_hit_locked(p,hit,hit.damage,cause,now,armor_respecting,actual,deferred,defer_damage_death,nullptr,npc_visibility);
        return true;
    }

    std::optional<skills::effects::HealingSeedReaction> prepare_player_reactive_seed_locked(
            Player& p,const combat::Hit& incoming,std::uint64_t now,bool weapon_damage=false){
        if(!p.resources||p.precise_health<=0||now<p.health_updated||
           incoming.reason!=combat::Reason::accepted||incoming.target!=p.id||incoming.generation!=p.effect_scope.generation||
           !std::isfinite(incoming.damage)||incoming.damage<0||!preflight_player_reversal_locked(p,incoming,now,weapon_damage))return {};
        if(!player_absorption_hit_locked(p,incoming,now,false))return {};
        // The Hit is already post-protection. Quote that amount directly;
        // feeding it through Damage again would apply a second admission cap
        // and could change a valid amplified/fractional incoming event.
        auto loss=std::min(incoming.damage,p.precise_health);
        if(incoming.damage>0&&p.effect_registry&&p.effect_registry->newest_skill(p.effect_scope,307,now)){
            auto registry=*p.effect_registry;auto wound=p.deep_wound;auto hp=p.precise_health;
            const auto quote=skills::effects::prepare_reversal_fortune(registry,p.effect_scope,
                skills::effects::ReversalInput::damage,incoming.damage,now);
            if(!quote||!skills::effects::consume_reversal_fortune(registry,*quote,now))return {};
            const auto multiplier=skills::effects::healing_hex_multiplier(registry.active(p.effect_scope,now),now);
            wound.event(hp,quote->split.healing,true,multiplier);
            loss=hp>0?std::min(quote->split.remaining,hp):0.;
        }
        // Preflight every potential recipient. Current survival is checked
        // after the actual hit, including conversion healing and end reactions.
        return prepare_healing_seed_direct_locked(p,loss,now);
    }
    bool commit_player_weapon_hit_locked(Player& p,const combat::Hit& incoming,std::uint32_t cause,
            const PlayerWeaponHit& weapon,std::uint64_t now,std::optional<combat::Hit>& actual,
            const skills::effects::HealingSeedReaction& seed,const combat::weapon::QualifiedHit* qualified=nullptr){
        RetributionPublication publication(*this,qualified&&retribution_contacts_.contains(qualified->event));
        bool accepted=false;
        try{accepted=[&]()->bool{
        actual={};
        const auto absorption_before=p.shield_absorption_hits;
        if(!player_absorption_hit_locked(p,incoming,now,true))return false;
        const auto reversal=commit_reversal_player_hit_locked(p,incoming,cause,now,weapon.armor_respecting,&actual,nullptr,true,&weapon);
        if(reversal==ReversalHitCommit::invalid){p.shield_absorption_hits=absorption_before;return false;}
        if(reversal==ReversalHitCommit::absent)commit_player_remaining_hit_locked(p,incoming,incoming.damage,cause,now,
            weapon.armor_respecting,&actual,nullptr,true,&weapon);
        if(actual&&actual->applied>0&&seed.effect&&p.effect_registry){
            const auto lease=p.effect_registry->newest_skill(p.effect_scope,274,now);
            if(lease&&lease->instance==seed.effect&&lease->cause==seed.cause)publish_healing_seed_direct_locked(seed,now);
        }
        // Contact caller owns one death cleanup after Conjure/other reactions.
        return true;
        }();
        if(accepted&&qualified){publication.reaction();commit_retribution_damage_locked(*qualified,actual);}
        }catch(...){publication.flush();throw;}
        publication.flush();return accepted;
    }
    bool commit_player_reactive_hit_locked(Player& p,const combat::Hit& incoming,
            std::optional<std::uint32_t> cause,std::uint64_t now,bool defer_death=false,
            const skills::effects::HealingSeedReaction* prepared=nullptr,bool npc_visibility=false,bool armor_respecting=true,
            std::optional<combat::Hit>* completion=nullptr,std::optional<skills::effects::Scope> actor_cause={}){
        if(completion)*completion={};
        const auto seed=prepared?std::optional(*prepared):prepare_player_reactive_seed_locked(p,incoming,now);
        if(!seed)return false;
        if(actor_cause){
            const auto source=players_.find(actor_cause->actor);
            cause=source!=players_.end()&&source->second.effect_scope==*actor_cause?
                std::optional<unsigned>(actor_cause->actor):npc_damage_attribution_locked(actor_cause);
        }
        std::optional<combat::Hit> actual;
        if(!commit_player_resolved_hit_locked(p,incoming,cause,now,&actual,nullptr,true,npc_visibility,armor_respecting))return false;
        if(completion)*completion=actual;
        if(actual&&actual->applied>0&&seed->effect&&p.effect_registry){
            const auto lease=p.effect_registry->newest_skill(p.effect_scope,274,now);
            if(lease&&lease->instance==seed->effect&&lease->cause==seed->cause){
                publish_healing_seed_direct_locked(*seed,now);
            }
        }
        if(!defer_death&&p.precise_health<=0)kill_locked(p,now);
        return true;
    }

public:
    struct PlayerReversalLifeStealReceipt {
        skills::effects::Entry removed;skills::effects::ReversalFortuneQuote quote;
        combat::Health before_healing,after_healing,before_loss,after_loss;
        double lost=0;
    };
private:
    // Caller has selected the current scoped recipient under the World lock.
    // Geometry/payment stay with the effect; this owns only conversion state.
    bool commit_player_reversal_life_steal_locked(Player& recipient,skills::effects::Scope source,
            skills::effects::Scope target,double reduced_amount,std::uint64_t now,
            std::optional<PlayerReversalLifeStealReceipt>* completion=nullptr){
        if(completion)*completion={};
        if(!std::isfinite(reduced_amount)||reduced_amount<=0||reduced_amount>16000000||
           recipient.effect_scope!=target||!recipient.resources||recipient.precise_health<=0||
           now<recipient.health_updated||now<recipient.energy_updated)return false;
        if(!recipient.effect_registry||!recipient.effect_registry->matches(target))return false;
        const auto active=recipient.effect_registry->newest_skill(target,307,now);
        if(!active||!valid_reversal_owner_locked(target,*active))return false;
        const auto healing_caster=reversal_caster_scope_locked(target,*active);
        const auto quote=skills::effects::prepare_reversal_fortune(*recipient.effect_registry,target,
            skills::effects::ReversalInput::life_steal,reduced_amount,now);if(!quote)return false;
        const auto multiplier=skills::effects::healing_hex_multiplier(recipient.effect_registry->active(target,now),now);
        if(!std::isfinite(multiplier)||multiplier<=0||multiplier>1)return false;
        const auto removed=skills::effects::consume_reversal_fortune(*recipient.effect_registry,*quote,now);if(!removed)return false;
        PlayerReversalLifeStealReceipt receipt{*removed,*quote,{recipient.precise_health,recipient.resources->max_health},{},{},{},0};
        // Retire registry authority now, but emit the owned end only after its
        // real healing feedback. Nested settlement cannot consume307 again.
        health_event_locked(recipient,quote->split.healing,now,true);
        receipt.after_healing={recipient.precise_health,recipient.resources->max_health};
        const auto gained=receipt.after_healing.current-receipt.before_healing.current;
        const auto healer=healing_caster?reversal_healing_source_locked(*healing_caster).actor:std::nullopt;
        if(gained>0&&healer)for(auto& [id,peer]:players_)peer.outbox->enqueue({skills::particles::health_change_2022(recipient.id,*healer,gained,receipt.after_healing.maximum)});
        publish_health_locked(recipient);
        remove_health_effect_locked(recipient,removed->instance,now);
        receipt.before_loss={recipient.precise_health,recipient.resources->max_health};
        if(recipient.precise_health>0)health_event_locked(recipient,-quote->split.remaining,now);
        receipt.after_loss={recipient.precise_health,recipient.resources->max_health};
        receipt.lost=std::max(0.,receipt.before_loss.current-receipt.after_loss.current);
        if(receipt.lost>0){
            for(auto& [id,peer]:players_)peer.outbox->enqueue({skills::particles::health_change_2022(recipient.id,source.actor,-receipt.lost,receipt.after_loss.maximum)});
            if(recipient.passive_recovery)recipient.passive_recovery->record(recovery::Event::health_loss,now);
        }
        publish_health_locked(recipient);
        if(recipient.precise_health<=0)kill_locked(recipient,now);
        if(completion)*completion=receipt;return true;
    }
public:
    // Trusted recipient-only settlement. No skill payment or caster payout is
    // inferred. The before-loss snapshot permits intervening end reactions.
    bool player_reversal_life_steal_recipient(skills::effects::Scope source,skills::effects::Scope target,
            double reduced_amount,std::uint64_t now,std::optional<PlayerReversalLifeStealReceipt>* completion=nullptr){
        if(completion)*completion={};
        if(!std::isfinite(reduced_amount)||reduced_amount<=0||reduced_amount>16000000)return false;
        std::lock_guard lock(mutex_);
        const auto a=players_.find(source.actor),b=players_.find(target.actor);
        if(a==players_.end()||b==players_.end()||a->second.effect_scope!=source||b->second.effect_scope!=target||
           !a->second.resources||!b->second.resources||!a->second.team_token||!b->second.team_token||
           a->second.team_token==b->second.team_token||a->second.surface.plane!=b->second.surface.plane||
           !advance_resource_event_locked(a->second,now)||!advance_resource_event_locked(b->second,now)||
           a->second.precise_health<=0||b->second.precise_health<=0)return false;
        auto& recipient=b->second;
        const auto origin=healing_position_locked(a->second,now),point=healing_position_locked(recipient,now);
        if(!origin||!point||std::hypot(point->x-origin->x,point->z-origin->z)>1248)return false;
        if(navigation_){const auto line=layered_->trace(a->second.surface,*origin,*point);if(!line||line->plane!=recipient.surface.plane)return false;}
        return commit_player_reversal_life_steal_locked(recipient,source,target,reduced_amount,now,completion);
    }
private:
