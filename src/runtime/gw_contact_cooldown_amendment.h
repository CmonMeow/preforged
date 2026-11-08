#pragma once
#include "gw_cooldown_bar_authority.h"
#include <array>
#include <memory>
namespace fw::gw::world {class Instance;}
namespace fw::gw::skills::effects {
// A later physical contact is not another logical skill completion.
struct CooldownAmendment {
 CooldownStamp stamp;Slot slot;std::uint64_t skill_use=0,physical_event=0;
 CooldownStamp completion_stamp;std::uint64_t before_until=0,until=0;
 bool operator==(const CooldownAmendment&)const=default;
};
class ContactCooldownContinuation {
 friend class fw::gw::world::Instance;
 const Scope source_;const Slot slot_;const std::uint64_t use_,first_at_;
 std::array<std::uint64_t,2> events_{};
 std::optional<CooldownCompletion> completion_;bool spent_=false;
 ContactCooldownContinuation(Scope source,Slot slot,std::uint64_t use,std::uint64_t at):source_(source),slot_(slot),use_(use),first_at_(at){}
public:
 ContactCooldownContinuation(const ContactCooldownContinuation&)=delete;
 Scope source()const{return source_;}Slot slot()const{return slot_;}
 std::uint64_t skill_use()const{return use_;}
};
class ContactCooldownQuote {
 friend class fw::gw::world::Instance;
 ContactCooldownQuote()=default;
 std::shared_ptr<ContactCooldownContinuation> continuation_;
 std::uint64_t event_=0,at_=0,before_until_=0,until_=0;
 std::uint64_t completion_order_=0,reset_order_=0,amendment_order_=0;
public:
 ContactCooldownQuote(const ContactCooldownQuote&)=default;
 ContactCooldownQuote& operator=(const ContactCooldownQuote&)=default;
};
// Reviewed staged policy: additional recharge starts from the still-current
// local base timer, or from this contact when that timer expired/was reset.
// Independent World disables are not included in the base.
inline std::optional<std::uint64_t> contact_amendment_deadline(std::uint64_t now,std::uint64_t base_until){
 const auto anchor=std::max(now,base_until);
 if(anchor>UINT64_MAX-15000)return {};
 return anchor+15000;
}
}
