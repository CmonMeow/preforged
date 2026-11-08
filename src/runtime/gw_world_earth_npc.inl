// Included inside Instance. Caller holds the World lock and owns
// whole-batch admission plus fixed recipient membership/distance.
private:
    bool commit_npc_earth_sequence_locked(skills::effects::Scope source,
            skills::effects::Scope target,const skills::effects::AoEPlan& plan,
            double distance,unsigned level,std::uint64_t now){
        if((plan.skill!=174&&plan.skill!=937)||!source.instance||!source.actor||source.actor>65535||
           !source.generation||target.instance!=source.instance||target.actor==source.actor||
           !target.actor||target.actor>65535||!target.generation||!level||level>255||
           !std::isfinite(distance)||distance<0||
           (plan.skill==174?!skills::effects::valid_aftershock_plan(plan):!skills::effects::valid_shockwave_plan(plan)))return false;
        const auto initial=npc_combat_.find(target.actor);
        if(initial==npc_combat_.end()||initial->second.hex_scope()!=target)return true;
        const auto expected=initial->second.lifetime();
        const auto* initial_actor=initial->second.actor(expected);
        if(!initial_actor||!initial_actor->health||initial_actor->health->current<=0)return true;
        const auto current=[&](){const auto found=npc_combat_.find(target.actor);
            return found!=npc_combat_.end()&&found->second.lifetime()==expected;};
        const auto credit=[&]()->Player*{const auto found=players_.find(source.actor);
            return found!=players_.end()&&found->second.effect_scope==source?&found->second:nullptr;};
        const auto refused=[&](const char* reason){
            std::cerr<<"Earth AoE late NPC refusal actor="<<target.actor<<" generation="<<target.generation
                     <<" skill="<<plan.skill<<" reason="<<reason<<std::endl;return false;};
        // No reference obtained before a World callback survives this boundary.
        const auto ended=[&](bool owns_death=false){
            const auto found=npc_combat_.find(target.actor);const auto placed=npcs_.find(target.actor);
            if(found==npc_combat_.end()||placed==npcs_.end()||found->second.lifetime()!=expected)return true;
            const auto* actor=found->second.actor(expected);
            if(!actor||!actor->health)return true;
            if(actor->health->current>0)return false;
            if(owns_death)record_npc_death_locked(credit(),placed->second,found->second,now);return true;
        };
        bool finished=false,shook=false;
        const auto damage_event=[&](double amount){
            if(ended()){finished=true;return true;}
            const auto found=npc_combat_.find(target.actor);const auto placed=npcs_.find(target.actor);
            if(found==npc_combat_.end()||placed==npcs_.end()||found->second.lifetime()!=expected){finished=true;return true;}
            std::optional<combat::Hit> actual;
            if(!commit_npc_reversal_or_damage_locked(credit(),placed->second,found->second,
                    {combat::Kind::earth,amount,level,plan.skill},now,nullptr,nullptr,&actual,true,source))
                return refused("shared_damage");
            // Healing-conversion death has no actual damage Hit. Acceptance,
            // rather than a fabricated Hit, owns this single cast visual.
            if(plan.skill==174&&!shook){
                for(auto& [id,peer]:players_)peer.outbox->enqueue({skills::particles::aftershock_screen_shake_2022(target.actor)});
                shook=true;
            }
            if(ended(true))finished=true;
            return true;
        };
        for(unsigned index=0;index<plan.count;++index){
            const auto& strike=plan.strikes[index];if(distance>strike.radius)continue;
            if(!damage_event(strike.earth_damage))return false;
            if(finished)return true;
            if(plan.skill==174){
                // The captured bonus is a second damage event, with its own
                // cumulative protection/conversion/armor-charge authority.
                const auto found=npc_combat_.find(target.actor);
                if(found==npc_combat_.end()||found->second.lifetime()!=expected)return true;
                if(strike.knocked_down_bonus>0&&found->second.knocked_down(expected,now)){
                    if(!damage_event(strike.knocked_down_bonus))return false;
                    if(finished)return true;
                }
                continue;
            }
            const auto condition=strike.condition==skills::effects::Condition::weakness?combat::conditions::Kind::weakness:
                strike.condition==skills::effects::Condition::cracked_armor?combat::conditions::Kind::cracked_armor:
                combat::conditions::Kind::blind;
            auto found=npc_combat_.find(target.actor);
            if(found==npc_combat_.end()||found->second.lifetime()!=expected)return true;
            if(!found->second.apply_condition(expected,condition,source.actor,now,
                    std::uint64_t(strike.duration_seconds)*1000))return refused("condition");
            if(!current())return true;
            npc_condition_credit_[{target.actor,source.actor}]={target,source};
            found=npc_combat_.find(target.actor);
            // Local condition evaluation may already have queued a lethal
            // reaction. Publish its death only with that reaction's damage,
            // after the condition status that caused it.
            const auto condition_effects=found->second.actor(expected)->effects&~0x10u;
            for(auto& [id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,condition_effects)});
            if(publish_npc_fragility_locked(target.actor,found->second))return true;
            if(ended())return true;
            commit_applications_locked(target,now);
            if(ended())return true;
            found=npc_combat_.find(target.actor);
            const auto* actor=found->second.actor(expected);
            for(auto& [id,peer]:players_)peer.outbox->enqueue({combat::detail::fraction(target.actor,*actor->health),
                emotes::effects_2022(target.actor,actor->effects)});
        }
        if(ended())return true;
        const auto found=npc_combat_.find(target.actor);const auto* actor=found->second.actor(expected);
        for(auto& [id,peer]:players_)peer.outbox->enqueue({combat::detail::fraction(target.actor,*actor->health),
            emotes::effects_2022(target.actor,actor->effects)});
        return true;
    }
