#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
std::string context;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("critical dependent line "+std::to_string(at.line())+context);}
struct Fixture{
 world::Instance world;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope source,target;combat::weapon::Owned weapon;
 Fixture(bool npc,bool bow){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.x=60;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;d.physical_armor=60;d.physical_armor_evidence="test";n.combat_definition=d;world.configure_npcs({n});}
  world.join(1,"Assassin",0,0,box,0,{}, {},100,1);world.set_vitals(1,{1000,1000,30,30},100);source=*world.effect_scope(1);check(world.energy_capacity(1,7,0,100,{10,-2}));world.configure_attack_miss_draws([]{return .5;});
  if(!npc){world.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);world.set_vitals(2,{1000,1000,30,30},100);target=*world.effect_scope(2);check(world.physical_armor(target,60));check(world.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{1,0,20}));}else target=*world.effect_scope(100);
  weapon={12,1,{20,20,bow?1u:2u},12,false,false,bow?5u:2u,0.};check(world.ordinary_weapon(source,weapon,20,100,combat::weapon::CriticalProfile{7,13,20}));
  check(world.consume_energy(1,10,source,100));check(world.resources(1)->energy==20);box->drain();
 }
 double health()const{return target.actor==100?world.npc_combat_snapshot(target)->health->current:double(world.resources(target.actor)->health);}
};
bool critical_notice(const std::vector<Packet>& packets){return std::any_of(packets.begin(),packets.end(),[](const auto& p){return p.size()==18&&p[0]==162&&p[1]==0&&p[2]==17&&p[3]==0&&p[4]==0&&p[5]==0;});}
int main()try{
 const std::array<unsigned,21> keencrit{5,6,8,9,10,12,13,14,16,17,18,20,21,22,24,25,26,28,29,30,32};
 const std::array<unsigned,21> pvpbase{1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,6,6,6,6};
 for(unsigned rank=0;rank<=20;++rank){const auto keen=combat::weapon::attack_skill(1720,rank),pvp=combat::weapon::attack_skill(3147,rank),chop=combat::weapon::attack_skill(1402,rank);check(keen&&pvp&&chop&&keen->bonus==5+rank&&keen->critical_bonus==keencrit[rank]&&pvp->bonus==pvpbase[rank]&&chop->bonus==5+rank&&chop->windup_ms==1000);}
 for(bool npc:{false,true})for(unsigned id:{1720u,3147u,1402u})for(unsigned mode=0;mode<5;++mode){context=" World npc="+std::to_string(npc)+" id="+std::to_string(id)+" mode="+std::to_string(mode);const bool bow=id!=1402;Fixture f(npc,bow);auto& w=f.world;w.configure_attack_critical_draws([&]{return mode==1?.999999:0.;});
  std::optional<combat::npcs::control::Token> token;if(id==1402){if(npc){token=w.begin_npc_action(f.target,combat::npcs::control::Action::spell,100,3000,Slot{281,0});check(bool(token));if(mode==4)check(w.npc_interrupt_stance(f.target,16,12,1000,100));}else{check(w.cast_activity(f.target,281,100,3000));if(mode==4)check(w.status_mutation(f.target,16,123,12,100,100+timing::stance(16,12)->duration,true));}}
  if(mode==2)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,3000));
  if(mode==3){if(npc)check(w.stage_npc_shield_deflection(f.target,*shield_deflection(12),100));else check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));w.configure_attack_block_draws([]{return 0.;});}
  const auto epoch=w.control_epoch(f.target);check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(id,12,0),1000));if(bow)w.tick(1100);const bool hit=mode!=2&&mode!=3;const bool critical=hit&&mode!=1;
  check((f.health()<1000)==hit);check(critical_notice(f.box->drain())==critical);check(w.resources(1)->energy==(critical?23u:20u));
  if(hit){const auto base=id==3147?4u:17u;const auto extra=id==1720?21u:id==3147?25u:0u;const double expected=critical?20*std::sqrt(2.)+base+extra:20+base;check(npc?std::abs(f.health()-(1000-expected))<1e-9:f.health()==std::ceil(1000-expected));}
  if(id==1402){const bool interrupted=critical&&mode!=4;if(npc)check(w.complete_npc_action(*token,3000)==!interrupted);else check((w.control_epoch(f.target)!=epoch)==interrupted);}
 }
 for(bool npc:{false,true})for(unsigned id:{1720u,1402u})for(unsigned mode=0;mode<7;++mode){context=" Runtime npc="+std::to_string(npc)+" id="+std::to_string(id)+" mode="+std::to_string(mode);Fixture f(npc,id==1720);auto& w=f.world;w.configure_attack_critical_draws([]{return 0.;});casting::Runtime rt(1);casting::Hooks h;std::vector<Packet> wire;unsigned commits=0;
  h.events=[&](const auto& p){wire.insert(wire.end(),p.begin(),p.end());};h.resources=[&](auto actor){return w.resources(actor);};h.target_scope=[&](auto actor){return w.effect_scope(actor);};h.consume_energy_at=[&](auto n,auto at){check(n==5);return w.consume_energy(1,n,f.source,at);};h.weapon_snapshot=[&](){return std::optional(f.weapon);};h.critical_attack_ready=[&](auto target,const auto& weapon,auto at){return w.critical_attack_ready(f.source,target,weapon,at);};h.melee_target=[&](auto actor,auto at){return w.melee_target(f.source,actor,at,id);};h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto at){++commits;return w.melee_attack(f.source,target,weapon,plan,at);};h.attack_activity=[&](auto until){w.attack_activity(f.source,until,1000);};h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(f.source,at);};Policy policy;policy.primary=id==1720?2:1;policy.campaigns=255;policy.learned[id]=1;policy.pvp_match=mode==6;Slots bar{};bar[0]={static_cast<std::uint16_t>(id),0};attributes::Ranks ranks{};ranks[id==1720?25:18]=12;
  if(mode==4)h.critical_attack_ready={};if(mode==5)check(w.ordinary_weapon(f.source,f.weapon,20,100));const auto start=rt.start(1000,{id,0,f.target.actor},bar,policy,ranks,true,h);check(start==((mode==4||mode==5)?casting::Reason::unsupported:casting::Reason::accepted));if(mode==4||mode==5){check(w.resources(1)->energy==20);continue;}
  check(w.resources(1)->energy==15);check(std::count(wire.begin(),wire.end(),casting::activation(1,f.target.actor,id,true))==1);if(mode==1)rt.cancel(h);if(mode==2)rt.interrupt(1200,h);if(mode==3)h.critical_attack_ready={};const auto release=id==1720?1500u:2000u;rt.update(release-1,h);check(commits==0);rt.update(release,h);w.tick(release+100);check(commits==unsigned(mode==0||mode==6));check((f.health()<1000)==(mode==0||mode==6));check(w.resources(1)->energy==((mode==0||mode==6)?18u:15u));check(std::count(wire.begin(),wire.end(),casting::generic(21,1,2077))==0);if(mode==1||mode==2||mode==3)check(std::count(wire.begin(),wire.end(),casting::terminal(49,1))==1);if(mode==0||mode==6){check(rt.start(release+(id==1720?(mode==6?12000u:6000u):15000u)-1,{id,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::recharge);check(rt.start(release+(id==1720?(mode==6?12000u:6000u):15000u),{id,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);rt.cancel(h);}
 }
 std::cout<<"critical dependent attacks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
