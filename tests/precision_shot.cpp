#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Precision line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=combat::weapon::attack_skill(400,rank);check(p&&p->required_type==5&&p->windup_ms==1000&&p->bonus==effects::interpolate(3,10,rank));}
 for(bool npc:{false,true})for(bool missed:{false,true}){warrior_test::Fixture f(400,npc);auto& w=f.world;unsigned draws=0;
 if(!npc)check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));
 w.configure_attack_block_draws([&]{++draws;return 0.;});if(missed)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
 check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(400,12),1000));w.tick(1100);check(f.health()==(missed?1000:971));check(!draws);
 }
 // Easily interrupted is a reviewed skill property, not a generic attack flag.
 for(bool npc:{false,true})for(bool bow:{false,true})for(bool missed:{false,true}){warrior_test::Fixture f(bow?403:320,npc);auto& w=f.world;
 std::optional<combat::npcs::control::Token> token;if(npc){token=w.begin_npc_action(f.target,combat::npcs::control::Action::spell,200,5000,Slot{400,22});check(token.has_value());}else check(w.cast_activity(f.target,Slot{400,22},200,5000));
 if(missed)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,200,5000));const auto epoch=npc?std::optional<std::uint64_t>{}:w.control_epoch(f.target);
 f.box->drain();check(w.ordinary_attack(f.source,f.target.actor,1000));w.tick(2000);w.tick(2200);
 if(npc){check(w.complete_npc_action(*token,5000)==missed);const auto wire=f.box->drain();check(std::count(wire.begin(),wire.end(),casting::terminal(49,f.target.actor))==unsigned(!missed));}
 else check((w.control_epoch(f.target)!=epoch)==!missed);
 }
 for(unsigned mode=0;mode<4;++mode){warrior_test::Fixture f(400,false);auto& w=f.world;unsigned commits=0;
 casting::Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.consume_energy_at=[&](unsigned n,auto at){check(n==10);return w.consume_energy(1,n,f.source,at);};h.target_scope=[&](auto id){return w.effect_scope(id);};h.weapon_snapshot=[&](){return std::optional{f.weapon};};h.melee_target=[&](auto id,auto at){return w.melee_target(f.source,id,at,400);};h.melee_attack=[&](auto target,const auto& gear,const auto& plan,auto at){++commits;return w.melee_attack(f.source,target,gear,plan,at);};h.attack_activity=[&](auto until){w.attack_activity(f.source,until,1000);};h.cast_slot_activity=[&](auto slot,auto begins,auto ends){check(w.cast_activity(f.source,slot,begins,ends));};h.control_epoch=[&](){return w.control_epoch(f.source);};h.attack_interrupt_epoch=[&](){return w.attack_interrupt_epoch(f.source);};h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(f.source,at);};
 Policy p;p.primary=2;p.campaigns=255;p.learned[400]=1;Slots bar{};bar[0]={400,0};attributes::Ranks ranks{};ranks[25]=12;casting::Runtime cast(1);
 check(cast.start(1000,{400,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);check(w.resources(1)->energy==20);
 check(w.physical_armor(f.source,60));
 if(mode){combat::weapon::Owned sword{13,1,{10,10,2},12,false,false,27};check(w.ordinary_weapon(f.target,sword,20));
 if(mode==2)check(w.apply_condition(f.target,f.target,combat::conditions::Kind::blind,1100,5000));
 if(mode==3)check(w.status_mutation(f.source,17,123,12,1100,1100+timing::stance(17,12)->duration,true));
 check(w.melee_attack(f.target,f.source,sword,*combat::weapon::attack_skill(320,0),1500));}
 cast.update(1999,h);check(!commits);cast.update(2000,h);w.tick(2100);check(commits==unsigned(mode!=1));check(f.health()==(mode==1?1000:971));if(mode==3)check(w.resources(1)->energy==15);
 }
 {warrior_test::Fixture f(330,true);auto& w=f.world;const auto token=w.begin_npc_action(f.target,combat::npcs::control::Action::spell,200,5000,Slot{400,23});check(token.has_value());
 f.box->drain();check(w.melee_attack(f.source,f.source,f.weapon,*combat::weapon::attack_skill(330,0),1000));check(!w.complete_npc_action(*token,5000));
 const auto wire=f.box->drain();check(std::count(wire.begin(),wire.end(),casting::terminal(49,f.target.actor))==1);check(std::count(wire.begin(),wire.end(),casting::terminal(59,f.target.actor))==0);}
 combat::conditions::State clear;const combat::weapon::AttackActionFacts action{combat::weapon::AttackTargetAction::skill,Slot{400,0}};
 for(auto outcome:{combat::weapon::AttackOutcome::miss,combat::weapon::AttackOutcome::block,combat::weapon::AttackOutcome::dodge,combat::weapon::AttackOutcome::obstruction,combat::weapon::AttackOutcome::cancelled})check(combat::weapon::reviewed_hit_interrupt(clear,action,outcome,100)==false);
 std::cout<<"PASS Precision damage/unblockable,ordinary melee/bow easily-interrupted player/NPC action,stop49,Runtime1s/payment and protection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
