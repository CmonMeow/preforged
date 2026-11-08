#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Death Charge relocation line "+std::to_string(at.line()));
}
void fixture(const std::filesystem::path& path,unsigned kind=0){
    std::filesystem::create_directories(path.parent_path());std::ofstream out(path,std::ios::binary);
    if(!out)throw std::runtime_error("fixture output unavailable");
    const auto u32=[&](std::uint32_t v){for(unsigned i=0;i<4;++i)out.put(char(v>>(8*i)));};
    const auto f32=[&](float v){u32(std::bit_cast<std::uint32_t>(v));};
    out.write("FWNAV001",8);for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})u32(n);
    f32(-2000);f32(-2000);f32(kind==3?8000.f:kind==2?4000.f:2000.f);f32(2000);u32(2);
    const auto cell=[&](float left,float right){for(unsigned i=0;i<5;++i)u32(0xffffffff);
        f32(-2000);f32(2000);f32(left);f32(right);f32(left);f32(right);};
    u32(kind==1?2:1);if(kind==1){cell(-2000,200);cell(400,2000);}else cell(-2000,kind==3?8000.f:2000.f);
    u32(1);cell(kind==2?2000.f:-2000.f,kind==3?8000.f:kind==2?4000.f:2000.f);out.close();
    if(kind==2){std::ofstream links(path.string()+".links");links<<"FWLINK01 7 2 2\n0 0 1 0\n1 0 0 0\n";}
}
void join(world::Instance& w,unsigned id,float x,unsigned team,unsigned plane=0,std::shared_ptr<world::Outbox> box={}){
    w.join(id,"Actor",x,0,box?box:std::make_shared<world::Outbox>(),0,{},plane,100,team);
    w.set_vitals(id,{1000,1000,100,100},100);
}
int main(int argc,char** argv)try{
    check(argc==2);fixture(argv[1]);
    auto mesh=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(argv[1]));
    for(auto disposition:{bundles::Disposition::ground,bundles::Disposition::destroy}){
        world::Instance w;w.configure_navigation(mesh);
        auto box=std::make_shared<world::Outbox>(),peer=std::make_shared<world::Outbox>();
        join(w,1,0,1,0,box);join(w,2,600,1,0,peer);
        join(w,3,0,1);
        const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);
        items::Definition item;item.id=42;item.file=1234;item.type=6;item.model=456;item.encoded_name=u"Bundle";
        const auto wire=w.reserve_ground_item_id();check(bool(wire));
        const auto floor=w.drop_ground_item(owner,item,1,*wire,200,true,[](const auto&){return true;});check(bool(floor));
        box->drain();peer->drain();
        check(w.pickup_ground_bundle(owner,floor->scope,1,disposition,200,210));
        const auto held=w.held_bundle(owner);check(held&&held->item.id==*wire&&w.ground_item_count()==0);
        check(!w.pickup_ground_bundle(target,floor->scope,1,disposition,200,211));
        const auto picked=box->drain(),other=peer->drain();
        const auto assignment=items::held_bundle_2022(1,*wire);
        check(std::find(picked.begin(),picked.end(),assignment)!=picked.end());
        check(std::find(other.begin(),other.end(),assignment)==other.end());
        check(std::find(picked.begin(),picked.end(),items::delete_2022(*wire))==picked.end());
        check(!w.drop_held_bundle(owner,held->generation+1,220));
        const skills::effects::RecallAnchor anchor{owner,target,77,210};
        auto stale_anchor=anchor;++stale_anchor.target.generation;
        check(!w.recall_step(stale_anchor,220)&&w.held_bundle(owner)&&w.ground_item_count()==0);
        check(bool(w.recall_step_preview(anchor,220))&&w.held_bundle(owner)&&w.ground_item_count()==0);
        const auto step=w.recall_step(anchor,230);check(bool(step));
        check(!w.held_bundle(owner)&&w.position(1,230).first==600);
        check(w.ground_item_count()==(disposition==bundles::Disposition::ground?1:0));
        check(!w.drop_held_bundle(owner,held->generation,231));
        if(disposition==bundles::Disposition::ground){
            const auto returned=step->bundle_drop->returned;check(bool(returned));
            check(returned->position.x==0&&returned->definition.id!=*wire&&returned->scope!=floor->scope);
            check(returned->definition.model==456&&returned->definition.encoded_name==u"Bundle");
            check(bool(w.recall_step({owner,*w.effect_scope(3),78,240},245)));
            check(w.pickup_ground_bundle(owner,returned->scope,1,disposition,200,250));
            const auto replacement=w.held_bundle(owner);check(replacement&&replacement->generation>held->generation);
            check(!w.drop_held_bundle(owner,held->generation,260));
            check(w.held_bundle(owner)&&w.ground_item_count()==0);
            check(w.drop_held_bundle(owner,replacement->generation,260));
            check(!w.held_bundle(owner)&&w.ground_item_count()==1);
        }
        const auto packets=box->drain();
        check(!packets.empty()&&packets.front()==items::held_bundle_2022(1,0));
    }
    // Owned upkeep end: no owner drop packets may overtake its retained UI batch.
    {
        world::Instance w;w.configure_navigation(mesh);
        auto box=std::make_shared<world::Outbox>(),peer=std::make_shared<world::Outbox>();
        join(w,1,0,1,0,box);join(w,2,600,1,0,peer);
        const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);
        skills::Slots bar{};bar[0]={925,0};bar[1]={293,0};check(w.configure_adrenaline(owner,bar,100));
        check(w.energy_capacity(1,6,0,100,{80,-2},22));
        check(w.stage_recall(owner,target,150));const auto effect=w.status_snapshot(owner,150)->front().instance;
        items::Definition item;item.id=42;item.file=1234;item.type=6;
        const auto floor=w.drop_ground_item(owner,item,1,*w.reserve_ground_item_id(),200,true,[](const auto&){return true;});check(bool(floor));
        check(w.pickup_ground_bundle(owner,floor->scope,1,bundles::Disposition::ground,200,210));
        check(w.resources(1)->max_energy==78);
        auto ordinary=item;ordinary.type=5;
        for(unsigned n=0;n<world::ground::State::capacity-1;++n)
            check(bool(w.drop_ground_item(owner,ordinary,1,*w.reserve_ground_item_id(),220,true,[](const auto&){return true;})));
        bool attempted_commit=false;
        check(!w.drop_ground_item(owner,ordinary,1,*w.reserve_ground_item_id(),220,true,[&](const auto&){attempted_commit=true;return true;}));
        check(!attempted_commit); // No durable inventory debit for reserved capacity.
        box->drain();peer->drain();check(w.cancel_maintained_enchantment(owner,effect,300));
        check(!w.held_bundle(owner)&&w.ground_item_count()==world::ground::State::capacity&&w.position(1,300).first==600);
        check(w.resources(1)->max_energy==100);
        const auto before_feedback=box->drain();
        check(std::find(before_feedback.begin(),before_feedback.end(),items::held_bundle_2022(1,0))==before_feedback.end());
        check(std::find(before_feedback.begin(),before_feedback.end(),items::delete_2022(floor->definition.id))==before_feedback.end());
        const auto peer_packets=peer->drain();
        const auto clear=items::held_bundle_2022(1,0),deleted=items::delete_2022(floor->definition.id);
        const auto trails=skills::particles::shadow_step_trail_2022(1,0,0,0,600,0,0);
        check(std::find(peer_packets.begin(),peer_packets.end(),trails[0])!=peer_packets.end());
        check(std::find(peer_packets.begin(),peer_packets.end(),clear)==peer_packets.end());
        check(std::find(peer_packets.begin(),peer_packets.end(),deleted)<std::find(peer_packets.begin(),peer_packets.end(),trails[0]));
        skills::casting::Runtime runtime(1);skills::casting::Hooks hooks;
        hooks.target_scope=[&](auto id){return w.effect_scope(id);};
        hooks.current_skillbar=[&](){return std::optional{bar};};
        hooks.skill_disabled_slot=[&](auto slot,auto at){return w.skill_disabled_slot(owner,slot,at);};
        hooks.recall_feedback_pending=[&](auto slot,auto at){return w.recall_feedback_pending(owner,slot,at);};
        const auto notices=w.pending_recall_feedback(owner);check(notices&&notices->size()==1);
        check(runtime.consume_recall_feedback(notices->front(),300,hooks,[&](const auto& reply){return w.publish_recall_feedback(reply);}));
        const auto packets=box->drain();check(!packets.empty()&&packets.front()==clear);
        check(std::find(packets.begin(),packets.end(),trails[0])!=packets.end());
        const auto restored_capacity=vitals::packets_2022(1,*w.resources(1)).front();
        check(std::find(packets.begin(),packets.end(),restored_capacity)<std::find(packets.begin(),packets.end(),trails[0]));
        check(std::find(packets.begin(),packets.end(),deleted)<std::find(packets.begin(),packets.end(),trails[0]));
        check(w.pending_recall_feedback(owner)->empty());
    }
    // Held persistent objects reserve a floor slot even at effective capacity.
    {
        world::Instance w;w.configure_navigation(mesh);join(w,1,0,1);join(w,2,600,2);
        const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);
        items::Definition item;item.id=42;item.file=1234;item.type=6;
        const auto floor=w.drop_ground_item(owner,item,1,*w.reserve_ground_item_id(),200,true,[](const auto&){return true;});check(bool(floor));
        check(w.pickup_ground_bundle(owner,floor->scope,1,bundles::Disposition::ground,200,210));
        std::optional<world::ground::Item> first;
        item.type=5;
        for(unsigned n=0;n<world::ground::State::capacity-1;++n){
            const auto extra=w.drop_ground_item(owner,item,1,*w.reserve_ground_item_id(),220,true,[](const auto&){return true;});
            check(bool(extra));if(!first)first=extra;
        }
        const auto plan=*skills::effects::death_charge(12);
        check(!w.drop_ground_item(owner,item,1,*w.reserve_ground_item_id(),230,true,[](const auto&){return true;}));
        check(w.held_bundle(owner)&&w.position(1,230).first==0&&w.ground_item_count()==world::ground::State::capacity-1);
        const auto step=w.death_charge_step(owner,target,plan,250);check(bool(step));
        check(!w.held_bundle(owner)&&step->bundle_drop->returned->position.x==0);
        check(w.ground_item_count()==world::ground::State::capacity&&w.position(1,250).first!=0);
    }
    // Every persistent holder owns one lease; destroying bundles own none.
    for(auto disposition:{bundles::Disposition::ground,bundles::Disposition::destroy}){
        world::Instance w;w.configure_navigation(mesh);join(w,1,0,1);join(w,2,0,1);
        const auto owner=*w.effect_scope(1),other=*w.effect_scope(2);
        items::Definition item;item.id=42;item.file=1234;item.type=6;
        for(auto actor:{owner,other}){
            const auto floor=w.drop_ground_item(actor,item,1,*w.reserve_ground_item_id(),200,true,[](const auto&){return true;});check(bool(floor));
            check(w.pickup_ground_bundle(actor,floor->scope,1,disposition,200,210));
        }
        item.type=5;const auto available=world::ground::State::capacity-(disposition==bundles::Disposition::ground?2:0);
        for(unsigned n=0;n<available;++n)
            check(bool(w.drop_ground_item(owner,item,1,*w.reserve_ground_item_id(),220,true,[](const auto&){return true;})));
        check(!w.drop_ground_item(owner,item,1,*w.reserve_ground_item_id(),220,true,[](const auto&){return true;}));
        check(w.drop_held_bundle(owner,w.held_bundle(owner)->generation,230));
        check(w.drop_held_bundle(other,w.held_bundle(other)->generation,230));
        check(!w.held_bundle(owner)&&!w.held_bundle(other)&&w.ground_item_count()==world::ground::State::capacity);
    }
    for(bool departure:{false,true})for(auto disposition:{bundles::Disposition::ground,bundles::Disposition::destroy}){
        world::Instance w;w.configure_navigation(mesh);
        auto peer=std::make_shared<world::Outbox>();join(w,1,0,1);join(w,2,600,1,0,peer);
        const auto owner=*w.effect_scope(1);
        items::Definition item;item.id=42;item.file=1234;item.type=6;
        const auto floor=w.drop_ground_item(owner,item,1,*w.reserve_ground_item_id(),200,true,[](const auto&){return true;});check(bool(floor));
        check(w.pickup_ground_bundle(owner,floor->scope,1,disposition,200,210));
        const auto held=*w.held_bundle(owner);peer->drain();
        if(departure)w.leave(1);else check(w.kill(owner,300));
        check(!w.held_bundle(owner));
        check(w.ground_item_count()==(disposition==bundles::Disposition::ground?1:0));
        const auto released=w.ground_item(floor->scope.actor+1);
        if(disposition==bundles::Disposition::ground)check(released&&released->position.x==0);
        check(!w.drop_held_bundle(owner,held.generation,400));
        if(!departure)check(!w.kill(owner,400));
        w.tick(500);check(w.ground_item_count()==(disposition==bundles::Disposition::ground?1:0));
        if(departure){
            w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{},0,600,1);
            const auto replacement=*w.effect_scope(1);check(replacement!=owner&&!w.held_bundle(replacement));
        }
        const auto packets=peer->drain();
        check(std::count(packets.begin(),packets.end(),items::delete_2022(floor->definition.id))==1);
    }
    for(auto disposition:{bundles::Disposition::ground,bundles::Disposition::destroy}){
        world::Instance w;w.configure_navigation(mesh);auto box=std::make_shared<world::Outbox>();join(w,1,0,1,0,box);
        const auto owner=*w.effect_scope(1);
        items::Definition item;item.id=42;item.file=1234;item.type=6;
        const auto floor=w.drop_ground_item(owner,item,1,*w.reserve_ground_item_id(),200,true,[](const auto&){return true;});check(bool(floor));
        check(w.pickup_ground_bundle(owner,floor->scope,1,disposition,200,210));
        const auto generation=w.held_bundle(owner)->generation;box->drain();unsigned calls=0;
        check(!w.switch_weapon_set(owner,generation+1,220,[&]{++calls;return true;})&&calls==0);
        check(!w.switch_weapon_set(owner,std::nullopt,220,[&]{++calls;return true;})&&calls==0);
        check(!w.switch_weapon_set(owner,generation,220,[&]{++calls;return false;})&&calls==1);
        check(w.held_bundle(owner)&&w.ground_item_count()==0&&box->drain().empty());
        check(w.switch_weapon_set(owner,generation,230,[&]{++calls;return true;})&&calls==2);
        check(!w.held_bundle(owner)&&w.ground_item_count()==(disposition==bundles::Disposition::ground?1:0));
        const auto packets=box->drain();check(!packets.empty()&&packets.front()==items::held_bundle_2022(1,0));
        check(w.switch_weapon_set(owner,std::nullopt,240,[&]{++calls;return true;})&&calls==3);
        check(box->drain().empty());
    }
    // Exhaust the real monotonic wire allocator; never inject mutable World state.
    {
        world::Instance w;w.configure_navigation(mesh);auto box=std::make_shared<world::Outbox>();join(w,1,0,1,0,box);
        const auto owner=*w.effect_scope(1);
        items::Definition item;item.id=42;item.file=1234;item.type=6;
        const auto floor=w.drop_ground_item(owner,item,1,*w.reserve_ground_item_id(),200,true,[](const auto&){return true;});check(bool(floor));
        check(w.pickup_ground_bundle(owner,floor->scope,1,bundles::Disposition::ground,200,210));
        const auto generation=w.held_bundle(owner)->generation;
        while(w.reserve_ground_item_id()){}
        box->drain();check(w.kill(owner,300)&&w.held_bundle(owner));
        check(w.resurrect(owner,400));bool saved=false;
        check(!w.switch_weapon_set(owner,generation,410,[&]{saved=true;return true;})&&!saved);
        w.tick(500);check(w.held_bundle(owner)&&w.ground_item_count()==0);
        const auto packets=box->drain();
        check(std::find(packets.begin(),packets.end(),items::delete_2022(floor->definition.id))==packets.end());
        w.leave(1);w.join(1,"Replacement",0,0,box,0,{},0,600,1);box->drain();
        w.tick(700);check(!w.held_bundle(*w.effect_scope(1))&&w.ground_item_count()==0);
        const auto later=box->drain();check(std::find(later.begin(),later.end(),items::held_bundle_2022(1,0))==later.end());
    }
    {
        world::Instance w;w.configure_navigation(mesh);join(w,1,0,1);join(w,2,60,2);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        const auto attack=*combat::weapon::attack_skill(340,12);
        const combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,attack.required_type};
        check(w.ordinary_weapon(source,weapon,20));check(w.physical_armor(target,60));
        check(w.shield_equipment(target,skills::effects::ShieldEquipment{1,42},8,100));
        check(w.status_mutation(target,378,123,12,100,5100,true));
        unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return .99;});
        items::Definition item;item.id=42;item.file=1234;item.type=6;
        const auto floor=w.drop_ground_item(target,item,1,*w.reserve_ground_item_id(),200,true,[](const auto&){return true;});check(bool(floor));
        check(w.pickup_ground_bundle(target,floor->scope,1,bundles::Disposition::ground,200,210));
        check(w.shield_equipment(target,skills::effects::ShieldEquipment{2,43},8,220));
        check(w.melee_attack(source,target,weapon,attack,300));
        check(w.resources(2)->health==980&&draws==0); // No shield block OR flat reduction while held.
        check(w.drop_held_bundle(target,w.held_bundle(target)->generation,310));
        check(w.melee_attack(source,target,weapon,attack,400));
        check(w.resources(2)->health==975&&draws==1); // Same saved revision, restored immediately.
    }
    for(unsigned mode=0;mode<4;++mode){
        world::Instance w;w.configure_navigation(mesh);join(w,1,0,1);join(w,2,600,1);
        const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.energy_capacity(1,6,0,100,{22,-2},22));
        check(w.consume_energy(1,32,owner,150));check(w.resources(1)->energy==10);
        items::Definition item;item.id=42;item.file=1234;item.type=6;
        const auto floor=w.drop_ground_item(owner,item,1,*w.reserve_ground_item_id(),200,true,[](const auto&){return true;});check(bool(floor));
        check(w.pickup_ground_bundle(owner,floor->scope,1,bundles::Disposition::ground,200,210));
        check(w.resources(1)->max_energy==20&&w.resources(1)->energy==0);
        check(w.overcast_energy_snapshot(owner)->debt==12);
        check(w.energy_capacity(1,6,0,220,{22,-2},22)); // Full-source refresh must not double-subtract.
        check(w.resources(1)->max_energy==20&&w.overcast_energy_snapshot(owner)->debt==12);
        if(mode==0)check(w.drop_held_bundle(owner,w.held_bundle(owner)->generation,230));
        if(mode==1)check(bool(w.recall_step({owner,target,77,210},230)));
        if(mode==2){
            check(!w.switch_weapon_set(owner,w.held_bundle(owner)->generation,230,[]{return false;}));
            check(w.resources(1)->max_energy==20&&w.overcast_energy_snapshot(owner)->debt==12);
            check(w.switch_weapon_set(owner,w.held_bundle(owner)->generation,240,[]{return true;}));
        }
        if(mode==3)check(w.kill(owner,230));
        check(!w.held_bundle(owner)&&w.resources(1)->max_energy==42&&w.resources(1)->energy==10);
        check(w.overcast_energy_snapshot(owner)->debt==0);
    }
    // A lethal pickup zero reaction must publish assignment before its death
    // release clear, and never leave an assigned item on the corpse.
    for(bool exact_zero:{false,true}){
        world::Instance w;w.configure_navigation(mesh);auto box=std::make_shared<world::Outbox>();join(w,1,0,1,0,box);join(w,2,60,2);
        const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);
        w.set_vitals(1,{1,1000,100,100},100);
        check(w.energy_capacity(1,6,0,100,{22,-2},22));check(w.consume_energy(1,exact_zero?20:32,owner,150));
        check(w.apply_mind_wrack(foe,owner,12,false,160));
        items::Definition item;item.id=42;item.file=1234;item.type=6;
        const auto floor=w.drop_ground_item(owner,item,1,*w.reserve_ground_item_id(),200,true,[](const auto&){return true;});check(bool(floor));
        box->drain();check(w.pickup_ground_bundle(owner,floor->scope,1,bundles::Disposition::ground,200,210));
        if(!exact_zero){
            check(w.resources(1)->health==1&&w.held_bundle(owner)&&w.ground_item_count()==0);
            check(w.overcast_energy_snapshot(owner)->debt==12);continue;
        }
        check(w.resources(1)->health==0&&!w.held_bundle(owner)&&w.ground_item_count()==1);
        check(w.resources(1)->max_energy==42&&w.resources(1)->energy==22);
        const auto packets=box->drain();const auto assignment=items::held_bundle_2022(1,floor->definition.id),clear=items::held_bundle_2022(1,0);
        const auto assigned=std::find(packets.begin(),packets.end(),assignment),cleared=std::find(packets.begin(),packets.end(),clear);
        check(assigned!=packets.end()&&cleared!=packets.end()&&assigned<cleared);
        check(std::count(packets.begin(),packets.end(),clear)==1);
        w.tick(300);check(w.ground_item_count()==1);
    }
    // Pickup can exhaust maintained Recall at the very same resource frontier.
    {
        world::Instance w;w.configure_navigation(mesh);join(w,1,0,1);join(w,2,600,1);
        const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);
        skills::Slots bar{};bar[0]={925,0};bar[1]={293,0};check(w.configure_adrenaline(owner,bar,100));
        check(w.energy_capacity(1,6,0,100,{22,-2},22));check(w.consume_energy(1,20,owner,200));
        check(w.stage_recall(owner,target,200));
        items::Definition item;item.id=42;item.file=1234;item.type=6;
        const auto floor=w.drop_ground_item(owner,item,1,*w.reserve_ground_item_id(),200,true,[](const auto&){return true;});check(bool(floor));
        check(w.pickup_ground_bundle(owner,floor->scope,1,bundles::Disposition::ground,200,200));
        check(!w.held_bundle(owner)&&w.ground_item_count()==1&&w.position(1,200).first==600);
        check(w.resources(1)->max_energy==42&&w.resources(1)->energy==22);
        check(w.status_snapshot(owner,200)->empty());
        check(w.skill_disabled_slot(owner,{293,0},200)==10200);
        check(w.pending_recall_feedback(owner)->size()==1);
        skills::casting::Runtime runtime(1);skills::casting::Hooks hooks;
        hooks.target_scope=[&](auto id){return w.effect_scope(id);};
        hooks.current_skillbar=[&](){return std::optional{bar};};
        hooks.skill_disabled_slot=[&](auto slot,auto at){return w.skill_disabled_slot(owner,slot,at);};
        hooks.recall_feedback_pending=[&](auto slot,auto at){return w.recall_feedback_pending(owner,slot,at);};
        check(runtime.consume_recall_feedback(w.pending_recall_feedback(owner)->front(),200,hooks,[&](const auto& reply){return w.publish_recall_feedback(reply);}));
        check(w.pending_recall_feedback(owner)->empty());w.tick(300);
        check(w.ground_item_count()==1&&w.position(1,300).first==600);
    }
    // Actual World Overcast recovery must survive suppression/restoration with
    // the fractional Energy and burden predicted by the shared settlement core.
    {
        world::Instance w;w.configure_navigation(mesh);join(w,1,0,1);const auto owner=*w.effect_scope(1);
        check(w.energy_capacity(1,6,0,100,{22,2},22));
        check(w.activate_overcast_energy(owner,231,5,1,200));
        items::Definition item;item.id=42;item.file=1234;item.type=6;
        const auto floor=w.drop_ground_item(owner,item,1,*w.reserve_ground_item_id(),210,true,[](const auto&){return true;});check(bool(floor));
        check(w.pickup_ground_bundle(owner,floor->scope,1,bundles::Disposition::ground,200,210));
        const auto held=*w.overcast_energy_snapshot(owner);check(held.resources.max_energy==20);
        const auto expected=overcast::settle(held.curve,owner,held.resources,held.regeneration,held.debt,510,4,held.fractional_tail);check(bool(expected));
        check(w.drop_held_bundle(owner,w.held_bundle(owner)->generation,510));
        const auto released=*w.overcast_energy_snapshot(owner);
        check(released.resources.max_energy==42&&released.curve.burden_units()==expected->curve.burden_units());
        check(overcast::precise_current(released)==overcast::precise_current(*expected)+22*overcast::energy_unit);
        check(released.regeneration.remainder()==expected->regeneration.remainder()&&released.fractional_tail==expected->fractional_tail);
    }
    // Morale changes the base component while hands remain suppressed. The
    // next release must use the new morale, not the pickup's nominal capacity.
    {
        world::Instance w;w.configure_navigation(mesh);join(w,1,0,1);const auto owner=*w.effect_scope(1);
        check(w.reviewed_health_capacity(owner,1000,0,100));
        check(w.energy_capacity(1,6,0,100,{22,-2},22));
        items::Definition item;item.id=42;item.file=1234;item.type=6;
        const auto floor=w.drop_ground_item(owner,item,1,*w.reserve_ground_item_id(),200,true,[](const auto&){return true;});check(bool(floor));
        check(w.pickup_ground_bundle(owner,floor->scope,1,bundles::Disposition::ground,200,210));
        check(w.resources(1)->max_energy==20);unsigned commits=0;
        check(w.honeycomb(owner,220,true,[&]{++commits;return true;})&&commits==1);
        check(w.morale(owner)==105&&w.resources(1)->max_energy==21&&w.held_bundle(owner));
        check(w.drop_held_bundle(owner,w.held_bundle(owner)->generation,230));
        check(w.resources(1)->max_energy==43&&w.resources(1)->energy==43);
    }
    std::cout<<"Bundle World transactions passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

