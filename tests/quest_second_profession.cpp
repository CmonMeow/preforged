#include "gw_quest_second_profession.h"
#include "gw_quest_catalog.h"
#include <iostream>
#include <source_location>
int main(int argc,char** argv)try{
    using namespace fw::gw::quests;
    namespace second=fw::gw::quests::second_profession;
    unsigned checks=0;
    auto check=[&](bool ok,const std::source_location where=std::source_location::current()){
        ++checks;if(!ok)throw std::runtime_error("quest79 invariant line "+std::to_string(where.line()));};
    check(argc==2);const auto catalog=load_catalog(std::filesystem::path(argv[1]));
    check(catalog.size()==1&&catalog.contains(79)&&catalog.at(79).completed_variant);
    const second::Interaction armin{{1,10021,1},{1,10021,1},"Armin Saberlin",148,true};
    for(unsigned primary=1;primary<=6;++primary)for(unsigned selected=1;selected<=6;++selected){
        if(primary==selected)continue;
        Profile current;current.records.emplace(second::primary_tests[primary],Progress{1,Stage::rewarded});
        const second::Eligibility unselected{true,true,primary,0},chosen{true,true,primary,selected};
        check(second::eligible(current,unselected));
        check(!second::objective_ready(current,unselected));
        check(!second::plan(current,second::Action::accept,0,unselected,armin));
        const auto active=*second::plan(current,second::Action::accept,1,unselected,armin);
        check(second::valid_transition(current,active,unselected));
        check(!second::plan(active,second::Action::complete,1,unselected));
        check(!second::plan(active,second::Action::complete,1,chosen));
        auto ready=active;ready.records.emplace(second::secondary_training[selected],Progress{1,Stage::rewarded});
        const auto complete=*second::plan(ready,second::Action::complete,1,chosen);
        check(second::valid_transition(ready,complete,chosen));
        check(!second::valid_transition(ready,complete,unselected));
        check(!second::plan(complete,second::Action::complete,1,chosen));
        check(!second::plan(ready,second::Action::complete,2,chosen));
        const auto restored=decode(encode(complete));check(restored==complete);
        auto stale_npc=armin;++stale_npc.current.generation;
        check(!second::plan(complete,second::Action::reward,1,chosen,stale_npc));
        stale_npc=armin;stale_npc.reachable=false;
        check(!second::plan(complete,second::Action::reward,1,chosen,stale_npc));
        const auto rewarded=*second::plan(complete,second::Action::reward,1,chosen,armin);
        check(!second::valid_transition(complete,rewarded,chosen)); // XP transaction only.
        check(!second::plan(rewarded,second::Action::reward,1,chosen,armin));
        check(!second::plan(rewarded,second::Action::accept,2,chosen,armin));
        const auto abandoned=*abandon(ready,79,1);
        const auto retry=*second::plan(abandoned,second::Action::accept,2,chosen,armin);
        check(second::valid_transition(ready,abandoned,chosen));
        check(second::valid_transition(abandoned,retry,chosen));
        check(!second::plan(retry,second::Action::complete,1,chosen));
        check(second::plan(retry,second::Action::complete,2,chosen).has_value());
        // Already chosen before acceptance follows the same active->completed path.
        auto prior=current;prior.records.emplace(second::secondary_training[selected],Progress{1,Stage::rewarded});
        const auto late_accept=*second::plan(prior,second::Action::accept,1,chosen,armin);
        check(second::plan(late_accept,second::Action::complete,1,chosen).has_value());
        for(auto invalid:{second::Eligibility{false,true,primary,selected},second::Eligibility{true,false,primary,selected},
                         second::Eligibility{true,true,primary,primary},second::Eligibility{true,true,primary,7}})
            check(!second::plan(ready,second::Action::complete,1,invalid));
        const auto packets=restore_2022(active,catalog);
        check(packets.size()==2&&packets[0][0]==72&&packets[1][0]==75);
        const auto& completed_content=*catalog.at(79).completed_variant;
        const Batch update{objectives_2022(79,*completed_content.objectives),marker_2022(completed_content.entry)};
        check(update.size()==2&&update[0][0]==83&&update[1][0]==76);
        for(const auto& packet:update)check(packet[0]!=72&&packet[0]!=79);
        const auto reopened=restore_2022(restored,catalog);
        check(reopened.size()==2&&reopened[0][0]==72&&reopened[1][0]==75);
    }
    const auto original_marker=catalog.at(79).completed_variant->entry.x;
    for(auto map:{146u,148u,160u,161u,164u,165u}){
        const auto bound=second::bind_markers(catalog,map);
        check(std::isfinite(bound.at(79).completed_variant->entry.x));
        check(bound.at(79).completed_variant->entry.map_to==148);
        check(catalog.at(79).completed_variant->entry.x==original_marker);
    }
    for(auto map:{0u,145u,147u,162u,163u,898u}){
        const auto bound=second::bind_markers(catalog,map);
        check(std::bit_cast<unsigned>(bound.at(79).completed_variant->entry.x)==0x7f800000u);
        check(std::bit_cast<unsigned>(bound.at(79).completed_variant->entry.y)==0x7f800000u);
    }
    Profile no_test;check(!second::plan(no_test,second::Action::accept,1,{true,true,1,0},armin));
    std::cout<<"PASS quest79 "<<checks<<" policy, lifetime, cycle, publication and marker checks\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
