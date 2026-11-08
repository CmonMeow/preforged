// Unregistered community-policy166 edge regression; NOT measured retail proof.
// Original current156 characterization is preserved in dagger_radius_edges_draft.cpp.
#include "dagger_radius_fixture_draft.h"
#include "gw_world_adjacent_policy_draft.h"
int main()try{
 static_assert(world::geometry::adjacent_center_community==166.f);
 check(skills::find_skill(775)->aoe_range==156&&skills::find_skill(777)->aoe_range==156);
 for(float edge:{155.f,156.f,160.f,166.f,166.01f}){
  {Fixture f(false);f.world.join(3,"Edge foe",60+edge,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
   f.world.set_vitals(3,{1000,1000,30,30},100);f.offhand();check(f.hit(775));
   const auto actual=f.world.resources(3)->health;const bool community166=edge<=166;
   check(actual==(community166?960u:1000u));
   std::cout<<"DeathBlossom edge="<<edge<<" health="<<actual<<" alternate166_included="<<(edge<=166)<<'\n';}
  {Fixture f(false);f.world.join(3,"Edge ally",60+edge,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
   f.world.set_vitals(3,{1000,1000,30,30},100);f.offhand();check(f.hit(777,4000));
   const bool knocked=f.world.knocked_down(f.target,f.contact_at);check(knocked==(edge>166));
   std::cout<<"Horns edge="<<edge<<" knocked="<<knocked<<" alternate166_suppresses="<<(edge<=166)<<'\n';}
 }
 std::cout<<"Community166-centre policy; catalog156 retained; no invented body expansion; retail edge unmeasured\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
