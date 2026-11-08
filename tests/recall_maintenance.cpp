#include "gw_recall_maintenance.h"
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Recall maintenance");}
int main(){
 const Scope caster{1,1,1},target{1,2,1};const auto plan=*recall_shadow();
 Registry initial(caster);MaintainedEnchantments ownership;
 const auto application=prepare_recall_application(plan,caster,target,100,1000,true,true,true,100,initial,ownership);
 check(bool(application));check(initial.active(caster,1000).empty()&&ownership.size()==0);
 check(application->registry.lookup(caster,100)->cause==caster.actor);
 check(application->ownership.maintained_target(caster,100,1000)==caster);
 check(!prepare_recall_application(plan,caster,target,101,1001,true,true,true,100,application->registry,application->ownership));
 for(bool alive:{false,true}){
  const auto end=prepare_recall_retirement(plan,application->anchor,caster,target,true,alive,2000,application->registry,application->ownership);
  check(end&&end->action.step==alive&&end->action.disabled_until==(alive?12000:0));
  check(end->registry.active(caster,2000).empty()&&end->ownership.size()==0);
  check(!prepare_recall_retirement(plan,end->anchor,caster,target,true,alive,2000,end->registry,end->ownership));
 }
 auto replacement=target;++replacement.generation;
 const auto stale=prepare_recall_retirement(plan,application->anchor,caster,replacement,true,true,2000,application->registry,application->ownership);
 check(stale&&!stale->action.step&&!stale->action.disabled_until);
 auto forged=application->anchor;++forged.applied;
 check(!prepare_recall_retirement(plan,forged,caster,target,true,true,2000,application->registry,application->ownership));
 check(!prepare_recall_retirement(plan,application->anchor,caster,target,true,true,999,application->registry,application->ownership));
 fw::gw::skills::Slots slots{};slots[0]={925,0};slots[1]={293,0};CooldownBarAuthority authority(caster);
 const auto bar=*authority.publish(caster,slots,1000);RecallDisables disables(caster);
 const auto transaction=prepare_recall_end_transaction(plan,application->anchor,caster,target,true,true,2000,
  application->registry,application->ownership,disables,bar,1);
 check(transaction&&transaction->retirement.ownership.size()==0&&transaction->disables.disabled_until(caster,{293,0},2000)==12000);
 auto badbar=bar;badbar.published.owner=target;
 check(!prepare_recall_end_transaction(plan,application->anchor,caster,target,true,true,2000,
  application->registry,application->ownership,disables,badbar,1));
 check(application->ownership.size()==1&&application->registry.lookup(caster,100).has_value());
}
