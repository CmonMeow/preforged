#include "gw_world_simulation.h"
#include "gw_feast_of_corruption.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Feast selected-anchor regression");}
int main()try{
 world::Instance world;
 for(unsigned id:{1u,2u}){world.join(id,"Actor",id==1?0.f:60.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);world.set_vitals(id,{500,500,100,100},100);}
 const auto caster=*world.effect_scope(1),target=*world.effect_scope(2);
 const auto observe=[&](std::uint64_t at){const auto p=world.position(2,at);return FeastAnchorObservation{*world.effect_scope(2),p.first,p.second,world.plane(2),world.resources(2)->health>0,at};};
 const auto anchor=acquire_feast_anchor(caster,observe(100),100);check(bool(anchor));
 check(resolve_feast_anchor(*anchor,caster,observe(150),150).has_value()&&!resolve_feast_anchor(*anchor,caster,observe(100),150));
 check(world.kill(target,200));const auto corpse=observe(200);check(!corpse.alive);
 const auto resolved=resolve_feast_anchor(*anchor,caster,corpse,2100);check(resolved&&!resolved->alive&&resolved->target==target&&resolved->x==60);
 check(!acquire_feast_anchor(caster,corpse,200)&&!resolve_feast_anchor(*anchor,caster,{},2100));
 auto moved=corpse;moved.x=120;moved.y=40;moved.observed=2100;check(resolve_feast_anchor(*anchor,caster,moved,2100)->x==120); // Current observation,not frozen60.
 moved.observed=2101;check(!resolve_feast_anchor(*anchor,caster,moved,2100));moved.observed=99;check(!resolve_feast_anchor(*anchor,caster,moved,2100));moved.observed=2100;
 check(!resolve_feast_anchor(*anchor,caster,moved,99));
 moved.x=std::numeric_limits<double>::infinity();check(!resolve_feast_anchor(*anchor,caster,moved,2100));
 world.leave(2);world.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},300,2);world.set_vitals(2,{500,500,100,100},100);
 check(*world.effect_scope(2)!=target&&!resolve_feast_anchor(*anchor,caster,observe(2100),2100));
 world.leave(1);world.join(1,"New caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},300,1);
 check(!resolve_feast_anchor(*anchor,*world.effect_scope(1),corpse,2100));
 auto malformed=*anchor;malformed.selected=caster;check(!resolve_feast_anchor(malformed,caster,corpse,2100));
 std::cout<<"PASS Feast original selected actor corpse retained,current location used,missing/replaced actors and invalid clocks/positions rejected;casting/world AoE not yet wired\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
