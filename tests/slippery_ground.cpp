#include "gw_slippery_ground.h"
#include <iostream>
#include <limits>
#include <source_location>
#include <string>
using namespace fw::gw::skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){
    if(!b)throw std::runtime_error("Slippery Ground line "+std::to_string(at.line()));
}
int main()try{
    const SlipperyGroundActor source{{1,1,1},0,0,0,true,false,false,false};
    std::vector<SlipperyGroundActor> foes{
        {{1,2,1},100,0,0,true,true,false,false},
        {{1,3,1},266,0,0,true,true,true,false},
        {{1,4,1},266.001,0,0,true,true,true,true},
        {{1,5,1},100,0,0,true,true,false,true},
        {{1,6,1},100,0,0,true,false,true,true},
        {{1,7,1},100,0,1,true,true,true,true},
        {{1,8,1},100,0,0,false,true,true,true}};
    const auto high=*slippery_ground(5),low=*slippery_ground(4);
    check(!slippery_ground(21)&&high.failure_percent==0&&low.failure_percent==50);
    const auto quote=prepare_slippery_ground(high,source,foes[0].scope,foes);
    check(quote&&!quote->failed&&quote->knockdowns==std::vector<Scope>{foes[1].scope,foes[3].scope});
    for(unsigned draw=0;draw<100;++draw){
        auto q=prepare_slippery_ground(low,source,foes[0].scope,foes,draw);
        check(q&&q->failed==(draw<50)&&q->knockdowns.size()==(draw<50?0:2));
    }
    check(!prepare_slippery_ground(low,source,foes[0].scope,foes));
    check(!prepare_slippery_ground(low,source,foes[0].scope,foes,100));
    auto forged=high;forged.radius=167;check(!prepare_slippery_ground(forged,source,foes[0].scope,foes));
    auto changed=foes;changed[0].blind.reset();check(!prepare_slippery_ground(high,source,changed[0].scope,changed));
    changed[0].moving=true;check(prepare_slippery_ground(high,source,changed[0].scope,changed)->knockdowns.size()==3);
    changed=foes;changed.push_back(foes[0]);check(!prepare_slippery_ground(high,source,foes[0].scope,changed));
    changed=foes;changed[0].scope.generation++;check(!prepare_slippery_ground(high,source,foes[0].scope,changed));
    changed=foes;changed[0].x=1248;check(prepare_slippery_ground(high,source,changed[0].scope,changed).has_value());
    changed[0].x=1248.001;check(!prepare_slippery_ground(high,source,changed[0].scope,changed));
    changed=foes;changed[1].x=std::numeric_limits<double>::quiet_NaN();check(!prepare_slippery_ground(high,source,foes[0].scope,changed));
    std::cout<<"Slippery Ground selection passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
