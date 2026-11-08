#include "gw_selfless_spirit.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
static void check(bool ok){if(!ok)throw std::runtime_error("Selfless Spirit policy regression");}
int main()try{
    check(!selfless_spirit(1951)&&!selfless_spirit(2094)&&!selfless_spirit(0));
    for(unsigned id:{1952u,2095u}){
        constexpr unsigned seconds[]{15,16,17,17,18,19,20};
        for(unsigned rank=0;rank<=12;++rank){
            const auto ranked=selfless_spirit(id,rank);check(bool(ranked));
            check(ranked->rank==rank&&ranked->duration_ms==seconds[rank<6?rank:6]*1000);
        }
        check(!selfless_spirit(id,13));
        const auto plan=selfless_spirit(id);check(bool(plan));
        check(plan->energy==5&&plan->reduction==3&&plan->activation_ms==250&&
              plan->aftercast_ms==750&&plan->recharge_ms==45000&&
              plan->duration0==15&&plan->duration15==20&&plan->caster_body==553);
        for(unsigned energy:{0u,1u,2u,3u,4u,5u,15u,25u,0xffffffffu}){
            auto quote=quote_selfless_spirit(*plan,true,true,SelflessTarget::other_ally,energy);
            check(quote&&quote->before==energy&&quote->after==(energy>3?energy-3:0));
            for(auto target:{SelflessTarget::self,SelflessTarget::foe,SelflessTarget::untargeted}){
                auto no=quote_selfless_spirit(*plan,true,true,target,energy);
                check(no&&no->after==energy&&no->reduction==0);
            }
            check(quote_selfless_spirit(*plan,false,true,SelflessTarget::other_ally,energy)->after==energy);
            check(quote_selfless_spirit(*plan,true,false,SelflessTarget::other_ally,energy)->after==energy);
        }
        check(!quote_selfless_spirit(*plan,true,true,SelflessTarget::unknown,5));
        auto forged=*plan;++forged.reduction;
        check(!quote_selfless_spirit(forged,true,true,SelflessTarget::other_ally,5));
        forged=*plan;++forged.duration15;
        check(!quote_selfless_spirit(forged,true,true,SelflessTarget::other_ally,5));
    }
    std::cout<<"Selfless Spirit detached catalog/cost policy passed; unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
