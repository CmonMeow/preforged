// Detached actual Echo acquisition through World finalized1375 receipt.
// Requires shared-completion-prefix-publication successor; no2138 dependency.
#include "shared_completion_prefix_publication_fixture_draft.h"
#include "gw_echo_commit.h"
int main()try{
 CompletionPrefixPublicationFixture f;f.policy.secondary=5;f.policy.learned[75]=1;
 const auto before=f.bar;f.bar[1]={75,0};publication_check(f.w.publish_skillbar_transition(f.owner,{before,f.bar},100));
 effects::TemporarySkillSession session(f.owner,f.bar);effects::TemporarySkillIds ids(f.owner);
 const auto echo=*effects::echo(75);publication_check(f.w.status_mutation(f.owner,75,800,0,100,100+echo.armed_duration,true,Slot{75,0}));
 publication_check(f.w.aura_restoration_enchantment(f.owner,180,0,100));
 unsigned observations=0;std::optional<effects::EchoBatchCommit> committed;
 f.h.completed_skill_use=[&](const effects::CooldownCompletion& receipt){
  ++observations;++f.copied;f.health_at_copy=f.w.resources(1)->health;
  publication_check(f.health_at_copy==125&&f.finalized==1&&receipt.slot==Slot{1375,0});
  committed=effects::observe_echo_completion(f.w,session,ids,f.owner,receipt);
  publication_check(bool(committed)&&committed->changes.size()==1&&committed->packets.size()==1);
  publication_check(session.bar()[1]==Slot{1375,1});
  publication_check(f.w.cooldown_bar_snapshot(f.owner)->bar==session.bar());
  // Same actual owner-thread helper/publication route as Session auth. These
  // UI packets must join the prefix-owned stream, never bypass it through raw transport.
  f.box->enqueue(committed->packets);publication_check(f.box->drain().empty());return true;
 };
 const auto packets=f.finish(3000);publication_check(f.payments==1&&f.paid==10&&f.completed==1&&f.finalized==1&&observations==1);
 publication_check(f.health_at_finalize==125&&f.health_at_copy==125&&f.w.resources(1)->health==125);
 const auto copy=slot_packet_2022(1,1,{1375,1});const auto finish=casting::terminal(58,1),body=casting::generic(21,1,1184);
 const auto vigor=particles::health_change_2022(1,1,f.vigor.healing,1000);
 const auto restored=vitals::public_health_2022(1,*f.w.resources(1)).back();
 auto a=std::find(packets.begin(),packets.end(),finish),b=std::find(packets.begin(),packets.end(),body),c=std::find(packets.begin(),packets.end(),vigor),d=std::find(packets.begin(),packets.end(),restored),e=std::find(packets.begin(),packets.end(),copy);
 publication_check(a!=packets.end()&&b!=packets.end()&&c!=packets.end()&&d!=packets.end()&&e!=packets.end());
 publication_check(a<b&&b<c&&c<d&&d<e&&std::count(packets.begin(),packets.end(),copy)==1);
 const auto admission=session.admit(f.owner,f.policy,1,{1375,1},3001);publication_check(bool(admission)&&session.valid(*admission,f.policy,3001));
 f.update(3000);f.update(3001);publication_check(f.box->drain().empty()&&observations==1&&f.payments==1&&f.w.resources(1)->health==125);
 std::cout<<"PASS actual finalized1375 Echo copy1 acquisition and Session-outbox prefix publication after Vigor/Aura/native prefix\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
