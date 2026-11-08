#include "gw_npc_catalog.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Panic World line "+std::to_string(at.line()));}
int main()try{using namespace skills;using namespace effects;
 for(unsigned mode=0;mode<5;++mode){
  const std::string hash(64,'a');std::ostringstream a,m;a<<"gw-npcs 1\n";m<<"gw-npc-combat 2 "<<hash<<"\nsource test authored "<<hash<<" tests/panic_world.cpp\n";
  for(unsigned id:{100u,101u,102u}){const int x=id==100?0:id==101?252:253;a<<"npc 146 "<<id<<" 1 1234 0 1677721600 0 8 1 1 0 "<<x<<" 0 0 - -\n";m<<"npc 146 "<<id<<" 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test\n";}
  std::istringstream appearances(a.str()),metadata(m.str());auto population=world::load_npc_catalog(appearances);world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
  world::Instance w;w.configure_npcs(population.for_map(146));auto box=std::make_shared<world::Outbox>();
  for(unsigned id=1;id<=5;++id){float x=id==1?-100:id==2?0:id==3?252:id==4?253:0;w.join(id,"Actor",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},mode==2&&id==3?300:100,id==1||id==5?1:2);w.set_vitals(id,{500,500,100,100},100);}
  const auto caster=*w.effect_scope(1),selected=*w.effect_scope(mode==1?100:2);
  if(mode==2||mode==3)check(w.hex_breaker(selected,*hex_breaker(12),20,100));
  box->drain();check(w.apply_panic(caster,selected,12,200)==(mode!=2));
  const auto p2=*w.status_snapshot(*w.effect_scope(2),200),p3=*w.status_snapshot(*w.effect_scope(3),mode==2?300:200);
  if(mode==2){check(w.resources(1)->health==500&&p2.size()==1&&p2.front().skill==10&&p3.empty());check(!(w.npc_combat_snapshot(*w.effect_scope(100))->effects&0x800));continue;}
  check((mode==3?p2.empty():p2.size()==1&&p2.front().skill==52));check(p3.size()==1&&p3.front().skill==52&&p3.front().expires==8200);
  check(w.status_snapshot(*w.effect_scope(4),200)->empty()&&w.status_snapshot(*w.effect_scope(5),200)->empty());
  check((w.npc_combat_snapshot(*w.effect_scope(100))->effects&0x800)&&(w.npc_combat_snapshot(*w.effect_scope(101))->effects&0x800)&&!(w.npc_combat_snapshot(*w.effect_scope(102))->effects&0x800));
  const auto packets=box->drain();const auto body=*particles::targeted_body_2022(selected.actor,1,52);check(std::count(packets.begin(),packets.end(),body)==(mode==3?0:1));
  check(w.resources(1)->health==(mode==3?461u:500u));
  if(mode==4){w.join(6,"Replacement caster",-100,0,std::make_shared<world::Outbox>(),0,{}, {},300,1);w.set_vitals(6,{500,500,100,100},100);
   check(w.apply_panic(*w.effect_scope(6),selected,0,400));const auto renewed=*w.status_snapshot(*w.effect_scope(3),400);
   check(renewed.size()==1&&renewed.front().cause==6&&renewed.front().rank==0&&renewed.front().expires==1400);
   w.tick(1400);check(w.status_snapshot(*w.effect_scope(3),1400)->empty()&&!(w.npc_combat_snapshot(*w.effect_scope(100))->effects&0x800));continue;
  }
  w.tick(4200);check(w.resources(3)->health==500&&w.npc_combat_snapshot(*w.effect_scope(100))->health->current==500);
  w.tick(8200);check(w.status_snapshot(*w.effect_scope(3),8200)->empty()&&!(w.npc_combat_snapshot(*w.effect_scope(100))->effects&0x800));
  auto stale=selected;++stale.generation;check(!w.apply_panic(caster,stale,12,8300));
 }
 std::cout<<"PASS Panic mixed player/NPC application,252 boundary, selected body87, zero degeneration, expiry, Hex Breaker and atomic invalid-recipient rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
