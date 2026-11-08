#pragma once
#include "gw_effect_registry.h"
#include <optional>
namespace fw::gw::skills::effects {
// A narrow capability captured from the actual owned Registry BEFORE an
// expiration callback removes its hex. It qualifies only this recipient and
// this exact boundary;it never rewinds general actor armor/condition state.
class EndingHexContext {
 Scope target_;Entry entry_;Time at_;
 EndingHexContext(Scope target,Entry entry,Time at):target_(target),entry_(entry),at_(at){}
public:
 static std::optional<EndingHexContext> capture(const Registry&registry,Scope target,unsigned effect,Time at){
  if(!registry.matches(target)||!target.instance||!target.actor||!target.generation)return {};
  const auto entry=registry.lookup(target,effect);
  if(!entry||entry->kind!=Category::hex||!entry->cause||!entry->expires||*entry->expires!=at||entry->applied>=at)return {};
  const auto*r=find_skill(entry->skill);if(!r||category(*r)!=Category::hex)return {};
  return EndingHexContext(target,*entry,at);
 }
 bool qualifies(Scope recipient,Time at)const{return recipient==target_&&at==at_;}
 unsigned effect()const{return entry_.instance;}
};
}
