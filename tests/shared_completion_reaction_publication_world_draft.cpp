#include "shared_completion_reaction_publication_fixture_draft.h"
#include "shared_completion_reaction_publication_edge_cases_draft.h"
#include "stoneflesh_vigorous_publication_exact_after_draft.h"
int main()try{
 for(unsigned skill:{1375u,281u})for(bool refuse:{false,true}){
  CompletionPublicationFixture f(skill);f.inject_refusal=refuse;const auto due=skill==1375?3000u:2000u;const auto packets=f.finish(due);
  publication_check(f.completed==1&&f.effects_committed==1&&f.finalized==1&&f.copied==1);
  const auto expected=100+f.vigor.healing+(skill==281?effects::direct_heal_amount(281,0,0,100,1000).value():0);
  publication_check(f.w.resources(1)->health==expected&&f.health_at_finalize==expected&&f.health_at_copy==expected);
  const auto heal=particles::health_change_2022(1,1,f.vigor.healing,1000);const auto finish=casting::terminal(58,1);
  publication_check(std::count(packets.begin(),packets.end(),heal)==1&&std::count(packets.begin(),packets.end(),finish)==1);
  publication_check(std::find(packets.begin(),packets.end(),finish)<std::find(packets.begin(),packets.end(),heal));
  if(skill==1375){const auto recharge=casting::slot_event(228,1,{1375,0},15),body=casting::generic(21,1,1184);auto a=std::find(packets.begin(),packets.end(),recharge),b=std::find(packets.begin(),packets.end(),body),c=std::find(packets.begin(),packets.end(),heal);
   publication_check(a<std::find(packets.begin(),packets.end(),finish)&&std::find(packets.begin(),packets.end(),finish)<b&&b<c);}
  f.update(due);f.update(due+1);publication_check(f.completed==1&&f.effects_committed==1&&f.box->drain().empty()&&f.w.resources(1)->health==expected);
 }
 // Spiteful STATE remains before real World finalization/copy observation.
 for(bool lethal:{false,true}){
  CompletionPublicationFixture f(1375,lethal?1u:100u,true);const auto packets=f.finish(3000);publication_check(f.completed==1&&f.effects_committed==1&&f.finalized==1);
  publication_check(f.health_at_finalize==f.w.resources(1)->health);publication_check(lethal?f.health_at_finalize==0:f.health_at_finalize<100);
  publication_check(lethal?f.copied==0:f.copied==1&&f.health_at_copy==f.health_at_finalize);
  const auto heal=particles::health_change_2022(1,1,f.vigor.healing,1000);const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));const auto body=std::find(packets.begin(),packets.end(),casting::generic(21,1,1184));const auto reaction=std::find(packets.begin(),packets.end(),heal);
  publication_check(finish!=packets.end()&&body!=packets.end()&&reaction!=packets.end()&&finish<body&&body<reaction);
  f.update(3000);publication_check(f.completed==1&&f.box->drain().empty());
 }
 run_completion_publication_edges();run_stoneflesh_publication_exact_after();
 std::cout<<"PASS generic deferred/nondeferred completion reaction publication, refusal, real finalizer/copy state and lethal Spiteful\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
