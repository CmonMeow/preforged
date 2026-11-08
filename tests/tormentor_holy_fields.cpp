#include "gw_world_simulation.h"
#include "gw_player_holy_equipment.h"
#include "gw_basic_armor_energy.h"
#include "gw_armor_upgrades.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Tormentor/holy fields line "+std::to_string(at.line())+context);
}
int main()try{
 std::array<CharacterProfile::EquipmentAppearance,5> visual{};for(auto& piece:visual)piece.composite=1;
 auto basic=energy::private_starter_armor(4,visual,u"Necromancer");auto tormentor=basic;unsigned total=0;
 for(auto&[id,item]:tormentor.items){item.definition.modifiers.push_back(0x253003d8);total+=item.slot==2?6:item.slot==3?4:2;
  check(equipment::reviewed_player_holy_bonus(tormentor,4)==total);
  const auto armor=energy::physical_armor_locations(tormentor,4);check(bool(armor));
  for(const auto&[other_id,other]:tormentor.items)check((*armor)[other.slot-2]==(other_id<=id?70:60));
 }
 check(total==16&&energy::fully_modeled_elemental_armor(tormentor,4)==70&&energy::fully_modeled_physical_armor(tormentor,4)==70&&energy::fully_modeled_neutral_armor(tormentor,4)==70);
 check(equipment::attribute_runes::worn(tormentor,4).has_value());
 const std::array<std::uint32_t,1> word{0x253003d8};check(!equipment::attribute_runes::decode(word,3).valid);
 auto duplicate=tormentor;duplicate.items.begin()->second.definition.modifiers.push_back(0x253003d8);check(!equipment::reviewed_player_holy_bonus(duplicate,4));
 {auto profile=basic;auto& armor=profile.items.begin()->second;armor.definition.modifiers={0x253003ca};const auto target=armor.definition.id;
  inventory::Item source;source.definition.id=profile.next_item++;source.definition.file=source.definition.model=1;source.definition.type=8;source.definition.quantity=2;
  source.definition.modifiers={0x25b80000,0x253003d8};source.definition.interaction=0x100000;source.profession=4;source.bag=1;source.slot=0;profile.items.emplace(source.definition.id,source);
  const auto before=inventory::encode(profile);const auto applied=inventory::upgrades::apply(profile,source.definition.id,target);
  check(applied&&inventory::encode(profile)==before&&applied->profile.items.at(source.definition.id).definition.quantity==1);
  const auto slot=armor.slot;check(equipment::reviewed_player_holy_bonus(applied->profile,4)==(slot==2?6u:slot==3?4u:2u));
  auto replacement=applied->profile;replacement.items.at(source.definition.id).definition.modifiers={0x25b80000,0x253003cc};
  const auto removed=inventory::upgrades::apply(replacement,source.definition.id,target);check(removed&&equipment::reviewed_player_holy_bonus(removed->profile,4)==0);
  auto wrong=profile;wrong.items.at(target).profession=3;check(!inventory::upgrades::apply(wrong,source.definition.id,target));
 }
 for(unsigned skill:{247u,1113u})for(unsigned mode=0;mode<8;++mode){context=" skill="+std::to_string(skill)+" mode="+std::to_string(mode);world::Instance w;
  for(unsigned id:{1u,2u,3u,4u}){w.join(id,"Actor",id==1?0:id==3?300:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==4?1:2);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);if(mode!=0)check(w.player_holy_equipment(target,mode==2||mode==4||mode==6?16:0));
  check(w.player_holy_equipment(*w.effect_scope(3),16)&&w.player_holy_equipment(*w.effect_scope(4),16));
  if(mode==6)check(w.protective_enchantment(target,target,*regeneration::reviewed(299,20),0,100));
  check(w.damage_field(owner,*effects::damage_field(skill,0),20,100));
  if(mode==5)check(w.kill(owner,200));if(mode==7){w.leave(1);w.join(1,"Reused",0,0,std::make_shared<world::Outbox>(),0,{}, {},200,1);w.set_vitals(1,{1000,1000,100,100},100);}
  unsigned damage=0;
  for(unsigned pulse=1;pulse<=5;++pulse){const auto now=100+1000*pulse;
   if(mode==3&&pulse==3)check(w.player_holy_equipment(target,equipment::reviewed_player_holy_bonus(tormentor,4)));
   if(mode==4&&pulse==3)check(w.player_holy_equipment(target,{}));if(mode==4&&pulse==4)check(w.player_holy_equipment(target,0));
   const bool hit=mode!=0&&mode!=7&&!(mode==4&&pulse==3);
   const unsigned bonus=mode==2||mode==6||(mode==4&&pulse<3)||(mode==3&&pulse>=3)?16:0;
   if(hit)damage+=mode==6?1:8+bonus;w.tick(now);check(w.resources(2)->health==1000-damage);
   w.tick(now);check(w.resources(2)->health==1000-damage&&w.resources(3)->health==1000&&w.resources(4)->health==1000);
  }
  w.tick(6100);check(w.resources(2)->health==1000-damage);
 }
 {world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",float((id-1)*100),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);check(w.player_holy_equipment(target,0,100));
  check(w.damage_field(owner,*effects::damage_field(247,0),20,100));
  check(w.player_holy_equipment(target,16,2500)&&w.resources(2)->health==984); // Earlier1100/2100 pulses use the old gear.
  check(!w.player_holy_equipment(target,0,2000));w.tick(5100);check(w.resources(2)->health==912);
  check(!w.player_holy_equipment(target,0,5000));
 }
 std::cout<<"PASS Tormentor native upgrade identity/class/replacement/debit,per-piece armor,stacked16holy vulnerability,Symbol/Kirin per-pulse and delayed equipment/unknown/protection/death/lifetime and no replay\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
