// Shared owned self-enchantment application. Included inside Instance.
private:
    bool complete_owned_self_enchantment(skills::effects::Scope owner,unsigned skill,unsigned rank,
            std::uint64_t base_duration,std::uint64_t now,std::optional<std::uint64_t> completion_event,
            const skills::effects::EnchantmentDuration* adjusted,std::uint64_t* completed_at=nullptr,std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        if(completed_at)*completed_at=0;
        const auto quoted=adjusted?adjusted->for_enchantment(skill,base_duration,owner,now):std::optional(base_duration);
        if(!quoted)return false;const auto duration=*quoted;
        if((skill!=2138&&skill!=82&&skill!=182&&skill!=207&&skill!=221&&skill!=38&&skill!=180&&skill!=3375&&skill!=1261&&skill!=1375&&!skills::effects::attunement(skill,rank))||!duration||now>UINT64_MAX-duration)return false;
        std::lock_guard lock(mutex_);const auto found=players_.find(owner.actor);
        if(found==players_.end()||found->second.effect_scope!=owner)return false;
        auto& player=found->second;
        DamagePublication pending;if(publication)begin_damage_publication_locked(pending);
        bool accepted=false;
        struct PublicationExit {std::function<void()> finish;~PublicationExit(){finish();}} exit{[&]{
            auto release=finish_damage_publication_locked(pending);
            if(accepted&&publication)*publication=std::move(release);else if(release)release();
        }};
        // The quote belongs to the caller's observation above. An admitted
        // spell settles against current resource clocks if World ticked since.
        if(completion_event)now=std::max({now,player.health_updated,player.energy_updated});
        if(now>UINT64_MAX-duration)return false;
        if(!advance_resource_event_locked(player,now)||player.precise_health<=0||player.knocked_down_until>now||player.next_health_effect>UINT32_MAX)return false;
        unsigned admitted_skill=skill;
        if(skill==3375&&completion_event){const auto admitted=spell_activation_cursors_.find(owner.actor);
            if(admitted!=spell_activation_cursors_.end()&&admitted->second.actor==owner&&
               admitted->second.event==*completion_event&&admitted->second.skill==180)admitted_skill=180;}
        if(completion_event&&!complete_ether_renewal_locked(owner,admitted_skill,*completion_event,now,true,false))return false;
        const auto bodies=[&]{
            std::vector<Packet> result;
            const auto add=[&](unsigned resource){Packet p;u16(p,158);u32(p,21);u32(p,owner.actor);u32(p,resource);result.push_back(std::move(p));};
            if(skill==2138||skill==38||skill==180||skill==3375||skill==1261||skill==1375||skills::effects::attunement(skill,rank))return result; // Runtime owns the single pinned success body.
            if(skill==82)add(140);
            else{const auto* row=skills::find_skill(skill);add(row->caster_body_resource);add(row->target_body_resource);}
            return result;
        };
        if(well_blocks_player_enchantment_locked(player,now)){
            if(completion_event&&!complete_ether_renewal_locked(owner,admitted_skill,*completion_event,now,true))return false;
            const auto packets=bodies();for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);if(completed_at)*completed_at=now;accepted=true;return true;
        }
        unsigned effect=0;skills::effects::Entry entry{};
        std::optional<skills::effects::Registry> registry;
        std::optional<skills::regeneration::State> timers;
        skills::effects::Applied applied;
        const auto prepare=[&]{
            if(player.next_health_effect>UINT32_MAX)return false;
            effect=static_cast<unsigned>(player.next_health_effect);
            entry={effect,owner.actor,skill,rank,skills::effects::Category::enchantment,now,now+duration};
            registry=*player.effect_registry;timers=*player.health_regeneration;
            std::vector<skills::effects::Entry> variants;
            if(skill==180||skill==3375)for(const auto& old:registry->active(owner,now))
                if(old.skill==(skill==180?3375u:180u)){variants.push_back(old);registry->remove(owner,old.instance);timers->remove(owner,old.instance);}
            applied=registry->apply(owner,entry,skills::effects::Replace::same_skill);if(!applied.accepted)return false;
            applied.removed.insert(applied.removed.end(),variants.begin(),variants.end());
            for(const auto& old:applied.removed)timers->remove(owner,old.instance);
            const auto vigor=skill==2138?skills::regeneration::reviewed(skill,rank):std::nullopt;
            if(skill==2138&&!vigor)return false;
            return timers->apply(owner,{effect,owner.actor,skill,vigor?vigor->pips:0,now,now+duration});
        };
        if(!prepare())return false;
        if(completion_event){
            if(!complete_ether_renewal_locked(owner,admitted_skill,*completion_event,now,true))return false;
            if(player.precise_health<=0){if(completed_at)*completed_at=now;accepted=true;return true;}
            if(!prepare())throw std::logic_error("validated self enchantment changed during Renewal settlement");
        }
        *player.effect_registry=std::move(*registry);*player.health_regeneration=std::move(*timers);++player.next_health_effect;
        std::vector<Packet> packets;
        // Replacing Recall removes its old timer without an end reward.
        for(const auto& old:applied.removed){player.health_effects.erase(old.instance);packets.push_back(health_effect_remove(player.id,old.instance));}
        const auto visuals=bodies();packets.insert(packets.end(),visuals.begin(),visuals.end());
        player.health_effects.emplace(effect,entry);packets.push_back(health_effect_add(entry,player.id,now));
        for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
        commit_applications_locked(owner,now);publish_status_locked(player,now);if(completed_at)*completed_at=now;accepted=true;return true;
    }
