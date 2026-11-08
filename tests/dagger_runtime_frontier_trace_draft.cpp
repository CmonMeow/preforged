// Unregistered evidence exporter. Every packet is drained from the actual
// paying Runtime/World fixture; no missing frame is manufactured.
#include "dagger_successful_dual_fixture_draft.h"
#include <iomanip>
#include <sstream>
static void scope_json(effects::Scope scope){std::cout<<"{\"instance\":"<<scope.instance<<",\"actor\":"<<scope.actor<<",\"generation\":"<<scope.generation<<'}';}
static void emit(Fixture& f,const char* name,const char* frontier){
 const auto packets=f.box->drain();const auto outcomes=f.world.take_attack_outcomes(f.source);
 const auto current=f.world.effect_scope(f.source.actor);const auto resources=f.world.resources(f.source.actor);
 std::cout<<"{\"origin\":\"generated_actual_runtime_world\",\"case\":\""<<name<<"\",\"frontier\":\""<<frontier<<"\",\"time\":"<<f.clock<<",\"skill\":"<<f.skill<<",\"copy\":"<<f.active_copy<<",\"source_scope\":";scope_json(f.source);
 std::cout<<",\"target_scope\":";scope_json(f.target);std::cout<<",\"current_source_scope\":";
 if(current)scope_json(*current);else std::cout<<"null";
 std::cout<<",\"active\":"<<(f.cast.active()?"true":"false")<<",\"payments\":"<<f.payments<<",\"paid\":"<<f.paid<<",\"logical_completions\":"<<f.completions<<",\"copy_observers\":"<<f.copied_observations<<",\"source_energy\":"<<(resources?resources->energy:0)<<",\"packets\":[";
 for(std::size_t i=0;i<packets.size();++i){if(i)std::cout<<',';std::ostringstream hex;for(auto value:packets[i])hex<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(value);std::cout<<'"'<<hex.str()<<'"';}
 std::cout<<"],\"outcomes\":[";
 for(std::size_t i=0;i<outcomes.size();++i){const auto& event=outcomes[i];if(i)std::cout<<',';std::cout<<"{\"event\":"<<event.event<<",\"released\":"<<event.released<<",\"at\":"<<event.at<<",\"outcome\":"<<unsigned(event.outcome)<<",\"source_scope\":";scope_json(event.attacker);std::cout<<",\"target_scope\":";scope_json(event.target);std::cout<<'}';}
 std::cout<<"]}\n";
}
static void run(bool npc,bool frenzy){
 for(unsigned id:{775u,776u,777u}){
  Fixture f(id,npc);f.arm();const std::string name=std::string(npc?"npc_":"player_")+std::to_string(id)+(frenzy?"_frenzy":"_normal");
  if(frenzy)check(f.world.status_mutation(f.source,346,700,0,f.clock,f.clock+8000,true));
  emit(f,name.c_str(),"setup");check(f.start()==skills::casting::Reason::accepted);emit(f,name.c_str(),"start");
  const auto first=f.last_permit->release_time();f.update(first-1);emit(f,name.c_str(),"before_first");
  f.update(first);check(bool(f.last_result.successful_second));const auto phase=f.last_result.successful_second;emit(f,name.c_str(),"first");
  f.update(phase->due()-1);emit(f,name.c_str(),"before_second");f.update(phase->due());emit(f,name.c_str(),"second");
  f.update(phase->due());emit(f,name.c_str(),"replay");check(f.payments==1&&f.completions==1&&f.copied_observations==1);
 }
}
static void copied(){
 Fixture f(775);f.arm();f.policy.secondary=5;f.policy.learned[74]=1;f.bar[1]={74,0};check(f.world.configure_adrenaline(f.source,f.bar,f.clock));
 effects::TemporarySkillSession session(f.source,f.bar);effects::TemporarySkillIds ids(f.source);const auto echo=*effects::echo(74);
 check(f.world.status_mutation(f.source,74,100,0,f.clock,f.clock+echo.armed_duration,true,skills::Slot{74,0}));
 check(f.start()==skills::casting::Reason::accepted);f.update(f.last_permit->release_time());const auto phase=f.last_result.successful_second;check(bool(phase));
 check(bool(effects::commit_echo_copy(f.world,session,ids,f.source,{74,0},{775,0},f.clock,f.last_use_event)));f.bar=session.bar();
 emit(f,"copy1_775","prime_first");f.update(phase->due());emit(f,"copy1_775","prime_second");
 f.active_copy=1;f.clock=phase->due()+501;f.arm();
 f.hooks.admit_session_cast=[&](const auto& bar,const auto& policy,auto request,auto at)->std::optional<effects::HexCopySession::CastAdmission>{if(bar!=session.bar())return {};const auto slot=std::find(bar.begin(),bar.end(),skills::Slot{request.skill,request.copy});if(slot==bar.end())return {};return session.admit(f.source,policy,unsigned(slot-bar.begin()),*slot,at);};
 f.hooks.valid_session_cast=[&](const auto& grant,auto at){return session.valid(grant,f.policy,at);};
 emit(f,"copy1_775","setup");check(f.start()==skills::casting::Reason::accepted);emit(f,"copy1_775","start");
 f.update(f.last_permit->release_time());const auto copied_phase=f.last_result.successful_second;check(bool(copied_phase));emit(f,"copy1_775","first");
 f.update(copied_phase->due());emit(f,"copy1_775","second");f.update(copied_phase->due());emit(f,"copy1_775","replay");
 check(f.payments==2&&f.completions==2&&f.copied_observations==2);
}
int main()try{for(bool npc:{false,true})for(bool frenzy:{false,true})run(npc,frenzy);copied();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
