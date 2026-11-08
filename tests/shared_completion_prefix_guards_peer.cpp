// Detached concrete prefix-frame guards. Requires current composed candidate.
#include "shared_completion_prefix_publication_fixture_draft.h"
struct PrefixGuard {std::optional<casting::CompletionPublication> lease;~PrefixGuard(){if(lease)lease->finish();}void finish(){if(lease){lease->finish();lease.reset();}}};
static void no_success(const std::vector<Packet>& packets){
 publication_check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))==packets.end());
 publication_check(std::find(packets.begin(),packets.end(),casting::generic(21,1,1184))==packets.end());
}
int main()try{
 // Partial all-recipient reservation failure rolls back earlier recipient slots.
 {CompletionPrefixPublicationFixture f;auto other=std::make_shared<world::Outbox>();f.w.join(2,"Full observer",60,0,other,0,{}, {},100,2);f.w.set_vitals(2,{1000,1000,30,30},100);f.box->drain();other->drain();
  std::vector<std::uint64_t> blocked;for(unsigned i=0;i<128;++i){auto ticket=other->reserve_publication();publication_check(bool(ticket));blocked.push_back(*ticket);}
  publication_check(!f.w.prepare_completion_publication(f.owner,200));publication_check(!f.box->captured_on_this_thread()&&!other->captured_on_this_thread());
  std::vector<std::uint64_t> owner;for(unsigned i=0;i<128;++i){auto ticket=f.box->reserve_publication();publication_check(bool(ticket));owner.push_back(*ticket);}publication_check(!f.box->reserve_publication());
  for(auto ticket:owner)publication_check(f.box->resolve_publication(ticket,{}));for(auto ticket:blocked)publication_check(other->resolve_publication(ticket,{}));
  PrefixGuard fresh;fresh.lease=f.w.prepare_completion_publication(f.owner,200);publication_check(bool(fresh.lease));fresh.finish();publication_check(!f.box->captured_on_this_thread()&&f.box->drain().empty());
 }
 // Runtime already paid, but no producer/reaction is committed when P0 cannot be reserved.
 {CompletionPrefixPublicationFixture f;f.start();std::vector<std::uint64_t> blocked;for(unsigned i=0;i<128;++i){auto ticket=f.box->reserve_publication();publication_check(bool(ticket));blocked.push_back(*ticket);}
  f.update(3000);publication_check(!f.runtime.active()&&f.payments==1&&f.paid==10&&f.effects_committed==0&&f.completed==0&&f.finalized==0&&f.w.resources(1)->health==100&&f.w.resources(1)->energy==30);
  publication_check(!f.box->captured_on_this_thread());for(auto ticket:blocked)publication_check(f.box->resolve_publication(ticket,{}));const auto packets=f.box->drain();no_success(packets);
  f.update(3001);publication_check(f.payments==1&&f.effects_committed==0&&f.box->drain().empty());
 }
 // Actual lease acquisition followed by missing post-acquisition observation.
 {CompletionPrefixPublicationFixture f;unsigned frame_calls=0;const auto prepare=f.h.prepare_completion_publication;f.h.prepare_completion_publication=[&](auto owner,auto at){++frame_calls;return prepare(owner,at);};const auto snapshot=f.h.effect_snapshot;f.h.effect_snapshot=[&](auto at)->std::optional<std::vector<effects::Entry>>{if(at==3000)return {};return snapshot(at);};
  f.start();f.update(3000);publication_check(frame_calls==1&&!f.runtime.active()&&f.payments==1&&f.effects_committed==0&&f.completed==0&&f.finalized==0&&f.w.resources(1)->health==100&&f.w.resources(1)->energy==30);no_success(f.box->drain());publication_check(!f.box->captured_on_this_thread());
  PrefixGuard fresh;fresh.lease=f.w.prepare_completion_publication(f.owner,3001);publication_check(bool(fresh.lease));fresh.finish();publication_check(f.box->drain().empty());
 }
 // Rejected producer cannot strand native prefix or fabricate success.
 {CompletionPrefixPublicationFixture f;f.h.simple_enchantment_completed=[](auto,auto,auto,unsigned,auto,auto)->casting::EffectCompletion{return {};};f.start();f.update(3000);
  publication_check(!f.runtime.active()&&f.payments==1&&f.effects_committed==0&&f.completed==0&&f.finalized==0);no_success(f.box->drain());publication_check(!f.box->captured_on_this_thread());
 }
 // Approved ground visual resolves native P0 BEFORE publication-clock sampling;
 // only then can the existing late World callback arm the actual ground event.
 for(bool valid:{false,true}){
  CompletionPrefixPublicationFixture f;auto foe=std::make_shared<world::Outbox>();f.w.join(2,"Rift foe",600,0,foe,0,{}, {},100,2);f.w.set_vitals(2,{1000,1000,30,30},100);const auto target=*f.w.effect_scope(2);publication_check(f.w.lightning_armor(target,60));f.box->drain();
  PrefixGuard guard;guard.lease=f.w.prepare_completion_publication(f.owner,1000);publication_check(bool(guard.lease));std::vector<Packet> visuals;std::function<void()> arm;
  publication_check(f.w.spirit_rift(f.owner,target,*effects::spirit_rift(0),20,1000,&visuals,&arm)&&visuals.size()==1&&bool(arm));
  auto offered=visuals;if(!valid)offered[0][2]^=1;guard.lease->prepare_prefix();bool clock_called=false,visible_at_clock=false;
  f.w.skill_event_live(1,offered,[&]{clock_called=true;const auto packets=f.box->drain();visible_at_clock=std::find(packets.begin(),packets.end(),visuals[0])!=packets.end();return std::uint64_t{2000};});
  publication_check(clock_called&&visible_at_clock==valid);guard.lease->publish_prefix();arm();guard.finish();
  publication_check(f.w.settle_resources(f.owner,4999));publication_check(f.w.resources(2)->health==1000);
  publication_check(f.w.settle_resources(f.owner,5000));publication_check(f.w.resources(2)->health==(valid?975u:1000u));
 }
 // Delayed prefix never gains authority over a replacement raw actor ID.
 {CompletionPrefixPublicationFixture f;const auto old=f.owner;PrefixGuard guard;guard.lease=f.w.prepare_completion_publication(old,200);publication_check(bool(guard.lease));
  f.w.leave(1);auto replacement=std::make_shared<world::Outbox>();f.w.join(1,"Replacement",0,0,replacement,0,{}, {},200,1);f.w.set_vitals(1,{777,1000,30,30},100);const auto fresh=*f.w.effect_scope(1);publication_check(fresh!=old);replacement->drain();
  guard.lease->prepare_prefix();f.w.skill_event(1,{casting::terminal(58,1),casting::generic(21,1,1184)},200);guard.lease->publish_prefix();guard.finish();
  no_success(replacement->drain());no_success(f.box->drain());publication_check(f.w.resources(1)->health==777&&f.w.resources(1)->energy==30&&!f.w.prepare_completion_publication(old,200));
  PrefixGuard current;current.lease=f.w.prepare_completion_publication(fresh,200);publication_check(bool(current.lease));current.finish();publication_check(!replacement->captured_on_this_thread());
 }
 std::cout<<"PASS prefix reservation rollback, paid early exits, actual native visual approval/arming and fullScope replacement gates\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
