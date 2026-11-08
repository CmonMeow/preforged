// NEW unregistered follow-on. Requires complete detached source package.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <fstream>
#include <filesystem>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Cyclone targetless actual World line "+std::to_string(at.line()));}
// Same known-good synthetic nav container shape as bundle_world.cpp (kind0).
void nav_fixture(const std::filesystem::path& path){
 std::filesystem::create_directories(path.parent_path());std::ofstream out(path,std::ios::binary);check(bool(out));
 const auto put=[&](std::uint32_t value){for(unsigned i=0;i<4;++i)out.put(char(value>>(8*i)));};
 const auto number=[&](float value){put(std::bit_cast<std::uint32_t>(value));};
 out.write("FWNAV001",8);for(auto value:{1u,7u,0xeefe704cu,12u,1u,1u})put(value);
 number(-2000);number(-2000);number(2000);number(2000);put(2);
 const auto cell=[&]{for(unsigned i=0;i<5;++i)put(0xffffffff);number(-2000);number(2000);number(-2000);number(2000);number(-2000);number(2000);};
 put(1);cell();put(1);cell();out.close();check(bool(out));
}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope actor,hexer;
 combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,2,0.};combat::weapon::AttackPlan plan=*combat::weapon::attack_skill(330,0);
 Fixture(unsigned foes,unsigned hp=500,std::shared_ptr<const navigation::Mesh> mesh={}){
  if(mesh)w.configure_navigation(mesh);
  w.join(1,"Cyclone owner",0,0,box,0,{}, {},100,1);w.set_vitals(1,{hp,1000,100,100},100);actor=*w.effect_scope(1);
  w.join(3,"Hex caster",-1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);hexer=*w.effect_scope(3);
  for(unsigned i=0;i<foes;++i){const unsigned id=i?4:2;w.join(id,"Adjacent foe",60+40.f*i,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(id,{1000,1000,100,100},100);const auto target=*w.effect_scope(id);check(w.physical_armor(target,60));check(w.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{3,0,20}));}
  check(w.ordinary_weapon(actor,weapon,20,100,combat::weapon::CriticalProfile{1,0,20}));w.configure_attack_miss_draws([]{return .5;});w.configure_attack_critical_draws([]{return .999999;});box->drain();
 }
 void hex(){check(w.empathy(hexer,actor,26,0,20,100));}
 std::pair<combat::weapon::AttackPermit,std::uint64_t> prepare(){const auto duration=w.attack_duration(actor,plan.windup_ms,1000,true);check(duration&&*duration);const auto at=1000+*duration;auto permit=w.prepare_cyclone_cast_followon(actor,weapon,plan,1000,at);check(bool(permit));return {permit,at};}
};
int main(int argc,char** argv)try{
 check(argc==2);nav_fixture(argv[1]);const auto mesh=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(argv[1]));
 for(unsigned foes:{0u,1u,2u})for(bool vigor:{false,true})for(bool lethal:{false,true}){
  Fixture f(foes,lethal?10:500);f.hex();if(vigor)check(f.w.complete_vigorous_spirit(f.actor,f.actor,*vigorous_spirit(12),0,100));
  auto [permit,at]=f.prepare();f.box->drain();auto result=f.w.complete_cyclone_cast_followon(f.actor,f.weapon,f.plan,permit,at);check(result.accepted);if(result.publication)result.publication();
  check(f.w.resources(1)->health==(lethal?0u:500u-10u+(vigor?vigorous_spirit(12)->healing:0u)));
  check(result.contacts.size()==foes);for(const auto& hit:result.contacts){check(hit.target.actor==2||hit.target.actor==4);check(hit.outcome==(lethal?combat::weapon::AttackOutcome::cancelled:combat::weapon::AttackOutcome::hit));}
  if(foes){check((f.w.resources(2)->health==1000)==lethal);if(foes==2)check((f.w.resources(4)->health==1000)==lethal);}
  const auto packetBatch=f.box->drain();if(!foes){check(f.w.take_attack_outcomes(f.actor).empty());for(const auto& packet:packetBatch)if(packet.size()>=6&&packet[0]==159&&packet[1]==0){const unsigned stage=packet[2]|(unsigned(packet[3])<<8)|(unsigned(packet[4])<<16)|(unsigned(packet[5])<<24);check(stage!=38&&stage!=47);}}
  const auto hp=f.w.resources(1)->health;auto replay=f.w.complete_cyclone_cast_followon(f.actor,f.weapon,f.plan,permit,at);check(!replay.accepted&&replay.contacts.empty()&&!replay.publication&&f.w.resources(1)->health==hp&&f.box->drain().empty());
 }
 // Other common source observers remain once, even when the attack has no foe.
 for(unsigned foes:{0u,2u}){Fixture f(foes);f.hex();check(f.w.spiteful_spirit(f.hexer,f.actor,*spiteful_spirit(0),20,100));check(f.w.complete_vigorous_spirit(f.actor,f.actor,*vigorous_spirit(12),0,100));
  check(f.w.dodge_this(f.actor,*dodge_this(0),100));const auto haste=*hekets_rampage(0);check(f.w.status_mutation(f.actor,1728,200,0,100,100+haste.duration,true));
  auto [permit,at]=f.prepare();auto result=f.w.complete_cyclone_cast_followon(f.actor,f.weapon,f.plan,permit,at);check(result.accepted);if(result.publication)result.publication();
  check(f.w.resources(1)->health==500-10+vigorous_spirit(12)->healing-spiteful_spirit(0)->damage);
  const auto status=f.w.status_snapshot(f.actor,at);check(status&&std::none_of(status->begin(),status->end(),[](const auto&e){return e.skill==1728||e.skill==2354;}));
 }
 // PvP Warding ends on the made attack even with no recipient hit.
 {Fixture f(0),baseline(0);const auto ward=*weapon_spell(2893,12,1,0);
  for(auto* fixture:{&f,&baseline}){fixture->hex();check(fixture->w.apply_weapon_spell(fixture->actor,fixture->actor,ward,100));}
  auto [permit,at]=f.prepare();auto [baseline_permit,baseline_at]=baseline.prepare();check(at==baseline_at);
  // Warding regenerates Health before release. Advance an independently owned
  // World through the same preparation/completion clocks without making attack.
  baseline.w.tick(at);const auto before_attack=baseline.w.resources(1)->health;
  const auto baseline_status=baseline.w.status_snapshot(baseline.actor,at);
  check(baseline_status&&std::any_of(baseline_status->begin(),baseline_status->end(),[](const auto&e){return e.skill==2893;}));
  auto result=f.w.complete_cyclone_cast_followon(f.actor,f.weapon,f.plan,permit,at);check(result.accepted&&result.contacts.empty());if(result.publication)result.publication();
  const auto status=f.w.status_snapshot(f.actor,at);
  const bool ended=status&&std::none_of(status->begin(),status->end(),[](const auto&e){return e.skill==2893;});
  const auto after_attack=f.w.resources(1)->health;
  if(!ended||after_attack!=before_attack-10)std::cerr<<"Warding ended="<<ended<<" baselineHP="<<before_attack<<" attackHP="<<after_attack<<" at="<<at<<'\n';
  check(ended);check(after_attack==before_attack-10);}
 // Permit is exact source generation/control and one-shot completion authority.
 {Fixture f(0);f.hex();auto [permit,at]=f.prepare();permit->cancel();check(!f.w.complete_cyclone_cast_followon(f.actor,f.weapon,f.plan,permit,at).accepted);check(f.w.resources(1)->health==500);}
 {Fixture f(0);f.hex();auto [permit,at]=f.prepare();check(f.w.gale(f.hexer,f.actor,12,.5,at-1));check(!f.w.complete_cyclone_cast_followon(f.actor,f.weapon,f.plan,permit,at).accepted);check(f.w.resources(1)->health==500);}
 // Actual equipment mutation, including same-revision damage metadata, must
 // refuse BEFORE claiming the stale caller-supplied equipment capability.
 for(bool revision:{false,true}){Fixture f(0);f.hex();auto [permit,at]=f.prepare();auto changed=f.weapon;
  if(revision)++changed.revision;else changed.damage.minimum=changed.damage.maximum=21;
  check(f.w.ordinary_weapon(f.actor,changed,20,at-1,combat::weapon::CriticalProfile{1,0,20}));
  check(!f.w.complete_cyclone_cast_followon(f.actor,f.weapon,f.plan,permit,at).accepted);check(f.w.resources(1)->health==500&&f.w.take_attack_outcomes(f.actor).empty());}
 // A held bundle preserves ordinary weapon facts, so bundle is an independent
 // completion admission fact, not an inferred equipment epoch change.
 {Fixture f(0,500,mesh);f.hex();auto [permit,at]=f.prepare();items::Definition item;item.id=42;item.file=1234;item.type=6;item.model=456;item.encoded_name=u"Bundle";
  const auto wire=f.w.reserve_ground_item_id();check(bool(wire));const auto floor=f.w.drop_ground_item(f.actor,item,1,*wire,at-2,true,[](const auto&){return true;});check(bool(floor));
  check(f.w.pickup_ground_bundle(f.actor,floor->scope,1,bundles::Disposition::destroy,200,at-1));check(bool(f.w.held_bundle(f.actor)));
  check(!f.w.complete_cyclone_cast_followon(f.actor,f.weapon,f.plan,permit,at).accepted);check(f.w.resources(1)->health==500&&f.w.take_attack_outcomes(f.actor).empty());}
 // Real departure/rejoin of the numeric ID creates another generation; the old
 // source capability cannot authorize that new actor or emit a targetless hit.
 {Fixture f(0);f.hex();auto [permit,at]=f.prepare();const auto stale=f.actor;f.w.leave(1);
  f.w.join(1,"Rejoined owner",0,0,std::make_shared<world::Outbox>(),0,{}, {},at,1);f.w.set_vitals(1,{500,1000,100,100},100);const auto current=*f.w.effect_scope(1);check(current!=stale);
  check(!f.w.complete_cyclone_cast_followon(stale,f.weapon,f.plan,permit,at).accepted);check(f.w.resources(1)->health==500);}
 std::cout<<"PASS targetless actual Cyclone owned completion zero/one/many, once observers, lethal ordering and replay\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
