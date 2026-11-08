#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location where=std::source_location::current()){
    if(!value)throw std::runtime_error("scoped energy regression line "+std::to_string(where.line()));
}
int main()try{
    world::Instance w;auto out=std::make_shared<world::Outbox>();w.join(1,"Original",0,0,out);w.set_vitals(1,{100,100,20,40},100);
    const auto original=*w.effect_scope(1);check(w.consume_energy(1,7,original)&&w.resources(1)->energy==13);
    check(w.restore_energy(1,2,original)&&w.resources(1)->energy==15);
    auto stale=original;++stale.generation;
    check(!w.consume_energy(1,0,stale)&&!w.consume_energy(1,5,stale)&&!w.restore_energy(1,100,stale)&&w.resources(1)->energy==15);
    w.leave(1);w.join(1,"Replacement",0,0,out);w.set_vitals(1,{100,100,6,40},100);const auto replacement=*w.effect_scope(1);
    check(replacement!=original&&!w.consume_energy(1,1,original)&&!w.restore_energy(1,40,original)&&w.resources(1)->energy==6);
    w.join(2,"Other",0,0,std::make_shared<world::Outbox>());w.set_vitals(2,{100,100,10,20},100);
    check(!w.consume_energy(1,1,*w.effect_scope(2))&&!w.restore_energy(1,1,*w.effect_scope(2)));
    check(w.consume_energy(1,0,replacement)&&w.restore_energy(1,100,replacement)&&w.resources(1)->energy==40);
    check(w.kill(replacement,200)&&!w.consume_energy(1,0,replacement)&&!w.restore_energy(1,100,replacement));
    std::cout<<"Scoped energy costs/refunds reject stale, replaced, foreign and dead actors PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
