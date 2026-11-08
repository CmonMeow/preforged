#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills;using combat::conditions::Kind;
unsigned mode=0;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Martyr cast line "+std::to_string(at.line())+" mode "+std::to_string(mode));}
int main()try{
 for(mode=0;mode<16;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*30),0,id==1?box:std::make_shared<world::Outbox>(),0,id==1?std::vector<world::Packet>{{165,0,1,0,0,0,std::uint8_t(mode==13?6:3),0}}:std::vector<world::Packet>{},{},100,id==3?2:1);w.set_vitals(id,{mode==14&&id==1?10u:500u,1000,30,30},100);}
  const auto owner=*w.effect_scope(1),ally=*w.effect_scope(2),foe=*w.effect_scope(3);
  check(w.apply_condition(foe,ally,Kind::blind,100,10000)&&w.apply_condition(foe,ally,Kind::bleeding,100,10000));
  if(mode==14)check(w.backfire(foe,owner,*effects::backfire(28,12),20,100));
  casting::Runtime cast(1);casting::Hooks h;unsigned paid=0,commits=0;
  h.events=[&](const auto& p){box->enqueue(p);};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](unsigned cost,auto at){++paid;check(cost==5);return mode!=5&&w.consume_energy(1,cost,owner,at);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};
  h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(owner,skill,event,at);};
  h.martyr=[&](auto scope,const auto& plan,unsigned divine,auto at){++commits;check(scope==owner&&divine==(mode==13?0u:12u));return w.complete_martyr(scope,plan,divine,at);};
  if(mode==4)h.martyr={};if(mode==6)h.before_spell_effect={};if(mode==7)h.target_scope={};if(mode==8)h.consume_energy_at={};
  Policy p;p.primary=mode==13?6:3;p.secondary=mode==13?3:0;p.campaigns=255;p.learned[298]=1;Slots bar{};bar[0]={298,0};attributes::Ranks ranks{};ranks[16]=12;
  const auto admission=cast.start(100,{298,0,3},bar,p,ranks,true,h);
  const bool missing=mode==4||mode==6||mode==7||mode==8;
  if(missing){check(admission==casting::Reason::unsupported&&!paid&&!commits);continue;}
  if(mode==5){check(admission==casting::Reason::energy&&paid==1&&!commits);continue;}
  check(admission==casting::Reason::accepted&&paid==1&&w.resources(1)->energy==25);cast.update(1099,h);check(!commits);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1099,h);
  if(mode==3){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},1099,1);w.set_vitals(1,{500,1000,25,30},100);}
  if(mode==9)h.martyr={};if(mode==10)h.before_spell_effect={};if(mode==11)h.target_valid={};
  if(mode==12)h.target_scope=[&](auto id){auto scope=w.effect_scope(id);if(scope&&id==1)++scope->generation;return scope;};
  box->drain();cast.update(mode==15?2000:1100,h);check(!cast.active());const bool success=mode==0||mode==13||mode==15;
  check(commits==unsigned(success));const auto packets=box->drain();const auto body=casting::generic(21,1,524),finish=casting::terminal(58,1);
  check(std::count(packets.begin(),packets.end(),body)==unsigned(success)&&std::count(packets.begin(),packets.end(),finish)==unsigned(success));
  if(!success){check(w.has_condition(ally,Kind::blind,1100));continue;}
  check(!w.has_condition(ally,Kind::blind,2000)&&w.has_condition(owner,Kind::blind,2000));
  check(std::find(packets.begin(),packets.end(),finish)<std::find(packets.begin(),packets.end(),body));
  check(std::count(packets.begin(),packets.end(),casting::slot_event(228,1,{298,0},10))==1);
  const auto next=mode==15?2800:2100;cast.update(next,h);check(commits==1&&cast.start(next,{298,0,3},bar,p,ranks,true,h)==casting::Reason::recharge);
 }
 std::cout<<"PASS normal paid Martyr self normalization,scope/hooks,interrupt/cancel,Backfire,primary/secondary DivineFavor,late completion,one ordered body/finish and pinned cooldown\n";
}catch(const std::exception& e){std::cerr<<e.what()<<" mode "<<mode<<'\n';return 1;}


