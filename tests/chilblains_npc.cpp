#include "gw_chilblains_npc.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Chilblains NPC line "+std::to_string(at.line()));
}
combat::npcs::LiveActor actor(unsigned id=100,std::optional<int> armor=60,double hp=1000){
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
    d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{hp,1000};
    d.cold_armor=armor;d.damageable=d.conditions_allowed=true;
    return {d,{1,1,1,146,id}};
}
int main()try{
    const Scope source{1,1,1};combat::conditions::State conditions;
    using RemovalResult=combat::npcs::LiveActor::EnchantmentRemovalResult;
    // Exact application leases survive shared damage without restoring Health.
    {
        auto n=actor();const auto life=n.lifetime();
        check(n.stage_shield_absorption(life,12,1,77,100));
        const auto removal=n.prepare_enchantment_removal(life,77,100);check(bool(removal));
        const auto hit=n.apply_damage(life,{combat::Kind::cold,50,20,144},100);
        check(hit.reason==combat::Reason::accepted&&hit.damage==45);
        if(n.health_clock(life))check(bool(n.advance_conditions(life,100)));
        check(n.commit_enchantment_removal(*removal,100)==RemovalResult::removed);
        check(n.actor(life)->health->current==955);
        check(n.apply_damage(life,{combat::Kind::cold,50,20,144},101).damage==50);
        if(n.health_clock(life))check(bool(n.advance_conditions(life,101)));
        check(n.commit_enchantment_removal(*removal,101)==RemovalResult::already_absent);
        check(n.stage_shield_absorption(life,12,1,77,102));
        const auto lease=n.prepare_enchantment_removal(life,77,102);check(bool(lease));
        // Identical fields and wire ID still represent a distinct application.
        check(n.stage_shield_absorption(life,12,1,77,102));
        if(n.health_clock(life))check(bool(n.advance_conditions(life,102)));
        check(n.commit_enchantment_removal(*lease,102)==RemovalResult::replaced);
        check(n.apply_damage(life,{combat::Kind::cold,50,20,144},102).damage==45);
        auto stale=*lease;++stale.expected.actor_generation;
        check(!n.commit_enchantment_removal(stale,102));
        check(!n.commit_enchantment_removal(*lease,99));
        check(!n.prepare_enchantment_removal(life,77,99));
    }
    {
        auto n=actor();const auto life=n.lifetime();
        check(n.apply_watchful_healing(life,*watchful_healing(12),1,78,100));
        check(n.stage_shield_absorption(life,12,1,77,100));
        const auto removal=n.prepare_enchantment_removal(life,78,100);check(bool(removal));
        check(n.apply_damage(life,{combat::Kind::cold,100,20,144},100).damage==95);
        const auto health=n.actor(life)->health->current;
        if(n.health_clock(life))check(bool(n.advance_conditions(life,100)));
        check(n.commit_enchantment_removal(*removal,100)==RemovalResult::removed);
        check(n.actor(life)->health->current>health);
        // Removing another enchantment cannot reset1399's damage counter.
        check(n.apply_damage(life,{combat::Kind::cold,50,20,144},101).damage==40);
        const auto ends=n.take_enchantment_ends(life);check(ends.size()==1&&ends[0].entry.skill==1392);
    }
    {
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=100;
        d.relation=combat::npcs::Relation::hostile_to_players;
        d.spawn_health=combat::Health{500,1000};d.cold_armor=60;d.damageable=true;
        d.healing_allowed=true;d.healing_evidence="authored conversion fixture";
        combat::npcs::LiveActor n(d,{1,1,1,146,100});const auto life=n.lifetime();
        check(n.apply_reversal_fortune(life,12,1,78,100));
        const auto removal=n.prepare_enchantment_removal(life,78,100);check(bool(removal));
        check(bool(n.apply_reversal_damage(life,{combat::Kind::cold,50,20,144},100,true)));
        const auto health=n.actor(life)->health->current;
        check(n.stage_shield_absorption(life,12,1,79,100));
        check(n.commit_enchantment_removal(*removal,100)==RemovalResult::already_absent);
        check(n.newest_enchantment(life,100)->instance==79&&n.actor(life)->health->current==health);
    }
    {
        auto n=actor();const auto life=n.lifetime();const auto plan=*watchful_healing(12);
        check(n.apply_watchful_healing(life,plan,1,78,100));
        const auto removal=n.prepare_enchantment_removal(life,78,100);check(bool(removal));
        const auto deadline=100+plan.duration;
        // A stale timeline cannot mutate/expire the effect inside commit.
        check(!n.commit_enchantment_removal(*removal,deadline));
        check(n.take_enchantment_ends(life).empty());
        check(bool(n.advance_conditions(life,deadline)));
        n.refresh_hexes(life,deadline);
        check(n.take_enchantment_ends(life).empty());
        check(n.commit_enchantment_removal(*removal,deadline)==RemovalResult::already_absent);
        check(n.take_enchantment_ends(life).empty());
    }
    for(auto order:{ChilblainsOrder::damage_then_remove,ChilblainsOrder::remove_then_damage}){
        for(unsigned rank=0;rank<=20;++rank){
            auto n=actor();const auto life=n.lifetime();
            check(n.observe_overcast(life,2,1000,"authored test"));
            check(n.apply_ice_spear_regeneration(life,*water_projectile(214,12),77,1000));
            std::vector<ChilblainsNpcRecipient> input{{n,life,322}};
            const auto p=prepare_chilblains_npcs(*chilblains(rank),source,20,conditions,input,order,1000);
            check(p&&p->recipients.size()==1&&p->recipients[0].removed.size()==1);
            check(p->recipients[0].removed[0].instance==77&&!p->recipients[0].actor.newest_enchantment(life,1000));
            check(std::abs(p->recipients[0].hit.damage-chilblains(rank)->damage)<1e-9);
            check(p->recipients[0].hit.kind==combat::Kind::cold&&p->recipients[0].hit.skill==144);
            check(p->recipients[0].visual==combat::Packet({159,0,20,0,0,0,100,0,0,0,1,0,0,0,1,1,0,0}));
            check(n.newest_enchantment(life,1000)&&n.actor(life)->health->current==1000);
            check(!conditions.has(combat::conditions::Kind::poison,1000));
        }
        auto n=actor();const auto life=n.lifetime();check(n.apply_condition(life,combat::conditions::Kind::blind,1,1000,10000));
        std::vector<ChilblainsNpcRecipient> inputs{{n,life,322.001}};
        check(prepare_chilblains_npcs(*chilblains(12),source,20,conditions,inputs,order,1000)->recipients.empty());
        inputs[0].distance=0;inputs.push_back(inputs[0]);
        check(!prepare_chilblains_npcs(*chilblains(12),source,20,conditions,inputs,order,1000));
        inputs.erase(inputs.begin()+1,inputs.end());++inputs[0].expected.actor_generation;
        check(!prepare_chilblains_npcs(*chilblains(12),source,20,conditions,inputs,order,1000));
        inputs[0].expected=life;inputs[0].distance=std::numeric_limits<double>::quiet_NaN();
        check(!prepare_chilblains_npcs(*chilblains(12),source,20,conditions,inputs,order,1000));
        inputs[0].distance=0;
        check(!prepare_chilblains_npcs(*chilblains(12),source,20,conditions,inputs,order,1001));
        auto unknown=actor(101,{});unknown.advance_conditions(unknown.lifetime(),1000);
        inputs.push_back({unknown,unknown.lifetime(),0});
        check(!prepare_chilblains_npcs(*chilblains(12),source,20,conditions,inputs,order,1000));
        check(inputs[0].actor.actor(life)->health->current==1000);
        auto armored=actor(102,100);armored.advance_conditions(armored.lifetime(),1000);
        inputs={{armored,armored.lifetime(),0}};
        const auto half=prepare_chilblains_npcs(*chilblains(12),source,20,conditions,inputs,order,1000);
        check(half&&std::abs(half->recipients[0].hit.damage-chilblains(12)->damage/2)<1e-9);
        auto lethal=actor(103,60,10);
        inputs={{n,life,0},{lethal,lethal.lifetime(),322}};
        const auto batch=prepare_chilblains_npcs(*chilblains(12),source,20,conditions,inputs,order,1000);
        check(batch&&batch->recipients.size()==2&&batch->recipients[1].hit.killed);
        check(batch->recipients[1].actor.actor(lethal.lifetime())->health->current==0);
        check(lethal.actor(lethal.lifetime())->health->current==10);
        auto foreign=source;++foreign.instance;
        check(!prepare_chilblains_npcs(*chilblains(12),foreign,20,conditions,inputs,order,1000));
        check(!prepare_chilblains_npcs(*chilblains(12),source,0,conditions,inputs,order,1000));
        inputs[0].distance=-1;
        check(!prepare_chilblains_npcs(*chilblains(12),source,20,conditions,inputs,order,1000));
        // This skill has no reviewed target cap. Do not inherit a registry's
        // capacity limit as an invented maximum number of area recipients.
        inputs.clear();
        for(unsigned id=200;id<330;++id){auto next=actor(id);inputs.push_back({next,next.lifetime(),0});}
        const auto crowd=prepare_chilblains_npcs(*chilblains(12),source,20,conditions,inputs,order,1000);
        check(crowd&&crowd->recipients.size()==130);
    }
    std::cout<<"PASS staged Chilblains NPC batch:21ranks,both explicit ordering candidates,cold armor,reviewed stripping,scope/range/clock guards and atomic rejection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}



