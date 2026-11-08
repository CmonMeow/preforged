#include "gw_torment_slash_npc.h"
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Torment Slash NPC line "+
        std::to_string(at.line()));
}
int main(){
    const auto plan=*skills::effects::torment_slash_npc();
    combat::Actor foe{42,1,combat::Health{500,500},std::nullopt,0,true};
    foe.physical_armor=60;
    const auto hits=skills::effects::torment_slash_target(plan,foe,20,true,true);
    check(hits&&hits->size()==2&&foe.health->current==200);
    check((*hits)[0].kind==combat::Kind::slashing&&
          (*hits)[1].kind==combat::Kind::slashing&&
          (*hits)[0].after.current==350&&(*hits)[1].before.current==350);
    check(combat::damage_packets_2022(1,(*hits)[0]).size()==3&&
          combat::damage_packets_2022(1,(*hits)[1]).size()==3);
    auto off_axis=foe;
    const auto missed=skills::effects::torment_slash_target(plan,off_axis,20,true,false);
    check(missed&&missed->empty()&&off_axis.health->current==200);
    auto lethal=foe;lethal.health->current=50;
    const auto killed=skills::effects::torment_slash_target(plan,lethal,20,true,true);
    check(killed&&killed->size()==1&&killed->front().killed&&
          lethal.health->current==0);
    auto unknown=foe;unknown.physical_armor.reset();
    check(!skills::effects::torment_slash_target(plan,unknown,20,true,true));
    check(unknown.health->current==200);
    auto forged=plan;++forged.damage_per_hit;
    check(!skills::effects::torment_slash_target(forged,foe,20,true,true));
    check(!skills::effects::torment_slash_target(plan,foe,0,true,true));
    check(!skills::effects::torment_slash_target(plan,foe,20,false,true));
    const auto tendrils=*skills::effects::torment_slash_tendrils_npc();
    check(tendrils.skill==1880&&tendrils.damage_per_hit==200&&tendrils.hits==2);
    combat::Actor tendril_foe{43,1,combat::Health{500,500},std::nullopt,0,true};
    tendril_foe.physical_armor=60;
    const auto tendril_hits=skills::effects::torment_slash_target(
        tendrils,tendril_foe,20,true,true);
    check(tendril_hits&&tendril_hits->size()==2&&tendril_foe.health->current==100);
    check((*tendril_hits)[0].kind==combat::Kind::slashing&&
          (*tendril_hits)[1].kind==combat::Kind::slashing&&
          (*tendril_hits)[0].after.current==300&&
          (*tendril_hits)[1].before.current==300);
    auto tendril_miss=tendril_foe;
    check(skills::effects::torment_slash_target(tendrils,tendril_miss,20,true,false)->empty());
    check(tendril_miss.health->current==100);
    auto tendril_lethal=tendril_foe;tendril_lethal.health->current=150;
    const auto tendril_kill=skills::effects::torment_slash_target(
        tendrils,tendril_lethal,20,true,true);
    check(tendril_kill&&tendril_kill->size()==1&&tendril_kill->front().killed);
    auto forged_variant=tendrils;forged_variant.damage_per_hit=150;
    check(!skills::effects::torment_slash_target(forged_variant,tendril_foe,20,true,true));
}
