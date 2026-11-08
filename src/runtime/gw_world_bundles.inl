// Trusted transient bundle ownership. No normal pickup/drop request is bound
// until content supplies a reviewed drop policy and peer equipment is proved.
private:
    struct HeldBundle {ground::Item original;std::uint16_t inventory=0;};
    std::map<skills::effects::Scope,bundles::State> bundle_states_;
    std::map<skills::effects::Scope,HeldBundle> held_bundles_;
    struct PendingBundleRelease {
        navigation::SurfacePoint origin;std::uint64_t at=0,generation=0;bundles::Cause cause;
    };
    std::map<skills::effects::Scope,PendingBundleRelease> pending_bundle_releases_;
    std::uint64_t next_bundle_event_=1;
    bool holding_bundle_locked(skills::effects::Scope owner)const{
        const auto state=bundle_states_.find(owner);
        return state!=bundle_states_.end()&&state->second.held().has_value();
    }
    std::optional<skills::effects::ShieldEquipment> effective_shield_locked(const Player& player)const{
        auto equipment=player.reviewed_shield;
        if(equipment&&holding_bundle_locked(player.effect_scope))equipment->shield=0;
        return equipment;
    }
    // A persistent held object still owns its eventual floor capacity. Normal
    // loot/drop producers may not consume this lease between pickup and release.
    bool ground_capacity_available_locked()const{
        std::size_t reserved=0;
        for(const auto& [owner,state]:bundle_states_)
            if(state.held()&&state.held()->disposition==bundles::Disposition::ground)++reserved;
        return ground_items_.size()<ground::State::capacity&&
            reserved<ground::State::capacity-ground_items_.size();
    }
    struct BundleDropProposal {
        skills::effects::Scope owner;
        std::optional<bundles::State::End> transition;
        std::optional<ground::State> ground;
        std::optional<ground::Item> returned;
        std::uint64_t ground_revision=0,actor_generation=0,event=0;
        std::uint32_t item_cursor=0,actor_cursor=0,next_item=0,next_actor=0;
        Packet owner_clear;std::vector<Packet> shared;
        std::optional<bundles::EnergyTransition> energy;
        std::vector<Packet> owner_energy;
    };
    std::optional<bundles::EnergyTransition> prepare_bundle_energy_locked(const Player& player,bool before,bool after,std::uint64_t now)const{
        if(!player.equipment_energy_configured||!player.resources)return {};
        const bundles::EnergySources sources{player.morale_energy_base,static_cast<int>(player.morale_energy_equipment),
            static_cast<int>(player.hand_energy_capacity),player.morale};
        const auto curve=player.overcast_curve?*player.overcast_curve:overcast::CeilingCurve(player.effect_scope,player.energy_updated,1);
        return bundles::prepare_energy_transition(sources,before,after,curve,player.effect_scope,*player.resources,
            player.energy_regeneration,player.energy_debt,now,player.energy_pips,player.energy_fractional_tail);
    }
    std::optional<BundleDropProposal> prepare_bundle_drop_locked(skills::effects::Scope owner,std::uint64_t now,
            bundles::Cause cause,const navigation::SurfacePoint& origin)const{
        BundleDropProposal result;result.owner=owner;
        const auto state=bundle_states_.find(owner);
        if(state==bundle_states_.end()||!state->second.held())return result;
        const auto player=players_.find(owner.actor);
        if(player!=players_.end()&&player->second.effect_scope==owner&&player->second.equipment_energy_configured){
            // Live callers settle timed resource boundaries before preparation;
            // dead departure work may advance its no-regeneration snapshot here.
            if(player->second.precise_health>0&&player->second.energy_updated!=now)return {};
            result.energy=prepare_bundle_energy_locked(player->second,true,false,now);
            if(!result.energy)return {};
        }
        const auto held=held_bundles_.find(owner);
        if(held==held_bundles_.end()||held->second.original.definition.id!=state->second.held()->item.id||
           origin.surface.plane>65535||!next_bundle_event_||next_bundle_event_==UINT64_MAX)return {};
        result.transition=state->second.drop(owner,state->second.held()->generation,next_bundle_event_,now,cause,
            origin.point.x,origin.point.z,static_cast<std::uint16_t>(origin.surface.plane));
        if(!result.transition)return {};
        result.event=next_bundle_event_;result.ground_revision=ground_items_.revision();
        result.item_cursor=next_ground_item_;result.actor_cursor=next_ground_actor_;result.actor_generation=next_actor_generation_;
        result.owner_clear=items::held_bundle_2022(held->second.inventory,0);
        result.shared.push_back(items::delete_2022(held->second.original.definition.id));
        if(state->second.held()->disposition==bundles::Disposition::ground){
            if(!next_actor_generation_||next_actor_generation_==UINT64_MAX)return {};
            auto wire=next_ground_item_;while(wire<=59999&&ground_items_.used_item(wire))++wire;
            auto actor=next_ground_actor_;
            while(actor<=65535&&(players_.contains(actor)||npcs_.contains(actor)||ground_items_.used_actor(actor)||retired_quest_actors_.contains(actor)))++actor;
            if(wire>59999||actor>65535)return {};
            auto floor=held->second.original;floor.scope={instance_scope_,actor,next_actor_generation_};
            floor.source=owner;floor.source_item=state->second.held()->item.id;floor.definition.id=wire;
            floor.position={origin.point.x,origin.point.z,static_cast<std::uint16_t>(origin.surface.plane)};
            floor.created=now;floor.allocation.reset();
            result.ground=ground_items_;if(!result.ground->add(floor))return {};
            auto packets=ground::presentation(floor,now);result.shared.insert(result.shared.end(),packets.begin(),packets.end());
            result.returned=std::move(floor);result.next_item=wire+1;result.next_actor=actor+1;
        }
        return result;
    }
    bool commit_bundle_drop_locked(BundleDropProposal& proposal,bool publish=true,bool retiring=false){
        if(!proposal.transition)return true;
        const auto owner=players_.find(proposal.owner.actor);const auto state=bundle_states_.find(proposal.owner);
        const auto held=held_bundles_.find(proposal.owner);
        const bool live_owner=owner!=players_.end()&&owner->second.effect_scope==proposal.owner;
        const auto pending=pending_bundle_releases_.find(proposal.owner);
        if((!live_owner&&!retiring)||(!retiring&&pending!=pending_bundle_releases_.end())||
           (retiring&&(pending==pending_bundle_releases_.end()||pending->second.generation!=proposal.transition->drop.held.generation))||
           state==bundle_states_.end()||!state->second.held()||
           held==held_bundles_.end()||state->second.held()->generation!=proposal.transition->drop.held.generation||
           state->second.held()->item.id!=proposal.transition->drop.held.item.id||next_bundle_event_!=proposal.event||
           ground_items_.revision()!=proposal.ground_revision||next_ground_item_!=proposal.item_cursor||
           next_ground_actor_!=proposal.actor_cursor||next_actor_generation_!=proposal.actor_generation)return false;
        state->second=std::move(proposal.transition->state);held_bundles_.erase(held);++next_bundle_event_;
        if(proposal.ground){ground_items_=std::move(*proposal.ground);next_ground_item_=proposal.next_item;next_ground_actor_=proposal.next_actor;++next_actor_generation_;}
        if(live_owner&&proposal.energy){
            commit_overcast_energy_locked(owner->second,proposal.energy->after);
            proposal.owner_energy=owner_vitals(owner->second);
        }
        if(publish){
            if(live_owner)owner->second.outbox->enqueue({proposal.owner_clear});
            for(auto&[id,peer]:players_)peer.outbox->enqueue(proposal.shared);
            if(live_owner&&!proposal.owner_energy.empty())owner->second.outbox->enqueue(proposal.owner_energy);
        }
        return true;
    }
    void queue_bundle_release_locked(const Player& player,std::uint64_t now,bundles::Cause cause){
        const auto state=bundle_states_.find(player.effect_scope);
        if(state==bundle_states_.end()||!state->second.held()||pending_bundle_releases_.contains(player.effect_scope))return;
        const auto origin=shadow_position_locked(player,now);
        if(!origin)throw std::logic_error("bundle departure origin unavailable");
        pending_bundle_releases_.emplace(player.effect_scope,PendingBundleRelease{*origin,now,state->second.held()->generation,cause});
    }
    void dispatch_bundle_releases_locked(std::uint64_t now){
        for(auto it=pending_bundle_releases_.begin();it!=pending_bundle_releases_.end();){
            if(it->second.at>now){++it;continue;}
            auto proposal=prepare_bundle_drop_locked(it->first,now,it->second.cause,it->second.origin);
            if(!proposal||!proposal->transition||!commit_bundle_drop_locked(*proposal,true,true)){++it;continue;}
            const auto player=players_.find(it->first.actor);
            if(player==players_.end()||player->second.effect_scope!=it->first)bundle_states_.erase(it->first);
            it=pending_bundle_releases_.erase(it);
        }
    }
