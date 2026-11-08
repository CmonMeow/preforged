#include "gw_world_simulation.h"
#include <iostream>
#include <stdexcept>

using namespace fw::gw;

static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static unsigned count_opcode(const std::vector<world::Packet>& packets,unsigned opcode){
    unsigned count=0;
    for(const auto& packet:packets)if(packet.size()>=2&&(unsigned(packet[0])|(unsigned(packet[1])<<8))==opcode)++count;
    return count;
}

int main()try{
    world::Instance instance;
    auto caster_packets=std::make_shared<world::Outbox>();
    auto target_packets=std::make_shared<world::Outbox>();
    instance.join(1,"Caster",0,0,caster_packets);
    instance.join(2,"Corpse",100,0,target_packets);
    for(unsigned id:{1u,2u})instance.set_vitals(id,{480,480,40,40},100);
    check(instance.party_action(1,158,2,0)&&instance.party_action(2,154,1,0),"party setup");
    const auto source=*instance.effect_scope(1),target=*instance.effect_scope(2);
    check(instance.kill(target,1000),"corpse setup");
    caster_packets->drain();target_packets->drain();
    const auto plan=*skills::effects::resurrection(1816,7);
    check(plan.health_percent==100&&plan.energy_percent==70&&plan.morale_recharge,"rank seven effect plan");
    check(instance.resurrect_target(source,target,plan,1000),"Sunspear revive");
    check(instance.resources(2)->health==480&&instance.resources(2)->energy==28,"full health and seventy percent energy");
    check(instance.position(2,1000)==instance.position(1,1000),"corpse shadow steps to caster");
    check(count_opcode(target_packets->drain(),44)==1&&count_opcode(caster_packets->drain(),44)==1,"native position packet delivered");
    auto forged=plan;forged.energy_percent=100;
    check(instance.kill(target,2000)&&!instance.resurrect_target(source,target,forged,2000),"forged plan rejected");
    check(instance.resources(2)->health==0,"failed revive preserves death");
    std::cout<<"Sunspear Rebirth world passed\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
