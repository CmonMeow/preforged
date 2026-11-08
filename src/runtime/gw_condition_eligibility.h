#pragma once
#include "gw_conditions.h"
namespace fw::gw::combat::conditions {
// Partial eligibility only: caller separately validates lifetime,allegiance,
// capacity and other immunities. Unknown flesh is not immunity or permission.
inline std::optional<bool> flesh_eligibility(Kind kind,std::optional<bool> flesh){
 if(static_cast<unsigned>(kind)>=static_cast<unsigned>(Kind::count))return {};
 if(kind==Kind::bleeding||kind==Kind::poison||kind==Kind::disease)return flesh;
 return true;
}
}
