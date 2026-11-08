#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Watch World line "+std::to_string(at.line()));}
struct Fixture{world::Instance w;Scope source,target,foe;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(){for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*30),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{10000,10000,100,100},100);}check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));source=*w.effect_scope(1);target=*w.effect_scope(2);foe=*w.effect_scope(3);check(w.lightning_armor(target,60));}
};
int main()try{
 for(bool pvp:{false,true})for(unsigned rank=0;rank<=20;++rank){Fixture f;const auto plan=*watch_yourself(rank,pvp);
  check(f.w.stage_watch_yourself_recipient(f.source,f.target,plan,100));const auto entry=f.w.status_snapshot(f.target,100)->front();
  check(entry.skill==plan.skill&&entry.expires==10100&&f.w.watch_yourself_armor(f.target,100)==plan.armor);f.box->drain();
  for(unsigned n=0;n<plan.charges;++n){const auto before=f.w.resources(2)->health;check(f.w.lightning(f.foe,f.target,*lightning(224,12),20,100));
   const auto loss=before-f.w.resources(2)->health;check(std::abs(double(loss)-45*std::exp2(-.75*(60.+plan.armor-80)/40))<1.1);
   const WatchYourselfDamage receipt{f.target,n+1,100,true,true,double(loss)};check(!f.w.consume_watch_yourself_damage(receipt));
   check(f.w.watch_yourself_armor(f.target,100)==(n+1==plan.charges?0u:plan.armor));}
  check(f.w.watch_yourself_armor(f.target,100)==0&&f.w.status_snapshot(f.target,100)->empty());
  world::Packet end{67,0};combat::detail::put(end,2);combat::detail::put(end,entry.instance);const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),end)==1);
 }
 {Fixture f;check(f.w.stage_watch_yourself_recipient(f.source,f.target,*watch_yourself(12),100));
  check(f.w.stage_watch_yourself_recipient(f.source,f.target,*watch_yourself(0,true),200));const auto entries=*f.w.status_snapshot(f.target,200);check(entries.size()==1&&entries.front().skill==2858);
  f.w.tick(10200);check(f.w.watch_yourself_armor(f.target,10200)==0&&f.w.status_snapshot(f.target,10200)->empty());
 }
 {Fixture f;auto stale=f.target;++stale.generation;check(!f.w.stage_watch_yourself_recipient(f.source,stale,*watch_yourself(12),100));
  check(!f.w.stage_watch_yourself_recipient(f.source,f.foe,*watch_yourself(12),100));check(!f.w.status_mutation(f.target,348,123,12,100,10100,true));
  check(f.w.stage_watch_yourself_recipient(f.source,f.target,*watch_yourself(12),100));check(f.w.kill(f.target,101));check(f.w.watch_yourself_armor(f.target,101)==0);
 }
 {world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{500,1000};d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});
  auto box=std::make_shared<world::Outbox>();w.join(1,"Actor",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
  check(w.stage_watch_yourself_recipient(source,target,*watch_yourself(12),100)&&w.watch_yourself_armor(target,100)==21);
  check(w.stage_watch_yourself_recipient(source,target,*watch_yourself(0,true),200));
  const auto entries=*w.status_snapshot(target,200);check(entries.size()==1&&entries.front().skill==2858&&w.watch_yourself_armor(target,200)==5);
  box->drain();w.tick(10200);check(w.watch_yourself_armor(target,10200)==0&&w.status_snapshot(target,10200)->empty());
  world::Packet end{67,0};combat::detail::put(end,100);combat::detail::put(end,entries.front().instance);const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),end)==1);
 }
 for(bool pvp:{false,true})for(unsigned rank=0;rank<=20;++rank){Fixture f;const auto plan=*watch_yourself(rank,pvp);
  check(f.w.stage_watch_yourself_party(f.source,plan,100));
  check(f.w.watch_yourself_armor(f.source,100)==plan.armor&&f.w.watch_yourself_armor(f.target,100)==plan.armor&&f.w.watch_yourself_armor(f.foe,100)==0);
  check(f.w.stage_watch_yourself_party(f.target,*watch_yourself(0,!pvp),200));
  for(auto scope:{f.source,f.target}){const auto entries=*f.w.status_snapshot(scope,200);check(entries.size()==1&&entries.front().skill==(!pvp?2858u:348u));}
 }
 {Fixture f;auto stale=f.source;++stale.generation;check(!f.w.stage_watch_yourself_party(stale,*watch_yourself(12),100));
  check(f.w.settle_resources(f.target,200));
  check(!f.w.stage_watch_yourself_party(f.source,*watch_yourself(12),100));
  check(f.w.status_snapshot(f.source,100)->empty());
 }
 {world::Instance w;for(unsigned id=1;id<=5;++id){w.join(id,"Actor",id==2?1012.f:id==3?1013.f:0.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{10000,10000,100,100},100);}
  for(unsigned id:{2u,3u,4u})check(w.party_action(1,158,id,100)&&w.party_action(id,154,1,100));
  const auto source=*w.effect_scope(1);check(w.kill(*w.effect_scope(4),100));check(w.stage_watch_yourself_party(source,*watch_yourself(12),100));
  for(unsigned id=1;id<=5;++id)check(w.watch_yourself_armor(*w.effect_scope(id),100)==(id<=2?21u:0u));
 }
 std::cout<<"PASS42Watch Yourself World player armor/damage and explicit post-commit receipts,one end,player/NPC alias refresh/expiry,death/scope/injection guards;automatic damage/shout paths pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}




