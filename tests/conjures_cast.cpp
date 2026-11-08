#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Conjure cast line "+std::to_string(at.line()));}
int main()try{using namespace skills;
 for(unsigned skill:{182u,207u,221u})for(unsigned mode=0;mode<11;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Elementalist",0,0,box,0,{}, {},100,1);w.join(2,"Foe",10,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{mode==5?1u:480u,480,30,30},100);w.set_vitals(2,{480,480,30,30},100);
  const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);check(w.energy_capacity(1,6,0,100,{10,-2}));
  if(mode==10)w.set_vitals(1,{480,480,9,30},100);
  if(mode==5)check(w.backfire(foe,owner,*effects::backfire(28,12),20,100));
  casting::Hooks h;unsigned commits=0;std::vector<world::Packet> sent;
  h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};h.resources=[&](unsigned id){return w.resources(id);};
  h.consume_energy_at=[&](unsigned cost,auto at){check(cost==10);return w.consume_energy(1,cost,owner,at);};
  h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.target_valid=[](unsigned id,float,bool self){return id==1&&self;};
  h.before_spell_effect=[&](unsigned id,auto event,auto at){return w.before_spell_effect(owner,id,event,at);};
  h.conjure_enchantment=[&](auto scope,const auto& plan,auto at){++commits;check(scope==owner&&plan.skill==skill&&plan.rank==12);return w.conjure_enchantment(scope,plan,at);};
  if(mode==4)h.conjure_enchantment={};if(mode==6)h.consume_energy_at={};if(mode==7)h.before_spell_effect={};if(mode==8)h.target_scope={};if(mode==9)h.target_valid={};
  Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[skill]=1;Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};attributes::Ranks ranks{};ranks[skill==182?10:skill==207?11:8]=12;casting::Runtime cast(1);
  const bool missing=mode==4||(mode>=6&&mode<=9);
  check(cast.start(100,{skill,0,2},bar,policy,ranks,true,h)==(missing?casting::Reason::unsupported:mode==10?casting::Reason::energy:casting::Reason::accepted));
  if(missing||mode==10){check(w.resources(1)->energy==(mode==10?9u:30u)&&!cast.active()&&!commits);continue;}
  check(w.resources(1)->energy==20);cast.update(1099,h);check(!commits);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1099,h);
  if(mode==3){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},1099,1);w.set_vitals(1,{480,480,20,30},100);}
  box->drain();cast.update(1100,h);check(!cast.active()&&commits==(mode==0?1u:0u));
  const auto* row=find_skill(skill);const auto caster_body=casting::generic(21,1,row->caster_body_resource),target_body=casting::generic(21,1,row->target_body_resource);const auto packets=box->drain();
  check(std::count(packets.begin(),packets.end(),caster_body)==unsigned(mode==0)&&std::count(packets.begin(),packets.end(),target_body)==unsigned(mode==0));
  check(std::count(sent.begin(),sent.end(),caster_body)==0&&std::count(sent.begin(),sent.end(),target_body)==0);
  if(mode==0){const auto status=*w.status_snapshot(owner,1100);check(status.size()==1&&status[0].skill==skill&&status[0].rank==12&&status[0].expires==61100);
   const auto c=std::find(packets.begin(),packets.end(),caster_body),t=std::find(packets.begin(),packets.end(),target_body),entry=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>1&&p[0]==65&&p[1]==0;});check(c<t&&t<entry);
   check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{static_cast<std::uint16_t>(skill),0},45))==1);
   check(cast.start(46099,{skill,0,2},bar,policy,ranks,true,h)==casting::Reason::recharge);
   check(w.elemental_armor(foe,60,60)&&w.lightning_armor(foe,60)&&w.configure_projectiles({100,2}));
   combat::weapon::Owned staff{12,1,{20,20,skill==182?5u:skill==207?3u:4u},12,false,true,26};check(w.ordinary_weapon(owner,staff,20)&&w.ordinary_attack(owner,2,1200));w.tick(1700);check(w.resources(2)->health==480);w.tick(1800);check(w.resources(2)->health==443);w.cancel_ordinary_attack(owner);
  }else check(w.status_snapshot(*w.effect_scope(1),1100)->empty());
 }
 std::cout<<"PASS three Conjures1s/10Energy/2022-45s casting,self scope,rank,60s enchantment,captured body order,cancel/interrupt/Backfire/retirement/hooks and cast-to-hit bonus\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
