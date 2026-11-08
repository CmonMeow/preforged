#pragma once
#include "gw_presearing_quests.h"
#include "gw_quest_restore.h"
#include "gw_quest_wayward_wizard.h"
#include "gw_quest_unsettling_rumors.h"
#include "gw_quest_opposition.h"
#include "gw_quest_new_elementalist_trainer.h"
#include "gw_quest_message_from_friend.h"
#include "gw_quest_path_to_glory.h"
#include <vector>
namespace fw::gw::quests::presearing {
struct Choice {std::uint32_t token,quest,cycle;bool reward;bool operator==(const Choice&)const=default;};
// These are private-server menu tokens, not inferred retail dialog identifiers.
inline constexpr unsigned accept_token=0x7e010000,reward_token=0x7e020000,wayward_ralena_token=0x7e030037,wayward_orion_token=0x7e030137;
inline constexpr unsigned opposition_question_token=0x7e03004e;
inline constexpr unsigned unsettling_meerak_token=0x7e03003e,unsettling_armin_token=0x7e03013e;
inline constexpr unsigned further_devona_token=0x7e030036;
inline constexpr unsigned new_elementalist_aziure_token=0x7e030041;
// Identity cross-reference: Desktop/gw/game info/Quest.cs, named profession
// test entries81..86. This does not enable their uncaptured dialogue/content.
inline bool primary_test_rewarded(const Profile& state,unsigned primary){
    constexpr std::array<unsigned,7> tests{0,85,86,84,82,81,83};
    if(!primary||primary>=tests.size())return false;
    const auto found=state.records.find(tests[primary]);
    return found!=state.records.end()&&found->second.stage==Stage::rewarded;
}
inline std::vector<Choice> choices(const Profile& state,const Catalog& content,
    unsigned primary,unsigned secondary,unsigned map,std::string_view npc,unsigned level=0){
    std::vector<Choice> result;
    const bool primary_test=primary_test_rewarded(state,primary);
    for(const auto& rule:rules){
        if(!content.contains(rule.id))continue;
        if(rule.id==80&&!message_from_friend::admission_open(state,{true,true})&&
           (!state.records.contains(80)||state.records.at(80).stage!=Stage::active))continue;
        if(rule.id==65&&!new_elementalist_trainer::eligible(state,{true,true,primary}))continue;
        if(rule.id==78){
            const auto progress=state.records.find(78);const auto target=opposition::witness(map,npc);
            if(target&&progress!=state.records.end()&&progress->second.stage==Stage::active&&
               opposition::valid_progress(progress->second)&&!progress->second.objective_credits.contains(target->credit))
                result.push_back({opposition_question_token,78,progress->second.cycle,false});
        }
        if(rule.id==62&&primary_test){
            const auto progress=state.records.find(62);
            if(progress!=state.records.end()&&progress->second.stage==Stage::active&&unsettling_rumors::valid_progress(progress->second)){
                if(map==164&&npc=="Meerak the Scribe"&&progress->second.objective_credits.empty())result.push_back({unsettling_meerak_token,62,progress->second.cycle,false});
                if(map==148&&npc=="Armin Saberlin"&&progress->second.objective_credits==std::set<std::uint64_t>{unsettling_rumors::meerak_credit})result.push_back({unsettling_armin_token,62,progress->second.cycle,false});
            }
        }
        if(rule.id==55){
            if(level<2)continue;
            const auto progress=state.records.find(55);
            if(progress!=state.records.end()&&progress->second.stage==Stage::active&&wayward_wizard::valid_progress(progress->second)){
                if(map==165&&npc=="Ralena Stormbringer"&&progress->second.objective_credits.empty())result.push_back({wayward_ralena_token,55,progress->second.cycle,false});
                if(map==161&&npc=="Orion Elek"&&progress->second.objective_credits==std::set<std::uint64_t>{wayward_wizard::ralena_credit})result.push_back({wayward_orion_token,55,progress->second.cycle,false});
            }
        }
        const bool location=((rule.id==79||rule.id==80||rule.id==89)&&map==148)||((rule.primary_only||rule.id==71||rule.id==54)&&map==146)||(rule.id==75&&map==160)||(rule.id==73&&map==161)||
            ((rule.id==42||rule.id==76)&&map==162)||(rule.id==55&&map==148)||(rule.id==43&&map==160)||(rule.id==49&&map==165)||
            (rule.id==78&&map==146)||
            (rule.id==62&&((npc==rule.offer_npc&&map==146)||(npc==rule.reward_npc&&map==148)))||
            (rule.id==65&&((npc==rule.offer_npc&&map==146)||(npc==rule.reward_npc&&map==161)))||
            (rule.id==37&&((npc==rule.offer_npc&&map==165)||(npc==rule.reward_npc&&map==161)));
        // Grawl's captured offer/reward location is Green Hills County160.
        // The session must also require its atomic item/skill/encounter grant.
        if(!location||!eligible(rule,{primary,secondary,true,true,primary_test}))continue;
        const auto progress=state.records.find(rule.id);
        if(rule.id==80&&npc==rule.reward_npc&&progress!=state.records.end()&&progress->second.stage==Stage::active&&
           message_from_friend::valid_progress(progress->second))
            result.push_back({reward_token+rule.id,rule.id,progress->second.cycle,true});
        if(rule.id==89&&npc==rule.reward_npc&&progress!=state.records.end()&&progress->second.stage==Stage::active&&
           path_to_glory::valid_progress(progress->second))
            result.push_back({reward_token+rule.id,rule.id,progress->second.cycle,true});
        if(rule.id==54&&npc==rule.reward_npc&&progress!=state.records.end()&&progress->second.stage==Stage::active&&
           progress->second.objective_credits.empty()&&!progress->second.timed)
            result.push_back({further_devona_token,54,progress->second.cycle,false});
        if(rule.id==65&&npc==rule.reward_npc&&progress!=state.records.end()&&progress->second.stage==Stage::active&&
           progress->second.objective_credits.empty()&&!progress->second.timed)
            result.push_back({new_elementalist_aziure_token,65,progress->second.cycle,false});
        if(npc==rule.offer_npc&&(progress==state.records.end()||(progress->second.stage==Stage::abandoned&&progress->second.cycle!=UINT32_MAX)))
            result.push_back({accept_token+rule.id,rule.id,progress==state.records.end()?1:progress->second.cycle+1,false});
        if(npc==rule.reward_npc&&progress!=state.records.end()&&progress->second.stage==Stage::completed)
            result.push_back({reward_token+rule.id,rule.id,progress->second.cycle,true});
    }
    return result;
}
}
