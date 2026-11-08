#pragma once
#include "gw_item_wire.h"
#include <algorithm>
#include <map>
#include <optional>
#include <span>
#include <set>
namespace fw::gw::quests {
enum class Stage:std::uint32_t {active=1,completed=2,rewarded=3,abandoned=4};
struct TimedAttempt {
    // Absolute server UTC milliseconds, not client or process uptime.
    std::uint64_t started_utc_ms=0,deadline_utc_ms=0;bool expired=false;
    bool operator==(const TimedAttempt&)const=default;
};
struct Progress {
    std::uint32_t cycle=1;Stage stage=Stage::active;
    // Server-owned credit identities, scoped to this quest attempt. Never use a
    // bare client agent ID: those are recycled within and between instances.
    std::set<std::uint64_t> objective_credits;
    std::optional<TimedAttempt> timed;
    bool operator==(const Progress&)const=default;
};
struct Profile {std::map<std::uint32_t,Progress> records;bool operator==(const Profile&)const=default;};
inline void validate(const Profile& profile){
    if(profile.records.size()>4096)throw std::invalid_argument("quest profile count exceeds private storage bound");
    for(const auto& [id,progress]:profile.records){
        if(!id||!progress.cycle||progress.stage<Stage::active||progress.stage>Stage::abandoned||
           progress.objective_credits.size()>64||progress.objective_credits.contains(0))
            throw std::invalid_argument("invalid quest progress");
        if(progress.timed){const auto& timed=*progress.timed;
            if(id!=42||!timed.started_utc_ms||timed.started_utc_ms>UINT64_MAX-90000||
               timed.deadline_utc_ms!=timed.started_utc_ms+90000||progress.objective_credits.size()>5||
               ((progress.stage==Stage::completed||progress.stage==Stage::rewarded)&&progress.objective_credits.size()!=5)||
               (progress.stage==Stage::active&&progress.objective_credits.size()==5)||
               (timed.expired&&(progress.stage==Stage::completed||progress.stage==Stage::rewarded)))
                throw std::invalid_argument("invalid timed quest progress");
        }
    }
}
// These planners do not authorize NPCs/objectives/rewards. The quest controller
// must validate those, then atomically persist the proposal with reward changes.
inline std::optional<Profile> accept(const Profile& current,std::uint32_t id,bool repeatable=false){
    validate(current);if(!id)return {};auto next=current;const auto found=next.records.find(id);
    if(found==next.records.end()){
        if(next.records.size()==4096)return {};next.records.emplace(id,Progress{});
    }else{
        if(found->second.cycle==UINT32_MAX||
            !(found->second.stage==Stage::abandoned||(repeatable&&found->second.stage==Stage::rewarded)))return {};
        found->second={found->second.cycle+1,Stage::active};
    }
    return next;
}
inline std::optional<Profile> advance(const Profile& current,std::uint32_t id,std::uint32_t cycle,Stage from,Stage to){
    validate(current);
    if(!((from==Stage::active&&to==Stage::completed)||(from==Stage::completed&&to==Stage::rewarded)))return {};
    const auto found=current.records.find(id);
    if(found==current.records.end()||found->second.cycle!=cycle||found->second.stage!=from)return {};
    if(found->second.timed&&to==Stage::completed&&(found->second.timed->expired||found->second.objective_credits.size()!=5))return {};
    auto next=current;next.records.at(id).stage=to;return next;
}
inline std::optional<Profile> abandon(const Profile& current,std::uint32_t id,std::uint32_t cycle){
    validate(current);const auto found=current.records.find(id);
    if(found==current.records.end()||found->second.cycle!=cycle||
        (found->second.stage!=Stage::active&&found->second.stage!=Stage::completed))return {};
    // Retain the attempt counter so delayed operations cannot affect a reaccepted quest.
    auto next=current;next.records.at(id).stage=Stage::abandoned;return next;
}
// The controller validates the encounter actor lifetime and supplies a stable
// credit identity. Persistence uses the existing profile revision transaction.
inline std::optional<Profile> credit_objective(const Profile& current,std::uint32_t id,
    std::uint32_t cycle,std::uint64_t credit,unsigned required,std::optional<std::uint64_t> occurred_utc_ms={}){
    validate(current);if(!credit||!required||required>64)return {};
    const auto found=current.records.find(id);
    if(found==current.records.end()||found->second.cycle!=cycle||found->second.stage!=Stage::active||
       found->second.objective_credits.contains(credit)||found->second.objective_credits.size()>=required)return {};
    if(id==42){const auto& timed=found->second.timed;
        // Durable worm keys must match MarksmanshipObjective::logical_credit
        // (1..5), so reconnect can restore the same logical kills. Preserve
        // decoding of legacy opaque keys, but require a fresh attempt before
        // adding credit to those unresolvable histories.
        if(credit>5||std::any_of(found->second.objective_credits.begin(),found->second.objective_credits.end(),
               [](std::uint64_t key){return key>5;})||
           !timed||timed->expired||required!=5||!occurred_utc_ms||*occurred_utc_ms<timed->started_utc_ms||
           *occurred_utc_ms>=timed->deadline_utc_ms)return {};
    }
    auto next=current;auto& progress=next.records.at(id);progress.objective_credits.insert(credit);
    if(progress.objective_credits.size()==required)progress.stage=Stage::completed;
    return next;
}
inline items::Packet encode(const Profile& profile){
    validate(profile);bool counted=false,timed=false;for(const auto& [id,p]:profile.records){counted|=!p.objective_credits.empty();timed|=p.timed.has_value();}
    counted|=timed;
    items::Packet out{'F','W','Q',static_cast<std::uint8_t>(timed?'3':counted?'2':'1')};items::put(out,static_cast<unsigned>(profile.records.size()),4);
    for(const auto& [id,progress]:profile.records){items::put(out,id,4);items::put(out,progress.cycle,4);items::put(out,unsigned(progress.stage),4);
        if(counted){items::put(out,static_cast<unsigned>(progress.objective_credits.size()),4);
            for(auto credit:progress.objective_credits){items::put(out,static_cast<std::uint32_t>(credit),4);items::put(out,static_cast<std::uint32_t>(credit>>32),4);}}
        if(timed){items::put(out,progress.timed?(progress.timed->expired?3:1):0,4);
            if(progress.timed)for(auto value:{progress.timed->started_utc_ms,progress.timed->deadline_utc_ms}){
                items::put(out,static_cast<std::uint32_t>(value),4);items::put(out,static_cast<std::uint32_t>(value>>32),4);}}
    }
    return out;
}
inline Profile decode(std::span<const std::uint8_t> bytes){
    if(bytes.size()<8||bytes[0]!='F'||bytes[1]!='W'||bytes[2]!='Q'||(bytes[3]!='1'&&bytes[3]!='2'&&bytes[3]!='3'))throw std::invalid_argument("unsupported quest profile");
    const bool counted=bytes[3]!='1',timed=bytes[3]=='3';
    const auto word=[&](std::size_t at){std::uint32_t value=0;for(unsigned n=0;n<4;++n)value|=std::uint32_t(bytes[at+n])<<(n*8);return value;};
    const auto count=word(4);if(count>4096||(!counted&&bytes.size()!=8+std::size_t(count)*12))throw std::invalid_argument("invalid quest profile framing");
    Profile out;std::uint32_t previous=0;std::size_t at=8;
    for(unsigned i=0;i<count;++i){if(bytes.size()-at<(counted?16u:12u))throw std::invalid_argument("truncated quest progress");const auto id=word(at);
        if(id<=previous)throw std::invalid_argument("noncanonical quest profile identity order");previous=id;
        Progress progress{word(at+4),static_cast<Stage>(word(at+8))};at+=12;
        if(counted){const auto credits=word(at);at+=4;if(credits>64||bytes.size()-at<std::size_t(credits)*8)throw std::invalid_argument("invalid objective credit framing");
            std::uint64_t previous_credit=0;
            for(unsigned c=0;c<credits;++c){const auto credit=std::uint64_t(word(at))|(std::uint64_t(word(at+4))<<32);at+=8;
                if(credit<=previous_credit)throw std::invalid_argument("noncanonical objective credit");previous_credit=credit;progress.objective_credits.insert(credit);}}
        if(timed){if(bytes.size()-at<4)throw std::invalid_argument("truncated timed quest flags");
            const auto flags=word(at);at+=4;if(flags!=0&&flags!=1&&flags!=3)throw std::invalid_argument("invalid timed quest flags");
            if(flags){if(bytes.size()-at<16)throw std::invalid_argument("truncated timed quest deadline");
                progress.timed=TimedAttempt{std::uint64_t(word(at))|(std::uint64_t(word(at+4))<<32),
                    std::uint64_t(word(at+8))|(std::uint64_t(word(at+12))<<32),flags==3};at+=16;}}
        out.records.emplace(id,std::move(progress));}
    if(at!=bytes.size())throw std::invalid_argument("trailing quest profile bytes");
    validate(out);return out;
}
}
