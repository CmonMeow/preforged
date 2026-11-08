#include "gw_world_simulation.h"
#include "gw_basic_armor_energy.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Mixed elemental weapon armor line "+std::to_string(at.line())+context);
}
int main()try{
 std::array<CharacterProfile::EquipmentAppearance,5> visual{};for(auto& piece:visual)piece.composite=1;
 for(unsigned primary:{1u,2u}){const auto profile=energy::private_starter_armor(primary,visual,u"Reviewed armor");
  const auto physical=energy::physical_armor_locations(profile,primary),elemental=energy::elemental_armor_locations(profile,primary);check(physical&&elemental);
  for(unsigned slot=0;slot<5;++slot){check((*physical)[slot]==(primary==1?100:70));check((*elemental)[slot]==(primary==1?80:100));}
 }
 const std::array<int,5> locations{70,60,50,40,30};
 {
  for(auto insignia:{0xa53003f4u,0xa53003ceu}){
  auto suit=energy::private_starter_armor(1,visual,u"Insignia Warrior");
  for(auto&[id,item]:suit.items)if(item.slot==2)item.definition.modifiers.push_back(insignia);
  for(unsigned raw:{2u,3u,4u,5u,11u})for(unsigned bucket=0;bucket<8;++bucket){
   context=" insignia="+std::to_string(insignia)+" raw="+std::to_string(raw)+" bucket="+std::to_string(bucket);world::Instance w;
   for(unsigned id:{1u,2u}){w.join(id,"Actor",float((id-1)*100),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}
   const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
   check(w.elemental_armor(target,{},{})&&w.lightning_armor(target,{})&&w.combat_armor(target,{}));
   check(w.physical_armor_locations(target,energy::physical_armor_locations(suit,1))&&w.elemental_armor_locations(target,energy::elemental_armor_locations(suit,1)));
   unsigned draws=0;w.configure_armor_location_draws([&]{++draws;return bucket;});w.configure_attack_miss_draws([]{return .5;});
   combat::weapon::Owned weapon{12,1,{20,20,raw},12,false,false,27};check(w.ordinary_weapon(source,weapon,20));
   check(w.melee_attack(source,target,weapon,*combat::weapon::attack_skill(322,12,0),100));
   const int armor=(raw==2?100:80)+(bucket<3&&((raw==2&&insignia==0xa53003ce)||(raw!=2&&insignia==0xa53003f4))?10:0);
   check(draws==1&&w.resources(2)->health==unsigned(std::ceil(1000-20*std::exp2((60.-armor)/40.)-34)));
  }
  }
 }
 for(unsigned raw:{3u,4u,5u,11u})for(unsigned mode=0;mode<4;++mode)for(unsigned bucket=0;bucket<8;++bucket){
  context=" raw="+std::to_string(raw)+" mode="+std::to_string(mode)+" bucket="+std::to_string(bucket);world::Instance w;
  for(unsigned id:{1u,2u}){w.join(id,"Actor",float((id-1)*100),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.elemental_armor(target,{},{})&&w.lightning_armor(target,{})&&w.combat_armor(target,{}));
  check(w.physical_armor_locations(target,std::array<int,5>{0,0,0,0,0})&&w.elemental_armor_locations(target,locations));
  unsigned draws=0;w.configure_armor_location_draws([&]{++draws;return bucket;});w.configure_attack_miss_draws([]{return .5;});
  const bool caster=mode==2;combat::weapon::Owned weapon{12,1,{20,20,raw},12,false,caster,caster?26u:mode==1?5u:27u};check(w.ordinary_weapon(source,weapon,20));
  unsigned bonus=mode==0||mode==3?34:0;
  if(mode==3&&raw!=11)check(w.conjure_enchantment(source,*skills::effects::conjure(raw==3?207:raw==4?221:182,12),100));
  if(mode==2){check(w.ordinary_attack(source,2,100)&&draws==0);for(std::uint64_t now=200;now<=1800;now+=100)w.tick(now);w.cancel_ordinary_attack(source);}
  else {check(w.melee_attack(source,target,weapon,*combat::weapon::attack_skill(mode==1?393:322,mode==1?0:12,0),100));if(mode==1){check(draws==0);w.tick(200);}}
  const int armor=bucket<3?70:bucket<5?60:bucket==5?50:bucket==6?40:30;
  const unsigned conjure=mode==3&&raw!=11?17:0;
  const auto expected=unsigned(std::ceil(1000-20*std::exp2((60.-armor)/40.)-bonus-conjure));
  check(draws==1&&w.resources(2)->health==expected);
 }
 for(unsigned mode=0;mode<4;++mode){context=" clearing="+std::to_string(mode);world::Instance w;
  for(unsigned id:{1u,2u}){w.join(id,"Actor",float((id-1)*100),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);unsigned draws=0;w.configure_armor_location_draws([&]{++draws;return 0u;});
  check(w.elemental_armor_locations(target,locations));if(mode==0)check(w.elemental_armor_locations(target,{}));
  if(mode==1)check(w.elemental_armor(target,60,60));
  if(mode==2){auto invalid=locations;invalid[0]=501;check(!w.elemental_armor_locations(target,invalid));}
  if(mode==3){auto stale=target;++stale.generation;check(!w.elemental_armor_locations(stale,{}));}
  combat::weapon::Owned weapon{12,1,{20,20,5},12,false,false,27};check(w.ordinary_weapon(source,weapon,20));
  check(w.melee_attack(source,target,weapon,*combat::weapon::attack_skill(322,12,0),100)==(mode!=0));
  check(draws==(mode>=2?1u:0u));check(w.resources(2)->health==(mode==0?1000u:mode==1?946u:950u));
 }
 std::cout<<"PASS mixed elemental weapon armor all4elements/8buckets,melee/bow/staff,Conjure no extra armor draw,physical separation,reviewed profession bonuses,clearing/scalar override and invalid/stale state\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
