#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include "gw_npc_elemental_ai.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Flame Burst line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=skills::effects::elemental(188,rank);
  check(p&&p->skill==188&&p->caster_center&&p->radius==252&&p->kind==combat::Kind::fire&&
        p->damage==skills::effects::damage_scale(15,120,rank)&&skills::effects::valid_elemental(*p));}
 auto forged=*skills::effects::elemental(188,12);forged.radius=166;check(!skills::effects::valid_elemental(forged));
 world::Instance w;std::vector<world::Instance::Npc> npcs;
 for(unsigned id=100;id<=102;++id){world::Instance::Npc n;n.id=n.definition.id=id;
  n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  n.x=id==101?300.f:80.f;
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
  d.relation=id==102?combat::npcs::Relation::protected_ally:combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{480,480};d.damageable=id!=102;d.conditions_allowed=id!=102;d.fire_armor=60;
  n.combat_definition=d;npcs.push_back(n);
 }
 w.configure_npcs(npcs);auto box=std::make_shared<world::Outbox>();
 w.join(1,"Caster",0,0,box,0,{}, {},100,1);
 w.join(2,"Nearby",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
 w.join(3,"Edge",250,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
 w.join(4,"Beyond",253,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
 w.join(5,"Ally",120,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
 for(unsigned id=1;id<=5;++id)w.set_vitals(id,{480,480,100,100},100);
 for(unsigned id=2;id<=5;++id)check(w.elemental_armor(*w.effect_scope(id),60,60));
 const auto caster=*w.effect_scope(1);
 check(w.elemental(caster,caster,*skills::effects::elemental(188,12),20,100));
 check(w.resources(2)->health==381&&w.resources(3)->health==381&&
       w.resources(4)->health==480&&w.resources(5)->health==480);
 check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==381&&
       w.npc_combat_snapshot(*w.effect_scope(101))->health->current==480&&
       w.npc_combat_snapshot(*w.effect_scope(102))->health->current==480);
 for(unsigned id=2;id<=5;++id)w.set_vitals(id,{480,480,100,100},100);
 skills::Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[188]=1;
 skills::Slots bar{};bar[0]={188,0};attributes::Ranks ranks{};ranks[10]=12;
 skills::casting::Runtime runtime(1);skills::casting::Hooks hooks;unsigned releases=0;
 hooks.events=[](const auto&){};
 hooks.resources=[&](auto id){return w.resources(id);};
 hooks.target_scope=[&](auto id){return w.effect_scope(id);};
 hooks.consume_energy_at=[&](auto cost,auto at){return w.consume_energy(1,cost,caster,at);};
 hooks.target_valid=[](auto id,float,bool allow_self){return id==1&&allow_self;};
 hooks.elemental=[&](auto target,const auto& plan,auto at){++releases;check(target==caster&&plan.skill==188);return w.elemental(caster,target,plan,20,at);};
 check(runtime.start(1000,{188,0,0},bar,policy,ranks,true,hooks)==skills::casting::Reason::accepted);
 check(w.resources(1)->energy==85); // 15 Energy, no target selected.
 runtime.update(1749,hooks);check(releases==0);
 runtime.update(1750,hooks);check(releases==1&&w.resources(2)->health==381&&w.resources(3)->health==381);
 check(runtime.start(6749,{188,0,0},bar,policy,ranks,true,hooks)==skills::casting::Reason::recharge);
 {
  world::Instance npc_world;auto npc_box=std::make_shared<world::Outbox>();
  world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;
  n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};
  d.spawn_energy=combat::npcs::Energy{50,50};d.energy_pips=0;d.damageable=true;d.conditions_allowed=true;
  n.combat_definition=d;npc_world.configure_npcs({n});
  for(unsigned id=1;id<=3;++id){npc_world.join(id,"Recipient",id==1?100.f:id==2?250.f:300.f,0,
      id==1?npc_box:std::make_shared<world::Outbox>(),0,{}, {},100,1);
   npc_world.set_vitals(id,{1000,1000,0,100},100);
   check(npc_world.elemental_armor(*npc_world.effect_scope(id),60,60));}
  combat::ai::FlarePolicy npc_policy;npc_policy.source=*npc_world.effect_scope(100);
  npc_policy.level=20;npc_policy.fire_magic=12;npc_policy.speed=300;
  npc_policy.acquire=1000;npc_policy.follow=1500;npc_policy.leash=1500;
  npc_policy.flight={100,32};npc_policy.evidence="authored Flame Burst test actor and loadout";
  std::vector<combat::ai::elemental::Reviewed> spells{{{188,0},
      combat::ai::elemental::Plan{*skills::effects::elemental(188,12)},"authored Flame Burst test"}};
  check(npc_world.configure_npc_elemental_ai(npc_policy,spells,4,1000));
  npc_box->drain();
  npc_world.tick(1000);
  const auto pending=npc_world.npc_elemental_ai_snapshot(npc_policy.source);
  check(pending&&pending->pending&&pending->pending->token.completes==1750);
  check(npc_world.npc_energy_snapshot(npc_policy.source)->current==35);
  const auto starts=npc_box->drain();
  const auto dword=[](const world::Packet& p,std::size_t offset){return std::uint32_t(p[offset])|
      std::uint32_t(p[offset+1])<<8|std::uint32_t(p[offset+2])<<16|std::uint32_t(p[offset+3])<<24;};
  check(std::any_of(starts.begin(),starts.end(),[&](const auto& p){return p.size()==14&&p[0]==158&&
        dword(p,2)==60&&dword(p,6)==100&&dword(p,10)==188;}));
  npc_world.tick(1750);
  check(npc_world.resources(1)->health==901&&npc_world.resources(2)->health==901&&
        npc_world.resources(3)->health==1000);
 }
 std::cout<<"Flame Burst passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
