#include "../src/runtime/gw_world_simulation.h"
#include "../src/runtime/gw_presearing_items.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace world;
void check(bool v,std::source_location where=std::source_location::current()){if(!v)throw std::runtime_error("world weapon attack regression line"+std::to_string(where.line()));}
int main()try{
 // A reviewed duration-only suffix must not disable the actual World attack
 // route or change its damage. Gear is authored; this is not retail item proof.
 for(unsigned type:{2u,15u,27u,26u}){
  std::optional<double> baseline;
  for(unsigned percent:{0u,10u,20u}){
   auto gear=inventory::baseline();inventory::Item item;
   item.definition.id=1;item.definition.file=1;item.definition.type=static_cast<std::uint8_t>(type);item.definition.quantity=1;
   const unsigned attribute=type==2?18:type==15?19:type==27?20:9;
   item.definition.modifiers={0xa7a80f0f,0x24b80000u|((type==26?11u:type==15?0u:2u)<<8),0x27980009u|(attribute<<8)};
   if(percent)item.definition.modifiers.push_back(0x22b80000u|percent);
   item.bag=2;item.slot=0;item.profession=type==26?6:1;item.customized=u"";gear.items.emplace(1,item);gear.weapons.sets[0].weapon=1;
   combat::weapon::Policy policy;policy.primary=type==26?6:1;policy.character="Caster";policy.caster_weapon=type==26;policy.mastery_attribute=attribute;policy.effective[attribute]=12;
   const auto owned=combat::weapon::owned_weapon(gear,1,policy);check(owned.has_value());
   Instance world;auto box=std::make_shared<Outbox>();
   world.join(1,"Caster",0,0,box,0,{}, {},100,1);world.join(2,"Foe",100,0,box,0,{}, {},100,2);
   world.set_vitals(1,{500,500,100,100},100);world.set_vitals(2,{500,500,100,100},100);
   const auto source=*world.effect_scope(1),target=*world.effect_scope(2);
   check(world.combat_armor(target,60)&&world.physical_armor(target,60));
   world.configure_attack_critical_draws([]{return .999999;});check(world.configure_projectiles({1000,2}));
   check(world.ordinary_weapon(source,*owned,20)&&world.ordinary_attack(source,2,100));
   world.tick(600);world.tick(700);world.cancel_ordinary_attack(source);
   const double remaining=world.resources(2)->health;check(remaining<500&&remaining>=470);
   if(baseline)check(remaining==*baseline);else baseline=remaining;
  }
 }
 for(unsigned raw:{3u,4u}){
  Instance elemental;auto box=std::make_shared<Outbox>();
  elemental.join(1,"Caster",0,0,box,0,{}, {},100,1);elemental.join(2,"Foe",10,0,std::make_shared<Outbox>(),0,{}, {},100,2);
  for(unsigned id:{1u,2u})elemental.set_vitals(id,{480,480,20,20},100);
  const auto source=*elemental.effect_scope(1),target=*elemental.effect_scope(2);
  combat::weapon::Owned staff{12,1,{20,20,raw},12,false,true,26};
  check(elemental.ordinary_weapon(source,staff,20)&&elemental.configure_projectiles({100,2}));
  check(!elemental.ordinary_attack(source,2,100)); // No guessed target armor.
  check(raw==3?elemental.elemental_armor(target,{},60):elemental.lightning_armor(target,60));
  check(elemental.ordinary_attack(source,2,100));elemental.tick(600);check(elemental.resources(2)->health==480);
  elemental.tick(700);check(elemental.resources(2)->health==460);elemental.tick(800);check(elemental.resources(2)->health==460);
 }
 {
  Instance power;auto box=std::make_shared<Outbox>();power.join(1,"Ranger",0,0,box,0,{}, {},100,1);power.join(2,"Foe",900,0,box,0,{}, {},100,2);
  power.set_vitals(1,{480,480,20,20},100);power.set_vitals(2,{480,480,20,20},100);
  const auto source=*power.effect_scope(1),target=*power.effect_scope(2);
  combat::weapon::Owned bow{8,1,{500,500,1},0,true,false,5};check(power.ordinary_weapon(source,bow,1));
  const auto plan=*combat::weapon::attack_skill(394,12);check(plan.fixed_damage==45&&power.melee_target(source,2,100,394));
  check(power.melee_attack(source,target,bow,plan,100));power.tick(460);check(power.resources(2)->health==435);
  combat::weapon::Strike input;input.blind=false;input.weakness=true;input.location_armor=500;
  check(combat::weapon::resolve_attack_skill(input,plan,false)->damage==45);
  input.blind=true;input.miss_roll=.5;check(combat::weapon::resolve_attack_skill(input,plan,false)->missed);
 }

 // Real Determined Shot ledger results create retained reset notices only
 // for failed hits; the current impact bar includes exact copied attack keys.
 for(unsigned mode=0;mode<7;++mode){
  Instance flight;auto box=std::make_shared<Outbox>();
  flight.join(1,"Ranger",0,0,box,0,{}, {},100,1);flight.join(2,"Foe",900,0,box,0,{}, {},100,2);
  for(unsigned id:{1u,2u})flight.set_vitals(id,{480,480,20,20},100);
  const auto source=*flight.effect_scope(1),target=*flight.effect_scope(2);
  combat::weapon::Owned bow{8,1,{20,20,1},12,false,false,5};check(flight.ordinary_weapon(source,bow,20)&&flight.physical_armor(target,60));
  skills::Slots bar{};bar[0]={402,0};bar[1]={400,7};bar[2]={281,0};check(flight.configure_adrenaline(source,bar,100));
  if(mode==1||mode==4||mode==6){flight.configure_attack_miss_draws([]{return .5;});check(flight.apply_condition(target,source,combat::conditions::Kind::blind,100,5000));}
  if(mode==4){check(flight.diversion(target,source,*skills::effects::diversion(12),100));
   check(flight.prepare_diversion_skill_use(source,{400,7},1,100));check(flight.finalize_diversion_skill_use(source,1,100,6000,0).has_value());
   check(flight.skill_disabled_slot(source,{400,7},100).value()>100);}
  check(flight.melee_attack(source,target,bow,*combat::weapon::attack_skill(402,12),100));
  if(mode==5){flight.configure_attack_block_draws([]{return 0.;});check(flight.status_mutation(target,448,1,12,100,100+*skills::effects::escape(448,12),true));}
  if(mode==6){bar[1]={400,9};check(flight.configure_adrenaline(source,bar,300));}
  if(mode==2)flight.set_vitals(2,{0,480,20,20},100);
  if(mode==3){++bow.revision;check(flight.ordinary_weapon(source,bow,20));}
  flight.tick(460);const auto outcomes=flight.take_attack_outcomes(source);check(outcomes.size()==1);
  check(outcomes[0].delivery==combat::weapon::AttackDelivery::ranged);
  const auto notices=*flight.pending_attack_resets(source);
  if(mode==1||mode==4||mode==5||mode==6){check(outcomes[0].outcome==(mode==5?combat::weapon::AttackOutcome::block:combat::weapon::AttackOutcome::miss)&&notices.size()==1);
   check(notices[0].recharge.trigger.event==outcomes[0].event&&!notices[0].recharge.before.local_recharge_known);
   check(notices[0].recharge.barriers==std::vector<skills::Slot>{{402,0},{400,mode==6?9u:7u}});
   if(mode==4)check(flight.skill_disabled_slot(source,{400,7},460)==0);
   check(flight.pending_attack_resets(source)==notices&&flight.acknowledge_attack_reset(notices[0].stamp)&&flight.pending_attack_resets(source)->empty());
  }else check(notices.empty());
  check(flight.cooldown_order_failures(source)==0);flight.tick(600);check(flight.take_attack_outcomes(source).empty()&&flight.pending_attack_resets(source)->empty());
 }

 // Retained reset backpressure preserves the next flight until the session
 // acknowledges space. Proven equipment cancellation must still drain it.
 for(bool cancel:{false,true}){
  Instance flight;auto box=std::make_shared<Outbox>();
  flight.join(1,"Ranger",0,0,box,0,{}, {},100,1);flight.join(2,"Foe",900,0,box,0,{}, {},100,2);
  for(unsigned id:{1u,2u})flight.set_vitals(id,{480,480,20,20},100);
  const auto source=*flight.effect_scope(1),target=*flight.effect_scope(2);
  combat::weapon::Owned bow{8,1,{20,20,1},12,false,false,5};check(flight.ordinary_weapon(source,bow,20)&&flight.physical_armor(target,60));
  skills::Slots bar{};bar[0]={402,0};check(flight.configure_adrenaline(source,bar,100));
  flight.configure_attack_miss_draws([]{return .5;});check(flight.apply_condition(target,source,combat::conditions::Kind::blind,100,5000));
  for(unsigned i=0;i<65;++i)check(flight.melee_attack(source,target,bow,*combat::weapon::attack_skill(402,12),100+i));
  flight.tick(700);const auto first=*flight.pending_attack_resets(source);check(first.size()==64&&flight.take_attack_outcomes(source).size()==64);
  if(cancel){++bow.revision;check(flight.ordinary_weapon(source,bow,20));}
  else check(flight.acknowledge_attack_reset(first.back().stamp));
  flight.tick(800);const auto final=flight.take_attack_outcomes(source);check(final.size()==1);
  check(final[0].outcome==(cancel?combat::weapon::AttackOutcome::cancelled:combat::weapon::AttackOutcome::miss));
  check(flight.pending_attack_resets(source)->size()==(cancel?64u:1u)&&flight.cooldown_order_failures(source)==0);
 }

 // A stale sampled release must not finalize a402 use when its outcome
 // ledger already observed a newer arrow, even if resource clocks lag behind.
 {Instance flight;auto box=std::make_shared<Outbox>();
  flight.join(1,"Ranger",0,0,box,0,{}, {},100,1);flight.join(2,"Foe",900,0,box,0,{}, {},100,2);
  for(unsigned id:{1u,2u})flight.set_vitals(id,{480,480,20,20},100);
  const auto source=*flight.effect_scope(1),target=*flight.effect_scope(2);
  combat::weapon::Owned bow{8,1,{20,20,1},12,false,false,5};check(flight.ordinary_weapon(source,bow,20)&&flight.physical_armor(target,60));
  skills::Slots bar{};bar[0]={402,0};check(flight.configure_adrenaline(source,bar,100));
  check(flight.melee_attack(source,target,bow,*combat::weapon::attack_skill(403,12),1600));
  check(flight.prepare_diversion_skill_use(source,{402,0},77,1500));
  check(!flight.completed_bow_attack(source,target,bow,*combat::weapon::attack_skill(402,12),77,1500,10000,0));
  check(!flight.cooldown_completion_stamp(source,77)&&flight.abort_diversion_skill_use(source,77));
  flight.tick(2000);check(flight.take_attack_outcomes(source).size()==1&&flight.pending_attack_resets(source)->empty());
 }

 // A newer resource event or shot release settles the460 arrival first.
 // A subsequently observed older tick must not postpone or replay it.
 for(unsigned advanced:{1u,2u,3u}){
  Instance flight;auto box=std::make_shared<Outbox>();
  flight.join(1,"Ranger",0,0,box,0,{}, {},100,1);flight.join(2,"Foe",900,0,box,0,{}, {},100,2);
  for(unsigned id:{1u,2u})flight.set_vitals(id,{480,480,20,20},100);
  const auto source=*flight.effect_scope(1),target=*flight.effect_scope(2);
  combat::weapon::Owned bow{8,1,{20,20,1},0,false,false,5};check(flight.ordinary_weapon(source,bow,20));
  const auto plan=*combat::weapon::attack_skill(394,12);
  check(flight.melee_attack(source,target,bow,plan,100));
  if(advanced==3)check(flight.melee_attack(source,target,bow,plan,600));
  else check(flight.settle_resources(advanced==1?source:target,461));
  const auto first=flight.take_attack_outcomes(source);
  if(flight.resources(2)->health!=435||first.size()!=1||first[0].at!=460)
   std::cerr<<"advanced="<<advanced<<" HP="<<flight.resources(2)->health<<" outcomes="<<first.size()<<" at="<<(first.empty()?0:first[0].at)<<'\n';
  check(flight.resources(2)->health==435&&first.size()==1&&first[0].outcome==combat::weapon::AttackOutcome::hit&&first[0].at==460);
  flight.tick(460);check(flight.resources(2)->health==435&&flight.take_attack_outcomes(source).empty());
  flight.tick(700);check(flight.resources(2)->health==435&&flight.take_attack_outcomes(source).empty());
  flight.tick(800);check(flight.resources(2)->health==435&&flight.take_attack_outcomes(source).empty());
  if(advanced==3){flight.tick(1060);const auto second=flight.take_attack_outcomes(source);
   check(flight.resources(2)->health==390&&second.size()==1&&second[0].outcome==combat::weapon::AttackOutcome::hit&&second[0].at==960);}
 }
 for(unsigned cancelled=0;cancelled<3;++cancelled){
  Instance flight;auto box=std::make_shared<Outbox>();
  flight.join(1,"Ranger",0,0,box,0,{}, {},100,1);flight.join(2,"Foe",900,0,box,0,{}, {},100,2);
  for(unsigned id:{1u,2u})flight.set_vitals(id,{480,480,20,20},100);
  const auto source=*flight.effect_scope(1),target=*flight.effect_scope(2);
  combat::weapon::Owned bow{8,1,{20,20,1},0,false,false,5};check(flight.ordinary_weapon(source,bow,20));
  check(flight.melee_attack(source,target,bow,*combat::weapon::attack_skill(394,12),100));
  check(flight.settle_resources(source,459));
  if(cancelled==0){++bow.revision;check(flight.ordinary_weapon(source,bow,20));}
  else flight.set_vitals(cancelled==1?1:2,{0,480,20,20},100);
  flight.tick(458);check(flight.take_attack_outcomes(source).empty());
  flight.tick(600);const auto outcomes=flight.take_attack_outcomes(source);
  check(outcomes.size()==1&&outcomes[0].outcome==(cancelled==2?combat::weapon::AttackOutcome::target_retired:combat::weapon::AttackOutcome::cancelled));
  check(flight.resources(2)->health==(cancelled==2?0u:480u));
  flight.tick(700);check(flight.take_attack_outcomes(source).empty());
 }

 for(const bool prepared:{false,true}){
  Instance dual;auto box=std::make_shared<Outbox>();dual.join(1,"Ranger",0,0,box,0,{}, {},100,1);dual.join(2,"Foe",900,0,box,0,{}, {},100,2);
  dual.set_vitals(1,{480,480,20,20},100);dual.set_vitals(2,{480,480,20,20},100);
  const auto source=*dual.effect_scope(1),target=*dual.effect_scope(2);
  combat::weapon::Owned bow{7,1,{20,20,1},12,false,false,5};
  check(dual.ordinary_weapon(source,bow,20)&&dual.physical_armor(target,60));
  if(prepared)check(dual.status_mutation(source,432,1,12,100,24100,true));
  check(dual.melee_target(source,2,100,396)&&!dual.melee_target(source,2,100,407));
  auto plan=*combat::weapon::attack_skill(396,0);check(plan.arrows==2&&plan.base_percent==75);
  auto forged=plan;forged.arrows=3;check(!dual.melee_attack(source,target,bow,forged,100));
  check(dual.melee_attack(source,target,bow,plan,100));check(dual.resources(2)->health==480);
  const unsigned arrival=460; // Read the Wind adds damage, not attack-skill flight speed.
  dual.tick(arrival-100);check(dual.resources(2)->health==480);
  dual.tick(arrival);check(dual.resources(2)->health==(prepared?432u:450u));
  dual.tick(arrival+1);check(dual.resources(2)->health==(prepared?432u:450u));
 }

 for(unsigned skill:{1953u,2096u}){
  Instance triple;auto box=std::make_shared<Outbox>();
  triple.join(1,"Ranger",0,0,box,0,{}, {},100,1);triple.join(2,"Foe",900,0,box,0,{}, {},100,2);
  triple.set_vitals(1,{480,480,20,20},100);triple.set_vitals(2,{480,480,20,20},100);
  const auto source=*triple.effect_scope(1),target=*triple.effect_scope(2);
  combat::weapon::Owned bow{7,1,{20,20,1},12,false,false,5};
  check(triple.ordinary_weapon(source,bow,20)&&triple.physical_armor(target,60));
  const auto plan=combat::weapon::attack_skill(skill,6);check(plan&&plan->arrows==3&&plan->base_percent==65);
  check(triple.melee_target(source,2,100,skill)&&triple.melee_attack(source,target,bow,*plan,100));
  triple.tick(460);check(triple.resources(2)->health==441);
  check(triple.take_attack_outcomes(source).size()==3);
 }

 {
  Instance fire;auto box=std::make_shared<Outbox>();fire.join(1,"Fire Mage",0,0,box,0,{}, {},100,1);fire.join(2,"Foe",10,0,box,0,{}, {},100,2);
  fire.set_vitals(1,{480,480,20,20},100);fire.set_vitals(2,{480,480,20,20},100);
  const auto owner=*fire.effect_scope(1),target=*fire.effect_scope(2);
  const auto gear=*inventory::presearing::creation_weapon(inventory::baseline(),6,u"Fire Mage");
  const auto rod=combat::weapon::owned_weapon(gear,1,{6,"Fire Mage",{},true,{}});
  check(rod&&fire.ordinary_weapon(owner,*rod,1)&&fire.combat_armor(target,60)&&fire.elemental_armor(target,0,60)&&fire.configure_projectiles({100,2}));
  box->drain();check(fire.ordinary_attack(owner,target.actor,100));
  const auto animation=box->drain();
  const auto timing=std::find(animation.begin(),animation.end(),combat::weapon::timing_2022(1,1.75f,1.f));
  check(timing!=animation.end()&&std::next(timing)!=animation.end()&&*std::next(timing)==combat::weapon::start_2022(1,2));
  fire.tick(600);check(fire.resources(2)->health==480);
  fire.tick(700);check(fire.resources(2)->health==475);fire.tick(800);check(fire.resources(2)->health==475);
 }
 {
  Instance swords;auto box=std::make_shared<Outbox>();swords.join(1,"Warrior",0,0,box,0,{}, {},100,1);swords.join(2,"Foe",60,0,box,0,{}, {},100,2);
  for(unsigned id:{1u,2u})swords.set_vitals(id,{480,480,20,20},100);
  const auto s=*swords.effect_scope(1),t=*swords.effect_scope(2);
  combat::weapon::Owned sword{10,1,{10,10,2},12,false,false,27};
  check(swords.ordinary_weapon(s,sword,20)&&swords.physical_armor(t,60));
  const auto sever=*combat::weapon::attack_skill(382,12),gash=*combat::weapon::attack_skill(384,12);
  check(swords.melee_attack(s,t,sword,gash,100));check(!swords.has_condition(t,combat::conditions::Kind::deep_wound,100));
  check(swords.melee_attack(s,t,sword,sever,101));check(swords.has_condition(t,combat::conditions::Kind::bleeding,101));
  check(swords.melee_attack(s,t,sword,gash,102));check(swords.has_condition(t,combat::conditions::Kind::deep_wound,102));
  check(!swords.has_condition(t,combat::conditions::Kind::bleeding,21101));
  check(!swords.has_condition(t,combat::conditions::Kind::deep_wound,17102));
 }
 {
  Instance flight;auto box=std::make_shared<Outbox>();flight.join(1,"Caster",0,0,box,0,{}, {},100,1);flight.join(2,"Target",100,0,box,0,{}, {},100,2);flight.join(3,"Collateral",50,4,box,0,{}, {},100,2);
  for(unsigned id:{1u,2u,3u})flight.set_vitals(id,{480,480,20,20},100);
  const auto caster=*flight.effect_scope(1),target=*flight.effect_scope(2),collateral=*flight.effect_scope(3);
  check(flight.lightning_armor(target,80)&&flight.lightning_armor(collateral,80)&&flight.configure_projectiles({100,2}));
  check(flight.attack_activity(target,2000,100));const auto epoch=flight.attack_interrupt_epoch(target);
  check(flight.lightning_javelin(caster,target,*skills::effects::lightning_javelin(0),20,5,100));
  check(!flight.configure_projectiles({100,2}));flight.tick(1000);check(flight.resources(2)->health==480&&flight.resources(3)->health==480);
  flight.tick(1100);check(flight.resources(2)->health==465&&flight.resources(3)->health==465);
  check(flight.attack_interrupt_epoch(target)==*epoch+1);flight.tick(1200);check(flight.resources(2)->health==465&&flight.resources(3)->health==465);
  check(flight.configure_projectiles({100,2}));
 }
 {
  Instance stance;auto box=std::make_shared<Outbox>();stance.join(1,"Warrior",0,0,box);stance.set_vitals(1,{480,480,20,20},100);
  const auto owner=*stance.effect_scope(1);
  check(stance.status_mutation(owner,180,1,0,100,60100,true));
  check(stance.status_mutation(owner,346,2,0,100,8100,true));box->drain();
  check(stance.status_mutation(owner,1043,3,0,200,3200,true));
  const auto snapshot=*stance.status_snapshot(owner,200);
  check(snapshot.size()==2&&!skills::effects::frenzy_active(snapshot,200));
  check(std::any_of(snapshot.begin(),snapshot.end(),[](const auto& e){return e.skill==180;}));
  const auto packets=box->drain();check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==67&&p[1]==0&&p[6]==2;})==1);
  check(stance.status_mutation(owner,346,4,0,300,8300,true));
  check(skills::effects::frenzy_active(*stance.status_snapshot(owner,300),300));
  check(stance.status_snapshot(owner,300)->size()==2);
 }
 for(const bool frenzy:{false,true}){
  Instance spell;auto box=std::make_shared<Outbox>();spell.join(1,"Elementalist",0,0,box,0,{}, {},100,1);spell.join(2,"Warrior",60,0,box,0,{}, {},100,2);
  spell.set_vitals(1,{480,480,20,20},100);spell.set_vitals(2,{480,480,20,20},100);
  const auto source=*spell.effect_scope(1),target=*spell.effect_scope(2);
  check(spell.lightning_armor(target,60));if(frenzy)check(spell.status_mutation(target,346,1,0,100,8100,true));
  const auto plan=*skills::effects::lightning(232,0);
  check(spell.lightning(source,target,plan,20,100));
  const auto base=*combat::amount({combat::Kind::lightning,plan.damage,20,232,plan.penetration},60);
  check(spell.resources(2)->health==static_cast<unsigned>(std::ceil(480-base*(frenzy?2:1))));
 }
 for(const bool frenzy:{false,true}){
  Instance stance;auto box=std::make_shared<Outbox>();stance.join(1,"Source",0,0,box,0,{}, {},100,1);stance.join(2,"Target",60,0,box,0,{}, {},100,2);
  stance.set_vitals(1,{480,480,20,20},100);stance.set_vitals(2,{480,480,20,20},100);
  const auto s=*stance.effect_scope(1),t=*stance.effect_scope(2);combat::weapon::Owned sword{3,1,{20,20,2},12,false,false,27};
  check(stance.ordinary_weapon(s,sword,20)&&stance.physical_armor(t,60)&&stance.physical_absorption(t,3));
  if(frenzy)check(stance.status_mutation(t,346,1,0,100,8100,true));
  check(stance.ordinary_attack(s,2,100));stance.tick(600);check(stance.resources(2)->health==(frenzy?443:463));stance.cancel_ordinary_attack(s);
  stance.set_vitals(2,{480,480,20,20},100);
  const auto attack=*combat::weapon::attack_skill(322,0);check(stance.melee_attack(s,t,sword,attack,700));
  const unsigned ordinary=20+attack.bonus;check(480-stance.resources(2)->health==(frenzy?2*ordinary:ordinary)-3);
  if(frenzy)check(stance.status_mutation(t,346,1,0,800,{},false));
  stance.set_vitals(2,{480,480,20,20},100);check(stance.ordinary_attack(s,2,900));stance.tick(1400);check(stance.resources(2)->health==463);
 }
 for(const bool expires_before_impact:{false,true}){
  Instance flight;auto box=std::make_shared<Outbox>();flight.join(1,"Wand",0,0,box,0,{}, {},100,1);flight.join(2,"Frenzy",100,0,box,0,{}, {},100,2);
  flight.set_vitals(1,{480,480,20,20},100);flight.set_vitals(2,{480,480,20,20},100);
  const auto s=*flight.effect_scope(1),t=*flight.effect_scope(2);combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};
  check(flight.ordinary_weapon(s,wand,20)&&flight.combat_armor(t,60)&&flight.physical_absorption(t,3)&&flight.configure_projectiles({100,32}));
  check(flight.status_mutation(t,346,1,0,100,expires_before_impact?1500:8100,true));
  check(flight.ordinary_attack(s,2,100));flight.tick(600);check(flight.resources(2)->health==480);
  flight.tick(1600);if(flight.resources(2)->health!=(expires_before_impact?460u:440u))throw std::runtime_error("Frenzy projectile health="+std::to_string(flight.resources(2)->health)+" expired="+std::to_string(expires_before_impact));
 }
 Instance arena;auto a=std::make_shared<Outbox>(),b=std::make_shared<Outbox>();
 arena.join(1,"Caster",0,0,a,0,{}, {},100,1);arena.join(2,"Foe",100,0,b,0,{}, {},100,2);
 arena.set_vitals(1,{480,480,20,20},100);arena.set_vitals(2,{480,480,20,20},100);
 const auto source=*arena.effect_scope(1),target=*arena.effect_scope(2);
 combat::weapon::Owned weapon{1,1,{11,22,11},0,false,true};
 check(arena.ordinary_weapon(source,weapon,20));check(!arena.ordinary_attack(source,2,100));
 check(arena.combat_armor(target,60));a->drain();b->drain();check(arena.ordinary_attack(source,2,100));
 const auto count=[](const auto& packets,unsigned opcode,unsigned kind){unsigned n=0;for(const auto&p:packets)if(p.size()>=6&&(p[0]|(p[1]<<8))==opcode&&(unsigned(p[2])|(unsigned(p[3])<<8)|(unsigned(p[4])<<16)|(unsigned(p[5])<<24))==kind)++n;return n;};
 check(count(a->drain(),159,4)==1&&count(b->drain(),159,4)==1);
 arena.tick(500);check(arena.resources(2)->health==480);arena.tick(600);check(arena.resources(2)->health==480);arena.tick(656);check(arena.resources(2)->health==480);check(count(a->drain(),158,2)==1);
 arena.tick(700);check(arena.resources(2)->health==464);arena.tick(1850);check(count(a->drain(),159,4)==1);arena.tick(1950);check(count(a->drain(),159,4)==0);arena.tick(2350);check(arena.resources(2)->health==464);arena.tick(2406);check(arena.resources(2)->health==464);arena.tick(2450);check(arena.resources(2)->health==447);check(count(a->drain(),158,2)==1);
 weapon.revision=2;check(arena.ordinary_weapon(source,weapon,20));arena.tick(4100);check(arena.resources(2)->health==447);
 check(arena.ordinary_attack(source,2,4200));a->drain();arena.move(1,0,10,4201);check(count(a->drain(),158,3)==1);arena.cancel_ordinary_attack(source);check(count(a->drain(),158,3)==0);arena.tick(4700);check(arena.resources(2)->health==447);
 check(!arena.ordinary_attack({source.instance,source.actor,source.generation+1},2,4800));
 check(arena.ordinary_attack(source,2,4800));arena.leave(2);arena.join(2,"Recycled",100,0,b,0,{}, {},4801,2);arena.set_vitals(2,{480,480,20,20},100);arena.tick(5300);check(arena.resources(2)->health==480);
 arena.set_vitals(1,{480,480,20,20},100);const auto recycled=*arena.effect_scope(2);check(arena.physical_armor(recycled,100));combat::weapon::Owned sword{3,3,{15,22,2},12,false,false,27};check(arena.ordinary_weapon(source,sword,20));check(arena.ordinary_attack(source,2,5500));a->drain();arena.tick(6000);check(arena.resources(2)->health<480);check(count(a->drain(),158,1)==1);arena.cancel_ordinary_attack(source);
 const auto hp_before=arena.resources(2)->health;const auto power=*combat::weapon::attack_skill(322,12);check(arena.melee_attack(source,recycled,sword,power,6100));check(arena.resources(2)->health+34<hp_before);auto changed_sword=sword;changed_sword.revision++;check(!arena.melee_attack(source,recycled,changed_sword,power,6200));check(!arena.melee_attack(source,{recycled.instance,recycled.actor,recycled.generation+1},sword,power,6200));
 Instance moving;auto mb=std::make_shared<Outbox>();moving.join(1,"Warrior",0,0,mb,0,{}, {},100,1);moving.join(2,"Foe",60,0,mb,0,{}, {},100,2);moving.set_vitals(1,{480,480,20,20},100);moving.set_vitals(2,{480,480,20,20},100);const auto ms=*moving.effect_scope(1),mt=*moving.effect_scope(2);check(moving.physical_armor(mt,60));check(moving.ordinary_weapon(ms,sword,20));const auto protector=*combat::weapon::attack_skill(326,12);check(moving.melee_attack(ms,mt,sword,protector,100));const auto stationary_loss=480-moving.resources(2)->health;moving.set_vitals(2,{480,480,20,20},100);moving.steer(2,0,1,100);check(moving.melee_attack(ms,mt,sword,protector,101));check(480-moving.resources(2)->health==stationary_loss+34);moving.steer(2,0,0,102);moving.set_vitals(2,{480,480,20,20},100);check(moving.melee_attack(ms,mt,sword,protector,103));check(480-moving.resources(2)->health==stationary_loss);
 const unsigned wiki_refund[]={2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,7,7,7,7};for(unsigned rank=0;rank<=20;++rank)check(combat::weapon::attack_skill(1693,rank)->refund==wiki_refund[rank]);check(!combat::weapon::attack_skill(1693,21));
 Instance counter;counter.configure_attack_miss_draws([]{return .5;});auto cb=std::make_shared<Outbox>();counter.join(1,"Warrior",0,0,cb,0,{}, {},100,1);counter.join(2,"Foe",60,0,cb,0,{}, {},100,2);counter.set_vitals(1,{480,480,0,20},100);counter.set_vitals(2,{480,480,20,20},100);const auto cs=*counter.effect_scope(1),ct=*counter.effect_scope(2);check(counter.physical_armor(ct,60));check(counter.physical_armor(cs,60));check(counter.ordinary_weapon(cs,sword,20));check(counter.ordinary_weapon(ct,sword,20));const auto counter_plan=*combat::weapon::attack_skill(1693,12);check(counter.melee_attack(cs,ct,sword,counter_plan,100));check(counter.resources(1)->energy==0);
 check(counter.attack_activity(ct,600,100));check(counter.melee_attack(cs,ct,sword,counter_plan,101));check(counter.resources(1)->energy==5);counter.set_vitals(1,{480,480,0,20},100);check(counter.attack_activity(ct,{},102));check(counter.melee_attack(cs,ct,sword,counter_plan,103));check(counter.resources(1)->energy==0);check(counter.attack_activity(ct,600,103));check(counter.melee_attack(cs,ct,sword,counter_plan,600));check(counter.resources(1)->energy==0);check(!counter.attack_activity({ct.instance,ct.actor,ct.generation+1},1000,600));
 counter.set_vitals(2,{480,480,20,20},100);check(counter.ordinary_attack(ct,1,601));check(counter.melee_attack(cs,ct,sword,counter_plan,602));check(counter.resources(1)->energy==5);counter.cancel_ordinary_attack(ct);counter.set_vitals(1,{480,480,0,20},100);check(counter.melee_attack(cs,ct,sword,counter_plan,604));check(counter.resources(1)->energy==0);
 check(counter.attack_activity(ct,1100,604));check(counter.apply_condition(ct,cs,combat::conditions::Kind::blind,604,1000));counter.set_vitals(2,{480,480,20,20},100);check(counter.melee_attack(cs,ct,sword,counter_plan,605));check(counter.resources(2)->health==480&&counter.resources(1)->energy==0);
 Instance flight;auto out=std::make_shared<Outbox>();flight.join(1,"Source",0,0,out,0,{}, {},100,1);flight.join(2,"Target",100,0,out,0,{}, {},100,2);flight.set_vitals(1,{480,480,20,20},100);flight.set_vitals(2,{480,480,20,20},100);const auto fs=*flight.effect_scope(1),ft=*flight.effect_scope(2);check(flight.combat_armor(ft,60));check(flight.configure_projectiles({100,32}));weapon.revision=1;check(flight.ordinary_weapon(fs,weapon,20));check(flight.ordinary_attack(fs,2,100));flight.tick(600);check(flight.resources(2)->health==480);flight.steer(2,0,1,601);flight.tick(1600);check(flight.resources(2)->health==480);flight.cancel_ordinary_attack(fs);
 for(unsigned mode=0;mode<5;++mode){Instance pending;auto box=std::make_shared<Outbox>();pending.join(1,"Source",0,0,box,0,{}, {},100,1);pending.join(2,"Target",100,0,box,0,{}, {},100,2);pending.set_vitals(1,{480,480,20,20},100);pending.set_vitals(2,{480,480,20,20},100);const auto ps=*pending.effect_scope(1),pt=*pending.effect_scope(2);check(pending.combat_armor(pt,60));check(pending.configure_projectiles({100,32}));weapon.revision=1;check(pending.ordinary_weapon(ps,weapon,20));check(pending.ordinary_attack(ps,2,100));pending.tick(600);if(mode==0)pending.leave(1);else if(mode==1){pending.leave(2);pending.join(2,"New lifetime",100,0,box,0,{}, {},601,2);pending.set_vitals(2,{480,480,20,20},100);check(pending.combat_armor(*pending.effect_scope(2),60));}else if(mode==2){weapon.revision=2;check(pending.ordinary_weapon(ps,weapon,20));}else if(mode==3){check(pending.ordinary_weapon(ps,std::nullopt,20));}else pending.set_vitals(1,{0,480,20,20},100);pending.tick(1600);check(pending.resources(2)->health==480);}
 {
  Instance runes;auto box=std::make_shared<Outbox>();runes.join(1,"Attacker",0,0,box,0,{}, {},100,1);runes.join(2,"Runed Foe",60,0,box,0,{}, {},100,2);
  runes.set_vitals(1,{480,480,0,20},100);runes.set_vitals(2,{480,480,20,20},100);const auto rs=*runes.effect_scope(1),rt=*runes.effect_scope(2);
  auto fixed=sword;fixed.damage.minimum=20;fixed.damage.maximum=20;fixed.mastery=12;check(runes.ordinary_weapon(rs,fixed,20));check(runes.physical_armor(rt,60));
  check(runes.physical_absorption(rt,3));check(!runes.physical_absorption({rt.instance,rt.actor,rt.generation+1},1)&&!runes.physical_absorption(rt,4));
  check(runes.ordinary_attack(rs,2,100));runes.tick(600);check(runes.resources(2)->health==463);runes.cancel_ordinary_attack(rs);
  runes.set_vitals(2,{480,480,20,20},100);check(runes.physical_absorption(rt,0));check(runes.ordinary_attack(rs,2,700));runes.tick(1200);check(runes.resources(2)->health==460);runes.cancel_ordinary_attack(rs);
  // A real Counterattack still refunds on hit after subtractive reduction.
  fixed.damage.minimum=1;fixed.damage.maximum=1;check(runes.ordinary_weapon(rs,fixed,20));check(runes.physical_absorption(rt,3));runes.set_vitals(2,{480,480,20,20},100);check(runes.attack_activity(rt,2000,1200));
  check(runes.melee_attack(rs,rt,fixed,*combat::weapon::attack_skill(1693,0),1201));check(runes.resources(1)->energy==2&&runes.resources(2)->health==477);
  runes.set_vitals(2,{480,480,20,20},100);check(runes.ordinary_attack(rs,2,1250));runes.tick(1750);check(runes.resources(2)->health==480);runes.cancel_ordinary_attack(rs);
  runes.set_vitals(2,{480,480,20,20},100);check(runes.combat_armor(rt,60));
  // This fixture already installed revision3;the shared staff fixture is older.
  // Preserve the production revision guard while testing elemental absorption.
  check(!runes.ordinary_weapon(rs,weapon,20));auto staff=weapon;staff.revision=fixed.revision+1;
  check(runes.ordinary_weapon(rs,staff,20));check(runes.ordinary_attack(rs,2,1800));runes.tick(2300);runes.tick(2400);
  check(runes.resources(2)->health==464);runes.cancel_ordinary_attack(rs);
 } {
    Instance signet;auto box=std::make_shared<Outbox>();signet.join(1,"Attacker",0,0,box,0,{}, {},100,1);signet.join(2,"Healing Warrior",60,0,box,0,{}, {},100,2);
    signet.set_vitals(1,{480,480,20,20},100);signet.set_vitals(2,{480,480,20,20},100);
    const auto attacker=*signet.effect_scope(1),healer=*signet.effect_scope(2);
    check(signet.physical_armor(healer,80));check(signet.ordinary_weapon(attacker,sword,20));
    check(signet.melee_attack(attacker,healer,sword,power,100));const auto normal_loss=480-signet.resources(2)->health;
    signet.set_vitals(2,{480,480,20,20},100);check(signet.healing_signet_activity(healer,101,2101));
    check(signet.melee_attack(attacker,healer,sword,power,102));check(480-signet.resources(2)->health>normal_loss);
    check(signet.healing_signet_activity(healer,103,{}));signet.set_vitals(2,{480,480,20,20},100);
    check(signet.melee_attack(attacker,healer,sword,power,104));check(480-signet.resources(2)->health==normal_loss);
    check(!signet.healing_signet_activity({healer.instance,healer.actor,healer.generation+1},105,2105));
 } {
    Instance interruption;auto box=std::make_shared<Outbox>();interruption.join(1,"Attacker",0,0,box,0,{}, {},100,1);interruption.join(2,"Foe",60,0,box,0,{}, {},100,2);
    interruption.set_vitals(1,{480,480,20,20},100);interruption.set_vitals(2,{480,480,20,20},100);
    const auto actor=*interruption.effect_scope(1),foe=*interruption.effect_scope(2);
    check(!interruption.interrupt_attack(actor,100)&&interruption.attack_interrupt_epoch(actor)==0);
    check(interruption.physical_armor(foe,60)&&interruption.ordinary_weapon(actor,sword,20));
    check(interruption.ordinary_attack(actor,2,100)&&interruption.interrupt_attack(actor,200));interruption.tick(700);
    check(interruption.resources(2)->health==480&&interruption.attack_interrupt_epoch(actor)==1);
    check(!interruption.interrupt_attack(actor,701)&&interruption.attack_interrupt_epoch(actor)==1);
    check(interruption.attack_activity(actor,1200,702)&&interruption.interrupt_attack(actor,703)&&interruption.attack_interrupt_epoch(actor)==2);
    check(!interruption.interrupt_attack({actor.instance,actor.actor,actor.generation+1},704));
    check(interruption.attack_activity(actor,1200,704)&&!interruption.interrupt_attack(actor,1200));
 } std::cout<<"PASS scoped actual damage, windup/repeat, revision/movement/departure cancellation\n";
}catch(const std::exception&e){std::cerr<<e.what();return 1;}


