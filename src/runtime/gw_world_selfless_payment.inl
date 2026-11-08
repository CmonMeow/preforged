// Included in Instance public scope. Trusted Runtime passes the semantic target
// after its skill-specific target normalization, never the UI selection.
private:
    std::optional<unsigned> selfless_payment_cost_locked(Player& caster,unsigned skill,
            std::optional<skills::effects::Scope> target,unsigned cost,std::uint64_t now){
        using namespace skills::effects;
        const auto* row=skills::find_skill(skill);
        if(!row||!caster.effect_registry||!caster.resources||caster.precise_health<=0)return {};
        SelflessTarget semantic=SelflessTarget::untargeted;
        if(target){
            if(*target==caster.effect_scope)semantic=SelflessTarget::self;
            else if(const auto found=players_.find(target->actor);found!=players_.end()){
                const auto& ally=found->second;
                if(ally.effect_scope!=*target||!ally.resources)return {};
                // Mirror player ally policy: a known opposing team denies,
                // while a matching team or authoritative party can permit.
                if(caster.team_token&&ally.team_token&&caster.team_token!=ally.team_token)
                    semantic=SelflessTarget::foe;
                else{
                    const bool same_team=caster.team_token&&ally.team_token&&caster.team_token==ally.team_token;
                    const auto caster_party=caster.id<=65535
                        ?parties_.group(static_cast<std::uint16_t>(caster.id)):0;
                    const bool same_party=caster_party&&ally.id<=65535&&
                        caster_party==parties_.group(static_cast<std::uint16_t>(ally.id));
                    semantic=same_team||same_party?SelflessTarget::other_ally:SelflessTarget::foe;
                }
            }else{
                const auto npc_found=npc_combat_.find(target->actor);
                if(npc_found==npc_combat_.end()||npc_found->second.hex_scope()!=*target)return {};
                semantic=npc_found->second.healable_ally(npc_found->second.lifetime())
                    ?SelflessTarget::other_ally:SelflessTarget::foe;
            }
        }
        const auto live=live_selfless_spirit_entry(*caster.effect_registry,caster.effect_scope,now);
        if(!live)return {};
        if(!*live)return cost;
        const auto plan=selfless_spirit((**live).skill,(**live).rank);if(!plan)return {};
        const auto quote=quote_selfless_spirit(*plan,true,is_spell(*row),semantic,cost);
        return quote?std::optional<unsigned>{quote->after}:std::nullopt;
    }
