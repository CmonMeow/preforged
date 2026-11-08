#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
int main()try{
    using namespace fw::gw;using namespace world;using namespace skills;using namespace casting;
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    for(unsigned scenario=0;scenario<3;++scenario){
        Instance w;w.join(1,"Monk",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.join(2,"Ally",100,0,std::make_shared<Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{100,200,40,40},100);w.set_vitals(2,{50,200,40,40},100);check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100),"healing party fixture");const auto source=*w.effect_scope(1),original=*w.effect_scope(2);
        const auto replace=[&](){w.leave(2);w.join(2,"Replacement",100,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.set_vitals(2,{50,200,40,40},100);check(w.party_action(1,158,2,1100)&&w.party_action(2,154,1,1100),"replacement party fixture");};
        Hooks h;unsigned commits=0,legacy=0;h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.consume_energy=[](auto){return true;};
        h.target_valid=[&](auto id,float range,bool self){return w.valid_heal_target(1,id,range,self);};h.target_scope=[&](auto id){return w.effect_scope(id);};
        h.heal=[&](auto,auto,float,bool){++legacy;return true;};h.heal_scoped=[&](auto target,unsigned amount,float range,bool self,auto now){++commits;check(target==original,"original target scope not retained");if(scenario==2)replace();return w.heal_target_scoped(source,target,amount,range,self,now);};
        Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned[281]=1;Slots bar{};bar[0]={281,0};attributes::Ranks ranks{};
        Runtime cast(1);check(cast.start(100,{281,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted,"scoped healing start");if(scenario==1)replace();cast.update(1100,h);
        check(!legacy&&commits==(scenario==1?0u:1u)&&w.resources(2)->health==(scenario==0?70u:50u),"target reuse healed replacement or legacy fallback");
        auto stale=source;++stale.generation;check(!w.heal_target_scoped(stale,*w.effect_scope(2),20,1248,true,1100),"stale caster healed ally");
        stale=*w.effect_scope(2);++stale.instance;check(!w.heal_target_scoped(source,stale,20,1248,true,1100),"foreign instance target healed");
    }
    {
        Instance w;w.join(1,"Monk",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.set_vitals(1,{50,200,40,40},100);const auto scope=*w.effect_scope(1);
        Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.consume_energy=[](auto){return true;};h.target_valid=[](auto id,float,bool){return id==1;};h.target_scope=[&](auto id){return w.effect_scope(id);};h.heal_scoped=[&](auto target,unsigned amount,float range,bool self,auto now){return w.heal_target_scoped(scope,target,amount,range,self,now);};
        Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned[281]=1;Slots bar{};bar[0]={281,0};attributes::Ranks ranks{};
        Runtime self(1);check(self.start(100,{281,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted,"scoped-only heal hook rejected");w.tick(1200);self.update(1100,h);check(w.resources(1)->health==70,"scoped self healing/late clock");
        h.target_scope={};Runtime missing(1);check(missing.start(2000,{281,0,1},bar,policy,ranks,true,h)==casting::Reason::unsupported,"scoped hook accepted without capture provider");
    }
    for(bool departing:{false,true}){
        Instance w;w.join(1,"Monk",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.join(2,"Moving ally",departing?1200.f:1300.f,0,std::make_shared<Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{100,200,40,40},100);w.set_vitals(2,{50,200,40,40},100);check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100),"moving party fixture");const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);w.move(2,departing?2000.f:0.f,0,100);
        check(w.heal_target_scoped(caster,target,20,1248,true,1100)==!departing&&w.resources(2)->health==(departing?50u:70u),"scoped heal used stale moving position");
    }
    for(unsigned scenario=0;scenario<3;++scenario){
        Instance w;w.join(1,"Monk",0,0,std::make_shared<Outbox>(),0,{Packet{165,0,1,0,0,0,3,0}}, {},100,1);w.join(2,"Ally",100,0,std::make_shared<Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{100,200,40,40},100);w.set_vitals(2,{50,200,40,40},100);check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100),"Breeze party fixture");const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);
        const auto replace=[&](){w.leave(2);w.join(2,"Replacement",100,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.set_vitals(2,{50,200,40,40},100);check(w.party_action(1,158,2,1100)&&w.party_action(2,154,1,1100),"Breeze replacement party");};
        Hooks h;unsigned commits=0,cost=0;h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.consume_energy=[&](unsigned value){cost+=value;return true;};h.target_valid=[&](auto id,float range,bool self){return w.valid_heal_target(1,id,range,self);};h.target_scope=[&](auto id){return w.effect_scope(id);};
        h.protective_enchantment=[&](auto captured,const auto& plan,unsigned divine,auto now){++commits;check(captured==target&&plan.skill==288&&plan.rank==12&&divine==12,"Breeze scoped plan");if(scenario==2)replace();return w.protective_enchantment(caster,captured,plan,divine,now);};
        Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned[288]=1;Slots bar{};bar[0]={288,0};attributes::Ranks ranks{};ranks[13]=12;ranks[16]=12;
        Runtime cast(1);check(cast.start(100,{288,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted&&cost==10,"scoped-only Breeze activation");if(scenario==1)replace();cast.update(1099,h);check(!commits,"Breeze completed early");cast.update(1100,h);
        check(commits==(scenario==1?0u:1u)&&w.resources(2)->health==(scenario==0?88u:50u),"Breeze Divine Favor crossed target lifetime");
        const auto active=w.status_snapshot(*w.effect_scope(2),1100);check(active&&active->size()==(scenario==0?1u:0u),"Breeze regeneration crossed target lifetime");
        w.tick(2100);check(w.resources(2)->health==(scenario==0?104u:50u),"Breeze regeneration or duplicate Divine Favor");
    }
    for(bool departing:{false,true}){
        Instance w;w.join(1,"Monk",0,0,std::make_shared<Outbox>(),0,{Packet{165,0,1,0,0,0,3,0}}, {},100,1);w.join(2,"Moving ally",departing?1200.f:1300.f,0,std::make_shared<Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{100,200,40,40},100);w.set_vitals(2,{50,200,40,40},100);check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100),"Breeze moving party");const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);w.move(2,departing?2000.f:0.f,0,100);
        check(w.protective_enchantment(caster,target,*regeneration::reviewed(288,12),12,1100)==!departing&&w.resources(2)->health==(departing?50u:88u),"Breeze used stale moving position");
    }
    std::cout<<"Direct healing and Healing Breeze retain target lifetime through atomic commit\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
