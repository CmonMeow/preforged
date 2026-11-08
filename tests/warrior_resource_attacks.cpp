#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
std::string context;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Resource attack line "+std::to_string(at.line())+context);}
unsigned charge(world::Instance& w,Scope s){auto p=w.adrenaline(s);check(p.has_value());for(const auto& v:*p)if(v.slot.skill==340)return v.units;throw std::runtime_error("pool");}
int main()try{
 for(unsigned id:{904u,905u,1409u})for(unsigned rank=0;rank<=20;++rank){auto p=combat::weapon::attack_skill(id,rank,0);check(p&&p->required_type==(id==904?2u:15u));auto forged=*p;++forged.hit_adrenaline;check(!combat::weapon::valid(forged));check(!combat::weapon::attack_skill(id,21));if(id==904)check(!combat::weapon::blocked_attack_effect(*p,combat::weapon::AttackOutcome::block));}
 for(unsigned id:{904u,905u,1409u})for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){
  context=" id="+std::to_string(id)+" npc="+std::to_string(npc)+" mode="+std::to_string(mode);
  warrior_test::Fixture f(id,npc,mode==5?1:1000);auto& w=f.world;Slots bar{};bar[0]={340,0};check(w.configure_adrenaline(f.source,bar,100));
  w.set_vitals(1,{1000,1000,10,30},100);if(mode==1)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
  if(mode==2)check(f.condition(combat::conditions::Kind::weakness,100,5000));
  if(mode==3)check(w.for_great_justice(f.source,*for_great_justice(),100));
  if(mode==4)check(w.status_mutation(f.source,56,123,0,100,8100,true));
  const auto p=*combat::weapon::attack_skill(id,12,0);const auto before=f.health();check(w.melee_attack(f.source,f.target,f.weapon,p,200));
  const bool hit=mode!=1;check(f.health()==std::max(0.,before-(hit?20+p.bonus:0)));
  check(w.resources(1)->energy==10+(hit&&id==905?p.refund:0));
  check(charge(w,f.source)==(!hit||mode==4?0u:(25+(id==1409?50u:0u))*(mode==3?2u:1u)));
 }
 for(unsigned id:{904u,905u,1409u})for(bool npc:{false,true})for(bool weakness:{false,true})for(bool miss:{false,true}){
  warrior_test::Fixture f(id,npc);auto& w=f.world;Slots bar{};bar[0]={340,0};check(w.configure_adrenaline(f.source,bar,100));w.set_vitals(1,{1000,1000,10,30},100);
  if(npc)check(w.stage_npc_shield_deflection(f.target,*shield_deflection(12),100));
  else check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));w.configure_attack_block_draws([]{return 0.;});
  if(weakness)check(f.condition(combat::conditions::Kind::weakness,100,5000));if(miss)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
  const auto p=*combat::weapon::attack_skill(id,12,0);check(w.melee_attack(f.source,f.target,f.weapon,p,200));const bool hit=!miss&&id==905&&weakness;
  const auto events=w.take_attack_outcomes(f.source);check(events.size()==1&&events[0].outcome==(miss?combat::weapon::AttackOutcome::miss:hit?combat::weapon::AttackOutcome::hit:combat::weapon::AttackOutcome::block));
  check(charge(w,f.source)==(miss?0u:hit?25u:id==904?75u:0u));check(w.resources(1)->energy==10+(hit?p.refund:0));
 }
 // A lethal Gladiator block reaction resets source adrenaline. Furious Axe
 // must not grant its blocked reward back into a dead source's cleared pools.
 {warrior_test::Fixture f(904,false);auto& w=f.world;Slots bar{};bar[0]={340,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.ordinary_weapon(f.target,f.weapon,20));w.configure_attack_block_draws([]{return .1;});
  check(w.status_mutation(f.target,372,123,12,100,10100,true));w.set_vitals(1,{20,1000,30,30},100);
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(904,12,0),300));check(w.resources(1)->health==0&&charge(w,f.source)==0&&f.health()==1000);
  const auto events=w.take_attack_outcomes(f.source);check(events.size()==1&&events[0].outcome==combat::weapon::AttackOutcome::block);
 }
 // Real Runtime consumes native Energy/adrenaline before contact; cancellation
 // and interruption must not invent hit resources. Unknown weapon fails early.
 for(unsigned id:{904u,905u,1409u})for(bool npc:{false,true})for(unsigned mode=0;mode<4;++mode){
  context=" Runtime id="+std::to_string(id)+" npc="+std::to_string(npc)+" mode="+std::to_string(mode);
  warrior_test::Fixture f(id,npc);auto& w=f.world;Slots bar{};bar[0]={static_cast<std::uint16_t>(id),0};check(w.configure_adrenaline(f.source,bar,100));
  if(id==905){for(unsigned i=0;i<5;++i)check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(1409,0),101+i));check(w.adrenaline(f.source)->front().units==120);}
  const auto before=f.health();casting::Runtime cast(1);casting::Hooks h;std::vector<Packet> wire;unsigned commits=0;
  h.events=[&](const auto& p){wire.insert(wire.end(),p.begin(),p.end());};h.resources=[&](auto actor){return w.resources(actor);};h.target_scope=[&](auto actor){return w.effect_scope(actor);};
  h.weapon_snapshot=[&](){auto v=f.weapon;if(mode==3)v.item_type=5;return std::optional(v);};h.melee_target=[&](auto actor,auto at){return w.melee_target(f.source,actor,at,id);};
  h.consume_energy_at=[&](auto n,auto at){return w.consume_energy(1,n,f.source,at);};h.consume_adrenaline=[&](auto slot,auto at){return w.consume_adrenaline(f.source,slot,at);};h.reconcile_adrenaline=[&](auto at){w.reconcile_adrenaline(f.source,at);};
  h.melee_attack=[&](auto target,const auto& gear,const auto& plan,auto at){++commits;return w.melee_attack(f.source,target,gear,plan,at);};h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(f.source,at);};h.attack_activity=[&](auto until){w.attack_activity(f.source,until,1000);};
  Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[id]=1;attributes::Ranks ranks{};ranks[find_skill(id)->attribute]=12;
  const auto admitted=cast.start(1000,{id,0,f.target.actor},bar,policy,ranks,true,h);check(admitted==(mode==3?casting::Reason::weapon:casting::Reason::accepted));if(mode==3)continue;
  check(std::count(wire.begin(),wire.end(),casting::activation(1,f.target.actor,id,true))==1);
  check(w.resources(1)->energy==(id==905?30u:25u));if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1200,h);cast.update(1500,h);cast.update(1500,h);
  check(std::count(wire.begin(),wire.end(),casting::terminal(49,1))==unsigned(mode==1||mode==2));
  // Native226 releases the pending slot on success as well as cancellation;
  // the separate46/49 action terminal identifies the outcome.
  const auto acknowledged=skills::cancellation_packet(1,{static_cast<std::uint16_t>(id),0});
  check(std::count(wire.begin(),wire.end(),acknowledged)==1);
  if(mode==0)check(std::find(wire.begin(),wire.end(),acknowledged)<
      std::find(wire.begin(),wire.end(),casting::slot_event(228,1,{static_cast<std::uint16_t>(id),0},find_skill(id)->recharge)));
  check(std::count(wire.begin(),wire.end(),combat::detail::integer(21,1,2077))==0);
  check(std::count(wire.begin(),wire.end(),combat::detail::integer(21,f.target.actor,2077))==0);
  check(!particles::target_body_2022(f.target.actor,id));
  check(commits==unsigned(mode==0)&&f.health()==before-(mode==0?20+combat::weapon::attack_skill(id,12)->bonus:0));check(std::count(wire.begin(),wire.end(),casting::terminal(46,1))==unsigned(mode==0));
  check(w.resources(1)->energy==(id==905?30u:25u)); // full Energy clips Auspicious reward
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<3;++mode){
  context=" precise Energy npc="+std::to_string(npc)+" mode="+std::to_string(mode);
  warrior_test::Fixture f(905,npc);auto& w=f.world;w.set_vitals(1,{1000,1000,mode==2?29u:0u,30},100);
  // The precise snapshot belongs to an actual owned Overcast curve. Creating
  // it clips only to the ceiling; it does not subtract ten from current Energy.
  check(w.activate_overcast_energy(f.source,231,0,1,100));
  if(mode==0)check(w.energy_capacity(1,1,0,100,{0,-2})); // capacity30->20 preserves deficit: -10
  if(mode==1)check(w.energy_capacity(1,1,0,100,{10,-1})); // one pip, 900ms = .3Energy
  const auto at=mode==1?1000u:100u;check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(905,12,0),at));
  const auto e=w.overcast_energy_snapshot(f.source);check(e.has_value());
  check(overcast::precise_current(*e)==(mode==0?-3*std::int64_t(overcast::energy_unit):mode==1?7*std::int64_t(overcast::energy_unit)+90000:20*std::int64_t(overcast::energy_unit)));
 }
 std::cout<<"PASS resource attacks native plans/playerNPC/hit/miss/lethal/Weakness block/FGJ/forbidden/Runtime costs and cancellation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
