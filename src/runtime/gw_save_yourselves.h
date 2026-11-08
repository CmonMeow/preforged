#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace fw::gw::skills::effects {
struct SaveYourselvesPlan {
 unsigned skill=0,rank=0,adrenaline=200,armor=100;
 Time duration=4000;float range=1012;
 auto operator<=>(const SaveYourselvesPlan&)const=default;
};
inline std::optional<SaveYourselvesPlan> save_yourselves(unsigned skill,unsigned rank){
 if((skill!=1954&&skill!=2097)||rank>12)return {};
 const auto* row=find_skill(skill);
 if(!row||row->profession!=1||row->attribute!=51||row->type!=15||
    row->special!=524288||row->energy_cost()||row->adrenaline!=200||
    row->activation||row->aftercast||row->recharge||row->duration0!=4||
    row->duration15!=6||row->caster_body_resource!=596||
    row->target_body_resource!=2077)return {};
 constexpr std::array<unsigned,7> seconds{4,4,5,5,5,6,6};
 return SaveYourselvesPlan{skill,rank,200,100,Time(seconds[std::min(rank,6u)])*1000};
}
inline std::optional<Entry> prepare_save_yourselves(const SaveYourselvesPlan& plan,
        Scope source,Scope target,unsigned effect,Time now,bool party_member,
        bool alive,float distance){
 const auto canonical=save_yourselves(plan.skill,plan.rank);
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!canonical||*canonical!=plan||!valid(source)||!valid(target)||
    source.instance!=target.instance||source.actor==target.actor||!effect||!party_member||
    !alive||!std::isfinite(distance)||distance<0||distance>plan.range||
    now>UINT64_MAX-plan.duration)return {};
 return Entry{effect,source.actor,plan.skill,plan.rank,Category::other,
     now,now+plan.duration};
}
inline unsigned save_yourselves_armor(const Registry& registry,Scope target,Time now){
 for(const auto& entry:registry.active(target,now))
  if(entry.skill==1954||entry.skill==2097){
   const auto plan=save_yourselves(entry.skill,entry.rank);
   if(plan&&entry.kind==Category::other&&entry.cause&&entry.cause<=65535&&
      entry.expires&&*entry.expires>=entry.applied&&
      *entry.expires-entry.applied==plan->duration)return plan->armor;
  }
 return 0;
}
}
