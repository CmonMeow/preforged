#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
std::string context;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Thrill of Victory line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned rank:{0u,7u,8u,12u,20u})for(bool npc:{false,true})for(unsigned mode=0;mode<5;++mode){
  warrior_test::Fixture f(324,npc);f.weapon.item_type=27;check(f.world.ordinary_weapon(f.source,f.weapon,20));
  // Test absolute current Health, including equal/less Health before a hit
  // that subsequently lowers the foe below the attacker.
  const unsigned source_health=mode==0?1000:mode==1?999:mode==2?1001:1500;
  f.world.set_vitals(1,{source_health,2000,30,30},100);
  skills::Slots bar{};bar[0]={324,0};bar[1]={331,0};check(f.world.configure_adrenaline(f.source,bar,100));
  if(mode==3)check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
  if(mode==4&&!npc){f.world.configure_attack_block_draws([]{return 0.;});check(f.world.status_mutation(f.target,448,123,12,100,100+*skills::effects::escape(448,12),true));}
  const auto plan=*combat::weapon::attack_skill(324,rank,0);
  check(plan.bonus==unsigned(std::lround(20.+20.*rank/15.)));
  check(plan.health_advantage_adrenaline==unsigned(std::lround(1.+rank/15.)));
  check(f.world.melee_attack(f.source,f.target,f.weapon,plan,200));
  const bool hit=mode!=3&&(mode!=4||npc);
  check(f.health()==1000-(hit?20+plan.bonus:0));
  const auto pools=f.world.adrenaline(f.source);check(pools&&pools->size()==1);
  check(pools->front().units==(hit?25+(source_health>1000?25*plan.health_advantage_adrenaline:0):0));
 }
 for(unsigned mode=0;mode<4;++mode){warrior_test::Fixture f(324,false,900);f.weapon.item_type=27;check(f.world.ordinary_weapon(f.source,f.weapon,20));
  skills::Slots bar{};bar[0]={324,0};skills::Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[324]=1;attributes::Ranks ranks{};ranks[21]=12;
  casting::Hooks h;std::vector<world::Packet> events;bool stale=false;h.resources=[&](unsigned id){return f.world.resources(id);};h.events=[&](const auto& packets){events.insert(events.end(),packets.begin(),packets.end());};
  h.target_scope=[&](unsigned id){if(stale&&id==2)return std::optional<skills::effects::Scope>{};return f.world.effect_scope(id);};h.weapon_snapshot=[&](){return std::optional(f.weapon);};
  h.melee_target=[&](unsigned id,auto now){return f.world.melee_target(f.source,id,now,324);};h.consume_energy_at=[&](unsigned cost,auto now){check(cost==5);return f.world.consume_energy(1,cost,f.source,now);};
  h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto now){return f.world.melee_attack(f.source,target,weapon,plan,now);};
  h.weapon_attack_timing=[&](auto now){return f.world.weapon_attack_timing(f.source,now);};h.attack_activity=[&](auto until){f.world.attack_activity(f.source,until,1000);};
  skills::Slots pools{};pools[0]={324,0};pools[1]={331,0};check(f.world.configure_adrenaline(f.source,pools,1000));
  casting::Runtime runtime(1);check(runtime.start(1000,{324,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted&&f.world.resources(1)->energy==25);
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1200,h);if(mode==3)stale=true;
  runtime.update(1499,h);check(f.health()==900);runtime.update(1500,h);
  check(!f.world.knocked_down(f.target,1500));check((f.health()<900)==(mode==0));
  check(std::count(events.begin(),events.end(),casting::terminal(46,1))==unsigned(mode==0));
  const auto reward=f.world.adrenaline(f.source);check(reward&&reward->front().units==unsigned(mode==0?75:0));
  const auto before=f.health();runtime.update(1500,h);check(f.health()==before);
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<4;++mode){
  context=" reward mode="+std::to_string(mode)+" npc="+std::to_string(npc);
  warrior_test::Fixture f(324,npc,mode==3?5:900);f.weapon.item_type=27;check(f.world.ordinary_weapon(f.source,f.weapon,20));
  skills::Slots bar{};bar[0]={324,0};bar[1]={331,0};bar[2]={335,0};bar[3]={335,1};check(f.world.configure_adrenaline(f.source,bar,100));
  // Soothing Images forbids both ordinary-hit and skill-granted adrenaline.
  // Exact expiry must restore gain, without carrying a blocked reward forward.
  if(mode<2)check(f.world.status_mutation(f.source,56,123,12,100,mode==0?201:200,true));
  const auto plan=*combat::weapon::attack_skill(324,12,0);
  check(f.world.melee_attack(f.source,f.target,f.weapon,plan,200));
  auto pools=*f.world.adrenaline(f.source);check(pools.size()==3);
  for(const auto& pool:pools)check(pool.units==unsigned(mode==0?0:75));
  if(mode==2){
   check(f.world.melee_attack(f.source,f.target,f.weapon,plan,700));
   pools=*f.world.adrenaline(f.source);check(pools[0].units==130&&pools[1].units==80&&pools[2].units==80);
  }
  if(mode==3){check(f.health()==0);check(!f.world.melee_attack(f.source,f.target,f.weapon,plan,700));
   const auto final_pools=*f.world.adrenaline(f.source);for(const auto& pool:final_pools)check(pool.units==75);}
 }
 for(bool npc:{false,true}){
  warrior_test::Fixture f(324,npc,900);f.weapon.item_type=27;check(f.world.ordinary_weapon(f.source,f.weapon,20));
  check(f.world.health_capacity(f.source,1001,300));
  check(!f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(324,12,0),200));check(f.health()==900);
 }
 combat::weapon::Strike strike;strike.level=20;strike.mastery=12;strike.minimum=strike.maximum=20;strike.location_armor=60;strike.base_roll=strike.miss_roll=.5;
 const auto plan=*combat::weapon::attack_skill(324,12,0);
 check(!combat::weapon::resolve_attack_skill(strike,plan,false)); // Missing authoritative Health predicate.
 combat::weapon::ConditionContact known;known.attacker_health_greater=true;
 const auto hit=combat::weapon::resolve_attack_skill(strike,plan,false,false,false,false,known);check(hit&&hit->adrenaline==2);
 combat::adrenaline::State disabled;skills::Slots bar{};bar[0]={331,0};bar[1]={335,0};check(disabled.configure(bar,100));
 check(disabled.disable({331,0},300,100));disabled.gain(25+25*hit->adrenaline,200);
 check(disabled.pools()[0].units==0&&disabled.pools()[1].units==75);
 disabled.gain(25+25*hit->adrenaline,300);check(disabled.pools()[0].units==75&&disabled.pools()[1].units==80);
 auto forged=plan;++forged.health_advantage_adrenaline;check(!combat::weapon::valid(forged));check(!combat::weapon::attack_skill(324,21));
 std::cout<<"PASS Thrill of Victory damage,absolute pre-contact Health predicate,equal/lower/higher Health,rank rounding,player/NPC recipients,hit/miss/block and ordinary plus conditional adrenaline\n";
}catch(const std::exception& error){std::cerr<<error.what()<<context<<'\n';return 1;}



