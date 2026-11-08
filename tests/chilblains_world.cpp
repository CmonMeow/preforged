#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <thread>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Chilblains World line "+std::to_string(at.line()));
}
int main()try{
    for(auto order:{ChilblainsOrder::damage_then_remove,ChilblainsOrder::remove_then_damage})
    for(bool reversal:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);
            w.set_vitals(id,{1000,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.status_mutation(target,1399,80,12,100,6100,true));
        if(reversal)check(w.status_mutation(target,307,81,12,100,8100,true));
        box->drain();check(w.stage_chilblains(source,target,*chilblains(0),order,20,100));
        const bool captured=order==ChilblainsOrder::damage_then_remove;
        const unsigned after=captured?(reversal?1000:995):(reversal?995:990);
        check(w.resources(2)->health==after);
        const auto packets=box->drain();
        check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==67;})==(reversal&&captured?2:1));
        check(w.energy_drain(source,target,*energy_drain(42,12),20,101));
        check(w.resources(2)->health==after-(reversal&&!captured?62:72));
    }
    for(auto order:{ChilblainsOrder::damage_then_remove,ChilblainsOrder::remove_then_damage}){
        world::Instance w;
        for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
            w.set_vitals(id,{id==1?1000u:500u,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2),monk=*w.effect_scope(3);
        check(w.party_action(3,158,2,100)&&w.party_action(2,154,3,100));
        const auto seed=*healing_seed(12);check(w.complete_healing_seed(monk,target,seed,0,100));
        for(auto scope:{target,monk})check(w.status_mutation(scope,1399,80,12,100,6100,true));
        const auto plan=*chilblains(12);
        check(w.stage_chilblains(source,target,plan,order,20,100));
        const bool captured=order==ChilblainsOrder::damage_then_remove;
        const unsigned expected=unsigned(std::ceil(500-plan.damage+(captured?5+seed.healing:0)));
        check(w.resources(2)->health==expected&&w.resources(3)->health==expected);
    }
    for(auto order:{ChilblainsOrder::damage_then_remove,ChilblainsOrder::remove_then_damage})
    for(bool reversal:{false,true}){
        world::Instance w;world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=100;
        npc.definition.visual_adjustment={0,0,0,100};npc.definition.level=20;combat::npcs::Definition d;
        d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};
        d.cold_armor=60;d.damageable=d.conditions_allowed=true;d.healing_allowed=true;
        d.healing_evidence="authored Chilblains conversion fixture";npc.combat_definition=d;w.configure_npcs({npc});
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
        check(w.stage_npc_shield_absorption(target,12,100));
        if(reversal)check(w.stage_npc_reversal(target,12,100));
        box->drain();
        check(w.stage_chilblains(source,target,*chilblains(0),order,20,100));
        const bool captured=order==ChilblainsOrder::damage_then_remove;
        const auto packets=box->drain();
        check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==67;})==(reversal&&captured?2:1));
        const double after=captured?(reversal?1000:995):(reversal?995:990);
        check(w.npc_combat_snapshot(target)->health->current==after);
        check(w.damage_npc(source,target,{combat::Kind::cold,50,20,144},1248,101).reason==combat::Reason::accepted);
        check(w.npc_combat_snapshot(target)->health->current==after-(reversal&&!captured?40:50));
        check(w.has_condition(source,combat::conditions::Kind::poison,101));
    }
    for(auto order:{ChilblainsOrder::damage_then_remove,ChilblainsOrder::remove_then_damage})
    for(unsigned mode=0;mode<9;++mode){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        if(mode==7){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
            d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};
            d.cold_armor=60;d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});}
        for(unsigned id=1;id<=5;++id){
            w.join(id,"Actor",id==1?0:id==3?422:id==4?422.1f:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);
            w.set_vitals(id,{1000,1000,100,100},100);
            check(w.elemental_armor(*w.effect_scope(id),60,id==3?100:60));
        }
        auto source=*w.effect_scope(1),selected=*w.effect_scope(2);auto plan=*chilblains(mode==6?0:12);
        if(mode==1)++selected.generation;
        if(mode==2)check(w.elemental_armor(*w.effect_scope(3),60,{}));
        if(mode==3)++plan.damage;
        if(mode==4){check(w.status_mutation(selected,1399,11,12,100,7100,true));}
        else{const auto recipient=*w.effect_scope(2);check(w.status_mutation(recipient,214,11,12,100,5100,true));
            std::optional<skills::sacrifice::Loss> paid;check(w.energy_enchantment(*w.effect_scope(3),recipient,*energy_enchantment(119,0),200,&paid));
            w.set_vitals(3,{1000,1000,100,100},100);}
        if(mode==5){w.leave(2);}
        if(mode==8)w.set_vitals(2,{10,1000,100,100},100);
        box->drain();std::vector<world::Packet> visuals;
        const bool ok=w.stage_chilblains(source,selected,plan,order,20,1000,&visuals);
        if(ok!=(mode==0||mode==4||mode==6||mode==7||mode==8))std::cerr<<"mode="<<mode<<" order="<<unsigned(order)<<" accepted="<<ok<<"\n";
        check(ok==(mode==0||mode==4||mode==6||mode==7||mode==8));
        if(!ok){check(visuals.empty()&&!w.has_condition(source,combat::conditions::Kind::poison,1000));
            if(mode!=5)check(w.resources(2)->health==1000);check(w.resources(3)->health==1000);continue;}
        check(visuals.size()==(mode==7?3:2)&&visuals[0]==skills::particles::chilblains_target_2022(2,1));
        check(w.resources(2)->health==(mode==8?0:unsigned(std::ceil(1000-plan.damage+(mode==4&&order==ChilblainsOrder::damage_then_remove?5:0)))));
        check(w.resources(3)->health==unsigned(std::ceil(1000-plan.damage/2)));
        check(w.resources(4)->health==1000&&w.resources(5)->health==1000);
        check(w.has_condition(source,combat::conditions::Kind::poison,1000));
        const auto statuses=w.status_snapshot(selected,1000);check(statuses&&statuses->size()==(mode==6?1:0));
        if(mode==6)check(statuses->front().instance==11);
        if(mode==7)check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==1000-plan.damage);
        const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),visuals[0])==0);
    }
    for(auto order:{ChilblainsOrder::damage_then_remove,ChilblainsOrder::remove_then_damage})
    for(unsigned mode=0;mode<6;++mode){
        world::Instance w;std::vector<world::Instance::Npc> npcs;
        for(unsigned id=100;id<=103;++id){world::Instance::Npc n;n.id=n.definition.id=id;
            n.x=id==101?422:id==103?422.1f:100;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
            d.relation=id==102?combat::npcs::Relation::allied_to_players:combat::npcs::Relation::hostile_to_players;
            d.spawn_health=combat::Health{mode==4&&id==100?10.:1000.,1000};
            if(!(mode==1&&id==101))d.cold_armor=id==101?100:60;
            d.damageable=d.conditions_allowed=true;n.combat_definition=d;npcs.push_back(n);}
        w.configure_npcs(npcs);
        for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:mode==5?1000:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
            w.set_vitals(id,{1000,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
        const auto source=*w.effect_scope(1);auto selected=*w.effect_scope(100);
        if(mode==2)check(w.elemental_armor(*w.effect_scope(2),60,{}));
        if(mode==3)++selected.generation;
        std::vector<world::Packet> visuals;const auto plan=*chilblains(12);
        const bool ok=w.stage_chilblains(source,selected,plan,order,20,1000,&visuals);
        check(ok==(mode==0||mode==4||mode==5));
        const auto hp=[&](unsigned id){return w.npc_combat_snapshot(*w.effect_scope(id))->health->current;};
        if(!ok){check(visuals.empty()&&hp(100)==1000&&hp(101)==1000&&w.resources(2)->health==1000);
            check(!w.has_condition(source,combat::conditions::Kind::poison,1000));continue;}
        check(visuals.size()==(mode==5?2:3));
        check(hp(100)==(mode==4?0:1000-plan.damage)&&hp(101)==1000-plan.damage/2);
        check(hp(102)==1000&&hp(103)==1000);
        check(w.resources(2)->health==(mode==5?1000:unsigned(std::ceil(1000-plan.damage))));
        check(w.has_condition(source,combat::conditions::Kind::poison,1000));
        const auto deaths=w.take_npc_deaths(source);check(deaths.size()==(mode==4?1:0));
        check(w.take_npc_deaths(source).empty());
    }
    for(unsigned poison_case:{0u,1u,2u}){
        const bool already_poisoned=poison_case==1;
        world::Instance w;std::array<std::shared_ptr<world::Outbox>,3> boxes;
        for(unsigned id=1;id<=3;++id){boxes[id-1]=std::make_shared<world::Outbox>();
            w.join(id,"Actor",id==1?0:100,0,boxes[id-1],0,{}, {},100,id==1?1:2);
            w.set_vitals(id,{1000,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        std::optional<skills::sacrifice::Loss> paid;
        check(w.energy_enchantment(*w.effect_scope(3),target,*energy_enchantment(119,0),200,&paid));
        w.set_vitals(3,{1000,1000,100,100},100);
        unsigned old_poison=0;
        if(poison_case){check(w.apply_condition(target,source,combat::conditions::Kind::poison,200,poison_case==1?20000:5000));w.tick(1000);
            for(const auto& p:boxes[0]->drain())if(p.size()>=16&&p[0]==65&&p[2]==1&&p[6]==228&&p[7]==1)
                old_poison=unsigned(p[12])|(unsigned(p[13])<<8)|(unsigned(p[14])<<16)|(unsigned(p[15])<<24);
            check(old_poison!=0);}

        for(auto& box:boxes)box->drain();
        std::function<void()> publication;std::vector<world::Packet> visuals;
        check(w.stage_chilblains(source,target,*chilblains(12),20,1000,&visuals,&publication));
        check(w.has_condition(source,combat::conditions::Kind::poison,11000)==already_poisoned);
        check(bool(publication));for(auto& box:boxes)check(box->drain().empty());
        const world::Packet prefix{1,0};for(auto& box:boxes)box->enqueue({prefix});
        std::thread first([&]{publication();}),second([&]{publication();});first.join();second.join();
        const auto private_target_capacity=[](const world::Packet& p){return p.size()==14&&p[0]==158&&p[1]==0&&p[2]==41&&p[6]==2;};
        for(unsigned i=0;i<3;++i){const auto packets=boxes[i]->drain();check(!packets.empty()&&packets.front()==prefix);
            const auto announcements=std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=8&&p[0]==65&&p[1]==0&&p[2]==1&&p[6]==228&&p[7]==1;});
            check(announcements==unsigned(!already_poisoned));
            if(poison_case==2){
                const auto removed=std::find_if(packets.begin(),packets.end(),[&](const auto& p){return p.size()==10&&p[0]==67&&p[2]==1&&(unsigned(p[6])|(unsigned(p[7])<<8)|(unsigned(p[8])<<16)|(unsigned(p[9])<<24))==old_poison;});
                const auto added=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=8&&p[0]==65&&p[2]==1&&p[6]==228&&p[7]==1;});
                check(removed!=packets.end()&&added!=packets.end()&&removed<added);
            }
            const auto count=std::count_if(packets.begin(),packets.end(),private_target_capacity);
            check(i==1?count>0:count==0);}
        publication();for(auto& box:boxes)check(box->drain().empty());
    }
    std::cout<<"PASS staged Chilblains mixed World area,recipient-private deferred publication,once-only concurrent release and spell guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}






