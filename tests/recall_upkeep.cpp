#include "gw_maintained_energy.h"
#include <stdexcept>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Recall upkeep");}
int main(){
 const Scope owner{1,1,1},ally{1,2,1};const auto p=*recall_shadow();
 const auto anchor=*prepare_recall_anchor(p,owner,ally,100,1000,true,true,true,100);
 Registry registry(owner);check(registry.apply(owner,{100,1,925,0,Category::enchantment,1000,{}},Replace::same_skill).accepted);
 MaintainedEnchantments upkeep;check(upkeep.add_recall(anchor,registry,1000));
 check(upkeep.count(owner,1000)==1&&upkeep.count(ally,1000)==0);
 check(upkeep.maintained_target(owner,100,1000)==owner);
 check(!upkeep.add_recall(anchor,registry,1000));
 auto bad=anchor;bad.target=owner;check(!MaintainedEnchantments{}.add_recall(bad,registry,1000));
 overcast::CeilingCurve curve(owner,1000,33);energy::Regeneration regeneration;
 vitals::Resources resources{100,100,1,50};
 const auto result=prepare_maintained_energy(upkeep,curve,owner,resources,regeneration,0,10000,0);
 check(result&&result->ended.size()==1&&result->ended[0].at==4000);
 check(result->ended[0].release.target==owner&&result->ended[0].release.effect==100);
 check(result->ownership.count(owner,10000)==0&&result->energy.resources.energy==0);
 const auto positive=prepare_maintained_energy(upkeep,curve,owner,resources,regeneration,0,4000,4);
 check(positive&&positive->ended.empty()&&positive->energy.resources.energy==4&&positive->final_pips==3);
 check(blessed_signet_gain(*blessed_signet(0),owner,upkeep,1000)==3);
}
