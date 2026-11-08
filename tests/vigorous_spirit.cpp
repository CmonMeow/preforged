#include "gw_vigorous_spirit.h"
#include <iostream>
#include <source_location>
#include <string>
#include <stdexcept>
using namespace fw::gw::skills;using namespace effects;
void check(bool value,std::source_location at=std::source_location::current()){if(!value)throw std::runtime_error("Vigorous Spirit line "+std::to_string(at.line()));}
int main()try{
 Scope owner{10,25,9};
 for(unsigned rank=0;rank<=20;++rank){
  const auto p=vigorous_spirit(rank);check(p&&p->healing==5+rank&&p->duration==30000&&p->energy==5&&p->activation==250&&p->aftercast==750&&p->recharge==4000);
  for(unsigned cause:{25u,31u}){
   Registry registry(owner);check(registry.apply(owner,{77,cause,254,rank,Category::enchantment,100,30100},Replace::same_skill).accepted);
   for(auto kind:{VigorousActionKind::attack,VigorousActionKind::completed_spell}){
    const auto skill=kind==VigorousActionKind::attack?0u:254u;
    check(vigorous_spirit_healing(registry,{owner,1,100,kind,skill})==VigorousHealing{77,5+rank});
    check(vigorous_spirit_healing(registry,{owner,2,30099,kind,skill})==VigorousHealing{77,5+rank});
    check(vigorous_spirit_healing(registry,{owner,3,99,kind,skill})==VigorousHealing{});
    check(vigorous_spirit_healing(registry,{owner,4,30100,kind,skill})==VigorousHealing{});
   }
   check(vigorous_spirit_healing(registry,{owner,5,200,VigorousActionKind::other_skill,347})==VigorousHealing{});
   check(!vigorous_spirit_healing(registry,{owner,6,200,VigorousActionKind::completed_spell,347}));
   check(!vigorous_spirit_healing(registry,{owner,0,200}));auto stale=owner;++stale.generation;check(!vigorous_spirit_healing(registry,{stale,7,200}));
   check(registry.remove(owner,77).has_value());check(vigorous_spirit_healing(registry,{owner,8,200})==VigorousHealing{});
   check(registry.apply(owner,{78,cause,254,rank,Category::enchantment,200,36200},Replace::same_skill).accepted);
   check(vigorous_spirit_healing(registry,{owner,9,35000})==VigorousHealing{78,5+rank});
  }
 }
 check(!vigorous_spirit(21));Registry registry(owner);
 check(!registry.apply(owner,{90,31,254,21,Category::enchantment,100,30100},Replace::same_skill).accepted);
 check(vigorous_spirit_healing(registry,{owner,1,200})==VigorousHealing{});
 std::cout<<"PASS Vigorous Spirit canonical21ranks,owned attack/spell healing selection,ally causes,expiry,extended duration,removal and stale scopes;World event consumption pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
