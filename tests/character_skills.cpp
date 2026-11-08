#include "gw_character_skills.h"
#include "gw_quest_skills.h"
#include "gw_profession.h"
#include "../tools/skills/capture_import.h"
#include <iostream>
#include <sstream>

int main()try{
    using namespace fw::gw::skills;auto check=[](bool condition,const char* message){if(!condition)throw std::runtime_error(message);};
    Profile apprentice;apprentice.primary=6;apprentice.learned={{194,1}};apprentice.bar[0]={194,0};
    const std::array<std::uint32_t,3> lessons{197,180,200};
    const auto taught=fw::gw::quests::teach(apprentice,lessons,true);
    check(taught.bar[0]==apprentice.bar[0]&&taught.bar[1]==Slot{197,0}&&
        taught.bar[2]==Slot{180,0}&&taught.bar[3]==Slot{200,0},"quest lessons fill vacancies in grant order without replacing a build");
    auto cleared=taught;cleared.bar[1]={};
    check(fw::gw::quests::teach(cleared,lessons,true).bar==cleared.bar,"already learned skills are not reinserted");
    const auto learning_only=fw::gw::quests::teach(apprentice,lessons,false);
    check(learning_only.bar==apprentice.bar&&learning_only.learned.size()==4,"learning-only grant retains build");
    Profile trial;trial.primary=1;trial.profession_mask=1u<<1;
    trial.learned={{197,1},{180,1},{200,1}};trial.trial_skills={197,180,200};trial.bar[0]={197,0};
    const auto trial_bytes=encode_profile(trial);const auto trial_restored=decode_profile(trial_bytes);
    check(trial_bytes[3]==2&&trial_restored.trial_skills==trial.trial_skills&&
        trial_restored.secondary==0&&trial_restored.profession_mask==(1u<<1),"trial persistence must not unlock secondary");
    check(validate(trial.bar,policy(trial,0x1e,true))==Reason::profession,
        "trial permissions must default off outside reviewed area");
    check(validate(trial.bar,policy(trial,0x1e,true,false,true))==Reason::accepted,
        "explicit trial area accepts only trial skill grants");
    auto foreign_trial=trial;foreign_trial.learned.emplace(230,1);foreign_trial.bar[0]={230,0};
    check(validate(foreign_trial.bar,policy(foreign_trial,0x1e,true,false,true))==Reason::profession,
        "trial does not authorize all skills of profession");
    auto invalid_trial=trial;invalid_trial.trial_skills.insert(1043);bool trial_rejected=false;
    try{encode_profile(invalid_trial);}catch(const std::invalid_argument&){trial_rejected=true;}
    check(trial_rejected,"non-pre skill trial rejected");
    auto truncated_trial=trial_bytes;truncated_trial.pop_back();trial_rejected=false;
    try{decode_profile(truncated_trial);}catch(const std::invalid_argument&){trial_rejected=true;}
    check(trial_rejected,"truncated trial profile rejected");
    check(encode_profile(apprentice)[3]==1,"ordinary profiles retain v1 compatibility");
    const auto outside=trial_view(trial,false);
    check(outside.trial_skills.empty()&&!outside.bar[0].skill&&trial.bar[0].skill==197&&
        validate(outside.bar,policy(outside,0x1e,true))==Reason::accepted,
        "leaving pre suppresses trial bar without overwriting saved build");
    check(trial_view(trial,true).bar==trial.bar,"returning to pre restores saved trial slots");
    const auto denied_trial_selection=fw::gw::professions::propose({42,6,0},trial,{},
        {42,3,0x1e,1,false,true});
    check(denied_trial_selection.status==fw::gw::professions::Reason::locked&&
        !denied_trial_selection.authoritative.empty(),"trial must reconcile denied profession request without crashing or unlocking");
    check(fw::gw::quests::presearing::trial_area(148)&&fw::gw::quests::presearing::trial_area(161)&&
        !fw::gw::quests::presearing::trial_area(55)&&!fw::gw::quests::presearing::trial_area(0xffffffff),
        "trial area must follow authoritative pre map metadata");
    Profile warrior_trial;warrior_trial.primary=1;warrior_trial.profession_mask=2;
    const auto experiment_trial=fw::gw::quests::teach_trial(warrior_trial,73);
    check(experiment_trial.secondary==0&&experiment_trial.profession_mask==2&&
        experiment_trial.bar[0]==Slot{197,0}&&experiment_trial.bar[1]==Slot{180,0}&&
        experiment_trial.bar[2]==Slot{200,0},"captured Experiment trial grants preserve profession state");
    check(fw::gw::quests::teach_trial(experiment_trial,73).bar==experiment_trial.bar,
        "repeated trial grant must not overwrite build");
    const auto unlocked_trial=fw::gw::quests::unlock_trial(experiment_trial,73);
    check(unlocked_trial.secondary==0&&unlocked_trial.profession_mask==(2u|64u)&&
        unlocked_trial.learned==experiment_trial.learned,"trial unlock must not select profession or add learning");
    const auto mesmer_trial=fw::gw::quests::teach_trial(warrior_trial,71);
    const auto mesmer_unlocked=fw::gw::quests::unlock_trial(mesmer_trial,71);
    check(mesmer_unlocked.secondary==0&&mesmer_unlocked.profession_mask==(2u|32u)&&
        mesmer_unlocked.learned==mesmer_trial.learned&&mesmer_unlocked.bar==mesmer_trial.bar,
        "Mesmer Burden unlock preserves trial learning and bar without selecting secondary");
    check(fw::gw::professions::propose({42,5,0},mesmer_trial,{}, {42,3,0x1e,1,false,true}).status==
        fw::gw::professions::Reason::locked,"unrewarded Mesmer trial cannot select secondary");
    const auto mesmer_selected=fw::gw::professions::propose({42,5,0},mesmer_unlocked,{}, {42,3,0x1e,1,false,true});
    check(mesmer_selected.accepted()&&mesmer_selected.skill_profile.secondary==5&&
        mesmer_selected.skill_profile.trial_skills.empty(),"rewarded Mesmer trial permits normal selection");
    for(auto quest:{42u,43u,49u,72u,74u,76u}){
        bool rejected=false;try{(void)fw::gw::quests::unlock_trial(mesmer_trial,quest);}
        catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"auxiliary or uninstalled quests must not unlock professions");
    }
    for(auto missing:{31u,40u,76u}){
        auto incomplete=mesmer_trial;incomplete.trial_skills.erase(missing);
        bool rejected=false;try{(void)fw::gw::quests::unlock_trial(incomplete,71);}
        catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"Mesmer unlock requires every reviewed trial grant");
    }
    const auto selected_trial=fw::gw::professions::propose({42,6,0},unlocked_trial,{}, {42,3,0x1e,1,false,true});
    check(selected_trial.accepted()&&selected_trial.skill_profile.secondary==6&&selected_trial.skill_profile.trial_skills.empty(),
        "reviewed unlock permits normal selection and retires temporary authorization");
    const auto two_trials=fw::gw::quests::teach_trial(experiment_trial,37);
    fw::gw::quests::Profile remaining_trials;remaining_trials.records={{73,{1,fw::gw::quests::Stage::abandoned}},
        {37,{1,fw::gw::quests::Stage::active}}};
    const auto pruned_trial=fw::gw::quests::prune_trials(two_trials,remaining_trials);
    check(pruned_trial.trial_skills==std::set<std::uint32_t>{230,220}&&
        !pruned_trial.bar[0].skill&&pruned_trial.bar[3].skill==230&&pruned_trial.learned.contains(197),
        "abandonment must retain another trial and learning, clear only revoked slots");
    auto removed_trial=experiment_trial;removed_trial.bar[0]={};
    check(fw::gw::quests::teach_trial(removed_trial,73).bar==removed_trial.bar,
        "ongoing trial grant must not reinsert a manually removed skill");
    auto full=taught;const std::array<std::uint32_t,4> more{230,220,219,202};
    full=fw::gw::quests::teach(full,more,true);
    const std::array<std::uint32_t,1> overflow_lesson{212};
    const auto overflow=fw::gw::quests::teach(full,overflow_lesson,true);
    check(overflow.bar==full.bar&&overflow.learned.contains(212),"full bar still learns without overwriting");
    Profile p;p.primary=8;p.secondary=7;p.profession_mask=0x6fe;p.learned={{3,81},{1239,1}};p.bar[0]={3,80};p.bar[1]={1239,0};
    const auto encoded=encode_profile(p);check(encode_profile(decode_profile(encoded))==encoded,"profile roundtrip preserves count81 and mask lacking primary bit");
    auto bad=encoded;bad.pop_back();bool rejected=false;try{decode_profile(bad);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"truncation rejected");
    p.bar[0].copy=81;rejected=false;try{encode_profile(p);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"copy81 rejected when81 copies indexed0..80");p.bar[0].copy=80;
    const std::array<std::uint32_t,1> previous{281};const auto packets=restore_packets(42,p,previous);check(packets.front()[0]==182&&packets[1][0]==181,"profession ordering");
    check(packets[2][0]==220&&packets[2][4]==0,"old duplicate count cleared");check(packets.back()[0]==217&&packets.back().back()==1,"bar created last");
    const auto cancel=cancellation_packet(42,{5,9});check(cancel==Packet({226,0,42,0,0,0,5,0,9,0,0,0}),"cancel matches pinned serializer");
    Profile split;split.primary=6;split.secondary=3;split.profession_mask=(1u<<6)|(1u<<3);
    split.learned={{865,2},{1397,1}};split.bar[0]={865,1};split.bar[1]={1397,0};
    const auto durable=encode_profile(split);const auto pvp_view=match_profile(split,true);
    check(pvp_view.bar[0]==Slot{3396,1}&&pvp_view.bar[1]==Slot{2871,0},"PvP match uses reciprocal variants and retains copy identity");
    check(pvp_view.learned.at(3396)==2&&pvp_view.learned.at(2871)==1&&pvp_view.learned.at(865)==2,"variant aliases inherit owned counts without removing base learning");
    check(validate(pvp_view.bar,policy(pvp_view,0x1e,false))==Reason::accepted,"effective match policy accepts owned variant bar");
    check(encode_profile(split)==durable&&encode_profile(match_profile(split,false))==durable,"PvP projection never overwrites saved PvE choices");
    split.learned[3396]=1;check(match_profile(split,true).learned.at(3396)==2,"alias counts use maximum rather than adding copies");
    Profile unlearned=split;unlearned.learned.clear();unlearned.bar={};check(match_profile(unlearned,true).learned.empty(),"PvP projection grants no unowned skill");
    Profile pve_only;pve_only.primary=8;pve_only.profession_mask=1u<<8;pve_only.learned={{2100,1}};pve_only.bar[0]={2100,0};
    const auto locked_view=match_profile(pve_only,true);
    check(locked_view.bar==pve_only.bar&&locked_view.learned==pve_only.learned,"PvE-only skill is retained for native lock overlay rather than erased");
    check(policy(locked_view,0x1e,false,true).pvp_match&&!policy(pve_only,0x1e,false).pvp_match,"authoritative PvP cast policy is explicit and defaults to PvE");
    unsigned pairs=0;
    for(const auto& base:skill_catalog_2022){
        if(base.special&0x400000u)continue;
        const auto* variant=find_skill(base.pvp_skill);
        if(!variant||!(variant->special&0x400000u)||variant->pvp_skill!=base.id)continue;
        Profile owned;owned.learned={{base.id,3}};owned.bar[7]={base.id,2};
        const auto projected=match_profile(owned,true);
        check(projected.bar[7]==Slot{variant->id,2}&&projected.learned.at(variant->id)==3,"every native reciprocal split preserves owned copy identity");
        check(match_profile(projected,true).bar==projected.bar,"match projection is idempotent");++pairs;
    }
    check(pairs==154,"all pinned reciprocal PvP pairs covered");
    check(capture::Parser("{\"x\":\"\\uD83D\\uDE00\",\"a\":[1,true,null,0.5]}").parse().at("a").items.size()==4,"bounded parser accepts escaped Unicode and attribute floats");
    for(const auto invalid:{"{\"a\":1,\"a\":2}","[01]","[1,]","{\"x\":\"\\uDC00\"}","true trailing"}){rejected=false;try{capture::Parser(invalid).parse();}catch(const std::runtime_error&){rejected=true;}check(rejected,"invalid JSON rejected");}
    const auto id=fw::gw::parse_token("00112233-4455-6677-8899-aabbccddeeff");check(bool(id),"canonical GUID supported");
    std::istringstream empty("{\"event\":\"start\",\"schema\":3,\"client_sha256\":\"wrong\",\"copy_index_base\":0,\"slot_index_base\":1,\"character_identity_status\":\"own_character_exact_modern_code_verified\"}\n");
    rejected=false;try{capture::read_profile(empty,*id);}catch(const std::runtime_error&){rejected=true;}check(rejected,"missing selected own character not fabricated");
    const std::string start=std::string("{\"event\":\"start\",\"schema\":4,\"client_sha256\":\"")+capture::supported_hash+"\",\"copy_index_base\":0,\"slot_index_base\":1,\"character_identity_status\":\"own_character_exact_modern_code_verified\"}\n";
    std::string snapshot="{\"event\":\"snapshot\",\"character\":{\"guid\":\"00112233-4455-6677-8899-aabbccddeeff\",\"uuid_bytes_hex\":\"33221100554477668899aabbccddeeff\"},\"professions\":{\"primary\":8,\"secondary\":7,\"unlocked_mask\":1790},\"character_learned\":{\"word_count\":1,\"words\":[8],\"copies\":[{\"skill_id\":3,\"count\":81}]},\"equipped_slots\":[";
    for(unsigned slot=1;slot<=8;++slot){if(slot>1)snapshot+=',';snapshot+="{\"slot\":"+std::to_string(slot)+",\"skill_id\":"+(slot==1?"3":"0")+",\"copy_index\":"+(slot==1?"80":"0")+"}";}
    snapshot+="]}\n";std::istringstream valid(start+snapshot+snapshot);
    const auto imported=capture::read_profile(valid,*id);check(imported.learned.at(3)==81&&imported.bar[0].copy==80,"stable exact owned capture imports duplicate learning");
    std::string wrong=start;wrong.replace(wrong.find("\"schema\":4"),10,"\"schema\":3");std::istringstream old(wrong+snapshot);
    rejected=false;try{capture::read_profile(old,*id);}catch(const std::runtime_error&){rejected=true;}check(rejected,"historical invalid capture schema rejected");
    const auto foreign=fw::gw::parse_token("11112233-4455-6677-8899-aabbccddeeff");std::istringstream other(start+snapshot);
    rejected=false;try{capture::read_profile(other,*foreign);}catch(const std::runtime_error&){rejected=true;}check(rejected,"foreign selected GUID never receives own capture");
    auto attribute_snapshot=snapshot;attribute_snapshot.erase(attribute_snapshot.size()-2);attribute_snapshot+=",\"attributes\":[";
    for(unsigned index=0;index<51;++index){if(index)attribute_snapshot+=',';const unsigned base=index==32?3:index==34||index==36?12:0;
        attribute_snapshot+="{\"source_index\":"+std::to_string(index)+",\"attribute_id\":"+std::to_string(base?index:0)+",\"base_level\":"+std::to_string(base)+",\"effective_level\":"+std::to_string(base)+",\"decrement_points\":0,\"increment_points\":0}";}
    attribute_snapshot+="]}\n";std::istringstream unused_rows(attribute_snapshot);
    const auto allocation=capture::read_attributes(unused_rows,*id,20,8,7);
    check(allocation.quest_bonus==30&&allocation.total_points(20)==200&&allocation.ranks[34]==12,"unused zero attribute rows accepted while captured spend proves quest bonus");
    std::cout<<"Character skills codec, restoration, cancellation and bounded JSON tests passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
