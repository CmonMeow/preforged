#pragma once
#include "gw_equipment_proposal.h"
#include "gw_requirement_stats.h"
#include <string_view>
namespace fw::gw::equipment::metadata {
inline std::optional<std::string> utf8(std::u16string_view value){
 std::string result;for(std::size_t i=0;i<value.size();++i){std::uint32_t cp=value[i];
  if(!cp)return {};if(cp>=0xd800&&cp<=0xdbff){if(++i>=value.size()||value[i]<0xdc00||value[i]>0xdfff)return {};cp=0x10000+((cp-0xd800)<<10)+(value[i]-0xdc00);}else if(cp>=0xdc00&&cp<=0xdfff)return {};
  if(cp<0x80)result+=char(cp);else if(cp<0x800){result+=char(0xc0|(cp>>6));result+=char(0x80|(cp&63));}else if(cp<0x10000){result+=char(0xe0|(cp>>12));result+=char(0x80|((cp>>6)&63));result+=char(0x80|(cp&63));}else{result+=char(0xf0|(cp>>18));result+=char(0x80|((cp>>12)&63));result+=char(0x80|((cp>>6)&63));result+=char(0x80|(cp&63));}}
 return result;
}
// FWI2 metadata is authoritative only when explicitly present. Absent owner
// metadata is unknown, not inferred "uncustomized" from a name/model. The
// capture adapter may store present-empty only when it actually observed no
// owner pointer after the fully populated native definition.
inline std::optional<proposal::Reviewed> reviewed(const inventory::Profile& owned,const items::Definition& definition){
 const auto it=owned.items.find(definition.id);if(it==owned.items.end())return {};
 const auto& item=it->second;const auto slot=proposal::native_slot(definition.type);
 if(!slot||definition.quantity!=1||!item.profession||*item.profession>10||!item.customized||item.customized->size()>31)return {};
 if(items::definition_2022(item.definition)!=items::definition_2022(definition))return {};
 const auto owner=utf8(*item.customized);if(!owner)return {};
 // Required attributes affect base benefits, not permission to equip. No
 // unknown raw modifier is interpreted as a fake requirement here.
 const auto validated=items::requirements::resolve(definition.modifiers,{},{});if(!validated)return {};
 const auto requirement=attribute_runes::requirement(definition.modifiers);
 return proposal::Reviewed{*slot,*item.profession,requirement?std::optional<unsigned>(requirement->attribute):std::nullopt,requirement?requirement->rank:0,*owner,bool(definition.interaction&0x200000u)};
}
}
