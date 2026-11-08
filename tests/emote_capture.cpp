#include "../tools/emotes/snapshot.h"
#include <iostream>
#include <stdexcept>
using namespace emote_capture;
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
template<class F>void rejected(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"invalid input accepted");}
int main()try{
    validate_array({0x1000,64,32,0},7);
    for(const auto a:{Array{0,64,32,0},Array{0x1001,64,32,0},Array{0x1000,16,32,0},Array{0x1000,65537,32,0}})
        rejected([&]{validate_array(a,7);});
    for(const Word id:{0u,32u,65536u})rejected([&]{validate_array({0x1000,64,32,0},id);});
    Anchor epoch{0x100,0x200,0x300,0x400,7,248,9,0x500,{0x1000,64,32,0}};
    Animation pose{0x31939cbb,12,0x3f800000,13};
    check(stable(epoch,pose,epoch,pose,epoch),"stable double read rejected");
    auto changed=epoch;changed.token=10;check(!stable(epoch,pose,changed,pose,epoch),"zoning mixed with old animation");
    changed=epoch;changed.view+=4;check(!stable(epoch,pose,epoch,pose,changed),"replaced actor accepted");
    changed=epoch;changed.agent=8;check(!stable(epoch,pose,changed,pose,changed),"character switch accepted");
    changed=epoch;changed.array.size++;check(!stable(epoch,pose,epoch,pose,changed),"array mutation accepted");
    auto animation=pose;animation.uid++;check(!stable(epoch,pose,epoch,animation,epoch),"UID transition mixed");
    animation=pose;animation.sequence_index++;check(!stable(epoch,pose,epoch,animation,epoch),"sequence transition mixed");
    animation=pose;animation.speed_bits++;check(!stable(epoch,pose,epoch,animation,epoch),"float bits transition mixed");
    animation=pose;animation.state_class++;check(!stable(epoch,pose,epoch,animation,epoch),"animation class transition mixed");
    check(label(L"/BoW")=="/bow"&&label(L"dance*")=="/dance*","manual label normalization failed");
    for(const auto text:{L"",L"/",L"/*",L"/bow**",L"/bow 1",L"/bow\n",L"/b\"ow",L"/b\\ow",L"/波",L"/abcdefghijklmnopqrstuvwxyzaaaaaaaa"})
        rejected([&]{(void)label(text);});
    std::cout<<"PASS external emote capture bounds, manual labels and coherent own-actor double reads\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
