// Detached acceptance regression for full completion-prefix publication.
// Both real World interpositions must preserve chronological Health fractions.
#include "shared_completion_prefix_publication_fixture_draft.h"
#include <thread>
static Packet health_fraction(const CompletionPrefixPublicationFixture& f,unsigned hp){
 auto state=*f.w.resources(1);state.health=hp;return vitals::public_health_2022(1,state).back();
}
static bool same_health_stat(const Packet& a,const Packet& b){
 return a.size()==b.size()&&a.size()>sizeof(float)&&std::equal(a.begin(),a.end()-sizeof(float),b.begin());
}
int main()try{
 for(bool between:{false,true}){
  CompletionPrefixPublicationFixture f;publication_check(f.w.aura_restoration_enchantment(f.owner,180,0,100));
  unsigned interpositions=0;const unsigned extra=between?10:20;
  const auto foreign_heal=[&]{
   ++interpositions;bool healed=false;
   std::thread other([&]{healed=f.w.heal_target(1,1,extra,0,true,3000);});other.join();
   publication_check(healed);
  };
  if(between){
   const auto completed=f.h.completed_action;
   f.h.completed_action=[&](auto id,auto at)->casting::ActionCompletion{
    auto result=completed(id,at);publication_check(f.w.resources(1)->health==105);
    foreign_heal();publication_check(f.w.resources(1)->health==115);return result;
   };
  }else{
   const auto copied=f.h.completed_skill_use;
   f.h.completed_skill_use=[&](const effects::CooldownCompletion& receipt){
    const bool accepted=copied(receipt);publication_check(accepted&&f.w.resources(1)->health==125);
    foreign_heal();publication_check(f.w.resources(1)->health==145);return accepted;
   };
  }
  const auto packets=f.finish(3000);const unsigned final=between?135:145;
  publication_check(interpositions==1&&f.completed==1&&f.effects_committed==2&&f.finalized==1&&f.copied==1&&f.payments==1&&f.paid==10);
  publication_check(f.w.resources(1)->health==final);
  publication_check(f.health_at_finalize==(between?135u:125u)&&f.health_at_copy==(between?135u:125u));
  const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
  const auto body=std::find(packets.begin(),packets.end(),casting::generic(21,1,1184));
  publication_check(finish!=packets.end()&&body!=packets.end()&&finish<body);
  const auto first=health_fraction(f,105),middle=health_fraction(f,between?115:125),last=health_fraction(f,final);
  const auto a=std::find(packets.begin(),packets.end(),first),b=std::find(packets.begin(),packets.end(),middle),c=std::find(packets.begin(),packets.end(),last);
  publication_check(a!=packets.end()&&b!=packets.end()&&c!=packets.end());
  // A whole-frame reservation alone fails BETWEEN:105/135 then external115.
  // TLS-only capture fails AFTER:external145 then delayed105/125.
  publication_check(body<a&&a<b&&b<c);
  const auto latest=std::find_if(packets.rbegin(),packets.rend(),[&](const auto& p){return same_health_stat(p,last);});
  publication_check(latest!=packets.rend()&&*latest==last);
  f.update(3000);f.update(3001);publication_check(f.box->drain().empty()&&interpositions==1&&f.completed==1&&f.payments==1&&f.w.resources(1)->health==final);
  std::cout<<"PASS real World interposition "<<(between?"BETWEEN Vigor/Aura":"AFTER Aura")<<" chronological finalHealth="<<final<<'\n';
 }
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
