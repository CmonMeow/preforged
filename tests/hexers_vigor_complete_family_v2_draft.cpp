#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
#include <fstream>
#include <filesystem>
using namespace fw::gw;using namespace skills;
static void check(bool good,std::source_location at=std::source_location::current()){
 if(!good)throw std::runtime_error("Hexer's Vigor line "+std::to_string(at.line()));
}
struct Fixture {
 std::unique_ptr<world::Instance> store=std::make_unique<world::Instance>();world::Instance& w=*store;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope source,target;
 casting::Runtime cast{1};casting::Hooks h;Slots bar{};Policy policy;attributes::Ranks ranks{};
 inventory::Profile gear;std::uint64_t clock=100;unsigned paid=0,completed=0;
 Fixture(unsigned rank=0,bool extended=false){
  if(extended){gear.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item item;item.definition.id=100;item.definition.type=26;item.definition.quantity=1;item.definition.interaction=0x200000;item.definition.modifiers={0x22b80014u};item.bag=1;gear.items.emplace(100,item);gear.weapons.sets[0].weapon=100;}
  w.join(1,"Vigor caster",0,0,box,0,{}, {},100,1);w.join(2,"Vigor foe",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{100,1000,100,100},100);w.set_vitals(2,{1000,1000,100,100},100);
  source=*w.effect_scope(1);target=*w.effect_scope(2);
  bar[0]={2138,0};bar[1]={1,0};bar[2]={99,0};bar[3]={2138,1};bar[4]={322,0};check(w.configure_adrenaline(source,bar,100));
  policy.primary=4;policy.secondary=1;policy.campaigns=255;for(unsigned id:{2138u,1u,99u,322u})policy.learned[id]=1;ranks[6]=rank;
  h.resources=[&](auto id){return w.resources(id);};h.events=[&](const auto& p){box->enqueue(p);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.target_valid=[&](auto id,float range,bool self){return w.valid_heal_target(1,id,range,self);};
  h.projectile_target_valid=[&](auto id,float){return id==target.actor;};
  h.consume_energy_at=[&](auto cost,auto at){paid+=cost;return w.consume_energy(1,cost,source,at);};
  h.effect_snapshot=[&](auto at){return w.status_snapshot(source,at);};
  h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(source,skill,event,at);};
  h.enchantment_duration_ready=[&](auto skill,auto base,auto at){return bool(quote(skill,base,at));};
  h.simple_enchantment_completed=[&](auto owner,auto skill,auto rank,unsigned,auto event,auto at)->casting::EffectCompletion{
   if(skill!=2138||owner!=source)return {};
   const auto duration=quote(skill,10000,at);casting::EffectCompletion result;std::uint64_t completed_at=0;
   result.accepted=duration&&w.hexers_vigor_enchantment(owner,rank,at,event,&*duration,&completed_at,&result.publication);
   if(result.accepted)result.effect_snapshot_at=completed_at;return result;
  };
  h.completed_action=[&](auto skill,auto at){++completed;return w.completed_skill_action(source,skill,at);};
  h.begin_action=[&](auto slot,auto at){check(w.accepted_skill_start(source,slot,at));};
  h.healing_signet_activity=[&](auto at,auto until){return w.healing_signet_activity(source,at,until);};
  h.heal=[&](auto id,auto amount,float range,bool self){return w.heal_target(1,id,amount,range,self,clock);};
  h.weapon_snapshot=[&](){return w.attack_weapon(source);};
  h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(source,at);};
  h.attack_duration=[&](auto base,auto at){return w.attack_duration(source,base,at);};
  h.melee_target=[&](auto id,auto at){return w.melee_target(source,id,at);};
  h.attack_activity=[&](auto until){w.attack_activity(source,until,clock);};
  h.attack_interrupt_epoch=[&](){return w.attack_interrupt_epoch(source);};
  h.control_epoch=[&](){return w.control_epoch(source);};h.control_event_time=[&](auto epoch){return w.control_event_time(source,epoch);};
  h.cast_slot_activity=[&](auto slot,auto begins,auto ends){w.cast_activity(source,slot,begins,ends);};
  h.melee_attack=[&](auto to,const auto& gear,const auto& plan,auto at){return w.melee_attack(source,to,gear,plan,at);};
  check(w.physical_armor(target,60));check(w.ordinary_weapon(source,combat::weapon::Owned{1,1,{20,20,2},0,false,false,27},20));
  h.degeneration_hex=[&](auto to,const auto& plan,auto at)->casting::EffectCompletion{return w.degeneration_hex(source,to,plan,at);};
  box->drain();
 }
 std::optional<effects::EnchantmentDuration> quote(unsigned skill,std::uint64_t base,std::uint64_t at){
  auto equip=items::weapon_passives::equipped_enchanting(gear,1);auto active=w.status_snapshot(source,at);
  return equip&&active?effects::enchanting_duration(skill,base,source,*equip,*active,at):std::nullopt;
 }
 casting::Reason start(unsigned id,std::uint64_t at,unsigned copy=0){clock=at;return cast.start(at,{id,copy,(id==99||id==322)?2u:999u},bar,policy,ranks,true,h);}
 void update(std::uint64_t at){clock=at;cast.update(at,h);}
 bool active(std::uint64_t at){auto a=w.status_snapshot(source,at);return a&&std::any_of(a->begin(),a->end(),[](const auto& e){return e.skill==2138;});}
 effects::Entry entry(std::uint64_t at){const auto a=*w.status_snapshot(source,at);auto i=std::find_if(a.begin(),a.end(),[](const auto& e){return e.skill==2138;});check(i!=a.end());return *i;}
 void grant(){check(start(2138,1000)==casting::Reason::accepted);check(paid==5&&w.resources(1)->energy==95);update(1249);check(!active(1249));box->drain();update(1250);check(active(1250)&&completed==1);}
};
int main(int argc,char** argv)try{
 for(unsigned rank=0;rank<=20;++rank){auto p=regeneration::reviewed(2138,rank);check(p&&p->duration==10000&&p->self_only&&p->pips==int(std::lround(1.+7.*rank/15.)));}
 check(!regeneration::reviewed(2138,21));
 for(unsigned mode=0;mode<5;++mode){Fixture f;
  if(mode==0)f.h.simple_enchantment_completed={};if(mode==1)f.h.before_spell_effect={};if(mode==2)f.h.completed_action={};if(mode==3)f.h.enchantment_duration_ready={};if(mode==4)f.h.target_scope={};
  check(f.start(2138,1000)==casting::Reason::unsupported);check(f.paid==0&&f.w.resources(1)->energy==100);
 }
 for(unsigned mode=0;mode<5;++mode){Fixture f;check(f.start(2138,1000)==casting::Reason::accepted);
  if(mode==0)f.h.simple_enchantment_completed={};if(mode==1)f.h.before_spell_effect={};if(mode==2)f.h.completed_action={};if(mode==3)f.h.enchantment_duration_ready={};if(mode==4)f.h.target_scope={};
  f.update(1250);check(!f.active(1250)&&f.paid==5&&f.completed==0);
 }
 for(bool interrupt:{false,true}){Fixture f;check(f.start(2138,1000)==casting::Reason::accepted);f.clock=1100;if(interrupt)f.cast.interrupt(1100,f.h);else f.cast.cancel(f.h,1100);f.update(1250);check(!f.active(1250)&&f.paid==5&&f.completed==0);}
 for(unsigned rank:{0u,15u}){Fixture f(rank);f.grant();auto p=f.box->drain();
  auto recharge=std::find(p.begin(),p.end(),casting::slot_event(228,1,{2138,0},10));auto finish=std::find(p.begin(),p.end(),casting::terminal(58,1));auto body=std::find(p.begin(),p.end(),casting::generic(21,1,1814));
  auto buff=std::find_if(p.begin(),p.end(),[](const auto& x){return x.size()==20&&x[0]==65;});
  check(recharge!=p.end()&&finish!=p.end()&&body!=p.end()&&buff!=p.end()&&recharge<finish&&finish<body&&body<buff);
  check(std::count(p.begin(),p.end(),casting::generic(21,1,1814))==1);check(f.entry(1250).expires==11250);
  f.w.tick(11250);check(!f.active(11250)&&f.w.resources(1)->health==(rank?260u:120u));f.w.tick(12250);check(f.w.resources(1)->health==(rank?260u:120u));
 }
 for(bool interrupted:{false,true}){Fixture f;f.grant();check(f.start(1,2250)==casting::Reason::accepted);check(f.active(2250));
  if(interrupted){f.clock=2500;f.cast.interrupt(2500,f.h);check(f.active(2500));f.w.tick(3250);check(f.active(3250)&&f.w.resources(1)->health==104);}
  else{f.update(4249);check(f.active(4249));f.update(4250);check(!f.active(4250)&&f.completed==2);auto hp=f.w.resources(1)->health;check(hp==188);f.w.tick(5250);check(f.w.resources(1)->health==hp);}
 }
 {Fixture f;f.grant();check(f.start(99,2250)==casting::Reason::accepted);const auto due=2250+std::uint64_t(std::llround(find_skill(99)->activation*1000.));f.update(due);check(f.active(due)&&f.completed==2);const auto target=*f.w.status_snapshot(f.target,due);check(std::any_of(target.begin(),target.end(),[](const auto& e){return e.skill==99;}));}
 {Fixture f;f.grant();check(f.w.physical_armor(f.target,60));combat::weapon::Owned sword{1,1,{20,20,2},0,false,false,27};check(f.w.ordinary_weapon(f.source,sword,20));check(f.w.ordinary_attack(f.source,2,2250));f.w.tick(2750);check(f.active(2750)&&f.w.resources(2)->health==980);}
 {Fixture f;f.grant();const auto e=f.entry(1250);check(f.w.status_mutation(f.source,2138,e.instance,0,2250,{},false));check(!f.active(2250));check(f.w.resources(1)->health==102);f.w.tick(3250);check(f.w.resources(1)->health==102);}
 {Fixture f;check(f.w.hexers_vigor_enchantment(f.source,0,1000));check(f.w.hexers_vigor_enchantment(f.source,15,2000));f.w.tick(12000);check(f.w.resources(1)->health==262&&!f.active(12000));}
 {Fixture f;auto stale=f.source;++stale.generation;check(!f.w.hexers_vigor_enchantment(stale,0,1000));check(!f.w.hexers_vigor_enchantment(f.source,21,1000));check(f.w.before_spell_effect(f.source,2138,1,1000));std::function<void()> publication;check(f.w.hexers_vigor_enchantment(f.source,0,1000,1,nullptr,nullptr,&publication));check(bool(publication)&&f.box->drain().empty());publication();check(!f.box->drain().empty());publication();check(f.box->drain().empty());check(!f.w.hexers_vigor_enchantment(f.source,0,1000,1));}
 {Fixture f(0,true);f.grant();check(f.entry(1250).expires==13250);f.w.tick(13250);check(!f.active(13250)&&f.w.resources(1)->health==124);}
 {Fixture f;f.grant();const auto old=f.source;f.w.leave(1);f.w.join(1,"New Vigor caster",0,0,f.box,0,{}, {},3000,1);f.w.set_vitals(1,{100,1000,100,100},100);const auto fresh=*f.w.effect_scope(1);check(fresh!=old);check(!f.w.hexers_vigor_enchantment(old,0,3000));f.w.tick(4000);check(f.w.resources(1)->health==100);const auto status=*f.w.status_snapshot(fresh,4000);check(std::none_of(status.begin(),status.end(),[](const auto& e){return e.skill==2138;}));}
 {Fixture f;f.grant();const auto old=f.entry(1250);check(f.start(2138,2250,1)==casting::Reason::accepted);f.update(2500);const auto fresh=f.entry(2500);check(fresh.instance!=old.instance&&fresh.expires==12500&&f.completed==2);f.w.tick(11250);check(f.active(11250));}
 for(bool interrupted:{false,true}){Fixture f(0,true);f.grant();const auto old=f.entry(1250);check(old.expires==13250);check(f.start(2138,11250)==casting::Reason::accepted);check(f.entry(11250).instance==old.instance);
  if(interrupted){f.clock=11300;f.cast.interrupt(11300,f.h);check(f.entry(11300).instance==old.instance&&f.entry(11300).expires==13250);}
  else{f.update(11500);const auto fresh=f.entry(11500);check(fresh.instance!=old.instance&&fresh.expires==23500&&f.completed==2);}
 }
 {Fixture f;f.grant();f.clock=2250;check(f.cast.start(2250,{322,0,999},f.bar,f.policy,f.ranks,true,f.h)==casting::Reason::target);check(f.active(2250)&&f.paid==5&&f.completed==1);}
 {Fixture f;f.grant();check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,2000,5000));f.w.configure_attack_miss_draws([]{return 0.;});check(f.start(322,2250)==casting::Reason::accepted);f.update(2749);check(f.active(2749));f.update(2750);check(!f.active(2750)&&f.completed==2&&f.w.resources(2)->health==1000);const auto outcomes=f.w.take_attack_outcomes(f.source);check(outcomes.size()==1&&outcomes.front().outcome==combat::weapon::AttackOutcome::miss);}
 {Fixture f;f.grant();check(!f.w.completed_skill_action(f.source,0,2250));check(f.active(2250));}
 {Fixture f;check(f.start(2138,1000)==casting::Reason::accepted);check(f.w.gale(f.target,f.source,12,.5,1100));f.update(4000);check(!f.active(4000)&&f.paid==5&&f.completed==0);}
 if(argc==2){Fixture f;std::vector<std::pair<std::uint64_t,Packet>> rows;const auto collect=[&](auto at){for(auto& packet:f.box->drain())rows.emplace_back(at,std::move(packet));};
  check(f.start(2138,1000)==casting::Reason::accepted);collect(1000);f.update(1250);collect(1250);f.w.tick(2250);collect(2250);check(f.active(2250)&&f.w.resources(1)->health==102);
  const std::filesystem::path path=argv[1];if(path.has_parent_path())std::filesystem::create_directories(path.parent_path());std::ofstream out(path);check(bool(out));out<<"{\"scope\":\"actual World/Runtime synthetic actors, no render claim\",\"health_at_2250\":102,\"packets\":[";bool first=true;constexpr char hex[]="0123456789abcdef";
  for(const auto& [at,packet]:rows){if(!first)out<<',';first=false;out<<"{\"at\":"<<at<<",\"hex\":\"";for(auto byte:packet)out<<hex[(byte>>4)&15]<<hex[byte&15];out<<"\"}";}out<<"]}\n";check(bool(out));
 }
 std::cout<<"PASS Hexer's Vigor actual paid Runtime/World, regen/expiry/strip/refresh, nonhex success versus interrupt, hex/ordinary preservation, scope/replay and publication\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
