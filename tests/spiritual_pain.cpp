#include "gw_spiritual_pain.h"
#include <cstdlib>

using namespace fw::gw;
static void check(bool ok){if(!ok)std::abort();}
int main(){
    for(unsigned rank=0;rank<=20;++rank){
        const auto plan=skills::effects::spiritual_pain(rank);
        check(plan&&plan->primary_damage==skills::effects::interpolate(15,75,rank)&&
              plan->summoned_damage==skills::effects::interpolate(25,125,rank));
    }
    check(!skills::effects::spiritual_pain(21));
    const auto plan=*skills::effects::spiritual_pain(0);
    combat::Actor foe{2,1,combat::Health{100,100},std::nullopt,0,true};
    auto primary=foe;
    auto one=skills::effects::spiritual_pain_target(plan,primary,20,true,true,false,0);
    check(one&&one->size()==1&&primary.health->current==85);
    auto summoned=foe;
    auto two=skills::effects::spiritual_pain_target(plan,summoned,20,true,true,true,0);
    check(two&&two->size()==2&&summoned.health->current==60);
    auto nearby=foe;
    auto extra=skills::effects::spiritual_pain_target(plan,nearby,20,true,false,true,322);
    check(extra&&extra->size()==1&&nearby.health->current==75);
    auto dying=foe;dying.health->current=10;
    auto lethal=skills::effects::spiritual_pain_target(plan,dying,20,true,true,true,0);
    check(lethal&&lethal->size()==1&&lethal->front().killed&&dying.health->current<=0);
    auto ordinary=foe;
    auto none=skills::effects::spiritual_pain_target(plan,ordinary,20,true,false,false,100);
    check(none&&none->empty()&&ordinary.health->current==100);
    check(!skills::effects::spiritual_pain_target(plan,ordinary,20,true,false,std::nullopt,100));
    check(!skills::effects::spiritual_pain_target(plan,ordinary,20,false,true,false,0));
    check(!skills::effects::spiritual_pain_target(plan,ordinary,20,true,false,true,323));
    check(!skills::effects::spiritual_pain_target(plan,ordinary,0,true,true,false,0));
    check(foe.health->current==100); // detached proposal leaves source untouched
}
