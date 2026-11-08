#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Debilitating line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=combat::weapon::attack_skill(406,rank);check(p&&p->required_type==5&&!p->bonus&&*combat::weapon::debilitating_shot(rank)==effects::interpolate(1,10,rank));}
 for(bool npc:{false,true})for(unsigned mode=0;mode<8;++mode){const unsigned energy=mode==3?3u:mode==4?0u:mode==6?8u:30u;
 warrior_test::Fixture f(406,npc,mode==5?1:1000,true,true,1,mode==7?std::nullopt:std::optional{combat::npcs::Energy{energy,30}});auto& w=f.world;
 if(!npc){w.set_vitals(2,{mode==5?1u:1000u,1000,energy,30},100);check(w.energy_capacity(2,1,0,100,{10,-2}));}
 if(mode==1)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
 if(mode==2&&!npc){check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));w.configure_attack_block_draws([]{return 0.;});}
 if(mode==6)check(w.apply_mind_wrack(f.source,f.target,12,false,100));
 check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(406,12),1000));w.tick(1100);
 const bool landed=mode!=1&&(npc||mode!=2)&&(!npc||mode!=7);
 check(f.health()==(landed?(mode==5?0:mode==6?897:980):(mode==5?1:1000)));
 if(npc){const auto current=w.npc_energy_snapshot(f.target);if(mode==7)check(!current);else check(current&&current->current==(landed&&mode!=5?(energy>8?energy-8:0):energy));}
 else check(w.resources(2)->energy==(landed&&mode!=5?(energy>8?energy-8:0):energy));
 check(w.resources(1)->energy==30); // Loss never becomes an attacker Energy reward.
 }
 // Non-Overcast and Overcast proposals preserve negative debt and drain a
 // small positive fractional remainder to actual zero.
 {const effects::Scope owner{1,2,3};overcast::CeilingCurve curve(owner,1000,1);vitals::Resources r{100,100,0,30};energy::Regeneration regen;regen.restore_fraction(1500);
 const auto loss=overcast::prepare_loss(curve,owner,r,regen,0,8,1000,0,overcast::LossRule::drain_to_zero);check(loss&&loss->lost_units==150000&&overcast::precise_current(loss->state)==0);
 const auto debt=overcast::prepare_loss(curve,owner,r,energy::Regeneration{},3,8,1000,0,overcast::LossRule::drain_to_zero);check(debt&&debt->lost_units==0&&debt->state.debt==3);}
 // A real non-Overcast World target has only fractional Energy at impact.
 // Its external loss must trigger Mind Wrack and remove the hidden fraction.
 {warrior_test::Fixture f(406,false);auto& w=f.world;w.set_vitals(2,{1000,1000,0,30},100);
 check(w.energy_capacity(2,1,0,100,{10,0}));check(w.apply_mind_wrack(f.source,f.target,12,false,100));
 check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(406,12),1000));w.tick(1100);
 check(f.health()==897&&w.resources(2)->energy==0);check(w.settle_resources(f.target,2100));check(!w.consume_energy(2,1,f.target,2100));}
 for(bool negative:{false,true}){warrior_test::Fixture f(406,false),control(406,false);auto& w=f.world;
 const auto prepare=[&](warrior_test::Fixture& x){auto& world=x.world;world.set_vitals(2,{1000,1000,3,30},100);
 check(world.energy_capacity(2,6,0,100,{10,-2})&&world.activate_overcast_energy(x.target,231,0,1,1000));
 if(negative)for(unsigned event=2;event<=4;++event)check(world.activate_overcast_energy(x.target,231,0,event,1000));
 check(world.energy_capacity(2,6,0,1000,{10,2})&&world.settle_resources(x.target,1500));};prepare(f);prepare(control);
 check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(406,12),1500));w.tick(1600);check(control.world.settle_resources(control.target,1600));
 const auto after=*w.overcast_energy_snapshot(f.target),natural=*control.world.overcast_energy_snapshot(control.target);
 check(overcast::precise_current(after)==(negative?overcast::precise_current(natural):0));
 check(after.curve.burden_units()==natural.curve.burden_units());
 }
 {const combat::npcs::Lifetime life{1,1,2,146,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=100;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{100,100};d.damageable=true;d.spawn_energy=combat::npcs::Energy{3,30};d.energy_pips=2;
 combat::npcs::LiveActor npc(d,life);check(npc.advance_energy(life,100)&&npc.advance_energy(life,1100));const auto before=*npc.precise_energy_units(life);check(before==1100000);
 check(npc.drain_energy(life,3)==900000&&npc.precise_energy_units(life)==200000);
 check(npc.drain_energy(life,8)==200000&&npc.precise_energy_units(life)==0);
 auto stale=life;++stale.actor_generation;check(!npc.drain_energy(stale,1));}
 for(unsigned mode=0;mode<3;++mode){warrior_test::Fixture f(406,false);auto& w=f.world;check(w.energy_capacity(2,1,0,100,{10,-2}));unsigned commits=0;
 casting::Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.consume_energy_at=[&](unsigned n,auto at){check(n==10);return w.consume_energy(1,n,f.source,at);};h.target_scope=[&](auto id){return w.effect_scope(id);};h.weapon_snapshot=[&](){return std::optional{f.weapon};};h.melee_target=[&](auto id,auto at){return w.melee_target(f.source,id,at,406);};h.melee_attack=[&](auto target,const auto& gear,const auto& plan,auto at){++commits;return w.melee_attack(f.source,target,gear,plan,at);};h.attack_activity=[&](auto until){w.attack_activity(f.source,until,1000);};h.attack_interrupt_epoch=[&](){return w.attack_interrupt_epoch(f.source);};h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(f.source,at);};
 Policy p;p.primary=2;p.campaigns=255;p.learned[406]=1;Slots bar{};bar[0]={406,0};attributes::Ranks ranks{};ranks[25]=12;casting::Runtime cast(1);
 check(cast.start(1000,{406,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);check(w.resources(1)->energy==20);if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1200,h);
 cast.update(1499,h);check(!commits);cast.update(1500,h);w.tick(1600);check(w.resources(2)->energy==(mode?30u:22u));check(f.health()==(mode?1000:980));}
 std::cout<<"PASS Debilitating player/known-NPC capped Energy loss,miss/block/zero/death/unknown suppression,Mind Wrack,fraction/debt proposal and Runtime lifecycle\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
