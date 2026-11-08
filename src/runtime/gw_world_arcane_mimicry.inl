private:
    bool mimicry_donor_current_locked(const skills::effects::MimicryDonorLease& lease,std::uint64_t at)const{
        const auto plan=skills::effects::arcane_mimicry();
        if(!plan||at!=lease.at_||lease.source_.skill!=65||lease.owner_==lease.target_||
           lease.owner_.instance!=instance_scope_||lease.target_.instance!=instance_scope_)return false;
        const auto source=players_.find(lease.owner_.actor),target=players_.find(lease.target_.actor);
        if(source==players_.end()||target==players_.end()||source->second.effect_scope!=lease.owner_||
           target->second.effect_scope!=lease.target_)return false;
        const auto& a=source->second;const auto& b=target->second;
        if(a.death_sequence!=lease.owner_death_||b.death_sequence!=lease.target_death_)return false;
        if(!a.resources||!b.resources||a.precise_health<=0||b.precise_health<=0||!a.team_token||a.team_token!=b.team_token||
           !resurrection_clock_valid(a,at)||!resurrection_clock_valid(b,at)||!a.cooldown_authority||!b.cooldown_authority)return false;
        const auto owner=a.cooldown_authority->snapshot(lease.owner_),donor=b.cooldown_authority->snapshot(lease.target_);
        if(!owner||!donor||owner->published.at>at||donor->published.at>at||
           owner->revision!=lease.owner_bar_.revision||owner->bar!=lease.owner_bar_.bar||
           donor->revision!=lease.target_bar_.revision||donor->bar!=lease.target_bar_.bar||
           std::find(owner->bar.begin(),owner->bar.end(),lease.source_)==owner->bar.end())return false;
        const auto from=healing_position_locked(a,at),to=healing_position_locked(b,at);
        return from&&to&&std::hypot(from->x-to->x,from->z-to->z)<=plan->range;
    }
public:
    std::optional<skills::effects::MimicryDonorLease> prepare_mimicry_donor(skills::effects::Scope owner,
            skills::effects::Scope target,skills::Slot source,std::uint64_t at){
        std::lock_guard lock(mutex_);settle_damage_fields_locked(at);
        const auto a=players_.find(owner.actor),b=players_.find(target.actor);
        if(a==players_.end()||b==players_.end()||a->second.effect_scope!=owner||b->second.effect_scope!=target||
           !a->second.cooldown_authority||!b->second.cooldown_authority||
           !advance_health_locked(a->second,at)||!advance_health_locked(b->second,at))return {};
        const auto caster=a->second.cooldown_authority->snapshot(owner),donor=b->second.cooldown_authority->snapshot(target);
        if(!caster||!donor)return {};
        auto selection=skills::effects::select_mimicry_donor(donor->bar);
        if(selection.status==skills::effects::MimicrySelection::invalid_bar)return {};
        skills::effects::MimicryDonorLease lease(owner,target,source,*caster,*donor,selection,at,a->second.death_sequence,b->second.death_sequence);
        if(!mimicry_donor_current_locked(lease,at))return {};return lease;
    }
    bool mimicry_donor_current(const skills::effects::MimicryDonorLease& lease,std::uint64_t at)const{
        std::lock_guard lock(mutex_);return mimicry_donor_current_locked(lease,at);
    }
    std::optional<skills::effects::MimicryCopyReceipt> commit_mimicry_copy(const skills::effects::MimicryDonorLease& lease,
            const skills::effects::SkillbarTransition& change,skills::Slot replacement){
        std::lock_guard lock(mutex_);
        if(!mimicry_donor_current_locked(lease,lease.at_)||lease.donor_.status!=skills::effects::MimicrySelection::copy||
           !lease.donor_.slot||replacement.skill!=lease.donor_.slot->skill||!replacement.copy||replacement.copy>65535||
           change.before!=lease.owner_bar_.bar)return {};
        const auto found=std::find(change.before.begin(),change.before.end(),lease.source_);
        if(found==change.before.end())return {};
        auto expected=change.before;expected[found-change.before.begin()]=replacement;if(expected!=change.after)return {};
        auto& player=players_.at(lease.owner_.actor);
        if(!settle_defensive_bar_locked(player,lease.at_)||!mimicry_donor_current_locked(lease,lease.at_))return {};
        auto pools=player.adrenaline;auto authority=*player.cooldown_authority;
        if(!pools.configure(change.after,lease.at_)||!authority.publish(lease.owner_,change.after,lease.at_))return {};
        player.adrenaline=std::move(pools);player.cooldown_authority=std::move(authority);publish_adrenaline_locked(player);
        return skills::effects::MimicryCopyReceipt(lease,replacement);
    }
