#include "gw_effect_registry.h"
#include <iostream>
#include <stdexcept>
int main()try{
    using namespace fw::gw::skills::effects;
    auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
    Scope owner{123,42,7};Registry effects(owner);
    check(effects.apply(owner,{1,42,169,12,Category::enchantment,0,55000},Replace::same_skill).accepted,"reviewed attunement status");
    check(effects.presentation(owner,0x2012,1)==0x2092,"enchantment status preserves dead/sitting/condition bits");
    check(effects.apply(owner,{2,43,169,12,Category::enchantment,1000,56000},Replace::same_skill).removed.size()==1&&effects.size()==1,"handler explicit same-skill replacement");
    check(!effects.remove({123,42,8},2)&&effects.size()==1,"recycled actor cannot remove prior lifetime effect");
    check(!effects.apply({124,42,7},{3,42,169,12,Category::enchantment,0,55000},Replace::same_skill).accepted,"foreign instance denied");
    check(!effects.apply(owner,{3,42,169,12,Category::hex,0,55000},Replace::independent).accepted,"false skill category cannot mark actor hexed");
    check(!effects.apply(owner,{3,42,486,12,Category::other,0,55000},Replace::independent).accepted,"conditions use separate authoritative state");
    check(effects.apply(owner,{4,42,1043,0,Category::stance,1000,4000},Replace::category).accepted,"actual stance status");
    check(!effects.has(owner,Category::stance,999),"future effect cannot grant present mechanics");
    check(effects.expire(owner,3999).empty()&&effects.expire(owner,4000).size()==1,"exact natural-expiry records distinguished from explicit removal");
    check(effects.has(owner,Category::enchantment,55999)&&!effects.has(owner,Category::enchantment,56000),"expired effects cannot provide conditional bonuses before sweep");
    check(effects.remove(owner,2).has_value()&&effects.presentation(owner,0x2092,56000)==0x2012,"removal clears only owned category flags");
    Registry bounded(owner);
    for(unsigned id=1;id<=128;++id)check(bounded.apply(owner,{id,42,169,12,Category::enchantment,0,55000},Replace::independent).accepted,"bounded independent statuses");
    check(!bounded.apply(owner,{129,42,169,12,Category::enchantment,0,55000},Replace::independent).accepted&&bounded.size()==128,"capacity denial is atomic");
    check(bounded.apply(owner,{129,42,169,12,Category::enchantment,0,55000},Replace::same_skill).removed.size()==128&&bounded.size()==1,"replacement permitted at capacity");
    Registry ordering(owner);check(ordering.apply(owner,{999,42,835,12,Category::hex,0,1000},Replace::independent).accepted&&ordering.apply(owner,{1,42,835,12,Category::hex,0,2000},Replace::independent).accepted,"same-time hex entries");
    check(ordering.newest(owner,Category::hex,1)->instance==1,"cover order follows actual insertion not effect ID");
    check(ordering.remove(owner,1).has_value()&&ordering.newest(owner,Category::hex,1)->instance==999,"next newest hex after removal");
    check(!ordering.newest(owner,Category::hex,1000)&&!ordering.newest({123,42,8},Category::hex,1),"expired and foreign hex excluded");
    std::cout<<"Authoritative effect lifetime, replacement and category tests passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
