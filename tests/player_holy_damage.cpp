#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include "gw_player_holy_equipment.h"
#include "gw_basic_armor_energy.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Player holy damage line "+std::to_string(at.line())+context);
}
int main()try{
 std::array<CharacterProfile::EquipmentAppearance,5> visual{};for(auto& piece:visual)piece.composite=1;
 auto gear=energy::private_starter_armor(4,visual,u"Reviewed human");check(equipment::reviewed_player_holy_bonus(gear,4)==0);
 gear.items.begin()->second.definition.modifiers.push_back(0x253003cc);check(equipment::reviewed_player_holy_bonus(gear,4)==0);
 auto opaque=gear;opaque.items.begin()->second.definition.modifiers.push_back(0x2530ffff);check(!equipment::reviewed_player_holy_bonus(opaque,4));
 auto captured=gear;captured.items.begin()->second.reviewed_armor.reset();check(!equipment::reviewed_player_holy_bonus(captured,4));
 check(equipment::reviewed_player_holy_bonus(inventory::baseline(),4)==0);
 for(unsigned id:{252u,296u,312u})for(unsigned rank:{0u,12u})for(unsigned mode=0;mode<13;++mode){
  context=" id="+std::to_string(id)+" rank="+std::to_string(rank)+" mode="+std::to_string(mode);world::Instance w;
  w.join(1,"Monk",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Human",mode==6?1500.f:100.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,mode==5?1:2);w.set_vitals(2,{mode==9?5u:1000u,1000,100,100},100);
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);auto plan=*effects::holy_damage(id,rank);
  if(mode!=0)check(w.player_holy_equipment(target,mode==2?6u:0u));
  if(mode==3)check(w.protective_enchantment(target,target,*regeneration::reviewed(299,20),0,100));
  if(mode==4)check(w.gale(owner,target,12,{},100));if(mode==8)check(w.player_holy_equipment(target,{}));
  if(mode==10){combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};check(w.physical_armor(owner,60)&&w.ordinary_weapon(target,sword,20)&&w.ordinary_attack(target,1,100));}
  if(mode==11)check(w.settle_resources(target,500));if(mode==12)++plan.damage;
  auto selected=target;if(mode==7)++selected.generation;std::vector<world::Packet> visuals;
  const bool accepted=mode!=0&&mode!=5&&mode!=6&&mode!=7&&mode!=8&&mode!=11&&mode!=12;check(w.holy_damage(owner,selected,plan,20,100,&visuals)==accepted);
  const double reduction=mode==3?23:0;
  const double damage=std::max(0.,double(plan.damage+(mode==2?6:0))-reduction)*(mode==4&&id==312?2:1);
  const double expected=mode==9?0:accepted?1000-damage:1000;check(w.resources(2)->health==unsigned(std::ceil(expected)));
  check(visuals.size()==unsigned(accepted&&id==312));
  if(mode==10)check(w.knocked_down(target,100)==(id==296));
  if(accepted)check(w.player_holy_equipment(target,{}));
 }
 {world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",float((id-1)*100),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);check(w.player_holy_equipment(target,0));Slots bar{};bar[0]={312,0};Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[312]=1;attributes::Ranks ranks{};
  casting::Hooks h;std::vector<world::Packet> packets;h.resources=[&](unsigned id){return w.resources(id);};h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.projectile_target_valid=[](unsigned id,float range){return id==2&&range==144;};
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};h.consume_energy_at=[&](unsigned cost,auto now){return w.consume_energy(1,cost,owner,now);};
  h.holy_damage=[&](auto selected,const auto& plan,auto now)->casting::EffectCompletion{std::vector<world::Packet> visuals;const bool ok=w.holy_damage(owner,selected,plan,20,now,&visuals);return {ok,std::move(visuals)};};
  casting::Runtime runtime(1);check(runtime.start(1000,{312,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);runtime.update(1750,h);check(w.resources(2)->health==990);
  const auto body=*particles::targeted_body_2022(2,1,312);check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))<std::find(packets.begin(),packets.end(),body));
  w.leave(2);w.join(2,"Reused",100,0,std::make_shared<world::Outbox>(),0,{}, {},1800,2);w.set_vitals(2,{1000,1000,100,100},100);
  check(!w.player_holy_equipment(target,0)&&!w.holy_damage(owner,*w.effect_scope(2),*effects::holy_damage(312),20,1800));
 }
 std::cout<<"PASS reviewed player holy equipment,opaque rejection,three holy skills,flat vulnerability,protection,knockdown,range/team/lifetime/clearing,death and Runtime556\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
