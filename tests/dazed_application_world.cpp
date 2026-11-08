#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Dazed application line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<6;++mode){warrior_test::Fixture f(383,false);auto& w=f.world;
 if(mode!=2)check(w.cast_activity(f.target,mode==1?1:281,100,3000));
 if(mode==5)check(w.status_mutation(f.target,17,123,12,100,100+timing::stance(17,12)->duration,true));
 if(mode==3)check(w.status_mutation(f.target,16,123,12,100,100+timing::stance(16,12)->duration,true));
 if(mode==4){check(w.apply_condition(f.source,f.target,combat::conditions::Kind::dazed,100,500));check(w.cast_activity(f.target,281,700,3000));}
 const auto epoch=w.control_epoch(f.target);
 check(w.apply_condition(f.source,f.target,combat::conditions::Kind::dazed,1000,5000));
 check(w.has_condition(f.target,combat::conditions::Kind::dazed,1000));
 check((w.control_epoch(f.target)!=epoch)==(mode==0||mode==4));
 if(mode==3){check(w.casting_skill(f.target,1000)==281);const auto entries=*w.status_snapshot(f.target,1000);check(std::none_of(entries.begin(),entries.end(),[](auto e){return e.skill==16;}));}
 if(mode==5)check(w.casting_skill(f.target,1000)==281&&w.resources(2)->energy==25);
 check(w.cast_activity(f.target,281,1100,3000));const auto after=w.control_epoch(f.target);
 check(w.apply_condition(f.source,f.target,combat::conditions::Kind::dazed,1200,1000));
 // Legacy renewal behavior only: retail generic reapplication interruption is unverified.
 check(w.control_epoch(f.target)==after&&w.casting_skill(f.target,1200)==281);
 check(w.has_condition(f.target,combat::conditions::Kind::dazed,5999)&&!w.has_condition(f.target,combat::conditions::Kind::dazed,6000));
 }
 // Player condition transfer and area spread use the same retained-Dazed control rule.
 for(bool spread:{false,true})for(unsigned mode=0;mode<3;++mode){
 warrior_test::Fixture f(383,false);auto& w=f.world;auto victim=f.target;
 if(spread){
 w.join(3,"Recipient",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
 w.set_vitals(3,{1000,1000,30,30},100);victim=*w.effect_scope(3);
 check(w.apply_condition(f.source,f.target,combat::conditions::Kind::dazed,200,5000));
 }else check(w.apply_condition(f.source,f.source,combat::conditions::Kind::dazed,200,5000));
 check(w.cast_activity(victim,Slot{mode==1?1u:281u,7},300,3000));
 if(mode==2)check(w.status_mutation(victim,16,123,12,300,300+timing::stance(16,12)->duration,true));
 const auto epoch=w.control_epoch(victim);
 if(spread)check(w.condition_spell(f.source,f.target,*effects::condition_spell(78),1000));
 else check(w.condition_transfer(f.source,f.target,*effects::condition_transfer(154),1000));
 check(w.has_condition(victim,combat::conditions::Kind::dazed,1000));
 check((w.control_epoch(victim)!=epoch)==(mode==0));
 check(bool(w.casting_slot(victim,1000))==(mode!=0));
 if(!spread)check(!w.has_condition(f.source,combat::conditions::Kind::dazed,1000));
 }
 // Fevered's initial Dazed, later generated Dazed and propagation to a
 // separate player all interrupt spells; PvP does not generate Dazed.
 for(bool pvp:{false,true})for(bool initial:{false,true})for(unsigned mode=0;mode<3;++mode){
 warrior_test::Fixture f(383,false);auto& w=f.world;
 w.join(3,"Nearby",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
 w.set_vitals(3,{1000,1000,30,30},100);const auto nearby=*w.effect_scope(3);
 if(initial)check(w.apply_condition(f.source,f.target,combat::conditions::Kind::blind,200,5000));
 else{check(w.apply_fevered_dreams(f.source,f.target,12,pvp,200,effects::FeveredDazedSpan::attempted_duration));
 check(w.apply_condition(f.source,f.target,combat::conditions::Kind::blind,250,5000));}
 for(auto target:{f.target,nearby}){
 check(w.cast_activity(target,Slot{mode==1?1u:281u,9},300,3000));
 if(mode==2)check(w.status_mutation(target,16,123,12,300,300+timing::stance(16,12)->duration,true));
 }
 const auto a=w.control_epoch(f.target),b=w.control_epoch(nearby);
 if(initial)check(w.apply_fevered_dreams(f.source,f.target,12,pvp,500,effects::FeveredDazedSpan::attempted_duration));
 else check(w.apply_condition(f.source,f.target,combat::conditions::Kind::poison,500,5000));
 check(w.process_fevered_condition_events(500));
 check(w.has_condition(f.target,combat::conditions::Kind::dazed,500)==!pvp);
 check(w.has_condition(nearby,combat::conditions::Kind::dazed,500)==!pvp);
 check((w.control_epoch(f.target)!=a)==(!pvp&&mode==0));
 check((w.control_epoch(nearby)!=b)==(!pvp&&mode==0));
 for(auto target:{f.target,nearby})check(bool(w.casting_slot(target,500))==(pvp||mode!=0));
 }
 // Transferring Dazed to an identified NPC spell cancels its authoritative
 // token and publishes spell interruption; unknown identity rejects atomically.
 for(unsigned mode=0;mode<6;++mode){warrior_test::Fixture f(383,true);auto& w=f.world;
 std::optional<Slot> slot;if(mode!=2&&mode!=4)slot=Slot{mode==1?1u:281u,11};
 const auto token=w.begin_npc_action(f.target,mode==2?combat::npcs::control::Action::attack:combat::npcs::control::Action::spell,100,mode==3?900:3000,slot);check(token.has_value());
 check(w.apply_condition(f.source,f.source,combat::conditions::Kind::dazed,200,5000));
 if(mode==5){auto stale=f.target;++stale.generation;check(!w.condition_transfer(f.source,stale,*effects::condition_transfer(154),1000));continue;}
 f.box->drain();const auto transferred=w.condition_transfer(f.source,f.target,*effects::condition_transfer(154),1000);
 check(transferred==(mode!=4));
 check(w.has_condition(f.source,combat::conditions::Kind::dazed,1000)==(mode==4));
 check(w.has_condition(f.target,combat::conditions::Kind::dazed,1000)==(mode!=4));
 check(w.complete_npc_action(*token,3000)==(mode!=0));
 const auto packets=f.box->drain();const auto stop=casting::terminal(59,f.target.actor);
 check(std::count(packets.begin(),packets.end(),stop)==unsigned(mode==0));
 }
 // NPC Fevered initial and condition-spell-triggered Dazed use staged
 // cancellation; an unknown active skill must leave the proposal uncommitted.
 for(bool pvp:{false,true})for(bool initial:{false,true})for(unsigned mode=0;mode<4;++mode){
 warrior_test::Fixture f(383,true);auto& w=f.world;
 if(!initial)check(w.apply_fevered_dreams(f.source,f.target,12,pvp,150,effects::FeveredDazedSpan::attempted_duration));
 check(w.condition_spell(f.source,f.target,*effects::condition_spell(117),200));
 std::optional<Slot> slot;if(mode!=3)slot=Slot{mode==1?1u:281u,12};
 const auto token=w.begin_npc_action(f.target,mode==2?combat::npcs::control::Action::attack:combat::npcs::control::Action::spell,300,3000,mode==2?std::nullopt:slot);check(token.has_value());
 f.box->drain();
 const bool accepted=initial?w.apply_fevered_dreams(f.source,f.target,12,pvp,500,effects::FeveredDazedSpan::attempted_duration):w.condition_spell(f.source,f.target,*effects::condition_spell(131),500);
 check(accepted==(pvp||mode!=3));
 check(w.has_condition(f.target,combat::conditions::Kind::dazed,500)==(!pvp&&mode!=3));
 check(w.complete_npc_action(*token,3000)==(pvp||mode!=0));
 const auto wire=f.box->drain();check(std::count(wire.begin(),wire.end(),casting::terminal(59,f.target.actor))==unsigned(!pvp&&mode==0));
 if(!accepted&&!initial)check(!w.has_condition(f.target,combat::conditions::Kind::bleeding,500));
 }
 // Player-origin Fevered propagation into an NPC is staged independently
 // of the source event. Unknown skill identity reports a boundary failure.
 for(bool pvp:{false,true})for(bool initial:{false,true})for(unsigned mode=0;mode<4;++mode){
 warrior_test::Fixture f(383,true);auto& w=f.world;
 w.join(2,"Fevered source",40,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
 w.set_vitals(2,{1000,1000,30,30},100);const auto marked=*w.effect_scope(2);
 if(!initial)check(w.apply_fevered_dreams(f.source,marked,12,pvp,150,effects::FeveredDazedSpan::attempted_duration));
 check(w.apply_condition(f.source,marked,combat::conditions::Kind::blind,200,5000));
 std::optional<Slot> slot;if(mode!=3&&mode!=2)slot=Slot{mode==1?1u:281u,13};
 const auto token=w.begin_npc_action(f.target,mode==2?combat::npcs::control::Action::attack:combat::npcs::control::Action::spell,300,3000,slot);check(token.has_value());
 f.box->drain();
 if(initial)check(w.apply_fevered_dreams(f.source,marked,12,pvp,500,effects::FeveredDazedSpan::attempted_duration));
 else check(w.apply_condition(f.source,marked,combat::conditions::Kind::poison,500,5000));
 check(w.has_condition(f.target,combat::conditions::Kind::dazed,500)==(!pvp&&mode!=3));
 check(w.complete_npc_action(*token,3000)==(pvp||mode!=0));
 check(w.fevered_boundary_failures()==unsigned(!pvp&&mode==3));
 const auto wire=f.box->drain();check(std::count(wire.begin(),wire.end(),casting::terminal(59,f.target.actor))==unsigned(!pvp&&mode==0));
 }
 // Landed melee/bow attacks on Dazed players interrupt spells. Overlap
 // with an interrupt attack must consume Mantra protection only once.
 for(unsigned skill:{320u,403u,390u,409u})for(unsigned mode=0;mode<7;++mode){
 warrior_test::Fixture f(skill,false);auto& w=f.world;const bool direct=skill==390||skill==409;
 check(w.apply_condition(f.source,f.target,combat::conditions::Kind::dazed,100,mode==4?500:5000));
 check(w.cast_activity(f.target,Slot{mode==1?1u:281u,14},200,mode==6?900:3000));
 if(mode==2)check(w.status_mutation(f.target,17,123,12,200,200+timing::stance(17,12)->duration,true));
 if(mode==5)check(w.status_mutation(f.target,16,123,12,200,200+timing::stance(16,12)->duration,true));
 if(mode==3)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,200,5000));
 const auto epoch=w.control_epoch(f.target);
 check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(skill,12),1000));w.tick(1100);
 const bool interrupts=mode!=2&&mode!=3&&mode!=5&&mode!=6&&(direct||(mode!=1&&mode!=4));
 check((w.control_epoch(f.target)!=epoch)==interrupts);
 if(mode==2)check(w.resources(2)->energy==25&&w.casting_skill(f.target,1100)==281);
 if(mode==5)check(w.casting_skill(f.target,1100)==281);
 }
 for(unsigned skill:{320u,403u,390u,409u})for(unsigned mode=0;mode<4;++mode){
 warrior_test::Fixture f(skill,true);auto& w=f.world;const bool direct=skill==390||skill==409;
 check(f.condition(combat::conditions::Kind::dazed,100,5000));
 const auto token=w.begin_npc_action(f.target,combat::npcs::control::Action::spell,200,3000,Slot{mode==1?1u:281u,15});check(token.has_value());
 if(mode==2)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,200,5000));
 f.box->drain();check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(skill,12),1000));w.tick(1100);
 const bool interrupts=mode!=2&&(direct||mode!=1);
 check(w.complete_npc_action(*token,3000)==!interrupts);
 const auto wire=f.box->drain();check(std::count(wire.begin(),wire.end(),casting::terminal(59,f.target.actor))==unsigned(interrupts));
 }
 // One Cyclone hits multiple distinct Dazed recipients, with independent
 // protection and no interrupt on miss/block. The NPC shares the same area hit.
 for(unsigned mode=0;mode<4;++mode){warrior_test::Fixture f(330,true);auto& w=f.world;
 check(f.condition(combat::conditions::Kind::dazed,100,5000));
 const auto token=w.begin_npc_action(f.target,combat::npcs::control::Action::spell,200,3000,Slot{281,16});check(token.has_value());
 for(unsigned id:{2u,3u}){w.join(id,"Foe",float(40+id*10),0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(id,{1000,1000,30,30},100);
 const auto target=*w.effect_scope(id);check(w.physical_armor(target,60));
 check(w.apply_condition(f.source,target,combat::conditions::Kind::dazed,200,5000));check(w.cast_activity(target,Slot{281,16},300,3000));}
 const auto a=*w.effect_scope(2),b=*w.effect_scope(3);const auto ea=w.control_epoch(a),eb=w.control_epoch(b);
 check(w.status_mutation(b,17,123,12,300,300+timing::stance(17,12)->duration,true));
 if(mode==1)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,300,5000));
 if(mode==2){check(w.apply_health_regeneration(a,2,*regeneration::reviewed(258,12),300,6300,1248,true,a));w.configure_attack_block_draws([]{return 0.;});}
 if(mode==3)w.set_vitals(2,{1,1000,30,30},100);
 f.box->drain();check(w.melee_attack(f.source,f.source,f.weapon,*combat::weapon::attack_skill(330,0),1000));
 check((w.control_epoch(a)!=ea)==(mode==0||mode==3)); // lethal mode changes epoch through death cleanup
 check(w.control_epoch(b)==eb&&w.casting_skill(b,1000)==281);
 check(w.resources(3)->energy==(mode==1?30u:25u));
 check(w.complete_npc_action(*token,3000)==(mode==1));
 const auto wire=f.box->drain();check(std::count(wire.begin(),wire.end(),casting::terminal(59,f.target.actor))==unsigned(mode!=1));
 }
 // Ordinary weapon attacks exercise the non-skill scheduler and bow
 // projectile contact, not merely attacks dispatched through melee_attack.
 for(bool bow:{false,true})for(unsigned mode=0;mode<7;++mode){
 warrior_test::Fixture f(bow?403:320,false);auto& w=f.world;
 check(w.apply_condition(f.source,f.target,combat::conditions::Kind::dazed,100,mode==5?1500:5000));
 if(mode!=6)check(w.cast_activity(f.target,Slot{mode==1?1u:281u,18},200,5000));
 if(mode==2)check(w.status_mutation(f.target,17,123,12,200,200+timing::stance(17,12)->duration,true));
 if(mode==3)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,200,5000));
 if(mode==4){check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),200,6200,1248,true,f.target));w.configure_attack_block_draws([]{return 0.;});}
 const auto epoch=w.control_epoch(f.target);check(w.ordinary_attack(f.source,f.target.actor,1000));w.tick(2000);
 if(mode==6)check(w.cast_activity(f.target,Slot{281,18},2001,5000));
 w.tick(2200);
 check((w.control_epoch(f.target)!=epoch)==(mode==0||(mode==6&&bow)));
 if(mode==2)check(w.resources(2)->energy==25&&w.casting_skill(f.target,2200)==281);
 }
 for(bool bow:{false,true})for(unsigned mode=0;mode<3;++mode){
 warrior_test::Fixture f(bow?403:320,true);auto& w=f.world;check(f.condition(combat::conditions::Kind::dazed,100,5000));
 const auto token=w.begin_npc_action(f.target,combat::npcs::control::Action::spell,200,5000,Slot{mode==1?1u:281u,19});check(token.has_value());
 if(mode==2)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,200,5000));
 f.box->drain();check(w.ordinary_attack(f.source,f.target.actor,1000));w.tick(2000);w.tick(2200);
 check(w.complete_npc_action(*token,5000)==(mode!=0));
 const auto wire=f.box->drain();check(std::count(wire.begin(),wire.end(),casting::terminal(59,f.target.actor))==unsigned(mode==0));
 }
 // Copied spell identity must survive protected Dazed, then a genuinely
 // fresh application at the exact expiry boundary must cancel that copy.
 {warrior_test::Fixture f(383,false);auto& w=f.world;
 check(w.cast_activity(f.target,Slot{281,65535},100,3000));
 check(w.status_mutation(f.target,16,123,12,100,100+timing::stance(16,12)->duration,true));
 check(w.apply_condition(f.source,f.target,combat::conditions::Kind::dazed,1000,1000));
 check(w.casting_slot(f.target,1000)==Slot{281,65535});
 const auto epoch=w.control_epoch(f.target);
 check(w.apply_condition(f.source,f.target,combat::conditions::Kind::dazed,2000,1000));
 check(!w.casting_slot(f.target,2000)&&w.control_epoch(f.target)!=epoch);
 }
 {warrior_test::Fixture f(383,false);auto stale=f.target;++stale.generation;check(!f.world.apply_condition(f.source,stale,combat::conditions::Kind::dazed,1000,5000));check(!f.world.has_condition(f.target,combat::conditions::Kind::dazed,1000));}
 std::cout<<"PASS authoritative new Dazed application interrupts spells,not signets/idle;protection,copied-slot/expiry boundary,legacy renewal and scope rejection;player transfer/spread verified;Fevered initial/generated/propagated verified;NPC transfer/initial/generated/propagated verified;other producers/hit paths pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

