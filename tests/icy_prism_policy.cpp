#include "gw_icy_prism.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Icy Prism policy line "+std::to_string(at.line()));
}
int main()try{
 const Scope caster{1,1,1},target{1,2,1};
 for(unsigned rank=0;rank<=20;++rank){const auto p=icy_prism(rank);check(p&&p->damage==15+4*rank&&p->secondary_damage==p->damage&&p->overcast==5&&p->energy==10);}
 check(!icy_prism(21));
 for(unsigned status:{0u,209u,76u,129u,66u})for(unsigned at:{1000u,1100u,2000u}){
  Registry registry(target);if(status)check(registry.apply(target,{1,caster.actor,status,12,Category::hex,1000,2000},Replace::same_skill).accepted);
  IcyPrismAnchor anchor{caster,target,100,0,0,registry};
  std::vector<IcyPrismRecipient> candidates{{caster,0,0,0,false,true,true,60},{target,100,0,0,true,true,true,60},
   {{1,3,1},352,0,0,true,true,true,60},{{1,4,1},352.01f,0,0,true,true,true,{}},
   {{1,5,1},100,0,0,false,true,true,{}},{{1,6,1},100,0,1,true,true,true,{}},
   {{1,7,1},100,0,0,true,false,true,{}},{{1,8,1},100,0,0,true,true,false,{}}};
  const auto plan=*icy_prism(12);const bool expanded=status==209&&at<2000;
  const auto result=select_icy_prism(plan,anchor,candidates,at);check(result&&result->expanded==expanded&&result->recipients.size()==(expanded?2u:1u));
  check(std::count(result->recipients.begin(),result->recipients.end(),target)==1);
  candidates[2].cold_armor.reset();check(bool(select_icy_prism(plan,anchor,candidates,at))==!expanded);
 }
 Registry registry(target);const IcyPrismAnchor anchor{caster,target,100,0,0,registry};const auto plan=*icy_prism(12);
 const std::vector<IcyPrismRecipient> candidates{{target,100,0,0,true,true,true,60}};
 auto bad=plan;++bad.secondary_damage;check(!select_icy_prism(bad,anchor,candidates,1000));
 bad=plan;bad.kind=combat::Kind::fire;check(!select_icy_prism(bad,anchor,candidates,1000));
 auto stale=candidates;++stale[0].owner.generation;check(!select_icy_prism(plan,anchor,stale,1000));
 auto duplicate=candidates;duplicate.push_back(candidates[0]);check(!select_icy_prism(plan,anchor,duplicate,1000));
 auto mismatch=anchor;mismatch.registry=Registry({1,2,2});check(!select_icy_prism(plan,mismatch,candidates,1000));
 check(!select_icy_prism(plan,anchor,{},1000));
 std::cout<<"PASS Icy Prism pinned plan,primary-only versus Water-hex splash,once-only primary,inclusive nearby boundary,expiry,eligibility,cold-profile and scope guards;World/Runtime pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

