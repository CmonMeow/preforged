#include "../src/runtime/gw_ai_action_provenance.h"
#include "../src/runtime/gw_summoned_fire_imp.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::ai::action;
void check(bool yes,const char* message){if(!yes)throw std::runtime_error(message);}
int main()try{
 {
  namespace fi=fw::gw::ai::fire_imp;
  namespace combat=fw::gw::combat;
  const fi::Binding binding{{1,16,1},{1,7,1},20,14,"synthetic owner binding","summon wiki level20 FireMagic14"};
  const fi::Pose source{binding.caster,0,0,0,true},target{{1,17,1},180,0,0,true};
  combat::npcs::control::Token cast{binding.caster,0,1,combat::npcs::control::Action::spell,1000,2500,fw::gw::skills::Slot{186,0}};
  fi::Payment payment{binding.caster,{186,0},1,1000,30,20,30,"synthetic actual NPC Energy ledger"};
  auto launch=[&]{return fi::Projectile::launch(binding,cast,payment,source,target,0,0,true,true,{},2500);};
  check(launch().has_value(),"reviewed owned Fireball release rejected");
  auto wrong=payment;wrong.after=21;
  check(!fi::Projectile::launch(binding,cast,wrong,source,target,0,0,true,true,{},2500),"unpaid Fireball released");
  wrong=payment;wrong.caster=binding.owner;
  check(!fi::Projectile::launch(binding,cast,wrong,source,target,0,0,true,true,{},2500),"summoner Energy funded imp spell");
  check(!fi::Projectile::launch(binding,cast,payment,source,target,0,0,false,true,{},2500),"obstructed imp spell launched");
  auto unreviewed=binding;unreviewed.rank_evidence.clear();
  check(!fi::Projectile::launch(unreviewed,cast,payment,source,target,0,0,true,true,{},2500),"unknown FireMagic inferred from level");
  auto contact=[&](unsigned id,float x){combat::Actor actor;actor.id=id;actor.generation=1;
   actor.health=combat::Health{500,500};actor.damageable=true;actor.fire_armor=60;
   return fi::Recipient{{{1,id,1},x,0,0,true},actor,{},true};};
  std::vector<fi::Recipient> contacts{contact(17,180),contact(18,300),contact(19,400),contact(17,180)};
  auto flight=*launch();const auto at=flight.arrives();
  check(flight.impact(binding,true,true,contacts,at-1).outcome==fi::Outcome::pending,"damage before projectile flight");
  auto result=flight.impact(binding,true,true,contacts,at);
  check(result.outcome==fi::Outcome::hit&&result.hits.size()==2,"Fireball adjacent selection or dedup wrong");
  check(result.hits[0].caster==binding.caster&&result.hits[0].credit_owner==binding.owner&&
   result.hits[0].result.hits[0].skill==186&&result.hits[0].result.actor.health->current<500,
   "actual imp caster/owner credit or Fireball damage lost");
  check(flight.impact(binding,true,true,contacts,at).outcome==fi::Outcome::consumed,"projectile hit replayed");
  flight=*launch();auto stale=binding;++stale.owner.generation;
  check(flight.impact(stale,true,true,contacts,at).outcome==fi::Outcome::stale,"recycled owner inherited imp projectile");
  flight=*launch();check(flight.impact(binding,false,true,contacts,at).outcome==fi::Outcome::stale,"departed summoner left live release");
  flight=*launch();auto reused=contacts;reused.erase(reused.begin()+3);++reused[0].pose.scope.generation;
  check(flight.impact(binding,true,true,reused,at).outcome==fi::Outcome::stale,"recycled target inherited projectile");
  flight=*launch();auto dodged=contacts;dodged[0].pose.x=500;
  check(flight.impact(binding,true,true,dodged,at).outcome==fi::Outcome::miss,"moved target ignored explicit hit policy");
  flight=*launch();auto unknown=contacts;unknown[1].actor.fire_armor.reset();
  const auto denied=flight.impact(binding,true,true,unknown,at);
  check(denied.outcome==fi::Outcome::unsupported&&denied.hits.empty(),"unknown adjacent armor produced partial commit");
  cast.skill=fw::gw::skills::Slot{194,0};cast.completes=2000;payment.slot=*cast.skill;payment.after=25;
  auto flare=fi::Projectile::launch(binding,cast,payment,source,target,0,0,true,true,{},2000);
  check(flare.has_value(),"paid Flare release rejected");auto single=std::move(*flare);
  check(flare->impact(binding,true,true,contacts,single.arrives()).outcome==fi::Outcome::consumed,
   "moved projectile retained duplicate impact authority");
  check(single.impact(binding,true,true,contacts,single.arrives()).hits.size()==1,"Flare invented adjacent damage");
 }
 const Actor imp{1,2,49},owner{1,1,31},foe{1,1,45},other{1,1,39};
 Latch latch;latch.bind(imp);
 Context fireball{imp,foe,owner,Kind::skill,186};
 auto a=latch.start(fireball);check(a.has_value(),"imp starts owned-credit spell");
 check(!latch.start({imp,other,owner,Kind::skill,194}),"one pending noninstant action");
 check(!latch.cancel(*a+1)&&latch.pending(),"stale cancel cannot cancel active action");
 auto released=latch.release(*a,imp,foe);check(released.has_value(),"valid original lifetime releases");
 check(!latch.release(*a,imp,foe),"one release only");
 auto b=latch.start({imp,other,owner,Kind::skill,194});check(b&&*b!=*a,"unique serial across retarget");
 check(released->context.target==foe&&released->context.caster==imp&&released->context.credit_owner==owner,
   "released action keeps old target caster and owner across retarget");
 check(latch.cancel(*b)&&released->context.target==foe,"pending cancel cannot mutate release");
 b=latch.start(fireball);check(b.has_value(),"restart pending");
 check(!latch.release(*b,imp,Actor{1,2,45})&&!latch.pending(),"target reuse consumes stale pending");
 b=latch.start(fireball);check(b.has_value(),"restart before source replacement");
 latch.bind({1,3,49});check(!latch.pending(),"source replacement clears pending");
 check(!latch.start(fireball),"old caster generation rejected");
 check(!latch.release(*b,imp,foe),"old source release rejected");
 latch.bind(imp);
 check(!latch.start({imp,Actor{2,1,45},owner,Kind::skill,186}),"cross-instance target rejected");
 check(!latch.start({imp,foe,Actor{1,0,31},Kind::skill,186}),"unresolved credit owner rejected");
 check(!latch.start({imp,foe,owner,Kind::skill,0}),"skill ID required");
 check(!latch.start({imp,foe,owner,Kind::weapon_attack,186}),"weapon attack has no invented skill");
 b=latch.start({imp,{},owner,Kind::skill,1376});check(b.has_value(),"untargeted glyph accepted");
 check(!latch.release(*b,imp,foe),"untargeted release cannot invent target");
 b=latch.start({imp,{},owner,Kind::skill,1376});
 check(latch.release(*b,imp,{}).has_value(),"untargeted glyph released without target");
 std::cout<<"AI action provenance tests passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
