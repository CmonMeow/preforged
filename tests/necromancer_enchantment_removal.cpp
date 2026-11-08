#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Necromancer removal regression line "+std::to_string(at.line()));
}
int main()try{
    using namespace skills;using namespace effects;
    check(enchantment_removal(141,0)->count==5&&enchantment_removal(141,12)->count==8&&enchantment_removal(141,20)->count==10);
    check(enchantment_removal(141,0)->monk_loss==55&&enchantment_removal(141,12)->monk_loss==31&&enchantment_removal(141,20)->monk_loss==15);
    check(enchantment_removal(143,0)->count==0&&enchantment_removal(143,4)->count==1&&enchantment_removal(143,12)->count==2&&enchantment_removal(143,20)->count==3);
    check(enchantment_removal(143,12)->steal==53&&!life_steal(143)); // No unconditional life-steal dispatcher bypass.
    for(unsigned rank:{0u,12u,20u})for(bool lethal:{false,true}){
        world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1&&lethal?50u:500u,1000,100,100},100);}
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);unsigned effect=1;
        // Armor of Earth owns both its timer and movement interval; raw status
        // insertion is intentionally rejected. Preserve application order for
        // the newest-enchantment removal and Monk-enchantment sacrifice test.
        for(unsigned skill:{241u,242u,243u,244u,160u,164u,165u,166u,168u}){
            if(skill==165)check(w.armor_earth(b,12,100));
            else check(w.status_mutation(b,skill,effect++,12,100,60100,true));
        }
        const auto epoch=*w.control_epoch(a);const auto plan=*enchantment_removal(141,rank);
        auto forged=plan;++forged.count;check(!w.enchantment_removal(a,b,forged,20,100));
        check(w.enchantment_removal(a,b,plan,20,100));
        const auto removed=std::min(9u,plan.count),monks=removed>5?removed-5:0;
        const auto loss=monks*plan.monk_loss,health=lethal?50u:500u;
        check(w.resources(1)->health==(loss>=health?0u:health-loss));
        check(w.status_snapshot(b,100)->size()==9-removed&&w.resources(2)->health==500);
        check(*w.control_epoch(a)==epoch+(loss>=health?1u:0u));
    }
    for(unsigned rank:{0u,4u,12u,20u}){
        world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,1000,100,100},100);}
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);unsigned effect=1;
        for(unsigned skill:{241u,180u,242u,169u})check(w.status_mutation(b,skill,effect++,12,100,60100,true));
        const auto plan=*enchantment_removal(143,rank);check(w.enchantment_removal(a,b,plan,20,100));
        const auto amount=plan.count?plan.steal:0;
        check(w.resources(1)->health==500+amount&&w.resources(2)->health==500-amount);
        check(w.status_snapshot(b,100)->size()==4-plan.count);
    }
    // Remaining Shielding Hands reduces life stealing; removing it first
    // instead triggers its end heal and leaves the full transfer afterward.
    for(unsigned rank:{4u,12u}){
        world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,1000,100,100},100);}
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
        check(w.protective_enchantment(b,b,*regeneration::reviewed(299,12),0,100));
        check(w.status_mutation(b,180,1,12,100,60100,true));
        check(w.enchantment_removal(a,b,*enchantment_removal(143,rank),20,100));
        check(w.resources(1)->health==(rank==4?506u:553u)&&w.resources(2)->health==(rank==4?494u:488u));
    }
    for(bool wounded:{false,true}){
        world::Instance w;auto out=std::make_shared<world::Outbox>();for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:100,0,id==1?out:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1&&wounded?50u:500u,1000,100,100},100);}
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
        check(w.degeneration_hex(b,a,*degeneration_hex(129,12),100));
        if(wounded)check(w.apply_condition(b,a,combat::conditions::Kind::deep_wound,100,60000));
        check(w.status_mutation(b,180,1,12,100,60100,true));out->drain();
        check(w.enchantment_removal(a,b,*enchantment_removal(143,12),20,100));
        check(w.resources(1)->health==(wounded?3u:553u));
        const auto packets=out->drain();const auto hp=w.resources(1);
        check(std::find(packets.begin(),packets.end(),combat::detail::fraction(1,combat::Health{double(hp->health),hp->max_health}))!=packets.end());
    }
    {
        world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1?500u:5u,1000,100,100},100);}
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.status_mutation(b,180,1,12,100,60100,true));
        check(w.enchantment_removal(a,b,*enchantment_removal(143,12),20,100));check(w.resources(1)->health==506&&w.resources(2)->health==0);
    }
    const std::string hash(64,'a');std::istringstream p("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
    std::istringstream m("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/necromancer_enchantment_removal.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 1000 1000 80 1 1 test test test - test 60 60 test test\n");
    auto population=world::load_npc_catalog(p);world::bind_combat_catalog(population,combat::npcs::load_catalog(m,hash),hash);
    for(unsigned skill:{141u,143u})for(unsigned mode:{0u,1u,2u,3u,4u,5u,6u,7u}){
        if((mode==6&&skill!=143)||(mode==7&&skill!=141))continue;
        using namespace casting;world::Instance w;auto box=std::make_shared<world::Outbox>();w.configure_npcs(population.for_map(146));
        for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1&&mode==7?50u:500u,1000,100,100},100);}
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.energy_capacity(1,4,0,100,{80,-2}));
        unsigned effect=1;if(mode!=4)for(unsigned enchant:{241u,180u,242u,169u})check(w.status_mutation(b,enchant,effect++,12,100,60100,true));
        std::uint64_t now=100;Hooks h;std::vector<Packet> sent;
        h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};h.resources=[&](auto id){return w.resources(id);};
        h.consume_energy=[&](auto n){return w.consume_energy(1,n);};h.target_scope=[&](auto id){return w.effect_scope(id);};
        h.projectile_target_valid=[&](auto id,float range){return w.projectile_target_valid(a,id,range,now);};
        h.enchantment_removal=[&](auto target,const auto& plan,auto at){return w.enchantment_removal(a,target,plan,20,at);};
        Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[skill]=1;Slots slots{};slots[0]={skill,0};attributes::Ranks ranks{};ranks[4]=ranks[7]=12;Runtime runtime(1);
        if(mode==6)ranks[4]=0;
        check(runtime.start(now,{skill,0,mode==5?100u:2u},slots,policy,ranks,true,h)==casting::Reason::accepted);
        if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(500,h);
        if(mode==3){w.leave(2);w.join(2,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,1000,100,100},100);}
        now=100+static_cast<std::uint64_t>(find_skill(skill)->activation*1000);runtime.update(now,h);
        check(w.resources(1)->health==(mode==7?0u:!mode?(skill==141?438u:553u):500u));
        check(w.resources(2)->health==(!mode&&skill==143?447u:500u));
        check(w.resources(1)->energy==100-find_skill(skill)->energy_cost());
        if(!mode||mode>=4)check(std::find(sent.begin(),sent.end(),slot_event(228,1,{skill,0},20))!=sent.end());
        if(mode==6)check(w.status_snapshot(b,now)->size()==4);
        if(mode==7)check(w.status_snapshot(b,now)->empty());
        if(!mode){auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,skill))==1);}
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
