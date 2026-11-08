#include "gw_ward_against_foes.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace fw::gw;
using namespace skills::effects;
static void check(bool ok){if(!ok)throw std::runtime_error("Ward177 policy failed");}
int main()try {
    const Scope caster{1,2,3};
    const auto ward=ward_against_foes(caster,99,10,0,0,0,100);
    check(ward&&valid_ward_against_foes(*ward)&&ward->expires==16100);
    WardFoeRecipient foe{{1,5,7},0,0,0,true,false,true};
    check(ward_foes_movement_multiplier(*ward,foe,100)==.5f);
    check(ward_foes_movement_multiplier(*ward,foe,16099)==.5f);
    check(!ward_foes_movement_multiplier(*ward,foe,16100));
    check(!ward_foes_movement_multiplier(*ward,foe,99));
    foe.x=322;check(ward_foes_covers(*ward,foe,100));
    foe.x=322.1f;check(!ward_foes_covers(*ward,foe,100));foe.x=0;
    auto denied=foe;denied.authoritative_foe=false;check(!ward_foes_covers(*ward,denied,100));
    denied=foe;denied.spirit=true;check(!ward_foes_covers(*ward,denied,100));
    denied=foe;denied.living=false;check(!ward_foes_covers(*ward,denied,100));
    denied=foe;++denied.scope.instance;check(!ward_foes_covers(*ward,denied,100));
    denied=foe;++denied.plane;check(!ward_foes_covers(*ward,denied,100));
    auto forged=*ward;++forged.expires;check(!ward_foes_covers(forged,foe,100));
    check(!ward_against_foes({},99,10,0,0,0,100));
    check(!ward_against_foes(caster,99,21,0,0,0,100));
    check(!ward_against_foes(caster,99,10,0,0,0,UINT64_MAX));
    foe.x=-500;foe.y=0;
    const auto cross=ward_foes_segment_fraction(*ward,foe,500,0,100);
    check(cross&&std::abs(*cross-.644)<1e-9);
    const auto crossing=ward_foes_segment_interval(*ward,foe,500,0,100);
    check(crossing&&std::abs(crossing->enter-.178)<1e-9&&
        std::abs(crossing->leave-.822)<1e-9);
    check(std::abs(*ward_foes_segment_travel_ms(*ward,foe,500,0,100,1000.)-1644.)<1e-9);
    check(std::abs(*ward_foes_segment_travel_ms(*ward,foe,500,0,15600,1000.)-1161.)<1e-9);
    check(std::abs(*ward_foes_segment_travel_ms(*ward,foe,500,0,15950,1000.)-1000.)<1e-9);
    check(!ward_foes_segment_travel_ms(*ward,foe,500,0,100,0.));
    foe.x=0;check(ward_foes_segment_fraction(*ward,foe,100,0,100)==1.);
    const auto outbound=ward_foes_segment_interval(*ward,foe,500,0,100);
    check(outbound&&outbound->enter==0&&std::abs(outbound->leave-.644)<1e-9);
    foe.x=500;check(ward_foes_segment_fraction(*ward,foe,600,0,100)==0.);
    check(ward_foes_segment_fraction(*ward,foe,500,0,100)==0.);
    const auto inbound=ward_foes_segment_interval(*ward,foe,0,0,100);
    check(inbound&&std::abs(inbound->enter-.356)<1e-9&&inbound->leave==1.);
    check(ward_foes_segment_fraction(*ward,foe,500,0,100)==0.);
    foe.x=-500;foe.y=322;check(ward_foes_segment_fraction(*ward,foe,500,322,100)==0.);
    check(!ward_foes_segment_fraction(*ward,foe,500,322,ward->expires));
    std::cout<<"Ward177 foe-only spatial slow and segment clipping passed (detached)\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
