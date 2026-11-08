#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Disease provenance line "+std::to_string(at.line()));
}
int main()try{
 using namespace combat::npcs;using combat::conditions::Kind;
 Definition d;d.binding.map=146;d.binding.actor=100;d.spawn_health=combat::Health{10,100};d.conditions_allowed=d.damageable=true;
 d.relation=Relation::hostile_to_players;
 const Lifetime life{1,1,7,146,100};LiveActor live(d,life);
 const skills::effects::Scope origin{1,1,2};
 check(live.apply_condition(life,Kind::disease,1,100,2000));
 check(!live.disease_credit(life,100)&&live.record_disease_credit(life,origin,100));
 auto stale=life;++stale.actor_generation;
 check(!live.disease_credit(stale,100)&&!live.record_disease_credit(stale,origin,100));
 for(const auto invalid:std::vector<skills::effects::Scope>{{2,1,2},{1,2,2},{1,1,0}}){
  check(!live.record_disease_credit(life,invalid,100)&&live.disease_credit(life,100)==origin);
 }
 check(live.apply_condition(life,Kind::disease,2,200,500));
 check(live.disease_credit(life,200)==origin);
 const auto fatal=live.advance_conditions(life,3000);
 check(fatal&&fatal->killed&&fatal->condition_kind==Kind::disease&&fatal->condition_credit==origin);
 check(fatal->death_time==1350&&!live.disease_credit(life,3000));
 LiveActor renewed(d,life);
 check(renewed.apply_condition(life,Kind::disease,1,100,500)&&renewed.record_disease_credit(life,origin,100));
 check(renewed.apply_condition(life,Kind::disease,1,200,1500)&&!renewed.disease_credit(life,200));
 check(renewed.record_disease_credit(life,origin,200)&&renewed.remove_condition(life,Kind::disease,200));
 check(!renewed.disease_credit(life,200));
 check(renewed.apply_condition(life,Kind::disease,1,200,1500)&&!renewed.disease_credit(life,200));
 check(renewed.record_disease_credit(life,origin,200)&&renewed.clear_conditions(life,200));
 check(!renewed.disease_credit(life,200)&&!renewed.record_disease_credit(life,origin,200));
 LiveActor unknown(d,life);check(unknown.apply_condition(life,Kind::disease,1,100,2000));
 const auto uncredited=unknown.advance_conditions(life,3000);
 check(uncredited&&uncredited->killed&&uncredited->condition_kind==Kind::disease&&!uncredited->condition_credit);
 LiveActor mixed(d,life);
 check(mixed.apply_condition(life,Kind::disease,1,100,2000)&&mixed.record_disease_credit(life,origin,100));
 check(mixed.apply_condition(life,Kind::poison,2,100,2000)&&mixed.disease_credit(life,100)==origin);
 const auto poison_death=mixed.advance_conditions(life,3000);
 check(poison_death&&poison_death->killed&&poison_death->condition_kind==Kind::poison&&poison_death->cause==2&&!poison_death->condition_credit);
 LiveActor replacement(d,life);
 check(replacement.apply_condition(life,Kind::disease,1,100,500)&&replacement.record_disease_credit(life,origin,100));
 check(replacement.apply_condition(life,Kind::disease,2,200,1500)&&!replacement.disease_credit(life,200));
 check(!replacement.record_disease_credit(life,origin,200));
 const skills::effects::Scope second{1,2,3};check(replacement.record_disease_credit(life,second,200)&&replacement.disease_credit(life,200)==second);
 std::cout<<"PASS Disease infection-specific lifetime provenance,short renewal,extension invalidation,cleanse/expiry,death crossing and unknown credit\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
