#include "gw_condition_equipment.h"
#include "gw_bundle_equipment.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Condition equipment line "+std::to_string(at.line()));
}
inventory::Profile profile(){
 auto p=inventory::baseline();
 const auto add=[&](unsigned id,unsigned type,unsigned bag,unsigned slot,std::vector<std::uint32_t> mods){
  inventory::Item item;item.definition.id=id;item.definition.type=type;item.definition.quantity=1;
  item.definition.modifiers=std::move(mods);item.bag=bag;item.slot=slot;p.items.emplace(id,item);
 };
 add(100,26,2,0,{0x246801e4});add(101,24,2,1,{0x247801e4});
 add(102,26,1,2,{0x246801e6});add(103,7,2,2,{0x27780605});add(104,19,2,3,{0x27780605});
 add(105,16,1,5,{0x28580100});p.weapons.sets[0]={100,101};p.weapons.sets[1]={102,101};return p;
}
int main()try{
 constexpr auto poison=unsigned(combat::conditions::Kind::poison),weakness=unsigned(combat::conditions::Kind::weakness),blind=unsigned(combat::conditions::Kind::blind);
 auto p=profile();const auto first=equipment::condition_equipment(p,7);
 check(first&&first->weapon()==100&&first->offhand()==101&&first->revision()==7&&first->active_set()==0);
 check(first->bonuses().extension[poison]&&first->bonuses().inscription[poison]&&first->bonuses().rune[poison]);
 check(!first->bonuses().extension[weakness]&&!first->bonuses().rune[blind]);
 // A saved reference in carrying storage is not a physically equipped weapon.
 p.weapons.active=1;check(!equipment::condition_equipment(p,8));
 p.items.at(100).bag=1;p.items.at(100).slot=0;p.items.at(102).bag=2;p.items.at(102).slot=0;
 const auto switched=equipment::condition_equipment(p,8);check(switched&&switched->bonuses().extension[weakness]&&!switched->bonuses().extension[poison]);
 // The original receipt is an immutable snapshot; later mutations do not rewrite it.
 check(first->weapon()==100&&first->revision()==7&&first->bonuses().extension[poison]);
 p.items.at(102).definition.interaction=0x200000;
 check(!equipment::condition_equipment(p,9)->offhand()&&!equipment::condition_equipment(p,9)->bonuses().inscription[poison]);
 const auto suppressed=equipment::condition_equipment(bundles::without_weapon_benefits(p),10);
 check(suppressed&&!suppressed->weapon()&&!suppressed->offhand()&&suppressed->bonuses().rune[poison]);
 for(unsigned mode=0;mode<7;++mode){auto bad=profile();
  if(mode==0)bad.items.at(100).definition.id=999;
  if(mode==1)bad.items.at(101).definition.quantity=2;
  if(mode==2)bad.items.at(103).definition.type=26;
  if(mode==3)bad.items.at(104).slot=2;
  if(mode==4)bad.items.at(103).definition.modifiers={0x27780905};
  if(mode==5)bad.items.at(100).definition.modifiers={0x247801e4};
  if(mode==6)bad.weapons.sets[0].weapon=999;
  check(!equipment::condition_equipment(bad,10));
 }
 check(equipment::condition_equipment(inventory::baseline(),0).has_value());
 std::cout<<"Active condition equipment, switching, twohand masking, bundles, rune collapse and malformed ownership pass\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
