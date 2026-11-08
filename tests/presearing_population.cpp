#include "gw_presearing_population.h"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace fw::gw::population::presearing;
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
int main(){
    for(const auto& trainer:starters){
        for(auto stage:{TestStage::not_started,TestStage::active,TestStage::objective_complete}){
            const std::array party{Member{trainer.profession,stage}};
            check(resolve(146,trainer.name,party)==Decision::visible,"trainer vanished before turn-in");
            check(resolve(146,"Haversdan",party)==Decision::hidden,"guide arrived before reward");
        }
        std::array party{Member{trainer.profession,TestStage::rewarded,Handoff::walking}};
        check(resolve(146,trainer.name,party)==Decision::visible,"walking trainer deleted at reward");
        check(resolve(146,"Haversdan",party)==Decision::visible,"guide missing during overlap");
        party[0].handoff=Handoff::settled;
        check(resolve(146,trainer.name,party)==Decision::hidden,"rewarded trainer returned on entry");
        check(resolve(146,"Haversdan",party)==Decision::visible,"guide missing after reentry");
        for(const auto& other:starters)if(other.profession!=trainer.profession)
            check(resolve(146,other.name,party)==Decision::hidden,"unrelated profession spawned");
        check(resolve(165,trainer.name,party)==Decision::unmanaged,"non-Lakeside actor filtered");
    }
    const std::array party{Member{1,TestStage::active},Member{6,TestStage::rewarded}};
    check(resolve(146,"Van the Warrior",party)==Decision::unresolved,"unknown party precedence invented");
    check(resolve(146,"Van the Warrior",party,{PartyPolicy::union_members})==Decision::visible,"union policy failed");
    check(resolve(146,"Haversdan",party,{PartyPolicy::union_members})==Decision::visible,"union guide failed");
    check(resolve(146,"Van the Warrior",party,{PartyPolicy::selected_member,1})==Decision::hidden,"authority not honored");
    check(resolve(146,"Haversdan",party,{PartyPolicy::selected_member,0})==Decision::hidden,"wrong member stage");
    check(resolve(146,"Haversdan",party,{PartyPolicy::selected_member,2})==Decision::unresolved,"invalid authority accepted");
    check(resolve(146,"Haversdan",{})==Decision::unresolved,"empty party guessed");
    const std::array unknown{Member{1,TestStage::unknown}};
    check(resolve(146,"Van the Warrior",unknown)==Decision::unresolved,"unknown save state guessed");
    check(resolve(146,"Devona",unknown)==Decision::unmanaged,"ordinary NPC accidentally gated");
    std::cout<<"PASS all six trainer stages, reward overlap, fresh entry, map scope and explicit party authority\n";
}
