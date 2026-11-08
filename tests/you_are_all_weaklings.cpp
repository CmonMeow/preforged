#include "gw_you_are_all_weaklings.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
int main()try{
    static constexpr unsigned seconds[]{8,9,10,10,11,12};
    for(unsigned rank=0;rank<=10;++rank){
        const auto plan=you_are_all_weaklings(rank);
        if(!plan||plan->skill!=2359||plan->weakness_ms!=1000ull*seconds[std::min(rank,5u)]||
           plan->range!=1248||plan->adjacent!=166)
            throw std::runtime_error("Norn progression mismatch");
    }
    if(you_are_all_weaklings(11))throw std::runtime_error("invalid rank accepted");
    std::cout<<"PASS You Are All Weaklings catalog and Norn progression\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
