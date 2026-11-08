#include "dagger_radius_fixture_draft.h"
#include "gw_dagger_identity_draft.h"
#include <cmath>
int main()try{
 // Base weapon mastery12 and armor60 stay fixed to isolate skill bonus ranks.
 for(bool npc:{false,true})for(unsigned rank:{0u,12u,15u,20u}){
  {Fixture f(npc);check(f.hit(782));const auto before=f.health();
   if(!npc){const auto guardian=*skills::regeneration::reviewed(258,12);
    check(f.world.apply_health_regeneration(f.target,2,guardian,1000,1000+guardian.duration,1248,true,f.target));
    f.world.configure_attack_block_draws([]{return 0.;});}
   check(f.hit(3251,1000,rank));check(f.health()==before-20-skills::effects::interpolate(10,25,rank));
   check(f.world.dagger_stage(f.source,f.target,1000)==2);
   check(f.world.take_attack_outcomes(f.source).size()==2);}
  {Fixture f(npc);f.world.join(3,"PvP edge foe",226,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
   f.world.set_vitals(3,{1000,1000,30,30},100);f.offhand();const auto before=f.health();
   check(f.hit(3061,1000,rank));const auto bonus=skills::effects::interpolate(20,45,rank);
   check(f.health()==before-8-2*(20+bonus)&&f.world.resources(3)->health==1000-2*bonus);
   const auto events=f.world.take_attack_outcomes(f.source);check(events.size()==2&&events[0].event!=events[1].event);
   check(f.world.dagger_stage(f.source,f.target,f.contact_at)==3);}
 }
 std::cout<<"PASS candidate PvP alias World identity/ranks/unblock/dual/community166; no retail rendered claim\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
