#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Divine Spirit payment line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance world;Scope owner;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    explicit Fixture(unsigned energy=50){
        world.join(1,"Monk",0,0,box,0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);
        world.set_vitals(1,{1000,1000,energy,100},100);owner=*world.effect_scope(1);
        check(world.complete_divine_spirit(owner,*divine_spirit(0),0,100));box->drain();
    }
};
int main()try{
    for(unsigned base:{1u,5u,10u,15u,25u}){
        Fixture f;const auto cost=base<=5?1u:base-5;
        check(f.world.consume_divine_spirit_spell_energy(f.owner,290,base,200));check(f.world.resources(1)->energy==50-cost);
        check(f.world.consume_divine_spirit_spell_energy(f.owner,174,15,200)&&f.world.resources(1)->energy==35-cost);
        check(f.world.consume_divine_spirit_spell_energy(f.owner,297,0,200)&&f.world.resources(1)->energy==35-cost);
    }
    {Fixture f(4);check(!f.world.consume_divine_spirit_spell_energy(f.owner,290,10,200));check(f.world.resources(1)->energy==4);
        check(f.world.consume_divine_spirit_spell_energy(f.owner,290,5,200)&&f.world.resources(1)->energy==3);}
    {Fixture f;check(f.world.divine_spirit_spell_cost(f.owner,290,10,1099)==5);
        check(f.world.consume_divine_spirit_spell_energy(f.owner,290,10,1100)&&f.world.resources(1)->energy==40);
        check(f.world.status_snapshot(f.owner,1100)->empty());}
    {Fixture f;const auto effect=f.world.status_snapshot(f.owner,100)->front().instance;
        check(f.world.divine_spirit_spell_cost(f.owner,290,10,200)==5);
        check(f.world.status_mutation(f.owner,310,effect,0,200,{},false));
        check(f.world.consume_divine_spirit_spell_energy(f.owner,290,10,200)&&f.world.resources(1)->energy==40);}
    {Fixture f;auto stale=f.owner;++stale.generation;
        check(!f.world.consume_divine_spirit_spell_energy(stale,290,10,200));
        check(!f.world.consume_divine_spirit_spell_energy(f.owner,0,10,200));check(!f.world.consume_divine_spirit_spell_energy(f.owner,999999,10,200));
        check(!f.world.consume_divine_spirit_spell_energy(f.owner,290,0,200));check(f.world.resources(1)->energy==50);
        check(f.world.consume_divine_spirit_spell_energy(f.owner,290,10,300));
        check(!f.world.consume_divine_spirit_spell_energy(f.owner,290,10,299)&&f.world.resources(1)->energy==45);}
    {Fixture f(5);f.world.join(2,"Mesmer",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.world.set_vitals(2,{500,500,100,100},100);
        check(f.world.apply_mind_wrack(*f.world.effect_scope(2),f.owner,12,false,100));
        // Same resource clock:positive fractional regeneration would make
        // a later five-Energy debit leave a nonzero precise balance.
        check(f.world.consume_divine_spirit_spell_energy(f.owner,290,10,100));
        check(f.world.resources(1)->energy==0&&f.world.resources(1)->health==917);
        check(f.world.consume_energy(1,0,f.owner,201)&&f.world.resources(1)->health==917);}
    std::cout<<"PASS staged Divine Spirit atomic scoped cost/payment,minimum,classification,insufficient balance,expiry/strip,shared zero-Energy reaction and stale/backdated guards;modifier ordering/normal binding remain open\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
