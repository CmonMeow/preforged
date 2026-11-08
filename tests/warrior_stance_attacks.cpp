#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <array>
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
std::string context;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Stance attacks line "+std::to_string(at.line())+context);}
int main()try{
 constexpr std::array<unsigned,21> jaiz{1,3,5,7,9,11,13,15,16,18,20,22,24,26,28,30,32,34,36,38,40};
 for(unsigned rank=0;rank<=20;++rank){
  const auto a=combat::weapon::attack_skill(996,rank,0),b=combat::weapon::attack_skill(1135,rank,0);check(a&&b&&a->required_type==27&&b->required_type==27);
  check(a->bonus==5+rank&&a->stance_bonus==5+rank&&b->bonus==jaiz[rank]&&b->no_stance_unblockable);
  auto forged=*a;++forged.stance_bonus;check(!combat::weapon::valid(forged));forged=*b;forged.no_stance_unblockable=false;check(!combat::weapon::valid(forged));
 }check(!combat::weapon::attack_skill(996,21)&&!combat::weapon::attack_skill(1135,21));
 for(unsigned id:{996u,1135u,1695u})for(bool npc:{false,true})for(unsigned mode=0;mode<5;++mode){
  context=" direct id="+std::to_string(id)+" npc="+std::to_string(npc)+" mode="+std::to_string(mode);
  warrior_test::Fixture f(id,npc);auto& w=f.world;if(id==1695){f.weapon.item_type=27;check(w.ordinary_weapon(f.source,f.weapon,20));}
  if(mode==1||mode==2)check(w.status_mutation(f.source,346,123,0,100,mode==2?200:3000,true));
  if(mode==3)check(w.for_great_justice(f.source,*for_great_justice(),100)); // shout is not a stance
  if(mode==4)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,3000));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(id,12,0),200));
  const unsigned bonus=id==996?(mode==1?34u:17u):id==1135?24u:34u;check(f.health()==1000-(mode==4?0:20+bonus));
 }
 for(unsigned id:{996u,1135u,1695u})for(bool npc:{false,true})for(bool stance:{false,true})for(bool miss:{false,true}){
  context=" block id="+std::to_string(id)+" npc="+std::to_string(npc)+" stance="+std::to_string(stance)+" miss="+std::to_string(miss);
  warrior_test::Fixture f(id,npc);auto& w=f.world;if(id==1695){f.weapon.item_type=27;check(w.ordinary_weapon(f.source,f.weapon,20));}w.configure_attack_block_draws([]{return 0.;});
  if(npc)check(w.stage_npc_shield_deflection(f.target,*shield_deflection(12),100));else check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));
  if(stance)check(w.status_mutation(f.source,346,123,0,100,3000,true));if(miss)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,3000));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(id,12,0),200));const bool hit=id==1135&&!stance&&!miss;
  const auto events=w.take_attack_outcomes(f.source);check(events.size()==1&&events[0].outcome==(miss?combat::weapon::AttackOutcome::miss:hit?combat::weapon::AttackOutcome::hit:combat::weapon::AttackOutcome::block));check((f.health()<1000)==hit);
 }
 for(unsigned id:{996u,1135u,1695u})for(bool npc:{false,true})for(unsigned mode=0;mode<7;++mode){
  context=" Runtime id="+std::to_string(id)+" npc="+std::to_string(npc)+" mode="+std::to_string(mode);
  warrior_test::Fixture f(id,npc);auto& w=f.world;if(id==1695){f.weapon.item_type=27;check(w.ordinary_weapon(f.source,f.weapon,20));}Slots bar{};bar[0]={static_cast<std::uint16_t>(id),0};check(w.configure_adrenaline(f.source,bar,100));
  if(id==996){for(unsigned n=0;n<6;++n)check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(322,0,0),101+n));check(w.adrenaline(f.source)->front().units==140);}
  if(mode==2||mode==3)check(w.status_mutation(f.source,346,123,0,900,mode==2?1500:3000,true));
  casting::Runtime cast(1);casting::Hooks h;std::vector<Packet> wire;unsigned commits=0;
  h.events=[&](const auto& p){wire.insert(wire.end(),p.begin(),p.end());};h.resources=[&](auto actor){return w.resources(actor);};h.target_scope=[&](auto actor){return w.effect_scope(actor);};
  h.weapon_snapshot=[&](){auto v=f.weapon;if(mode==6)v.item_type=5;return std::optional(v);};h.melee_target=[&](auto actor,auto at){return w.melee_target(f.source,actor,at,id);};
  h.consume_energy_at=[&](auto n,auto at){return w.consume_energy(1,n,f.source,at);};h.consume_adrenaline=[&](auto slot,auto at){return w.consume_adrenaline(f.source,slot,at);};h.reconcile_adrenaline=[&](auto at){w.reconcile_adrenaline(f.source,at);};
  h.melee_attack=[&](auto target,const auto& gear,const auto& plan,auto at){++commits;return w.melee_attack(f.source,target,gear,plan,at);};
  // Reviewed timing hook pins this fixture's release to500ms. This is not a
  // retail total-cycle timing claim or an inferred capture animation speed.
  h.weapon_attack_timing=[](auto){return std::optional(combat::weapon::Timing{500,1000,128});};
  h.attack_activity=[&](auto until){w.attack_activity(f.source,until,1000);};
  Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[id]=1;attributes::Ranks ranks{};ranks[20]=ranks[21]=12;
  const auto before=f.health();const auto admitted=cast.start(1000,{id,0,f.target.actor},bar,policy,ranks,true,h);check(admitted==(mode==6?casting::Reason::weapon:casting::Reason::accepted));if(mode==6)continue;
  check(std::count(wire.begin(),wire.end(),casting::activation(1,f.target.actor,id,true))==1);check(w.resources(1)->energy==(id==996?30u:25u));
  if(mode==1){if(id==1695)check(w.for_great_justice(f.source,*for_great_justice(),1200));else check(w.status_mutation(f.source,346,123,0,1200,3000,true));}if(mode==3)check(w.status_mutation(f.source,346,123,0,1200,{},false));
  if(mode==4)cast.cancel(h);if(mode==5)cast.interrupt(1200,h);cast.update(1499,h);check(!commits);cast.update(1500,h);cast.update(1500,h);
  check(commits==unsigned(mode<4));const unsigned bonus=id==996?(mode==1?34u:17u):id==1135?24u:34u;check(f.health()==before-(mode<4?20+bonus:0));
  check(std::count(wire.begin(),wire.end(),casting::terminal(46,1))==unsigned(mode<4));check(std::count(wire.begin(),wire.end(),casting::terminal(49,1))==unsigned(mode==4||mode==5));
  check(std::count(wire.begin(),wire.end(),skills::cancellation_packet(1,{static_cast<std::uint16_t>(id),0}))==1);
  check(std::count(wire.begin(),wire.end(),combat::detail::integer(21,1,2077))==0&&!particles::target_body_2022(f.target.actor,id));
 }
 // Registry fixtures prove only the authoritative predicate, not chant casting.
 {const Scope owner{1,1,1},foreign{1,1,2};Registry registry(owner);
  check(registry.apply(owner,{1,2,1559,0,Category::chant,100,200},Replace::independent).accepted);
  check(!combat::weapon::active_shout_or_chant(registry,owner,99));check(combat::weapon::active_shout_or_chant(registry,owner,100));
  check(!combat::weapon::active_shout_or_chant(registry,owner,200)&&!combat::weapon::active_shout_or_chant(registry,foreign,100));
  registry.clear(owner);check(registry.apply(owner,{2,1,346,0,Category::stance,100,200},Replace::independent).accepted);check(!combat::weapon::active_shout_or_chant(registry,owner,100));
 }
 for(unsigned rank=0;rank<=20;++rank){auto p=combat::weapon::attack_skill(1695,rank,0);check(p&&p->bonus==10+2*rank&&p->shout_unblockable);auto forged=*p;forged.shout_unblockable=false;check(!combat::weapon::valid(forged));}
 for(bool npc:{false,true})for(unsigned mode=0;mode<5;++mode){
  context=" Soldier shout npc="+std::to_string(npc)+" mode="+std::to_string(mode);
  warrior_test::Fixture f(1695,npc);auto& w=f.world;f.weapon.item_type=27;check(w.ordinary_weapon(f.source,f.weapon,20));w.configure_attack_block_draws([]{return 0.;});
  if(npc)check(w.stage_npc_shield_deflection(f.target,*shield_deflection(12),100));else check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));
  if(mode==0||mode==1)check(w.for_great_justice(f.source,*for_great_justice(),100));
  if(mode==2)check(w.status_mutation(f.source,346,123,0,100,3000,true));
  if(mode==3)check(w.resource_gain(f.source,f.target,*resource_gain(1141,0),100).has_value()); // instant shout has no duration effect
  if(mode==4){check(w.for_great_justice(f.source,*for_great_justice(),100));const auto active=*w.status_snapshot(f.source,200);check(active.size()==1);check(w.status_mutation(f.source,343,active[0].instance,0,200,{},false));}
  if(mode==1){
   if(npc)check(w.stage_npc_shield_deflection(f.target,*shield_deflection(12),20000));
   else check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),20000,26000,1248,true,f.target));
  }
  const auto at=mode==1?20100u:300u;check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(1695,12,0),at));
  const auto events=w.take_attack_outcomes(f.source);check(events.size()==1&&events[0].outcome==(mode==0?combat::weapon::AttackOutcome::hit:combat::weapon::AttackOutcome::block));
 }
 {warrior_test::Fixture f(1695,false);auto& w=f.world;f.weapon.item_type=27;check(w.ordinary_weapon(f.source,f.weapon,20));
  check(w.for_great_justice(f.source,*for_great_justice(),100));check(w.kill(f.source,200));check(w.status_snapshot(f.source,200)->empty());
  check(!w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(1695,12,0),300));
 }
 std::cout<<"PASS stance sword attacks native guards/independent ranks/playerNPC contact stance/expiry/shout exclusion/block/miss/Runtime action changes and packet lifecycle\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
