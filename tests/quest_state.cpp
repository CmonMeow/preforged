#include "gw_quest_state.h"
#include "gw_quest_restore.h"
#include "gw_quest_request.h"
#include "gw_quest_catalog.h"
#include "gw_presearing_quests.h"
#include "gw_presearing_skills.h"
#include "gw_quest_text_binding.h"
#include "gw_quest_menu.h"
#include "gw_quest_waves.h"
#include "gw_quest_corpse.h"
#include "gw_quest_escort.h"
#include "gw_quest_monk_mission.h"
#include "gw_quest_further_adventures.h"
#include "gw_quest_unsettling_reward.h"
#include "gw_npc_dialogue.h"
#include "gw_npc_catalog.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv)try{
    using namespace fw::gw::quests;
    auto check=[](bool ok){if(!ok)throw std::runtime_error("quest state invariant");};
    check(argc==2);
    {
        namespace mission=fw::gw::quests::monk_mission;
        const auto directory=std::filesystem::path(argv[1]).parent_path();
        const auto standalone=load_catalog(directory/"monk-mission.fwqc");
        check(bool(standalone.at(74).completed_variant));
        check(!load_catalog(directory/"presearing.fwqc").contains(74)&&!presearing::find(74));
        check(mission::experience==250&&mission::acceptance_skills==std::array<unsigned,2>{296,281});
        Profile state;
        check(!mission::eligible(state,{true,true,3,0}));
        state.records[84]={1,Stage::rewarded};
        check(mission::eligible(state,{true,true,3,0})&&!mission::eligible(state,{false,true,3,0}));
        state.records[85]={1,Stage::rewarded};
        check(mission::eligible(state,{true,true,1,3})&&mission::eligible(state,{true,true,1,0}));
        check(!mission::eligible(state,{true,true,1,6})&&!mission::eligible(state,{true,true,0,3}));
        const mission::Scope owner{8,1,2},paulus{8,19,3},stale{8,19,2};
        check(!mission::Escort::create(owner,{8,1,9},1)&&!mission::Escort::create(owner,{9,19,3},1));
        auto escort=*mission::Escort::create(owner,paulus,1);
        state=*accept(state,74);
        check(!escort.returned(owner,1,paulus)&&!escort.artifact_retrieved(owner,1,paulus));
        check(!escort.discover(owner,2,paulus)&&!escort.discover(owner,1,stale));
        check(escort.discover(owner,1,paulus)&&!escort.discover(owner,1,paulus));
        check(escort.may_move(owner,1,paulus,true,true)&&!escort.may_move(owner,1,paulus,false,true)&&
              !escort.may_move(owner,1,paulus,true,false));
        check(!escort.complete(state,owner,1,paulus)&&!escort.returned(owner,1,paulus));
        check(escort.artifact_retrieved(owner,1,paulus)&&escort.returned(owner,1,paulus));
        check(!escort.may_move(owner,1,paulus,true,true));
        const auto completed=escort.complete(state,owner,1,paulus);
        check(completed.has_value()&&!escort.complete(*completed,owner,1,paulus));
        check(!escort.invalidated(stale)&&escort.invalidated(paulus));
        check(!escort.complete(state,owner,1,paulus)&&!escort.returned(owner,1,paulus));
        auto dead=*mission::Escort::create(owner,paulus,1);
        check(dead.invalidated(owner)&&!dead.discover(owner,1,paulus));
    }



    {
        namespace op=fw::gw::quests::opposition;
        const auto catalog=load_catalog(std::filesystem::path(argv[1]).parent_path()/"opposition-to-the-king.fwqc");
        const op::Eligibility eligible{true,true};
        const op::Interaction devona{{1,42,1},{1,42,1},"Devona",146,true};
        check(presearing::choices({},catalog,1,0,146,"Devona").size()==1); // no primary-test requirement
        check(!op::plan({},op::Action::accept,1,{false,true},devona));
        check(!op::plan({},op::Action::accept,1,{true,false},devona));
        const auto accepted=*op::plan({},op::Action::accept,1,eligible,devona);
        check(op::valid_transition({},accepted));
        const auto& content=catalog.at(78);check(content.objective_variants.size()==2&&content.completed_variant);
        auto contact=[](const op::Witness& w){return op::Interaction{{2,10,1},{2,10,1},w.name,w.map,true};};
        // All sixty ordered triples from five eligible locals must work.
        for(const auto& first:op::witnesses)for(const auto& second:op::witnesses)for(const auto& third:op::witnesses){
            if(first.credit==second.credit||first.credit==third.credit||second.credit==third.credit)continue;
            auto state=accepted;unsigned count=0;
            for(const auto* target:{&first,&second,&third}){
                const auto npc=contact(*target);
                check(presearing::choices(state,catalog,1,0,npc.map,npc.name).front().token==presearing::opposition_question_token);
                const auto next=op::plan(state,op::Action::question,1,eligible,npc);check(bool(next)&&op::valid_transition(state,*next));
                state=decode(encode(*next));++count;
                check(state.records.at(78).objective_credits.size()==count);
                check(!op::plan(state,op::Action::question,1,eligible,npc));
                auto respawn=npc;++respawn.selected.generation;++respawn.current.generation;
                check(!op::plan(state,op::Action::question,1,eligible,respawn));
                check(presearing::choices(state,catalog,1,0,npc.map,npc.name).empty());
                const auto reply=op::reply(state,catalog,eligible,npc.map,npc.name);check(bool(reply));
                check(!fw::gw::dialogue::open(npc.selected.actor,*reply,{}).empty());
                check(restore_2022(state,catalog)[1]==details_2022(78,*progress_content(content,state.records.at(78)).description,*progress_content(content,state.records.at(78)).objectives));
            }
            check(state.records.at(78).stage==Stage::completed);
            const auto rewarded=op::plan(state,op::Action::reward,1,eligible,devona);check(bool(rewarded));
            check(!op::valid_transition(state,*rewarded)&&!op::plan(*rewarded,op::Action::reward,1,eligible,devona));
            check(presearing::choices(state,catalog,1,0,146,"Devona").front().reward);
        }
        auto npc=contact(op::witnesses[0]);
        for(unsigned variant=0;variant<5;++variant){auto invalid=npc;
            switch(variant){case 0:++invalid.current.generation;break;case 1:++invalid.current.instance;break;
                case 2:invalid.reachable=false;break;case 3:invalid.map=146;break;case 4:invalid.name="Warmaster Grast";break;}
            check(!op::plan(accepted,op::Action::question,1,eligible,invalid));
        }
        auto injected=accepted;injected.records.at(78).objective_credits={1,2};check(!op::valid_transition(accepted,injected));
        injected.records.at(78).objective_credits={6};check(!op::valid_progress(injected.records.at(78)));
        const auto halfway=*op::plan(accepted,op::Action::question,1,eligible,npc);
        const auto abandoned=*abandon(halfway,78,1);check(op::valid_transition(halfway,abandoned));
        const auto again=*op::plan(abandoned,op::Action::accept,2,eligible,devona);check(op::valid_transition(abandoned,again));
        check(again.records.at(78).objective_credits.empty()&&!op::plan(again,op::Action::question,1,eligible,npc));
        const auto bound=op::bind_markers(catalog,146);check(bound.at(78).completed_variant->entry.x==-7832);
        check(content.completed_variant->entry.x==-14926); // source catalog not mutated by binding
        check(op::bind_markers(catalog,160).at(78).completed_variant->entry.x==22279);
        check(!std::isfinite(op::bind_markers(catalog,163).at(78).entry.x));
        check(!op::reply(accepted,catalog,eligible,160,"Farrah Cappo"));
    }
    {
        namespace rumor=fw::gw::quests::unsettling_rumors;
        // Multiple independently available quests share Devona; do not rely
        // on whole-menu ordering or emptiness when asserting one quest's state.
        const auto installed_menu=load_catalog(std::filesystem::path(argv[1]).parent_path()/"presearing.fwqc");
        Profile shared;shared.records[85]={1,Stage::rewarded};shared.records[54]={1,Stage::active};
        auto choices=presearing::choices(shared,installed_menu,1,0,146,"Devona");
        auto has_quest=[&](unsigned id){return std::any_of(choices.begin(),choices.end(),[&](const auto& c){return c.quest==id;});};
        check(has_quest(54)&&has_quest(62));
        check(has_quest(78));
        shared.records.at(54).stage=Stage::rewarded;
        choices=presearing::choices(shared,installed_menu,1,0,146,"Devona");
        check(!has_quest(54)&&has_quest(62));
        check(has_quest(78));

        const auto catalog=load_catalog(std::filesystem::path(argv[1]).parent_path()/"unsettling-rumors.fwqc");
        const auto& content=catalog.at(62);check(content.completed_variant&&content.objective_variants.size()==1);
        const auto bound=rumor::bind_markers(catalog,148);
        check(!std::isfinite(bound.at(62).entry.x)&&bound.at(62).objective_variants.at(1)->entry.x==12035);
        check(content.objective_variants.at(1)->entry.x==-11215); // unchanged shared source
        const auto abbey=rumor::bind_markers(catalog,164);
        check(abbey.at(62).entry.x==-12342&&abbey.at(62).objective_variants.at(1)->entry.x==-11215);
        check(!std::isfinite(rumor::bind_markers(catalog,165).at(62).completed_variant->entry.x));
        const rumor::Eligibility eligible{true,true,true};
        const rumor::Interaction devona{{1,1,1},{1,1,1},"Devona",146,true};
        const rumor::Interaction meerak{{2,2,1},{2,2,1},"Meerak the Scribe",164,true};
        const rumor::Interaction armin{{3,3,1},{3,3,1},"Armin Saberlin",148,true};
        Profile before;before.records[85]={1,Stage::rewarded};
        check(presearing::choices({},catalog,1,0,146,"Devona").empty());
        check(presearing::choices(before,catalog,1,0,146,"Devona").size()==1);
        check(!rumor::plan(before,rumor::Action::accept,1,{true,true,false},devona));
        check(!rumor::plan(before,rumor::Action::accept,1,{false,true,true},devona));
        check(!rumor::plan(before,rumor::Action::accept,1,{true,false,true},devona));
        auto state=*rumor::plan(before,rumor::Action::accept,1,eligible,devona);
        check(rumor::valid_transition(before,state,true)&&!rumor::valid_transition(before,state,false));
        check(!rumor::plan(state,rumor::Action::talk_armin,1,eligible,armin));
        auto skipped=state;skipped.records.at(62).stage=Stage::completed;skipped.records.at(62).objective_credits={1,2};
        check(!rumor::valid_transition(state,skipped,true));
        for(unsigned variant=0;variant<5;++variant){auto stale=meerak;
            switch(variant){case 0:++stale.current.generation;break;case 1:++stale.current.instance;break;
                case 2:stale.reachable=false;break;case 3:stale.map=146;break;case 4:stale.captured_name="Armin Saberlin";break;}
            check(!rumor::plan(state,rumor::Action::talk_meerak,1,eligible,stale));
        }
        check(presearing::choices(state,catalog,1,0,164,"Meerak the Scribe").front().token==presearing::unsettling_meerak_token);
        const auto active=state;state=*rumor::plan(state,rumor::Action::talk_meerak,1,eligible,meerak);
        check(rumor::valid_transition(active,state,true));state=decode(encode(state));
        check(state.records.at(62).stage==Stage::active&&restore_2022(state,catalog)[1]==details_2022(62,*content.objective_variants.at(1)->description,*content.objective_variants.at(1)->objectives));
        check(!rumor::plan(state,rumor::Action::talk_meerak,1,eligible,meerak));
        check(!rumor::plan(state,rumor::Action::talk_armin,2,eligible,armin));
        check(presearing::choices(state,catalog,1,0,148,"Armin Saberlin").front().token==presearing::unsettling_armin_token);
        const auto halfway=state;state=*rumor::plan(state,rumor::Action::talk_armin,1,eligible,armin);
        check(rumor::valid_transition(halfway,state,true));state=decode(encode(state));
        check(restore_2022(state,catalog)[1]==details_2022(62,*content.completed_variant->description,*content.completed_variant->objectives));
        check(presearing::choices(state,catalog,1,0,148,"Armin Saberlin").front().reward);
        check(!rumor::plan(state,rumor::Action::reward,1,eligible,armin));
        auto inventory=fw::gw::inventory::baseline();const auto grant=rumor::reward(state,inventory,1,eligible,armin);
        check(grant&&grant->xp==250&&grant->inventory.items.at(grant->item).definition.model==2817);
        check(!rumor::valid_transition(state,grant->quests,true));
        check(!rumor::reward(grant->quests,grant->inventory,1,eligible,armin));
        auto full=grant->inventory;full.bags.at(1).capacity=1;
        check(!rumor::reward(state,full,1,eligible,armin));
        const auto abandoned=*abandon(halfway,62,1);check(rumor::valid_transition(halfway,abandoned,true));
        const auto again=*rumor::plan(abandoned,rumor::Action::accept,2,eligible,devona);
        check(rumor::valid_transition(abandoned,again,true)&&again.records.at(62).objective_credits.empty());
        check(!rumor::plan(again,rumor::Action::talk_meerak,1,eligible,meerak));
    }
    {
        namespace trainer=fw::gw::quests::new_elementalist_trainer;
        const auto directory=std::filesystem::path(argv[1]).parent_path();
        const auto installed=load_catalog(directory/"presearing.fwqc");
        const auto standalone=load_catalog(directory/"new-elementalist-trainer.fwqc");
        const auto& content=installed.at(65);const Catalog catalog{{65,content}};
        check(content.description==standalone.at(65).description&&content.objectives==standalone.at(65).objectives&&
              content.entry.name.front()==5760&&content.completed_variant);
        check(content.entry.map_from==146&&content.entry.map_to==161&&content.entry.x==-14107&&content.entry.y==-20101);
        check(content.completed_variant->entry.map_to==161&&content.completed_variant->entry.x==-7774&&
              content.completed_variant->entry.y==1246);
        const auto* rule=presearing::find(65);
        check(rule&&!rule->primary_only&&rule->profession==6&&rule->experience==250&&
              rule->acceptance_skills==std::array<std::uint32_t,3>{}&&rule->reward_skills==std::array<std::uint32_t,3>{});
        const presearing::TrainerStage members[]={{6,true}};
        check(presearing::starter_trainer_visible("Haversdan",members,1));
        const trainer::Eligibility eligible{true,true,6};
        const trainer::Interaction haversdan{{1,39,1},{1,39,1},"Haversdan",146,true};
        const trainer::Interaction aziure{{2,18,1},{2,18,1},"Elementalist Aziure",161,true};
        Profile state;
        check(!trainer::plan(state,trainer::Action::accept,1,eligible,haversdan));
        for(const auto stage:{Stage::active,Stage::completed,Stage::abandoned}){
            state.records[54]={1,stage};
            check(!trainer::plan(state,trainer::Action::accept,1,eligible,haversdan)&&
                  presearing::choices(state,catalog,6,0,146,"Haversdan").empty());
        }
        state.records.at(54).stage=Stage::rewarded;
        for(unsigned primary=1;primary<=5;++primary){
            check(!trainer::plan(state,trainer::Action::accept,1,{true,true,primary},haversdan));
            check(presearing::choices(state,catalog,primary,6,146,"Haversdan").empty());
        }
        check(!trainer::plan(state,trainer::Action::accept,1,{false,true,6},haversdan)&&
              !trainer::plan(state,trainer::Action::accept,1,{true,false,6},haversdan));
        check(presearing::choices(state,catalog,6,0,146,"Haversdan").size()==1&&
              presearing::choices(state,catalog,6,0,161,"Haversdan").empty());
        state=*trainer::plan(state,trainer::Action::accept,1,eligible,haversdan);
        check(presearing::choices(state,catalog,6,0,161,"Elementalist Aziure").front().token==presearing::new_elementalist_aziure_token);
        check(!trainer::plan(state,trainer::Action::reward,1,eligible,aziure));
        auto stale=aziure;++stale.current.generation;
        check(!trainer::plan(state,trainer::Action::talk_aziure,1,eligible,stale));
        stale=aziure;stale.reachable=false;
        check(!trainer::plan(state,trainer::Action::talk_aziure,1,eligible,stale));
        stale=aziure;stale.map=146;
        check(!trainer::plan(state,trainer::Action::talk_aziure,1,eligible,stale));
        check(!trainer::plan(state,trainer::Action::talk_aziure,2,eligible,aziure));
        auto active=state;
        state=*trainer::plan(state,trainer::Action::talk_aziure,1,eligible,aziure);
        check(restore_2022(state,catalog)!=restore_2022(active,catalog));
        check(presearing::choices(state,catalog,6,0,161,"Elementalist Aziure").front().reward);
        check(!trainer::plan(state,trainer::Action::reward,1,eligible,haversdan));
        state=*trainer::plan(state,trainer::Action::reward,1,eligible,aziure);
        check(presearing::choices(state,catalog,6,0,146,"Haversdan").empty()&&
              presearing::choices(state,catalog,6,0,161,"Elementalist Aziure").empty());
        check(!trainer::plan(state,trainer::Action::reward,1,eligible,aziure));
        active=*abandon(active,65,1);
        active=*trainer::plan(active,trainer::Action::accept,2,eligible,haversdan);
        check(!trainer::plan(active,trainer::Action::talk_aziure,1,eligible,aziure));
    }
    {
        namespace further=fw::gw::quests::further_adventures;
        const auto directory=std::filesystem::path(argv[1]).parent_path();
        const auto standalone=load_catalog(directory/"further-adventures.fwqc");
        const auto installed_catalog=load_catalog(directory/"presearing.fwqc");
        const Catalog catalog{{54,installed_catalog.at(54)}}; // isolate this quest's menu fixture
        const auto& content=catalog.at(54);
        check(content.source_sha256==standalone.at(54).source_sha256&&content.description==standalone.at(54).description&&
              content.objectives==standalone.at(54).objectives);
        check(content.completed_variant&&content.entry.map_from==146&&content.entry.map_to==146&&
              content.entry.x==-7832&&content.entry.y==-15081&&content.entry.name.front()==5632);
        check(content.description&&content.description->front()==0x108&&content.description->size()<=127&&
              content.description->find(u"Elementalist")==std::u16string::npos);
        std::ifstream actors(directory/"actors/further-adventures-devona.txt");
        const auto population=fw::gw::world::load_npc_catalog(actors);
        check(population.for_map(146).size()==1&&population.for_map(146).front().captured_name=="Devona"&&
              population.for_map(146).front().definition.model_file==128780);
        std::ifstream installed_actors(directory/"actors/presearing-givers.txt");
        const auto installed=fw::gw::world::load_npc_catalog(installed_actors);
        unsigned devonas=0;
        for(const auto& actor:installed.for_map(146))if(actor.captured_name=="Devona"){
            ++devonas;check(actor.definition.model_file==128780&&actor.x==content.entry.x&&actor.y==content.entry.y);
        }
        check(devonas==1);
        const further::Interaction haversdan{{1,47,1},{1,47,1},"Haversdan",146,true};
        const further::Interaction devona{{1,112,1},{1,112,1},"Devona",146,true};
        constexpr unsigned tests[]{0,85,86,84,82,81,83};
        for(unsigned primary=1;primary<=6;++primary){
            const further::Eligibility eligible{true,true,primary};
            Profile state;
            check(presearing::choices(state,catalog,primary,0,146,"Haversdan").empty());
            check(!further::plan(state,further::Action::accept,1,eligible,haversdan));
            state.records.emplace(tests[primary],Progress{1,Stage::completed});
            check(!further::plan(state,further::Action::accept,1,eligible,haversdan));
            state.records.at(tests[primary]).stage=Stage::rewarded;
            check(presearing::choices(state,catalog,primary,0,146,"Haversdan").size()==1);
            check(presearing::choices(state,catalog,primary,0,148,"Haversdan").empty());
            check(!further::plan(state,further::Action::accept,1,{false,true,primary},haversdan));
            check(!further::plan(state,further::Action::accept,1,{true,false,primary},haversdan));
            check(!further::plan(state,further::Action::accept,2,eligible,haversdan));
            state=*further::plan(state,further::Action::accept,1,eligible,haversdan);
            check(presearing::choices(state,catalog,primary,0,146,"Devona").front().token==presearing::further_devona_token);
            check(!further::plan(state,further::Action::reward,1,eligible,devona));
            auto stale=devona;++stale.current.generation;
            check(!further::plan(state,further::Action::talk_devona,1,eligible,stale));
            stale=devona;stale.reachable=false;
            check(!further::plan(state,further::Action::talk_devona,1,eligible,stale));
            stale=devona;stale.map=55;
            check(!further::plan(state,further::Action::talk_devona,1,eligible,stale));
            check(!further::plan(state,further::Action::talk_devona,2,eligible,devona));
            const auto initial=restore_2022(state,catalog);
            state=*further::plan(state,further::Action::talk_devona,1,eligible,devona);
            check(restore_2022(decode(encode(state)),catalog)!=initial);
            check(presearing::choices(state,catalog,primary,0,146,"Devona").front().reward);
            check(!further::plan(state,further::Action::reward,1,eligible,haversdan));
            const auto rewarded=*further::plan(state,further::Action::reward,1,eligible,devona);
            check(presearing::choices(rewarded,catalog,primary,0,146,"Devona").empty()&&
                  !further::plan(rewarded,further::Action::reward,1,eligible,devona));
            state=*abandon(state,54,1);
            state=*further::plan(state,further::Action::accept,2,eligible,haversdan);
            check(!further::plan(state,further::Action::talk_devona,1,eligible,devona));
        }
    }
    {
        const auto* rule=presearing::find(42);
        check(rule&&rule->profession==2&&rule->offer_npc=="Ivor Trueshot"&&rule->reward_npc=="Ivor Trueshot");
        check(rule->acceptance_skills==std::array<std::uint32_t,3>{407,432,0}&&rule->experience==250);
        const auto staged=load_catalog(std::filesystem::path(argv[1]).parent_path()/"marksmanship.fwqc");
        check(presearing::choices(Profile{},staged,2,0,162,"Ivor Trueshot").size()==1);
        check(presearing::choices(Profile{},staged,2,0,148,"Ivor Trueshot").empty());
    }
    {
        const auto catalog=load_catalog(std::filesystem::path(argv[1]).parent_path()/"power-of-blood.fwqc");
        const auto& content=catalog.at(43);check(content.objective_variants.size()==2&&content.completed_variant);
        auto state=*accept(Profile{},43);
        const auto initial=restore_2022(state,catalog);check(initial.size()==2);
        state=*credit_objective(state,43,1,0x10000001eull,3);
        const auto one=restore_2022(decode(encode(state)),catalog);
        check(one.size()==2&&one[1]!=initial[1]);
        check(one[1]==details_2022(43,*content.objective_variants.at(1)->description,*content.objective_variants.at(1)->objectives));
        state=*credit_objective(state,43,1,0x100000027ull,3);
        const auto two=restore_2022(decode(encode(state)),catalog);check(two[1]!=one[1]);
        state=*credit_objective(state,43,1,0x100000031ull,3);
        const auto completed=restore_2022(decode(encode(state)),catalog);check(completed[1]!=two[1]);
        auto incomplete=catalog;incomplete.at(43).objective_variants.erase(1);
        auto active=*credit_objective(*accept(Profile{},43),43,1,0x10000001eull,3);
        bool rejected=false;try{restore_2022(active,incomplete);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
        std::ifstream input(std::filesystem::path(argv[1]).parent_path()/"power-of-blood.fwqc");
        std::string text((std::istreambuf_iterator<char>(input)),{});
        const auto at=text.find("count:1");check(at!=std::string::npos);text.replace(at,7,"count:2");
        rejected=false;try{std::istringstream invalid(text);read_catalog(invalid);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
    }
    {
        auto state=*accept(Profile{},43);
        const auto legacy=encode(state);check(legacy[3]=='1'&&decode(legacy)==state);
        state=*credit_objective(state,43,1,0x10000001eull,3);
        check(state.records.at(43).stage==Stage::active&&state.records.at(43).objective_credits.size()==1);
        const auto saved=encode(state);check(saved[3]=='2');state=decode(saved);
        check(!credit_objective(state,43,1,0x10000001eull,3));
        check(!credit_objective(state,43,2,0x100000027ull,3));
        state=*credit_objective(state,43,1,0x100000027ull,3);
        state=*credit_objective(state,43,1,0x100000031ull,3);
        check(state.records.at(43).stage==Stage::completed);
        check(!credit_objective(state,43,1,0x100000030ull,3));
        state=decode(encode(state));
        auto rewarded=*advance(state,43,1,Stage::completed,Stage::rewarded);
        check(decode(encode(rewarded))==rewarded);
        auto restarted=*accept(*abandon(state,43,1),43);
        check(restarted.records.at(43).cycle==2&&restarted.records.at(43).objective_credits.empty());
        for(std::size_t size=0;size<saved.size();++size){
            bool rejected=false;try{decode(std::span(saved).first(size));}catch(const std::invalid_argument&){rejected=true;}check(rejected);
        }
        auto trailing=saved;trailing.push_back(0);bool rejected=false;
        try{decode(trailing);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
    }
    {
        const auto content=load_catalog(std::filesystem::path(argv[1]).parent_path()/"presearing.fwqc");
        const auto* rule=presearing::find(49);
        check(rule&&rule->acceptance_skills==std::array<std::uint32_t,3>{28,27,0}&&rule->experience==250);
        Profile state;
        check(presearing::choices(state,content,5,0,165,"Vassar").size()==1);
        check(presearing::choices(state,content,5,0,161,"Vassar").empty());
        state.records.emplace(49,Progress{1,Stage::completed});
        check(presearing::choices(state,content,5,0,165,"Vassar").front().reward);
        state.records.at(49).stage=Stage::rewarded;
        check(presearing::choices(state,content,5,0,165,"Vassar").empty());
    }
    {
        const auto content=load_catalog(std::filesystem::path(argv[1]).parent_path()/"presearing.fwqc");
        const auto* rule=presearing::find(71);check(rule&&rule->acceptance_skills==std::array<std::uint32_t,3>{31,40,76}&&rule->experience==250);
        Profile state;
        check(presearing::choices(state,content,5,0,146,"Lady Althea").empty());
        state.records.emplace(81,Progress{1,Stage::completed});
        check(presearing::choices(state,content,5,0,146,"Lady Althea").empty());
        state.records.at(81).stage=Stage::rewarded;
        const auto offer=presearing::choices(state,content,5,0,146,"Lady Althea");
        check(offer.size()==1&&offer.front().quest==71&&!offer.front().reward);
        check(presearing::choices(state,content,5,0,148,"Lady Althea").empty());
        state.records.emplace(71,Progress{1,Stage::completed});
        const auto reward=presearing::choices(state,content,5,0,146,"Lady Althea");
        check(reward.size()==1&&reward.front().reward);
        state.records.at(71).stage=Stage::rewarded;
        check(presearing::choices(state,content,5,0,146,"Lady Althea").empty());
    }
    {
        using namespace presearing;
        const TrainerStage party[]={{6,false},{3,false},{1,true},{4,false}};
        check(starter_trainer_visible("Howland the Elementalist",party,4));
        check(starter_trainer_visible("Ciglo the Monk",party,4));
        check(!starter_trainer_visible("Van the Warrior",party,4));
        check(!starter_trainer_visible("Artemis the Ranger",party,4));
        check(starter_trainer_visible("Gwen",party,4));
        check(next_guide_visible(party,4));
        const TrainerStage solo[]={{6,false}};
        check(!next_guide_visible(solo,1));
        check(!starter_trainer_visible("Verata the Necromancer",solo,1));
        const TrainerStage rewarded[]={{6,true}};
        check(!starter_trainer_visible("Howland the Elementalist",rewarded,1));
        check(next_guide_visible(rewarded,1));
    }
    {
        using Scope=EscortObjective::Scope;using State=EscortObjective::State;
        const Scope owner{1,1,1},gwen{1,2,1},ciglo{1,3,1};const std::array blockers{Scope{1,4,1},Scope{1,5,1},Scope{1,6,1}};
        auto escort=EscortObjective::create(owner,gwen,ciglo,1,blockers);check(escort.has_value());
        check(!escort->arrive(owner,1,gwen,ciglo)&&!escort->reward_ready());
        check(escort->discover(owner,1,gwen)&&escort->state()==State::blocked);
        auto recycled=blockers[0];++recycled.generation;check(!escort->death(owner,1,recycled)&&!escort->death(owner,2,blockers[0]));
        check(escort->death(owner,1,blockers[0])&&!escort->death(owner,1,blockers[0])&&escort->remaining()==2);
        check(escort->death(owner,1,blockers[1])&&!escort->objective_complete());
        check(escort->death(owner,1,blockers[2])&&escort->objective_complete()&&!escort->reward_ready());
        auto stale_ciglo=ciglo;++stale_ciglo.generation;check(!escort->arrive(owner,1,gwen,stale_ciglo));
        check(escort->arrive(owner,1,gwen,ciglo)&&escort->reward_ready()&&!escort->arrive(owner,1,gwen,ciglo));
        // Arrival remains provisional until reward persistence. The same
        // host death path used for blockers must revoke participant loss.
        for(const auto participant:{owner,gwen,ciglo}){
            auto arrived=*escort;
            auto recycled=participant;++recycled.generation;
            check(!arrived.death(owner,1,recycled)&&!arrived.death(owner,2,participant)&&arrived.reward_ready());
            check(arrived.death(owner,1,participant)&&arrived.state()==State::failed&&!arrived.reward_ready()&&!arrived.objective_complete());
            check(!arrived.death(owner,1,participant)&&!arrived.arrive(owner,1,gwen,ciglo));
        }
        auto vanished=*escort;check(vanished.invalidated(owner,1,gwen)&&!vanished.reward_ready());
        auto undiscovered=*EscortObjective::create(owner,gwen,ciglo,2,blockers);
        check(undiscovered.death(owner,2,gwen)&&undiscovered.state()==State::failed&&!undiscovered.discover(owner,2,gwen));
        auto failed=*EscortObjective::create(owner,gwen,ciglo,2,blockers);check(failed.invalidated(owner,2,gwen)&&!failed.discover(owner,2,gwen)&&!failed.objective_complete());
    }
    Profile empty;check(decode(encode(empty))==empty);
    {
        const auto actors=std::filesystem::path(argv[1]).parent_path()/"actors";
        std::ifstream quest_file(actors.parent_path()/"necromancer-test.fwqc");const auto necromancer=read_catalog(quest_file);
        check(necromancer.size()==1&&necromancer.contains(82));const auto& content=necromancer.at(82);
        check(content.entry.x==4347&&content.entry.y==548&&content.entry.map_to==146&&content.completed_variant&&
              content.description&&content.objectives&&content.completed_variant->description&&content.completed_variant->objectives);
        Profile active;active.records.emplace(82,Progress{1,Stage::active});
        check(restore_2022(active,necromancer).size()==2);
        active.records.at(82).stage=Stage::completed;check(restore_2022(active,necromancer).size()==2);
        std::ifstream bone_file(actors/"necromancer-test-bone.txt"),trainer_file(actors/"necromancer-test-verata.txt");
        const auto bone=fw::gw::world::load_npc_catalog(bone_file),trainer=fw::gw::world::load_npc_catalog(trainer_file);
        check(bone.for_map(146).size()==1&&trainer.for_map(146).size()==1);
        const auto& summon=bone.for_map(146).front();const auto& verata=trainer.for_map(146).front();
        check(summon.captured_name=="Bone Horror"&&summon.definition.model_file==48647&&summon.definition.flags==256&&summon.definition.primary==4&&summon.definition.level==5);
        check(summon.definition.visual_adjustment==std::array<std::uint8_t,4>{0,0,0,150}&&summon.model_files.empty());
        check(verata.captured_name=="Verata the Necromancer"&&verata.definition.model_file==116227&&verata.model_files==std::vector<std::uint32_t>{116709});
        fw::gw::world::Instance instance;instance.configure_npcs({summon,verata});
        std::ifstream skale_file(actors/"necromancer-test-skale.txt");const auto skale=fw::gw::world::load_npc_catalog(skale_file);
        check(skale.maps.size()==1&&skale.for_map(146).size()==4);
        for(const auto& enemy:skale.for_map(146))check(enemy.captured_name=="River Skale Tad"&&enemy.definition.model_file==82023&&enemy.definition.primary==6&&!enemy.combat_definition);
        check(skale.for_map(146)[1].x==4605&&skale.for_map(146)[1].y==581);
        instance.configure_npcs(skale.for_map(146));
        std::ifstream monk_file(actors.parent_path()/"monk-test.fwqc");const auto monk=read_catalog(monk_file);
        check(monk.size()==1&&monk.contains(84)&&monk.at(84).completed_variant&&monk.at(84).description&&monk.at(84).objectives);
        std::ifstream monk_giver_file(actors/"monk-test-givers.txt"),monk_enemy_file(actors/"monk-test-skale.txt");
        const auto monk_givers=fw::gw::world::load_npc_catalog(monk_giver_file),monk_enemies=fw::gw::world::load_npc_catalog(monk_enemy_file);
        check(monk_givers.for_map(146).size()==2&&monk_enemies.for_map(146).size()==3);
        const auto& gwen=monk_givers.for_map(146)[1];check(gwen.captured_name=="Gwen"&&gwen.x==3802&&gwen.y==-4559&&gwen.model_files==std::vector<std::uint32_t>{116759,116760});
        instance.configure_npcs(monk_givers.for_map(146));instance.configure_npcs(monk_enemies.for_map(146));
        std::ifstream ranger_file(actors.parent_path()/"ranger-test.fwqc"),queen_file(actors/"ranger-test-queen.txt"),artemis_file(actors/"ranger-test-artemis.txt");
        const auto ranger=read_catalog(ranger_file);const auto queen=fw::gw::world::load_npc_catalog(queen_file),artemis=fw::gw::world::load_npc_catalog(artemis_file);
        check(ranger.size()==1&&ranger.contains(86)&&ranger.at(86).completed_variant&&queen.for_map(146).size()==1&&artemis.for_map(146).size()==1);
        const auto& monarch=queen.for_map(146).front();check(monarch.captured_name=="River Skale Queen"&&monarch.x==5670&&monarch.y==-4379&&monarch.definition.model_file==82023&&
            monarch.definition.encoded_name==std::u16string{char16_t(3897),char16_t(42335),char16_t(60294),char16_t(2169)});
        instance.configure_npcs(queen.for_map(146));
    }
    {
        using Scope=CorpseObjective::Scope;using State=CorpseObjective::State;
        Scope owner{1,2,1},trainer{1,3,1},enemy{1,4,1},bone{1,5,1};const std::array candidates{enemy};
        auto objective=CorpseObjective::create(owner,trainer,1,candidates);check(objective.has_value());
        check(!objective->summon_published(owner,1,trainer,bone));
        auto stale=enemy;++stale.generation;check(!objective->death(owner,1,stale));
        check(objective->death(owner,1,enemy)&&objective->state()==State::waiting_corpse);
        check(!objective->summon_published(owner,1,trainer,bone));
        check(!objective->corpse_claimed(owner,2,trainer,enemy)&&!objective->corpse_claimed(owner,1,owner,enemy));
        check(objective->corpse_claimed(owner,1,trainer,enemy)&&objective->state()==State::waiting_summon);
        check(!objective->summon_published(owner,1,trainer,enemy));
        check(objective->summon_published(owner,1,trainer,bone)&&objective->state()==State::completed&&objective->summon()==bone);
        check(!objective->summon_published(owner,1,trainer,bone));
        auto lost=*CorpseObjective::create(owner,trainer,2,candidates);
        check(lost.death(owner,2,enemy)&&lost.invalidated(owner,2,enemy)&&lost.state()==State::failed);
        check(!lost.corpse_claimed(owner,2,trainer,enemy));
    }
    {
        using Scope=WaveObjective::Scope;using State=WaveObjective::State;
        const Scope owner{1,2,1},guardian{1,3,1};
        auto objective=WaveObjective::create(owner,guardian,73,1,3,4);check(objective.has_value());
        const std::array<unsigned,3> first{17,24,31};
        for(unsigned wave=0;wave<3;++wave){
            std::array<Scope,4> actors{};for(unsigned i=0;i<4;++i)actors[i]={1,first[wave]+i,1};
            check(objective->state()==State::ready&&objective->wave()==wave);
            auto duplicate=actors;duplicate[3]=duplicate[0];check(!objective->bind(owner,1,duplicate));
            check(!objective->bind(owner,2,actors)&&objective->bind(owner,1,actors));
            auto recycled=actors[0];++recycled.generation;check(!objective->death(owner,1,recycled));
            for(unsigned i=0;i<4;++i){
                check(objective->death(owner,1,actors[i])&&!objective->death(owner,1,actors[i]));
                if(i<3)check(objective->state()==State::fighting&&objective->wave()==wave);
            }
            check(!objective->bind(owner,1,actors));
        }
        check(objective->state()==State::completed&&objective->quest()==73);
        check(objective->death(owner,1,guardian)&&objective->state()==State::failed);
        auto failure=WaveObjective::create(owner,guardian,73,2,3,4);
        const std::array<Scope,4> actors{{{1,17,2},{1,18,2},{1,19,2},{1,20,2}}};
        check(failure->bind(owner,2,actors)&&failure->death(owner,2,guardian));
        for(auto actor:actors)check(!failure->death(owner,2,actor));
        check(failure->state()==State::failed&&!failure->bind(owner,2,actors));
    }
    {
        const auto directory=std::filesystem::path(argv[1]).parent_path();
        const auto installed=load_catalog(directory/"presearing.fwqc");
        for(const unsigned id:{85u,75u,37u,73u,83u})check(installed.contains(id));
        const auto& test=installed.at(83);check(!test.completed_variant&&test.entry.map_to==146);
        const auto active=*accept({},83);const auto completed=*advance(active,83,1,Stage::active,Stage::completed);
        const auto active_packets=restore_2022(active,installed),completed_packets=restore_2022(completed,installed);
        auto expected=test.entry;expected.state=(expected.state&~3u)|2u;
        check(completed_packets.size()==2&&completed_packets[0]==add_2022(expected));
        check(completed_packets[1]==active_packets[1]); // Reuse captured text, no invented completed description.
        std::ifstream input(directory/"actors/presearing-givers.txt");const auto givers=fw::gw::world::load_npc_catalog(input);
        unsigned matches=0;for(const auto& npc:givers.for_map(146))if(npc.captured_name=="Howland the Elementalist"){
            ++matches;check(npc.definition.encoded_name==test.entry.npc&&npc.definition.primary==6&&npc.model_files==std::vector<std::uint32_t>{116715});
        }
        check(matches==1);
    }
    {
        const auto captured=load_catalog(std::filesystem::path(argv[1])/"quest-83.fwqc");
        check(captured.size()==1&&captured.contains(83));
        const auto& content=captured.at(83);
        check(content.source_sha256=="8b64d9637576aeb479a172077fc3390ef3284bdbeed85e5ab358a4e6370cae4b");
        check(content.entry.state==32&&content.entry.map_to==146&&content.entry.map_from==146&&content.entry.plane==0);
        check(std::bit_cast<std::uint32_t>(content.entry.x)==1167075328u&&std::bit_cast<std::uint32_t>(content.entry.y)==3309649920u);
        check(content.entry.location==u"\x3d64"&&content.entry.name==std::u16string({5920,47839,57024,27216})&&
            content.entry.npc==std::u16string({5928,63480,60623,21553}));
        check(content.description&&content.objectives&&!content.completed_variant);
        const auto packets=restore_2022(*accept({},83),captured);
        check(packets.size()==2&&packets[0][0]==72&&packets[1]==details_2022(83,*content.description,*content.objectives));
        check(!marker_reply_2022(*accept({},83),captured,{83,true}).packet->empty());
        std::ifstream giver_input(std::filesystem::path(argv[1])/"howland.txt");
        const auto giver=fw::gw::world::load_npc_catalog(giver_input);
        check(giver.for_map(146).size()==1);
        const auto& howland=giver.for_map(146).front();
        check(howland.captured_name=="Howland the Elementalist"&&howland.definition.encoded_name==content.entry.npc);
        check(howland.definition.model_file==15490&&howland.model_files==std::vector<std::uint32_t>{116715}&&
            howland.definition.primary==6&&howland.definition.level==2&&howland.x==6142&&howland.y==4014);
    }
    // Newly captured profession stages: framing validation is independent from
    // enabling offers or asserting that 2022 resource text renders correctly.
    for(unsigned id:{81u,82u,84u,86u}){
        const auto root=std::filesystem::path(argv[1]);
        const auto a=load_catalog(root/("quest-"+std::to_string(id)+".fwqc"));
        const auto b=load_catalog(root/("quest-"+std::to_string(id)+"-completed.fwqc"));
        check(a.size()==1&&b.size()==1&&a.contains(id)&&b.contains(id));
        check(a.at(id).source_sha256==b.at(id).source_sha256);
        check(a.at(id).description&&a.at(id).objectives&&b.at(id).description&&b.at(id).objectives);
        check(a.at(id).objectives!=b.at(id).objectives);
        const auto active=*accept({},id);
        const auto completed=*advance(active,id,1,Stage::active,Stage::completed);
        check(restore_2022(active,a).size()==2&&restore_2022(completed,b).size()==2);
        const auto* rule=presearing::find(id);check(rule&&rule->primary_only&&rule->experience==500);
        const auto offer=presearing::choices({},a,rule->profession,0,146,rule->offer_npc);
        check(offer.size()==1&&offer[0].quest==id&&!offer[0].reward);
        check(presearing::choices({},a,rule->profession,0,148,rule->offer_npc).empty());
        check(presearing::choices({},a,rule->profession==1?2:1,rule->profession,146,rule->offer_npc).empty());
        check(presearing::choices(active,a,rule->profession,0,146,rule->offer_npc).empty());
        const auto reward=presearing::choices(completed,b,rule->profession,0,146,rule->reward_npc);
        check(reward.size()==1&&reward[0].reward&&reward[0].cycle==1);
    }
    const auto staged_catalog=load_catalog(std::filesystem::path(argv[1])/"presearing-staged.fwqc");
    check(staged_catalog.size()==4);
    const auto staged_session=bind_catalog_character(staged_catalog,u"Cmon Meow");
    for(auto id:{85u,73u,37u,75u}){
        const auto& content=staged_session.at(id);check(content.completed_variant!=nullptr);
        const auto active=*accept({},id);const auto complete=*advance(active,id,1,Stage::active,Stage::completed);
        const auto packets=restore_2022(complete,staged_session);
        check(packets.size()==2&&packets[1]==details_2022(id,*content.completed_variant->description,*content.completed_variant->objectives));
        check(stage_content(content,Stage::active).objectives==content.objectives);
    }
    check(staged_session.at(85).completed_variant->description->find(u"Cmon Meow")!=std::u16string::npos);
    check(staged_catalog.at(85).completed_variant->description->find(u"Logger")!=std::u16string::npos);
    for(auto id:{85u,73u,37u,75u}){
        const auto path=std::filesystem::path(argv[1])/("quest-"+std::to_string(id)+".fwqc");
        auto captured=load_catalog(path);check(captured.size()==1&&captured.contains(id));
        auto& content=captured.at(id);
        check(content.source_sha256=="3388d5edbe0ca0488f91d13244d6ec89670d14bed93b8ef0790b3fdaf7f283ff");
        check(content.description&&content.objectives);
        auto state=Profile{};state.records.emplace(85,Progress{1,Stage::rewarded});
        if(id==85){
            const auto menu=presearing::choices({},captured,1,6,146,"Van the Warrior");
            check(menu.size()==1&&!menu[0].reward&&menu[0].quest==85);
            check(presearing::choices({},captured,1,6,134,"Van the Warrior").empty());
            check(presearing::choices(state,captured,1,6,146,"Van the Warrior").empty());
        }
        if(id==73){
            const std::array<unsigned,7> tests{0,85,86,84,82,81,83};
            for(unsigned primary=1;primary<=6;++primary){
                Profile own;own.records.emplace(tests[primary],Progress{1,Stage::completed});
                check(presearing::choices(own,captured,primary,primary==6?1:6,161,"Elementalist Aziure").empty());
                own.records.at(tests[primary]).stage=Stage::rewarded;
                check(presearing::choices(own,captured,primary,primary==6?1:6,161,"Elementalist Aziure").size()==1);
                own.records.at(tests[primary]).stage=Stage::abandoned;
                check(presearing::choices(own,captured,primary,primary==6?1:6,161,"Elementalist Aziure").empty());
            }
            check(!presearing::primary_test_rewarded(state,0)&&!presearing::primary_test_rewarded(state,7));
            check(!presearing::primary_test_rewarded(state,6)); // Warrior Test cannot unlock a primary Elementalist.
            check(presearing::choices({},captured,1,6,161,"Elementalist Aziure").empty());
            check(presearing::choices(state,captured,1,6,161,"Elementalist Aziure").size()==1);
            check(presearing::choices(state,captured,1,0,161,"Elementalist Aziure").size()==1);
            check(presearing::choices({},captured,1,0,161,"Elementalist Aziure").empty());
            state.records.emplace(73,Progress{2,Stage::completed});
            const auto menu=presearing::choices(state,captured,1,6,161,"Elementalist Aziure");
            check(menu.size()==1&&menu[0].reward&&menu[0].cycle==2);
        }
        if(id==75){
            check(presearing::choices(state,captured,1,6,146,"Warmaster Grast").empty());
            check(presearing::choices({},captured,1,6,160,"Warmaster Grast").empty());
            const auto offer=presearing::choices(state,captured,1,6,160,"Warmaster Grast");
            check(offer.size()==1&&offer[0].quest==75&&!offer[0].reward&&offer[0].cycle==1);
            state.records.emplace(75,Progress{1,Stage::active});
            check(presearing::choices(state,captured,1,6,160,"Warmaster Grast").empty());
            state.records.at(75).stage=Stage::completed;
            const auto reward=presearing::choices(state,captured,1,6,160,"Warmaster Grast");
            check(reward.size()==1&&reward[0].reward&&reward[0].quest==75);
            state.records.at(75).stage=Stage::rewarded;
            check(presearing::choices(state,captured,1,6,160,"Warmaster Grast").empty());
        }
        if(id==85){
            content.captured_character_argument=u"Logger";
            const auto personalized=bind_catalog_character(captured,u"Nineteen Char Names");
            check(personalized.at(id).description->find(u"Logger")==std::u16string::npos);
            check(personalized.at(id).description->find(u"Nineteen Char Names")!=std::u16string::npos);
            const auto active=restore_2022(*accept({},id),personalized);
            check(active.size()==2&&active[0][0]==72&&active[1][0]==75);
        }else check(restore_2022(*accept({},id),captured).size()==2);
    }
    const auto argument=std::u16string{char16_t(0x173b),char16_t(0xe8e3),char16_t(0x10a),char16_t(0xba9),char16_t(0x107)}+u"Logger"+char16_t(1)+char16_t(2);
    const auto rebound=bind_character_argument(argument,u"Logger",u"Cmon Meow");
    check(rebound.substr(0,4)==argument.substr(0,4)&&rebound.back()==2&&argument.find(u"Logger")!=argument.npos);
    check(rebound.find(u"Cmon Meow")!=rebound.npos&&rebound.find(u"Logger")==rebound.npos);
    bool missing_argument=false;try{bind_character_argument(u"Logger",u"Logger",u"Cmon Meow");}catch(const std::invalid_argument&){missing_argument=true;}check(missing_argument);
    bool overflow_argument=false;try{bind_character_argument(std::u16string(120,char16_t(0x173b))+char16_t(0x107)+u"Logger"+char16_t(1),u"Logger",u"Cmon Meow");}catch(const std::invalid_argument&){overflow_argument=true;}check(overflow_argument);
    for(const auto& rule:presearing::rules){
        check(rule.source_sha256.size()==64);
        for(auto skills:{rule.acceptance_skills,rule.reward_skills})for(auto id:skills)
            if(id)check(fw::gw::presearing::obtainable_skill(id));
    }
    const auto& warrior=*presearing::find(85);const auto& experiment=*presearing::find(73);
    check(presearing::eligible(warrior,{1,6,true,true,false}));
    check(!presearing::eligible(warrior,{6,1,true,true,true}));
    check(!presearing::eligible(experiment,{1,6,true,true,false}));
    check(presearing::eligible(experiment,{1,6,true,true,true}));
    check(!presearing::eligible(experiment,{1,6,true,false,true}));
    check(warrior.acceptance_skills[0]==0&&warrior.reward_skills[0]==346);
    check(experiment.acceptance_skills[0]==197&&experiment.reward_skills[0]==0);
    check(presearing::find(37)->offer_npc!=presearing::find(37)->reward_npc);
    const auto& elementalist=*presearing::find(83);
    check(elementalist.experience==500&&elementalist.reward_skills[0]==194&&elementalist.reward_skills[1]==180);
    check(presearing::eligible(elementalist,{6,1,true,true,false}));
    check(!presearing::eligible(elementalist,{1,6,true,true,true}));
    Content staged_test;staged_test.entry.id=83;Catalog elementalist_catalog{{83,staged_test}};
    check(presearing::choices({},elementalist_catalog,6,1,146,"Howland the Elementalist").size()==1);
    check(presearing::choices({},elementalist_catalog,6,1,145,"Howland the Elementalist").empty());
    check(presearing::choices({},elementalist_catalog,1,6,146,"Howland the Elementalist").empty());
    check(presearing::choices({}, {},6,1,146,"Howland the Elementalist").empty());
    const auto row=std::string(64,'a')+"\t101\t32\t2139095040\t2139095040\t0\t72\t72\t.\ta817\t.\t-\t-\n";
    std::istringstream content_input("FWQC1\r\n"+row);auto read=read_catalog(content_input);
    check(read.size()==1&&read.at(101).source_sha256==std::string(64,'a')&&read.at(101).entry.name==u"\x17a8"&&!read.at(101).description);
    auto personalized=read;personalized.at(101).description=argument;personalized.at(101).objectives=u"";
    personalized.at(101).captured_character_argument=u"Logger";
    const auto session_content=bind_catalog_character(personalized,u"Cmon Meow");
    check(session_content.at(101).description==rebound&&!session_content.at(101).captured_character_argument);
    check(personalized.at(101).description==argument&&personalized.at(101).captured_character_argument==u"Logger");
    check(bind_catalog_character(session_content,u"Different Player").at(101).description==rebound);
    auto bound_row=row;bound_row.pop_back();bound_row+="\t4c006f006700670065007200\n";
    bool missing_detail_binding=false;try{std::istringstream stream("FWQC1\n"+bound_row);read_catalog(stream);}catch(const std::invalid_argument&){missing_detail_binding=true;}check(missing_detail_binding);
    std::istringstream duplicate_input("FWQC1\n"+row+row);bool duplicate=false;try{read_catalog(duplicate_input);}catch(const std::invalid_argument&){duplicate=true;}check(duplicate);
    MarkerRequest request{999,false};std::vector<std::uint8_t> frame{18,128,101,0,0,0,1};
    for(unsigned length=0;length<7;++length){check(parse_marker_request_2022(std::span(frame).first(length),request)==fw::gw::world::FrameState::incomplete);check(request.id==999);}
    check(parse_marker_request_2022(frame,request)==fw::gw::world::FrameState::complete&&request.id==101&&request.missing);
    frame[6]=2;check(parse_marker_request_2022(frame,request)==fw::gw::world::FrameState::invalid);
    frame[6]=0;frame[2]=0;check(parse_marker_request_2022(frame,request)==fw::gw::world::FrameState::invalid);
    auto active=*accept(empty,101);check(empty.records.empty()&&active.records.at(101).cycle==1);
    check(!accept(active,101)&&!advance(active,101,1,Stage::completed,Stage::rewarded));
    auto completed=*advance(active,101,1,Stage::active,Stage::completed);
    auto rewarded=*advance(completed,101,1,Stage::completed,Stage::rewarded);
    check(!advance(rewarded,101,1,Stage::completed,Stage::rewarded)&&!accept(rewarded,101)&&!abandon(rewarded,101,1));
    auto repeated=*accept(rewarded,101,true);check(repeated.records.at(101).cycle==2);
    check(!advance(repeated,101,1,Stage::active,Stage::completed)&&!abandon(repeated,101,1));
    check(decode(encode(repeated))==repeated&&decode(encode(rewarded))==rewarded);
    auto abandoned=*abandon(completed,101,1);
    check(abandoned.records.at(101).stage==Stage::abandoned&&!abandon(abandoned,101,1));
    check(decode(encode(abandoned))==abandoned);
    auto reaccepted=*accept(abandoned,101);
    check(reaccepted.records.at(101).cycle==2);
    check(!advance(reaccepted,101,1,Stage::active,Stage::completed)&&!abandon(reaccepted,101,1));
    auto rejects=[&](auto bytes){bool failed=false;try{decode(bytes);}catch(const std::invalid_argument&){failed=true;}check(failed);};
    auto bytes=encode(rewarded);bytes.push_back(0);rejects(bytes);
    bytes=encode(rewarded);bytes.pop_back();rejects(bytes);
    bytes=encode(rewarded);bytes[16]=0;rejects(bytes);
    bytes=encode(rewarded);bytes[16]=5;rejects(bytes);
    bytes=encode(rewarded);bytes[12]=0;rejects(bytes);
    bytes=encode(rewarded);bytes[8]=0;rejects(bytes);
    auto overflow=rewarded;overflow.records.at(101).cycle=UINT32_MAX;check(!accept(overflow,101,true));
    Content content;content.entry.id=101;content.entry.state=0x23;content.description=u"";content.objectives=u"";
    Catalog catalog{{101,content}};
    auto packets=restore_2022(active,catalog);
    auto word=[](const auto& p,unsigned at){std::uint32_t n=0;for(unsigned i=0;i<4;++i)n|=std::uint32_t(p[at+i])<<(8*i);return n;};
    check(packets.size()==2&&packets[0][0]==72&&packets[1][0]==75&&word(packets[0],18)==0x20);
    check(word(restore_2022(completed,catalog)[0],18)==0x22);
    auto reply=marker_reply_2022(active,catalog,{101,true});check(reply.outcome==MarkerOutcome::available&&reply.packet&&(*reply.packet)[0]==76);
    check(marker_reply_2022(rewarded,catalog,{101,true}).outcome==MarkerOutcome::not_owned);
    check(marker_reply_2022(abandoned,catalog,{101,true}).outcome==MarkerOutcome::not_owned);
    check(marker_reply_2022(active,{}, {101,true}).outcome==MarkerOutcome::content_missing);
    auto unloaded=catalog;unloaded.at(101).description.reset();unloaded.at(101).objectives.reset();
    check(restore_2022(active,unloaded).size()==1);
    unloaded.at(101).description=u"";bool partial=false;try{restore_2022(active,unloaded);}catch(const std::invalid_argument&){partial=true;}check(partial);
    check(restore_2022(rewarded,{}).empty()&&restore_2022(abandoned,{}).empty());
    content.entry.x=content.entry.y=std::bit_cast<float>(0x7f800000u);catalog.at(101)=content;
    packets=restore_2022(active,catalog);check(word(packets[0],6)==0x7f800000u&&word(packets[0],10)==0x7f800000u);
    reply=marker_reply_2022(active,catalog,{101,true});check(reply.outcome==MarkerOutcome::unresolved&&!reply.packet);
    check(valid_marker(content.entry.x,content.entry.y)&&!valid_marker(content.entry.x,0.f)&&
        !valid_marker(std::bit_cast<float>(0x7fc00000u),std::bit_cast<float>(0x7fc00000u)));
    bool missing=false;try{restore_2022(active,{});}catch(const std::invalid_argument&){missing=true;}check(missing);
    catalog.at(101).entry.id=102;missing=false;try{restore_2022(active,catalog);}catch(const std::invalid_argument&){missing=true;}check(missing);
    std::cout<<"PASS quest lifecycle, repeat reward denial, cycle fences and profile codec\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

