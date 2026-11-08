#include "gw_wandering_eye.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
static void check(bool yes){if(!yes)throw std::runtime_error("Wandering Eye plan/attack classification");}
int main(){
    using namespace skills::effects;
    using combat::weapon::AttackActionFacts;
    using combat::weapon::AttackTargetAction;
    const auto low=wandering_eye(0), middle=wandering_eye(12), high=wandering_eye(15);
    check(low&&low->damage==30&&middle&&middle->damage==94&&high&&high->damage==110);
    check(low->duration==4000&&low->range==1248.f&&low->nearby==252.f);
    check(!wandering_eye(21));
    check(wandering_eye_attack_trigger(*middle,{AttackTargetAction::idle,{}})==false);
    check(wandering_eye_attack_trigger(*middle,{AttackTargetAction::attack,{}})==true);
    check(wandering_eye_attack_trigger(*middle,{AttackTargetAction::attack,skills::Slot{322,0}})==true);
    check(wandering_eye_attack_trigger(*middle,{AttackTargetAction::skill,skills::Slot{322,0}})==true);
    check(wandering_eye_attack_trigger(*middle,{AttackTargetAction::skill,skills::Slot{830,0}})==false);
    check(!wandering_eye_attack_trigger(*middle,{AttackTargetAction::skill,skills::Slot{65535,0}}));
    auto forged=*middle;++forged.damage;
    check(!wandering_eye_attack_trigger(forged,{AttackTargetAction::attack,{}}));
    const Scope caster{1,1,1},hexed{1,42,2},near{1,43,1},ally{1,44,1},far{1,45,1};
    Registry empty(hexed);
    const auto applied=prepare_wandering_eye(empty,caster,hexed,7,*middle,100,4000);
    check(applied&&applied->entry.skill==2056&&applied->entry.expires==4100&&
          applied->registry.newest_skill(hexed,2056,100)&&
          !empty.newest_skill(hexed,2056,100));
    check(!prepare_wandering_eye(empty,caster,hexed,7,*middle,UINT64_MAX,4000));
    check(!prepare_wandering_eye(empty,caster,ally,7,*middle,100,4000));
    const auto renewed=prepare_wandering_eye(applied->registry,caster,hexed,8,*middle,200,4000);
    check(renewed&&renewed->replaced.size()==1&&renewed->replaced[0].instance==7&&
          renewed->registry.newest_skill(hexed,2056,200)->instance==8);
    const auto consumed=consume_wandering_eye(renewed->registry,hexed,250);
    check(consumed&&consumed->entry.instance==8&&
          !consumed->registry.newest_skill(hexed,2056,250)&&
          renewed->registry.newest_skill(hexed,2056,250));
    check(!consume_wandering_eye(consumed->registry,hexed,250));
    check(!consume_wandering_eye(renewed->registry,hexed,4200));
    std::vector<WanderingEyeCandidate> area{
        {far,253,true,true,true},{near,150,true,true,true},
        {ally,100,true,true,false},{hexed,0,true,true,true}};
    check(wandering_eye_targets(*middle,caster,hexed,area)==
          std::optional(std::vector<Scope>{hexed,near}));
    area[1].hostile_to_caster.reset();check(!wandering_eye_targets(*middle,caster,hexed,area));
    area[1].hostile_to_caster=true;area.push_back(area[1]);
    check(!wandering_eye_targets(*middle,caster,hexed,area));
    area.pop_back();area[3].alive=false;
    check(!wandering_eye_targets(*middle,caster,hexed,area));
    std::vector<combat::Actor> victims{
        {42,2,combat::Health{100,100},{},0,true},
        {43,1,combat::Health{50,100},{},0,true}};
    const auto hits=wandering_eye_hits(*middle,20,{hexed,near},victims);
    check(hits&&hits->size()==2&&(*hits)[0].after.current==6&&
          (*hits)[1].after.current==0&&(*hits)[1].killed);
    check(victims[0].health->current==100&&victims[1].health->current==50);
    victims[1].health.reset();
    check(!wandering_eye_hits(*middle,20,{hexed,near},victims));
    std::cout<<"PASS Wandering Eye canonical rank plan and trusted attack trigger; World binding pending\n";
}
