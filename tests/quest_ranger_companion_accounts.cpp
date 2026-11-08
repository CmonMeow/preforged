#include "accounts.h"
#include "common.h"
#include "framework/module.h"
#include "quest_ranger_companion.h"
#include "gw_quest_catalog.h"
#include "gw_quest_text_binding.h"
#include "gw_profession.h"
#include "gw_storage.h"
#include <filesystem>
#include <iostream>
#include <source_location>
#include <sodium.h>
namespace q=fw::gw::quests;
namespace r=q::ranger_companion;
namespace inv=fw::gw::inventory;
static unsigned checks=0;
static void check(bool ok,const std::source_location at=std::source_location::current()){
    ++checks;if(!ok)throw std::runtime_error("Ranger Companion persistence invariant line "+std::to_string(at.line()));
}
static std::string hex(std::span<const std::uint8_t> bytes){return gwserver::storage_hex(bytes);}
int main(int argc,char** argv)try{
    check(argc==3&&sodium_init()>=0);
    const auto catalog_path=std::filesystem::absolute(argv[2]);
    const auto dir=std::filesystem::absolute(argv[1]).lexically_normal();
    check(dir.filename()=="ranger-companion-accounts");std::filesystem::create_directories(dir);
    const auto database=dir/"state.db";
    for(auto suffix:{"","-wal","-shm","-journal"}){std::error_code error;std::filesystem::remove(database.string()+suffix,error);check(!error);}
    gwserver::OwnedCharacter owner,ranger_owner,preowned;
    {
        fw::Accounts accounts(database.string());
        const auto call=[&](const std::string& op,const std::string& payload){
            const auto result=accounts.execute(op,payload,[]{return true;});check(result.status==FW_OK);return result.payload;
        };
        const auto account=fw::split(call("register","RangerQuestFixture\t71756573742d666978747572652d70617373776f7264"))[0];
        const auto fixture=[&](std::string name,unsigned primary,bool bow){
            const auto character=fw::split(call("gw-character-create",account+"\t"+name))[0];
            gwserver::OwnedCharacter target{gwserver::Identity::from_hex(account),gwserver::Identity::from_hex(character)};
            fw::gw::skills::Profile skills;skills.primary=primary;skills.profession_mask=1u<<primary;
            if(primary==2)skills.learned.emplace(446,1);
            call("gw-character-skills-import",account+"\t"+character+"\t0\t"+hex(fw::gw::skills::encode_profile(skills))+"\t"+std::string(64,'a'));
            auto inventory=inv::baseline();if(bow)inventory=*inv::presearing::creation_weapon(inventory,2,fw::gw::character_name_to_utf16(name,19));
            check(accounts.saveInventory({target,0,inventory},[]{return true;}).status==FW_OK);
            q::Profile quests;quests.records.emplace(primary==2?86:85,q::Progress{1,q::Stage::rewarded});
            check(accounts.saveQuests({target,0,quests},[]{return true;}).status==FW_OK);return target;
        };
        owner=fixture("Companion Trial",1,false);ranger_owner=fixture("Companion Ranger",2,true);preowned=fixture("Companion Preowned",1,false);
        check(accounts.adoptPet({preowned,0,{},fw::gw::pets::Profile{fw::gw::pets::Pet{}}},[]{return true;}).status==FW_OK);
    }
    gwserver::NetworkManager manager(database.string());auto& endpoint=manager.server();
    auto catalog=r::bind_markers(q::bind_catalog_character(q::load_catalog(catalog_path),u"Companion Trial"),162);
    const r::Interaction nente{{1,23001,1},{1,23001,1},"Master Ranger Nente",162,true};
    const auto inventory=endpoint.characterInventory(owner);check(inventory.status==FW_OK&&inventory.record.has_value());
    inv::identity::Session identities;auto wire=identities.hydrate(inventory.record->profile,fw::gw::storage::baseline());
    auto character=wire.character,bank=wire.bank;auto revision=inventory.record->revision;
    const auto before=endpoint.characterQuests(owner);check(before.record.has_value());
    const auto baseline_xp=endpoint.characterExperience(owner);check(baseline_xp.value.has_value());
    auto bad=catalog;bad.at(76).completed_variant.reset();
    const auto invalid=gwserver::acceptRangerCompanion(endpoint,owner,bad,1,{true,true,1,0},nente,15,identities,character,bank,revision);
    check(invalid.status==FW_INVALID&&invalid.packets.empty()&&revision==inventory.record->revision);
    check(endpoint.characterQuests(owner).record->revision==before.record->revision);
    const auto stale=gwserver::acceptRangerCompanion(endpoint,owner,catalog,2,{true,true,1,0},nente,15,identities,character,bank,revision);
    check(stale.status==FW_CONFLICT&&stale.packets.empty());
    struct RejectQuestSave:gwserver::NetworkServer {
        using NetworkServer::NetworkServer;
        gwserver::RevisionResult saveQuests(const gwserver::SaveQuests&)override{return {FW_CONFLICT,"injected quest CAS conflict"};}
    } rejected(manager);
    const auto identity_revision=identities.revision();const auto wire_before=inv::encode(character);
    const auto conflict=gwserver::acceptRangerCompanion(rejected,owner,catalog,1,{true,true,1,0},nente,15,identities,character,bank,revision);
    check(conflict.status==FW_CONFLICT&&conflict.packets.empty()&&identities.revision()==identity_revision&&inv::encode(character)==wire_before);
    check(endpoint.characterQuests(owner).record->revision==before.record->revision);
    check(endpoint.characterSkills(owner).record->profile.learned.empty());
    check(endpoint.characterInventory(owner).record->revision==inventory.record->revision);
    {
        const auto skills=endpoint.characterSkills(owner);check(skills.record.has_value());
        const auto proposed=q::accept(before.record->profile,76);check(proposed.has_value());
        const auto bow=r::grant_bow(inventory.record->profile);check(bow&&bow->item);
        gwserver::SaveQuests request{owner,before.record->revision,*proposed,skills.record->revision,
            {r::grants.begin(),r::grants.end()},revision};
        request.acceptance_fill_skillbar=true;request.acceptance_trial=true;request.acceptance_inventory=bow->profile;
        auto wrong=request;wrong.acceptance_skills.pop_back();check(endpoint.saveQuests(wrong).status==FW_INVALID);
        wrong=request;wrong.acceptance_inventory->items.begin()->second.profession=0;check(endpoint.saveQuests(wrong).status==FW_INVALID);
        wrong=request;wrong.acceptance_inventory->items.begin()->second.customized=u"Companion Trial";check(endpoint.saveQuests(wrong).status==FW_INVALID);
        wrong=request;wrong.acceptance_inventory.reset();check(endpoint.saveQuests(wrong).status==FW_INVALID);
        wrong=request;++wrong.expected_inventory_revision;check(endpoint.saveQuests(wrong).status==FW_CONFLICT);
        fw::Accounts direct(database.string());unsigned permissions=0;
        check(direct.saveQuests(request,[&]{return ++permissions<3;}).status==FW_TIMEOUT);
        check(direct.characterQuests(owner,[]{return true;}).record->revision==before.record->revision);
        check(direct.characterSkills(owner,[]{return true;}).record->profile.learned.empty());
        check(direct.characterInventory(owner,[]{return true;}).record->profile.items.empty());
    }
    const auto accepted=gwserver::acceptRangerCompanion(endpoint,owner,catalog,1,{true,true,1,0},nente,15,identities,character,bank,revision);
    check(accepted.status==FW_OK&&!accepted.packets.empty());
    check(accepted.packets.back()[0]==75&&accepted.packets[accepted.packets.size()-2][0]==72);
    check(revision==inventory.record->revision+1&&character.items.size()==1);
    const auto saved_bow=endpoint.characterInventory(owner);check(saved_bow.record.has_value());
    check(saved_bow.record->profile.items.size()==1&&r::has_starter_bow(saved_bow.record->profile));
    const auto& bow=saved_bow.record->profile.items.begin()->second;
    check(bow.profession==2&&!bow.customized&&bow.definition.modifiers.size()==3);
    auto skill=endpoint.characterSkills(owner);check(skill.record.has_value());
    check(skill.record->profile.trial_skills==std::set<std::uint32_t>{411,436,446}&&skill.record->profile.profession_mask==2);
    const auto replay=gwserver::acceptRangerCompanion(endpoint,owner,catalog,1,{true,true,1,0},nente,15,identities,character,bank,revision);
    check(replay.status==FW_CONFLICT&&replay.packets.empty());
    const auto not_tamed=gwserver::completeRangerCompanion(endpoint,owner,catalog,1,true,true);
    check(not_tamed.status==FW_OK&&not_tamed.packets.empty());
    const auto active=endpoint.characterQuests(owner);check(active.record.has_value());
    auto forged=*q::advance(active.record->profile,76,1,q::Stage::active,q::Stage::completed);
    check(endpoint.saveQuests({owner,active.record->revision,forged}).status==FW_INVALID);
    check(gwserver::rewardRangerCompanion(endpoint,owner,1,{true,true,1,0},nente,15).status==FW_CONFLICT);
    // Production adoption endpoint is the durable authority. The shipped
    // Stalker World test separately covers shipped population/reservation and
    // World adoption. This test does not execute the paid Runtime cast path.
    check(endpoint.adoptPet({owner,0,{},fw::gw::pets::Profile{fw::gw::pets::Pet{}}}).status==FW_OK);
    {fw::Accounts reopened(database.string());
        check(reopened.characterPets(owner,[]{return true;}).record->profile.pet.has_value());
        check(reopened.characterQuests(owner,[]{return true;}).record->profile.records.at(76).stage==q::Stage::active);
    }
    const auto old_cycle=gwserver::completeRangerCompanion(endpoint,owner,catalog,2,true,true);
    check(old_cycle.status==FW_CONFLICT&&old_cycle.packets.empty());
    const auto refused_completion=gwserver::completeRangerCompanion(rejected,owner,catalog,1,true,true);
    check(refused_completion.status==FW_CONFLICT&&refused_completion.packets.empty());
    check(endpoint.characterQuests(owner).record->profile.records.at(76).stage==q::Stage::active);
    // Four-profession packets30352:16446/16447 at82072343 are modern77/76,
    // native2022 marker76/details75. Snapshot1864 confirms completed76.
    // Preserve that observed pair; never recreate the active quest with72/79.
    const auto completed=gwserver::completeRangerCompanion(endpoint,owner,catalog,1,true,true);
    check(completed.status==FW_OK&&completed.packets.size()==2&&completed.packets[0][0]==76&&completed.packets[1][0]==75);
    const auto& completed_content=*catalog.at(76).completed_variant;
    check(completed.packets==q::Batch{q::marker_2022(completed_content.entry),
        q::details_2022(76,*completed_content.description,*completed_content.objectives)});
    const auto again=gwserver::completeRangerCompanion(endpoint,owner,catalog,1,true,true);
    check(again.status==FW_OK&&again.packets.empty());
    auto ready=endpoint.characterQuests(owner);skill=endpoint.characterSkills(owner);
    gwserver::RewardQuestExperience request{owner,ready.record->revision,76,1,250};request.unlock_trial=true;request.expected_skill_revision=skill.record->revision;
    auto wrong=request;wrong.experience=251;check(endpoint.rewardQuestExperience(wrong).status==FW_INVALID);
    wrong=request;wrong.unlock_trial=false;check(endpoint.rewardQuestExperience(wrong).status==FW_INVALID);
    wrong=request;++wrong.expected_skill_revision;check(endpoint.rewardQuestExperience(wrong).status==FW_CONFLICT);
    {fw::Accounts direct(database.string());unsigned permissions=0;
        check(direct.rewardQuestExperience(request,[&]{return ++permissions<3;}).status==FW_TIMEOUT);
        check(direct.characterNumber(owner,false,[]{return true;}).value==baseline_xp.value);
        check(direct.characterQuests(owner,[]{return true;}).record->profile.records.at(76).stage==q::Stage::completed);
    }
    const auto paid=gwserver::rewardRangerCompanion(endpoint,owner,1,{true,true,1,0},nente,15);
    check(paid.status==FW_OK&&!paid.packets.empty()&&paid.packets.back()==q::remove_2022(76));
    for(const auto& packet:paid.packets)check(packet[0]!=72&&packet[0]!=79);
    check(endpoint.characterExperience(owner).value==*baseline_xp.value+250);
    const auto paid_again=gwserver::rewardRangerCompanion(endpoint,owner,1,{true,true,1,0},nente,15);
    check(paid_again.status==FW_CONFLICT&&paid_again.packets.empty());
    {fw::Accounts reopened(database.string());
        const auto state=reopened.characterSkills(owner,[]{return true;});check(state.record.has_value());
        check(state.record->profile.secondary==0&&state.record->profile.profession_mask==(2u|4u));
        const auto selection=fw::gw::professions::propose({15,2,0},state.record->profile,{}, {15,3,0x1e,1,false,true});
        check(selection.accepted());
        check(reopened.saveSecondaryProfession({owner,state.record->revision,0,selection.skill_profile,selection.attribute_profile},[]{return true;}).status==FW_OK);
    }
    {fw::Accounts reopened(database.string());
        check(reopened.characterSkills(owner,[]{return true;}).record->profile.secondary==2);
        check(reopened.characterSkills(owner,[]{return true;}).record->profile.trial_skills.empty());
        check(reopened.characterQuests(owner,[]{return true;}).record->profile.records.at(76).stage==q::Stage::rewarded);
        check(q::second_profession::objective_ready(reopened.characterQuests(owner,[]{return true;}).record->profile,{true,true,1,2}));
        check(reopened.characterPets(owner,[]{return true;}).record->profile.pet.has_value());
        check(reopened.characterNumber(owner,false,[]{return true;}).value==*baseline_xp.value+250);
    }
    // Existing primary Ranger bow is preserved; no duplicate item/revision.
    const auto initial=endpoint.characterInventory(ranger_owner);check(initial.record.has_value());
    inv::identity::Session ranger_ids;auto ranger_wire=ranger_ids.hydrate(initial.record->profile,fw::gw::storage::baseline());
    auto ranger_revision=initial.record->revision;
    const auto primary=gwserver::acceptRangerCompanion(endpoint,ranger_owner,catalog,1,{true,true,2,0},nente,16,
        ranger_ids,ranger_wire.character,ranger_wire.bank,ranger_revision);
    check(primary.status==FW_OK&&ranger_revision==initial.record->revision&&ranger_wire.character.items.size()==1);
    check(endpoint.adoptPet({ranger_owner,0,{},fw::gw::pets::Profile{fw::gw::pets::Pet{}}}).status==FW_OK);
    const auto prior=endpoint.characterQuests(ranger_owner);auto abandoned=q::abandon(prior.record->profile,76,1);check(abandoned.has_value());
    check(endpoint.saveQuests({ranger_owner,prior.record->revision,*abandoned}).status==FW_OK);
    const auto retained=gwserver::acceptRangerCompanion(endpoint,ranger_owner,catalog,2,{true,true,2,0},nente,16,
        ranger_ids,ranger_wire.character,ranger_wire.bank,ranger_revision);
    check(retained.status==FW_CONFLICT&&retained.packets.empty());
    const auto existing=gwserver::acceptRangerCompanion(endpoint,preowned,catalog,1,{true,true,1,0},nente,17,
        ranger_ids,ranger_wire.character,ranger_wire.bank,ranger_revision);
    check(existing.status==FW_CONFLICT&&existing.packets.empty());
    std::cout<<"PASS Ranger Companion production service/storage "<<checks<<" checks; rendered client flow unverified\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
