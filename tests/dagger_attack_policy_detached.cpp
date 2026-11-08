#include "gw_dagger_attack_policy.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::combat::weapon;
using namespace fw::gw::skills::effects;
static void check(bool yes){if(!yes)throw std::runtime_error("detached dagger policy");}
int main()try{
    const Scope source{1,25,1},target{1,30,2},other{1,31,3};
    // Preserve original2022 metadata; actual attack contact is half listed
    // activation per oldwiki Attack_skill and explicit Jagged Strike note.
    check(fw::gw::skills::find_skill(782)->activation==.5f&&fw::gw::skills::find_skill(782)->recharge==1);
    check(fw::gw::skills::find_skill(780)->activation==.5f&&fw::gw::skills::find_skill(780)->recharge==3);
    for(unsigned rank=0;rank<=20;++rank){
        const auto lead=dagger_attack_policy(782,rank),fox=dagger_attack_policy(780,rank);
        check(lead&&fox&&lead->bonus==0&&lead->bleeding_ms==(5+rank)*1000);
        check(fox->bonus==interpolate(10,35,rank)&&fox->unblockable&&!lead->unblockable);
        check(lead->windup_ms==250&&fox->windup_ms==250);
        DaggerChain chain;
        check(dagger_attack_eligible(*lead,chain,source,target,32,false,100));
        check(!dagger_attack_eligible(*fox,chain,source,target,32,false,100));
        QualifiedHit hit{source,target,1,90,100,AttackDelivery::melee};
        const auto a=dagger_contact_proposal(*lead,chain,hit,32,false);
        check(a&&a->publish_stage&&a->after.current(target,100)==1&&chain.stage==0);
        chain=a->after;
        check(!dagger_attack_eligible(*fox,chain,source,other,32,false,100));
        check(!dagger_attack_eligible(*fox,chain,source,Scope{1,30,3},32,false,100));
        check(dagger_attack_eligible(*fox,chain,source,target,32,false,15099));
        check(!dagger_attack_eligible(*fox,chain,source,target,32,false,15100));
        hit.event=2;hit.at=200;
        const auto refresh=dagger_contact_proposal(*lead,chain,hit,32,false);
        check(refresh&&!refresh->publish_stage&&refresh->after.expires==15200);
        const auto b=dagger_contact_proposal(*fox,chain,hit,32,false);
        check(b&&b->publish_stage&&b->after.stage==2&&b->bonus==fox->bonus&&b->bleeding_ms==0);
        check(dagger_chain_matches(chain,b->before));
        check(!dagger_chain_matches(refresh->after,b->before));
        check(!dagger_contact_proposal(*fox,b->after,hit,32,false));
        for(unsigned stage=0;stage<=4;++stage)for(unsigned item=0;item<=40;++item)for(bool caster:{false,true}){
            DaggerChain candidate{target,stage,1000};
            check(dagger_attack_eligible(*lead,candidate,source,target,item,caster,100)==(item==32&&!caster));
            check(dagger_attack_eligible(*fox,candidate,source,target,item,caster,100)==(item==32&&!caster&&stage==1));
        }
        for(const auto& canonical_plan:{*lead,*fox}){
            for(unsigned field=0;field<8;++field){auto bad=canonical_plan;
                switch(field){case 0:bad.skill=775;break;case 1:bad.rank=21;break;
                case 2:++bad.required_stage;break;case 3:++bad.result_stage;break;
                case 4:++bad.bonus;break;case 5:++bad.bleeding_ms;break;
                case 6:++bad.windup_ms;break;case 7:bad.unblockable=!bad.unblockable;break;}
                check(!canonical(bad));check(!dagger_contact_proposal(bad,chain,hit,32,false));
            }
        }
    }
    const auto lead=*dagger_attack_policy(782,0);DaggerChain empty;
    check(!dagger_attack_policy(775,0)&&!dagger_attack_policy(782,21));
    auto forged=lead;forged.unblockable=true;check(!canonical(forged));
    QualifiedHit hit{source,target,1,10,20,AttackDelivery::melee};
    for(auto item:{0u,2u,5u,22u,27u,35u})check(!dagger_contact_proposal(lead,empty,hit,item,false));
    check(!dagger_contact_proposal(lead,empty,hit,32,true));
    hit.delivery=AttackDelivery::ranged;check(!dagger_contact_proposal(lead,empty,hit,32,false));
    hit.delivery=AttackDelivery::melee;hit.event=0;check(!dagger_contact_proposal(lead,empty,hit,32,false));
    hit.event=1;hit.released=21;check(!dagger_contact_proposal(lead,empty,hit,32,false));
    hit.released=10;hit.target.instance=2;check(!dagger_contact_proposal(lead,empty,hit,32,false));
    hit.target=source;check(!dagger_contact_proposal(lead,empty,hit,32,false));
    hit.target=target;hit.at=UINT64_MAX;check(!dagger_contact_proposal(lead,empty,hit,32,false));
    hit.at=20;hit.attacker.generation=0;check(!dagger_contact_proposal(lead,empty,hit,32,false));
    // Failed terminals are finished without qualification by the owner. The
    // proposal cannot accept an AttackOutcome and must not fabricate a hit.
    for(auto failed:{AttackOutcome::miss,AttackOutcome::block,AttackOutcome::dodge,
            AttackOutcome::obstruction,AttackOutcome::cancelled,AttackOutcome::target_retired,AttackOutcome::replaced}){
        AttackOutcomes ledger(source);check(ledger.release(source,target,1,10,AttackDelivery::melee));
        const auto done=ledger.finish(source,target,1,failed,20);check(done&&done->outcome==failed);
        check(!ledger.qualify_hit(source,target,1,20));
        check(!ledger.finish(source,target,1,failed,20));
        check(empty.stage==0&&empty.expires==0);
    }
    AttackOutcomes ledger(source);check(ledger.release(source,target,1,10,AttackDelivery::melee));
    const auto qualified=ledger.qualify_hit(source,target,1,20);check(bool(qualified));
    check(!ledger.qualify_hit(source,target,1,20));
    const auto proposed=dagger_contact_proposal(lead,empty,*qualified,32,false);check(bool(proposed));
    // Copy-only helpers are deliberately not a replay authority: owner must
    // commit once, and an exact snapshot check must protect later mutations.
    check(bool(dagger_contact_proposal(lead,empty,*qualified,32,false)));
    auto changed=empty;changed.lead(other,20);check(!dagger_chain_matches(changed,proposed->before));
    check(!ledger.finish(source,target,1,AttackOutcome::block,20));
    check(bool(ledger.finish(source,target,1,AttackOutcome::cancelled,20)));
    for(auto invalid:{Scope{0,25,1},Scope{1,0,1},Scope{1,65536,1},Scope{1,25,0}})
        check(!dagger_attack_eligible(lead,empty,invalid,target,32,false,20));
    for(auto invalid:{Scope{0,30,1},Scope{1,0,1},Scope{1,65536,1},Scope{1,30,0},Scope{2,30,1}})
        check(!dagger_attack_eligible(lead,empty,source,invalid,32,false,20));
    std::cout<<"PASS detached Jagged782/Fox780 policy; paid World and native consumer integration pending\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
