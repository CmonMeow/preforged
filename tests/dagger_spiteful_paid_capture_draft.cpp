#include "dagger_paid_pvp_fixture_draft.h"
int main()try{
 {Fixture f(780);check(f.world.spiteful_spirit(f.target,f.source,*effects::spiteful_spirit(0),20,1000));f.box->drain();
  check(f.start()==skills::casting::Reason::accepted);f.update(f.last_permit->release_time());
  check(f.last_result.accepted&&f.last_result.prerequisite_failed&&f.world.resources(1)->health==995);
  const auto packets=f.box->drain();const auto finish=skills::casting::terminal(46,1);
  const auto failure=*combat::weapon::dagger_failure_wire_draft(f.source,f.target),damage=combat::detail::integer(10,1,121);
  const auto a=std::find(packets.begin(),packets.end(),finish),b=std::find(packets.begin(),packets.end(),failure),c=std::find(packets.begin(),packets.end(),damage);
  check(a!=packets.end()&&b!=packets.end()&&c!=packets.end()&&a<b&&b<c);
  check(std::count(packets.begin(),packets.end(),damage)==1);
  if(f.last_result.publication)f.last_result.publication();check(f.box->drain().empty()&&f.world.resources(1)->health==995);
 }
 {Fixture f(780);check(f.world.spiteful_spirit(f.target,f.source,*effects::spiteful_spirit(0),20,1000));f.box->drain();
  world::Outbox::DeferredQueue outer;const auto before=combat::detail::integer(8,1,1),after=combat::detail::integer(8,1,0),damage=combat::detail::integer(10,1,121);
  {world::Outbox::Capture hold(*f.box,outer);bool refused=false;
   try{world::Outbox::DeferredQueue duplicate;world::Outbox::Capture invalid(*f.box,duplicate);}catch(const std::logic_error&){refused=true;}check(refused);
   f.box->enqueue({before});check(f.world.spiteful_action({f.source,effects::SpitefulPhase::attack_release,0,901,1000,true}));f.box->enqueue({after});
   check(f.box->drain().empty()&&f.world.resources(1)->health==995);
  }
  check(outer.packets.front()==before&&outer.packets.back()==after&&std::count(outer.packets.begin(),outer.packets.end(),damage)==1);
  f.box->enqueue(std::move(outer.packets));const auto published=f.box->drain();check(published.front()==before&&published.back()==after&&std::count(published.begin(),published.end(),damage)==1);
 }
 std::cout<<"PASS candidate nested Spiteful preserves outer capture, strict guard, paid failure prefix and once-only damage publication\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
