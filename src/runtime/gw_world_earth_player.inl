// Detached Instance implementation. Caller owns whole-batch admission and
// fixed recipient membership/distance; every actual contact uses current state.
private:
    bool commit_player_earth_sequence_locked(skills::effects::Scope source,
            skills::effects::Scope target,const skills::effects::AoEPlan& plan,
            double distance,unsigned level,std::uint64_t now){
        if((plan.skill!=174&&plan.skill!=937)||!source.instance||!source.actor||source.actor>65535||
           !source.generation||target.instance!=source.instance||target.actor==source.actor||
           !target.actor||target.actor>65535||!target.generation||!level||level>255||
           !std::isfinite(distance)||distance<0||
           (plan.skill==174?!skills::effects::valid_aftershock_plan(plan):!skills::effects::valid_shockwave_plan(plan)))return false;
        const auto current=[&]()->Player*{
            const auto found=players_.find(target.actor);
            return found!=players_.end()&&found->second.effect_scope==target?&found->second:nullptr;
        };
        const auto ended=[&](){const auto* player=current();return !player||!player->resources||player->precise_health<=0;};
        const auto refused=[&](const char* reason){
            std::cerr<<"Earth AoE late player refusal actor="<<target.actor<<" generation="<<target.generation
                     <<" skill="<<plan.skill<<" reason="<<reason<<std::endl;return false;
        };
        bool finished=false,shook=false;
        const auto damage_event=[&](double amount){
            if(ended()){finished=true;return true;}
            auto* player=current();
            if(!player->reviewed_earth_armor)return refused("earth_armor");
            auto actor=incoming_spell_actor_locked(*player,{player->id,player->effect_scope.generation,
                combat::Health{player->precise_health,player->resources->max_health},
                player->reviewed_earth_armor,player->effects,true},now,true);
            const auto hit=combat::apply(actor,{combat::Kind::earth,amount,level,plan.skill},true);
            if(hit.reason!=combat::Reason::accepted)return refused("damage_quote");
            if(!commit_player_reactive_hit_locked(*player,hit,source.actor,now,false,nullptr,false,true,nullptr,source))
                return refused("shared_damage");
            if(plan.skill==174&&!shook){
                for(auto& [id,peer]:players_)peer.outbox->enqueue({skills::particles::aftershock_screen_shake_2022(target.actor)});
                shook=true;
            }
            if(ended())finished=true;
            return true;
        };
        for(unsigned index=0;index<plan.count;++index){
            const auto& strike=plan.strikes[index];if(distance>strike.radius)continue;
            if(!damage_event(strike.earth_damage))return false;
            if(finished)return true;
            if(plan.skill==174){
                const auto* player=current();if(!player||player->precise_health<=0)return true;
                if(strike.knocked_down_bonus>0&&player->knocked_down_until>now){
                    if(!damage_event(strike.knocked_down_bonus))return false;
                    if(finished)return true;
                }
                continue;
            }
            const auto kind=strike.condition==skills::effects::Condition::weakness?combat::conditions::Kind::weakness:
                strike.condition==skills::effects::Condition::cracked_armor?combat::conditions::Kind::cracked_armor:
                combat::conditions::Kind::blind;
            if(ended())return true;
            auto* player=current();auto conditions=player->conditions;
            if(!conditions.apply(kind,source.actor,now,std::uint64_t(strike.duration_seconds)*1000))return refused("condition");
            const auto old=player->conditions.lookup(kind),applied=conditions.lookup(kind);
            const bool needs_effect=!old||old->expires!=applied->expires||!player->condition_effects.contains(kind);
            if(needs_effect&&player->next_health_effect>UINT32_MAX)return refused("condition_capacity");
            commit_conditions_locked(*player,conditions,{kind},now);
            if(ended())return true;
        }
        if(!ended())publish_health_locked(*current());
        return true;
    }
