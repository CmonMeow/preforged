#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Feast casting regression line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<14;++mode){
  using namespace skills::casting;world::Instance w;const std::string hash(64,'a');
  std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 60 0 0 - -\n");auto population=world::load_npc_catalog(appearance);
  std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/feast_of_corruption_cast.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test 60 test\n");
  const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(population,catalog,hash);w.configure_npcs(population.for_map(146));
  for(unsigned id=1;id<=4;++id){w.join(id,"Actor",id==1||id==4?0.f:id==2?60.f:100.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==4?1:2);w.set_vitals(id,{id==1?100u:500u,500,100,100},100);}
  const auto caster=*w.effect_scope(1),ally=*w.effect_scope(4);const unsigned selected=mode>=8?100:2;
  check(w.degeneration_hex(caster,*w.effect_scope(selected),*skills::effects::degeneration_hex(101,0),100));
  const auto kill_npc=[&]{combat::weapon::Owned sword{1,1,{500,500,2},12,false,false,27};check(w.ordinary_weapon(ally,sword,20)&&w.melee_attack(ally,*w.effect_scope(100),sword,*combat::weapon::attack_skill(322,0),200));};
  if(mode==6)check(w.kill(*w.effect_scope(2),200));if(mode==10)kill_npc();
  Hooks h;unsigned commits=0;std::vector<skills::Packet> sent;
  h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};h.resources=[&](unsigned id){return w.resources(id);};
  h.consume_energy=[&](unsigned amount){check(amount==15);return w.consume_energy(1,amount,caster);};h.target_scope=[&](unsigned id){auto scope=w.effect_scope(id);if(scope&&mode==12&&id==selected)scope->generation=0;if(scope&&mode==13&&id==1)scope->actor=999;return scope;};
  h.projectile_target_valid=[&](unsigned id,float range){check(range==1248);if(id!=selected)return false;const auto player=w.resources(id);if(player)return player->health>0;const auto scope=w.effect_scope(id);if(!scope)return false;const auto npc=w.npc_combat_snapshot(*scope);return npc&&npc->health&&npc->health->current>0;};
  if(mode!=5)h.feast_of_corruption=[&](const auto& anchor,const auto& plan,auto now){check(anchor.started==100&&anchor.caster==caster&&now==2100&&plan.rank==12);++commits;return w.feast_of_corruption(*w.effect_scope(1),anchor,plan,now);};
  skills::Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[151]=1;skills::Slots slots{};slots[0]={151,0};attributes::Ranks ranks{};ranks[7]=12;Runtime cast(1);
  const auto reason=cast.start(mode==6||mode==10?300:100,{151,0,selected},slots,policy,ranks,true,h);
  if(mode==5||mode==6||mode==10||mode>=12){check(reason==(mode==5?Reason::unsupported:Reason::target)&&w.resources(1)->energy==100);continue;}
  check(reason==Reason::accepted&&w.resources(1)->energy==85);
  if(mode==1)check(w.kill(*w.effect_scope(2),200));
  if(mode==2){w.leave(2);w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},200,2);w.set_vitals(2,{500,500,100,100},100);}
  if(mode==7)check(w.kill(caster,200));if(mode==9)kill_npc();
  if(mode==11){w.leave(1);w.join(1,"New caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},200,1);w.set_vitals(1,{100,500,100,100},100);}
  cast.update(2099,h);check(commits==0);if(mode==3)cast.cancel(h);if(mode==4)cast.interrupt(500,h);
  cast.update(2100,h);cast.update(2100,h);
  const bool applied=mode==0||mode==1||mode==8||mode==9;check(commits==(applied?1u:0u));
  if(applied){check(w.resources(3)->health==433&&w.resources(1)->health==(mode==0||mode==8?134u:100u));
   if(mode<8)check(w.resources(2)->health==(mode==1?0u:399u));else check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==(mode==9?0:399));
   check(std::count(sent.begin(),sent.end(),slot_event(228,1,{151,0},20))==1);
  }else check(w.resources(3)->health==500);
  if(mode==4)check(std::count(sent.begin(),sent.end(),slot_event(228,1,{151,0},20))==1);
  if(mode==3)check(std::count(sent.begin(),sent.end(),slot_event(228,1,{151,0},20))==0);
 }
 std::cout<<"PASS Feast actual cast/cost/recharge,player/NPC death during cast,replacement/cancel/interrupt/dead caster/missing-hook/dead-start gates and once-only completion\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
