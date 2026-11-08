#include "warrior_condition_fixture.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Cooldown bar World line "+std::to_string(at.line()));}
int main()try{
 warrior_test::Fixture f(403,false);auto& w=f.world;Slots bar{};bar[0]={403,0};bar[1]={403,7};bar[2]={281,0};
 check(!w.cooldown_bar_snapshot(f.source));check(w.configure_adrenaline(f.source,bar,100));const auto first=*w.cooldown_bar_snapshot(f.source);check(first.bar==bar&&first.revision==1&&first.published.order==1);
 check(w.cast_activity(f.source,281,200,1000));const auto epoch=*w.control_epoch(f.source);check(w.interrupt_cast(f.source,281,300));const auto control=*w.cooldown_control_stamp(f.source,epoch);
 check(control.owner==f.source&&control.at==300&&control.order==2&&w.control_event_time(f.source,epoch)==300);
 bar[1]={320,0};check(w.configure_adrenaline(f.source,bar,400));const auto next=*w.cooldown_bar_snapshot(f.source);check(next.bar==bar&&next.revision==2&&next.published.order==3);
 check(w.cooldown_control_stamp(f.source,epoch)==control);check(w.configure_adrenaline(f.source,bar,400));const auto repeated=*w.cooldown_bar_snapshot(f.source);check(repeated.revision==2&&repeated.published.order==4);
 auto invalid=bar;invalid[3]=invalid[0];check(!w.configure_adrenaline(f.source,invalid,500)&&w.cooldown_bar_snapshot(f.source)==repeated);
 invalid=bar;invalid[3]={0,1};check(!w.configure_adrenaline(f.source,invalid,500)&&w.cooldown_bar_snapshot(f.source)==repeated);
 invalid=bar;invalid[3]={403,65536};check(!w.configure_adrenaline(f.source,invalid,500)&&w.cooldown_bar_snapshot(f.source)==repeated);
 check(!w.configure_adrenaline(f.source,bar,399)&&w.cooldown_bar_snapshot(f.source)==repeated);
 auto stale=f.source;++stale.generation;check(!w.cooldown_bar_snapshot(stale)&&!w.cooldown_control_stamp(stale,epoch)&&!w.configure_adrenaline(stale,bar,500));
 check(w.cooldown_order_failures(f.source)==0&&!w.cooldown_control_stamp(f.source,UINT64_MAX));
 check(w.prepare_diversion_skill_use(f.source,{403,0},7,500));check(!w.finalize_diversion_skill_use(f.source,8,500,10000,0)&&!w.cooldown_completion_stamp(f.source,8));
 check(w.finalize_diversion_skill_use(f.source,7,500,10000,0).has_value());const auto completion=*w.cooldown_completion_stamp(f.source,7);
 check(completion.stamp.owner==f.source&&completion.stamp.order==5&&completion.stamp.at==500&&completion.slot==Slot{403,0}&&completion.base_ms==10000&&completion.existing_until==0);
 effects::LocalCooldownSnapshot published{f.source,1,500,{{{403,0},10500,effects::CooldownOrigin{completion.stamp,effects::CooldownOriginKind::completion,7}}}};
 published.control_epoch=*w.control_epoch(f.source);published.completion_order=completion.stamp.order;
 check(w.publish_local_cooldowns(f.source,published)&&w.local_cooldown_snapshot(f.source)->verified==std::vector<Slot>{{403,0}});
 check(w.pending_attack_resets(f.source)->empty()&&!w.pending_attack_resets(stale));
 const auto permit=w.prepare_cooldown_checkpoint(f.source,published,500);check(permit&&permit->accepted.snapshot==published&&permit->watermark.order>completion.stamp.order);
 auto different=published;++different.at;
 check(!w.prepare_cooldown_checkpoint(f.source,different,501)&&!w.prepare_cooldown_checkpoint(stale,published,500)&&!w.prepare_cooldown_checkpoint(f.source,published,499));
 const auto next_permit=w.prepare_cooldown_checkpoint(f.source,published,500);
 check(next_permit&&next_permit->watermark.order==permit->watermark.order+1&&!w.acknowledge_attack_reset(permit->watermark));
 auto invented=published;invented.reset_order=1;
 check(!w.publish_local_cooldowns(f.source,invented)&&!w.prepare_cooldown_checkpoint(f.source,published,500));check(w.publish_local_cooldowns(f.source,published));
 check(!w.finalize_diversion_skill_use(f.source,7,500,10000,0)&&w.cooldown_completion_stamp(f.source,7)==completion);
 check(w.prepare_diversion_skill_use(f.source,{403,0},8,550)&&w.abort_diversion_skill_use(f.source,8)&&!w.cooldown_completion_stamp(f.source,8));
 check(!w.cooldown_completion_stamp(stale,7));
 for(unsigned i=0;i<65;++i){check(w.cast_activity(f.source,281,500+i,1000+i));check(w.interrupt_cast(f.source,281,600+i));}
 check(!w.cooldown_control_stamp(f.source,epoch));
 for(unsigned i=0;i<65;++i){check(w.prepare_diversion_skill_use(f.source,{403,0},9+i,700+i));check(w.finalize_diversion_skill_use(f.source,9+i,700+i,1000,0).has_value());}
 check(!w.cooldown_completion_stamp(f.source,7));
 published.at=800;check(!w.local_cooldown_snapshot(f.source)&&!w.publish_local_cooldowns(f.source,published));
 published.control_epoch=*w.control_epoch(f.source);published.completion_order=w.cooldown_completion_stamp(f.source,73)->stamp.order;
 check(w.publish_local_cooldowns(f.source,published)&&w.local_cooldown_snapshot(f.source)->verified==std::vector<Slot>{{403,0}}); // Keep a proven live timer once all newer source work is observed.
 check(w.prepare_cooldown_checkpoint(f.source,published,800).has_value());
 auto opaque=published;++opaque.revision;opaque.timers.front().origin.reset();
 check(w.publish_local_cooldowns(f.source,opaque)&&!w.prepare_cooldown_checkpoint(f.source,opaque,800));

 const auto old=f.source;w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);check(*w.effect_scope(1)!=old&&!w.cooldown_bar_snapshot(*w.effect_scope(1))&&!w.cooldown_control_stamp(old,0));
 // A control sampled before another thread publishes a bar retains provenance.
 warrior_test::Fixture g(403,false);check(g.world.configure_adrenaline(g.source,bar,1000));check(g.world.cast_activity(g.source,281,100,900));const auto before=*g.world.control_epoch(g.source);check(g.world.interrupt_cast(g.source,281,500));check(g.world.control_epoch(g.source)==before+1&&g.world.cooldown_control_stamp(g.source,before).has_value()&&g.world.cooldown_order_failures(g.source)==0);
 effects::LocalCooldownSnapshot conflicting{g.source,1,1000,{}};conflicting.control_epoch=before+1;
 check(g.world.publish_local_cooldowns(g.source,conflicting)&&g.world.local_cooldown_snapshot(g.source).has_value());
 // An instant completion resets the exact ordinary slot, preserving the
 // original inputs for auditing and a lifetime barrier against delayed work.
 warrior_test::Fixture reset(403,false);auto& rw=reset.world;const auto ro=reset.source;
 check(rw.prepare_diversion_skill_use(ro,{403,0},1,200));
 check(rw.finalize_diversion_skill_use(ro,1,200,10000,0).has_value());
 const auto prior=*rw.cooldown_completion_stamp(ro,1);
 check(rw.prepare_diversion_skill_use(ro,{403,7},2,210));
 check(rw.finalize_diversion_skill_use(ro,2,210,10000,0).has_value());
 const auto copy=*rw.cooldown_completion_stamp(ro,2);
 check(rw.prepare_diversion_skill_use(ro,{403,0},3,300));
 check(rw.finalize_diversion_skill_use(ro,3,300,10000,10200,true).has_value());
 const auto instant=*rw.cooldown_completion_stamp(ro,3);
 check(instant.instant_recharge&&instant.base_ms==10000&&instant.existing_until==10200&&effects::completion_deadline(instant)==300);
 effects::LocalCooldownSnapshot cleared{ro,1,300,{{{403,7},10210,effects::CooldownOrigin{copy.stamp,effects::CooldownOriginKind::completion,2}}}};
 cleared.completion_order=instant.stamp.order;
 check(rw.publish_local_cooldowns(ro,cleared)); // Other copy is unaffected.
 auto stale_reset=cleared;++stale_reset.revision;
 stale_reset.timers.push_back({{403,0},10200,effects::CooldownOrigin{prior.stamp,effects::CooldownOriginKind::completion,1}});
 check(!rw.publish_local_cooldowns(ro,stale_reset));
 stale_reset.timers.back()={{403,0},10300,effects::CooldownOrigin{instant.stamp,effects::CooldownOriginKind::completion,3}};
 check(!rw.publish_local_cooldowns(ro,stale_reset)); // No timer may be invented from the reset receipt.
 stale_reset.timers.back().origin.reset();check(!rw.publish_local_cooldowns(ro,stale_reset));
 for(unsigned i=0;i<65;++i){check(rw.prepare_diversion_skill_use(ro,{281,0},4+i,400+i));check(rw.finalize_diversion_skill_use(ro,4+i,400+i,0,0).has_value());}
 check(!rw.cooldown_completion_stamp(ro,3)); // The reset barrier outlives bounded receipts.
 stale_reset.at=500;stale_reset.completion_order=rw.cooldown_completion_stamp(ro,68)->stamp.order;
 check(!rw.publish_local_cooldowns(ro,stale_reset));
 check(rw.prepare_diversion_skill_use(ro,{403,0},69,600));check(rw.finalize_diversion_skill_use(ro,69,600,1000,0).has_value());
 const auto newer=*rw.cooldown_completion_stamp(ro,69);
 effects::LocalCooldownSnapshot resumed{ro,3,600,{{{403,0},1600,effects::CooldownOrigin{newer.stamp,effects::CooldownOriginKind::completion,69}}}};
 resumed.completion_order=newer.stamp.order;check(rw.publish_local_cooldowns(ro,resumed));
 std::cout<<"PASS World bar/copy revision and control ordering,atomic invalid publication,scope reuse,bounded provenance history,instant reset barriers and delayed control provenance\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