public:
    // Trusted session transaction. Callback performs only its durable inventory
    // CAS and must not reenter World. Failed persistence leaves the bundle held.
    bool switch_weapon_set(skills::effects::Scope owner,std::optional<std::uint64_t> generation,
            std::uint64_t now,const std::function<bool()>& persist){
        if(!persist)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner||
           !resurrection_clock_valid(player->second,now)||pending_bundle_releases_.contains(owner))return false;
        if(holding_bundle_locked(owner)&&!advance_resource_event_locked(player->second,now))return false;
        const auto state=bundle_states_.find(owner);
        const auto held=state==bundle_states_.end()?std::nullopt:state->second.held();
        if(bool(held)!=bool(generation)||(held&&held->generation!=*generation))return false;
        std::optional<BundleDropProposal> proposal;
        if(held){
            const auto origin=shadow_position_locked(player->second,now);if(!origin)return false;
            proposal=prepare_bundle_drop_locked(owner,now,bundles::Cause::weapon_switch,*origin);
            if(!proposal||!proposal->transition)return false;
        }
        if(!persist())return false;
        if(proposal&&!commit_bundle_drop_locked(*proposal))throw std::logic_error("committed weapon switch lost bundle proposal");
        return true;
    }
    std::optional<bundles::Held> held_bundle(skills::effects::Scope owner)const{
        std::lock_guard lock(mutex_);const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner)return {};
        const auto state=bundle_states_.find(owner);return state==bundle_states_.end()?std::nullopt:state->second.held();
    }
    bool pickup_ground_bundle(skills::effects::Scope owner,skills::effects::Scope item,std::uint16_t inventory,
            bundles::Disposition disposition,float range,std::uint64_t now){
        if(!inventory)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner||!player->second.resources||
           !advance_resource_event_locked(player->second,now)||player->second.precise_health<=0)return false;
        const auto origin=shadow_position_locked(player->second,now);const auto floor=ground_items_.find(item);
        if(!origin||!floor||origin->surface.plane>65535||!ground::can_pickup(*floor,owner,
           {origin->point.x,origin->point.z,static_cast<std::uint16_t>(origin->surface.plane)},now,range))return false;
        const auto current=bundle_states_.find(owner);
        const auto proposed=(current==bundle_states_.end()?bundles::State(owner):current->second).acquire(owner,floor->definition,disposition,now);
        if(!proposed)return false;
        const auto energy=prepare_bundle_energy_locked(player->second,false,true,now);
        if(player->second.equipment_energy_configured&&!energy)return false;
        auto staged=ground_items_;if(!staged.consume(item,1))return false;const auto original=*floor;
        auto assignment=items::held_bundle_2022(inventory,original.definition.id);
        ground_items_=std::move(staged);bundle_states_.insert_or_assign(owner,*proposed);
        held_bundles_.insert_or_assign(owner,HeldBundle{original,inventory});
        cancel_ordinary_locked(player->second,combat::weapon::Cancel::explicit_input);
        cancel_npc_interaction(player->second);
        for(auto&[id,peer]:players_){
            if(peer.selected_agent==item.actor)peer.selected_agent=0;
            if(peer.secondary_selected_agent==item.actor)peer.secondary_selected_agent=0;
            peer.outbox->enqueue({ground::despawn_2022(item.actor)});
        }
        // Keep the item definition alive;it is now held,not credited to a bag.
        player->second.outbox->enqueue({assignment});
        if(energy){
            const auto before=mind_wrack_energy_locked(player->second);
            commit_overcast_energy_locked(player->second,energy->after);
            player->second.outbox->enqueue(owner_vitals(player->second));
            observe_mind_wrack_energy_locked(player->second,before,now);
        }
        return true;
    }
    bool drop_held_bundle(skills::effects::Scope owner,std::uint64_t generation,std::uint64_t now,bundles::Cause cause=bundles::Cause::manual){
        std::lock_guard lock(mutex_);const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner)return false;
        if(!advance_resource_event_locked(player->second,now))return false;
        const auto state=bundle_states_.find(owner);
        if(state==bundle_states_.end()||!state->second.held()||state->second.held()->generation!=generation)return false;
        const auto origin=shadow_position_locked(player->second,now);if(!origin)return false;
        auto proposal=prepare_bundle_drop_locked(owner,now,cause,*origin);
        return proposal&&proposal->transition&&commit_bundle_drop_locked(*proposal);
    }
