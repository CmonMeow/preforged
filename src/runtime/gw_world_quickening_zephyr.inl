// Included inside Instance. Only the trusted completed-cast producer creates
// these scoped aura records; a raw network request cannot register one.
private:
    void expire_quickening_zephyr_locked(std::uint64_t now){
        std::vector<QuickeningSpirit> expired;
        for(const auto& [scope,record]:quickening_spirits_)
            if(record.expires<=now)expired.push_back(record);
        for(const auto& record:expired){
            // A completed ritual will own its actor through quest-id zero.
            // Static test NPCs have no such owner and retain their population.
            retire_quest_npcs_locked(record.owner,0,record.spirit.actor);
            quickening_spirits_.erase(record.spirit);
        }
    }
public:
    bool record_quickening_zephyr_spirit(
            skills::effects::Scope owner,skills::effects::Scope spirit,
            const skills::effects::QuickeningZephyrPlan& plan,
            std::uint64_t now){
        if(skills::effects::quickening_zephyr(plan.rank)!=plan||
           now>UINT64_MAX-plan.lifetime)return false;
        const auto expires=now+plan.lifetime;
        if(!owner.instance||owner.instance!=spirit.instance||
           !spirit.actor||!spirit.generation||owner==spirit||
           expires<=now)return false;
        std::lock_guard lock(mutex_);
        const auto source=players_.find(owner.actor);
        const auto placed=npcs_.find(spirit.actor);
        const auto live=npc_combat_.find(spirit.actor);
        if(source==players_.end()||source->second.effect_scope!=owner||
           !source->second.resources||source->second.precise_health<=0||
           placed==npcs_.end()||live==npc_combat_.end()||
           live->second.hex_scope()!=spirit||
           placed->second.definition.level!=plan.level||
           live->second.creature_type(live->second.lifetime())!=
               std::optional<std::string>{"spirit"})return false;
        const auto* body=live->second.actor(live->second.lifetime());
        if(!body||!body->health||body->health->current<=0||
           quickening_spirits_.size()>=1024)return false;
        return quickening_spirits_.emplace(spirit,
            QuickeningSpirit{owner,spirit,now,expires}).second;
    }

    // Completed-cast producer for a caller-supplied, reviewed 2022 NPC
    // definition. No network request or capture-only model can enter here.
    // The returned spawn batch belongs after the cast-finish packet.
    std::optional<skills::effects::Scope> spawn_quickening_zephyr_spirit(
            skills::effects::Scope owner,
            const skills::effects::QuickeningZephyrPlan& plan,Npc entry,
            std::uint64_t now,std::vector<Packet>& packets){
        packets.clear();
        if(skills::effects::quickening_zephyr(plan.rank)!=plan||
           now>UINT64_MAX-plan.lifetime||!entry.combat_definition||
           !entry.definition.id||!entry.definition.model_file||
           entry.definition.level!=plan.level||
           entry.combat_definition->binding.map==0||
           entry.combat_definition->binding.definition!=entry.definition.id||
           entry.combat_definition->binding.model_file!=entry.definition.model_file||
           entry.combat_definition->binding.level!=plan.level||
           entry.combat_definition->creature_type!=std::optional<std::string>{"spirit"}||
           entry.combat_definition->creature_type_evidence.empty()||
           entry.combat_definition->holy_traits.summoned!=std::optional(true)||
           entry.combat_definition->summoned_evidence.empty()||
           entry.combat_definition->relation!=combat::npcs::Relation::allied_to_players||
           !entry.combat_definition->spawn_health||
           entry.combat_definition->spawn_health->current<=0||
           !entry.combat_definition->damageable||
           entry.combat_definition->healing_allowed!=std::optional(false)||
           entry.combat_definition->healing_evidence.empty()||
           entry.combat_definition->fleshy!=std::optional(false)||
           entry.combat_definition->fleshy_evidence.empty()||
           entry.xunlai_storage||entry.guild_emblemer||entry.guild_trim_vendor)return {};
        std::lock_guard lock(mutex_);
        const auto source=players_.find(owner.actor);
        if(source==players_.end()||source->second.effect_scope!=owner||
           !source->second.resources||source->second.precise_health<=0||
           !source->second.team_token||
           npcs_.size()>=1024||quickening_spirits_.size()>=1024||
           next_actor_generation_==UINT64_MAX)return {};
        const auto origin=healing_position_locked(source->second,now);
        if(!origin)return {};
        auto actor=next_quest_actor_;
        while(actor<=65535&&(players_.contains(actor)||npcs_.contains(actor)||
              ground_items_.used_actor(actor)||retired_quest_actors_.contains(actor)))++actor;
        if(actor>65535)return {};
        entry.id=actor;entry.x=float(origin->x);entry.y=float(origin->z);
        entry.plane=source->second.surface.plane;
        auto& body=*entry.combat_definition;
        body.binding.actor=actor;
        bool existing_definition=false;
        for(const auto& [old_id,old]:npcs_)
            if(old.definition.id==entry.definition.id){
                // A recast may reuse a definition only with identical 2022
                // appearance. Do not replace a live client's model record.
                if(old.definition.model_file!=entry.definition.model_file||
                   old.definition.skin_file!=entry.definition.skin_file||
                   old.definition.visual_adjustment!=entry.definition.visual_adjustment||
                   old.definition.appearance!=entry.definition.appearance||
                   old.definition.flags!=entry.definition.flags||
                   old.definition.primary!=entry.definition.primary||
                   old.definition.level!=entry.definition.level||
                   old.definition.encoded_name!=entry.definition.encoded_name||
                   old.model_files!=entry.model_files||
                   old.literal_display_name!=entry.literal_display_name)return {};
                existing_definition=true;break;
            }
        const skills::effects::Scope spirit{instance_scope_,actor,next_actor_generation_};
        Instance validation;
        if(navigation_)validation.configure_navigation(navigation_);
        try{validation.configure_npcs({entry});packets=validation.npc_packets();}
        catch(const std::exception&){packets.clear();return {};}
        if(existing_definition)std::erase_if(packets,[&](const Packet& packet){
            if(packet.size()<6||(packet[0]!=85&&packet[0]!=86)||packet[1])return false;
            const auto id=std::uint32_t(packet[2])|(std::uint32_t(packet[3])<<8)|
                (std::uint32_t(packet[4])<<16)|(std::uint32_t(packet[5])<<24);
            return id==entry.definition.id;
        });
        if(packets.empty())return {};
        auto next_npcs=npcs_;
        auto next_combat=npc_combat_;
        auto next_owners=quest_npc_owners_;
        auto next_auras=quickening_spirits_;
        auto next_unpublished=unpublished_quickening_spirits_;
        next_npcs.emplace(actor,entry);
        next_combat.emplace(actor,combat::npcs::LiveActor(body,
            {instance_scope_,instance_scope_,next_actor_generation_,body.binding.map,actor}));
        next_owners.emplace(actor,std::pair{owner,0u});
        if(!next_auras.emplace(spirit,QuickeningSpirit{owner,spirit,now,
                now+plan.lifetime}).second){packets.clear();return {};}
        if(!next_unpublished.insert(spirit).second){packets.clear();return {};}
        npcs_.swap(next_npcs);npc_combat_.swap(next_combat);
        quest_npc_owners_.swap(next_owners);quickening_spirits_.swap(next_auras);
        unpublished_quickening_spirits_.swap(next_unpublished);
        bind_npc_ward_queries_locked();next_quest_actor_=actor+1;
        ++next_actor_generation_;
        return spirit;
    }

    // Called by the skill host only after the completion prefix is queued.
    // Until then the actor is hidden from both entry snapshots and visibility
    // refreshes. This also makes a duplicate publish fail closed.
    bool publish_quickening_zephyr_spirit(skills::effects::Scope spirit,
            std::uint64_t now){
        std::lock_guard lock(mutex_);
        const auto pending=unpublished_quickening_spirits_.find(spirit);
        const auto aura=quickening_spirits_.find(spirit);
        const auto npc=npc_combat_.find(spirit.actor);
        if(pending==unpublished_quickening_spirits_.end()||
           aura==quickening_spirits_.end()||npc==npc_combat_.end()||
           npc->second.hex_scope()!=spirit||now<aura->second.begins||
           now>=aura->second.expires)return false;
        const auto* actor=npc->second.actor(npc->second.lifetime());
        if(!actor||!actor->health||actor->health->current<=0)return false;
        // Archived Spirit.html: a newly summoned allied spirit replaces
        // the same type within passive spirit range. Stage first, replace
        // only at publication so a cancelled/unpublished ritual cannot kill
        // an existing aura. Team membership is host authority, not allegiance.
        const auto owner=players_.find(aura->second.owner.actor);
        const auto placed=npcs_.find(spirit.actor);
        if(owner==players_.end()||owner->second.effect_scope!=aura->second.owner||
           !owner->second.team_token||placed==npcs_.end())return false;
        std::vector<QuickeningSpirit> replaced;
        for(const auto& [scope,old]:quickening_spirits_){
            if(scope==spirit||scope.instance!=spirit.instance||old.expires<=now||
               unpublished_quickening_spirits_.contains(scope))continue;
            const auto ally=players_.find(old.owner.actor);
            const auto old_npc=npcs_.find(scope.actor);
            const auto summoned=quest_npc_owners_.find(scope.actor);
            if(ally==players_.end()||ally->second.effect_scope!=old.owner||
               ally->second.team_token!=owner->second.team_token||
               old_npc==npcs_.end()||old_npc->second.plane!=placed->second.plane||
               summoned==quest_npc_owners_.end()||
               summoned->second!=std::pair{old.owner,0u})continue;
            if(std::hypot(double(old_npc->second.x)-placed->second.x,
                          double(old_npc->second.y)-placed->second.y)<=2512.)
                replaced.push_back(old);
        }
        for(const auto& old:replaced)
            retire_quest_npcs_locked(old.owner,0,old.spirit.actor);
        unpublished_quickening_spirits_.erase(pending);
        if(npc_visibility_radius_)refresh_npc_visibility_locked();
        else{
            const std::set<std::uint32_t> only{spirit.actor};
            publish_new_npcs_locked(npc_packets_locked(&only));
        }
        return true;
    }

    // A null result means the player's lifetime/state is unavailable. False
    // means no registered, live in-range spirit at this completion instant.
    std::optional<bool> quickening_zephyr_affects_player(
            skills::effects::Scope caster,std::uint64_t now){
        std::lock_guard lock(mutex_);
        const auto found=players_.find(caster.actor);
        if(found==players_.end()||found->second.effect_scope!=caster||
           !found->second.resources||found->second.precise_health<=0||
           !settle_timed_effects_locked(now))return {};
        const auto origin=healing_position_locked(found->second,now);
        if(!origin)return {};
        constexpr double spirit_range=2512.; // pinned GWCA Range::Spirit
        for(const auto& [scope,record]:quickening_spirits_){
            if(scope.instance!=caster.instance||record.begins>now||
               record.expires<=now||unpublished_quickening_spirits_.contains(scope))continue;
            const auto placed=npcs_.find(scope.actor);
            const auto live=npc_combat_.find(scope.actor);
            if(placed==npcs_.end()||live==npc_combat_.end()||
               live->second.hex_scope()!=scope||
               placed->second.plane!=found->second.surface.plane)continue;
            const auto* actor=live->second.actor(live->second.lifetime());
            if(!actor||!actor->health||actor->health->current<=0||
               live->second.creature_type(live->second.lifetime())!=
                   std::optional<std::string>{"spirit"})continue;
            if(std::hypot(double(placed->second.x)-origin->x,
                          double(placed->second.y)-origin->z)<=spirit_range)
                return true;
        }
        return false;
    }
