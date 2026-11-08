#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static unsigned checks=0;
static void check(bool value,std::source_location at=std::source_location::current()){
    ++checks;if(!value)throw std::runtime_error("NPC Retribution maintainer line "+std::to_string(at.line()));
}
static world::Instance::Npc make_npc(unsigned id,bool hostile,unsigned energy=100,unsigned pips=4,double health=500){
    world::Instance::Npc n;n.id=n.definition.id=id;n.x=float(60+(id-100)*60);n.definition.level=20;n.definition.primary=3;n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
    d.relation=hostile?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
    d.packed_allegiance=hostile?3u:6u;
    d.spawn_health=combat::Health{health,1000};d.spawn_energy=combat::npcs::Energy{energy,100};d.energy_pips=pips;
    d.physical_armor=60;d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic NPC maintainer";
    d.holy_traits={false,false};d.light_sensitive_evidence=d.summoned_evidence="synthetic Holy traits";n.combat_definition=d;return n;
}
static void join(world::Instance& w,std::shared_ptr<world::Outbox> box){w.join(1,"Observer",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);}
int main()try{
    {auto w=std::make_unique<world::Instance>();w->configure_npcs({make_npc(100,false),make_npc(101,true)});
        auto box=std::make_shared<world::Outbox>();join(*w,box);const auto source=*w->effect_scope(100),target=*w->effect_scope(1);
        const combat::ai::ordinary::Reviewed weapon{20,20,12,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1200,32},"synthetic current standing ordinary controller"};
        check(w->configure_npc_opposed_ordinary(source,weapon,100));
        auto stale_source=source,stale_target=target;++stale_source.generation;++stale_target.generation;
        check(!w->stage_retribution(stale_source,target,*retribution(12),100));
        check(!w->stage_retribution(source,stale_target,*retribution(12),100));
        check(!w->stage_retribution(target,stale_source,*retribution(12),100));
        check(w->maintained_enchantment_count(source,100)==0&&w->status_snapshot(target,100)->empty());
        // A current ordinary controller with no retained route is proven
        // standing. Both legal allegiance directions must now succeed.
        check(w->stage_retribution(source,target,*retribution(12),100));
        const auto player_entry=w->status_snapshot(target,100)->front();check(w->maintained_enchantment_count(source,100)==1);
        check(w->cancel_maintained_enchantment(source,player_entry.instance,100));
        check(w->stage_retribution(target,source,*retribution(12),100));
        const auto npc_entry=w->status_snapshot(source,100)->front();check(w->maintained_enchantment_count(target,100)==1);
        check(w->cancel_maintained_enchantment(target,npc_entry.instance,100));
        // Future controller observation cannot prove an older source pose;
        // preserve genuine negative coverage instead of an obsolete blanket.
        const auto future=*w->effect_scope(101);combat::ai::FlarePolicy policy;policy.source=future;policy.target=target;policy.level=20;
        policy.speed=100;policy.acquire=1000;policy.follow=1500;policy.leash=2000;policy.flight={1000,32};policy.evidence="synthetic future controller rejection";
        check(w->configure_npc_flare_ai(policy,200));
        check(!w->maintained_range_path(future,100,100));check(!w->stage_retribution(future,future,*retribution(0),100));
        check(w->maintained_enchantment_count(future,100)==0);
    }
    // The same authority supports allied NPC -> player and NPC -> same-side
    // NPC. There is no observer-private maintained bar for an NPC source.
    for(bool player:{false,true}){
        auto w=std::make_unique<world::Instance>();w->configure_npcs({make_npc(100,false),make_npc(101,false)});auto box=std::make_shared<world::Outbox>();join(*w,box);
        const auto source=*w->effect_scope(100),target=*w->effect_scope(player?1:101);box->drain();
        check(w->maintained_enchantment_count(source,100)==0);
        check(w->stage_retribution(source,target,*retribution(12),100));const auto status=w->status_snapshot(target,100);check(status&&status->size()==1);
        check(w->maintained_enchantment_count(source,100)==1);auto stale=source;++stale.generation;
        check(!w->maintained_enchantment_count(stale,100).has_value());
        const auto wire=*mending_wire(source,target,status->front());const auto applied=box->drain();
        check(std::count(applied.begin(),applied.end(),wire.effect)==1&&std::count(applied.begin(),applied.end(),wire.upkeep)==0);
        if(!player){w->join(2,"Late observer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1,false);
            const auto entry=w->npc_entry_packets(2,100);
            check(std::count(entry.begin(),entry.end(),wire.effect)==1&&std::count(entry.begin(),entry.end(),wire.upkeep)==0);}
        w->tick(1100);check(w->maintained_enchantment_count(source,1100)==1&&w->npc_energy_snapshot(source)->current==100);
        check(w->cancel_maintained_enchantment(source,status->front().instance,1100));check(w->maintained_enchantment_count(source,1100)==0);
        const auto ended=*mending_end_wire(source,target,status->front());const auto removed=box->drain();
        check(std::count(removed.begin(),removed.end(),ended.effect)==1&&std::count(removed.begin(),removed.end(),ended.upkeep)==0);
        if(!player){const auto entry=w->npc_packets();check(std::count(entry.begin(),entry.end(),wire.effect)==0);}
    }
    // Exact first zero under dense/sparse observation. The self effect is the
    // newest fifth link, so its removal precedes its own pending zero observer.
    for(bool dense:{false,true}){
        auto w=std::make_unique<world::Instance>();std::vector<world::Instance::Npc> actors{make_npc(100,true,1,4)};
        for(unsigned id=101;id<=104;++id)actors.push_back(make_npc(id,true));w->configure_npcs(actors);auto box=std::make_shared<world::Outbox>();join(*w,box);
        const auto source=*w->effect_scope(100);
        for(unsigned id=101;id<=104;++id)check(w->stage_retribution(source,*w->effect_scope(id),*retribution(0),100));
        check(w->stage_retribution(source,source,*retribution(0),100));check(w->maintained_enchantment_count(source,100)==5);
        if(dense)for(std::uint64_t at=200;at<3100;at+=100){w->tick(at);box->drain();}
        w->tick(3099);check(w->maintained_enchantment_count(source,3099)==5);
        w->tick(3100);const auto remaining=w->maintained_enchantment_count(source,3100);const auto energy=w->npc_energy_snapshot(source);
        if(remaining!=4||!energy||energy->current!=0)std::cerr<<"NPC zero frontier dense="<<dense<<" count="<<(remaining?std::to_string(*remaining):"unknown")
            <<" energy="<<(energy?std::to_string(energy->current):"unknown")<<" self_entries="<<w->status_snapshot(source,3100)->size()<<'\n';
        check(remaining==4&&energy&&energy->current==0);
        check(w->status_snapshot(source,3100)->empty());w->tick(5100);check(w->maintained_enchantment_count(source,5100)==4&&w->npc_energy_snapshot(source)->current==0);
    }
    // Actual NPC opposition: hostile NPC maintains its ally, and a friendly
    // attacker receives protected Holy return through the opposed-owned path.
    for(bool reversal:{false,true}){
        auto w=std::make_unique<world::Instance>();w->configure_npcs({make_npc(100,true),make_npc(101,true),make_npc(102,false)});
        auto box=std::make_shared<world::Outbox>();join(*w,box);const auto caster=*w->effect_scope(100),target=*w->effect_scope(101),attacker=*w->effect_scope(102);
        check(w->stage_retribution(caster,target,*retribution(12),100));if(reversal)check(w->stage_npc_reversal_from(attacker,attacker,12,100));
        const combat::ai::ordinary::Reviewed weapon{20,20,12,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1200,32},"synthetic opposed return"};
        check(w->configure_npc_opposed_ordinary(attacker,weapon,1000));check(w->begin_npc_opposed_ordinary(attacker,target,1000).has_value());
        box->drain();w->tick(1750);check(w->npc_combat_snapshot(target)->health->current==480);
        check(w->npc_combat_snapshot(attacker)->health->current==(reversal?506:494));
        check(w->retribution_pending_damage_contacts()==0&&w->retribution_damage_failures()==0);
    }
    // Death of the actual NPC maintainer retires its remote lease, with no
    // attribution or ownership transferred to a reused actor/player ID.
    {auto w=std::make_unique<world::Instance>();w->configure_npcs({make_npc(100,true,100,4,1),make_npc(101,true)});auto box=std::make_shared<world::Outbox>();join(*w,box);
        const auto source=*w->effect_scope(100),target=*w->effect_scope(101),player=*w->effect_scope(1);
        check(w->stage_retribution(source,target,*retribution(12),100));combat::weapon::Owned sword{12,1,{20,20,2},12,false,false,27};
        check(w->ordinary_weapon(player,sword,20)&&w->ordinary_attack(player,100,500));w->tick(1300);w->cancel_ordinary_attack(player);
        check(w->npc_combat_snapshot(source)->health->current==0&&w->maintained_enchantment_count(source,1300)==0&&w->status_snapshot(target,1300)->empty());
    }
    // Actual independently moving NPC paths: caster only, recipient only,
    // different speeds, and equal-speed no-expiry control through40 seconds.
    for(unsigned mode=0;mode<4;++mode)for(bool dense:{false,true}){
        auto w=std::make_unique<world::Instance>();auto guardian=make_npc(100,false);guardian.x=9000;
        guardian.combat_definition->relation=combat::npcs::Relation::neutral;w->configure_npcs({guardian});
        auto box=std::make_shared<world::Outbox>();join(*w,box);const auto owner=*w->effect_scope(1),anchor=*w->effect_scope(100);
        auto a=make_npc(101,true),b=make_npc(102,true);a.x=0;b.x=100;
        check(w->begin_wave_objective(owner,anchor,73,1,1,2)&&w->spawn_quest_wave(owner,73,1,{a,b},[]{return true;}));
        const auto source=*w->effect_scope(60000),target=*w->effect_scope(60001);
        check(w->stage_retribution(source,target,*retribution(12),100));
        std::cerr<<"NPC range fixture mode="<<mode<<" dense="<<dense<<'\n';
        if(mode!=1)check(w->chase_quest_guardian(source,mode==2?440:220,100));
        if(mode!=0)check(w->chase_quest_guardian(target,mode==2?100:220,100));
        // Mode2 uses the supported440 speed ceiling. Starting at0/100,
        // separation is abs(-100+340*t): exactly5000 at15s, first
        // integer millisecond outside at15001, relative to cast time100.
        // The equality positions6600/1600 are exactly representable floats.
        const std::uint64_t crossing=mode==0?23282:mode==1?22373:mode==2?15101:0;
        const auto stop=crossing?crossing:40100;
        if(dense)for(std::uint64_t at=200;at<stop-1;at+=100){w->tick(at);box->drain();}
        w->tick(stop-1);check(w->maintained_enchantment_count(source,stop-1)==1);
        w->tick(stop);check(w->maintained_enchantment_count(source,stop)==(crossing?0u:1u));
        w->tick(40100);check(w->maintained_enchantment_count(source,40100)==(crossing?0u:1u));
        check(w->status_snapshot(target,40100)->empty()==bool(crossing));
    }
    // With no player maintainer, observer departure alone must NOT end the
    // NPC lease. Supported population replacement must retire it before reuse.
    {auto w=std::make_unique<world::Instance>();w->configure_npcs({make_npc(100,false),make_npc(101,false)});
        auto box=std::make_shared<world::Outbox>();join(*w,box);const auto source=*w->effect_scope(100),target=*w->effect_scope(101);
        check(w->stage_retribution(source,target,*retribution(12),100));w->leave(1);
        check(w->maintained_enchantment_count(source,100)==1);
        w->configure_npcs({make_npc(100,false),make_npc(101,false)});join(*w,box);
        const auto next=*w->effect_scope(100),recipient=*w->effect_scope(101);
        check(next!=source&&recipient!=target&&!w->maintained_enchantment_count(source,100).has_value()&&w->maintained_enchantment_count(next,100)==0);
        check(w->status_snapshot(recipient,100)->empty()&&!w->cancel_maintained_enchantment(source,1,100));
        check(w->stage_retribution(next,recipient,*retribution(0),100));
    }
    std::cout<<"PASS staged NPC Retribution maintained chronology/recipient/opposed protection "<<checks<<" checks; no paid binding\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
