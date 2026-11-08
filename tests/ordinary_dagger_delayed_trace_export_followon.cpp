// NEW unregistered actual World wire trace. Exactly one isolated JSON argv.
#include "ordinary_dagger_delayed_trace_fixture_followon.h"
#include <filesystem>
#include <fstream>
struct TraceBatch {std::string scenario,phase;std::uint64_t at;std::vector<world::Packet> packets;};
static std::vector<TraceBatch> batches;
static std::uint32_t word(const world::Packet& p,unsigned at){check(p.size()>=at+4);return std::uint32_t(p[at])|(std::uint32_t(p[at+1])<<8)|(std::uint32_t(p[at+2])<<16)|(std::uint32_t(p[at+3])<<24);}
static unsigned opcode(const world::Packet& p){check(p.size()>=2);return p[0]|(unsigned(p[1])<<8);}
struct Counts {unsigned timing=0,start=0,finish=0,stop=0,damage=0,zero_skill=0;};
static Counts capture(Fixture& f,const std::string& label,const std::string& phase,std::uint64_t at){
 auto packets=f.box->drain();Counts count;
 for(const auto& p:packets){const auto op=opcode(p);
  check(op!=91&&op!=92); // ordinary is never dagger-chain stage publication
  if(op==53&&p.size()==14&&word(p,2)==f.source.actor)++count.timing;
  if(op==159&&p.size()==18&&word(p,2)==4&&word(p,6)==f.source.actor)++count.start;
  if(op==158&&p.size()==14&&word(p,6)==f.source.actor){const auto kind=word(p,2);check(kind!=2&&kind!=46&&kind!=47&&kind!=48&&kind!=49&&kind!=50&&kind!=58&&kind!=59&&kind!=60);
   if(kind==1)++count.finish;if(kind==3)++count.stop;if(kind==10){check(word(p,10)==0);++count.zero_skill;}}
  if(op==159&&p.size()==18)check(word(p,2)!=46&&word(p,2)!=47&&word(p,2)!=48&&word(p,2)!=49&&word(p,2)!=50&&word(p,2)!=58&&word(p,2)!=59&&word(p,2)!=60);
  if(op==162&&p.size()==18&&(word(p,2)==16||word(p,2)==17)&&word(p,6)==f.target.actor&&word(p,10)==f.source.actor&&std::bit_cast<float>(word(p,14))<0)++count.damage;
 }
 batches.push_back({label,phase,at,std::move(packets)});return count;
}
int main(int argc,char** argv)try{
 check(argc==2);
 for(unsigned mask=0;mask<4;++mask)for(unsigned mode=0;mode<3;++mode){
  const bool double_strike=mode!=0,cancel_second=mode==2;Fixture f(false,500,true,false,double_strike?0:.99);
  if(mask&1)check(f.w.status_mutation(f.source,346,123,0,100,8100,true));
  if(mask&2)check(f.w.faintheartedness(f.hexer,f.source,*shadow_of_fear(12),100));
  f.box->drain();const std::string label="mask"+std::to_string(mask)+"-mode"+std::to_string(mode);
  const auto timing=f.w.weapon_attack_timing(f.source,1000);check(bool(timing));const std::uint64_t multiplier=(mask&1?67u:100u)*(mask&2?150u:100u);
  const auto gap=(500*multiplier+5000)/10000,first=1000+timing->windup_ms,second=first+gap,animation=first+timing->period_ms-timing->windup_ms;
  f.start();const auto start=capture(f,label,"start",1000);check(start.timing==1&&start.start==1&&start.finish==0&&start.damage==0);
  f.w.tick(first-1);check(f.outcomes().empty());const auto before=capture(f,label,"pre_first",first-1);check(before.timing==0&&before.start==0&&before.finish==0&&before.damage==0);
  f.w.tick(first);const auto first_events=f.outcomes();check(first_events.size()==1&&first_events[0].at==first&&first_events[0].outcome==combat::weapon::AttackOutcome::hit&&f.health()==980);
  const auto hit=capture(f,label,"first",first);check(hit.timing==0&&hit.start==0&&hit.finish==1&&hit.damage==1&&hit.zero_skill==1);
  if(cancel_second){f.w.cancel_ordinary_attack(f.source,first+1);const auto stopped=capture(f,label,"cancel_after_first",first+1);check(stopped.start==0&&stopped.finish==0&&stopped.stop==0&&stopped.damage==0);}
  f.w.tick(second-1);check(f.outcomes().empty());const auto presecond=capture(f,label,"pre_second",second-1);check(presecond.timing==0&&presecond.start==0&&presecond.finish==0&&presecond.damage==0);
  f.w.tick(second);const auto second_events=f.outcomes();const bool second_contact=double_strike&&!cancel_second;
  check(second_events.size()==(second_contact?1u:0u)&&f.health()==(second_contact?960:980));if(second_contact)check(second_events[0].at==second&&second_events[0].outcome==combat::weapon::AttackOutcome::hit);
  const auto later=capture(f,label,"second",second);check(later.timing==0&&later.start==0&&later.finish==0&&later.stop==0&&later.damage==(second_contact?1u:0u)&&later.zero_skill==(second_contact?1u:0u));
  // Only the EXISTING next ordinary Intent windup starts another body. The
  // retained second damage must not restart it or synthesize a skill terminal.
  f.w.tick(animation-1);check(f.outcomes().empty());const auto preanimation=capture(f,label,"pre_next_animation",animation-1);check(preanimation.start==0&&preanimation.timing==0);
  f.w.tick(animation);check(f.outcomes().empty());const auto next=capture(f,label,"next_animation",animation);check(next.start==(cancel_second?0u:1u)&&next.timing==(cancel_second?0u:1u)&&next.finish==0&&next.damage==0);
  if(!cancel_second){f.w.cancel_ordinary_attack(f.source,animation+1);const auto stop=capture(f,label,"cancel_repeat",animation+1);check(stop.stop==1&&stop.start==0&&stop.finish==0&&stop.damage==0);}
  f.w.tick(first+timing->period_ms);check(f.outcomes().empty());const auto cancelled=capture(f,label,"cancelled_repeat_due",first+timing->period_ms);check(cancelled.start==0&&cancelled.finish==0&&cancelled.damage==0);
  check(f.selections==1&&f.w.resources(1)->energy==100);
 }
 const std::filesystem::path path=argv[1];if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());std::ofstream out(path,std::ios::binary);check(bool(out));
 out<<"{\"schema\":1,\"driver_manifest\":\"5E9A2366CAA27F34DA786AEEC001C7689F9D95DAA6A5B2EC5BBB0F422880507D\",\"actual_world_trace\":true,\"ordinary_source\":1,\"ordinary_target\":2,\"batches\":[";bool comma=false;const char digits[]="0123456789abcdef";
 for(const auto& batch:batches){if(comma)out<<',';comma=true;out<<"{\"scenario\":\""<<batch.scenario<<"\",\"phase\":\""<<batch.phase<<"\",\"at\":"<<batch.at<<",\"packets\":[";bool item=false;
  for(const auto& packet:batch.packets){if(item)out<<',';item=true;out<<'\"';for(auto byte:packet)out<<digits[byte>>4]<<digits[byte&15];out<<'\"';}out<<"]}";}
 out<<"]}\n";out.close();check(bool(out));std::cout<<"PASS12 actual World ordinary dagger wire sequences exported; no secondbody/finish, normalnextIntent and stop remain, no dualskill46/47; nativehandler next\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
