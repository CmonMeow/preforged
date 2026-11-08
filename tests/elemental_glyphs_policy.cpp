#include "../src/runtime/gw_elemental_glyphs.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills;
using namespace fw::gw::skills::effects;
void check(bool yes,const char* reason){if(!yes)throw std::runtime_error(reason);}
int main()try{
    const Scope owner{1,25,1};
    const auto power=*elemental_glyph(198),energy=*elemental_glyph(199,12),renewal=*elemental_glyph(203);
    check(power.charges==10&&power.elemental_bonus==2&&power.duration==25000,"Elemental Power canonical plan");
    check(energy.charges==3&&energy.elemental_bonus==2&&energy.energy_reduction==22&&!energy.duration&&energy.suppress_base_overcast,"Energy canonical plan");
    check(renewal.charges==1&&renewal.duration==15000&&renewal.instant_recharge,"Renewal canonical plan");
    for(unsigned rank=0;rank<=20;++rank){const auto plan=*elemental_glyph(199,rank);
        check(plan.energy_reduction==10+rank,"Energy discount progression");
        check(plan.charges==(rank<=3?1u:rank<=11?2u:rank<=18?3u:4u),"Energy charge breakpoints");
        check(plan.elemental_bonus==(rank<8?1u:2u),"Energy elemental rank breakpoints");
        check(!plan.duration,"Energy charges reinterpreted as duration");
    }
    check(!elemental_glyph(199,21)&&!elemental_glyph(200)&&!elemental_glyph(2803),"unsupported glyph/split/rank");
    auto altered=energy;++altered.charges;check(!prepare_elemental_glyph(owner,1,altered,1000),"modified glyph plan");
    auto active=*prepare_elemental_glyph(owner,100,power,1000);
    check(elemental_glyph_live(active,1000)&&elemental_glyph_live(active,25999)&&!elemental_glyph_live(active,26000),"Power duration boundary");
    check(!elemental_glyph_live(active,999),"glyph active before installation");
    check(!prepare_elemental_glyph(owner,100,power,UINT64_MAX-24999),"glyph duration overflow");
    auto indefinite=*prepare_elemental_glyph(owner,101,energy,1000);
    check(elemental_glyph_live(indefinite,UINT64_MAX),"Energy invented expiry");
    for(unsigned attribute=0;attribute<=51;++attribute)
        check(elemental_glyph_attribute_bonus(active,attribute,2000)==(attribute>=8&&attribute<=11?2u:0u),"elemental attributes or Energy Storage bonus wrong");
    check(elemental_glyph_attribute_bonus(active,8,26000)==0&&!elemental_glyph_attribute_bonus(active,52,2000),"expired/invalid attribute bonus");
    check(elemental_glyph_attribute_bonus(active,find_skill(232)->attribute,2000)==2,
        "nonspell Lightning Touch lost all-purpose rank bonus");
    check(!quote_elemental_glyph(active,{232,0},5,0,2000)&&active.remaining==10,"nonspell consumed or discounted glyph");
    for(unsigned id:{198u,199u,200u,201u,203u})check(replaces_elemental_glyph(id)&&!quote_elemental_glyph(active,{static_cast<std::uint16_t>(id),0},5,0,2000),"glyph self-trigger/replacement semantics");
    check(!replaces_elemental_glyph(194)&&!replaces_elemental_glyph(1),"nonglyph replaced glyph");
    const auto discount=*quote_elemental_glyph(indefinite,{185,9},25,5,2000);
    check(discount.energy_cost==3&&discount.base_overcast==0&&discount.elemental_bonus==2,"Energy cost/base Overcast/rank quote");
    check(quote_elemental_glyph(indefinite,{194,0},5,0,2000)->energy_cost==0,"glyph discount below-zero Energy");
    check(quote_elemental_glyph(active,{185,0},5,5,2000)->base_overcast==5,"Power incorrectly suppressed Overcast");
    check(!quote_elemental_glyph(active,{194,65536},5,0,2000),"invalid copied spell quote");
    check(elemental_glyph_trigger(energy,GlyphPhase::paid_admission).reserve&&
        !elemental_glyph_trigger(energy,GlyphPhase::paid_admission).consume&&
        elemental_glyph_trigger(power,GlyphPhase::paid_admission).reserve,"paid admission reservation/retained effect wrong");
    for(auto outcome:{GlyphOutcome::completed,GlyphOutcome::voluntary_cancel,GlyphOutcome::interrupted}){
        const auto done=elemental_glyph_trigger(energy,GlyphPhase::terminal,outcome);
        check(done.supported&&done.consume&&!done.instant_recharge,"Energy refunded admitted reservation on terminal outcome");
    }
    for(const auto& plan:{power,energy,renewal})check(!elemental_glyph_trigger(plan,GlyphPhase::paid_admission,GlyphOutcome::not_admitted).consume,"rejected spell consumed glyph");
    check(elemental_glyph_trigger(power,GlyphPhase::terminal,GlyphOutcome::completed).consume&&
        !elemental_glyph_trigger(power,GlyphPhase::terminal,GlyphOutcome::voluntary_cancel).consume,"Power completion/cancel trigger");
    check(!elemental_glyph_trigger(power,GlyphPhase::terminal,GlyphOutcome::interrupted).supported&&
        !elemental_glyph_trigger(power,GlyphPhase::terminal,GlyphOutcome::failed).supported,"unproved Power interrupted/failed consumption invented");
    check(elemental_glyph_trigger(renewal,GlyphPhase::terminal,GlyphOutcome::interrupted).instant_recharge&&
        !elemental_glyph_trigger(renewal,GlyphPhase::terminal,GlyphOutcome::voluntary_cancel).consume,"Renewal interrupt/cancel semantics");
    check(elemental_glyph_recharge(renewal,GlyphOutcome::completed,70000)==0&&
        elemental_glyph_recharge(renewal,GlyphOutcome::interrupted,70000)==0,"Renewal failed to replace normal/augmented recharge");
    check(elemental_glyph_recharge(power,GlyphOutcome::completed,70000)==70000&&
        !elemental_glyph_recharge(renewal,GlyphOutcome::failed,70000),"ordinary recharge or unknown failure handling wrong");
    const auto quote=*quote_elemental_glyph(active,{194,0},5,0,2000);
    const auto consumed=consume_elemental_glyph(active,quote);check(consumed.accepted&&consumed.remaining->remaining==9,"Power charge consumption");
    check(!consume_elemental_glyph(*consumed.remaining,quote).accepted,"stale glyph quote consumed twice");
    auto changed=active;++changed.owner.generation;check(!consume_elemental_glyph(changed,quote).accepted,"recycled owner inherited glyph quote");
    changed=active;++changed.effect;check(!consume_elemental_glyph(changed,quote).accepted,"replacement glyph inherited old cast charge");
    auto current=active;
    for(unsigned n=10;n>0;--n){const auto use=*quote_elemental_glyph(current,{194,0},5,0,2000+11-n);
        const auto update=consume_elemental_glyph(current,use);check(update.accepted,"live charge rejected");
        if(n==1)check(!update.remaining,"final charge did not end glyph");else{check(update.remaining->remaining==n-1,"charge count wrong");current=*update.remaining;}}
    const auto single_energy=*prepare_elemental_glyph(owner,102,*elemental_glyph(199,0),1000);
    const auto final_quote=*quote_elemental_glyph(single_energy,{185,0},5,5,2000);
    const auto lease=*reserve_elemental_glyph(single_energy,final_quote,42);
    check(single_energy.remaining==1&&elemental_glyph_attribute_bonus(single_energy,8,2500)==1,
        "last reserved Energy charge removed before terminal");
    for(auto outcome:{GlyphOutcome::completed,GlyphOutcome::voluntary_cancel,GlyphOutcome::interrupted}){
        const auto settled=settle_elemental_glyph(single_energy,lease,42,outcome,3000);
        check(settled.accepted&&settled.supported&&settled.consumed&&!settled.remaining&&!settled.instant_recharge,
            "last Energy charge terminal settlement");
        check(!settle_elemental_glyph(settled.remaining,lease,42,outcome,3000).accepted,"replayed final settlement");
    }
    for(const auto& plan:{power,energy,renewal})check(!elemental_glyph_trigger(plan,GlyphPhase::terminal,GlyphOutcome::failed).supported,
        "true failure charge accounting invented");
    check(!settle_elemental_glyph(single_energy,lease,42,GlyphOutcome::failed,3000).supported,
        "Energy failure silently classified");
    check(!reserve_elemental_glyph(single_energy,final_quote,0)&&
        !settle_elemental_glyph(single_energy,lease,43,GlyphOutcome::completed,3000).accepted,
        "missing/wrong cast event accepted");
    changed=single_energy;++changed.effect;
    check(!settle_elemental_glyph(changed,lease,42,GlyphOutcome::completed,3000).accepted,"replacement inherited lease");
    changed=single_energy;++changed.owner.generation;
    check(!settle_elemental_glyph(changed,lease,42,GlyphOutcome::completed,3000).accepted,"recycled owner inherited lease");
    const auto power_lease=*reserve_elemental_glyph(active,quote,43);
    const auto cancelled=settle_elemental_glyph(active,power_lease,43,GlyphOutcome::voluntary_cancel,3000);
    check(cancelled.accepted&&!cancelled.consumed&&cancelled.remaining==active,"Power cancel consumed reservation");
    check(!settle_elemental_glyph(active,power_lease,43,GlyphOutcome::completed,26000).supported,
        "expiry-crossing semantics invented");
    const auto renewal_state=*prepare_elemental_glyph(owner,103,renewal,1000);
    const auto renewal_quote=*quote_elemental_glyph(renewal_state,{194,0},5,0,2000);
    const auto renewal_lease=*reserve_elemental_glyph(renewal_state,renewal_quote,44);
    const auto interrupted=settle_elemental_glyph(renewal_state,renewal_lease,44,GlyphOutcome::interrupted,3000);
    check(interrupted.accepted&&interrupted.consumed&&interrupted.instant_recharge&&!interrupted.remaining,
        "Renewal interrupted terminal lost instant reset");
    check(!consume_elemental_glyph(single_energy,final_quote).remaining&&final_quote.elemental_bonus==1&&final_quote.base_overcast==0,
        "last Energy charge lost admitted spell benefits");
    Registry registry(owner);
    check(registry.apply(owner,{1,owner.actor,198,0,Category::glyph,1000,26000},Replace::category).accepted,"glyph registry fixture");
    check(!registry.newest(owner,Category::enchantment,2000),"glyph modeled as removable enchantment");
    const auto replacement=registry.apply(owner,{2,owner.actor,199,12,Category::glyph,2000,{}},Replace::category);
    check(replacement.accepted&&replacement.removed.size()==1&&replacement.removed[0].skill==198&&
        registry.newest(owner,Category::glyph,2000)->skill==199,"new glyph stacked instead of replacing");
    std::cout<<"PASS elemental glyph plans, charges, indefinite/timed effects, all-purpose ranks, Energy/Overcast, supported triggers and replacement\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
