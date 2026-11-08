#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Aegis cast line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<6;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u}){w.join(id,"Member",float(id*100),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto source=*w.effect_scope(1);
  casting::Hooks h;unsigned completions=0;
  h.resources=[&](unsigned id){return w.resources(id);};h.events=[&](const auto& packets){box->enqueue(packets);};
  h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.target_valid=[](unsigned id,float,bool self){return id==1&&self;};
  h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(source,skill,event,at);};
  h.consume_energy_at=[&](unsigned cost,auto at){check(cost==10);return w.consume_energy(1,cost,source,at);};
  h.enchantment_duration_ready=[](unsigned skill,auto base,auto){return skill==257&&base==10000;};
  if(mode!=3)h.aegis=[&](const auto& plan,unsigned divine,auto at)->casting::EffectCompletion{
   ++completions;const bool ok=w.complete_aegis(source,plan,divine,at);return {ok,{},{},{},false,true};
  };
  Policy policy;policy.primary=6;policy.secondary=3;policy.campaigns=255;policy.learned[257]=1;policy.pvp_match=mode==4;
  Slots bar{};bar[0]={257,0};attributes::Ranks ranks{};ranks[15]=12;
  casting::Runtime runtime(1);const bool denied=mode==3||mode==4;
  check(runtime.start(1000,{257,0,2},bar,policy,ranks,true,h)==(denied?casting::Reason::unsupported:casting::Reason::accepted));
  if(denied){check(w.resources(1)->energy==100);continue;}
  check(w.resources(1)->energy==90);runtime.update(2999,h);check(completions==0);
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(2999,h);if(mode==5)h.aegis={};
  box->drain();runtime.update(3000,h);check(completions==unsigned(mode==0));
  if(mode==0){check(w.status_snapshot(*w.effect_scope(2),3000)->size()==1);
   const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==1);
   check(std::count(packets.begin(),packets.end(),combat::detail::integer(21,1,469))==1);
   check(runtime.start(3750,{257,0,0},bar,policy,ranks,true,h)==casting::Reason::recharge);
   h.current_skillbar=[&](){return std::optional{bar};};
   h.skill_disabled_slot=[](auto,auto)->std::optional<std::uint64_t>{return 13750;};
   box->drain();const auto before=runtime.cooldown_source_snapshot(source,3750,h);
   check(before&&before->timers.size()==1&&before->timers.front().until==33000);
   const effects::RecallFeedbackNotice recall{source,1,3750,bar};
   const auto feedback=runtime.prepare_recall_feedback(recall,3750,h);
   check(feedback&&feedback->packets.empty()&&feedback->slots.size()==1&&feedback->slots.front().ordinary_until==33000&&box->drain().empty());
   check(runtime.cooldown_source_snapshot(source,3750,h)==before);
   const auto expired=runtime.prepare_recall_feedback(recall,35000,h);
   check(expired&&expired->packets.empty()); // Existing client timer expires itself;Recall never restarts or clears it.
  }
 }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
