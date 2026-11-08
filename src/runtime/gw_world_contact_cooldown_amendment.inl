// Included inside Instance. Caller holds the existing World mutex.
    std::shared_ptr<skills::effects::ContactCooldownContinuation> retain_contact_cooldown_locked(
            skills::effects::Scope source,skills::Slot slot,std::uint64_t use,std::uint64_t first_at){
        const auto p=players_.find(source.actor);const auto pending=pending_diversion_uses_.find(source);
        if(p==players_.end()||p->second.effect_scope!=source||!use||
           pending==pending_diversion_uses_.end()||pending->second.prepared.use.event!=use||
           pending->second.prepared.use.slot!=slot||pending->second.prepared.at!=first_at)return {};
        std::erase_if(contact_cooldown_continuations_,[](const auto& row){return row.second.expired();});
        const auto key=std::pair{source,use};
        if(contact_cooldown_continuations_.contains(key)||contact_cooldown_continuations_.size()>=4096)return {};
        auto result=std::shared_ptr<skills::effects::ContactCooldownContinuation>(
            new skills::effects::ContactCooldownContinuation(source,slot,use,first_at));
        contact_cooldown_continuations_.emplace(key,result);return result;
    }
    bool owns_contact_cooldown_locked(const std::shared_ptr<skills::effects::ContactCooldownContinuation>& continuation)const{
        if(!continuation)return false;
        const auto i=contact_cooldown_continuations_.find({continuation->source_,continuation->use_});
        return i!=contact_cooldown_continuations_.end()&&i->second.lock()==continuation;
    }
    // Only canonical World release/group code calls this with the actual event;
    // callers must not predict next_attack_outcome_ or accept a client event ID.
    bool bind_contact_event_locked(const std::shared_ptr<skills::effects::ContactCooldownContinuation>& continuation,
            unsigned index,std::uint64_t event){
        if(!owns_contact_cooldown_locked(continuation)||index>1||!event||continuation->spent_||
           (continuation->events_[index]&&continuation->events_[index]!=event)||
           continuation->events_[1-index]==event)return false;
        continuation->events_[index]=event;return true;
    }
    void bind_contact_cooldown_completion_locked(const skills::effects::CooldownCompletion& receipt){
        const auto i=contact_cooldown_continuations_.find({receipt.stamp.owner,receipt.skill_use});
        if(i==contact_cooldown_continuations_.end())return;
        const auto continuation=i->second.lock();if(!continuation)return;
        if(continuation->slot_!=receipt.slot||continuation->first_at_!=receipt.stamp.at||
           !continuation->events_[0]||continuation->completion_)return;
        continuation->completion_=receipt;
    }
    std::optional<skills::effects::ContactCooldownQuote> preflight_contact_amendment_locked(
            const std::shared_ptr<skills::effects::ContactCooldownContinuation>& continuation,std::uint64_t event,std::uint64_t now){
        if(!owns_contact_cooldown_locked(continuation)||continuation->spent_||!continuation->completion_||
           !continuation->events_[0]||(continuation->events_[1]&&continuation->events_[1]!=event)||!event)return {};
        const auto found=players_.find(continuation->source_.actor);
        if(found==players_.end()||found->second.effect_scope!=continuation->source_)return {};
        const auto& p=found->second;
        if(!p.local_cooldown_snapshot_ready||!p.local_cooldown_snapshot||!p.cooldown_authority||
           !p.resources||p.precise_health<=0||now<p.health_updated||p.cooldown_amendments.size()>=256)return {};
        const auto& verified=*p.local_cooldown_snapshot;const auto& snapshot=verified.snapshot;
        const auto amendment_order=p.cooldown_amendment_order;
        const auto completion_order=p.cooldown_completions.empty()?0:p.cooldown_completions.back().stamp.order;
        const auto reset_order=std::max(p.flourish_reset_order,p.attack_reset_delivery?*p.attack_reset_delivery->acknowledged_order(p.effect_scope):std::uint64_t{0});
        if(snapshot.owner!=p.effect_scope||snapshot.at!=now||snapshot.control_epoch!=p.control_epoch||
           snapshot.completion_order!=completion_order||snapshot.reset_order!=reset_order||snapshot.amendment_order!=amendment_order||
           (p.attack_reset_delivery&&!p.attack_reset_delivery->pending(p.effect_scope)->empty()))return {};
        std::uint64_t before=0;
        for(const auto& timer:snapshot.timers)if(timer.slot==continuation->slot_){
            if(std::find(verified.verified.begin(),verified.verified.end(),timer.slot)==verified.verified.end())return {};
            before=timer.until;
        }
        const auto until=skills::effects::contact_amendment_deadline(now,before);if(!until)return {};
        // Shield Bash may mint its knockdown/control stamp before the later
        // amendment. Prove capacity for both before committing either effect.
        auto clock=*p.cooldown_authority;if(!clock.order(p.effect_scope,now)||!clock.order(p.effect_scope,now))return {};
        skills::effects::ContactCooldownQuote quote;quote.continuation_=continuation;quote.event_=event;quote.at_=now;
        quote.before_until_=before;quote.until_=*until;quote.completion_order_=completion_order;
        quote.reset_order_=reset_order;quote.amendment_order_=amendment_order;return quote;
    }
    std::optional<skills::effects::CooldownAmendment> commit_contact_amendment_locked(
            const skills::effects::ContactCooldownQuote& quote,std::uint64_t event,std::uint64_t now){
        const auto& continuation=quote.continuation_;
        if(!owns_contact_cooldown_locked(continuation)||continuation->spent_||!continuation->completion_||
           quote.at_!=now||quote.event_!=event||continuation->events_[1]!=event)return {};
        const auto found=players_.find(continuation->source_.actor);
        if(found==players_.end()||found->second.effect_scope!=continuation->source_)return {};
        auto& p=found->second;
        const auto delivery=attack_outcomes_.find(continuation->source_);
        if(delivery==attack_outcomes_.end())return {};
        const auto buffered=delivery->second.buffered(continuation->source_);if(!buffered)return {};
        const auto terminal=std::find_if(buffered->begin(),buffered->end(),[&](const auto& value){
            return value.event==event&&value.attacker==continuation->source_&&value.at==now&&value.delivery==combat::weapon::AttackDelivery::melee&&
                !value.skill_failed&&(value.outcome==AttackOutcome::block||value.outcome==AttackOutcome::hit);
        });
        if(terminal==buffered->end())return {};
        const auto reset_order=std::max(p.flourish_reset_order,p.attack_reset_delivery?*p.attack_reset_delivery->acknowledged_order(p.effect_scope):std::uint64_t{0});
        if(!p.resources||p.precise_health<=0||now<p.health_updated||!p.cooldown_authority||p.cooldown_amendments.size()>=256||
           (p.cooldown_completions.empty()?0:p.cooldown_completions.back().stamp.order)!=quote.completion_order_||
           (p.cooldown_amendment_order)!=quote.amendment_order_||reset_order!=quote.reset_order_)return {};
        auto clock=*p.cooldown_authority;const auto stamp=clock.order(p.effect_scope,now);if(!stamp)return {};
        const skills::effects::CooldownAmendment result{*stamp,continuation->slot_,continuation->use_,event,
            continuation->completion_->stamp,quote.before_until_,quote.until_};
        p.cooldown_amendments.push_back(result);p.cooldown_authority=std::move(clock);
        p.cooldown_amendment_order=stamp->order;p.local_cooldown_snapshot_ready=false;continuation->spent_=true;return result;
    }