public:
    bool hexers_vigor_enchantment(skills::effects::Scope owner,unsigned rank,std::uint64_t now,
            std::optional<std::uint64_t> completion_event={},const skills::effects::EnchantmentDuration* adjusted=nullptr,
            std::uint64_t* completed_at=nullptr,std::function<void()>* publication=nullptr){
        if(publication)*publication={};const auto plan=skills::regeneration::reviewed(2138,rank);
        return plan&&complete_owned_self_enchantment(owner,2138,rank,plan->duration,now,completion_event,adjusted,completed_at,publication);
    }
    bool frigid_armor_enchantment(skills::effects::Scope owner,unsigned rank,std::uint64_t now,
            std::optional<std::uint64_t> completion_event={},const skills::effects::EnchantmentDuration* adjusted=nullptr,
            std::uint64_t* completed_at=nullptr){
        const auto plan=skills::effects::frigid_armor(rank);
        return plan&&complete_owned_self_enchantment(owner,1261,rank,plan->duration,now,completion_event,adjusted,completed_at);
    }
    bool stoneflesh_aura_enchantment(skills::effects::Scope owner,unsigned rank,std::uint64_t now,
            std::optional<std::uint64_t> completion_event={},const skills::effects::EnchantmentDuration* adjusted=nullptr,
            std::uint64_t* completed_at=nullptr,std::function<void()>* publication=nullptr){
        if(publication)*publication={};const auto plan=skills::effects::stoneflesh_aura(rank);
        return plan&&complete_owned_self_enchantment(owner,1375,rank,plan->duration,now,completion_event,adjusted,completed_at,publication);
    }
    bool channeling_enchantment(skills::effects::Scope owner,unsigned rank,std::uint64_t now,
            std::optional<std::uint64_t> completion_event={},const skills::effects::EnchantmentDuration* adjusted=nullptr,
            std::uint64_t* completed_at=nullptr){
        const auto plan=skills::effects::channeling(rank);
        return plan&&complete_owned_self_enchantment(owner,38,rank,plan->duration,now,completion_event,adjusted,completed_at);
    }
    bool aura_restoration_enchantment(skills::effects::Scope owner,unsigned skill,unsigned rank,std::uint64_t now,
            std::optional<std::uint64_t> completion_event={},const skills::effects::EnchantmentDuration* adjusted=nullptr,
            std::uint64_t* completed_at=nullptr){
        const auto plan=skills::effects::aura_restoration_plan(rank);const auto* row=skills::find_skill(skill);
        return (skill==180||skill==3375)&&plan&&row&&row->type==6&&row->attribute==12&&row->duration0==60&&row->duration15==60&&
            complete_owned_self_enchantment(owner,skill,rank,plan->duration,now,completion_event,adjusted,completed_at);
    }
    bool attunement_enchantment(skills::effects::Scope owner,const skills::effects::AttunementPlan& plan,
            std::uint64_t now,std::optional<std::uint64_t> completion_event={},const skills::effects::EnchantmentDuration* adjusted=nullptr,std::uint64_t* completed_at=nullptr){
        const auto reviewed=skills::effects::attunement(plan.skill,plan.rank);
        return reviewed&&*reviewed==plan&&complete_owned_self_enchantment(owner,plan.skill,plan.rank,plan.duration,now,completion_event,adjusted,completed_at);
    }
    bool conjure_enchantment(skills::effects::Scope owner,const skills::effects::ConjurePlan& plan,
            std::uint64_t now,std::optional<std::uint64_t> completion_event={},const skills::effects::EnchantmentDuration* adjusted=nullptr){
        const auto reviewed=skills::effects::conjure(plan.skill,plan.rank);
        return reviewed&&*reviewed==plan&&complete_owned_self_enchantment(owner,plan.skill,plan.rank,plan.duration,now,completion_event,adjusted);
    }
    bool mantra_recall(skills::effects::Scope owner,unsigned rank,std::uint64_t now,
            std::optional<std::uint64_t> completion_event={},const skills::effects::EnchantmentDuration* adjusted=nullptr){
        const auto plan=skills::effects::mantra_recall(rank);
        return plan&&complete_owned_self_enchantment(owner,82,rank,plan->duration,now,completion_event,adjusted);
    }
