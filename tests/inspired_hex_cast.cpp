#include "gw_inspired_hex_commit.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Inspired cast line "+std::to_string(at.line()));}
int main()try{using namespace skills;using namespace effects;
 for(unsigned skill:{22u,1049u})for(unsigned mode=0;mode<7;++mode){
  world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{500,500,50,100},100);}
  const auto owner=*w.effect_scope(1),ally=*w.effect_scope(mode==6?1:2),enemy=*w.effect_scope(3);
  Slots saved{};saved[0]={skill,0};saved[1]={28,0};HexCopySession session(owner,saved);HexCopyIds ids(owner);
  check(w.configure_adrenaline(owner,saved,100));
  Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[skill]=1;policy.learned[28]=1;
  if(mode!=4)check(w.backfire(enemy,ally,*backfire(28,12),20,100));
  casting::Hooks h;unsigned commits=0;std::vector<world::Packet> sent;
  h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};h.resources=[&](unsigned id){return w.resources(id);};
  h.consume_energy=[&](unsigned cost){check(cost==5);return w.consume_energy(1,cost,owner);};
  h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.target_valid=[&](unsigned id,float range,bool self){return id==(mode==6?1u:2u)&&range==1248&&self;};
  h.before_spell_effect=[](unsigned,std::uint64_t,auto){return true;};
  h.inspired_hex=[&](auto target,const auto& plan,auto source,auto now){++commits;const auto result=commit_inspired_hex_copy(w,session,ids,owner,target,plan,source,now);if(result&&result->changed)sent.push_back(slot_packet_2022(1,result->changed->index,result->changed->after));return bool(result);};
  if(mode==5)h.inspired_hex={};
  attributes::Ranks ranks{};ranks[3]=12;casting::Runtime cast(1);
  check(cast.start(100,{skill,0,mode==6?1u:2u},saved,policy,ranks,true,h)==(mode==5?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==5){check(w.resources(1)->energy==50);continue;}
  check(w.resources(1)->energy==45);cast.update(1099,h);check(!commits);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1099,h);
  if(mode==3){w.leave(2);w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},1099,1);w.set_vitals(2,{500,500,50,100},100);}
  cast.update(1100,h);check(!cast.active());check(commits==(mode==0||mode==4||mode==6?1u:0u));
  if(mode==0||mode==6){check(session.bar()[0]==Slot{28,1}&&session.base()==saved);check(w.status_snapshot(ally,1100)->empty());check(w.resources(1)->energy>=54);
   check(w.cooldown_bar_snapshot(owner)->bar==session.bar()&&w.cooldown_bar_snapshot(owner)->revision==2);
   check(std::count(sent.begin(),sent.end(),slot_packet_2022(1,0,{28,1}))==1);
   check(std::count(sent.begin(),sent.end(),casting::generic(21,1,39))==1);
   const auto restored=expire_hex_copy(w,session,owner,21100);check(restored&&restored->size()==1&&session.bar()==saved);
   check(w.cooldown_bar_snapshot(owner)->bar==saved&&w.cooldown_bar_snapshot(owner)->revision==3);
  }else {check(session.bar()==saved);if(mode==4)check(w.resources(1)->energy==45);}
  check(policy.learned.size()==2&&policy.learned.at(28)==1);
 }
 {
  world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{500,500,50,100},100);}
  const auto owner=*w.effect_scope(1),ally=*w.effect_scope(2),enemy=*w.effect_scope(3);
  Slots base{};base[0]={22,0};HexCopySession session(owner,base);HexCopyIds ids(owner);
  check(w.backfire(enemy,ally,*backfire(28,12),20,100));
  const auto plan=*inspired_hex(22,12);
  // Missing authority must reject before removing the allied hex or restoring Energy.
  check(!commit_inspired_hex_copy(w,session,ids,owner,ally,plan,{22,0},200));
  check(session.bar()==base&&w.resources(1)->energy==50&&w.status_snapshot(ally,200)->size()==1);
  check(w.configure_adrenaline(owner,base,200));
  const auto result=commit_inspired_hex_copy(w,session,ids,owner,ally,plan,{22,0},200);check(result&&result->changed);
  check(result->changed->after==Slot{28,1}); // Rejected publication must not spend a copy ID.
  const auto copied=session.bar();check(w.cooldown_bar_snapshot(owner)->bar==copied);
  Slots other{};other[0]={281,0};check(w.configure_adrenaline(owner,other,300));
  check(!expire_hex_copy(w,session,owner,20200)&&session.bar()==copied&&w.cooldown_bar_snapshot(owner)->bar==other);
  check(w.publish_skillbar_transition(owner,{other,copied},20200));
  check(expire_hex_copy(w,session,owner,20200)&&session.bar()==base&&w.cooldown_bar_snapshot(owner)->bar==base);
  Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[281]=1;
  // Base updates work both before and after the regular build publisher runs.
  check(replace_hex_copy_base(w,session,owner,other,policy,20300)&&session.bar()==other&&w.cooldown_bar_snapshot(owner)->bar==other);
  policy.primary=5;policy.learned[22]=1;check(w.configure_adrenaline(owner,base,20400));
  check(replace_hex_copy_base(w,session,owner,base,policy,20400)&&session.bar()==base);
  auto stale=owner;++stale.generation;check(!w.publish_skillbar_transition(stale,{base,other},20500));
  check(!w.publish_skillbar_transition(owner,{other,base},20500)&&w.cooldown_bar_snapshot(owner)->bar==base);
  check(w.backfire(enemy,ally,*backfire(28,12),20,20500));
  check(bool(commit_inspired_hex_copy(w,session,ids,owner,ally,plan,{22,0},20500)));
  const auto admission=session.admit(owner,policy,0,session.bar()[0],20500);check(bool(admission));
  policy.primary=3;check(w.configure_adrenaline(owner,other,20600));
  check(replace_hex_copy_base(w,session,owner,other,policy,20600)&&session.bar()==other);
  check(!session.valid(*admission,policy,20600));
  const auto retired=expire_hex_copy(w,session,owner,40500);
  check(retired&&retired->empty()&&session.bar()==other&&w.cooldown_bar_snapshot(owner)->bar==other);
 }
 std::cout<<"PASS Inspired/Revealed casts and World-atomic copied bar publication/restoration, failed-authority nonmutation and base synchronization\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
