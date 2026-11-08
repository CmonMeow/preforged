#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){if(!value)throw std::runtime_error("Divine Spirit World line "+std::to_string(at.line()));}
int main()try{
    for(bool area:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        for(unsigned id:{1u,2u}){w.join(id,"Actor",float(id*100),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,1000,50,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
        const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);const auto plan=*divine_spirit(12);
        inventory::Profile inventory;inventory.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item item;item.definition.id=100;item.definition.type=26;item.definition.quantity=1;item.definition.modifiers={0x22b80014};item.bag=1;inventory.items.emplace(100,item);inventory.weapons.sets[0].weapon=100;
        const auto equipment=items::weapon_passives::equipped_enchanting(inventory,1);check(equipment.has_value());const auto active=*w.status_snapshot(owner,1000);
        const auto quote=enchanting_duration(310,plan.duration,owner,*equipment,active,1000);check(quote.has_value());
        check(w.complete_divine_spirit(owner,plan,0,1000,&*quote));const auto effect=w.status_snapshot(owner,1000)->front();check(effect.expires==14000);
        check(!w.complete_divine_spirit(owner,plan,0,1001,&*quote));check(w.divine_spirit_spell_cost(owner,290,10,1001)==5);box->drain();
        if(area)check(w.complete_chilblains(foe,owner,*chilblains(12),20,1001));else check(w.enchantment_removal(foe,owner,*enchantment_removal(69,12),20,1001));
        check(w.status_snapshot(owner,1001)->empty()&&w.divine_spirit_spell_cost(owner,290,10,1001)==10);
        const auto packets=box->drain();combat::Packet end{67,0};combat::detail::put(end,1);combat::detail::put(end,effect.instance);check(std::count(packets.begin(),packets.end(),end)==1);
        auto stale=owner;++stale.generation;check(!w.divine_spirit_spell_cost(stale,290,10,1001));check(!w.divine_spirit_spell_cost(owner,999999,10,1001));
    }
    for(unsigned rank=0;rank<=20;++rank)for(unsigned mode=0;mode<5;++mode){
        const unsigned divine=mode==4?0:12;world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{world::Packet{165,0,1,0,0,0,static_cast<std::uint8_t>(mode==4?6:3),0}}, {},100,1);
        w.set_vitals(1,{500,1000,50,100},100);const auto owner=*w.effect_scope(1);auto source=owner;auto plan=*divine_spirit(rank);
        if(mode==1)++source.generation;if(mode==2)++plan.reduction;box->drain();
        const bool applied=w.complete_divine_spirit(source,plan,divine,1000);check(applied==(mode!=1&&mode!=2));
        if(!applied){check(w.status_snapshot(owner,1000)->empty()&&w.resources(1)->health==500);continue;}
        const auto effects=*w.status_snapshot(owner,1000);check(effects.size()==1&&effects.front().skill==310&&effects.front().expires==1000+plan.duration);
        check(w.resources(1)->health==(divine?538u:500u));const auto packets=box->drain();
        const auto finish=combat::detail::integer(58,1,0),body=combat::detail::integer(21,1,553);
        check(std::count(packets.begin(),packets.end(),finish)==1&&std::count(packets.begin(),packets.end(),body)==1);
        check(w.divine_spirit_spell_cost(owner,290,10,1000)==5&&w.divine_spirit_spell_cost(owner,174,15,1000)==15&&w.divine_spirit_spell_cost(owner,297,0,1000)==0);
        auto effect=effects.front().instance;auto expiry=*effects.front().expires;
        if(mode==3){check(w.complete_divine_spirit(owner,*divine_spirit(rank),divine,1001));const auto replaced=*w.status_snapshot(owner,1001);check(replaced.size()==1&&replaced.front().instance!=effect);const auto ended=combat::Packet{67,0,1,0,0,0,static_cast<std::uint8_t>(effect),static_cast<std::uint8_t>(effect>>8),static_cast<std::uint8_t>(effect>>16),static_cast<std::uint8_t>(effect>>24)};const auto replacement=box->drain();check(std::count(replacement.begin(),replacement.end(),ended)==1);expiry=*replaced.front().expires;}
        check(w.divine_spirit_spell_cost(owner,290,10,expiry-1)==5);check(w.divine_spirit_spell_cost(owner,290,10,expiry)==10&&w.status_snapshot(owner,expiry)->empty());
    }
    for(bool enabled:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Monk",0,0,box,0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);
        w.set_vitals(1,{500,1000,100,100},100);
        const auto owner=*w.effect_scope(1);
        skills::Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[310]=1;
        skills::Slots bar{};bar[0]={310,0};attributes::Ranks ranks{};ranks[16]=12;
        skills::casting::Hooks hooks;
        hooks.events=[&](const auto& batch){box->enqueue(batch);};
        hooks.resources=[&](auto id){return w.resources(id);};
        hooks.target_scope=[&](auto id){return w.effect_scope(id);};
        hooks.target_valid=[&](auto id,float,bool){return id==1;};
        hooks.consume_energy_at=[&](unsigned cost,auto now){return w.consume_energy(1,cost,owner,now);};
        unsigned completions=0;
        if(enabled)hooks.divine_spirit=[&](const DivineSpiritPlan& plan,unsigned divine,auto now){
            ++completions;
            return skills::casting::EffectCompletion{
                w.complete_divine_spirit(owner,plan,divine,now),{},{},{},false,true};
        };
        skills::casting::Runtime runtime(1);
        check(runtime.start(1000,{310,0,1},bar,policy,ranks,true,hooks)==
              (enabled?skills::casting::Reason::accepted:skills::casting::Reason::unsupported));
        if(!enabled){check(w.resources(1)->energy==100&&w.status_snapshot(owner,1000)->empty());continue;}
        box->drain();runtime.update(1249,hooks);
        check(completions==0&&w.resources(1)->energy==90&&w.status_snapshot(owner,1249)->empty());
        runtime.update(1250,hooks);
        check(completions==1&&w.status_snapshot(owner,1250)->size()==1);
        const auto packets=box->drain();
        check(std::count(packets.begin(),packets.end(),combat::detail::integer(58,1,0))==1);
        check(std::count(packets.begin(),packets.end(),combat::detail::integer(21,1,553))==1);
    }
    std::cout<<"PASS105 Divine Spirit World rank/self/DF/secondary/replacement/expiry/scope cases;normal paid casting and cost ordering pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
