// Merchant rows are captured stock; mainhands are explicitly synthetic owned
// fixtures exercising the independently proven native twohand interaction bit.
#include "../src/runtime/gw_merchant_panel.h"
#include "../src/runtime/gw_merchant_exchange.h"
#include "../src/runtime/gw_equipment_metadata_provider.h"
#include "../src/runtime/gw_weapon_passives.h"
#include "../src/runtime/gw_basic_armor_energy.h"
#include <iostream>
int main()try{
 namespace inv=fw::gw::inventory;namespace m=fw::gw::merchant::native;
 const auto check=[](bool value,const char* why){if(!value)throw std::runtime_error(why);};
 for(unsigned row:{0u,1u}){
  auto owned=inv::baseline();owned.gold=500;
  fw::gw::collectors::native::MenuIds ids;m::Binding binding;
  const m::Observation at{10012,148,0,99,7,10,500,true};
  check(m::open_panel(binding,ids,at,"Sanura",owned).has_value(),"merchant opening failed");
  const auto stock=binding.stock;const auto quote=fw::gw::merchant::buy_quote(stock[row].definition,binding.scale,1);
  const m::Request request{75,1,quote->gold,0,{},{{stock[row].definition.id,1}}};
  const auto plan=m::authorize(binding,request,owned,at);check(bool(plan),"offhand buy admission failed");
  const auto bought=m::propose(*plan,owned,10,stock,u"Tester");check(bool(bought),"offhand purchase failed");
  owned=inv::decode(inv::encode(bought->profile));const auto offhand=owned.items.begin()->first;
  const auto equip=[&](unsigned id){
   fw::gw::equipment::proposal::Policy policy;policy.allowed=true;policy.primary=6;policy.character="Tester";
   policy.reviewed=[&](const fw::gw::items::Definition& d){return fw::gw::equipment::metadata::reviewed(owned,d);};
   inv::Request request;request.change=inv::Change::equip;request.item=id;
   const auto result=fw::gw::equipment::proposal::propose(owned,request,policy);
   check(result.accepted(),"real equipment admission failed");owned=inv::decode(inv::encode(result.profile));
  };
  equip(offhand);
  const auto assert_benefits=[&](unsigned energy,int armor){
   check(fw::gw::items::weapon_passives::worn_energy(owned)==energy,"suppressed or restored offhand Energy incorrect");
   std::array<int,5> expected{};expected.fill(armor);
   check(fw::gw::energy::physical_armor_locations(owned,6)==expected,"effective Starter Shield missing physical location armor");
   check(fw::gw::energy::elemental_armor_locations(owned,6)==expected,"effective Starter Shield missing elemental location armor");
   check(fw::gw::energy::fully_modeled_physical_armor(owned,6)==armor&&
         fw::gw::energy::fully_modeled_elemental_armor(owned,6)==armor,"uniform armor path differs from locations");
  };
  assert_benefits(row==0?3:0,row==1?3:0);
  unsigned sword=0,staff=0;
  for(unsigned type:{27u,26u}){inv::Item item;item.definition.id=owned.next_item++;item.definition.type=type;
   item.definition.file=1234;item.definition.quantity=1;item.definition.interaction=type==26?0x200000:0;
   item.profession=0;item.customized=u"";item.bag=1;item.slot=type==26?1:0;
   if(type==26)item.definition.modifiers={0x62c80a00};
   owned.items.emplace(item.definition.id,item);(type==26?staff:sword)=item.definition.id;
  }
  equip(sword);assert_benefits(row==0?3:0,row==1?3:0);
  equip(staff);
  check(owned.items.at(offhand).bag==2&&owned.items.at(offhand).slot==1&&
        owned.weapons.sets[0].offhand==offhand,"twohand equip lost physical/stored offhand");
  assert_benefits(10,0);
  equip(sword);assert_benefits(row==0?3:0,row==1?3:0);
  {const auto original=owned;
   owned.items.at(offhand).bag=1;owned.items.at(offhand).slot=5;
   assert_benefits(0,0); // Stored reference to a carried offhand grants nothing.
   owned=original;owned.weapons.sets[0].weapon=staff;
   assert_benefits(0,0); // Carried active mainhand is not physically wielded.
   owned=original;
  }
  if(row==1){const auto original=owned;
   for(unsigned field=0;field<4;++field){owned=original;auto& shield=owned.items.at(offhand);
    switch(field){case 0:shield.profession.reset();break;case 1:shield.customized.reset();break;
     case 2:++shield.definition.model;break;case 3:shield.definition.modifiers={0xa3c81000};break;}
    assert_benefits(0,0);
   }
   owned=original;constexpr unsigned types[]{7,19,16,4,13};
   for(unsigned slot=2;slot<=6;++slot){inv::Item armor;armor.definition.id=owned.next_item++;
    armor.definition.type=types[slot-2];armor.definition.file=1234;armor.definition.quantity=1;armor.definition.modifiers={0xa3c83c00};
    armor.bag=2;armor.slot=static_cast<std::uint8_t>(slot);armor.profession=6;armor.customized=u"Tester";
    armor.reviewed_armor=inv::ReviewedArmor{60,0,0};owned.items.emplace(armor.definition.id,armor);
   }
   owned=inv::decode(inv::encode(owned));assert_benefits(0,63);
  }
 }
 std::cout<<"PASS native stock purchase, persisted equipment and effective offhand Energy/AR3\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

