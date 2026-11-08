#include "gw_effect_capacity.h"
#include <iostream>
#include <stdexcept>
using fw::gw::skills::effects::effect_instance_capacity;
int main()try{
 const auto last=std::uint64_t(UINT32_MAX);
 const auto check=[](bool ok){if(!ok)throw std::runtime_error("effect instance capacity boundary");};
 check(effect_instance_capacity(1,0)&&effect_instance_capacity(last+1,0)&&effect_instance_capacity(UINT64_MAX,0));
 check(effect_instance_capacity(last,1)&&!effect_instance_capacity(last,2));
 check(effect_instance_capacity(last-1,2)&&!effect_instance_capacity(last-1,3));
 check(!effect_instance_capacity(last+1,1)&&!effect_instance_capacity(UINT64_MAX,1));
 check(!effect_instance_capacity(0,1)&&effect_instance_capacity(0,0));
 check(effect_instance_capacity(1,std::size_t(UINT32_MAX))&&!effect_instance_capacity(1,std::size_t(UINT32_MAX)+1));
 std::cout<<"PASS effect instance capacity,zero additions,last ID and overflow\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
