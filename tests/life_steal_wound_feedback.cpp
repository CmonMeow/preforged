#include "gw_npc_catalog.h"
#include <iostream>
using namespace fw::gw;
int main()try{
    using namespace skills;const std::string hash(64,'a');
    std::istringstream p("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
    std::istringstream m("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/life_steal_wound_feedback.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 80 1 1 test test test - test 60 60 test test\n");
    auto catalog=world::load_npc_catalog(p);world::bind_combat_catalog(catalog,combat::npcs::load_catalog(m,hash),hash);
    unsigned failures=0;
    for(unsigned target:{2u,100u})for(unsigned rank:{0u,12u}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();w.configure_npcs(catalog.for_map(146));
        w.join(1,"Necromancer",0,0,box,0,{}, {},100,1);w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{50,1000,100,100},100);w.set_vitals(2,{500,500,100,100},100);
        const auto a=*w.effect_scope(1),b=*w.effect_scope(target);
        if(!w.apply_condition(a,a,combat::conditions::Kind::deep_wound,100,60000))return 2;
        const auto epoch=*w.control_epoch(a);
        box->drain();if(!w.life_steal(a,b,*effects::life_steal(153,rank),100))return 3;
        const auto hp=*w.resources(1);if(hp.health!=(rank?2u:0u))return 4;
        if(*w.control_epoch(a)!=epoch+(rank?0u:1u)){++failures;std::cerr<<"missing source death invalidation; target="<<target<<'\n';}
        const auto expected=combat::detail::fraction(1,combat::Health{double(hp.health),hp.max_health});
        auto last=expected;bool found=false;
        for(const auto& packet:box->drain())if(packet.size()==expected.size()&&std::equal(expected.begin(),expected.begin()+10,packet.begin())){last=packet;found=true;}
        if(!found||last!=expected){++failures;std::cerr<<"stale source Health fraction after wound settlement; target="<<target<<" rank="<<rank<<'\n';}
    }
    return failures?1:0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 5;}
