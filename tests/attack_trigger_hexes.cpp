#include "gw_attack_trigger_hexes.h"
#include <stdexcept>
void check(bool v){if(!v)throw std::runtime_error("attack-trigger hex regression");}
int main(){using namespace fw::gw;using namespace skills::effects;
 Scope target{1,42,7};Registry registry(target);
 for(unsigned id:{43u,47u}){
    const auto plan=*attack_hex(id,12);
    check(plan.damage==(id==43?76u:114u)&&plan.duration==4000&&plan.radius==166);
    check(plan.interrupt==(id==43)&&plan.blind_duration==(id==47?10000u:0u));
    check(!attack_hex(id,21));
    check(registry.apply(target,{id,43,id,12,Category::hex,100,4100},Replace::same_skill).accepted);
    check(!consume_attack_hex(registry,{1,42,8},id,200));
    check(!consume_attack_hex(registry,target,id,99));
    const auto reaction=consume_attack_hex(registry,target,id,200);check(bool(reaction));
    check(!consume_attack_hex(registry,target,id,200));
    combat::Actor actor{42,7,combat::Health{1000,1000},200,0,true};
    check(attack_hex_damage(*reaction,actor,20)->after.current==1000-plan.damage);
    ++actor.generation;check(!attack_hex_damage(*reaction,actor,20));
    auto forged=*reaction;++forged.plan.damage;--actor.generation;
    check(!attack_hex_damage(forged,actor,20));
    check(registry.apply(target,{id,43,id,12,Category::hex,100,4100},Replace::same_skill).accepted);
    check(!consume_attack_hex(registry,target,id,4100));registry.expire(target,4100);
 }
 check(!attack_hex(19));
 const auto shackles=*attack_hex(66,12);
 check(!shackles.consume&&!shackles.interrupt&&shackles.energy_loss==5&&shackles.damage==0&&shackles.duration==17000&&shackles.radius==0);
 check(registry.apply(target,{66,43,66,12,Category::hex,100,17100},Replace::same_skill).accepted);
 const auto first=consume_attack_hex(registry,target,66,200),second=consume_attack_hex(registry,target,66,300);
 check(first&&second&&first->cause.instance==second->cause.instance&&registry.size()==1);
 check(!consume_attack_hex(registry,target,66,17100));
}
