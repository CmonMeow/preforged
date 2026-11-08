#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Grenth cast line "+std::to_string(at.line()));}
int main()try{using namespace skills;
 for(unsigned mode=0;mode<11;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Necromancer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Foe",60,0,box,0,{}, {},100,2);
  w.set_vitals(1,{mode==1||mode==10?20u:mode==2?400u:100u,500,100,100},100);w.set_vitals(2,{500,500,100,100},100);
  const auto caster=*w.effect_scope(1),foe=*w.effect_scope(2);
  if(mode==1||mode==2||mode==10)check(w.backfire(foe,caster,*effects::backfire(28,12),20,100));
  casting::Hooks hooks;unsigned commits=0,prepared=0;std::vector<world::Packet> sent;
  hooks.events=[&](const auto& p){sent.insert(sent.end(),p.begin(),p.end());};hooks.resources=[&](unsigned id){return w.resources(id);};
  hooks.consume_energy=[&](unsigned amount){check(amount==10);return w.consume_energy(1,amount,caster);};
  hooks.target_scope=[&](unsigned id){return w.effect_scope(id);};hooks.projectile_target_valid=[](unsigned id,float range){return id==2&&range==1248;};
  hooks.control_epoch=[&]{return w.control_epoch(caster);};
  hooks.before_spell_effect=[&](unsigned skill,auto event,auto now){
   if(mode==8)return false;if(mode==9){check(w.kill(caster,now));return false;}
   return w.before_spell_effect(caster,skill,event,now);
  };
  hooks.prepare_channeling=[&](unsigned,auto,auto){++prepared;return true;};hooks.complete_channeling=[](auto,bool,auto){return true;};
  hooks.take_grenths_reaction_death=[&](auto event,auto now){return w.take_grenths_reaction_death(caster,event,now);};
  hooks.grenths_balance=[&](auto target,std::optional<std::uint64_t> epoch,auto now){
   ++commits;if(mode==10){check(w.resurrect(caster,now));check(w.kill(caster,now));}
   return w.grenths_balance(caster,target,now,bool(epoch),epoch);
  };
  if(mode==7)hooks.grenths_balance={};
  Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[86]=1;Slots bar{};bar[0]={86,0};attributes::Ranks ranks{};casting::Runtime cast(1);
  check(cast.start(100,{86,0,2},bar,policy,ranks,true,hooks)==(mode==7?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==7){check(w.resources(1)->energy==100);continue;}
  check(w.resources(1)->energy==90);cast.update(349,hooks);check(!commits);
  if(mode==3)cast.cancel(hooks);if(mode==4)cast.interrupt(349,hooks);
  if(mode==5){w.leave(2);w.join(2,"Replacement",60,0,box,0,{}, {},349,2);w.set_vitals(2,{500,500,100,100},100);}
  if(mode==6){w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},349,1);w.set_vitals(1,{100,500,100,100},100);}
  box->drain();cast.update(350,hooks);cast.update(351,hooks);
  const bool applied=mode==0||mode==1||mode==2;
  check(commits==(applied||mode==10?1u:0u));
  if(applied){const auto balanced=mode==2?static_cast<unsigned>(std::ceil((900.-effects::backfire(28,12)->damage)/2.)):300u;
   check(w.resources(1)->health==(mode==1?0u:balanced)&&w.resources(2)->health==(mode==1?250u:balanced));
   check(prepared==(mode==1?0u:1u));const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,86))==1);
   check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{86,0},10))==1);
  }
  if(mode==10)check(w.resources(2)->health==500); // Consumed proof cannot span a new death epoch.
 }
 std::cout<<"PASS Grenth actual250ms/10Energy/10recharge cast, lethal reaction proof, body164, no dead Channeling, cancellation/control/lifetime and false-reaction rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
