#include "gw_iron_palm_chain_wire.h"
#include <iostream>
#include <stdexcept>

int main(){
    using fw::gw::skills::effects::iron_palm_chain_2022;
    for(unsigned stage=0;stage<=3;++stage){
        const std::vector<std::uint8_t> expected{
            0x5b,0,42,0,0,0,43,0,0,0,std::uint8_t(stage),0,0,0};
        if(iron_palm_chain_2022(42,43,stage)!=expected)
            throw std::runtime_error("original serializer fixture mismatch");
    }
    for(const auto values:{std::vector<unsigned>{0,43,1},{42,0,1},
            {65536,43,1},{42,65536,1},{42,42,1},{42,43,4}}){
        bool rejected=false;
        try{iron_palm_chain_2022(values[0],values[1],values[2]);}
        catch(const std::invalid_argument&){rejected=true;}
        if(!rejected)throw std::runtime_error("invalid chain update accepted");
    }
    std::cout<<"PASS Iron Palm original-2022 chain wire fixtures\n";
}
