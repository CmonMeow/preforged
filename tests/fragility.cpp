#include "gw_fragility.h"
#include <stdexcept>
void check(bool v){if(!v)throw std::runtime_error("Fragility transition regression");}
int main(){using namespace fw::gw;using namespace skills::effects;using combat::conditions::Kind;
 const auto pve=*fragility(19,12),pvp=*fragility(19,12,true);
 check(pve.damage==17&&pve.duration==18000&&pve.radius==166);
 check(pvp.skill==2998&&pvp.damage==17&&pvp.duration==18000&&pvp.radius==0);
 check(fragility(19,0)->damage==5&&fragility(19,20)->damage==25&&!fragility(19,21));
 combat::conditions::State before,after;
 check(after.apply(Kind::blind,1,100,1000));
 auto changed=immediate_condition_transitions(before,after,100);
 check(changed.size()==1&&changed[0].gained&&changed[0].kind==Kind::blind&&changed[0].at==100);
 before=after;check(after.apply(Kind::blind,2,200,2000));
 check(immediate_condition_transitions(before,after,200).empty());
 before=after;check(after.remove(Kind::blind));
 changed=immediate_condition_transitions(before,after,300);
 check(changed.size()==1&&!changed[0].gained&&changed[0].at==300);
 check(before.apply(Kind::burning,1,200,500));
 check(before.apply(Kind::poison,1,200,1000));
 auto expired=expired_condition_transitions(before,200,2200);
 check(expired.size()==3&&expired[0].at==700&&expired[1].at==1200&&expired[2].at==2200);
 check(expired_condition_transitions(before,2200,2300).empty());
 check(expired_condition_transitions(before,2300,2200).empty());
 after=before;after.expire(2200);
 // An expiry is one loss at its deadline, not another loss when the later
 // state commit clears the now-inactive record.
 check(immediate_condition_transitions(before,after,2200).empty());
 check(after.apply(Kind::blind,1,2200,100));
 changed=immediate_condition_transitions(before,after,2200);
 check(changed.size()==1&&changed[0].gained);
 Scope target{1,42,7};Registry registry(target);
 check(registry.apply(target,{8,43,19,12,Category::hex,100,18100},Replace::same_skill).accepted);
 combat::Actor actor{42,7,combat::Health{100,100},200,0,true};
 const auto gain=*fragility_reaction(registry,target,{Kind::blind,true,200});
 const auto hit=*fragility_damage(gain,actor,20);
 check(hit.damage==17&&hit.after.current==83&&hit.skill==19);
 // Loss is an independent trigger with the same armor-ignoring damage.
 const auto loss=*fragility_reaction(registry,target,{Kind::blind,false,300});
 check(fragility_damage(loss,actor,20)->after.current==83);
 check(!fragility_reaction(registry,{1,42,8},{Kind::blind,true,200}));
 check(!fragility_reaction(registry,target,{Kind::blind,false,18100}));
 auto stale=actor;++stale.generation;check(!fragility_damage(gain,stale,20));
 auto forged=gain;++forged.plan.damage;check(!fragility_damage(forged,actor,20));
 forged=gain;forged.transition.at=18100;check(!fragility_damage(forged,actor,20));
 actor.health=combat::Health{10,100};check(fragility_damage(gain,actor,20)->killed);
 combat::conditions::State empty,all;
 for(unsigned i=0;i<10;++i)check(all.apply(static_cast<Kind>(i),43,100,1000));
 auto acquired=immediate_condition_transitions(empty,all,100);
 check(acquired.size()==10);
 unsigned total=0;actor.health=combat::Health{1000,1000};
 for(const auto& event:acquired){
    const auto reaction=fragility_reaction(registry,target,event);check(bool(reaction));
    const auto damage=fragility_damage(*reaction,actor,20);check(bool(damage));total+=static_cast<unsigned>(damage->damage);
 }
 check(total==170);
 const auto refreshed=all;
 for(unsigned i=0;i<10;++i)check(all.apply(static_cast<Kind>(i),44,200,2000));
 check(immediate_condition_transitions(refreshed,all,200).empty());
 auto cleansed=all;cleansed.clear();
 check(immediate_condition_transitions(all,cleansed,300).size()==10);
 // A delayed tick does not move a loss beyond the hex's actual lifetime.
 Registry short_hex(target);
 check(short_hex.apply(target,{9,43,19,12,Category::hex,100,1500},Replace::same_skill).accepted);
 const auto earlier=expired_condition_transitions(refreshed,100,2200);
 check(earlier.size()==10);
 for(const auto& event:earlier)check(bool(fragility_reaction(short_hex,target,event)));
 const auto later=expired_condition_transitions(all,200,2200);
 check(later.size()==10);
 for(const auto& event:later)check(!fragility_reaction(short_hex,target,event));
}
