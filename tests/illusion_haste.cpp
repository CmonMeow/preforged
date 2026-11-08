#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool v,std::source_location at=std::source_location::current()){
    if(!v)throw std::runtime_error("Illusion of Haste regression line "+std::to_string(at.line()));
}
void setup(world::Instance& w){
    w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
    w.set_vitals(1,{1000,1000,40,100},100);w.set_vitals(2,{1000,1000,40,100},100);
}
int main()try{
    using namespace skills;using namespace effects;using combat::conditions::Kind;
    check(illusion_haste(37,0)->duration==5000&&illusion_haste(37,12)->duration==10000&&illusion_haste(37,20)->duration==13000);
    check(illusion_haste(37,12,true)->skill==3373&&!illusion_haste(37,12,true)->cleanse&&illusion_haste(37,12)->cleanse);
    check(!illusion_haste(38)&&!illusion_haste(37,21));
    {
        MovementEffects m;m.apply({1,1,37,0,Category::enchantment,100,5100});
        check(std::abs(m.integral(100,10100)-10150)<.01&&m.at(5100)==.5f&&m.at(8100)==1);
        m.apply({2,1,349,0,Category::stance,100,10100});check(std::abs(m.at(100)-1.34f)<1e-5);
        combat::conditions::State c;check(c.apply(Kind::crippled,1,100,20000));m.crippled(c,100);
        check(std::abs(m.at(6100)-.625f)<1e-5); // Real+forecast Crippled do not stack twice.
        m.apply({3,1,1043,0,Category::stance,100,3100});check(m.at(100)<=1.5f);
        MovementEffects dash;dash.apply({1,1,1043,0,Category::stance,100,3100});check(dash.at(100)==1.5f);
    }
    for(unsigned skill:{37u,3373u}){
        world::Instance w;setup(w);const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
        check(w.apply_condition(b,a,Kind::crippled,100,20000));
        check(w.status_mutation(a,skill,1,0,100,5100,true));
        check(w.has_condition(a,Kind::crippled,100)==(skill==3373));
        auto stale=a;++stale.generation;check(!w.status_mutation(stale,skill,2,0,100,5100,true));
        check(!w.status_mutation(a,skill,2,0,100,5101,true));
    }
    for(unsigned skill:{37u,3373u}){
        world::Instance w;setup(w);const auto a=*w.effect_scope(1);
        check(w.status_mutation(a,skill,1,0,100,5100,true));
        check(w.status_mutation(a,skill,2,0,1100,6100,true));
        check(w.status_snapshot(a,1100)->size()==1&&w.has_condition(a,Kind::crippled,1100)==(skill==3373));
        w.tick(4100);check(!w.has_condition(a,Kind::crippled,4100));
        w.tick(6100);check(w.has_condition(a,Kind::crippled,6100)&&w.status_snapshot(a,6100)->empty());
        w.tick(9100);check(!w.has_condition(a,Kind::crippled,9100));
        check(w.status_mutation(a,skill,2,0,9200,{},false)&&!w.has_condition(a,Kind::crippled,9200));
    }
    for(unsigned skill:{37u,3373u})for(unsigned removal:{68u,69u,141u,143u}){
        world::Instance w;setup(w);const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
        check(w.status_mutation(a,skill,1,0,100,5100,true));
        check(w.enchantment_removal(b,a,*enchantment_removal(removal,12),20,200));
        check(w.has_condition(a,Kind::crippled,200)&&w.status_snapshot(a,200)->empty());
        w.move(1,10000,0,200);
        w.tick(5100);check(!w.has_condition(a,Kind::crippled,5100)); // No old forecast becomes a new condition.
        w.tick(6100);check(std::abs(w.position(1,6100).first-1267.2f)<1);
    }
    for(unsigned skill:{37u,3373u}){
        world::Instance w;setup(w);const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
        check(w.degeneration_hex(b,a,*degeneration_hex(19,12),100));
        check(w.status_mutation(a,skill,1,0,100,5100,true)&&w.status_mutation(a,skill,2,0,1100,6100,true));
        check(w.resources(1)->health==(skill==37?966u:983u));
        w.tick(4100);check(w.resources(1)->health==966);
    }
    {
        world::Instance w;setup(w);const auto a=*w.effect_scope(1),b=*w.effect_scope(2);w.set_vitals(1,{10,1000,40,100},100);
        check(w.degeneration_hex(b,a,*degeneration_hex(19,12),100));check(w.status_mutation(a,37,1,0,100,5100,true));
        const auto epoch=*w.control_epoch(a);w.tick(5100);
        check(w.resources(1)->health==0&&!w.has_condition(a,Kind::crippled,5100)&&*w.control_epoch(a)==epoch+1);
        w.tick(6100);check(*w.control_epoch(a)==epoch+1&&!w.has_condition(a,Kind::crippled,6100));
    }
    {
        world::Instance w;setup(w);const auto a=*w.effect_scope(1);check(w.status_mutation(a,37,1,0,100,5100,true));
        w.move(1,10000,0,100);w.tick(10100);check(std::abs(w.position(1,10100).first-2923.2f)<1);
        check(!w.has_condition(a,Kind::crippled,10100));
    }
    for(unsigned skill:{37u,3373u}){
        world::Instance w;setup(w);const auto a=*w.effect_scope(1);
        check(w.status_mutation(a,skill,1,0,100,5100,true)&&w.status_mutation(a,skill,2,0,1100,6100,true));
        w.move(1,10000,0,1100);w.tick(11100);
        check(std::abs(w.position(1,11100).first-(skill==37?2923.2f:2348.64f))<1);
    }
    for(bool pvp:{false,true})for(unsigned mode:{0u,1u,2u,3u}){
        using namespace casting;world::Instance w;setup(w);const auto a=*w.effect_scope(1);std::vector<Packet> sent;Hooks h;
        h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};h.resources=[&](auto id){return w.resources(id);};
        h.consume_energy=[&](auto n){return w.consume_energy(1,n);};h.target_valid=[](auto id,float,bool){return id==1;};
        h.actor_generation=[&]()->std::optional<std::uint64_t>{const auto scope=w.effect_scope(1);return scope?std::optional(scope->generation):std::nullopt;};
        h.status_mutation=[&](auto skill,auto effect,auto rank,auto at,auto ends,bool active){return w.status_mutation(a,skill,effect,rank,at,ends,active);};
        h.effect_snapshot=[&](auto at){return w.status_snapshot(a,at);};
        Policy policy;policy.primary=5;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[37]=1;Slots bar{};bar[0]={37,0};attributes::Ranks ranks{};ranks[1]=12;Runtime runtime(1);
        check(runtime.start(100,{37,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(w.resources(1)->energy==35);
        if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(500,h);
        if(mode==3){w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,40,100},100);}
        runtime.update(1100,h);
        if(mode){check(w.status_snapshot(*w.effect_scope(1),1100)->empty()&&!w.has_condition(*w.effect_scope(1),Kind::crippled,1100));continue;}
        auto entry=w.status_snapshot(a,1100)->front();check(entry.skill==(pvp?3373u:37u)&&entry.expires==11100);
        check(std::find(sent.begin(),sent.end(),slot_event(228,1,{37,0},pvp?8:5))!=sent.end());
        runtime.update(11100,h);check(w.has_condition(a,Kind::crippled,11100));
        runtime.update(11200,h);w.tick(14100);check(!w.has_condition(a,Kind::crippled,14100));
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