public:
    bool consume_selfless_energy(skills::effects::Scope owner,skills::Slot slot,
            std::optional<skills::effects::Scope> resolved_target,unsigned modified_cost,std::uint64_t now){
        std::lock_guard lock(mutex_);const auto found=players_.find(owner.actor);
        if(found==players_.end()||found->second.effect_scope!=owner)return false;
        auto& caster=found->second;
        if(!advance_resource_event_locked(caster,now)||caster.knocked_down_until>now)return false;
        const auto priced=selfless_payment_cost_locked(caster,slot.skill,resolved_target,modified_cost,now);
        if(!priced)return false;unsigned cost=*priced;const std::optional<std::uint64_t> at=now;
        if(!caster.resources||!caster.resources->health||caster.resources->energy<cost)return false;
        if(!cost)return true;
        auto& resources=*caster.resources;
        if(at&&resources.energy==cost&&mind_wrack_energy_locked(caster).fractional_units==0){
            const auto found=mind_wrack_credit_.find(caster.id);
            if(found!=mind_wrack_credit_.end()){
                const auto credit=found->second;const auto hex_caster=players_.find(credit.caster.actor);
                if(credit.target==caster.effect_scope&&hex_caster!=players_.end()&&hex_caster->second.effect_scope==credit.caster){
                    if(*at<hex_caster->second.health_updated||next_mind_wrack_event_==UINT64_MAX||!advance_health_locked(hex_caster->second,*at))return false;
                    // Caster death cleanup may have ended this hex. Resource
                    // returns from settling that lifetime precede this payment.
                    const auto repriced=selfless_payment_cost_locked(caster,slot.skill,resolved_target,modified_cost,now);
                    if(!repriced)return false;cost=*repriced;
                    if(!resources.health||resources.energy<cost)return false;
                }
            }
        }
        const auto before=mind_wrack_energy_locked(caster);
        if(caster.overcast_curve){
            auto payment=overcast::prepare_loss(*caster.overcast_curve,caster.effect_scope,resources,caster.energy_regeneration,caster.energy_debt,cost,caster.energy_updated,0,overcast::LossRule::full_cost,caster.energy_fractional_tail);
            if(!payment)return false;commit_overcast_energy_locked(caster,std::move(payment->state));
        }else resources.energy-=cost;
        caster.outbox->enqueue(owner_vitals(caster));
        if(at)observe_mind_wrack_energy_locked(caster,before,*at);
        return true;
    }
    bool activate_selfless_overcast_energy(skills::effects::Scope owner,unsigned skill,std::optional<skills::effects::Scope> resolved_target,unsigned ordinary_cost,std::uint64_t event,std::uint64_t now){
        const auto* row=skills::find_skill(skill);if(!row||!(row->special&1)||!row->overcast)return false;
        std::lock_guard lock(mutex_);const auto found=players_.find(owner.actor);
        if(found==players_.end()||found->second.effect_scope!=owner)return false;auto& player=found->second;
        if(!advance_resource_event_locked(player,now)||!player.resources->health||player.knocked_down_until>now)return false;
        const auto unpriced_cost=ordinary_cost;
        const auto priced=selfless_payment_cost_locked(player,skill,resolved_target,ordinary_cost,now);if(!priced)return false;ordinary_cost=*priced;
        const auto original=player.overcast_curve.value_or(overcast::CeilingCurve(owner,now,33));
        auto proposal=overcast::prepare_activation(original,owner,*player.resources,player.energy_regeneration,player.energy_debt,ordinary_cost,row->overcast,event,now,0,player.energy_fractional_tail);
        if(!proposal)return false;
        const auto before=mind_wrack_energy_locked(player);
        // Exact-zero caster settlement can change resources through owned ends.
        if((std::int64_t(before.current)-before.debt)*overcast::energy_unit+before.fractional_units>0&&overcast::precise_current(proposal->state)==0){
            if(!settle_mind_wrack_caster_locked(owner,now))return false;
            const auto repriced=selfless_payment_cost_locked(player,skill,resolved_target,unpriced_cost,now);if(!repriced)return false;ordinary_cost=*repriced;
            proposal=overcast::prepare_activation(player.overcast_curve.value_or(overcast::CeilingCurve(owner,now,33)),owner,*player.resources,player.energy_regeneration,player.energy_debt,ordinary_cost,row->overcast,event,now,0,player.energy_fractional_tail);
            if(!proposal)return false;
        }
        const auto observed=mind_wrack_energy_locked(player);commit_overcast_energy_locked(player,std::move(proposal->state));
        player.outbox->enqueue(owner_vitals(player));observe_mind_wrack_energy_locked(player,observed,now);return true;
    }
    std::optional<skills::effects::ElementalGlyphQuote> pay_selfless_elemental_glyph_use(skills::effects::Scope owner,skills::Slot slot,std::optional<skills::effects::Scope> resolved_target,
            unsigned modified_cost,std::uint64_t event,std::uint64_t now,const std::optional<skills::effects::ElementalGlyphState>& expected={}){
        const auto* row=skills::find_skill(slot.skill);if(!row||!skills::effects::is_spell(*row))return {};
        std::lock_guard lock(mutex_);const auto found=players_.find(owner.actor);
        if(found==players_.end()||found->second.effect_scope!=owner)return {};auto& p=found->second;
        const auto eligible=[&]{return p.precise_health>0&&p.resources&&p.effect_registry&&p.elemental_glyph&&
            !p.elemental_glyph_lease&&event>p.last_elemental_glyph_use&&(!expected||*expected==*p.elemental_glyph)&&
            skills::effects::registered_elemental_glyph(*p.effect_registry,*p.elemental_glyph);};
        if(!eligible()||!advance_resource_event_locked(p,now)||!eligible()||p.knocked_down_until>now)return {};
        const auto unpriced_cost=modified_cost;
        const auto priced=selfless_payment_cost_locked(p,slot.skill,resolved_target,modified_cost,now);if(!priced)return {};modified_cost=*priced;
        const unsigned base_overcast=(row->special&1)?row->overcast:0;
        auto quote=skills::effects::quote_elemental_glyph(*p.elemental_glyph,slot,modified_cost,base_overcast,now);if(!quote)return {};
        auto lease=skills::effects::reserve_elemental_glyph(*p.elemental_glyph,*quote,event);if(!lease)return {};
        const auto prepare=[&]()->std::optional<overcast::Settlement>{
            const auto curve=p.overcast_curve.value_or(overcast::CeilingCurve(owner,now,33));
            if(quote->base_overcast){
                const auto proposal=overcast::prepare_activation(curve,owner,*p.resources,p.energy_regeneration,p.energy_debt,
                    quote->energy_cost,quote->base_overcast,event,now,0,p.energy_fractional_tail);
                if(proposal)return proposal->state;
            }else{
                const auto proposal=overcast::prepare_loss(curve,owner,*p.resources,p.energy_regeneration,p.energy_debt,
                    quote->energy_cost,now,0,overcast::LossRule::full_cost,p.energy_fractional_tail);
                if(proposal)return proposal->state;
            }
            return {};
        };
        auto payment=prepare();if(!payment)return {};
        const auto before=mind_wrack_energy_locked(p);
        if((std::int64_t(before.current)-before.debt)*overcast::energy_unit+before.fractional_units>0&&overcast::precise_current(*payment)==0){
            if(!settle_mind_wrack_caster_locked(owner,now)||!eligible()||*p.elemental_glyph!=quote->glyph)return {};
            const auto repriced=selfless_payment_cost_locked(p,slot.skill,resolved_target,unpriced_cost,now);if(!repriced)return {};
            quote=skills::effects::quote_elemental_glyph(*p.elemental_glyph,slot,*repriced,base_overcast,now);if(!quote)return {};
            lease=skills::effects::reserve_elemental_glyph(*p.elemental_glyph,*quote,event);if(!lease)return {};
            payment=prepare();if(!payment)return {};
        }
        const auto observed=mind_wrack_energy_locked(p);commit_overcast_energy_locked(p,std::move(*payment));
        p.last_elemental_glyph_use=event;p.elemental_glyph_lease=*lease;
        p.outbox->enqueue(owner_vitals(p));observe_mind_wrack_energy_locked(p,observed,now);
        // Reactive death clears owned effects/lease; payment is never refunded.
        if(p.precise_health<=0){p.elemental_glyph_lease.reset();return {};}
        return quote;
    }
