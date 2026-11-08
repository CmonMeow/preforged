#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace fw::gw;
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Fixture {
 world::Instance w;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 skills::effects::Scope source,target;
 explicit Fixture(bool npc=false,bool armor=true){
  if(npc){world::Instance::Npc actor;actor.id=actor.definition.id=100;actor.x=600;actor.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;
   if(armor){d.earth_armor=60;d.evidence.armor="test";}actor.combat_definition=d;w.configure_npcs({actor});}
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);source=*w.effect_scope(1);
  if(!npc){w.join(2,"Foe",600,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
   check(w.combat_armor(*w.effect_scope(2),armor?std::optional<int>{60}:std::nullopt),"target armor");}
  target=*w.effect_scope(npc?100:2);box->drain();
 }
 double health()const{return target.actor==100?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 unsigned launches(){const auto packets=box->drain();return unsigned(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>1&&p[0]==163;}));}
 bool cast(std::uint64_t at=1000){return w.dancing_daggers(source,target,*skills::effects::dancing_daggers(15),20,at);}
};
int main()try{
 for(bool npc:{false,true}){
  Fixture f(npc);check(f.cast(),"cast accepted");check(f.launches()==1&&f.health()==1000,"first missile released");
  check(f.w.dagger_stage(f.source,f.target,1300)==0,"no lead status before contact");
  f.w.tick(1334);check(f.health()==965,"first hit");
  check(f.w.dagger_stage(f.source,f.target,1334)==1,"first landed missile sets lead stage");
  f.w.tick(1500);check(f.launches()==1,"second missile released");
  f.w.tick(1660);check(f.health()==930,"second hit");
  f.w.tick(1800);check(f.launches()==1,"third missile released");
  f.w.tick(2500);check(f.health()==895,"third hit once");f.w.tick(3000);check(f.health()==895,"no fourth hit");
  check(f.w.dagger_stage(f.source,f.target,16333)==1&&f.w.dagger_stage(f.source,f.target,16334)==0,"later missiles do not refresh 15s lead stage");
  Fixture missing(npc,false);check(!missing.cast(),"unknown armor rejects");
  Fixture departed(npc);check(departed.cast(),"departing cast");departed.w.leave(1);departed.w.tick(3000);check(departed.health()==1000,"departed caster scope");
 }
 {Fixture f;check(f.cast(),"move cast");f.w.move(2,600,500,1050);f.w.tick(3000);check(f.health()==1000,"miss moving target");}
 {Fixture f;check(f.cast(),"lead miss cast");f.w.move(2,600,500,1050);f.w.tick(3000);
  check(f.w.dagger_stage(f.source,f.target,3000)==0,"miss cannot create lead chain");}
 {Fixture f;check(f.cast(),"replacement cast");f.w.leave(2);
  f.w.join(2,"Replacement",600,0,std::make_shared<world::Outbox>(),0,{}, {},1100,2);
  f.w.set_vitals(2,{1000,1000,100,100},100);f.w.tick(3000);
  check(f.w.resources(2)->health==1000,"old target generation cannot receive missiles");
 }
 {Fixture f;check(f.cast(),"late-event cast");
  check(f.w.settle_resources(f.target,1500),"later resource event");
  check(f.health()==965,"first impact settled before later input");
  f.w.tick(1400);check(f.health()==965,"stale tick cannot rewind first impact");
  f.w.tick(3000);check(f.health()==895,"remaining impacts settle once");
 }
 {Fixture f;std::vector<world::Packet> visuals;
  check(f.w.dancing_daggers(f.source,f.target,*skills::effects::dancing_daggers(15),20,1000,&visuals)&&visuals.size()==1,"deferred cast");
  f.w.tick(2000);check(f.health()==1000&&f.launches()==0,"unpublished missile dormant");
  f.w.skill_event(1,visuals);check(f.launches()==0,"untimed publication rejected");
  auto forged=visuals;forged[0][20]^=1;f.w.skill_event(1,forged,2000);check(f.launches()==0,"forged model rejected");
  f.w.skill_event(1,visuals,2000);check(f.launches()==1,"trusted launch published");
  f.w.skill_event(1,visuals,2001);check(f.launches()==0,"no duplicate publication");
  f.w.tick(3000);check(f.health()==895,"late publication shifts all three impacts");
 }
 {Fixture f;check(f.cast(),"range cast");f.w.leave(2);f.w.join(2,"Far",800,0,std::make_shared<world::Outbox>(),0,{}, {},1100,2);
  f.w.set_vitals(2,{1000,1000,100,100},100);check(f.w.combat_armor(*f.w.effect_scope(2),60),"far armor");
  check(!f.w.dancing_daggers(f.source,*f.w.effect_scope(2),*skills::effects::dancing_daggers(15),20,1200),"half range rejects");}
 std::cout<<"PASS Dancing Daggers three timed player/NPC Earth impacts, miss, range and scope checks\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
