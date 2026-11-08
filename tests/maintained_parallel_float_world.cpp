// Detached acceptance regression for float-only exits hidden by sparse endpoints.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Parallel float World line "+std::to_string(at.line()));
}
int main()try{
 world::Instance w;
 for(unsigned id:{1u,2u}){w.join(id,"Party",id==1?0.f:500.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{1000,1000,100,100},100);}
 check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
 const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.stage_mending(source,target,*mending(12),100));
 w.move(2,5000,0,100);std::uint64_t low=100,arrival=40100;
 check(w.position(2,arrival).first==5000);
 while(arrival-low>1){const auto middle=low+(arrival-low)/2;if(w.position(2,middle).first==5000)arrival=middle;else low=middle;}
 check(w.settle_resources(source,arrival)&&w.maintained_enchantment_count(source,arrival)==1);
 w.move(1,100,0,arrival);w.move(2,5100,0,arrival);
 const auto a=w.maintained_range_path(source,arrival,arrival+100),b=w.maintained_range_path(target,arrival,arrival+100);
 // Stay before click ends-1 knots: source/target float endpoints at100ms
 // are inside, while the1ms independently stored positions are outside.
 check(bool(a));check(bool(b));
 const auto nominal=maintained_range_exit(*a,*b,arrival,arrival+100);
 check(bool(nominal));check(!nominal->first_outside);
 std::optional<std::uint64_t> exact;
 for(auto at=arrival;at<=arrival+1000;++at){const auto p=w.position(1,at),q=w.position(2,at);
  if(std::hypot(double(q.first)-p.first,double(q.second)-p.second)>5000.){exact=at;break;}}
 check(exact&&*exact>arrival);check(w.settle_resources(source,*exact-1)&&w.maintained_enchantment_count(source,*exact-1)==1);
 check(w.settle_resources(source,*exact)&&w.maintained_enchantment_count(source,*exact)==0);
 std::cout<<"PASS exact float parallel maintained exit at "<<*exact-arrival<<"ms despite no nominal endpoint exit\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
