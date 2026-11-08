#include "gw_elemental_lord.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
static void check(bool ok){if(!ok)throw std::runtime_error("Elemental Lord policy regression");}
int main()try{
    constexpr unsigned seconds[]{40,43,47,49,53,56,60};
    constexpr unsigned percent[]{100,127,167,193,233,260,300};
    for(unsigned id:{1951u,2094u})for(unsigned rank=0;rank<=12;++rank){
        auto plan=elemental_lord(id,rank);check(bool(plan));
        check(plan->duration_ms==seconds[std::min(rank,6u)]*1000&&plan->healing_percent==percent[std::min(rank,6u)]);
        for(unsigned attribute=0;attribute<=51;++attribute)
            check(elemental_lord_attribute_bonus(*plan,attribute)==(attribute>=8&&attribute<=11?1u:0u));
        for(unsigned storage=0;storage<=20;++storage){
            auto quote=quote_elemental_lord_spell(*plan,281,storage);
            check(quote&&quote->base_energy==5&&quote->energy_gain==storage/10&&
                  quote->published_heal_numerator==5*plan->healing_percent&&quote->published_heal_denominator==100);
        }
        check(!quote_elemental_lord_spell(*plan,200,12)); // glyph
        check(!quote_elemental_lord_spell(*plan,1,12)); // signet
        check(!quote_elemental_lord_spell(*plan,0,12));
        check(!quote_elemental_lord_spell(*plan,281,21));
        check(!elemental_lord_attribute_bonus(*plan,52));
        auto forged=*plan;++forged.healing_percent;check(!quote_elemental_lord_spell(forged,281,12));
        forged=*plan;++forged.attribute_bonus;check(!elemental_lord_attribute_bonus(forged,8));
    }
    check(!elemental_lord(1951,13)&&!elemental_lord(2094,13)&&!elemental_lord(1952,6));
    std::cout<<"Elemental Lord detached native/table policy passed; unbound, final healing precision unresolved\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
