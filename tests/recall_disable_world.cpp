#include "gw_world_simulation.h"
#include <stdexcept>
using namespace fw::gw;
void check(bool ok){if(!ok)throw std::runtime_error("Recall World disable");}
int main(){
 world::Instance w;w.join(1,"Owner",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
 w.set_vitals(1,{1000,1000,50,100},100);const auto owner=*w.effect_scope(1);
 skills::Slots bar{};bar[0]={925,0};bar[1]={74,0};bar[2]={293,0};bar[3]={281,1};
 check(w.configure_adrenaline(owner,bar,100));const auto before=*w.disable_reset_revision(owner);
 check(w.recall_end_disable(owner,1,200));
 check(w.skill_disabled_until(owner,74,200)==10200&&w.skill_disabled_slot(owner,{293,0},200)==10200);
 check(w.skill_disabled_slot(owner,{281,1},200)==10200&&w.skill_disabled_slot(owner,{281,0},200)==0);
 check(!w.reset_skill_disables(owner,{{293,0}},before,200));
 const auto current=*w.disable_reset_revision(owner);check(current!=before);
 check(w.reset_skill_disables(owner,{{293,0}},current,200));
 check(w.skill_disabled_slot(owner,{293,0},200)==0&&w.skill_disabled_slot(owner,{74,0},200)==10200);
 check(!w.recall_end_disable(owner,1,201)); // Reset cannot permit a replay.
 check(w.skill_disabled_slot(owner,{74,0},10200)==0);
 w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
 check(!w.skill_disabled_slot(owner,{74,0},10200));
 check(w.skill_disabled_slot(*w.effect_scope(1),{74,0},10200)==0);
}
