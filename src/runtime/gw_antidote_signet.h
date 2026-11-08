#pragma once
#include "gw_conditions.h"
namespace fw::gw::skills::effects {
inline std::vector<combat::conditions::Kind> antidote_signet_removals(const combat::conditions::State& original,std::uint64_t now){
 using combat::conditions::Kind;auto remaining=original;std::vector<Kind> result;
 for(auto kind:{Kind::poison,Kind::disease,Kind::blind})if(remaining.has(kind,now)){remaining.remove(kind);result.push_back(kind);}
 if(const auto additional=remaining.newest(now))result.push_back(*additional);
 return result;
}
}
