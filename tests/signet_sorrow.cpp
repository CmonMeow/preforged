#include "gw_signet_sorrow.h"
#include <stdexcept>
using namespace fw::gw;
static void check(bool value){if(!value)throw std::runtime_error("Signet of Sorrow test failed");}
int main(){
    check(combat::detail::integer(21,25,1619)==
          combat::Packet({158,0,21,0,0,0,25,0,0,0,83,6,0,0}));
    check(combat::detail::integer(42,30,480)==
          combat::Packet({158,0,42,0,0,0,30,0,0,0,224,1,0,0}));
    for(unsigned rank=0;rank<=20;++rank){
        const auto plan=skills::effects::signet_sorrow(rank);check(plan.has_value());
        check(plan->damage==15+4*rank&&plan->activation_ms==1000&&
              plan->aftercast_ms==750&&plan->recharge_ms==30000);
        skills::effects::SignetSorrowFacts facts{true,true,false,1248,{}};
        auto hit=skills::effects::prepare_signet_sorrow(*plan,20,facts);
        check(hit&&hit->damage.kind==combat::Kind::armor_ignoring&&
              hit->damage.amount==plan->damage&&hit->recharge_ms==30000);
        facts.nearest_corpse_distance=252.f;
        hit=skills::effects::prepare_signet_sorrow(*plan,20,facts);
        check(hit&&hit->recharge_ms==0);
        facts.nearest_corpse_distance=252.01f;
        hit=skills::effects::prepare_signet_sorrow(*plan,20,facts);
        check(hit&&hit->recharge_ms==30000);
        facts.nearest_corpse_distance.reset();facts.dead_pet=true;
        hit=skills::effects::prepare_signet_sorrow(*plan,20,facts);
        check(hit&&hit->recharge_ms==0);
        facts.target_is_foe=false;check(!skills::effects::prepare_signet_sorrow(*plan,20,facts));
        facts.target_is_foe=true;facts.target_distance=1249;
        check(!skills::effects::prepare_signet_sorrow(*plan,20,facts));
        facts.target_distance=1;facts.nearest_corpse_distance=-1.f;
        check(!skills::effects::prepare_signet_sorrow(*plan,20,facts));
    }
    check(!skills::effects::signet_sorrow(21));
    return 0;
}
