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
    if(argc!=2)throw std::runtime_error("fixture path required");fixture(argv[1]);
    auto mesh=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(argv[1]));
    const auto plan=*skills::effects::death_charge(12);
    for(unsigned mode=0;mode<3;++mode){
        auto owner_box=std::make_shared<world::Outbox>(),peer_box=std::make_shared<world::Outbox>();
        world::Instance w;w.configure_navigation(mesh);join(w,1,0,1,0,owner_box);join(w,2,600,1,0,peer_box);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        skills::Slots bar{};bar[0]={925,0};bar[1]={293,0};check(w.configure_adrenaline(source,bar,100));
        check(w.stage_recall(source,target,200));const auto effects=w.status_snapshot(source,200);
        check(effects&&effects->size()==1&&effects->front().skill==925);
        const auto effect=effects->front().instance;
        const skills::effects::RecallAnchor anchor{source,target,effect,200};
        const auto removed=*skills::effects::recall_end_wire(anchor,effects->front());
        owner_box->drain();peer_box->drain();
        if(mode==1)check(w.kill(target,300));
        if(mode==2)check(w.kill(source,300));
        else check(w.cancel_maintained_enchantment(source,effect,300));
        check(w.status_snapshot(source,300)->empty());
        check(!w.cancel_maintained_enchantment(source,effect,300));
        check(w.position(1,300).first==(mode==0?600:0));
        check(w.skill_disabled_slot(source,{293,0},300)==(mode==0?10300:0));
        const auto notices=w.pending_recall_feedback(source);check(bool(notices));
        if(mode==0){
            check(notices->size()==1);
            skills::casting::Runtime runtime(1);skills::casting::Hooks hooks;
            hooks.target_scope=[&](auto id){return w.effect_scope(id);};
            hooks.current_skillbar=[&](){return std::optional{bar};};
            hooks.skill_disabled_slot=[&](auto slot,auto at){return w.skill_disabled_slot(source,slot,at);};
            hooks.recall_feedback_pending=[&](auto slot,auto at){return w.recall_feedback_pending(source,slot,at);};
            hooks.events=[](const auto&){throw std::runtime_error("Recall preparation leaked feedback");};
            runtime.update(300,hooks); // Normal synchronization must defer the retained UI/cache update.
            const auto reply=runtime.prepare_recall_feedback(notices->front(),300,hooks);check(bool(reply));
            check(reply->packets.size()==2&&reply->packets[1]==skills::casting::slot_event(228,1,{293,0},10));
            auto invalid=*reply;++invalid.notice.event;check(!w.publish_recall_feedback(invalid));
            invalid=*reply;invalid.current[7]={22,0};check(!w.publish_recall_feedback(invalid));
            invalid=*reply;invalid.packets[1]=skills::casting::slot_event(228,1,{293,0},9);
            check(!w.publish_recall_feedback(invalid)&&w.pending_recall_feedback(source)->size()==1);
            invalid=*reply;invalid.slots.pop_back();check(!w.publish_recall_feedback(invalid));
            invalid=*reply;invalid.slots[0].slot.copy=99;check(!w.publish_recall_feedback(invalid));
            invalid=*reply;invalid.packets[1]=skills::casting::slot_event(228,1,{293,0},11);
            check(!w.publish_recall_feedback(invalid));
            check(!runtime.consume_recall_feedback(notices->front(),300,hooks,[](const auto&){return false;}));
            check(runtime.prepare_recall_feedback(notices->front(),300,hooks)->packets.size()==2);
            const world::Packet later{1,0,42};owner_box->enqueue({later});
            const auto before_reply=owner_box->drain();
            check(std::find(before_reply.begin(),before_reply.end(),later)==before_reply.end());
            check(runtime.consume_recall_feedback(notices->front(),300,hooks,[&](const auto& prepared){return w.publish_recall_feedback(prepared);}));
            check(!w.publish_recall_feedback(*reply));
            runtime.update(301,hooks); // Import after acknowledgement prevents duplicate ordinary UI.
            check(w.pending_recall_feedback(source)->empty());
        }else check(notices->empty());
        const auto owner_packets=owner_box->drain(),peer_packets=peer_box->drain();
        check(std::count(owner_packets.begin(),owner_packets.end(),removed.upkeep)==1);
        check(std::count(owner_packets.begin(),owner_packets.end(),removed.effect)==1);
        check(std::count(peer_packets.begin(),peer_packets.end(),removed.upkeep)==0);
        check(std::count(peer_packets.begin(),peer_packets.end(),removed.effect)==1);
        const auto trail=skills::particles::shadow_step_trail_2022(1,0,0,0,600,0,0);
        if(mode==0){
            const auto origin=std::find(owner_packets.begin(),owner_packets.end(),trail[0]);
            const auto destination=std::find(owner_packets.begin(),owner_packets.end(),trail[1]);
            const auto upkeep=std::find(owner_packets.begin(),owner_packets.end(),removed.upkeep);
            const auto status=std::find(owner_packets.begin(),owner_packets.end(),removed.effect);
            check(origin<destination&&destination<upkeep&&upkeep<status);
            const auto feedback=std::find(owner_packets.begin(),owner_packets.end(),skills::casting::slot_event(228,1,{293,0},10));
            check(destination<feedback&&feedback<upkeep);
            const auto position=std::find_if(status,owner_packets.end(),[](const auto& p){return p.size()==16&&p[0]==44;});
            check(position!=owner_packets.end());
            check(position<std::find(owner_packets.begin(),owner_packets.end(),world::Packet{1,0,42}));
            const auto peer_trail=std::find(peer_packets.begin(),peer_packets.end(),trail[1]);
            check(peer_trail<std::find(peer_packets.begin(),peer_packets.end(),removed.effect));
        }else check(std::find(owner_packets.begin(),owner_packets.end(),trail[0])==owner_packets.end());
    }
    // Leaving with unresolved owner feedback must release the outbox barrier
    // and must not deliver an old notice into a replacement actor lifetime.
    {
        auto box=std::make_shared<world::Outbox>();world::Instance w;w.configure_navigation(mesh);
        join(w,1,0,1,0,box);join(w,2,600,1);const auto source=*w.effect_scope(1);
        skills::Slots bar{};bar[0]={925,0};check(w.configure_adrenaline(source,bar,100));
        check(w.stage_recall(source,*w.effect_scope(2),100));
        const auto effect=w.status_snapshot(source,100)->front().instance;
        check(w.cancel_maintained_enchantment(source,effect,200)&&w.pending_recall_feedback(source)->size()==1);
        w.leave(1);box->drain();const world::Packet marker{1,0,99};box->enqueue({marker});
        check(box->drain()==std::vector<world::Packet>{marker}&&!w.pending_recall_feedback(source));
        join(w,1,0,1,0,box);check(w.pending_recall_feedback(*w.effect_scope(1))->empty());
    }
    // Recall upkeep reaches zero at3100,even when the observer settles later.
    // The return and disable deadline belong to that boundary,not the poll time.
    for(bool delayed:{false,true})for(bool dead_target:{false,true}){
        world::Instance w;w.configure_navigation(mesh);join(w,1,0,1);join(w,2,600,1);
        w.set_vitals(1,{1000,1000,1,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        skills::Slots bar{};bar[0]={925,0};bar[1]={293,0};
        check(w.configure_adrenaline(source,bar,100));
        check(w.energy_capacity(1,3,0,100,{80,-2})&&w.stage_recall(source,target,100));
        if(dead_target)check(w.kill(target,200));
        check(w.settle_resources(source,3099));
        check(w.status_snapshot(source,3099)->size()==1&&w.position(1,3099).first==0);
        const unsigned observed=delayed?6100:3100;
        check(w.settle_resources(source,observed));
        check(w.status_snapshot(source,observed)->empty()&&w.maintained_enchantment_count(source,observed)==0);
        check(w.resources(1)->energy==0&&w.position(1,observed).first==(dead_target?0:600));
        check(w.skill_disabled_slot(source,{293,0},observed)==(dead_target?0:13100));
        check(w.settle_resources(source,observed+1));
        check(w.skill_disabled_slot(source,{293,0},observed+1)==(dead_target?0:13100));
    }
    // The ally is alive before dispatch,but dies when return preparation
    // settles its outstanding poison. Ending Recall must still complete.
    {
        world::Instance w;w.configure_navigation(mesh);join(w,1,0,1);join(w,2,600,1);join(w,3,100,2);
        w.set_vitals(2,{1,1000,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        skills::Slots bar{};bar[0]={925,0};bar[1]={293,0};
        check(w.configure_adrenaline(source,bar,100)&&w.stage_recall(source,target,100));
        const auto effect=w.status_snapshot(source,100)->front().instance;
        check(w.apply_condition(*w.effect_scope(3),target,combat::conditions::Kind::poison,100,10000));
        check(w.cancel_maintained_enchantment(source,effect,1100));
        check(w.resources(2)->health==0&&w.position(1,1100).first==0);
        check(w.status_snapshot(source,1100)->empty()&&w.skill_disabled_slot(source,{293,0},1100)==0);
        check(w.settle_resources(source,1200));
    }
    // Recall's tether belongs to the selected ally,not its self-owned effect.
    // Forecast the first outside millisecond independently of polling cadence.
    for(bool late:{false,true})for(bool moving_caster:{false,true}){
        const auto wide=std::filesystem::path(argv[1]).string()+".recall";fixture(wide,3);
        world::Instance w;w.configure_navigation(std::make_shared<const navigation::Mesh>(navigation::Mesh::load(wide)));
        join(w,1,0,1);join(w,2,600,1);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        skills::Slots bar{};bar[0]={925,0};bar[1]={293,0};
        check(w.configure_adrenaline(source,bar,100)&&w.stage_recall(source,target,100));
        w.move(moving_caster?1:2,7000,0,100);
        const auto a=w.maintained_range_path(source,100,25100),b=w.maintained_range_path(target,100,25100);
        if(!a||!b)throw std::runtime_error("Player Recall range paths player="+std::to_string(bool(a))+" npc="+std::to_string(bool(b))+" late="+std::to_string(late));
        const auto exit=skills::effects::maintained_range_exit(*a,*b,100,25100);
        check(exit&&exit->first_outside);const auto boundary=*exit->first_outside;
        check(w.settle_resources(source,boundary-1)&&w.status_snapshot(source,boundary-1)->size()==1);
        const auto observed=late?25100:boundary;
        check(w.settle_resources(source,observed));
        check(w.status_snapshot(source,observed)->empty()&&w.maintained_enchantment_count(source,observed)==0);
        if(moving_caster)check(w.position(1,observed).first==600);
        else check(w.position(1,observed).first>5000&&w.position(1,observed).first<5001);
        check(w.skill_disabled_slot(source,{293,0},observed)==boundary+10000);
    }
    for(bool late:{false,true}){
        const auto wide=std::filesystem::path(argv[1]).string()+".recall-npc";fixture(wide,3);
        world::Instance w;w.configure_navigation(std::make_shared<const navigation::Mesh>(navigation::Mesh::load(wide)));
        world::Instance::Npc n;n.id=n.definition.id=100;n.x=600;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;
        n.combat_definition=d;w.configure_npcs({n});join(w,1,0,1);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
        skills::Slots bar{};bar[0]={925,0};bar[1]={293,0};
        check(w.configure_adrenaline(source,bar,100)&&w.stage_recall(source,target,100));
        auto stale=target;++stale.generation;check(!w.maintained_range_path(stale,100,25100));
        check(!w.npc_energy_snapshot(target)&&w.settle_resources(source,100));
        w.move(1,7000,0,100);
        const auto a=w.maintained_range_path(source,100,25100),b=w.maintained_range_path(target,100,25100);
        if(!a||!b){
            const auto earliest=[&](skills::effects::Scope actor){
                std::uint64_t low=100,high=25100;
                while(low<high){const auto mid=low+(high-low)/2;
                    if(w.maintained_range_path(actor,mid,25100))high=mid;else low=mid+1;
                }
                return w.maintained_range_path(actor,low,25100)?std::to_string(low):std::string("none");
            };
            throw std::runtime_error("NPC Recall range paths player="+std::to_string(bool(a))+" npc="+std::to_string(bool(b))+
                " first_player="+earliest(source)+" first_npc="+earliest(target)+" late="+std::to_string(late));
        }
        const auto exit=skills::effects::maintained_range_exit(*a,*b,100,25100);
        if(!exit||!exit->first_outside)throw std::runtime_error("NPC Recall exit missing valid="+std::to_string(bool(exit))+
            " source_from="+std::to_string(a->front().x)+","+std::to_string(a->front().y)+
            " source_to="+std::to_string(a->back().x)+","+std::to_string(a->back().y)+
            " target_from="+std::to_string(b->front().x)+","+std::to_string(b->front().y)+
            " target_to="+std::to_string(b->back().x)+","+std::to_string(b->back().y));
        const auto at=*exit->first_outside;check(w.settle_resources(source,at-1)&&w.status_snapshot(source,at-1)->size()==1);
        const auto observed=late?25100:at;check(w.settle_resources(source,observed));
        check(w.status_snapshot(source,observed)->empty()&&w.position(1,observed).first==600);
        check(!w.npc_energy_snapshot(target));
        check(w.skill_disabled_slot(source,{293,0},observed)==at+10000);
    }
    for(unsigned mode=0;mode<4;++mode){
        world::Instance w;w.configure_navigation(mesh);join(w,1,0,1);join(w,2,600,1);
        const auto source=*w.effect_scope(1);auto target=*w.effect_scope(2);
        w.steer(1,1,0,200);w.steer(2,0,1,200);
        if(mode==1)++target.generation;
        if(mode==2)check(w.kill(*w.effect_scope(2),200));
        if(mode==3)check(w.kill(source,200));
        const skills::effects::RecallAnchor anchor{source,target,123,100};
        const auto before_preview=w.position(1,400);
        const auto preview=w.recall_step_preview(anchor,400);
        check(bool(preview)==(mode==0)&&w.position(1,400)==before_preview);
        const auto step=w.recall_step(anchor,400);check(bool(step)==(mode==0));
        if(step){check(step->position[0]==44&&step->healing_quote==0&&step->position==preview->position);
            const auto destination=step->destination;w.tick(900);
            check(w.position(1,900)==std::pair<float,float>{destination.point.x,destination.point.z});
        }
    }
    for(unsigned mode=0;mode<5;++mode){
        world::Instance w;w.configure_navigation(mesh);join(w,1,0,1);join(w,2,600,2);
        w.set_vitals(1,{mode==4?990u:500u,1000,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        if(mode==1||mode==3)check(w.status_mutation(source,129,901,0,100,10100,true));
        if(mode==2||mode==3)check(w.status_mutation(source,142,902,0,100,10100,true));
        std::vector<world::Packet> visuals;std::function<void()> publication;
        check(w.death_charge(source,target,plan,100,&visuals,&publication));
        const double multiplier=mode==1?.66:mode==2?.8:mode==3?.6:1.;
        const double expected=mode==4?1000:std::ceil(500+173*multiplier);
        check(w.resources(1)->health==expected);
        publication();
    }
    for(unsigned rank=0;rank<=20;++rank)for(unsigned health:{300u,500u,900u}){
        world::Instance w;w.configure_navigation(mesh);
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{},0,100,1);
        w.set_vitals(1,{500,1000,100,100},100);join(w,2,600,2);
        w.set_vitals(2,{health,1000,100,100},100);box->drain();
        std::vector<world::Packet> visuals;std::function<void()> publish;
        check(w.death_charge(*w.effect_scope(1),*w.effect_scope(2),*skills::effects::death_charge(rank),100,&visuals,&publish));
        check(w.resources(1)->health==500+(health>500?65+9*rank:0));
        check(visuals.size()==4&&visuals.back()[0]==44&&w.position(1,100).first==600);
        check(box->drain().empty());publish();const auto packets=box->drain();
        check((health>500)==!packets.empty());publish();check(box->drain().empty());
    }
    for(unsigned mode=0;mode<4;++mode){
        world::Instance w,reference;w.configure_navigation(mesh);reference.configure_navigation(mesh);
        for(auto* state:{&w,&reference}){
            join(*state,1,0,1);join(*state,2,600,2);
            if(mode==1)state->move(1,400,0,200);
            if(mode>=2){if(mode==3)state->rotate(1,INFINITY,1,200);state->steer(1,1,0,200);}
            state->steer(2,0,1,200);
        }
        reference.tick(400);const auto expected_origin=reference.position(1,400),expected_target=reference.position(2,400);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        const auto moved=w.death_charge_step(source,target,plan,400);check(bool(moved));
        check(!moved->clipped&&!moved->blocked&&moved->position.size()==16&&moved->position[0]==44);
        check(std::hypot(moved->origin.point.x-expected_origin.first,moved->origin.point.z-expected_origin.second)<.001);
        check(std::hypot(moved->destination.point.x-expected_target.first,moved->destination.point.z-expected_target.second)<.001);
        // Sampling a foe doesn't materialize or otherwise alter their input.
        w.tick(900);const auto stationary=w.position(1,900);check(std::hypot(stationary.first-expected_target.first,stationary.second-expected_target.second)<.001);
        reference.tick(900);const auto actual_foe=w.position(2,900),expected_foe=reference.position(2,900);
        check(std::hypot(actual_foe.first-expected_foe.first,actual_foe.second-expected_foe.second)<.001);
        const auto old_report=world::Instance::SteeringReport{0,0,0};
        check(w.steer(1,0,0,900,1,old_report).status==world::Instance::CheckpointStatus::baseline);
        check(w.position(1,900)==stationary);
        check(w.steer(1,1,0,900,1,old_report).status==world::Instance::CheckpointStatus::baseline);
        check(w.steer(1,1,0,901,1,old_report).status==world::Instance::CheckpointStatus::over_budget);
        check(std::hypot(w.position(1,901).first-stationary.first,w.position(1,901).second-stationary.second)<1);
    }
    {
        world::Instance w;w.configure_navigation(mesh);join(w,1,0,1);join(w,2,600,2,1);
        const auto moved=w.death_charge_step(*w.effect_scope(1),*w.effect_scope(2),plan,400);
        check(moved&&moved->clipped&&!moved->blocked&&moved->destination.surface.plane==0&&w.plane(1)==0);
    }
    {
        const auto path=std::filesystem::path(std::string(argv[1])+".wall");fixture(path,1);
        world::Instance w;w.configure_navigation(std::make_shared<const navigation::Mesh>(navigation::Mesh::load(path)));
        join(w,1,0,1);join(w,2,600,2);
        const auto moved=w.death_charge_step(*w.effect_scope(1),*w.effect_scope(2),plan,400);
        check(moved&&moved->clipped&&!moved->blocked&&moved->destination.point.x<=200&&moved->destination.point.x>199.99f);
        w.tick(900);check(w.position(1,900).first==moved->destination.point.x);
    }
    {
        const auto path=std::filesystem::path(std::string(argv[1])+".bridge");fixture(path,2);
        const auto linked=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(path));
        for(bool shut:{false,true}){
            world::Instance w;w.configure_navigation(linked);join(w,1,1800,1);join(w,2,2200,2,1);
            const std::array<navigation::SurfaceLink,1> closed{{{{0,0},{1,0}}}};
            const auto moved=w.death_charge_step(*w.effect_scope(1),*w.effect_scope(2),plan,400,
                shut?std::span<const navigation::SurfaceLink>(closed):std::span<const navigation::SurfaceLink>{});
            check(bool(moved));
            if(shut)check(moved->clipped&&w.plane(1)==0&&moved->destination.point.x<=2000);
            else check(!moved->clipped&&w.plane(1)==1&&moved->destination.point.x==2200&&moved->position[14]==1);
        }
    }
    {
        world::Instance w;w.configure_navigation(mesh);join(w,1,0,1);join(w,2,1500,2);join(w,3,600,1);
        auto source=*w.effect_scope(1);const auto target=*w.effect_scope(2);
        check(!w.death_charge_step(source,target,plan,400)); // Spell range, not compass range.
        check(!w.death_charge_step(source,*w.effect_scope(3),plan,400));
        auto forged=plan;++forged.range;check(!w.death_charge_step(source,target,forged,400));
        ++source.generation;check(!w.death_charge_step(source,target,plan,400));
        check(w.position(1,400)==std::pair<float,float>{0.f,0.f});
    }
    {
        world::Instance w;w.configure_navigation(mesh);join(w,1,0,1);join(w,2,600,2);
        const auto owner=*w.effect_scope(1),old=*w.effect_scope(2);w.leave(2);join(w,2,600,2);
        check(!w.death_charge_step(owner,old,plan,400));
        w.steer(1,1,0,500);check(!w.death_charge_step(owner,*w.effect_scope(2),plan,400));
    }
    {
        world::Instance w;join(w,1,0,1);join(w,2,600,2);
        check(!w.death_charge_step(*w.effect_scope(1),*w.effect_scope(2),plan,400));
    }
    {
        world::Instance w;w.configure_navigation(mesh);
        world::Instance::Npc n;n.id=n.definition.id=100;n.x=600;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;
        n.combat_definition=d;w.configure_npcs({n});join(w,1,0,1);
        const auto moved=w.death_charge_step(*w.effect_scope(1),*w.effect_scope(100),plan,400);
        check(moved&&moved->destination.point.x==600&&!moved->clipped);
        world::Instance allied;allied.configure_navigation(mesh);
        n.combat_definition->relation=combat::npcs::Relation::allied_to_players;
        allied.configure_npcs({n});join(allied,1,0,1);
        const auto returned=allied.recall_step({*allied.effect_scope(1),*allied.effect_scope(100),123,100},400);
        check(returned&&returned->destination.point.x==600&&!returned->clipped);
    }
    std::cout<<"PASS World shadow relocation:click/held/rotating movement,moving foe,NPC,floor fallback,scope/range/plan/clock guards,prediction reset\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}










