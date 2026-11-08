#include "gw_disrupting_dagger.h"
#include "gw_skill_particle_wire.h"
#include "gw_interrupt_skills.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace fw::gw::skills;
namespace combat=fw::gw::combat;
static void check(bool ok){if(!ok)throw std::runtime_error("Disrupting Dagger policy");}
int main(){try{
    for(unsigned rank=0;rank<=20;++rank){
        const auto plan=effects::disrupting_dagger(rank);
        check(bool(plan)&&plan->rank==rank&&plan->range==624.f);
        check(std::abs(plan->earth_damage-(10.+25.*rank/15.))<0.00001);
    }
    check(!effects::disrupting_dagger(21));
    check(!effects::disrupting_dagger_interrupts(0));
    check(!effects::disrupting_dagger_interrupts(3433));
    check(effects::disrupting_dagger_interrupts(23)); // spell
    check(effects::disrupting_dagger_interrupts(325)); // attack skill
    check(effects::disrupting_dagger_interrupts(377)); // stance
    check(!effects::interrupt_skill(571)); // Never a completion-time Runtime route.
    const auto npc_interrupt=*effects::disrupting_dagger_interrupt_plan(12);
    check(effects::valid_interrupt(npc_interrupt)&&npc_interrupt.range==624.f);
    check(effects::interrupt_matches(npc_interrupt,325,false));
    check(effects::interrupt_matches(npc_interrupt,377,false));
    check(!effects::interrupt_matches(npc_interrupt,0,true));
    const auto plan=*effects::disrupting_dagger(12);
    combat::Actor target{2,1,combat::Health{500,500},60,0,true};
    const auto idle=effects::disrupting_dagger_impact(plan,target,20,0,true);
    check(idle.accepted&&!idle.interrupt_candidate&&idle.hit.kind==combat::Kind::earth);
    check(idle.target.health->current<500&&idle.hit.applied>0);
    const auto active=effects::disrupting_dagger_impact(plan,target,20,325,true);
    check(active.accepted&&active.interrupt_candidate&&
          std::abs(active.hit.applied-idle.hit.applied)<0.00001);
    target.earth_armor=100;
    const auto armored=effects::disrupting_dagger_impact(plan,target,20,23,true);
    check(armored.accepted&&armored.hit.applied<active.hit.applied);
    target.earth_armor.reset();
    check(!effects::disrupting_dagger_impact(plan,target,20,23,true).accepted);
    target.earth_armor=60;
    check(!effects::disrupting_dagger_impact(plan,target,20,23,false).accepted);
    target.generation=0;
    check(!effects::disrupting_dagger_impact(plan,target,20,23,true).accepted);
    auto forged=plan;forged.range=1248.f;
    target.generation=1;
    check(!effects::disrupting_dagger_impact(forged,target,20,23,true).accepted);
    const auto visual=particles::targeted_body_2022(2,1,571);
    check(visual==combat::Packet{159,0,20,0,0,0,2,0,0,0,1,0,0,0,118,4,0,0});
    check(!particles::targeted_body_2022(0,1,571));
    const auto launch=particles::spell_projectile_launch_2022(25,-5768.f,1978.f,0,
        std::bit_cast<float>(0x3ceac009u),571);
    check(launch.size()==29&&launch[0]==163&&launch[20]==0x75&&launch[21]==4);
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
