#include "gw_quest_wayward_wizard.h"
#include "gw_quest_wayward_reward.h"
#include "gw_quest_catalog.h"
#include "gw_quest_menu.h"
#include "gw_quest_wayward_dialogue.h"
#include "gw_npc_dialogue.h"
#include <iostream>

int main(int argc,char** argv)try{
    using namespace fw::gw::quests;
    namespace wizard=fw::gw::quests::wayward_wizard;
    auto check=[](bool ok){if(!ok)throw std::runtime_error("wayward wizard invariant");};
    check(argc==2);
    const auto catalog=load_catalog(std::filesystem::path(argv[1]));
    const auto& content=catalog.at(55);
    check(content.completed_variant&&content.objective_variants.size()==1);
    check(presearing::choices({},catalog,1,6,148,"Sandre Elek",1).empty());
    check(presearing::choices({},catalog,1,6,148,"Sandre Elek",2).size()==1);
    wizard::Eligibility eligible{true,true,2};
    wizard::Interaction sandre{{1,42,1},{1,42,1},"Sandre Elek",148,true};
    wizard::Interaction ralena{{2,3,1},{2,3,1},"Ralena Stormbringer",165,true};
    wizard::Interaction orion{{3,41,1},{3,41,1},"Orion Elek",161,true};
    check(!wizard::plan({},wizard::Action::accept,0,{true,true,1},sandre));
    auto stale=sandre;stale.current.generation=2;
    check(!wizard::plan({},wizard::Action::accept,0,eligible,stale));
    stale=sandre;stale.current.instance=2;
    check(!wizard::plan({},wizard::Action::accept,0,eligible,stale));
    stale=sandre;stale.reachable=false;
    check(!wizard::plan({},wizard::Action::accept,0,eligible,stale));
    check(!wizard::plan({},wizard::Action::accept,0,eligible,sandre));
    check(!wizard::plan({},wizard::Action::accept,2,eligible,sandre));
    auto state=*wizard::plan({},wizard::Action::accept,1,eligible,sandre);
    {
        const auto before=state;
        const auto refusal=wizard::reminder(state,catalog,eligible,161,"Orion Elek");
        const auto reminder=wizard::reminder(state,catalog,eligible,148,"Sandre Elek");
        check(refusal&&refusal->starts_with(u"Hmph.")&&reminder&&reminder->starts_with(u"Please."));
        // Exercise the existing literal wire path, including its native body
        // bounds. No buttons means this conversation grants no action token.
        for(const auto& body:{*refusal,*reminder}){
            const auto packets=fw::gw::dialogue::open(41,body,{});
            check(!packets.empty()&&packets.back()[0]==128);
            for(const auto& packet:packets)check(packet[0]!=125);
        }
        check(state==before);
        check(!wizard::reminder({},catalog,eligible,161,"Orion Elek"));
        check(!wizard::reminder(state,{},eligible,161,"Orion Elek"));
        check(!wizard::reminder(state,catalog,{false,true,2},161,"Orion Elek"));
        check(!wizard::reminder(state,catalog,{true,false,2},161,"Orion Elek"));
        check(!wizard::reminder(state,catalog,{true,true,1},161,"Orion Elek"));
        check(!wizard::reminder(state,catalog,eligible,165,"Orion Elek"));
        check(!wizard::reminder(state,catalog,eligible,161,"Sandre Elek"));
        check(!wizard::reminder(state,catalog,eligible,165,"Ralena Stormbringer"));
        for(const auto stage:{Stage::completed,Stage::rewarded,Stage::abandoned}){
            auto inactive=state;inactive.records.at(55).stage=stage;
            check(!wizard::reminder(inactive,catalog,eligible,161,"Orion Elek")&&
                  !wizard::reminder(inactive,catalog,eligible,148,"Sandre Elek"));
        }
        auto invalid=state;invalid.records.at(55).objective_credits={wizard::orion_credit};
        check(!wizard::reminder(invalid,catalog,eligible,161,"Orion Elek")&&
              !wizard::reminder(invalid,catalog,eligible,148,"Sandre Elek"));
    }
    check(presearing::choices(state,catalog,1,6,161,"Orion Elek",2).empty());
    check(presearing::choices(state,catalog,1,6,165,"Ralena Stormbringer",2).front().token==presearing::wayward_ralena_token);
    check(!wizard::plan(state,wizard::Action::talk_orion,1,eligible,orion));
    auto initial=restore_2022(state,catalog);
    state=*wizard::plan(state,wizard::Action::talk_ralena,1,eligible,ralena);
    state=decode(encode(state));
    check(state.records.at(55).stage==Stage::active);
    check(!wizard::reminder(state,catalog,eligible,161,"Orion Elek"));
    check(bool(wizard::reminder(state,catalog,eligible,148,"Sandre Elek")));
    check(presearing::choices(state,catalog,1,6,165,"Ralena Stormbringer",2).empty());
    check(presearing::choices(state,catalog,1,6,161,"Orion Elek",2).front().token==presearing::wayward_orion_token);
    check(restore_2022(state,catalog)!=initial);
    check(!wizard::plan(state,wizard::Action::talk_ralena,1,eligible,ralena));
    check(!wizard::plan(state,wizard::Action::talk_orion,2,eligible,orion));
    auto wrong=orion;wrong.map=165;
    check(!wizard::plan(state,wizard::Action::talk_orion,1,eligible,wrong));
    state=*wizard::plan(state,wizard::Action::talk_orion,1,eligible,orion);
    state=decode(encode(state));
    check(state.records.at(55).stage==Stage::completed);
    check(restore_2022(state,catalog)[1]==details_2022(55,*content.completed_variant->description,*content.completed_variant->objectives));
    check(!wizard::plan(state,wizard::Action::reward,1,eligible,sandre));
    const auto inventory=fw::gw::inventory::baseline();
    auto grant=wizard::reward(state,inventory,1,eligible,sandre);
    check(grant&&grant->xp==250&&grant->inventory.items.size()==1&&inventory.items.empty());
    check(grant->inventory.items.at(grant->item).definition.model==2949);
    check(grant->inventory.items.at(grant->item).definition.modifiers==wizard::air_staff(1).modifiers);
    check(!wizard::reward(grant->quests,grant->inventory,1,eligible,sandre));
    auto full=inventory;full.bags.at(1).capacity=1;
    auto occupied=grant->inventory.items.at(grant->item);full.items.emplace(grant->item,occupied);full.next_item=2;
    check(!wizard::reward(state,full,1,eligible,sandre));
    {
        auto packed=full;packed.bags.emplace(42,fw::gw::inventory::Bag{42,1,4,5,0});
        const auto before_inventory=fw::gw::inventory::encode(packed);
        const auto before_quests=encode(state);
        check(!grant->inventory.items.at(grant->item).customized);
        check(!wizard::reward(state,packed,1,eligible,sandre));
        check(fw::gw::inventory::encode(packed)==before_inventory&&encode(state)==before_quests);
        // A later ordinary bag remains usable even with an earlier empty pack.
        packed.bags.emplace(43,fw::gw::inventory::Bag{43,1,2,1,0});
        const auto fallback=wizard::reward(state,packed,1,eligible,sandre);
        check(fallback&&fallback->inventory.items.at(fallback->item).bag==43&&
              !fallback->inventory.items.at(fallback->item).customized&&fallback->xp==250&&
              fallback->quests.records.at(55).stage==Stage::rewarded);
        check(!packed.items.contains(fallback->item)&&state.records.at(55).stage==Stage::completed);
        check(fw::gw::inventory::decode(fw::gw::inventory::encode(fallback->inventory)).items.at(fallback->item).bag==43);
        check(!wizard::reward(fallback->quests,fallback->inventory,1,eligible,sandre));
    }
    auto rewarded=*wizard::plan(state,wizard::Action::reward,1,eligible,sandre,true);
    check(rewarded.records.at(55).stage==Stage::rewarded);
    check(!wizard::plan(rewarded,wizard::Action::reward,1,eligible,sandre,true));
    check(!wizard::plan(rewarded,wizard::Action::accept,0,eligible,sandre));
    state=*abandon(state,55,1);
    const auto abandoned=decode(encode(state));
    for(auto stale_cycle:{0u,1u,3u})check(!wizard::plan(abandoned,wizard::Action::accept,stale_cycle,eligible,sandre));
    check(state==abandoned);
    state=*wizard::plan(abandoned,wizard::Action::accept,2,eligible,sandre);
    check(state.records.at(55).cycle==2&&state.records.at(55).objective_credits.empty());
    check(!wizard::plan(state,wizard::Action::talk_ralena,1,eligible,ralena));
    state.records.at(55).objective_credits={wizard::orion_credit};
    check(!wizard::plan(state,wizard::Action::talk_orion,2,eligible,orion));
    std::cout<<"wayward wizard ordered dialogue, lifetime, persistence and reward guards passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
