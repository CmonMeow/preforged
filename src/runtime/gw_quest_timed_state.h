#pragma once
#include "gw_quest_state.h"
namespace fw::gw::quests {
inline std::optional<unsigned> marksmanship_credit_mask(const Progress& progress){
    unsigned mask=0;
    for(const auto credit:progress.objective_credits){
        if(credit<1||credit>5)return {};
        mask|=1u<<(credit-1);
    }
    return mask;
}
inline std::optional<Profile> arm_marksmanship(const Profile& current,std::uint32_t cycle,std::uint64_t now_utc_ms){
    validate(current);const auto found=current.records.find(42);
    if(found==current.records.end()||found->second.cycle!=cycle||found->second.stage!=Stage::active||
       found->second.timed||!found->second.objective_credits.empty()||!now_utc_ms||now_utc_ms>UINT64_MAX-90000)return {};
    auto next=current;next.records.at(42).timed=TimedAttempt{now_utc_ms,now_utc_ms+90000,false};return next;
}
inline std::optional<Profile> expire_marksmanship(const Profile& current,std::uint32_t cycle,std::uint64_t now_utc_ms){
    validate(current);const auto found=current.records.find(42);
    if(found==current.records.end()||found->second.cycle!=cycle||found->second.stage!=Stage::active||
       !found->second.timed||found->second.timed->expired||now_utc_ms<found->second.timed->deadline_utc_ms)return {};
    auto next=current;next.records.at(42).timed->expired=true;return next;
}
inline std::optional<std::uint32_t> marksmanship_elapsed(const Progress& progress,std::uint64_t now_utc_ms){
    if(progress.stage!=Stage::active||!progress.timed||progress.timed->expired||
       now_utc_ms<progress.timed->started_utc_ms||now_utc_ms>=progress.timed->deadline_utc_ms)return {};
    return static_cast<std::uint32_t>(now_utc_ms-progress.timed->started_utc_ms);
}
}
