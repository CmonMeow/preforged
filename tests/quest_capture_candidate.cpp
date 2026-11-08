#include "../tools/quests/capture_candidate.h"
#include <iostream>
int main()try{
    const std::string raw=R"({"event":"quest_snapshot_changed","quests":[{"quest_id":101,"log_state":19,"map_from":72,"map_to":72,"marker":{"x_bits":2139095040,"y_bits":2139095040,"plane":0},"category":{"encoded_utf16":[]},"name":{"encoded_utf16":[6056]},"npc":{"encoded_utf16":[]},"description":{"encoded_utf16":[]},"objectives":{"encoded_utf16":[810,101]}}]})";
    auto snapshot=capture::Parser(raw).parse();const auto hash=std::string(64,'a');
    auto row=capture::quests::candidate(snapshot,101,hash);std::istringstream in(row);auto catalog=fw::gw::quests::read_catalog(in);
    const auto& content=catalog.at(101);if(content.source_sha256!=hash||content.entry.name!=u"\x17a8"||!content.description||!content.description->empty()||*content.objectives!=u"\x32a\x65")throw std::runtime_error("candidate roundtrip");
    auto reject=[&](auto value,unsigned id=101){try{capture::quests::candidate(value,id,hash);}catch(const std::exception&){return;}throw std::runtime_error("invalid evidence accepted");};
    reject(snapshot,102);auto changed=snapshot;changed.members["quests"].items.push_back(changed.at("quests").items[0]);reject(changed);
    changed=snapshot;changed.members["quests"].items[0].members["marker"].members.erase("x_bits");reject(changed);
    changed=snapshot;changed.members["quests"].items[0].members["description"].members["encoded_utf16"]={};reject(changed);
    changed=snapshot;changed.members["quests"].items[0].members["name"].members["encoded_utf16"].items.resize(8);reject(changed);
    changed=snapshot;changed.members["quests"].items[0].members["log_state"].number=18;reject(changed);
    std::cout<<"PASS selected capture candidate, exact text, context marker, missing/duplicate/bounded evidence\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
