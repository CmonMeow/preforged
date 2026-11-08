#include "../tools/items/container_merge.h"
#include <iostream>
#include <stdexcept>
int main()try{
 using namespace fw::gw;
 const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
 auto current=inventory::baseline(),captured=inventory::baseline(),bank=storage::baseline();
 inventory::Item armor;armor.definition.id=1;armor.definition.file=544;armor.definition.type=7;armor.definition.modifiers={0xa3c83c00};armor.profession=6;armor.bag=2;armor.slot=2;
 current.items.emplace(1,armor);current.next_item=2;current.gold=1234;
 auto bank_item=armor;bank_item.definition.id=2;bank_item.bag=100;bank_item.slot=0;bank.items.emplace(2,bank_item);bank.next_item=3;
 constexpr std::array<unsigned,5> capacities{20,10,15,15,5};
 for(unsigned index=0;index<5;++index){
  const auto bagid=static_cast<std::uint16_t>(index?index+2:1);const auto itemid=100+index;
  captured.bags[bagid]={bagid,1,static_cast<std::uint8_t>(index),static_cast<std::uint8_t>(capacities[index]),itemid};
  inventory::Item item;item.definition.id=itemid;item.definition.file=111926+index;item.definition.type=3;item.definition.quantity=1;item.definition.encoded_name=u"Fixture Bag";
  captured.items.emplace(itemid,item);
 }
 captured.next_item=105;
 const auto original=inventory::encode(current),original_bank=inventory::encode(bank);
 const auto result=item_capture::merge_containers(current,captured,bank);
 check(result.bags.size()==6&&result.items.size()==6,"all captured carrying bags installed without copying contents");
 check(result.items.at(1).definition.modifiers==armor.definition.modifiers&&result.items.at(1).bag==2&&result.items.at(1).slot==2&&result.gold==1234,"existing equipment and gold preserved");
 check(!result.items.contains(2)&&!result.items.contains(100),"private identities rebound and bank collisions avoided");
 for(const auto& [id,bag]:result.bags)if(bag.type==1){
  check(bag.capacity==capacities[bag.index]&&bag.container&&inventory::installed(result.items.at(bag.container)),"captured capacity and installed link retained");
 }
 check(inventory::encode(item_capture::merge_containers(result,captured,bank))==inventory::encode(result),"repeat import is idempotent");
 check(inventory::encode(current)==original&&inventory::encode(bank)==original_bank,"plan never mutates original inventories");
 auto changed=captured;changed.items.at(100).definition.file=42;bool denied=false;
 try{item_capture::merge_containers(result,changed,bank);}catch(const std::runtime_error&){denied=true;}
 check(denied,"different existing owned bag cannot be silently discarded");
 auto occupied=current;auto carried=armor;carried.definition.id=3;carried.definition.type=11;carried.profession.reset();carried.bag=1;carried.slot=19;occupied.items.emplace(3,carried);occupied.next_item=4;
 changed=captured;changed.bags.at(1).capacity=10;denied=false;
 try{item_capture::merge_containers(occupied,changed,bank);}catch(const std::runtime_error&){denied=true;}
 check(denied,"shrinking captured container cannot lose occupied slots");
 changed=captured;changed.bags.erase(1);changed.items.erase(100);denied=false;
 try{item_capture::merge_containers(current,changed,bank);}catch(const std::runtime_error&){denied=true;}
 check(denied,"missing captured backpack rejected");
 std::cout<<"Installed bag merge preserves inventory, ownership domains and repeat imports\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
