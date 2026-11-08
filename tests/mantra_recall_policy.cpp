#include "gw_mantra_recall.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Recall end policy");}
int main()try{
 check(mantra_recall(0)->energy==10&&mantra_recall(12)->energy==22&&mantra_recall(15)->energy==25&&mantra_recall(20)->energy==30&&!mantra_recall(21));
 const Scope owner{1,42,1};
 for(auto reason:{RecallEnd::expiration,RecallEnd::removal,RecallEnd::renewal}){
  Registry registry(owner);check(registry.apply(owner,{1,42,82,12,Category::enchantment,100,20100},Replace::same_skill).accepted);
  const auto result=prepare_recall_end(registry,owner,1,reason,reason==RecallEnd::expiration?30100:200);
  check(result&&registry.lookup(owner,1)&&!result->registry.lookup(owner,1));
  check(bool(result->reward)==(reason!=RecallEnd::renewal));
  if(result->reward)check(result->reward->energy==22&&result->reward->at==(reason==RecallEnd::expiration?20100:200));
  check(!prepare_recall_end(result->registry,owner,1,reason,30100));
  check(!prepare_recall_end(registry,owner,1,RecallEnd::expiration,200));
  check(!prepare_recall_end(registry,owner,1,RecallEnd::removal,20100));
  auto stale=owner;++stale.generation;check(!prepare_recall_end(registry,stale,1,reason,30100));
 }
 Registry extended(owner);check(extended.apply(owner,{2,42,82,15,Category::enchantment,100,24100},Replace::same_skill).accepted);
 check(!prepare_recall_end(extended,owner,2,RecallEnd::expiration,20100));
 check(prepare_recall_end(extended,owner,2,RecallEnd::expiration,99999)->reward->at==24100);
 check(!prepare_recall_end(extended,owner,2,static_cast<RecallEnd>(99),200));
 Registry foreign(owner);check(foreign.apply(owner,{3,43,82,12,Category::enchantment,100,20100},Replace::same_skill).accepted);
 check(!prepare_recall_end(foreign,owner,3,RecallEnd::removal,200));
 std::cout<<"PASS Recall expiration/removal rewards, renewal suppression, delayed polling/extended expiry, committed one-use and lifetime/cause validation\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
