#include "gw_world_simulation.h"
#include <iostream>
#include <fstream>
#include <string>
#include <source_location>
#include <bit>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Reversal World line "+std::to_string(at.line()));
}
unsigned u32(const world::Packet& p,unsigned at){unsigned v=0;for(unsigned i=0;i<4;++i)v|=unsigned(p.at(at+i))<<(8*i);return v;}
int main(int argc,char** argv)try{
    check(argc<=2);std::ofstream exported;if(argc==2){exported.open(argv[1]);check(exported.good());}
    unsigned cases=0;
    for(unsigned skill:{39u,42u})for(unsigned rank:{0u,12u,20u})for(unsigned hp:{10u,100u,500u})
    for(unsigned renewal:{0u,181u,2860u})for(unsigned style=0;style<3;++style){
        ++cases;world::Instance w;auto box=std::make_shared<world::Outbox>();
        for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:60,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
            w.set_vitals(id,{id==1?100u:id==2?hp:500u,id==1?1000u:500u,20,40},100);}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2),other=*w.effect_scope(3);
        w.public_professions(1,6,5);
        if(renewal){const auto r=*effects::ether_renewal(renewal,15);check(w.status_mutation(source,renewal,80,15,100,100+r.duration,true));}
        check(w.status_mutation(target,307,5,rank,100,8100,true));check(w.status_mutation(other,307,6,rank,100,8100,true));
        double expected=hp;combat::deep_wound::State wound;wound.reset(500);double factor=1;
        if(style==1){check(w.status_mutation(target,785,11,12,100,8100,true));factor=.67;}
        if(style==2){check(w.apply_condition(source,target,combat::conditions::Kind::deep_wound,100,10000));wound.capacity(expected,500,true);}
        const auto initial=expected;const double heal=std::min(72u,effects::reversal_fortune(rank)->maximum_healing);
        wound.event(expected,heal,true,factor);const auto healed=expected-initial;
        if(expected>0)wound.event(expected,-(72-heal));
        box->drain();check(w.before_spell_effect(source,skill,1,100));
        check(w.energy_drain(source,target,*effects::energy_drain(skill,12),20,100,nullptr,1));
        if(w.resources(2)->health!=static_cast<unsigned>(std::ceil(expected))||w.resources(2)->max_health!=(expected==0?500:wound.maximum()))
            throw std::runtime_error("Reversal resources skill="+std::to_string(skill)+" rank="+std::to_string(rank)+" hp="+std::to_string(hp)+" renewal="+std::to_string(renewal)+" style="+std::to_string(style)+" actual="+std::to_string(w.resources(2)->health)+"/"+std::to_string(w.resources(2)->max_health)+" expected="+std::to_string(expected)+"/"+std::to_string(expected==0?500:wound.maximum()));
        check(w.resources(1)->health==100+(renewal?20:0));
        check(w.resources(1)->energy==20+(renewal?effects::ether_renewal(renewal,15)->energy_per_enchantment:0));
        const auto statuses=*w.status_snapshot(target,100);check(std::none_of(statuses.begin(),statuses.end(),[](auto e){return e.skill==307;}));
        const auto remaining=72-heal;const auto packets=box->drain();
        if(exported.is_open()&&rank==12&&!renewal&&!style&&(hp==10||hp==500)){
            exported<<"{\"skill\":"<<skill<<",\"initial_health\":"<<hp<<",\"expected_health\":"<<expected<<",\"packets\":[";bool first=true;
            for(const auto& packet:packets){if(!first)exported<<',';first=false;exported<<'"';for(auto byte:packet){constexpr char hex[]="0123456789abcdef";exported<<hex[byte>>4]<<hex[byte&15];}exported<<'"';}exported<<"]}\n";
        }
        const auto removal=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==67&&u32(p,2)==2&&u32(p,6)==5;});check(removal!=packets.end());
        auto positive=packets.end(),negative=packets.end();
        for(auto i=packets.begin();i!=packets.end();++i)if(i->size()==18&&(*i)[0]==162&&u32(*i,6)==2){
            const float delta=std::bit_cast<float>(u32(*i,14));if(delta>0)positive=i;if(delta<0)negative=i;}
        if(healed>0){check(positive!=packets.end()&&removal<positive);
            check(std::abs(std::bit_cast<float>(u32(*positive,14))-healed/wound.maximum())<1e-6);}
        if(remaining&&initial+healed>0){check(negative!=packets.end()&&removal<negative);if(healed>0)check(positive<negative);
            check(std::abs(std::bit_cast<float>(u32(*negative,14))+remaining/wound.maximum())<1e-6);}
        else check(negative==packets.end());
        // A primary-only Burn must leave the other enchantment intact;Surge
        // commits each nearby player's independent one-use conversion.
        const auto others=*w.status_snapshot(other,100);const bool retained=std::any_of(others.begin(),others.end(),[](auto e){return e.skill==307;});
        check(retained==(skill==42));check(w.resources(3)->health==500-(skill==39?static_cast<unsigned>(remaining):0));
        const auto old=w.resources(2)->health;check(!w.energy_drain(source,target,*effects::energy_drain(skill,12),20,100,nullptr,1));check(w.resources(2)->health==old);
    }
    check(cases==162);
    for(unsigned mode=0;mode<4;++mode){world::Instance w;
        for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{100,500,20,40},100);}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.status_mutation(target,307,5,12,100,8100,true));
        if(mode==0)check(w.status_mutation(target,307,5,12,100,{},false));
        const unsigned at=mode==1?8100:100;
        if(mode==2)check(w.status_mutation(target,307,6,0,100,8100,true)); // replacement rank0.
        if(mode==3){w.set_vitals(2,{100,500,0,40},100);} // No damage to convert.
        if(mode==1){check(w.settle_resources(target,at));w.set_vitals(2,{100,500,20,40},100);}
        check(w.energy_drain(source,target,*effects::energy_drain(42,12),20,at));
        if(mode==0||mode==1)check(w.resources(2)->health==28);
        if(mode==2)check(w.resources(2)->health==58); // Heal15,then57 damage.
        if(mode==3){check(w.resources(2)->health==100);const auto entries=*w.status_snapshot(target,100);check(std::any_of(entries.begin(),entries.end(),[](auto e){return e.skill==307;}));}
    }
    unsigned repeated=0;
    for(unsigned skill:{39u,42u})for(unsigned renewal:{0u,181u,2860u}){
        ++repeated;world::Instance w;
        for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{100,id==1?1000u:500u,20,40},100);}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);w.public_professions(1,6,5);
        if(renewal){const auto r=*effects::ether_renewal(renewal,15);check(w.status_mutation(source,renewal,80,15,100,100+r.duration,true));}
        check(w.status_mutation(target,307,5,12,100,8100,true));
        for(unsigned event:{1u,2u}){check(w.before_spell_effect(source,skill,event,100));check(w.energy_drain(source,target,*effects::energy_drain(skill,12),20,100,nullptr,event));
            check(w.resources(2)->health==(event==1?162:90));check(w.resources(1)->health==100+(renewal?20*event:0));}
    }
    check(repeated==6);
    std::cout<<"162 Reversal World healing-first/penalty/wound/Renewal transactions and4 lifecycle/zero plus6 repeated-hit cases PASS;cast remains unbound\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
