#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Copied cast line "+std::to_string(at.line()));}
int main()try{using namespace skills;using namespace effects;
 for(unsigned mode=0;mode<6;++mode){
  world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{500,500,100,100},100);}
  const auto owner=*w.effect_scope(1),ally=*w.effect_scope(2),foe=*w.effect_scope(3);
  Slots saved{};saved[0]={22,0};HexCopySession session(owner,saved);Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[22]=1;
  check(w.backfire(foe,ally,*backfire(28,12),20,100));
  const auto offer=w.prepare_inspired_hex(owner,ally,*inspired_hex(22,12),200);check(bool(offer));
  auto staged=session.prepare(*offer,0,{22,0},{28,1});check(bool(staged));const auto result=w.commit_inspired_hex(*offer);check(result&&session.commit(std::move(*staged),*result));
  casting::Hooks h;unsigned effects=0,consumed=0;std::vector<world::Packet> packets;
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};h.resources=[&](unsigned id){return w.resources(id);};
  h.consume_energy=[&](unsigned cost){consumed+=cost;return w.consume_energy(1,cost,owner);};
  h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.projectile_target_valid=[](unsigned id,float range){return id==3&&range==1248;};
  h.before_spell_effect=[](unsigned,std::uint64_t,auto){return true;};
  h.backfire=[&](auto target,const auto& plan,auto now){++effects;return w.backfire(owner,target,plan,20,now);};
  h.admit_session_cast=[&](const Slots& bar,const Policy& p,casting::Request request,auto now)->std::optional<HexCopySession::CastAdmission>{
   if(bar!=session.bar())return {};return session.admit(owner,p,0,{request.skill,request.copy},now);
  };
  h.valid_session_cast=[&](const auto& a,auto now){return session.valid(a,policy,now);};
  if(mode==3)h.admit_session_cast={};if(mode==4){h.admit_session_cast={};h.valid_session_cast={};}
  const std::uint64_t start=mode==1?19000:200;attributes::Ranks ranks{};ranks[2]=12;casting::Runtime cast(1);
  const auto accepted=cast.start(start,{28,1,3},session.bar(),policy,ranks,true,h);
  if(mode==3||mode==4){check(accepted==casting::Reason::policy&&!consumed);continue;}
  check(accepted==casting::Reason::accepted&&consumed==15&&cast.active());
  if(mode==2)check(bool(session.replace_base(owner,saved,policy,201)));
  if(mode==5)h.valid_session_cast={};
  cast.update(mode==1?20200:3200,h);
  check(!cast.active()&&effects==(mode==0?1u:0u));
  check(session.base()==saved&&!policy.learned.contains(28));
  if(mode==0)check(std::count(packets.begin(),packets.end(),casting::slot_event(228,1,{28,1},20))==1);
  else check(std::count(packets.begin(),packets.end(),cancellation_packet(1,{28,1}))==1);
 }
 std::cout<<"PASS copied unlearned hex casting, strict lifetime and base replacement cancellation, missing-hook rejection, unchanged learning and copy-specific recharge\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


