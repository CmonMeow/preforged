#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Concussion World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=combat::weapon::attack_skill(408,rank);check(p&&p->fixed_damage==rank+1&&p->required_type==5&&p->windup_ms==500);}
 for(bool npc:{false,true})for(unsigned mode=0;mode<11;++mode){
 warrior_test::Fixture f(408,npc,mode==7?1:1000,true,mode!=3);auto& w=f.world;
 if(mode==4)check(f.condition(combat::conditions::Kind::dazed,100,10000));
 std::optional<combat::npcs::control::Token> token;
 if(mode!=2&&mode!=10){if(npc){token=w.begin_npc_action(f.target,combat::npcs::control::Action::spell,200,mode==9?1020:5000,Slot{mode==1?1u:281u,21});check(token.has_value());}
 else check(w.cast_activity(f.target,Slot{mode==1?1u:281u,21},200,mode==9?1020:5000));}
 if(mode==5)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,200,5000));
 if(mode==6&&!npc){check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),200,6200,1248,true,f.target));w.configure_attack_block_draws([]{return 0.;});}
 if(mode==8&&!npc)check(w.status_mutation(f.target,17,123,12,200,200+timing::stance(17,12)->duration,true));
 const auto epoch=npc?std::optional<effects::Time>{}:w.control_epoch(f.target);
 check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(408,12),1000));
 if(mode==10){if(npc){token=w.begin_npc_action(f.target,combat::npcs::control::Action::spell,1010,5000,Slot{281,21});check(token.has_value());}else check(w.cast_activity(f.target,Slot{281,21},1010,5000));}
 f.box->drain();w.tick(1100);
 const bool landed=mode!=5&&(npc||mode!=6);
 check(f.health()==(landed?(mode==7?0:987):1000));
 const bool fresh=landed&&mode!=1&&mode!=2&&mode!=4&&mode!=7&&mode!=9&&(!npc||mode!=3);
 check(w.has_condition(f.target,combat::conditions::Kind::dazed,1100)==(fresh||mode==4));
 if(mode==4)check(!w.has_condition(f.target,combat::conditions::Kind::dazed,10100));
 const bool interrupted=(fresh||mode==4)&&!(mode==8&&!npc);
 if(npc&&token){check(w.complete_npc_action(*token,5000)==(!interrupted&&mode!=7));
 const auto wire=f.box->drain();check(std::count(wire.begin(),wire.end(),casting::terminal(59,f.target.actor))==unsigned(interrupted));}
 if(!npc&&mode!=7){check((w.control_epoch(f.target)!=epoch)==interrupted);if(mode==8)check(w.resources(2)->energy==25&&w.casting_skill(f.target,1100)==281);}
 }
 // Fixed damage ignores armor/mastery/Weakness and elemental bow vulnerability.
 {combat::weapon::Strike strike;strike.location_armor=200;strike.mastery=20;strike.minimum=100;strike.maximum=200;strike.customized=true;strike.critical=true;strike.weakness=true;strike.base_roll=strike.miss_roll=.5;
 const auto hit=combat::weapon::resolve_attack_skill(strike,*combat::weapon::attack_skill(408,12),false);check(hit&&!hit->missed&&hit->damage==13);}
 {warrior_test::Fixture f(408,true,1000,true,true,2);auto fire=f.weapon;fire.damage.raw_damage_type=5;check(f.world.ordinary_weapon(f.source,fire,20));
 check(f.world.melee_attack(f.source,f.target,fire,*combat::weapon::attack_skill(408,12),1000));f.world.tick(1100);check(f.health()==987);}
 for(unsigned mode=0;mode<5;++mode){warrior_test::Fixture f(408,false);auto& w=f.world;check(w.cast_activity(f.target,281,200,5000));unsigned commits=0;bool stale=false;
 casting::Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.consume_energy_at=[&](unsigned cost,auto at){check(cost==25);return w.consume_energy(1,cost,f.source,at);};h.target_scope=[&](auto id){return stale?std::optional<effects::Scope>{}:w.effect_scope(id);};h.weapon_snapshot=[&](){return std::optional{f.weapon};};h.melee_target=[&](auto id,auto at){return w.melee_target(f.source,id,at,408);};h.melee_attack=[&](auto target,const auto& gear,const auto& plan,auto at){++commits;return w.melee_attack(f.source,target,gear,plan,at);};h.attack_activity=[&](auto until){w.attack_activity(f.source,until,1000);};h.attack_interrupt_epoch=[&](){return w.attack_interrupt_epoch(f.source);};h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(f.source,at);};
 Policy p;p.primary=2;p.campaigns=255;p.learned[408]=1;Slots bar{};bar[0]={408,0};attributes::Ranks ranks{};ranks[25]=12;casting::Runtime cast(1);
 check(cast.start(1000,{408,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);check(w.resources(1)->energy==5);
 if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1200,h);if(mode==3)stale=true;if(mode==4){auto gear=f.weapon;gear.item_type=27;++gear.revision;check(w.ordinary_weapon(f.source,gear,20));}
 cast.update(1499,h);check(!commits);cast.update(1500,h);w.tick(1600);
 check(f.health()==(mode==0?987:1000));check(w.has_condition(f.target,combat::conditions::Kind::dazed,1600)==(mode==0));
 }
 std::cout<<"PASS Concussion fixed damage,spell-only retained Dazed/nonrenewal,player/NPC protection/immunity/expiry/late cast,miss/block/death and Runtime lifecycle\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
