#pragma once
#include "gw_inventory.h"
#include <charconv>
#include <string_view>
namespace fw::gw::trade::exchange {
struct Offered {std::uint32_t item,quantity;};
struct Offer {std::uint32_t gold=0;std::vector<Offered> items;};
inline std::uint32_t integer(std::string_view text){std::uint32_t value=0;auto result=std::from_chars(text.data(),text.data()+text.size(),value);if(text.empty()||result.ec!=std::errc()||result.ptr!=text.data()+text.size())throw std::invalid_argument("trade offer integer");return value;}
inline Offer parse(std::string_view gold,std::string_view items){
 Offer offer;offer.gold=integer(gold);if(offer.gold>100000)throw std::invalid_argument("trade gold cap");
 while(!items.empty()){auto comma=items.find(',');auto entry=items.substr(0,comma);auto colon=entry.find(':');if(colon==entry.npos)throw std::invalid_argument("trade item offer format");
 auto item=integer(entry.substr(0,colon)),quantity=integer(entry.substr(colon+1));if(!item||!quantity||offer.items.size()>=7||std::any_of(offer.items.begin(),offer.items.end(),[&](auto old){return old.item==item;}))throw std::invalid_argument("trade duplicate/empty/bounded offer");
 offer.items.push_back({item,quantity});if(comma==items.npos)break;items.remove_prefix(comma+1);if(items.empty())throw std::invalid_argument("trade trailing offer separator");}
 return offer;
}
struct Result {inventory::Profile left,right;};
// Pinned Toolbox InventoryItem.h IsTradable: interaction100 forbids trading.
// Customization alone does not. Keep ownership/placement and known type
// restrictions common to UI publication and authoritative exchange admission.
inline bool offerable(const inventory::Profile& profile,std::uint32_t identity){
 const auto found=profile.items.find(identity);if(found==profile.items.end())return false;
 const auto& item=found->second;if(item.definition.interaction&0x100u)return false;
 const auto type=item.definition.type;
 if(type==6||type==20||type==21||type==44||type==45)return false;
 const auto bag=profile.bags.find(item.bag);
 return bag!=profile.bags.end()&&bag->second.type==1;
}
inline Result plan(const inventory::Profile& left,const Offer& a,const inventory::Profile& right,const Offer& b,
                   std::u16string_view left_character={},std::u16string_view right_character={}){
 inventory::validate_character(left);inventory::validate_character(right);Result result{left,right};
 if(a.items.size()>7||b.items.size()>7||a.gold>left.gold||b.gold>right.gold)throw std::invalid_argument("trade offered resources unavailable");
 const auto extract=[](inventory::Profile& source,const Offer& offer){std::vector<inventory::Item> moved;std::set<std::uint32_t> seen;
 for(auto part:offer.items){auto found=source.items.find(part.item);if(!seen.insert(part.item).second||found==source.items.end()||!part.quantity||part.quantity>found->second.definition.quantity)throw std::invalid_argument("trade item not owned/quantity");
 auto& item=found->second;
 if(!offerable(source,part.item))throw std::invalid_argument("trade restricted or non-carried item");
 if(part.quantity!=item.definition.quantity&&!(item.definition.interaction&0x80000))throw std::invalid_argument("trade nonstack partial quantity");
 moved.push_back(item);moved.back().definition.quantity=part.quantity;
 if(part.quantity==item.definition.quantity){source.weapons=weapon_sets::erased(source.weapons,part.item);source.items.erase(found);}else item.definition.quantity-=part.quantity;
 }return moved;};
 auto outgoing_a=extract(result.left,a),outgoing_b=extract(result.right,b);
 const auto insert=[](inventory::Profile& target,const std::vector<inventory::Item>& incoming,std::u16string_view character){for(auto item:incoming){
 unsigned remaining=item.definition.quantity;
 if(item.definition.interaction&0x80000)for(auto&[id,old]:target.items){auto bag=target.bags.find(old.bag);if(bag==target.bags.end()||bag->second.type!=1||!inventory::accepts_item(bag->second,item,character)||old.profession!=item.profession||old.customized!=item.customized||old.reviewed_armor!=item.reviewed_armor||!inventory::compatible(old.definition,item.definition))continue;
 auto amount=std::min(remaining,250-old.definition.quantity);old.definition.quantity+=amount;remaining-=amount;if(!remaining)break;}
 if(!remaining)continue;
 bool placed=false;for(auto[bagid,bag]:target.bags){if(bag.type!=1||!inventory::accepts_item(bag,item,character))continue;for(unsigned slot=0;slot<bag.capacity;++slot){if(std::any_of(target.items.begin(),target.items.end(),[&](const auto& pair){return pair.second.bag==bagid&&pair.second.slot==slot;}))continue;
 if(target.items.size()>=inventory::maximum_items||target.next_item==UINT32_MAX)throw std::invalid_argument("trade recipient identity exhausted");item.definition.id=target.next_item++;item.definition.quantity=remaining;item.bag=bagid;item.slot=static_cast<std::uint8_t>(slot);target.items.emplace(item.definition.id,item);placed=true;break;}if(placed)break;}
 if(!placed)throw std::invalid_argument("trade recipient inventory full");
 }};
 insert(result.left,outgoing_b,left_character);insert(result.right,outgoing_a,right_character);
 const std::uint64_t ga=std::uint64_t(left.gold)-a.gold+b.gold,gb=std::uint64_t(right.gold)-b.gold+a.gold;
 if(ga>100000||gb>100000)throw std::invalid_argument("trade recipient gold cap");result.left.gold=static_cast<std::uint32_t>(ga);result.right.gold=static_cast<std::uint32_t>(gb);
 inventory::validate_character(result.left);inventory::validate_character(result.right);return result;
}
}



