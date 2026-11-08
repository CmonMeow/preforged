#pragma once
#include "gw_keystone_signet.h"
#include "gw_skillbar.h"
namespace fw::gw::skills::effects {
struct KeystoneCooldown {
 Slot slot;Time recharge_until=0,disabled_until=0;bool spent=false;
 bool operator==(const KeystoneCooldown&)const=default;
};
// Joint snapshot, not a replacement for World disable authority. Caller supplies
// authoritative eligible slots; dormant-copy membership needs retail evidence.
// Committing this proposal alone does not reset Runtime or World timers.
struct KeystoneCooldowns {
 Scope owner;std::uint64_t revision=1,last_reset=0;Time at=0;
 std::vector<KeystoneCooldown> slots;
 bool operator==(const KeystoneCooldowns&)const=default;
};
inline bool keystone_spent_signet(unsigned skill){
 const auto* row=find_skill(skill);
 // Both pinned rows match the older wiki's once-per-morale exception.
 return (skill==2||skill==1816)&&row&&row->type==7&&row->target==6&&(row->special&2048);
}
inline bool valid_keystone_cooldowns(const KeystoneCooldowns& state){
 if(!valid_keystone_scope(state.owner)||!state.revision)return false;
 std::set<std::pair<unsigned,unsigned>> seen;
 for(const auto& entry:state.slots){const auto* row=find_skill(entry.slot.skill);
  if(!row||!row->id||!row->type||entry.slot.copy>65535||!seen.emplace(entry.slot.skill,entry.slot.copy).second||
     (entry.spent&&!keystone_spent_signet(entry.slot.skill)))return false;
 }
 return true;
}
struct PreparedKeystoneRecharge {
 KeystoneCooldowns before,after;Time at=0;std::uint64_t event=0;
 std::vector<Slot> cleared;
 bool operator==(const PreparedKeystoneRecharge&)const=default;
};
// Called only for a verified successful Keystone initial effect. Signet
// recharge and skill-disable timers are both proposed for removal. Used
// resurrection signets stay used; a merely disabled unspent signet resets.
inline std::optional<PreparedKeystoneRecharge> prepare_keystone_recharge(const KeystoneCooldowns& current,Scope caster,Time now,std::uint64_t event){
 if(!valid_keystone_cooldowns(current)||caster!=current.owner||!event||event<=current.last_reset||now<current.at||current.revision==UINT64_MAX)return {};
 PreparedKeystoneRecharge p{current,current,now,event,{}};
 for(auto& entry:p.after.slots){const auto* row=find_skill(entry.slot.skill);
  if(row->type!=7||entry.slot.skill==63||entry.spent||(entry.recharge_until<=now&&entry.disabled_until<=now))continue;
  entry.recharge_until=entry.disabled_until=0;p.cleared.push_back(entry.slot);
 }
 p.after.revision++;p.after.last_reset=event;p.after.at=now;return p;
}
inline bool commit_keystone_recharge(KeystoneCooldowns& current,const PreparedKeystoneRecharge& proposal,Scope observed_caster){
 const auto expected=prepare_keystone_recharge(current,observed_caster,proposal.at,proposal.event);
 if(!expected||*expected!=proposal)return false;
 current=proposal.after;return true;
}
}
