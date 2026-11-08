#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Crippling Shot line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=combat::weapon::attack_skill(393,rank);check(p&&p->required_type==5&&!p->bonus&&p->arrows==1&&p->projectile_speed_percent==100&&combat::weapon::condition_duration(*p)==1000ull*std::lround(1.+10.*rank/15.));}
 for(bool npc:{false,true})for(unsigned mode=0;mode<7;++mode){using namespace skills;warrior_test::Fixture f(393,npc);unsigned draws=0,commits=0;bool stale=false;
 if(!npc)check(f.world.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));
 f.world.configure_attack_block_draws([&]{++draws;return 0.;});
 if(mode==1)check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::blind,300,5000));
 Slots bar{};bar[0]={393,0};Policy policy;policy.primary=2;policy.campaigns=255;policy.learned[393]=1;attributes::Ranks ranks{};ranks[25]=12;
 std::vector<world::Packet> packets;casting::Hooks h;h.resources=[&](unsigned id){return f.world.resources(id);};h.events=[&](const auto& p){packets.insert(packets.end(),p.begin(),p.end());};h.consume_energy_at=[&](unsigned n,auto at){check(n==10);return f.world.consume_energy(1,n,f.source,at);};h.target_scope=[&](unsigned id){auto scope=f.world.effect_scope(id);if(stale&&id==f.target.actor)return std::optional<effects::Scope>{};return scope;};h.weapon_snapshot=[&](){auto gear=f.weapon;if(mode==6)gear.item_type=27;return std::optional{gear};};h.melee_target=[&](unsigned id,auto at){return f.world.melee_target(f.source,id,at,393);};if(mode!=5)h.melee_attack=[&](auto target,const auto& gear,const auto& plan,auto at){++commits;return f.world.melee_attack(f.source,target,gear,plan,at);};h.attack_activity=[&](auto until){f.world.attack_activity(f.source,until,1000);};h.attack_interrupt_epoch=[&](){return f.world.attack_interrupt_epoch(f.source);};h.weapon_attack_timing=[&](auto at){return f.world.weapon_attack_timing(f.source,at);};casting::Runtime cast(1);
 check(cast.start(1000,{393,0,f.target.actor},bar,policy,ranks,true,h)==(mode==5?casting::Reason::unsupported:mode==6?casting::Reason::weapon:casting::Reason::accepted));
 if(mode>=5){check(f.world.resources(1)->energy==30);continue;}
 check(f.world.resources(1)->energy==20);
 check(std::count(packets.begin(),packets.end(),casting::activation(1,f.target.actor,393,true))==1);
 if(mode==2)cast.cancel(h);if(mode==3)cast.interrupt(1200,h);if(mode==4)stale=true;
 cast.update(1499,h);check(!commits);cast.update(1500,h);f.world.tick(1600);
 check(!draws);check(f.health()==(mode==0?980:1000));
 check(f.world.has_condition(f.target,combat::conditions::Kind::crippled,1600)==(mode==0));
 const auto outcomes=f.world.take_attack_outcomes(f.source);check(outcomes.size()==unsigned(mode<2));if(mode<2)check(outcomes.front().outcome==(mode==1?combat::weapon::AttackOutcome::miss:combat::weapon::AttackOutcome::hit));
 }
 // A condition-immune NPC still receives the weapon hit without Crippled.
 {warrior_test::Fixture f(393,true,1000,true,false);check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(393,12),1000));f.world.tick(1100);check(f.health()==980&&!f.world.has_condition(f.target,combat::conditions::Kind::crippled,1100));}
 for(bool npc:{false,true}){warrior_test::Fixture f(393,npc,1);check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(393,12),1000));f.world.tick(1100);check(f.health()==0&&!f.world.has_condition(f.target,combat::conditions::Kind::crippled,1100));}
 std::cout<<"PASS Crippling Shot native duration,player/NPC projectile hit,Guardian bypass,Blind,cancel/interrupt,stale target,wrong weapon and NPC immunity\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
