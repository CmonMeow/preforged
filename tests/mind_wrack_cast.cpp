#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Mind Wrack cast line "+std::to_string(at.line()));}
int main()try{
 for(bool pvp:{false,true})for(unsigned mode=0;mode<10;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1&&(mode==5||mode==9)?50u:500u,500,id==1?(mode==5?5u:50u):2u,100},100);}const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.energy_capacity(2,5,0,100,{80,-2}));if(mode==5)check(w.apply_mind_wrack(b,a,12,false,100));if(mode==9)check(w.backfire(b,a,*effects::backfire(28,12),20,100));
  casting::Hooks h;unsigned commits=0,uses=0;std::vector<world::Packet> sent;
  h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};h.resources=[&](unsigned id){return w.resources(id);};h.consume_energy_at=[&](unsigned amount,auto now){check(amount==5);return w.consume_energy(1,amount,a,now);};h.target_scope=[&](auto id){return w.effect_scope(id);};h.projectile_target_valid=[](unsigned id,float range){return id==2&&range==1248;};h.before_spell_effect=[&](unsigned skill,auto event,auto now){return w.before_spell_effect(a,skill,event,now);};
  h.mind_wrack=[&](auto target,const auto& plan,auto now){++commits;check(plan.rank==12&&plan.skill==(pvp?2734u:49u));return w.apply_mind_wrack(a,target,plan.rank,plan.skill==2734,now);};
  h.mind_wrack_targeted=[&](const auto& use){++uses;check(use.caster==a&&use.target==b&&use.skill==49&&use.at==1100&&use.event);return w.mind_wrack_targeted(use);};if(mode==4)h.consume_energy_at={};
  Policy policy;policy.primary=5;policy.pvp_match=pvp;policy.campaigns=255;policy.learned[49]=1;Slots bar{};bar[0]={49,0};attributes::Ranks ranks{};ranks[2]=12;casting::Runtime cast(1);
  check(cast.start(100,{49,0,2},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:mode==5?casting::Reason::policy:casting::Reason::accepted));
  if(mode==4){check(w.resources(1)->energy==50);continue;}if(mode==5){check(w.resources(1)->health==0&&w.resources(1)->energy==0&&!cast.active()&&!commits&&!uses);continue;}
  check(w.resources(1)->energy==45);cast.update(1099,h);check(!commits&&!uses);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1099,h);if(mode==3){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},1099,1);w.set_vitals(1,{500,500,45,100},100);}if(mode==6)h.mind_wrack_targeted={};if(mode==7){w.leave(2);w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},1099,2);w.set_vitals(2,{500,500,2,100},100);}if(mode==8)h.mind_wrack={};
  box->drain();cast.update(1100,h);check(!cast.active()&&commits==(mode==0?1u:0u)&&uses==(mode==0?1u:0u));
  if(mode==9)check(w.resources(1)->health==0);
  if(!mode){const auto status=*w.status_snapshot(b,1100);check(status.size()==1&&status[0].skill==(pvp?2734u:49u)&&status[0].expires==34100);auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,49))==1);check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{49,0},7))==1);}
 }
 // Actual completed non-hex casts feed the target observer. Cry has no
 // primary effect against this idle foe; Mind Wrack's own components remain
 // independently visible. A recharging request is not another target event.
 for(bool pvp:{false,true}){world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,id==1?100u:2u,100},100);}const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.energy_capacity(2,5,0,100,{80,-2}));casting::Hooks h;unsigned uses=0;h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.consume_energy_at=[&](auto cost,auto now){return w.consume_energy(1,cost,a,now);};h.target_scope=[&](auto id){return w.effect_scope(id);};h.projectile_target_valid=[](auto id,float){return id==2;};h.before_spell_effect=[&](auto skill,auto event,auto now){return w.before_spell_effect(a,skill,event,now);};h.mind_wrack=[&](auto target,const auto& plan,auto now){return w.apply_mind_wrack(a,target,plan.rank,plan.skill==2734,now);};h.mind_wrack_targeted=[&](const auto& use){++uses;return w.mind_wrack_targeted(use);};h.cry_frustration=[&](auto target,const auto& plan,auto now){return w.cry_frustration(a,target,plan,20,now);};Policy p;p.primary=5;p.pvp_match=pvp;p.campaigns=255;p.learned[49]=p.learned[57]=1;Slots bar{};bar[0]={49,0};bar[1]={57,0};attributes::Ranks ranks{};ranks[2]=12;casting::Runtime cast(1);
  check(cast.start(100,{49,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);cast.update(1100,h);check(uses==1&&w.resources(2)->energy==2);
  check(cast.start(2000,{57,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);cast.update(2250,h);check(uses==2&&w.resources(2)->energy==1&&w.resources(2)->health==(pvp?500u:479u));check(cast.start(2600,{57,0,2},bar,p,ranks,true,h)!=casting::Reason::accepted&&uses==2);
  check(cast.start(18000,{57,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);cast.update(18250,h);check(uses==3&&w.resources(2)->energy==0&&w.resources(2)->health==(pvp?417u:375u)&&w.status_snapshot(b,18250)->empty());
 }
 std::cout<<"PASS actual Mind Wrack1second/5Energy/7recharge PvE/PvP casts/body83, completed target callback, payment-death cancellation and hook/lifetime/failure guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
