#include "gw_arcane_mimicry.h"
#include <iostream>
using namespace fw::gw::skills;using namespace fw::gw::skills::effects;
void check(bool value){if(!value)throw std::runtime_error("Arcane Mimicry policy");}
int main()try{
    const auto plan=arcane_mimicry();check(plan&&plan->energy==15&&plan->copy_seconds==20&&plan->restoration_disable==60);
    Slots bar{};check(select_mimicry_donor(bar).status==MimicrySelection::no_elite);
    bar[0]={194,0};check(select_mimicry_donor(bar).status==MimicrySelection::no_elite);
    bar[5]={5,99};const auto selected=select_mimicry_donor(bar);
    check(selected.status==MimicrySelection::copy&&selected.slot==bar[5]&&selected.index==5);
    bar[6]={74,123};check(select_mimicry_donor(bar).slot==bar[5]);
    bar[2]={1518,7};const auto form=select_mimicry_donor(bar);
    check(form.status==MimicrySelection::form&&form.slot==bar[2]); // Do not skip a leftmost form to a later elite.
    bar[7]={999999,0};check(select_mimicry_donor(bar).status==MimicrySelection::invalid_bar);
    bar[7]={0,1};check(select_mimicry_donor(bar).status==MimicrySelection::invalid_bar);
    // Validate every slot before selecting the first elite, including a
    // malformed nonelite after it. Copy identities are client WORD values.
    bar[7]={194,65536};check(select_mimicry_donor(bar).status==MimicrySelection::invalid_bar);
    bar[7]={194,65535};check(select_mimicry_donor(bar).status==MimicrySelection::form);
    bar[2]={194,0};bar[5]={5,65535};
    check(select_mimicry_donor(bar).slot==bar[5]);
    bar[5]={5,65536};check(select_mimicry_donor(bar).status==MimicrySelection::invalid_bar);
    check(!find_skill(65)->elite()&&find_skill(1518)->type==26);
    std::cout<<"Arcane Mimicry native plan and first-elite selection passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
