#include "gw_ward_against_elements.h"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
using namespace skills::effects;
static void check(bool ok){if(!ok)throw std::runtime_error("ward policy failed");}
int main()try {
    auto ward=ward_against_elements(146,25,1,100,200,12,1000);
    check(ward.has_value());
    const auto one=std::array{*ward};
    const auto inside=[&](std::uint64_t at,combat::Kind kind= combat::Kind::fire){
        return ward_elemental_armor_bonus(one,146,100,200,true,false,at,kind);
    };
    check(inside(1000)==24&&inside(ward->expires-1)==24&&inside(ward->expires)==0);
    check(ward_covers(*ward,146,422,200,true,false,1001));
    check(!ward_covers(*ward,146,423,200,true,false,1001));
    check(!ward_covers(*ward,147,100,200,true,false,1001));
    check(!ward_covers(*ward,146,100,200,false,false,1001));
    check(!ward_covers(*ward,146,100,200,true,true,1001));
    check(!ward_covers(*ward,146,100,200,true,false,999));
    auto forged=*ward;forged.expires+=100000;
    check(!ward_covers(forged,146,100,200,true,false,1001));
    check(inside(1001,combat::Kind::cold)==24&&inside(1001,combat::Kind::earth)==24&&
          inside(1001,combat::Kind::lightning)==24&&inside(1001,combat::Kind::armor_ignoring)==0);
    check(ward_elemental_armor_bonus(one,146,100,200,true,false,1001,combat::Kind::fire,true)==0);
    const auto two=std::array{*ward,*ward};
    check(ward_elemental_armor_bonus(two,146,100,200,true,false,1001,combat::Kind::fire)==24);
    check(!ward_against_elements(0,25,1,100,200,12,1000));
    check(!ward_against_elements(146,25,1,100,200,21,1000));
    check(!ward_against_elements(146,25,1,100,200,12,UINT64_MAX));
    // The existing combat resolver must see the +24 as a real armor change.
    combat::Actor plain{.id=2,.health=combat::Health{1000,1000},.earth_armor=60,
        .damageable=true,.lightning_armor=60,.fire_armor=60,.cold_armor=60};
    auto protected_actor=plain;
    protected_actor.fire_armor= *plain.fire_armor+inside(1001);
    const combat::Damage fire{combat::Kind::fire,100,20,175};
    const auto a=combat::apply(plain,fire,true),b=combat::apply(protected_actor,fire,true);
    check(a.reason==combat::Reason::accepted&&b.reason==combat::Reason::accepted&&
          b.applied<a.applied&&b.applied/a.applied>0.65&&b.applied/a.applied<0.67);
    std::cout<<"Ward Against Elements ground eligibility and elemental armor policy passed (detached)\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
