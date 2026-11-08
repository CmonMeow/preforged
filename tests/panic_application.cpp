#include "gw_panic_application.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Panic application");}
int main()try{const Scope caster{1,42,1},target{1,43,1};Registry original(target);const auto plan=*panic(12);
 const auto applied=prepare_panic_application(original,caster,target,1,plan,100);check(applied&&original.active(target,100).empty()&&applied->entry.expires==8100);
 check(current_panic_application(*applied,applied->registry,caster,target,8099)&&!current_panic_application(*applied,applied->registry,caster,target,8100));
 auto stale=caster;++stale.generation;check(!current_panic_application(*applied,applied->registry,stale,target,200));
 stale=target;++stale.generation;check(!prepare_panic_application(original,caster,stale,1,plan,100));
 const Scope replacement{1,44,2};const auto renewed=prepare_panic_application(applied->registry,replacement,target,2,plan,500);
 check(renewed&&renewed->change.removed.size()==1&&renewed->change.removed.front().instance==1&&renewed->entry.cause==44&&renewed->entry.expires==8500);
 check(!current_panic_application(*applied,renewed->registry,caster,target,600)&&current_panic_application(*renewed,renewed->registry,replacement,target,600));
 check(!prepare_panic_application(original,target,target,1,plan,100)&&!prepare_panic_application(original,caster,target,0,plan,100));
 check(!prepare_panic_application(original,caster,target,1,plan,UINT64_MAX-7999));
 auto forged=plan;++forged.duration;check(!prepare_panic_application(original,caster,target,1,forged,100));
 forged=plan;forged.reaction_radius=322;check(!prepare_panic_application(original,caster,target,1,forged,100));
 std::cout<<"PASS immutable Panic application/renewal, exact caster/target generations and effect identity, expiry/overflow/forged-plan guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
