#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
std::string context;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Arrow variant line "+std::to_string(at.line())+context);}
struct Fixture:warrior_test::Fixture{
 Fixture(unsigned id,bool npc,unsigned health=1000):warrior_test::Fixture(id,npc,health){world.leave(1);world.join(1,"Archer",-190,0,box,0,{}, {},100,1);world.set_vitals(1,{1000,1000,30,30},100);source=*world.effect_scope(1);check(world.energy_capacity(1,1,0,100,{10,-2})&&world.ordinary_weapon(source,weapon,20));}
 void enchant(std::uint64_t now){check(world.apply_health_regeneration(source,1,*regeneration::reviewed(258,12),now,now+6000,1248,true,source));}
 void hex(std::uint64_t now){world.join(3,"Hex cause",-130,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);world.set_vitals(3,{1000,1000,30,30},100);check(world.degeneration_hex(*world.effect_scope(3),source,*degeneration_hex(129,0),now));}
};
int main()try{
 for(unsigned r=0;r<=20;++r){auto p=combat::weapon::attack_skill(1467,r);check(p&&p->bonus==10+r&&p->projectile_speed_percent==50&&p->required_type==5);auto forged=*p;forged.projectile_speed_percent=100;check(!combat::weapon::valid(forged));}
 check(!combat::weapon::arcing_shot(21)&&combat::weapon::forked_arrow());
 {const Scope owner{1,1,1},foreign{1,1,2};Registry registry(owner);check(combat::weapon::forked_arrow_count(registry,owner,100)==2);check(!combat::weapon::forked_arrow_count(registry,foreign,100));
  check(registry.apply(owner,{1,1,258,12,Category::enchantment,100,200},Replace::independent).accepted);check(combat::weapon::forked_arrow_count(registry,owner,99)==2&&combat::weapon::forked_arrow_count(registry,owner,100)==1&&combat::weapon::forked_arrow_count(registry,owner,200)==2);
  check(registry.apply(owner,{2,1,129,0,Category::hex,200,300},Replace::independent).accepted);check(combat::weapon::forked_arrow_count(registry,owner,200)==1);registry.clear(owner);
  check(registry.apply(owner,{3,1,346,0,Category::stance,100,300},Replace::independent).accepted);check(combat::weapon::forked_arrow_count(registry,owner,200)==2);
 }
 for(bool npc:{false,true})for(bool prepared:{false,true})for(unsigned mode=0;mode<4;++mode){context=" Arcing npc="+std::to_string(npc)+" prep="+std::to_string(prepared)+" mode="+std::to_string(mode);Fixture f(1467,npc);auto& w=f.world;unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return 0.;});
  if(prepared)check(w.status_mutation(f.source,432,123,12,100,24100,true));
  if(mode==1||mode==3){if(npc)check(w.stage_npc_shield_deflection(f.target,*shield_deflection(12),100));else check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));}
  if(mode==2||mode==3)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,3000));
  const auto arrival=prepared?1100u:1200u;check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(1467,12,0),1000));w.tick(arrival-100);check(f.health()==1000);w.tick(arrival);
  const auto events=w.take_attack_outcomes(f.source);check(events.size()==1&&events[0].at==arrival&&events[0].outcome==(mode>=2?combat::weapon::AttackOutcome::miss:combat::weapon::AttackOutcome::hit)&&draws==0);
  check((f.health()<1000)==(mode<2));if(mode==0)check(f.health()==1000-(prepared?51:42));
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<9;++mode){context=" Forked npc="+std::to_string(npc)+" mode="+std::to_string(mode);Fixture f(1722,npc,mode==6?1:1000);auto& w=f.world;Slots pools{};pools[0]={340,0};check(w.configure_adrenaline(f.source,pools,100));
  if(mode==1||mode==3)f.enchant(100);if(mode==2||mode==3)f.hex(100);if(mode==4)check(w.status_mutation(f.source,346,123,0,100,3000,true));if(mode==5)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,3000));
  if(mode==7){w.configure_attack_block_draws([]{return 0.;});if(npc)check(w.stage_npc_shield_deflection(f.target,*shield_deflection(12),100));else check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));}
  if(mode==8)check(w.status_mutation(f.source,432,123,12,100,24100,true));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(1722,0,0),1000));w.tick(1100);
  const unsigned arrows=mode>=1&&mode<=3?1:2;check(f.health()==((mode==5||mode==7)?1000:mode==6?0:1000-(mode==8?29u:20u)*arrows));const auto events=w.take_attack_outcomes(f.source);check(events.size()==arrows);
  for(const auto& e:events)check(e.outcome==(mode==5?combat::weapon::AttackOutcome::miss:mode==7?combat::weapon::AttackOutcome::block:mode==6&&e.event!=events[0].event?combat::weapon::AttackOutcome::target_retired:combat::weapon::AttackOutcome::hit));
  check(w.adrenaline(f.source)->front().units==((mode==5||mode==7)?0u:mode==6?25u:25*arrows));
 }
 for(unsigned id:{1467u,1722u})for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){context=" Runtime id="+std::to_string(id)+" npc="+std::to_string(npc)+" mode="+std::to_string(mode);Fixture f(id,npc);auto& w=f.world;casting::Runtime runtime(1);casting::Hooks h;std::vector<Packet> wire;
  h.events=[&](const auto& p){wire.insert(wire.end(),p.begin(),p.end());};h.resources=[&](auto actor){return w.resources(actor);};h.target_scope=[&](auto actor){return w.effect_scope(actor);};h.consume_energy_at=[&](auto n,auto at){check(n==(id==1467?5u:10u));return w.consume_energy(1,n,f.source,at);};
  h.weapon_snapshot=[&](){return std::optional(f.weapon);};h.melee_target=[&](auto actor,auto at){return w.melee_target(f.source,actor,at,id);};h.melee_attack=[&](auto target,const auto& gear,const auto& plan,auto at){return w.melee_attack(f.source,target,gear,plan,at);};h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(f.source,at);};h.attack_activity=[&](auto until){w.attack_activity(f.source,until,1000);};
  Policy p;p.primary=2;p.campaigns=255;p.learned[id]=1;Slots bar{};bar[0]={static_cast<std::uint16_t>(id),0};attributes::Ranks ranks{};ranks[25]=12;if(mode==5)h.melee_attack={};
  check(runtime.start(1000,{id,0,f.target.actor},bar,p,ranks,true,h)==(mode==5?casting::Reason::unsupported:casting::Reason::accepted));if(mode==5){check(w.resources(1)->energy==30);continue;}
  if(mode==1)f.enchant(1200);if(mode==3)runtime.cancel(h);if(mode==4)runtime.interrupt(1200,h);runtime.update(1500,h);if(mode==2)f.enchant(1550);w.tick(id==1467?1700:1600);
  const unsigned arrows=mode==3||mode==4?0:id==1722&&mode!=1?2:1;check(f.health()==1000-arrows*(id==1467?42:20));check(w.take_attack_outcomes(f.source).size()==arrows);check(w.resources(1)->energy==(id==1467?25u:20u));
  check(std::count(wire.begin(),wire.end(),casting::activation(1,f.target.actor,id,true))==1&&std::count(wire.begin(),wire.end(),casting::terminal(46,1))==0);check(!particles::target_body_2022(f.target.actor,id)&&std::count(wire.begin(),wire.end(),combat::detail::integer(21,1,2077))==0);
  if(mode==0){const auto until=id==1467?7500u:6500u;check(runtime.start(until-1,{id,0,f.target.actor},bar,p,ranks,true,h)==casting::Reason::recharge);check(runtime.start(until,{id,0,f.target.actor},bar,p,ranks,true,h)==casting::Reason::accepted);runtime.cancel(h);}
 }
 std::cout<<"PASS Arcing/Forked arrows nativeguards/ranks/slow and RTW flight/unblockable/Blind/playerNPC/scoped enchant-hex count/separate hit events/lethal/Runtime costs,recharge and release snapshot\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
