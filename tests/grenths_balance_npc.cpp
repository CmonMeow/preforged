#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Grenth NPC regression line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<8;++mode){
  const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
  auto population=world::load_npc_catalog(appearance);
  const std::string hp=mode==6?"- -":mode==1?"500 100":"500 500";
  const std::string relation=mode==7?"neutral":"hostile-to-players",health_evidence=mode==6?"-":"test";
  const std::string flags=mode==6?"0 0":"1 1",condition_evidence=mode==6?"-":"test";
  std::istringstream metadata("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/grenths_balance_npc.cpp\nnpc 146 100 1 1234 0 0 8 1 "+relation+" - "+hp+" 60 "+flags+" test "+health_evidence+" test - "+condition_evidence+" 60 60 test test\n");
  world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
  world::Instance w;w.configure_npcs(population.for_map(146));auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",mode==5?2000.f:0.f,0,box,0,{}, {},100,1);w.set_vitals(1,{mode==1?500u:mode==3?300u:100u,500,100,100},100);
  const auto caster=*w.effect_scope(1);auto npc=*w.effect_scope(100);
  if(mode==2)check(w.kill(caster,100));if(mode==3)check(w.apply_condition(caster,caster,combat::conditions::Kind::deep_wound,100,60000));if(mode==4)++npc.generation;
  box->drain();check(!w.resources(100));
  if(mode==0){using namespace skills;casting::Hooks hooks;
   hooks.events=[](const auto&){};hooks.resources=[&](unsigned id){return w.resources(id);};
   hooks.consume_energy=[&](unsigned cost){check(cost==10);return w.consume_energy(1,cost,caster);};
   hooks.target_scope=[&](unsigned id){return w.effect_scope(id);};hooks.projectile_target_valid=[](unsigned id,float range){return id==100&&range==1248;};
   hooks.before_spell_effect=[&](unsigned skill,auto event,auto now){return w.before_spell_effect(caster,skill,event,now);};
   hooks.take_grenths_reaction_death=[&](auto event,auto now){return w.take_grenths_reaction_death(caster,event,now);};
   hooks.grenths_balance=[&](auto selected,auto epoch,auto now){return w.grenths_balance(caster,selected,now,bool(epoch),epoch);};
   Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[86]=1;Slots bar{};bar[0]={86,0};attributes::Ranks ranks{};casting::Runtime cast(1);
   check(cast.start(100,{86,0,100},bar,policy,ranks,true,hooks)==casting::Reason::accepted);cast.update(350,hooks);check(w.resources(1)->energy==90);
  }else check(w.grenths_balance(caster,npc,200,mode==2)==(mode<4));
  if(mode<4){
   const auto hp_caster=mode==2?0u:mode==3?350u:300u;
   const auto hp_npc=mode==2?250u:mode==3?350u:300u;
   check(w.resources(1)->health==hp_caster&&w.npc_combat_snapshot(npc)->health->current==hp_npc);
   const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(100,1,86))==1);
  }
 }
 std::cout<<"PASS Grenth combat-NPC forward/reverse/reaction-death/wounded caster, no playerResources, body164 and stale/range rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
