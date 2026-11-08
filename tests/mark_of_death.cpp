#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Mark of Death line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope source,target;
 Fixture(bool npc=false){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,1000};d.damageable=d.conditions_allowed=true;
   n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{id==1?1000u:500u,1000,100,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);box->drain();
 }
};
int main()try{
 check(!mark_of_death(21));
 for(unsigned rank=0;rank<=20;++rank){const auto p=mark_of_death(rank);check(p&&p->reduction==33&&p->duration==std::uint64_t(interpolate(4,10,rank))*1000);
  const auto hex=*degeneration_hex(785,rank);check(hex.pips==0&&hex.duration==p->duration&&valid_degeneration_hex(hex));
  auto forged=hex;++forged.pips;check(!valid_degeneration_hex(forged));}
 const Entry mark{1,1,785,12,Category::hex,1000,10000};
 std::vector<Entry> entries{mark,mark};check(std::abs(healing_hex_multiplier(entries,1000)-.67)<1e-10);
 check(healing_hex_multiplier(entries,999)==1&&healing_hex_multiplier(entries,10000)==1);
 entries.push_back({2,1,129,12,Category::hex,1000,10000});
 check(std::abs(healing_hex_multiplier(entries,1000)-.67*.66)<1e-10);
 for(bool wound:{false,true}){combat::deep_wound::State state;state.reset(1000);double hp=500;
  if(wound)state.capacity(hp,1000,true);
  const double before=hp;state.event(hp,100,true,healing_hex_multiplier(entries,1000));check(std::abs(hp-before-60)<1e-10);
  state.event(hp,100,false,.67);check(std::abs(hp-before-160)<1e-10);}
 for(unsigned mode=0;mode<3;++mode){Fixture f;check(f.w.degeneration_hex(f.source,f.target,*degeneration_hex(785,0),1000));
  if(mode==1){const auto entry=f.w.status_snapshot(f.target,1000)->front();check(f.w.status_mutation(f.target,785,entry.instance,0,1100,{},false));}
  const unsigned now=mode==2?5000:1100;check(f.w.heal_target(2,2,100,1248,true,now));
  check(f.w.resources(2)->health==(mode?600u:567u));}
 {Fixture f;check(f.w.degeneration_hex(f.source,f.target,*degeneration_hex(785,0),1000));
  const auto first=f.w.status_snapshot(f.target,1000)->front();
  check(f.w.degeneration_hex(f.source,f.target,*degeneration_hex(785,12),2000));
  const auto renewed=*f.w.status_snapshot(f.target,2000);check(renewed.size()==1&&renewed.front().instance!=first.instance&&renewed.front().expires==11000);
  check(f.w.heal_target(2,2,100,1248,true,2100)&&f.w.resources(2)->health==567);
  f.w.tick(11000);check(f.w.status_snapshot(f.target,11000)->empty()&&f.w.resources(2)->health==567);}
 {Fixture f;check(f.w.hex_breaker(f.target,*hex_breaker(12),20,500));
  std::vector<world::Packet> out;check(f.w.degeneration_hex(f.source,f.target,*degeneration_hex(785,12),1000,20,&out));
  check(f.w.status_snapshot(f.target,1000)->empty()&&f.w.resources(1)->health==961&&out.empty());
  check(f.w.heal_target(2,2,100,1248,true,1100)&&f.w.resources(2)->health==600);}
 {Fixture f;check(f.w.degeneration_hex(f.source,f.target,*degeneration_hex(785,12),1000));
  check(f.w.degeneration_hex(f.source,f.target,*degeneration_hex(129,12),1000));
  check(f.w.heal_target(2,2,100,1248,true,1100)&&f.w.resources(2)->health==560);}
 // The NPC's own healing authority uses the same reduction, without needing a player ally route.
 {combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,1000};d.damageable=d.conditions_allowed=true;
  const combat::npcs::Lifetime life{1,2,3,146,100};combat::npcs::LiveActor actor(d,life);
  check(actor.apply_single_degeneration_hex(life,*degeneration_hex(785,12),1,7,1000).has_value());
  check(actor.heal(life,100,1100)&&std::abs(actor.actor(life)->health->current-567)<1e-10);
  check(actor.remove_hex_effect(life,7,1200)&&actor.heal(life,100,1300)&&std::abs(actor.actor(life)->health->current-667)<1e-10);}
 for(bool npc:{false,true})for(unsigned mode=0;mode<5;++mode){Fixture f(npc);skills::casting::Hooks h;unsigned commits=0;bool stale=false;std::vector<world::Packet> sent;
  h.resources=[&](unsigned id){return f.w.resources(id);};h.events=[&](const auto& p){sent.insert(sent.end(),p.begin(),p.end());};
  h.consume_energy_at=[&](unsigned cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned id){if(stale&&id==f.target.actor)return std::optional<Scope>{};return f.w.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==f.target.actor&&range==1248;};
  if(mode!=4)h.degeneration_hex=[&](auto target,const auto& plan,auto at)->skills::casting::EffectCompletion{
   ++commits;std::vector<world::Packet> out;const bool ok=f.w.degeneration_hex(f.source,target,plan,at,20,&out);check(ok);return {ok,std::move(out)};};
  skills::Slots bar{};bar[0]={785,0};skills::Policy policy;policy.primary=7;policy.campaigns=255;policy.learned[785]=1;attributes::Ranks ranks{};ranks[30]=12;
  skills::casting::Runtime cast(1);check(cast.start(1000,{785,0,f.target.actor},bar,policy,ranks,true,h)==(mode==4?skills::casting::Reason::unsupported:skills::casting::Reason::accepted));
  check(f.w.resources(1)->energy==(mode==4?100u:90u));if(mode==4)continue;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  cast.update(1249,h);check(!commits);cast.update(1250,h);cast.update(1250,h);check(commits==unsigned(mode==0));
  const auto particle=*skills::particles::targeted_body_2022(f.target.actor,1,785);
  check(std::count(sent.begin(),sent.end(),particle)==unsigned(mode==0));
  if(mode==0)check(std::find(sent.begin(),sent.end(),skills::casting::terminal(58,1))<std::find(sent.begin(),sent.end(),particle));
 }
 std::cout<<"PASS Mark of Death ranks,healing reduction/stack/cap/gain,player expiry/removal,NPC heal and ten Runtime lifecycle cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
