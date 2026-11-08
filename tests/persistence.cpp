#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include "gw_hex_duration.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Persistence regression line "+std::to_string(at.line()));
}
int main()try{
    using namespace skills;using namespace effects;
    const Scope source{1,1,1};
    for(unsigned rank:{0u,12u,20u}){
        std::vector<Entry> active{{1,1,14,rank,Category::stance,100,32100}};
        auto duration=hex_duration(43,4000,source,active,200);
        check(duration&&duration->for_hex(43,4000,1)==4000*(110+2*rank)/100);
        check(!duration->for_hex(47,4000,1)&&!duration->for_hex(43,5000,1)&&!duration->for_hex(43,4000,2));
        check(hex_duration(35,10000,source,active,200)->for_hex(35,10000,1)==10000); // Domination.
        check(hex_duration(66,10000,source,active,200)->for_hex(66,10000,1)==10000); // Inspiration.
        check(hex_duration(43,4000,source,active,99)->for_hex(43,4000,1)==4000);
        check(hex_duration(43,4000,source,active,32100)->for_hex(43,4000,1)==4000);
        active.push_back(active[0]);check(hex_duration(43,4000,source,active,200)->for_hex(43,4000,1)==duration->for_hex(43,4000,1));
        active[0].cause=active[1].cause=2;check(hex_duration(43,4000,source,active,200)->for_hex(43,4000,1)==4000);
    }
    check(!hex_duration(281,1000,source,{},100)&&!hex_duration(43,0,source,{},100));
    check(!hex_duration(43,4000,{}, {},100)&&!hex_duration(43,UINT64_MAX,source,{},100));
    check(!hex_duration(43,4000,source,{},UINT64_MAX-3999));
    for(auto kind:{Category::enchantment,Category::hex,Category::other}){
        std::vector<Entry> invalid{{1,1,14,12,kind,100,21100}};
        check(hex_duration(43,4000,source,invalid,200)->for_hex(43,4000,1)==4000);
    }
    for(auto entry:{Entry{1,1,14,21,Category::stance,100,21100},Entry{0,1,14,12,Category::stance,100,21100},Entry{1,1,14,12,Category::stance,100,{}}}){
        const std::vector<Entry> invalid{entry};check(hex_duration(43,4000,source,invalid,200)->for_hex(43,4000,1)==4000);
    }
    check(timing::stance(14,0)->duration==5000&&timing::stance(14,12)->duration==21000&&timing::stance(14,20)->duration==32000);

    const std::string hash(64,'a');
    std::istringstream p("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
    std::istringstream m("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/persistence.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 10000 10000 80 1 1 test test test - test 60 60 test test\n");
    auto catalog=world::load_npc_catalog(p);world::bind_combat_catalog(catalog,combat::npcs::load_catalog(m,hash),hash);
    const auto definition=*catalog.for_map(146).front().combat_definition;
    const std::vector<Entry> stance{{1,1,14,12,Category::stance,100,21100}};
    // Every NPC hex ledger keeps the adjusted expiry, while its canonical
    // plan remains unchanged. Tokens cannot be reused for another skill/cause.
    for(unsigned id:{31u,53u,56u,36u,43u,47u,19u,76u}){
        combat::npcs::LiveActor npc(definition,{1,1,1,146,100});const auto life=npc.lifetime();
        Time base=0;
        if(auto plan=movement_hex(id,12)){base=plan->duration;auto d=hex_duration(id,base,source,stance,200);check(npc.apply_movement_hex(life,*plan,1,1,200,d));}
        else if(auto plan=attack_hex(id,12)){base=plan->duration;auto d=hex_duration(id,base,source,stance,200);check(bool(npc.apply_attack_hex(life,*plan,1,20,1,200,d)));}
        else if(auto plan=fragility(id,12)){base=plan->duration;auto d=hex_duration(id,base,source,stance,200);check(bool(npc.apply_fragility(life,*plan,1,20,1,200,d)));}
        else {auto hex=*degeneration_hex(id,12);base=hex.duration;auto d=hex_duration(id,base,source,stance,200);check(bool(npc.apply_single_degeneration_hex(life,hex,1,1,200,d)));}
        const auto end=200+(base*134+50)/100;
        check(npc.mesmer_hex(life,200)->expires==end&&npc.mesmer_hex(life,200+base));
        check(!npc.mesmer_hex(life,end));
    }
    // Mixed-recipient world commits pass the same adjusted deadline into
    // generic NPC hexes, Fragility and one-shot attack reaction ledgers.
    for(unsigned id:{43u,47u,19u,56u,36u})for(bool expired:{false,true}){
        world::Instance w;w.configure_npcs(catalog.for_map(146));
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{10000,10000,100,100},100);
        const auto a=*w.effect_scope(1),b=*w.effect_scope(100);
        check(w.status_mutation(a,14,1,12,100,21100,true));const auto plan=*degeneration_hex(id,12);
        check(w.degeneration_hex(a,b,plan,200));
        const auto now=expired?200+(plan.duration*134+50)/100:201+plan.duration;
        check(w.shatter_delusions(a,b,*shatter_delusions(0),20,now));
        const auto health=w.npc_combat_snapshot(b)->health->current;
        check(expired?health==10000:health<10000);
    }
    for(unsigned id:{31u,53u,56u,36u,43u,47u,19u,76u,44u,35u,66u}){
        world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{10000,10000,100,100},100);w.set_vitals(2,{10000,10000,100,100},100);
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.status_mutation(a,14,1,12,100,21100,true));
        Time base=0;
        if(auto plan=movement_hex(id,12)){base=plan->duration;check(w.movement_hex(a,b,*plan,200));}
        else if(id==44){auto plan=*wound_spell(id,12);base=plan.hex_duration;check(w.wound_spell(a,b,plan,20,200));}
        else {auto plan=*degeneration_hex(id,12);base=plan.duration;check(w.degeneration_hex(a,b,plan,200));}
        const auto end=200+(find_skill(id)->attribute==1?(base*134+50)/100:base);
        auto entries=*w.status_snapshot(b,200);check(entries.size()==1&&entries[0].expires==end);
        check(w.status_mutation(a,14,1,12,300,{},false)); // Removal never shortens an already cast hex.
        check(w.status_snapshot(b,300)->front().expires==end);
        w.tick(end);check(w.status_snapshot(b,end)->empty());
        if(id==44)check(w.has_condition(b,combat::conditions::Kind::deep_wound,end));
    }
    // Actual instant stance application uses ordinary replacement, Energy and
    // native48 UI messages; it never fabricates a spell casting animation.
    {
        using namespace casting;world::Instance w;w.configure_npcs(catalog.for_map(146));
        w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{10000,10000,100,100},100);
        const auto a=*w.effect_scope(1),b=*w.effect_scope(100);std::vector<Packet> sent;Hooks h;
        h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};
        h.resources=[&](auto id){return w.resources(id);};h.consume_energy=[&](auto n){return w.consume_energy(1,n);};
        h.target_valid=[](auto id,float,bool){return id==1;};
        h.target_scope=[&](auto id){return w.effect_scope(id);};
        h.projectile_target_valid=[&](auto id,float range){return w.projectile_target_valid(a,id,range,100);};
        h.degeneration_hex=[&](auto target,const auto& plan,auto now){return w.degeneration_hex(a,target,plan,now);};
        h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto ends,bool active){return w.status_mutation(a,skill,effect,rank,now,ends,active);};
        h.effect_snapshot=[&](auto now){return w.status_snapshot(a,now);};
        Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[14]=policy.learned[16]=policy.learned[31]=1;
        Slots bar{};bar[0]={14,0};bar[1]={16,0};bar[2]={31,0};attributes::Ranks ranks{};ranks[3]=ranks[1]=12;Runtime runtime(1);
        check(runtime.start(100,{14,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);runtime.update(100,h);
        check(w.resources(1)->energy==95&&w.status_snapshot(a,100)->front().expires==21100);
        check(std::count(sent.begin(),sent.end(),generic(48,1,14))==1);
        check(std::count(sent.begin(),sent.end(),activation(1,1,14,false))==0);
        check(w.degeneration_hex(a,b,*degeneration_hex(53,12),200));
        check(runtime.start(300,{16,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);runtime.update(300,h);
        check(w.status_snapshot(a,300)->front().skill==16);
        check(w.activation_multiplier(b,281,17201)==2.); // Original17s lifetime has elapsed.
        const auto end=200+22780;check(w.activation_multiplier(b,281,end)==1.);
    }
    // Application-time semantics: an instant stance activated during a spell
    // affects that spell; a stance expiring during activation does not.
    for(bool expire:{false,true}){
        using namespace casting;world::Instance w;
        w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{10000,10000,100,100},100);w.set_vitals(2,{10000,10000,100,100},100);
        const auto a=*w.effect_scope(1);Hooks h;std::uint64_t now=100;
        h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};
        h.consume_energy=[&](auto n){return w.consume_energy(1,n);};h.target_scope=[&](auto id){return w.effect_scope(id);};
        h.target_valid=[](auto id,float,bool){return id==1;};
        h.projectile_target_valid=[&](auto id,float range){return w.projectile_target_valid(a,id,range,now);};
        h.degeneration_hex=[&](auto target,const auto& plan,auto at){return w.degeneration_hex(a,target,plan,at);};
        h.status_mutation=[&](auto skill,auto effect,auto rank,auto at,auto ends,bool active){return w.status_mutation(a,skill,effect,rank,at,ends,active);};
        h.effect_snapshot=[&](auto at){return w.status_snapshot(a,at);};
        Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[14]=policy.learned[31]=1;
        Slots bar{};bar[0]={14,0};bar[1]={31,0};attributes::Ranks ranks{};ranks[1]=12;Runtime runtime(1);
        if(expire){check(runtime.start(now,{14,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);runtime.update(now,h);now=5000;}
        check(runtime.start(now,{31,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);
        const auto end=now+static_cast<Time>(find_skill(31)->activation*1000);
        if(!expire){now=200;check(runtime.start(now,{14,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);}
        now=end;runtime.update(now,h);
        auto entries=*w.status_snapshot(*w.effect_scope(2),now);check(entries.size()==1);
        const auto base=degeneration_hex(31,12)->duration;
        check(entries.front().expires==now+(expire?base:base*110/100));
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
