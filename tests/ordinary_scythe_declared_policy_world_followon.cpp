// NEW unregistered declared-policy fixture. Numeric geometry is authored TEST
// policy, never claimed retail Scythe reach/arc/tie selection or normal enable.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("declared ordinary Scythe World line "+std::to_string(at.line()));}
struct Fixture{
 world::Instance w;Scope actor,hexer,primary;std::vector<Scope> foes;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,35,0.};
 static world::Instance::OrdinaryScythePolicyFollowon policy(){using P=world::Instance::OrdinaryScythePolicyFollowon;return {200,1.5707963267948966,P::Origin::wielder,P::SecondaryOrder::actor_id,P::ReactionOrder::secondary_first_provisional,"authored fixture ONLY; empirical geometry unresolved"};}
 Fixture(unsigned count=3,unsigned hp=500,bool mixed=false,bool enroll=true,bool npc_armor_known=true){
  if(mixed){world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.x=90;n.y=-30;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding={146,100,100,0,0,0,0,20};d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={1000,1000};d.damageable=d.conditions_allowed=true;if(npc_armor_known){d.physical_armor=60;d.physical_armor_evidence="authored target armor";}d.fire_armor=60;d.fire_armor_evidence="authored target fire armor";d.healing_allowed=d.fleshy=true;d.healing_evidence=d.fleshy_evidence="authored living target";n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Scythe owner",0,0,box,0,{}, {},100,1);w.set_vitals(1,{hp,1000,100,100},100);actor=*w.effect_scope(1);
  w.join(3,"Hex caster",-1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);hexer=*w.effect_scope(3);
  for(unsigned index=0;index<count;++index){const unsigned id=index==0?2:index==1?4:5;
   if(mixed&&index==2){foes.push_back(*w.effect_scope(100));continue;}
   const float x=index==0?80:index==1?70:90,y=index==0?0:index==1?30:-30;
   w.join(id,"Contact foe",x,y,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(id,{1000,1000,100,100},100);const auto scope=*w.effect_scope(id);foes.push_back(scope);
   check(w.physical_armor(scope,60)&&w.elemental_armor(scope,60,60)&&w.lightning_armor(scope,60));check(w.ordinary_weapon(scope,std::nullopt,20,100,combat::weapon::CriticalProfile{3,0,20}));
  }
  primary=foes.front();check(w.ordinary_weapon(actor,weapon,20,100,combat::weapon::CriticalProfile{1,0,20}));
  w.configure_attack_miss_draws([]{return .5;});w.configure_attack_critical_draws([]{return .999999;});
  if(enroll)check(w.configure_ordinary_scythe_policy_followon(actor,policy(),100));box->drain();
 }
 void hex(unsigned skill=26){check(w.empathy(hexer,actor,skill,0,20,100));}
 void vigor(){check(w.complete_vigorous_spirit(actor,actor,*vigorous_spirit(12),0,100));}
 void preparation(unsigned skill){const auto p=combat::weapon::preparation_contact_definition(skill,12);check(bool(p));check(w.status_mutation(actor,skill,123,12,100,100+p->duration,true));}
 void swing(){check(w.ordinary_attack(actor,primary.actor,1000));w.tick(1800);w.cancel_ordinary_attack(actor);}
 double health(Scope scope){return scope.actor==100?w.npc_combat_snapshot(scope)->health->current:double(w.resources(scope.actor)->health);}
 std::vector<combat::weapon::ResolvedAttack> outcomes(){return w.take_attack_outcomes(actor);}
};
int main()try{
 for(unsigned count:{1u,2u,3u})for(unsigned skill:{26u,3151u})for(bool vigor:{false,true}){
  Fixture f(count);f.hex(skill);if(vigor)f.vigor();f.swing();
  check(f.w.resources(1)->health==500-count*(skill==26?10u:15u)+(vigor?vigorous_spirit(12)->healing:0u));
  const auto outcomes=f.outcomes();check(outcomes.size()==count);for(const auto& event:outcomes){check(event.outcome==combat::weapon::AttackOutcome::hit&&event.target!=f.actor);}
  for(const auto target:f.foes)check(f.health(target)<1000);
  const auto hp=f.w.resources(1)->health;f.w.tick(1800);f.w.tick(1801);check(f.outcomes().empty()&&f.w.resources(1)->health==hp);
 }
 // Explicit geometry is mandatory: no default invalid policy is accepted and
 // existing unconfigured ordinary Scythe remains primary-only, not full support.
 {Fixture f(3,500,false,false);check(!f.w.configure_ordinary_scythe_policy_followon(f.actor,{},100));f.hex();f.swing();check(f.outcomes().size()==1&&f.w.resources(1)->health==490);check(f.health(f.foes[1])==1000&&f.health(f.foes[2])==1000);}
 // Host-authored tie policy changes selection without touching hit mechanics;
 // a closer foe behind the stored facing is excluded by the declared arc.
 {Fixture f;for(unsigned id:{6u,7u,8u}){const float x=id==6?-10:id==7?20:50;f.w.join(id,"Policy candidate",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(id,{1000,1000,100,100},100);const auto target=*f.w.effect_scope(id);check(f.w.physical_armor(target,60));check(f.w.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{3,0,20}));}
  auto policy=Fixture::policy();policy.secondary_order=world::Instance::OrdinaryScythePolicyFollowon::SecondaryOrder::nearest_wielder;check(f.w.configure_ordinary_scythe_policy_followon(f.actor,policy,100));f.hex();f.swing();const auto out=f.outcomes();check(out.size()==3);for(const auto& event:out)check(event.target.actor==2||event.target.actor==7||event.target.actor==8);check(f.w.resources(6)->health==1000&&f.health(f.foes[1])==1000&&f.health(f.foes[2])==1000);}
 // Source-only observers have action grain even across all three contacts.
 {Fixture f;f.hex();f.vigor();check(f.w.spiteful_spirit(f.hexer,f.actor,*spiteful_spirit(0),20,100));f.swing();check(f.w.resources(1)->health==500-30+vigorous_spirit(12)->healing-spiteful_spirit(0)->damage);}
 {Fixture f,baseline;const auto ward=*weapon_spell(2893,12,1,0);for(auto* value:{&f,&baseline}){value->hex();check(value->w.apply_weapon_spell(value->actor,value->actor,ward,100));}
  baseline.w.tick(1800);const auto before=baseline.w.resources(1)->health;f.swing();check(f.w.resources(1)->health==before-30);const auto status=f.w.status_snapshot(f.actor,1800);check(status&&std::none_of(status->begin(),status->end(),[](const auto& e){return e.skill==2893;}));const auto held=baseline.w.status_snapshot(baseline.actor,1800);check(held&&std::any_of(held->begin(),held->end(),[](const auto& e){return e.skill==2893;}));}
 // Standing continuous turn uses the SAME heading projection as public World
 // heading(), not the stale stored heading from the last rotation packet.
 {Fixture f;f.hex();check(f.w.ordinary_attack(f.actor,f.primary.actor,1000));f.w.rotate(f.actor.actor,std::numeric_limits<float>::infinity(),1,1100);const auto angle=f.w.heading(f.actor.actor,1800);check(angle>1.4&&angle<1.5);f.w.tick(1800);f.w.cancel_ordinary_attack(f.actor);const auto out=f.outcomes();check(out.size()==2&&f.w.resources(1)->health==480);for(const auto& event:out)check(event.target==f.primary||event.target==f.foes[1]);check(f.health(f.foes[2])==1000);}
 // The first real contact's Splinter cohort kills the other selected foes.
 // Their reserved entries retire without two phantom Empathy reactions.
 {Fixture f;f.hex();f.vigor();for(const auto target:{f.primary,f.foes[2]})f.w.set_vitals(target.actor,{1,1000,100,100},100);
  const auto splinter=weapon_spell(792,12,1,0);check(bool(splinter));check(f.w.apply_weapon_spell(f.actor,f.actor,*splinter,100));f.swing();const auto out=f.outcomes();check(out.size()==3&&f.w.resources(1)->health==500-10+vigorous_spirit(12)->healing);
  check(f.health(f.primary)==0&&f.health(f.foes[2])==0&&f.health(f.foes[1])<1000);for(const auto& event:out)check(event.outcome==(event.target==f.foes[1]?combat::weapon::AttackOutcome::hit:combat::weapon::AttackOutcome::target_retired));}
 // One miss, one block and one hit resolve independently. All three actual
 // contacts trigger Empathy; Vigor remains one attack. Poison follows only hit.
 {Fixture f;f.hex();f.vigor();f.preparation(435);check(f.w.apply_condition(f.hexer,f.actor,combat::conditions::Kind::blind,100,5000));
  check(f.w.complete_shield_deflection(f.foes[1],f.foes[1],*shield_deflection(12),0,100));
  unsigned miss_draws=0;f.w.configure_attack_miss_draws([&]{return miss_draws++==1?0.:.999999;});f.w.configure_attack_block_draws([]{return 0.;});f.swing();
  const auto out=f.outcomes();check(out.size()==3&&miss_draws==3);for(const auto& event:out)check(event.outcome==(event.target==f.foes[1]?combat::weapon::AttackOutcome::block:event.target==f.foes[2]?combat::weapon::AttackOutcome::miss:combat::weapon::AttackOutcome::hit));
  check(f.w.resources(1)->health==500-30+vigorous_spirit(12)->healing);check(f.w.has_condition(f.primary,combat::conditions::Kind::poison,1800));check(!f.w.has_condition(f.foes[1],combat::conditions::Kind::poison,1800)&&!f.w.has_condition(f.foes[2],combat::conditions::Kind::poison,1800));
 }
 // IW still has three actual contact observers, no physical-hit qualification,
 // no accuracy/block draws or preparation condition, and one Vigor action.
 {Fixture f;f.hex();f.vigor();f.preparation(435);skills::Slots bar{};bar[0]={33,0};check(f.w.configure_adrenaline(f.actor,bar,100));check(f.w.status_mutation(f.actor,33,124,12,100,30100,true));
  check(f.w.apply_condition(f.hexer,f.actor,combat::conditions::Kind::blind,100,5000));unsigned draws=0;f.w.configure_attack_miss_draws([&]{++draws;return 0.;});f.w.configure_attack_block_draws([&]{++draws;return 0.;});f.swing();
  const auto out=f.outcomes();check(out.size()==3&&draws==0);for(const auto& event:out)check(event.outcome==combat::weapon::AttackOutcome::replaced&&!event.before_damage);
  check(f.w.resources(1)->health==500-30+vigorous_spirit(12)->healing);for(const auto target:f.foes)check(!f.w.has_condition(target,combat::conditions::Kind::poison,1800));
 }
 // Fatal SECOND per-foe reaction preserves the first committed contact and
 // cancels current/later recipients; source is never revived by a later hit.
 {Fixture f(3,15);f.hex();f.swing();const auto out=f.outcomes();check(out.size()==3&&f.w.resources(1)->health==0);
  check(f.health(f.foes[1])<1000&&f.health(f.foes[2])==1000&&f.health(f.primary)==1000);
  for(const auto& event:out)check(event.outcome==(event.target==f.foes[1]?combat::weapon::AttackOutcome::hit:combat::weapon::AttackOutcome::cancelled));
 }
 // Required contact armor authority is checked for the ENTIRE selected set
 // before canonical attack-made/Empathy/Vigor, not guessed for an NPC.
 {Fixture f(3,500,true,true,false);f.hex();f.vigor();f.swing();check(f.outcomes().empty()&&f.w.resources(1)->health==500);for(const auto target:f.foes)check(f.health(target)==1000);}
 // Reuse actual NPC ordinary contact authority alongside player recipients.
 {Fixture f(3,500,true);f.hex();f.swing();check(f.outcomes().size()==3&&f.w.resources(1)->health==470);for(const auto target:f.foes)check(f.health(target)<1000);}
 // Shared Conjure and receiver Protection apply independently per recipient.
 {Fixture raw,conjured;for(auto* f:{&raw,&conjured}){auto fire=f->weapon;++fire.revision;fire.damage.raw_damage_type=5;check(f->w.ordinary_weapon(f->actor,fire,20,100,combat::weapon::CriticalProfile{1,0,20}));f->weapon=fire;check(f->w.configure_ordinary_scythe_policy_followon(f->actor,Fixture::policy(),100));}
  check(conjured.w.conjure_enchantment(conjured.actor,*conjure(182,12),100));raw.swing();conjured.swing();for(unsigned i=0;i<3;++i)check(conjured.health(conjured.foes[i])==raw.health(raw.foes[i])-conjure(182,12)->damage);
 }
 {Fixture f;f.w.set_vitals(f.primary.actor,{100,100,100,100},100);check(f.w.complete_protective_spirit(f.primary,f.primary,*protective_spirit(12),0,100));f.hex();f.swing();check(f.health(f.primary)==90&&f.health(f.foes[1])<1000&&f.health(f.foes[2])<1000);}
 // Stored replay damage is secondary-before-primary; this declared producer
 // uses that publication order, while internal reaction-order proof is pending.
 {Fixture f;f.swing();std::vector<unsigned> published;for(const auto& packet:f.box->drain())if(packet.size()>=18&&packet[0]==162&&packet[1]==0){const auto word=[&](unsigned at){return unsigned(packet[at])|(unsigned(packet[at+1])<<8)|(unsigned(packet[at+2])<<16)|(unsigned(packet[at+3])<<24);};if((word(2)==16||word(2)==17)&&word(10)==f.actor.actor&&word(6)!=f.actor.actor&&std::bit_cast<float>(word(14))<0)published.push_back(word(6));}check(published==std::vector<unsigned>({4,5,2}));}
 // Owned ordinary windup cancellation/control/equipment and generation remain
 // with the existing state machine; no independent attempt ledger is created.
 {Fixture f;f.hex();check(f.w.ordinary_attack(f.actor,f.primary.actor,1000));f.w.cancel_ordinary_attack(f.actor);f.w.tick(1800);check(f.outcomes().empty()&&f.w.resources(1)->health==500);}
 {Fixture f;f.hex();check(f.w.ordinary_attack(f.actor,f.primary.actor,1000));check(f.w.gale(f.hexer,f.actor,12,.5,1100));f.w.tick(1800);check(f.outcomes().empty()&&f.w.resources(1)->health==500);}
 {Fixture f;f.hex();check(f.w.ordinary_attack(f.actor,f.primary.actor,1000));auto changed=f.weapon;++changed.revision;check(f.w.ordinary_weapon(f.actor,changed,20,1100,combat::weapon::CriticalProfile{1,0,20}));f.w.tick(1800);check(f.outcomes().empty()&&f.w.resources(1)->health==500);}
 {Fixture f;f.hex();check(f.w.ordinary_attack(f.actor,f.primary.actor,1000));const auto stale=f.primary;f.w.leave(stale.actor);f.w.join(stale.actor,"Reused primary",80,0,std::make_shared<world::Outbox>(),0,{}, {},1100,2);f.w.set_vitals(stale.actor,{1000,1000,100,100},100);const auto replacement=*f.w.effect_scope(stale.actor);check(replacement!=stale);check(f.w.physical_armor(replacement,60));check(f.w.ordinary_weapon(replacement,std::nullopt,20,1100,combat::weapon::CriticalProfile{3,0,20}));f.w.tick(1800);check(f.outcomes().empty()&&f.w.resources(1)->health==500&&f.w.resources(stale.actor)->health==1000);}
 {Fixture f;const auto stale=f.actor;f.w.leave(1);f.w.join(1,"Reused owner",0,0,f.box,0,{}, {},1100,1);f.w.set_vitals(1,{500,1000,100,100},100);f.actor=*f.w.effect_scope(1);check(f.actor!=stale);check(f.w.ordinary_weapon(f.actor,f.weapon,20,1100,combat::weapon::CriticalProfile{1,0,20}));check(!f.w.configure_ordinary_scythe_policy_followon(stale,Fixture::policy(),1100));check(!f.w.ordinary_attack(stale,f.primary.actor,1200));check(f.w.ordinary_attack(f.actor,f.primary.actor,1200));f.w.tick(2000);f.w.cancel_ordinary_attack(f.actor);check(f.outcomes().size()==1);}
 std::cout<<"PASS DECLARED-policy ordinary Scythe canonical1/2/3 contacts, miss/block/IW/preparation/Conjure/Protection, once action observers, fatal ordering/control/generation/replay; retail geometry pending\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
