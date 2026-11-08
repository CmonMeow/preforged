#include "gw_world_simulation.h"
#include "gw_mimicry_commit.h"
#include "gw_temporary_skill_expiry.h"
#include <iostream>
#include <source_location>
#include <type_traits>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Mimicry World line "+std::to_string(at.line()));
}
void join(world::Instance& world,unsigned id,float x,unsigned team,std::uint64_t at=100){
    world.join(id,"Actor",x,0,std::make_shared<world::Outbox>(),0,{},0,at,team);
    world.set_vitals(id,{1000,1000,100,100},100);
}
int main()try{
    static_assert(!std::is_default_constructible_v<skills::effects::MimicryDonorLease>);
    for(unsigned mode=0;mode<21;++mode){
        world::Instance world;join(world,1,0,1);join(world,2,mode==5?2000:600,mode==4?2:1);
        const auto owner=*world.effect_scope(1);auto target=*world.effect_scope(2);
        skills::Slots caster{},donor{};caster[0]={65,0};donor[0]={5,17};
        if(mode==1)donor[0]={194,0};
        if(mode==2){donor[0]={1518,0};donor[1]={5,17};}
        if(mode==8)caster={};
        check(world.configure_adrenaline(owner,caster,100));
        if(mode!=13)check(world.configure_adrenaline(target,donor,100));
        if(mode==3)target=owner;
        if(mode==6)++target.generation;
        if(mode==9)check(world.kill(target,150));
        if(mode==10)check(world.kill(owner,150));
        if(mode==11)check(world.settle_resources(owner,300));
        if(mode==12)check(world.settle_resources(target,300));
        const auto lease=world.prepare_mimicry_donor(owner,target,{65,mode==7?8u:0u},200);
        if(mode>=3&&mode<=13){check(!lease);continue;}
        check(bool(lease));check(world.mimicry_donor_current(*lease,200));
        check(!world.mimicry_donor_current(*lease,201));
        check(lease->owner()==owner&&lease->target()==target&&lease->source()==skills::Slot{65,0});
        if(mode==1||mode==2){
            check(lease->donor().status==(mode==1?skills::effects::MimicrySelection::no_elite:skills::effects::MimicrySelection::form));
            skills::effects::TemporarySkillSession session(owner,caster);skills::effects::TemporarySkillIds ids(owner);
            check(!skills::effects::commit_mimicry_copy(world,session,ids,*lease));
            check(session.bar()==caster&&world.cooldown_bar_snapshot(owner)->bar==caster);continue;
        }
        check(lease->donor().slot==donor[0]&&lease->donor().index==0);
        if(mode==14||mode==15){
            auto changed=mode==14?donor:caster;changed[0]={194,0};
            const auto scope=mode==14?target:owner;
            check(world.configure_adrenaline(scope,changed,200));check(!world.mimicry_donor_current(*lease,200));
            check(world.configure_adrenaline(scope,mode==14?donor:caster,200));
            check(!world.mimicry_donor_current(*lease,200)); // ABA preserves values,not revision.
        }
        if(mode==16){
            check(world.configure_adrenaline(target,donor,200));
            check(world.mimicry_donor_current(*lease,200)); // Unchanged publication is not a bar replacement.
        }
        if(mode==17){
            world.leave(2);join(world,2,600,1,200);const auto replacement=*world.effect_scope(2);
            check(replacement!=target&&world.configure_adrenaline(replacement,donor,200));
            check(!world.mimicry_donor_current(*lease,200));
        }
        if(mode==18){check(world.kill(target,200));check(!world.mimicry_donor_current(*lease,200));}
        if(mode==19||mode==20){const auto who=mode==19?target:owner;
            check(world.kill(who,200)&&world.resurrect(who,200));
            check(!world.mimicry_donor_current(*lease,200));
        }
    }
    for(bool stale:{false,true}){
        world::Instance world;join(world,1,0,1);join(world,2,600,1);
        const auto owner=*world.effect_scope(1),target=*world.effect_scope(2);
        skills::Slots caster{},donor{};caster[0]={65,0};donor[0]={5,17};
        check(world.configure_adrenaline(owner,caster,100)&&world.configure_adrenaline(target,donor,100));
        skills::effects::TemporarySkillSession session(owner,caster);skills::effects::TemporarySkillIds ids(owner);
        auto lease=world.prepare_mimicry_donor(owner,target,{65,0},200);check(bool(lease));
        if(stale){
            auto changed=donor;changed[0]={74,0};check(world.configure_adrenaline(target,changed,200));
            check(!skills::effects::commit_mimicry_copy(world,session,ids,*lease));
            check(session.bar()==caster&&world.cooldown_bar_snapshot(owner)->bar==caster);
            check(world.configure_adrenaline(target,donor,200));lease=world.prepare_mimicry_donor(owner,target,{65,0},200);check(bool(lease));
        }
        auto unrelated=caster;unrelated[0]={5,1};unrelated[1]={194,0};
        check(!world.commit_mimicry_copy(*lease,{caster,unrelated},{5,1}));
        auto candidate=caster;candidate[0]={5,0};check(!world.commit_mimicry_copy(*lease,{caster,candidate},{5,0}));
        check(session.bar()==caster&&world.mimicry_donor_current(*lease,200));
        const auto copied=skills::effects::commit_mimicry_copy(world,session,ids,*lease);check(bool(copied));
        check(copied->change.after==skills::Slot{5,1}); // A failed prepare did not burn recipient copy IDs.
        check(copied->packets==std::vector<skills::Packet>{skills::slot_packet_2022(1,0,{5,1})});
        check(session.bar()==world.cooldown_bar_snapshot(owner)->bar);
        check(!skills::effects::commit_mimicry_copy(world,session,ids,*lease));
        skills::Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[65]=1;
        const auto grant=session.admit(owner,policy,0,{5,1},201);check(grant&&grant->cast.copied);
        auto forged=*grant;forged.cast.copied->require_campaign=false;check(!session.valid(forged,policy,201));
        auto restricted=policy;restricted.campaigns=0;
        check(!session.admit(owner,restricted,0,{5,1},201)&&!session.valid(*grant,restricted,201));
        world.leave(2);check(session.valid(*grant,policy,201)); // A committed copy is independent of the donor.
        const auto early=skills::effects::expire_temporary_copy(world,session,owner,20199);
        check(early&&early->changes.empty()&&session.bar()[0]==skills::Slot{5,1});
        check(session.valid(*grant,policy,20199));
        check(!session.admit(owner,policy,0,{5,1},20200));
        const auto expired=skills::effects::expire_temporary_copy(world,session,owner,20200);
        check(expired&&expired->changes.size()==1&&session.bar()==caster);
        check(!session.valid(*grant,policy,20200));
        check(world.skill_disabled_slot(owner,{65,0},20200)==80200);
        const auto duplicate=skills::effects::expire_temporary_copy(world,session,owner,20201);
        check(duplicate&&duplicate->changes.empty()&&world.skill_disabled_slot(owner,{65,0},20201)==80200);
        check(world.skill_disabled_slot(owner,{65,0},80200)==0);
        check(!policy.learned.contains(5));
    }
    {
        world::Instance world;join(world,1,0,1);join(world,2,600,1);
        const auto owner=*world.effect_scope(1),target=*world.effect_scope(2);
        skills::Slots caster{},donor{};caster[0]={65,0};donor[0]={3431,0};
        check(skills::find_skill(3431)->elite()&&(skills::find_skill(3431)->special&0x80000u));
        check(world.configure_adrenaline(owner,caster,100)&&world.configure_adrenaline(target,donor,100));
        skills::effects::TemporarySkillSession session(owner,caster);skills::effects::TemporarySkillIds ids(owner);
        const auto lease=world.prepare_mimicry_donor(owner,target,{65,0},200);check(bool(lease));
        check(bool(skills::effects::commit_mimicry_copy(world,session,ids,*lease)));
        skills::Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[65]=1;
        check(bool(session.admit(owner,policy,0,{3431,1},201)));
        policy.pvp_match=true;check(!session.admit(owner,policy,0,{3431,1},201));
    }
    std::cout<<"Mimicry donor leases,atomic temporary copies and20s/60s restoration passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
