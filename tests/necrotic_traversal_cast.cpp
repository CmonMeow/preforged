#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Necrotic cast line "+std::to_string(at.line()));
}
void fixture(const std::filesystem::path& path){
    std::filesystem::create_directories(path.parent_path());std::ofstream out(path,std::ios::binary);
    const auto u32=[&](std::uint32_t v){for(unsigned i=0;i<4;++i)out.put(char(v>>(8*i)));};
    const auto f32=[&](float v){u32(std::bit_cast<std::uint32_t>(v));};
    out.write("FWNAV001",8);for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})u32(n);
    f32(-2000);f32(-2000);f32(2000);f32(2000);u32(1);u32(1);
    for(unsigned i=0;i<5;++i)u32(0xffffffff);
    for(unsigned i=0;i<6;++i)f32(i%2?2000:-2000);
    out.close();
}
int main(int argc,char** argv)try{
 check(argc==2);fixture(argv[1]);
 const auto mesh=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(argv[1]));
 for(unsigned mode=0;mode<9;++mode){
  world::Instance w;w.configure_navigation(mesh);
  auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);
  w.join(2,"Corpse",600,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,1000,100,100},100);
  check(w.observe_condition_equipment(*w.effect_scope(1),equipment::condition_equipment(inventory::baseline(),0),100));
  check(w.observe_condition_equipment(*w.effect_scope(2),equipment::condition_equipment(inventory::baseline(),0),100));
  const auto source=*w.effect_scope(1),dead=*w.effect_scope(2);check(w.kill(dead,150));
  if(mode==4)w.set_vitals(1,{500,1000,0,100},100);
  skills::casting::Runtime runtime(1);skills::casting::Hooks h;
  std::vector<world::Packet> events;unsigned completions=0,reactions=0,draws=0;
  w.configure_necrotic_corpse_draws([&](std::size_t count){check(count==1);++draws;return 0u;});
  h.events=[&](const auto& packets){events.insert(events.end(),packets.begin(),packets.end());};
  h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](unsigned cost,auto now){check(cost==5);return w.consume_energy(1,cost,source,now);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.before_spell_effect=[&](auto skill,auto event,auto now){check(skill==97&&event&&now==1750);++reactions;return mode!=5;};
  h.necrotic_available=[&](auto owner,const auto& plan,auto now){return w.necrotic_traversal_available(owner,plan,now);};
  h.necrotic_traversal=[&](auto owner,const auto& plan,auto attempt,auto now)->skills::casting::EffectCompletion{
   ++completions;std::function<void()> publication;
   const auto result=w.complete_necrotic_traversal(owner,plan,attempt,now,&publication);
   return {bool(result),{},{},std::move(publication),true};
  };
  if(mode==6)h.necrotic_traversal={};
  skills::Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[97]=1;
  skills::Slots bar{};bar[0]={97,0};attributes::Ranks ranks{};ranks[5]=12;
  const auto reason=runtime.start(1000,{97,0,2},bar,policy,ranks,true,h);
  if(mode==4||mode==6){check(reason==(mode==4?skills::casting::Reason::energy:skills::casting::Reason::unsupported));check(!draws&&!completions);continue;}
  check(reason==skills::casting::Reason::accepted&&w.resources(1)->energy==95&&!draws);
  if(mode==1)runtime.cancel(h);
  if(mode==2)check(w.resurrect(dead,1100));
  if(mode==3)check(w.kill(source,1100));
  if(mode==7)h.before_spell_effect={};
  if(mode==8)h.necrotic_traversal={};
  runtime.update(1749,h);check(w.position(1,1749).first==0&&!completions&&!draws);
  runtime.update(1750,h);runtime.update(1750,h);
  if(mode){check(w.position(1,1750).first==0&&!draws);check(!w.traversal_corpse_snapshot(dead,1750)||!w.traversal_corpse_snapshot(dead,1750)->consumed);continue;}
  check(completions==1&&draws==1&&reactions==1&&w.position(1,1750).first==600);
  check(w.traversal_corpse_snapshot(dead,1750)->consumed);
  const auto finish=skills::casting::terminal(58,1);
  check(std::count(events.begin(),events.end(),finish)==1);
  // Fresh body permits an immediate second cast:97 has no shadow-step aftercast.
  check(w.resurrect(dead,1750)&&w.kill(dead,1750));
  check(runtime.start(1750,{97,0,2},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);
 }
 std::cout<<"Paid97 activation, cancellation, source death, lost corpse, spell reaction, Energy and zero-aftercast pass\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
