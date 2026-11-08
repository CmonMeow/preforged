// Included inside Instance. No effect/replay authority is introduced here.
private:
    struct CompletionPrefixPublication {
        Instance* world=nullptr;skills::effects::Scope source;
        struct Recipient {std::shared_ptr<Outbox> box;std::uint64_t ticket;std::vector<Packet> native;};
        std::vector<Recipient> recipients;
        std::vector<std::unique_ptr<Outbox::PublicationPassthrough>> markers;
        bool ready=false,resolved=false,closed=false;
        bool recipient(const std::shared_ptr<Outbox>& box){
            if(std::any_of(recipients.begin(),recipients.end(),[&](const auto& r){return r.box==box;}))return true;
            if(box->captured_on_this_thread())return false;
            recipients.reserve(recipients.size()+1);markers.reserve(markers.size()+1);
            const auto ticket=resolved?std::optional<std::uint64_t>{0}:box->reserve_publication();if(!ticket)return false;
            recipients.push_back({box,*ticket,{}});
            markers.push_back(std::make_unique<Outbox::PublicationPassthrough>(*box));return true;
        }
        void resolve(bool current){
            if(resolved)return;resolved=true;
            for(auto& r:recipients){if(!current)r.native.clear();r.box->resolve_publication(r.ticket,std::move(r.native));}
        }
        void close(){
            if(closed)return;closed=true;resolve(false);markers.clear();
            const auto found=completion_prefix_publications_.find(world);
            if(found!=completion_prefix_publications_.end()&&found->second==this)completion_prefix_publications_.erase(found);
        }
        ~CompletionPrefixPublication(){close();}
    };
    inline static thread_local std::map<const Instance*,CompletionPrefixPublication*> completion_prefix_publications_;
    CompletionPrefixPublication* completion_prefix_publication_locked()const{
        const auto found=completion_prefix_publications_.find(this);
        return found==completion_prefix_publications_.end()?nullptr:found->second;
    }
    bool completion_source_current_locked(const CompletionPrefixPublication& frame)const{
        const auto source=players_.find(frame.source.actor);
        return source!=players_.end()&&source->second.effect_scope==frame.source;
    }
    void ensure_completion_recipients_locked(){
        if(auto frame=completion_prefix_publication_locked())for(auto& [id,peer]:players_)
            if(!frame->recipient(peer.outbox))throw std::runtime_error("completion outbound reservation unavailable");
    }
    bool route_completion_native_locked(std::uint32_t caller,const std::vector<Packet>& approved){
        auto frame=completion_prefix_publication_locked();
        if(!frame||caller!=frame->source.actor)return false;
        if(!completion_source_current_locked(*frame)){frame->resolve(false);return true;}
        if(frame->resolved)return false;
        ensure_completion_recipients_locked();
        for(auto& [id,peer]:players_){
            auto recipient=std::find_if(frame->recipients.begin(),frame->recipients.end(),[&](const auto& r){return r.box==peer.outbox;});
            if(recipient!=frame->recipients.end())recipient->native.insert(recipient->native.end(),approved.begin(),approved.end());
        }
        // Resolve approved native bytes BEFORE skill_event samples its live
        // publication clock/arms projectile and ground visual publication.
        if(frame->ready)frame->resolve(true);return true;
    }
    // FINAL packet assembly only. Callers retain their existing World lock.
    // Control/publication callbacks that reacquire World must not use this.
    void defer_or_publish_completion_locked(std::function<void()>* publication,std::function<void()> assembled){
        if(!publication){if(assembled)assembled();return;}
        if(completion_prefix_publication_locked()){
            ensure_completion_recipients_locked();if(assembled){assembled();*publication=[]{};}else *publication={};
        }else *publication=std::move(assembled);
    }
public:
    std::optional<skills::casting::CompletionPublication> prepare_completion_publication(
            skills::effects::Scope source,std::uint64_t now){
        (void)now;std::lock_guard lock(mutex_);const auto player=players_.find(source.actor);
        if(player==players_.end()||player->second.effect_scope!=source||completion_prefix_publication_locked())return {};
        auto frame=std::make_shared<CompletionPrefixPublication>();frame->world=this;frame->source=source;
        for(auto& [id,peer]:players_)if(!frame->recipient(peer.outbox))return {};
        completion_prefix_publications_.emplace(this,frame.get());
        skills::casting::CompletionPublication result;
        result.prepare_prefix=[this,frame]{std::lock_guard lock(mutex_);frame->ready=true;};
        result.publish_prefix=[this,frame]{std::lock_guard lock(mutex_);frame->resolve(completion_source_current_locked(*frame));};
        result.finish=[this,frame]{std::lock_guard lock(mutex_);frame->resolve(completion_source_current_locked(*frame));frame->close();};
        return result;
    }
private:
