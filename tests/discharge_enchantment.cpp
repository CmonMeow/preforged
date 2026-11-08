#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Discharge line "+std::to_string(at.line()));}
struct Fixture{world::Instance w;Scope a,b;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(){for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:60,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,1000,30,30},100);}a=*w.effect_scope(1);b=*w.effect_scope(2);}
 void enchant(){check(w.status_mutation(b,241,1,12,100,60100,true));check(w.status_mutation(b,242,2,12,100,60100,true));}
 void hex(std::uint64_t end=60100){check(w.status_mutation(b,28,3,12,100,end,true));}
};
int main()try{
 check(enchantment_removal(1347,0)->hexed_recharge_percent==20&&enchantment_removal(1347,12)->hexed_recharge_percent==44&&enchantment_removal(1347,20)->hexed_recharge_percent==60);
 check(casting::Runtime::effect_supported(1347)&&!enchantment_removal(1347,21));
 for(bool hexed:{false,true})for(bool enchanted:{false,true}){Fixture f;if(hexed)f.hex();if(enchanted)f.enchant();f.box->drain();EnchantmentRemovalOutcome result;
  auto plan=*enchantment_removal(1347,12);auto bad=plan;++bad.hexed_recharge_percent;result.target_hexed=true;check(!f.w.enchantment_removal(f.a,f.b,bad,20,100,&result)&&!result.target_hexed);
  check(f.w.enchantment_removal(f.a,f.b,plan,20,100,&result)&&result.target_hexed==hexed);
  check(f.w.status_snapshot(f.b,100)->size()==unsigned(hexed)+unsigned(enchanted));check(f.w.resources(1)->health==500&&f.w.resources(1)->energy==30&&f.w.resources(2)->health==500);
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(21,1,1604))==1);
 }
 // Intrinsic factor joins ordinary factors before one final rounding,with
 // the documented single-effect exception and separate PvE Fast Casting.
 struct TimingCase{unsigned rank,fast;bool pvp,mantra;unsigned seconds;};
 // Fixed expected values exercise caps and single final rounding,without
 // rebuilding the production formula in the test.
 for(const auto c:std::array<TimingCase,16>{{
  {0,0,false,false,12},{6,0,false,false,10},{12,0,false,false,8},
  {15,0,false,false,8},{20,0,false,false,6},{0,12,false,false,8},
  {6,12,false,false,7},{12,12,false,false,5},{15,12,false,false,5},
  {20,12,false,false,4},{12,12,true,false,8},{0,0,false,true,8},
  {12,0,false,true,8},{20,0,false,true,6},{0,5,false,true,7},{12,5,false,true,6}}}){
  std::vector<Entry> active;if(c.mantra)active.push_back({1,1,13,12,Category::stance,100,60100});
  const auto factor=1.-enchantment_removal(1347,c.rank)->hexed_recharge_percent/100.;
  check(timing::recharge(*find_skill(1347),5,c.fast,c.pvp,active,100,15,factor)==c.seconds);
 }
 check(!timing::recharge(*find_skill(1347),5,0,false,{},100,15,0));
 for(unsigned mode=0;mode<8;++mode){Fixture f;if(mode!=7)f.enchant();if(mode==1)f.hex(1100);casting::Runtime cast(1);casting::Hooks h;std::vector<Packet> packets;
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};h.resources=[&](auto id){return f.w.resources(id);};h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.a,at);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return f.w.projectile_target_valid(f.a,id,range,100);};
  h.enchantment_removal_result=[&](auto target,const auto& plan,auto at)->std::optional<EnchantmentRemovalOutcome>{EnchantmentRemovalOutcome result;if(!f.w.enchantment_removal(f.a,target,plan,20,at,&result))return {};return result;};
  // A legacy bool callback alone cannot silently discard the recharge fact.
  h.enchantment_removal=[&](auto target,const auto& plan,auto at){return f.w.enchantment_removal(f.a,target,plan,20,at);};
  Policy p;p.primary=5;p.campaigns=255;p.learned[1347]=1;Slots bar{};bar[0]={1347,0};attributes::Ranks ranks{};ranks[3]=12;if(mode==6)h.enchantment_removal_result={};
  check(cast.start(100,{1347,0,2},bar,p,ranks,true,h)==(mode==6?casting::Reason::unsupported:casting::Reason::accepted));if(mode==6)continue;
  if(mode==0||mode==7)f.hex();if(mode==2)cast.cancel(h);if(mode==3)cast.interrupt(500,h);if(mode==4){f.w.leave(2);f.w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(2,{500,1000,30,30},100);}
  cast.update(1100,h);check(f.w.resources(1)->energy==20);
  if(mode==0||mode==1||mode==5||mode==7){const auto seconds=(mode==0||mode==7)?8u:15u;check(std::find(packets.begin(),packets.end(),casting::slot_event(228,1,{1347,0},seconds))!=packets.end());
   check(cast.start(1100+seconds*1000-1,{1347,0,2},bar,p,ranks,true,h)==casting::Reason::recharge);
   check(cast.start(1100+seconds*1000,{1347,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);
  }
 }
 std::cout<<"PASS Discharge atomic hex outcome,conditional recharge/no enchantment,stacking/rounding,completion expiry/control and caster1604\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
