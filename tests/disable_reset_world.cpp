#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Disable World line "+std::to_string(at.line()));}
int main()try{world::Instance w;
 for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,100,100},100);}
 const auto caster=*w.effect_scope(1),owner=*w.effect_scope(2);const auto before_interrupt=*w.disable_reset_revision(owner);check(w.cast_activity(owner,66,100,3100));check(w.interrupt_skill(caster,owner,*effects::interrupt_skill(5,12),20,200));
 check(!w.reset_skill_disables(owner,{{62,0}},before_interrupt,200));
 check(w.skill_disabled_slot(owner,{62,0},200)==10200&&w.skill_disabled_slot(owner,{62,65535},200)==10200&&w.skill_disabled_slot(owner,{63,0},200)==0);
 check(!w.skill_disabled_slot(owner,{62,65536},200)&&!w.skill_disabled_slot({owner.instance,owner.actor,owner.generation+1},{62,0},200));
 const auto permit=*w.disable_reset_revision(owner);
 check(permit.size()==11&&permit[3]==0&&permit[6]==0&&permit[7]==0&&permit[8]==0&&permit[9]==0&&permit[10]==0);
 auto forged_rust=permit;forged_rust[3]=1;
 check(!w.reset_skill_disables(owner,{{62,1}},forged_rust,250)&&w.disable_reset_revision(owner)==permit);

 check(!w.reset_skill_disables(owner,{{62,1},{62,1}},permit,250)&&w.disable_reset_revision(owner)==permit);
 check(w.reset_skill_disables(owner,{{62,1}},permit,250));
 check((*w.disable_reset_revision(owner))[3]>0);
 check(w.skill_disabled_slot(owner,{62,1},250)==0&&w.skill_disabled_slot(owner,{62,0},250)==10200&&w.skill_disabled_slot(owner,{66,0},250)==10200);
 check(!w.reset_skill_disables(owner,{{62,0}},permit,250)&&w.skill_disabled_slot(owner,{62,0},250)==10200);
 casting::Hooks hooks;hooks.events=[](const auto&){};hooks.resources=[&](auto id){return w.resources(id);};hooks.consume_energy=[&](auto cost){return w.consume_energy(2,cost,owner);};hooks.target_scope=[&](auto id){return w.effect_scope(id);};hooks.projectile_target_valid=[](auto id,float){return id==1;};hooks.skill_disabled_slot=[&](Slot slot,auto now){return w.skill_disabled_slot(owner,slot,now);};hooks.signet_humility=[&](auto target,const auto& plan,auto now){return w.signet_humility(owner,target,plan,now);};
 Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[62]=1;Slots bar{};bar[0]={62,0};attributes::Ranks ranks{};casting::Runtime runtime(2);
 check(runtime.start(300,{62,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge&&w.resources(2)->energy==100);
 const auto reset_revision=*w.disable_reset_revision(owner);unsigned reset_packets=0;
 hooks.events=[&](const auto& packets){reset_packets+=unsigned(packets.size());};
 check(!runtime.reset_other_signets(owner,{{62,0}},400,1,[](const auto&){return false;},hooks)&&reset_packets==0);
 check(runtime.start(401,{62,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
 reset_packets=0;
 check(runtime.reset_other_signets(owner,{{62,0}},500,1,[&](const auto& keys){return w.reset_skill_disables(owner,keys,reset_revision,500);},hooks)&&reset_packets==1);
 check(!runtime.reset_other_signets(owner,{{62,0}},500,1,[](const auto&){return true;},hooks));
 check(runtime.start(501,{62,0,1},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
 runtime.interrupt(600,hooks);
 check(runtime.start(601,{62,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
 reset_packets=0;
 check(!runtime.reset_other_signets(owner,{{62,0}},650,2,[](const auto&){return false;},hooks)&&reset_packets==0);
 check(runtime.start(651,{62,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
 const auto recharge_permit=*w.disable_reset_revision(owner);
 check(runtime.reset_other_signets(owner,{{62,0}},700,2,[&](const auto& keys){return w.reset_skill_disables(owner,keys,recharge_permit,700);},hooks));
 check(runtime.start(701,{62,0,1},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
 casting::Runtime resurrection_runtime(2);Slots resurrection_bar{};resurrection_bar[0]={2,0};policy.learned[2]=1;unsigned resurrected=0;
 hooks.resurrection_target_valid=[](auto id,float){return id==1;};
 // Runtime contract test: the scoped target is a corpse; resurrection itself
 // is a stub so this does not claim World resurrection behavior coverage.
 hooks.resources=[&](auto id){auto value=w.resources(id);if(value&&id==1)value->health=0;return value;};
 hooks.resurrect=[&](auto target,const auto&,auto){if(target!=caster)return false;++resurrected;return true;};
 check(resurrection_runtime.start(2000,{2,0,1},resurrection_bar,policy,ranks,true,hooks)==casting::Reason::accepted);
 resurrection_runtime.update(6000,hooks);check(resurrected==1);
 check(resurrection_runtime.start(7000,{2,0,1},resurrection_bar,policy,ranks,true,hooks)==casting::Reason::recharge);
 const auto original_disabled=hooks.skill_disabled_slot;
 hooks.skill_disabled_slot=[&](Slot slot,auto now)->std::optional<std::uint64_t>{if(slot==Slot{2,1})return 9000;return original_disabled(slot,now);};
 reset_packets=0;bool checked_spent=false;
 check(resurrection_runtime.reset_other_signets(owner,{{2,0},{2,1}},7100,1,[&](const auto& keys){checked_spent=keys==std::vector<Slot>{{2,1}};return true;},hooks)&&checked_spent&&reset_packets==1);
 hooks.skill_disabled_slot=original_disabled;
 check(resurrection_runtime.start(7200,{2,0,1},resurrection_bar,policy,ranks,true,hooks)==casting::Reason::recharge);
 resurrection_runtime.morale_boost(hooks);
 check(resurrection_runtime.start(7300,{2,0,1},resurrection_bar,policy,ranks,true,hooks)==casting::Reason::accepted);
 w.leave(2);w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},10300,2);w.set_vitals(2,{500,500,100,100},100);
 check(!w.skill_disabled_slot(owner,{62,0},10300)&&w.skill_disabled_slot(*w.effect_scope(2),{62,0},10300)==0);
 check(!w.disable_reset_revision(owner)&&!w.reset_skill_disables(owner,{{62,0}},permit,10300));
 std::cout<<"PASS combined World full-copy disable query and gateway-style slot-only Runtime gating, expiry and actor replacement\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

