#include "gw_dagger_failure_feedback_draft.h"
#include <array>
#include <stdexcept>
#include <type_traits>
using namespace fw::gw::combat::weapon;
static void check(bool value){if(!value)throw std::runtime_error("dagger failure wire/control regression");}
int main(){
 static_assert(!std::is_copy_constructible_v<DaggerFailureFeedbackDraft>);
 static_assert(!std::is_constructible_v<DaggerFailureFeedbackDraft,Scope,Scope,unsigned,unsigned,const AttackPermit&>);
 const Scope source{1,25,4},target{1,30,9};
 const std::vector<std::uint8_t> expected{0x9f,0,0x26,0,0,0,0x1e,0,0,0,0x19,0,0,0,2,0,0,0};
 check(dagger_failure_wire_draft(source,target)==expected);
 for(const Scope bad:std::array{Scope{0,25,4},Scope{1,0,4},Scope{1,65536,4},Scope{1,25,0},target})
  check(!dagger_failure_wire_draft(bad,target));
 for(const Scope bad:std::array{Scope{2,30,9},Scope{1,0,9},Scope{1,65536,9},Scope{1,30,0},source})
  check(!dagger_failure_wire_draft(source,bad));
}
