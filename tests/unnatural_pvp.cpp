#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Unnatural PvP line "+std::to_string(at.line()));}
int main()try{
 check(unnatural_signet(12,true)->primary==50&&unnatural_signet(12,true)->adjacent==50&&unnatural_signet(12,true)->skill==3188);
 for(unsigned mode=0;mode<7;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id=1;id<=3;++id){w.join(id,"Actor",float((id-1)*100),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);check(w.degeneration_hex(owner,target,*degeneration_hex(129,0),100));
  const unsigned skill=mode==4?3188:934;Slots bar{};bar[0]={skill,0};bar[1]={322,0};bar[2]={322,1};bar[3]={1,0};bar[4]={26,0};check(w.configure_adrenaline(owner,bar,100));
  casting::Hooks h;std::vector<Packet> wire;std::uint64_t used=0;
  h.events=[&](const auto& packets){wire.insert(wire.end(),packets.begin(),packets.end());};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto cost,auto at){return w.consume_energy(1,cost,owner,at);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return w.projectile_target_valid(owner,id,range,100);};
  h.current_skillbar=[&]()->std::optional<Slots>{return bar;};h.skill_disabled_slot=[&](auto slot,auto at){return w.skill_disabled_slot(owner,slot,at);};
  h.prepare_skill_use=[&](auto slot,auto event,auto at){return w.prepare_diversion_skill_use(owner,slot,event,at);};
  h.finalize_skill_use=[&](auto event,auto at,auto base,auto existing,bool instant){return w.finalize_diversion_skill_use(owner,event,at,base,existing,instant);};
  h.abort_skill_use=[&](auto event){return w.abort_diversion_skill_use(owner,event);};h.cooldown_completion_source=[&](auto event){return w.cooldown_completion_stamp(owner,event);};
  h.unnatural_signet=[&](auto foe,const auto& plan,auto event,auto at){used=event;return w.unnatural_signet(owner,foe,plan,20,at,event);};
  Policy p;p.primary=5;p.secondary=1;p.pvp_match=true;p.campaigns=255;p.learned[skill]=p.learned[1]=p.learned[26]=1;p.learned[322]=2;
  if(mode==2)h.prepare_skill_use={};
  if(mode==3)check(w.apply_unnatural_disables(owner,UINT64_MAX,100));
  casting::Runtime cast(1);const auto reason=cast.start(100,{skill,0,2},bar,p,attributes::Ranks{{0,0,12}},true,h);
  if(mode==2){check(reason==casting::Reason::unsupported);continue;}
  check(reason==casting::Reason::accepted);if(mode==1)cast.cancel(h);if(mode==5)cast.interrupt(500,h);
  if(mode==6){w.leave(2);w.join(2,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);}
  cast.update(1100,h);
  const bool success=mode==0||mode==4;
  check(w.resources(2)->health==(success?950u:1000u)&&w.resources(3)->health==(success?950u:1000u));
  if(!success)continue;
  check(w.skill_disabled_slot(owner,{322,0},1100)==11100&&w.skill_disabled_slot(owner,{322,1},1100)==11100);
  check(w.skill_disabled_slot(owner,{1,0},1100)==0&&w.skill_disabled_slot(owner,{26,0},1100)==0);
  check(std::count(wire.begin(),wire.end(),casting::slot_event(228,1,{skill,0},20))==1);
  cast.update(1101,h);check(std::count(wire.begin(),wire.end(),casting::slot_event(228,1,{322,1},10))==1);
  check(!w.unnatural_signet(owner,target,*unnatural_signet(12,true),20,1101,used)&&w.resources(2)->health==950);
  check(cast.start(2000,{322,1,2},bar,p,{},true,h)==casting::Reason::recharge);
 }
 std::cout<<"PASS Unnatural PvP alias/native damage+current-copy disables together,20s recharge/UI,replay-before-damage,cancel/interrupt/lifetime/missing-event checks\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
