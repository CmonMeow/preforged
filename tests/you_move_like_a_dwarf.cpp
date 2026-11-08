#include "gw_you_move_like_a_dwarf.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
int main()try{
    static constexpr unsigned damage[]{44,51,58,66,73,80};
    static constexpr unsigned cripple[]{8,9,11,12,14,15};
    for(unsigned rank=0;rank<=10;++rank){
        const auto plan=you_move_like_a_dwarf(rank);
        if(!plan||plan->skill!=2358||plan->damage!=damage[std::min(rank,5u)]||
           plan->cripple_ms!=1000ull*cripple[std::min(rank,5u)]||plan->knockdown_ms!=2000)
            throw std::runtime_error("Norn progression mismatch");
    }
    if(you_move_like_a_dwarf(11))throw std::runtime_error("invalid rank accepted");
    std::cout<<"PASS You Move Like a Dwarf catalog and Norn progression\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
