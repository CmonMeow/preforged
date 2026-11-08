#include "gw_bundle_energy.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Bundle Energy line "+std::to_string(at.line()));
}
int main()try{
    const skills::effects::Scope owner{1,1,1};energy::Regeneration regen;
    unsigned cases=0;
    for(unsigned morale:{40u,100u,110u})for(bool held:{false,true}){
        const bundles::EnergySources source{50,32,22,morale};
        const auto capacity=*source.nominal(held);
        for(int current=-30;current<=int(capacity);++current){
            overcast::CeilingCurve curve(owner,1000,33);
            const vitals::Resources original{100,100,unsigned(std::max(0,current)),capacity};
            const auto debt=unsigned(std::max(0,-current));
            const auto transition=bundles::prepare_energy_transition(source,held,!held,curve,owner,original,regen,debt,1000,0);
            check(bool(transition));const auto& after=transition->after;
            const auto expected=current+int(*source.nominal(!held))-int(capacity);
            check(after.resources.max_energy==*source.nominal(!held));
            check(overcast::precise_current(after)==std::int64_t(expected)*overcast::energy_unit);
            check(transition->crossed_zero()==(current>0&&expected<=0));
            const auto reverse=bundles::prepare_energy_transition(source,!held,held,after.curve,owner,
                after.resources,after.regeneration,after.debt,1000,0,after.fractional_tail);
            check(reverse&&overcast::precise_current(reverse->after)==std::int64_t(current)*overcast::energy_unit);
            check(curve.cursor()==1000&&original.max_energy==capacity&&original.energy==unsigned(std::max(0,current)));
            ++cases;
        }
    }
    const bundles::EnergySources source{50,32,22,100};
    overcast::CeilingCurve burden(owner,1000,33);check(burden.apply(owner,10,1,1000));
    vitals::Resources resources{100,100,10,82};
    const auto fractional=bundles::prepare_energy_transition(source,false,true,burden,owner,resources,regen,0,1001,4);
    check(fractional&&fractional->crossed_zero());
    check(overcast::precise_current(fractional->after)-overcast::precise_current(fractional->before)==-22*overcast::energy_unit);
    check(fractional->before.curve.burden_units()==fractional->after.curve.burden_units());
    check(fractional->after.regeneration.remainder()==fractional->before.regeneration.remainder());
    overcast::CeilingCurve clear(owner,1000,33);resources.energy=1;
    check(!bundles::prepare_energy_transition(source,false,true,clear,owner,resources,regen,0,7000,-1));
    check(!bundles::prepare_energy_transition(source,false,false,clear,owner,resources,regen,0,1000,0));
    auto stale=owner;++stale.generation;
    check(!bundles::prepare_energy_transition(source,false,true,clear,stale,resources,regen,0,1000,0));
    resources.max_energy=81;
    check(!bundles::prepare_energy_transition(source,false,true,clear,owner,resources,regen,0,1000,0));
    check(!bundles::EnergySources{50,10,1000001,100}.nominal(true));
    // Removing a negative-Energy weapon increases capacity; dropping it can
    // cross zero when its negative modifier becomes effective again.
    const bundles::EnergySources negative{50,5,-5,100};resources={100,100,1,55};
    const auto up=bundles::prepare_energy_transition(negative,false,true,clear,owner,resources,regen,0,1000,0);
    check(up&&up->after.resources.energy==6&&up->after.resources.max_energy==60&&!up->crossed_zero());
    resources={100,100,1,60};
    const auto down=bundles::prepare_energy_transition(negative,true,false,clear,owner,resources,regen,0,1000,0);
    check(down&&down->after.resources.energy==0&&down->after.debt==4&&down->crossed_zero());
    std::cout<<cases<<" bundle Energy round trips, signed debt, morale, fractional Overcast and event boundaries passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
