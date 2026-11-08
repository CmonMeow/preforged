#include "gw_signet_strength.h"
#include <source_location>
#include <stdexcept>
#include <string>
#include <vector>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Signet of Strength line "+std::to_string(at.line()));
}
int main(){
    for(unsigned rank:{0u,4u,12u,15u,20u}){
        const auto plan=skills::effects::signet_strength(rank);
        check(plan&&plan->charges==rank+1&&plan->bonus_damage==5&&
            plan->activation_ms==1000&&plan->recharge_ms==45000);
    }
    check(!skills::effects::signet_strength(21));
    const auto plan=*skills::effects::signet_strength(4);
    const skills::effects::Scope owner{1,1,1};
    const skills::effects::StrengthCharges active{owner,50,5};
    const std::vector<skills::effects::StrengthContact> contacts{
        {{1,2,1},true},{{1,3,1},false},{{1,4,1},true},
        {{1,5,1},true},{{1,6,1},true},{{1,7,1},true},{{1,8,1},true}};
    const auto result=skills::effects::signet_strength_contacts(plan,active,contacts);
    check(result&&result->next.remaining==0&&
        result->bonus_by_contact==std::vector<unsigned>({5,0,5,5,5,5,0}));
    check(active.remaining==5); // Preflight did not mutate the live ledger.
    const auto empty=skills::effects::signet_strength_contacts(plan,active,{});
    check(empty&&empty->next.remaining==5&&empty->bonus_by_contact.empty());
    auto forged=plan;++forged.bonus_damage;
    check(!skills::effects::signet_strength_contacts(forged,active,contacts));
    auto bad=contacts;bad[1].foe=bad[0].foe;
    check(!skills::effects::signet_strength_contacts(plan,active,bad));
    bad=contacts;bad[1].foe.generation=2;bad[1].foe.actor=2;
    check(!skills::effects::signet_strength_contacts(plan,active,bad));
    bad=contacts;bad[0].foe=owner;
    check(!skills::effects::signet_strength_contacts(plan,active,bad));
    bad=contacts;bad[0].foe.instance=2;
    check(!skills::effects::signet_strength_contacts(plan,active,bad));
    auto over=active;over.remaining=6;
    check(!skills::effects::signet_strength_contacts(plan,over,contacts));
}
