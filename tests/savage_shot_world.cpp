#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Savage Shot line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=combat::weapon::attack_skill(426,rank);check(p&&p->bonus==13+rank&&p->required_type==5&&p->windup_ms==500);}
 for(unsigned mode=0;mode<8;++mode){warrior_test::Fixture f(426,false);auto& w=f.world;
 if(mode!=4&&mode!=6)check(w.cast_activity(f.target,mode==1?1:281,100,mode==7?1020:3000));
 if(mode==2)check(w.status_mutation(f.target,16,123,12,100,100+timing::stance(16,12)->duration,true));
 if(mode==3)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
 if(mode==5){check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));w.configure_attack_block_draws([]{return 0.;});}
 const auto epoch=w.control_epoch(f.target);check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(426,12),1000));
 if(mode==6)check(w.cast_activity(f.target,281,1010,3000));
 w.tick(1100);check(f.health()==(mode==3||mode==5?1000:mode==0||mode==2||mode==6?955:980));
 check((w.control_epoch(f.target)!=epoch)==(mode==0||mode==1||mode==6));if(mode==2)check(w.casting_skill(f.target,1100)==281);
 }
 for(unsigned mode=0;mode<5;++mode){warrior_test::Fixture f(426,true);auto& w=f.world;std::optional<Slot> slot;if(mode!=2&&mode!=4)slot=Slot{static_cast<std::uint16_t>(mode==1?1:281),0};const auto token=w.begin_npc_action(f.target,mode==2?combat::npcs::control::Action::attack:combat::npcs::control::Action::spell,100,mode==3?1020:3000,slot);check(token.has_value());check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(426,12),1000));f.box->drain();w.tick(1100);check(f.health()==(mode==4?1000:mode==0?955:980));check(w.complete_npc_action(*token,3000)==(mode==3||mode==4));const auto wire=f.box->drain();const auto stopped=mode==2?combat::weapon::stop_2022(f.target.actor):casting::terminal(59,f.target.actor);check(std::count(wire.begin(),wire.end(),stopped)==unsigned(mode<3));}
 for(unsigned mode=0;mode<3;++mode){warrior_test::Fixture f(426,false);check(f.world.cast_activity(f.target,281,100,3000));unsigned commits=0;casting::Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return f.world.resources(id);};h.consume_energy_at=[&](unsigned n,auto at){check(n==10);return f.world.consume_energy(1,n,f.source,at);};h.target_scope=[&](auto id){return f.world.effect_scope(id);};h.weapon_snapshot=[&](){return std::optional{f.weapon};};h.melee_target=[&](auto id,auto at){return f.world.melee_target(f.source,id,at,426);};h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto at){++commits;return f.world.melee_attack(f.source,target,weapon,plan,at);};h.attack_activity=[&](auto until){f.world.attack_activity(f.source,until,1000);};h.attack_interrupt_epoch=[&](){return f.world.attack_interrupt_epoch(f.source);};h.weapon_attack_timing=[&](auto at){return f.world.weapon_attack_timing(f.source,at);};Policy p;p.primary=2;p.campaigns=255;p.learned[426]=1;Slots bar{};bar[0]={426,0};attributes::Ranks ranks{};ranks[25]=12;casting::Runtime r(1);check(r.start(1000,{426,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);if(mode==1)r.cancel(h);if(mode==2)r.interrupt(1200,h);r.update(1499,h);check(!commits);r.update(1500,h);f.world.tick(1600);check(commits==unsigned(mode==0)&&f.health()==(mode==0?955:1000));}
 std::cout<<"PASS Savage Shot contact-time spell/non-spell/expired/new actions,protected bonus,miss/block,NPC cancellation/unknown identity and Runtime lifecycle\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
