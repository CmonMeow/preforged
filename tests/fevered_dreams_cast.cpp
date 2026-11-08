#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Fevered cast line "+std::to_string(at.line()));}
int main()try{
 for(bool pvp:{false,true})for(unsigned mode=0;mode<17;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:float(id*60-60),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==1&&(mode==11||mode==15)?50u:500u,500,id==1&&mode==15?10u:50u,100},100);}
  const auto a=*w.effect_scope(1),b=*w.effect_scope(2),peer=*w.effect_scope(3);
  if(mode==15)check(w.apply_mind_wrack(b,a,12,false,100));
  if(mode==11)check(w.backfire(b,a,*effects::backfire(28,12),20,100));
  if(mode==12)check(w.hex_breaker(b,*effects::hex_breaker(12),20,100));
  if(mode==14)check(w.apply_condition(a,b,combat::conditions::Kind::blind,100,10000));
  casting::Hooks h;unsigned commits=0;std::vector<world::Packet> sent;
  h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};h.resources=[&](unsigned id){return w.resources(id);};
  h.consume_energy_at=[&](unsigned cost,auto now){check(cost==10);return w.consume_energy(1,cost,a,now);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.projectile_target_valid=[&](unsigned id,float range){return mode!=13&&id==2&&range==1248;};
  h.before_spell_effect=[&](auto skill,auto event,auto now){return w.before_spell_effect(a,skill,event,now);};
  h.fevered_dreams=[&](auto target,const auto& plan,auto now){++commits;check(plan.rank==12&&plan.skill==(pvp?3289u:55u));return mode==10?false:w.apply_fevered_dreams(a,target,plan.rank,plan.skill==3289,now,effects::FeveredDazedSpan::attempted_duration);};
  if(mode==6)h.fevered_dreams={};if(mode==7)h.target_scope={};if(mode==8)h.before_spell_effect={};if(mode==9)h.consume_energy_at={};
  Policy policy;policy.primary=5;policy.pvp_match=pvp;policy.campaigns=255;policy.learned[55]=1;Slots bar{};bar[0]={55,0};attributes::Ranks ranks{};ranks[1]=12;casting::Runtime cast(1);
  const auto reason=cast.start(100,{55,0,2},bar,policy,ranks,true,h);
  check(reason==(mode>=6&&mode<=9?casting::Reason::unsupported:mode==13?casting::Reason::target:mode==15?casting::Reason::policy:casting::Reason::accepted));
  if(mode>=6&&mode<=9||mode==13){check(w.resources(1)->energy==50&&!cast.active()&&!commits);continue;}
  if(mode==15){check(w.resources(1)->health==0&&w.resources(1)->energy==0&&!cast.active()&&!commits);continue;}
  check(w.resources(1)->energy==40);cast.update(2099,h);check(!commits);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(2099,h);
  if(mode==3){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},2099,1);w.set_vitals(1,{500,500,40,100},100);}
  if(mode==4){w.leave(2);w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},2099,2);w.set_vitals(2,{500,500,50,100},100);}
  if(mode==16)check(w.kill(b,2099));
  if(mode==5)h.fevered_dreams={};
  box->drain();cast.update(2100,h);check(!cast.active()&&commits==((mode==0||mode==10||mode==12||mode==14)?1u:0u));
  const auto packets=box->drain();const auto bodies=std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,55));
  if(mode==0||mode==14){const auto status=*w.status_snapshot(b,2100);auto hex=std::find_if(status.begin(),status.end(),[&](const auto& e){return e.skill==(pvp?3289:55);});check(hex!=status.end()&&hex->expires==(pvp?15100:24100)&&bodies==1);
   check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{55,0},pvp?20:10))==1);
   if(mode==14)check(w.has_condition(peer,combat::conditions::Kind::dazed,2100)==!pvp&&!w.has_condition(peer,combat::conditions::Kind::blind,2100));
   else{check(w.apply_condition(a,b,combat::conditions::Kind::blind,2200,5000)&&w.has_condition(peer,combat::conditions::Kind::blind,2200));check(w.apply_condition(a,b,combat::conditions::Kind::poison,2300,5000)&&w.has_condition(peer,combat::conditions::Kind::dazed,2300)==!pvp);check(cast.start(3000,{55,0,2},bar,policy,ranks,true,h)==casting::Reason::recharge);}
   check(w.fevered_boundary_failures()==0);
  }else{check(bodies==0);if(mode==11)check(w.resources(1)->health==0);if(mode==12)check(w.resources(1)->health==461);}
 }
 // Native NPCs have authoritative combat state, not player Resources.
 const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 60 0 0 - -\n");std::istringstream metadata("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/fevered_dreams_cast.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test\n");auto population=world::load_npc_catalog(appearance);world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
 for(bool pvp:{false,true}){world::Instance w;w.configure_npcs(population.for_map(146));auto box=std::make_shared<world::Outbox>();w.join(1,"Mesmer",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,500,50,100},100);const auto a=*w.effect_scope(1);
  casting::Hooks h;std::vector<world::Packet> sent;h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};h.resources=[&](unsigned id){return w.resources(id);};h.consume_energy_at=[&](auto cost,auto now){return w.consume_energy(1,cost,a,now);};h.target_scope=[&](auto id){return w.effect_scope(id);};h.projectile_target_valid=[](auto id,float range){return id==100&&range==1248;};h.before_spell_effect=[&](auto skill,auto event,auto now){return w.before_spell_effect(a,skill,event,now);};h.fevered_dreams=[&](auto target,const auto& plan,auto now){return w.apply_fevered_dreams(a,target,plan.rank,plan.skill==3289,now,effects::FeveredDazedSpan::attempted_duration);};
  Policy policy;policy.primary=5;policy.pvp_match=pvp;policy.campaigns=255;policy.learned[55]=1;Slots bar{};bar[0]={55,0};attributes::Ranks ranks{};ranks[1]=12;casting::Runtime cast(1);
  check(cast.start(100,{55,0,100},bar,policy,ranks,true,h)==casting::Reason::accepted);box->drain();cast.update(2100,h);check(!cast.active()&&(w.npc_combat_snapshot(*w.effect_scope(100))->effects&0x800));
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(100,1,55))==1&&std::count(sent.begin(),sent.end(),casting::activation(1,100,55))==1);
 }
 std::cout<<"PASS real Fevered2second/10Energy casts, PvE10/PvP20recharge, original body90, automatic spreading, initial Dazed, protection and hook/lifetime/cancel/interrupt/death guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
