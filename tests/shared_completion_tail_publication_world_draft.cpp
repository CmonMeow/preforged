#include "shared_completion_tail_publication_fixture_draft.h"
#include <algorithm>
static bool same_stat(const Packet& a,const Packet& expected){return a.size()==expected.size()&&a.size()>sizeof(float)&&std::equal(a.begin(),a.end()-sizeof(float),expected.begin());}
static void final_fraction(const std::vector<Packet>& packets,const Packet& expected){const auto last=std::find_if(packets.rbegin(),packets.rend(),[&](const auto& p){return same_stat(p,expected);});publication_check(last!=packets.rend()&&*last==expected);}
static void assert_tail_order(const std::vector<Packet>& packets,const CompletionTailPublicationFixture& f){const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));const auto heal=std::find(packets.begin(),packets.end(),particles::health_change_2022(1,1,f.vigor.healing,1000));publication_check(finish!=packets.end()&&heal!=packets.end()&&finish<heal);if(f.skill==1375){const auto body=std::find(packets.begin(),packets.end(),casting::generic(21,1,1184));publication_check(body!=packets.end()&&finish<body&&body<heal);}}
int main()try{
 // REQUIRED regression:9657 queues105 after later Aura125. The LAST actual
 // client Health fraction must represent authoritative125, not stale105.
 for(unsigned aura_rank:{0u,8u})for(bool refuse:{false,true}){
  CompletionTailPublicationFixture f;f.inject_refusal=refuse;publication_check(f.w.aura_restoration_enchantment(f.owner,180,aura_rank,100));const auto packets=f.finish(3000);const auto aura=*effects::aura_restoration(*find_skill(1375),aura_rank,false);
  const auto expected=100+f.vigor.healing+aura.healing;publication_check(expected==(aura_rank?141u:125u));publication_check(f.w.resources(1)->health==expected&&f.health_at_finalize==expected&&f.health_at_copy==expected&&f.completed==1&&f.finalized==1&&f.copied==1&&f.effects_committed==2);
  assert_tail_order(packets,f);const auto state=*f.w.resources(1);final_fraction(packets,vitals::public_health_2022(1,state).back());if(aura.energy){publication_check(state.energy==31);final_fraction(packets,vitals::packets_2022(1,state)[2]);}
  f.update(3000);f.update(3001);publication_check(f.completed==1&&f.w.resources(1)->health==expected&&f.box->drain().empty());
 }
 // Actual later sacrifice: heal precedes cost in STATE and in the captured
 // tail; delayed old505 must never overwrite the final cost-adjusted fraction.
 {CompletionTailPublicationFixture f(115,500);unsigned sacrifice_calls=0;double prior=0;std::optional<sacrifice::Loss> loss;f.h.health_sacrifice=[&](auto id,auto at){++sacrifice_calls;prior=f.w.resources(1)->health;loss=f.w.health_sacrifice(f.owner,id,at);return loss;};const auto due=1000+std::uint64_t(std::llround(find_skill(115)->activation*1000.));const auto packets=f.finish(due);publication_check(sacrifice_calls==1&&prior==500+f.vigor.healing&&loss&&!loss->killed&&f.w.resources(1)->health==static_cast<unsigned>(std::ceil(loss->remaining)));assert_tail_order(packets,f);final_fraction(packets,vitals::public_health_2022(1,*f.w.resources(1)).back());}
 // Native generic body/finish is outside the tail even without a deferred
 // producer. This also exercises real canonical copy completion observation.
 {CompletionTailPublicationFixture f(281);publication_check(f.w.aura_restoration_enchantment(f.owner,180,0,100));const auto packets=f.finish(2000);publication_check(f.completed==1&&f.finalized==1&&f.copied==1);assert_tail_order(packets,f);final_fraction(packets,vitals::public_health_2022(1,*f.w.resources(1)).back());}
 // Actual outer owner plus a LIVE child: later real World changes are held
 // with reaction output, then replayed in causal order after an owner marker.
 {CompletionTailPublicationFixture f;world::Outbox::DeferredQueue outer;casting::ActionCompletion result;const auto marker=casting::generic(8,1,1234);
  {world::Outbox::Capture capture(*f.box,outer);result.accepted=f.w.completed_skill_action(f.owner,1375,1000,nullptr,&result.publication,&result.seal_publication);publication_check(result.accepted&&bool(result.seal_publication)&&bool(result.publication));publication_check(f.w.heal_target(1,1,20,0,true,1000));publication_check(outer.packets.empty());result.seal_publication();f.box->enqueue({marker});result.publication();result.publication();}
  publication_check(!outer.packets.empty()&&outer.packets.front()==marker);f.box->enqueue(std::move(outer.packets));const auto packets=f.box->drain();publication_check(f.w.resources(1)->health==125);final_fraction(packets,vitals::public_health_2022(1,*f.w.resources(1)).back());
 }
 // Lethal Spiteful remains earlier than Aura/finalizer: Aura cannot resurrect
 // the source, and no stale predeath fraction survives at the end of output.
 {CompletionTailPublicationFixture f(1375,1,true);publication_check(f.w.aura_restoration_enchantment(f.owner,180,0,100));const auto packets=f.finish(3000);publication_check(f.w.resources(1)->health==0&&f.health_at_finalize==0&&f.completed==1&&f.finalized==1&&f.copied==0);assert_tail_order(packets,f);final_fraction(packets,vitals::public_health_2022(1,*f.w.resources(1)).back());}
 std::cout<<"PASS live completion tail: Aura125/141 finalHealth, rank8Energy31, sacrifice, nativeprefix, outerchild and lethalSpiteful\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
