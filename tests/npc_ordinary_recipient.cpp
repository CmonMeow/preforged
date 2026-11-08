#include "gw_npc_ordinary_recipient.h"
#include "gw_weapon_attack.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
void check(bool value){if(!value)throw std::runtime_error("ordinary NPC recipient adapter failed");}
int main()try{
    namespace o=combat::ai::ordinary;
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=101;d.binding.level=4;
    d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{100,100};
    d.damageable=true;d.physical_armor=0;combat::npcs::LiveActor target(d,{1,1,1,146,101});const auto life=target.lifetime();
    const skills::effects::Scope source{1,100,1};
    o::Reviewed profile{5,5,0,0,false,o::Delivery::melee,750,2000,128,{1200,32},"synthetic explicit ordinary profile; no inferred Charr/Rurik damage"};
    o::Ledger ledger(source,profile);combat::npcs::control::Token token{source,0,1,combat::npcs::control::Action::attack,1000,1750,{}};
    const auto recipient=o::hostile_npc_recipient(target,life,100,0,0,0,1000);check(recipient&&recipient->armor==0);
    check(ledger.accept(target.hex_scope(),token,2000));
    const o::Pose origin{source,0,0,0,true,1000};
    const auto release=ledger.prepare_release(token,origin,recipient->pose,0,true,1750);
    check(release.status==o::ReleaseStatus::ready&&ledger.commit_release(release));
    const auto impact=ledger.prepare_impact(release.attack.id,recipient->pose,true,1750);check(impact.status==o::Contact::contact);
    check(ledger.commit_impact(impact));
    const auto hit=o::apply_reviewed_npc_weapon_contact(target,life,5,0,1750);check(hit&&hit->applied==5);
    check(target.actor(life)->health->current==95&&target.health_loss_revision(life)==1);
    auto stale=life;++stale.actor_generation;check(!o::hostile_npc_recipient(target,stale,100,0,0,0,1750));
    check(!o::apply_reviewed_npc_weapon_contact(target,stale,5,0,1750));
    d.relation=combat::npcs::Relation::allied_to_players;combat::npcs::LiveActor ally(d,{1,1,1,146,101});
    check(!o::hostile_npc_recipient(ally,ally.lifetime(),100,0,0,0,1750));
    d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.physical_armor=60;
    combat::npcs::LiveActor armored(d,{1,1,1,146,101});const auto armored_life=armored.lifetime();
    const auto armored_facts=o::hostile_npc_recipient(armored,armored_life,100,0,0,0,2000);check(armored_facts&&armored_facts->armor==60);
    combat::weapon::Strike strike;strike.level=20;strike.minimum=strike.maximum=40;strike.mastery=0;
    strike.base_roll=.5;strike.miss_roll=.5;strike.location_armor=*armored_facts->armor;
    const auto resolved=combat::weapon::resolve(strike);check(resolved&&!resolved->missed);
    const auto armored_hit=o::apply_reviewed_npc_weapon_contact(armored,armored_life,resolved->damage,0,2000);
    check(armored_hit&&armored_hit->applied==resolved->damage&&armored.actor(armored_life)->health->current==1000-resolved->damage);
    std::cout<<"Generic ordinary ledger admits exact hostile NPC recipients without player-state fabrication\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
