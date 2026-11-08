#pragma once
#include "gw_quest_restore.h"
#include "gw_effect_registry.h"
#include <array>
#include <string_view>
namespace fw::gw::quests::opposition {
inline constexpr unsigned id=78,experience=500;
struct Witness {unsigned credit,map;std::string_view name;};
// Stable server-owned identity: neither transient agent IDs nor question order.
// All five are supported by local wiki revision2649446, including Estate locals.
inline constexpr std::array<Witness,5> witnesses{{
    {1,160,"Farrah Cappo"},{2,160,"Grazden the Protector"},{3,160,"Kasha Blackblood"},
    {4,163,"Little Thom"},{5,163,"Duke Barradin"}
}};
inline const Witness* witness(unsigned map,std::string_view name){
    for(const auto& row:witnesses)if(row.map==map&&row.name==name)return &row;return nullptr;
}
struct Eligibility {bool prophecies=false,presearing=false;};
struct Interaction {skills::effects::Scope selected,current;std::string_view name;unsigned map=0;bool reachable=false;};
enum class Action {accept,question,reward};
inline bool eligible(const Eligibility& e){return e.prophecies&&e.presearing;}
inline bool matches(const Interaction& npc){return npc.selected.instance&&npc.selected.actor&&npc.selected.generation&&
    npc.selected==npc.current&&npc.reachable;}
inline bool valid_progress(const Progress& progress){
    if(!progress.cycle||progress.timed||progress.objective_credits.size()>3)return false;
    for(auto credit:progress.objective_credits)if(credit<1||credit>5)return false;
    if(progress.stage==Stage::active)return progress.objective_credits.size()<3;
    if(progress.stage==Stage::completed||progress.stage==Stage::rewarded)return progress.objective_credits.size()==3;
    return progress.stage==Stage::abandoned;
}
inline std::optional<Profile> plan(const Profile& current,Action action,unsigned cycle,const Eligibility& e,const Interaction& npc){
    validate(current);if(!eligible(e)||!matches(npc))return {};
    if(action==Action::accept){
        if(npc.map!=146||npc.name!="Devona")return {};
        auto next=quests::accept(current,id);if(!next||next->records.at(id).cycle!=cycle)return {};return next;
    }
    const auto found=current.records.find(id);
    if(found==current.records.end()||found->second.cycle!=cycle||!valid_progress(found->second))return {};
    if(action==Action::question){
        const auto target=witness(npc.map,npc.name);if(!target)return {};
        return credit_objective(current,id,cycle,target->credit,3);
    }
    if(action==Action::reward&&npc.map==146&&npc.name=="Devona")
        return advance(current,id,cycle,Stage::completed,Stage::rewarded);
    return {};
}
// Storage preserves exact distinct-witness sets and prohibits reward via save.
// NPC scope/reachability remain controller authority, never client-supplied.
inline bool valid_transition(const Profile& previous,const Profile& next){
    validate(previous);validate(next);const auto before=previous.records.find(id),after=next.records.find(id);
    if(before==previous.records.end()&&after==next.records.end())return true;
    if(before!=previous.records.end()&&after!=next.records.end()&&before->second==after->second)return true;
    if(after==next.records.end()||!valid_progress(after->second))return false;
    auto proposed=quests::accept(previous,id);if(proposed&&*proposed==next)return true;
    if(before==previous.records.end()||!valid_progress(before->second))return false;
    proposed=quests::abandon(previous,id,before->second.cycle);if(proposed&&*proposed==next)return true;
    for(const auto& target:witnesses){
        proposed=credit_objective(previous,id,before->second.cycle,target.credit,3);
        if(proposed&&*proposed==next)return true;
    }
    return false;
}
// Captured markers: old world25387, latest packets72402/87821/88159.
// Count1/2 receipts explicitly carry infinite markers in Green Hills.
inline Catalog bind_markers(const Catalog& source,unsigned map){
    auto result=source;const auto found=result.find(id);if(found==result.end())return result;
    auto bind=[&](Content& content,bool completed){auto& e=content.entry;e.map_to=completed?146:160;
        e.x=e.y=std::bit_cast<float>(0x7f800000u);e.plane=0;
        if(!completed&&map==146){e.x=-14926;e.y=9979;}
        if(completed&&map==160){e.x=22279;e.y=13121;}
        if(completed&&map==164){e.x=-11215;e.y=-6200;}
        if(completed&&map==146){e.x=-7832;e.y=-15081;}
    };
    auto& content=found->second;bind(content,false);
    for(auto& [count,variant]:content.objective_variants){if(!variant)continue;variant=std::make_shared<Content>(*variant);bind(*variant,false);}
    if(content.completed_variant){content.completed_variant=std::make_shared<Content>(*content.completed_variant);bind(*content.completed_variant,true);}
    return result;
}
// Wiki-backed literal reply excerpts (revision2649446), not recovered native text IDs.
// Complete sentences fit the independently bounded256-unit literal dialogue path.
// They are presentation only: the recorded stable witness credit authorizes display.
inline std::optional<std::u16string> reply(const Profile& state,const Catalog& catalog,const Eligibility& e,unsigned map,std::string_view name){
    if(!eligible(e)||!catalog.contains(id))return {};
    const auto person=witness(map,name);const auto found=state.records.find(id);
    if(!person||found==state.records.end()||!valid_progress(found->second)||
       (found->second.stage!=Stage::active&&found->second.stage!=Stage::completed)||
       !found->second.objective_credits.contains(person->credit))return {};
    switch(person->credit){
    case 1:return u"You must be joking. Not since King Doric himself has Ascalon had a finer king than Adelbern. The only good Royalist is a dead Royalist, I say.";
    case 2:return u"From what I have seen, King Adelbern is a man of great compassion. There will always be those malcontents who seek to blame every hardship in life on the current monarch.";
    case 3:return u"Maybe you don't remember how Ascalon was before Adelbern. The crown peddled influence to guilds for the slightest show of coin. Who would wish for a thing like that to return?";
    case 4:return u"You've got a lot of nerve talking about that here. It's true the duke was next in line when the royal family was assassinated, but he fully supported the crowning of Adelbern.";
    case 5:return u"It saddens me greatly to see people use my name and lineage to stir up trouble. Adelbern is a great king, and I see already in his son Rurik the markings of greatness.";
    }
    return {};
}
}
