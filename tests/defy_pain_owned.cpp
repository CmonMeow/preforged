#include "gw_defy_pain_owned.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool x){if(!x)throw std::runtime_error("Defy owned identity");}
int main()try{const Scope owner{1,42,7};
 for(bool pvp:{false,true})for(unsigned rank=0;rank<=20;++rank){const auto plan=*defy_pain(rank,pvp);TemporaryHealth ledger(owner,500,100);Registry registry(owner);
  check(ledger.apply_defy_pain(plan,7,100));check(!active_defy_pain(registry,ledger,owner,100));
  const Entry entry{7,42,plan.skill,rank,plan.stance?Category::stance:Category::other,100,20100};check(registry.apply(owner,entry,Replace::independent).accepted);
  const auto active=active_defy_pain(registry,ledger,owner,100);check(active&&*active==plan);check(!active_defy_pain(registry,ledger,{1,42,8},100));check(!active_defy_pain(registry,ledger,owner,20100));
  for(unsigned field=0;field<4;++field){Registry wrong(owner);auto altered=entry;if(field==0)++altered.cause;if(field==1)altered.rank=(rank+1)%21;if(field==2)altered.kind=Category::hex;if(field==3)altered.expires=20200;check(wrong.apply(owner,altered,Replace::independent).accepted==(field!=2));check(!active_defy_pain(wrong,ledger,owner,100));}
  check(registry.remove(owner,7)&&!active_defy_pain(registry,ledger,owner,101));
 }
 std::cout<<"PASS42Defy owned bonuses require exact ledger/registry/lifetime/cause/rank/category/expiry identity\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


