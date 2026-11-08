#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Savage World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<8;++mode){warrior_test::Fixture f(390,false);auto& w=f.world;
 const bool spell=mode!=1&&mode!=4&&mode!=7;
 if(mode!=4)check(w.cast_activity(f.target,spell?281:1,100,mode==7?1000:3000));
 if(mode==2)check(w.status_mutation(f.target,16,123,12,100,100+timing::stance(16,12)->duration,true));
 if(mode==3)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
 if(mode==5){check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));w.configure_attack_block_draws([]{return 0.;});}
 if(mode==6){check(w.cast_activity(f.target,281,0,0));
check(w.attack_activity(f.target,3000,100));}
 const auto epoch=w.control_epoch(f.target);
 check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(390,12),1000));
 check(f.health()==(mode==3||mode==5?1000:spell&&mode!=6?948:980));
 check((w.control_epoch(f.target)!=epoch)==(mode==0||mode==1||mode==6));
 if(mode==2)check(w.casting_skill(f.target,1000)==281);
 }
 for(unsigned mode=0;mode<5;++mode){warrior_test::Fixture f(390,true);auto& w=f.world;
 const auto kind=mode==2?combat::npcs::control::Action::attack:combat::npcs::control::Action::spell;
 std::optional<Slot> slot;if(mode!=2&&mode!=4)slot=Slot{static_cast<std::uint16_t>(mode==1?1:281),0};
 const auto token=w.begin_npc_action(f.target,kind,100,mode==3?1000:3000,slot);
check(token.has_value());
 f.box->drain();
 check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(390,12),1000)==(mode!=4));
 const auto wire=f.box->drain();
 const auto stopped=mode==2?combat::weapon::stop_2022(f.target.actor):casting::terminal(59,f.target.actor);
 check(std::count(wire.begin(),wire.end(),stopped)==unsigned(mode<3));
 check(f.health()==(mode==4?1000:mode==0?948:980));
 check(w.complete_npc_action(*token,3000)==(mode==3||mode==4));
 }
 for(unsigned mode=0;mode<3;++mode){warrior_test::Fixture f(390,false);
check(f.world.cast_activity(f.target,281,100,3000));unsigned commits=0;casting::Hooks h;h.events=[](const auto&){};h.resources=[&](unsigned id){return f.world.resources(id);};h.consume_energy_at=[&](unsigned n,auto at){check(n==5);return f.world.consume_energy(1,n,f.source,at);};h.target_scope=[&](auto id){return f.world.effect_scope(id);};h.weapon_snapshot=[&](){return std::optional{f.weapon};};h.melee_target=[&](auto id,auto at){return f.world.melee_target(f.source,id,at,390);};h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto at){++commits;return f.world.melee_attack(f.source,target,weapon,plan,at);};h.attack_activity=[&](auto until){f.world.attack_activity(f.source,until,1000);};h.attack_interrupt_epoch=[&](){return f.world.attack_interrupt_epoch(f.source);};h.weapon_attack_timing=[&](auto at){return f.world.weapon_attack_timing(f.source,at);};Policy p;p.primary=1;p.campaigns=255;p.learned[390]=1;Slots bar{};bar[0]={390,0};attributes::Ranks ranks{};ranks[20]=12;casting::Runtime r(1);
check(r.start(1000,{390,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);if(mode==1)r.cancel(h);if(mode==2)r.interrupt(1200,h);
r.update(1499,h);
check(!commits);
r.update(1500,h);
check(commits==unsigned(mode==0)&&f.health()==(mode==0?948:1000));}
 std::cout<<"PASS Savage spell/non-spell/idle/expired contact,protected bonus,miss/block suppression,player/NPC interruption,unknown NPC rejection and Runtime cancel/interrupt\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
