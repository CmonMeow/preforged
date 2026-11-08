#include "gw_demonic_flesh.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Demonic Flesh line "+std::to_string(at.line()));}
int main()try{
 const auto plan=*demonic_flesh(12);check(plan.damage==17&&plan.duration==54000&&plan.radius==166);
 check(demonic_flesh(0)->damage==5&&demonic_flesh(15)->duration==60000&&demonic_flesh(20)->damage==25&&!demonic_flesh(21));
 const Scope caster{1,1,1},target{1,2,1};Registry registry(caster);
 check(registry.apply(caster,{7,1,130,12,Category::enchantment,100,54100},Replace::same_skill).accepted);
 const DemonicFleshUse use{caster,target,194,1,200,true,true};
 const auto trigger=demonic_flesh_trigger(registry,use);check(trigger.accepted&&trigger.effect==7&&trigger.plan==plan);
 auto changed=use;changed.success=false;check(demonic_flesh_trigger(registry,changed).accepted&&!demonic_flesh_trigger(registry,changed).plan);
 changed=use;changed.foe=false;check(!demonic_flesh_trigger(registry,changed).plan);changed.target=caster;check(demonic_flesh_trigger(registry,changed).accepted);
 changed=use;changed.target=caster;check(!demonic_flesh_trigger(registry,changed).accepted);
 changed=use;changed.skill=0;check(!demonic_flesh_trigger(registry,changed).accepted);
 changed=use;changed.event=0;check(!demonic_flesh_trigger(registry,changed).accepted);
 changed=use;changed.at=54100;check(demonic_flesh_trigger(registry,changed).accepted&&!demonic_flesh_trigger(registry,changed).plan);
 changed=use;changed.caster.generation=2;check(!demonic_flesh_trigger(registry,changed).accepted);
 Registry foreign(caster);check(foreign.apply(caster,{7,2,130,12,Category::enchantment,100,54100},Replace::same_skill).accepted);check(!demonic_flesh_trigger(foreign,use).accepted);
 std::vector<DemonicFleshRecipient> candidates{{target,0,0,0,true,true},{{1,3,1},166,0,0,true,true},{{1,4,1},166.01,0,0,true,true},
  {{1,5,1},0,0,1,true,true},{{1,6,1},0,0,0,true,false},{{1,7,1},0,0,0,false,true},{caster,0,0,0,true,false}};
 check(demonic_flesh_recipients(plan,use,0,0,0,candidates)==std::vector<Scope>{{1,3,1}});
 changed=use;changed.target={1,8,1};check(demonic_flesh_recipients(plan,changed,1000,0,0,candidates)->empty()); // Caster-centered,not selected-target centered.
 auto duplicate=candidates;duplicate.push_back(candidates.front());check(!demonic_flesh_recipients(plan,use,0,0,0,duplicate));
 duplicate=candidates;auto other_generation=candidates.front();++other_generation.owner.generation;duplicate.push_back(other_generation);check(!demonic_flesh_recipients(plan,use,0,0,0,duplicate));
 auto invalid=candidates;invalid.front().x=std::numeric_limits<double>::quiet_NaN();check(!demonic_flesh_recipients(plan,use,0,0,0,invalid));
 combat::Actor actor;actor.id=3;actor.generation=1;actor.damageable=true;actor.health=combat::Health{100,100};actor.earth_armor=500;actor.incoming_reduction=10;
 const auto hit=demonic_flesh_hit(plan,actor,20);check(hit&&hit->damage==7&&actor.health->current==93);
 actor.health=combat::Health{5,100};const auto killed=demonic_flesh_hit(plan,actor,20);check(killed&&killed->killed&&actor.health->current==0);
 const auto before=actor.health;check(!demonic_flesh_hit(plan,actor,20)&&actor.health->current==before->current);
 std::cout<<"PASS Demonic Flesh canonical plan,successful foe-skill trigger,excluded selection,caster-centered scope/plane geometry and separate protected shadow hits;pure policy verification only\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
