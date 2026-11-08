#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){
 if(!x)throw std::runtime_error("Obsidian Flame line "+std::to_string(at.line()));
}
int main()try{
 using namespace skills;
 for(bool npc:{false,true})for(unsigned mode=0;mode<7;++mode){
  warrior_test::Fixture f(322,npc);if(mode==4)f.world.set_vitals(1,{1000,1000,4,30},100);
  Slots bar{};bar[0]={219,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[219]=1;
  attributes::Ranks ranks{};ranks[9]=12;
  casting::Hooks h;unsigned paid=0,ordinary=0,commits=0;std::uint64_t payment_event=0;std::string phase;
  h.diagnostic=[&](unsigned,const char* value){phase=value;};
  h.resources=[&](unsigned id){return f.world.resources(id);};h.events=[](const auto&){};
  h.consume_energy_at=[&](unsigned,auto){++ordinary;return false;};
  h.consume_overcast_energy=[&](unsigned skill,unsigned cost,auto event,auto at){++paid;payment_event=event;return f.world.activate_overcast_energy(f.source,skill,cost,event,at);};
  h.target_scope=[&](unsigned id){return f.world.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float){return id==f.target.actor;};
  h.shadow_damage=[&](auto target,const auto& plan,auto at){++commits;return f.world.shadow_damage(f.source,target,plan,20,at);};
  if(mode==3)h.consume_overcast_energy={};
  casting::Runtime cast(1);
  const auto result=cast.start(1000,{219,0,f.target.actor},bar,policy,ranks,true,h);
  check(result==(mode==3?casting::Reason::unsupported:mode==4?casting::Reason::energy:casting::Reason::accepted));
  check(!ordinary);
  if(mode==3||mode==4){check(!f.world.overcast_energy_snapshot(f.source)&&!commits);continue;}
  check(paid==1&&payment_event);
  const auto initial=*f.world.overcast_energy_snapshot(f.source);
  check(initial.curve.burden_units()==find_skill(219)->overcast*100000u);
  check(!f.world.activate_overcast_energy(f.source,219,5,payment_event,1000));
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1200,h);
  if(mode==5){auto altered=f.target;++altered.generation;
   h.target_scope=[&,altered](unsigned id)->std::optional<effects::Scope>{return id==altered.actor?std::optional{altered}:f.world.effect_scope(id);};}
  if(mode==6)check(f.world.kill(f.source,1200));
  cast.update(2499,h);check(!commits);cast.update(2500,h);check(!cast.active());
  if(commits!=unsigned(mode==0))std::cerr<<"npc="<<npc<<" mode="<<mode<<" commits="<<commits<<" phase="<<phase<<'\n';
  check(commits==unsigned(mode==0));
  check(f.world.overcast_energy_snapshot(f.source)->curve.burden_units()>0);
  if(mode==0)check(f.health()==906); // Earth12:22..112 ->94,ignores armor.
 }
 // Glyph discounts ordinary Energy, never Overcast. Cancelled spells consume
 // a charge and keep their burden; a later cast must receive a fresh event.
 {
  warrior_test::Fixture f(322,false);
  Slots bar{};bar[0]={200,0};bar[1]={219,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[200]=policy.learned[219]=1;
  attributes::Ranks ranks{};ranks[9]=12;
  casting::Hooks h;std::vector<unsigned> costs;std::vector<std::uint64_t> events;unsigned ordinary=0,commits=0;
  h.resources=[&](unsigned id){return f.world.resources(id);};h.events=[](const auto&){};
  h.consume_energy_at=[&](unsigned cost,auto at){ordinary+=cost;return f.world.consume_energy(1,cost,f.source,at);};
  h.consume_overcast_energy=[&](unsigned skill,unsigned cost,auto event,auto at){costs.push_back(cost);events.push_back(event);return f.world.activate_overcast_energy(f.source,skill,cost,event,at);};
  h.target_scope=[&](unsigned id){return f.world.effect_scope(id);};
  h.target_valid=[](unsigned id,float,bool){return id==1;};
  h.projectile_target_valid=[&](unsigned id,float){return id==f.target.actor;};
  h.shadow_damage=[&](auto,const auto&,auto){++commits;return true;};
  h.status_mutation=[&](auto skill,auto effect,auto rank,auto at,auto until,bool active){return f.world.status_mutation(f.source,skill,effect,rank,at,until,active);};
  h.effect_snapshot=[&](auto at){return f.world.status_snapshot(f.source,at);};
  casting::Runtime cast(1);
  check(cast.start(1000,{200,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.update(2000,h);
  check(!cast.active()&&ordinary==5);
  std::uint64_t burden=0;
  for(std::uint64_t at:{3000u,4000u,5000u}){
   check(cast.start(at,{219,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);
   const auto next=f.world.overcast_energy_snapshot(f.source)->curve.burden_units();check(next>burden);burden=next;
   cast.cancel(h);check(!cast.active()&&!commits);
  }
  check((costs==std::vector<unsigned>{0,0,5})&&ordinary==5);
  check(events.size()==3&&events[0]<events[1]&&events[1]<events[2]);
 }
 std::cout<<"PASS Obsidian Flame atomic Overcast lifecycle and actual Glyph discounts,charge depletion,cancelled burden and unique recast events\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
