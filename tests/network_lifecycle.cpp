#include "gw_equipment_visibility.h"
#include "gw_profession.h"
#include "gw_quest_encounters.h"
#include "network.h"
#include "gw_player_skills_generated.h"
#include "quest_actions.h"
#include "quest_second_profession.h"
#include "quest_message_from_friend.h"
#include "quest_path_to_glory.h"
#include "gw_quest_text_binding.h"
#include "gw_presearing_items.h"
#include "gw_presearing_quests.h"
#include "gw_map_assets_generated.h"
#include "gw_inventory_ground.h"
#include "gw_quest_catalog.h"
#include "gw_world_simulation.h"
#include "server_settings.h"
#include <sstream>
#include <set>
#include "tcp.h"
#include "common.h"
#include "gw_summary_2022.h"
#include "gw_experience_level.h"
#include <filesystem>
#include <iostream>
#include <sodium.h>

int main(int argc,char** argv)try{
    if(argc!=2)throw std::runtime_error("source directory required");
    for(const auto [xp,level]:std::array<std::pair<std::uint32_t,unsigned>,6>{{{0,1},{1999,1},{2000,2},{4599,2},{4600,3},{140600,20}}})
        if(fw::gw::experience::level(xp)!=level)throw std::runtime_error("wiki experience level threshold mismatch");
    fw::gw::Summary2022 level_summary;level_summary.map=148;level_summary.level=1;level_summary.low_nibble=1;
    auto advanced_summary=fw::gw::experience::advance(level_summary.profile(),2000);
    if((advanced_summary.extended[12]>>4&31)!=2||advanced_summary.origin_campaign_2022()!=1||
       (fw::gw::experience::advance(advanced_summary,0).extended[12]>>4&31)!=2)
        throw std::runtime_error("level summary lost campaign or downgraded imported level");
    std::filesystem::current_path(argv[1]);
    if(sodium_init()<0)throw std::runtime_error("random source unavailable");
    gwserver::NetworkManager manager("build/tests/network-lifecycle.db");
    struct RecordingClient:gwserver::NetworkServer {
        using NetworkServer::NetworkServer;
        gwserver::Operation operation{};std::string payload;
        fw::AccountResult request(gwserver::Operation o,const std::string& p) {
            operation=o;payload=p;return {FW_NOT_FOUND,"remote result"};
        }
    } client(manager);
    const auto fixture_id=gwserver::Identity::from_hex("0123456789abcdef0123456789abcdef");
    const auto before_guild_op=client.operation;const auto before_guild_payload=client.payload;
    if(client.guild(fixture_id).status!=FW_NOT_FOUND)throw std::runtime_error("guild read accepted absent owner");
    if(client.operation!=before_guild_op||client.payload!=before_guild_payload||manager.client().guild(fixture_id).status!=FW_UNAVAILABLE)throw std::runtime_error("typed guild read bypassed transport");
    const auto guild_fixture=gwserver::storage_hex(fixture_id.bytes)+"\t4775696c64\t544147\t\t\t186\t1\t"+gwserver::storage_hex(fixture_id.bytes)+"\t4865726f\t0";
    const auto guild_decoded=gwserver::decode_guild({FW_OK,guild_fixture},fixture_id);
    if(guild_decoded.status!=FW_OK||!guild_decoded.value||guild_decoded.value->hall!=186||guild_decoded.value->members.size()!=1||guild_decoded.value->name!="Guild"||gwserver::decode_guild({FW_OK,"0"},fixture_id).value)throw std::runtime_error("guild fields/absence lost");
    for(const auto& bad:std::vector<std::string>{"","1","0\t1",guild_fixture+"\textra"})
        if(gwserver::decode_guild({FW_OK,bad},fixture_id).status!=FW_INTERNAL)throw std::runtime_error("malformed guild accepted");
    if(client.friends(fixture_id).status!=FW_NOT_FOUND)throw std::runtime_error("native friend list accepted missing owner");
    if(client.operation!=before_guild_op||client.payload!=before_guild_payload||manager.client().friends(fixture_id).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed friends bypassed transport");
    const gwserver::FriendTarget blocked_request{fixture_id,fixture_id};
    const gwserver::AddFriend add_request{fixture_id,1,"New Hero","Alias"};
    if(client.addFriend(add_request).status!=FW_NOT_FOUND)throw std::runtime_error("native friend add accepted missing owner");
    if(client.operation!=before_guild_op||client.payload!=before_guild_payload||manager.client().addFriend(add_request).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed friend addition bypassed transport");
    const auto before_bad_friend=client.payload;
    if(client.addFriend({fixture_id,3,"New Hero","Alias"}).status!=FW_INVALID||client.addFriend({fixture_id,1,"Bad\tName",""}).status!=FW_INVALID||client.payload!=before_bad_friend)
        throw std::runtime_error("invalid friend addition reached transport");
    client.removeFriend(blocked_request);
    if(client.operation!=before_guild_op||client.payload!=before_bad_friend||manager.client().removeFriend(blocked_request).status!=FW_UNAVAILABLE||gwserver::decode_friend_remove({FW_OK,"other"}).status!=FW_INTERNAL)
        throw std::runtime_error("typed friend removal contract failed");
    client.friendBlocked(blocked_request);
    if(client.operation!=before_guild_op||client.payload!=before_bad_friend||manager.client().friendBlocked(blocked_request).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed ignore check bypassed transport");
    const auto friend_row=gwserver::storage_hex(fixture_id.bytes)+"\t1\t4865726f\t";
    const auto decoded_friends=gwserver::decode_friends({FW_OK,"1\t"+friend_row});
    if(decoded_friends.status!=FW_OK||decoded_friends.entries.size()!=1||decoded_friends.entries[0].alias!="Hero"||!decoded_friends.entries[0].character.empty()||gwserver::decode_friends({FW_OK,"0"}).status!=FW_OK)
        throw std::runtime_error("friend decoding lost alias/absence");
    for(const auto& malformed:std::vector<std::string>{"","1","111","2\t"+friend_row+"\t"+friend_row,"1\t"+gwserver::storage_hex(fixture_id.bytes)+"\t0\t4865726f\t","1\t"+gwserver::storage_hex(fixture_id.bytes)+"\t1\tzz\t"}){
        const auto result=gwserver::decode_friends({FW_OK,malformed});
        if(result.status!=FW_INTERNAL||!result.entries.empty())throw std::runtime_error("invalid friend response exposed records");
    }
    if(gwserver::decode_friend_blocked({FW_OK,"0"}).value!=false||gwserver::decode_friend_blocked({FW_OK,"1"}).value!=true||gwserver::decode_friend_blocked({FW_OK,"2"}).status!=FW_INTERNAL)
        throw std::runtime_error("ignore boolean decoding failed");
    const std::string registration_password="networktest";
    const auto registration_bytes=std::span(reinterpret_cast<const std::uint8_t*>(registration_password.data()),registration_password.size());
    std::string boundary_name="Boundary";for(unsigned n=0;n<12;++n)boundary_name+=char('a'+randombytes_uniform(26));boundary_name+="@test.example";
    const gwserver::RegisterAccount registration_request{boundary_name,registration_bytes};
    const auto before_registration_op=client.operation;const auto before_registration_payload=client.payload;
    const auto registered_boundary=client.registerAccount(registration_request);
    if(registered_boundary.status!=FW_OK||!registered_boundary.login||registered_boundary.login->identity.name!=gwserver::normalized_account(boundary_name)||manager.server().validateSession({registered_boundary.login->session}).status!=FW_OK||manager.server().logout({registered_boundary.login->session}).status!=FW_OK||client.operation!=before_registration_op||client.payload!=before_registration_payload||manager.client().registerAccount(registration_request).status!=FW_UNAVAILABLE)throw std::runtime_error("native registration or token lifetime failed");
    const auto before_invalid_registration=client.payload;
    if(client.registerAccount({"bad\tname",registration_bytes}).status!=FW_INVALID||client.registerAccount({"valid@example.test",registration_bytes.first(3)}).status!=FW_INVALID||client.payload!=before_invalid_registration)
        throw std::runtime_error("invalid registration reached transport");
    const gwserver::OwnedCharacter inventory_owner{fixture_id,fixture_id};
    const gwserver::GuildCommand guild_command{inventory_owner,gwserver::GuildAction::motd,"Test announcement"};
    const auto before_guild_command_op=client.operation;const auto before_guild_command_payload=client.payload;
    if(client.guildCommand(guild_command).status!=FW_NOT_FOUND)throw std::runtime_error("guild command accepted missing character");
    if(client.operation!=before_guild_command_op||client.payload!=before_guild_command_payload||manager.client().guildCommand(guild_command).status!=FW_UNAVAILABLE)throw std::runtime_error("typed guild command bypassed transport");
    const auto before_bad_guild=client.payload;
    if(client.guildCommand({inventory_owner,static_cast<gwserver::GuildAction>(99),""}).status!=FW_INVALID||client.guildCommand({inventory_owner,gwserver::GuildAction::motd,"Bad\tText"}).status!=FW_INVALID||client.payload!=before_bad_guild)throw std::runtime_error("invalid guild command reached transport");
    if(gwserver::guild_action("motd")!=gwserver::GuildAction::motd||gwserver::guild_action("unknown")||gwserver::decode_guild_command({FW_OK,"bad\tmessage"}).status!=FW_INTERNAL)throw std::runtime_error("guild action/response validation failed");
    const gwserver::PurchaseGuildCape cape_purchase{inventory_owner,4,2000,{}};
    const gwserver::PurchaseGuildTrim trim_purchase{inventory_owner,2,{}};
    client.guildCape(fixture_id);
    if(client.operation!=before_guild_command_op||client.payload!=before_bad_guild||manager.client().guildCape(fixture_id).status!=FW_UNAVAILABLE)throw std::runtime_error("typed cape read bypassed transport");
    client.purchaseGuildCape(cape_purchase);
    if(client.operation!=before_guild_command_op||client.payload!=before_bad_guild||manager.client().purchaseGuildCape(cape_purchase).status!=FW_UNAVAILABLE)throw std::runtime_error("typed cape purchase bypassed transport");
    client.purchaseGuildTrim(trim_purchase);
    if(client.operation!=before_guild_command_op||client.payload!=before_bad_guild||manager.client().purchaseGuildTrim(trim_purchase).status!=FW_UNAVAILABLE)throw std::runtime_error("typed trim purchase bypassed transport");
    const auto before_bad_cape=client.payload;
    auto bad_cape=cape_purchase;bad_cape.quoted_price=1;
    if(client.purchaseGuildCape(bad_cape).status!=FW_INVALID||client.purchaseGuildTrim({inventory_owner,255,{}}).status!=FW_INVALID||client.payload!=before_bad_cape)throw std::runtime_error("invalid cape purchase reached transport");
    if(gwserver::decode_cape({FW_OK,"0"}).value||gwserver::decode_cape({FW_OK,"0000000000000000"}).status!=FW_OK||gwserver::decode_cape({FW_OK,"zz"}).status!=FW_INTERNAL)throw std::runtime_error("cape absence/encoding contract failed");
    const auto cape_result=gwserver::decode_cape_purchase({FW_OK,"5\t98000"},cape_purchase);
    if(cape_result.status!=FW_OK||cape_result.inventory_revision!=5||cape_result.gold!=98000||gwserver::decode_cape_purchase({FW_OK,"4\t98000"},cape_purchase).status!=FW_INTERNAL||gwserver::decode_cape_purchase({FW_OK,"5\t100001"},cape_purchase).status!=FW_INTERNAL)throw std::runtime_error("cape purchase values not validated");
    const gwserver::SaveFactions save_factions{fixture_id,0,{}};
    const gwserver::SaveProgression save_progression{inventory_owner,0,{}};
    const gwserver::AwardFaction award{fixture_id,0,fw::gw::progression::Faction::imperial,100};
    const gwserver::ExchangeImperialFaction exchange{fixture_id,1,fw::gw::progression::Faction::balthazar,100};
    const auto old_faction_save_op=client.operation;const auto old_faction_save_payload=client.payload;
    client.saveFactions(save_factions);
    if(client.operation!=old_faction_save_op||client.payload!=old_faction_save_payload||manager.client().saveFactions(save_factions).status!=FW_UNAVAILABLE)throw std::runtime_error("typed faction save transport");
    const auto old_progression_save_op=client.operation;const auto old_progression_save_payload=client.payload;
    client.saveProgression(save_progression);
    if(client.operation!=old_progression_save_op||client.payload!=old_progression_save_payload||manager.client().saveProgression(save_progression).status!=FW_UNAVAILABLE)throw std::runtime_error("typed progression save transport");
    const auto old_award_op=client.operation;const auto old_award_payload=client.payload;
    client.awardFaction(award);
    if(client.operation!=old_award_op||client.payload!=old_award_payload||manager.client().awardFaction(award).status!=FW_UNAVAILABLE)throw std::runtime_error("typed award transport");
    const auto old_exchange_op=client.operation;const auto old_exchange_payload=client.payload;
    client.exchangeImperialFaction(exchange);
    if(client.operation!=old_exchange_op||client.payload!=old_exchange_payload||manager.client().exchangeImperialFaction(exchange).status!=FW_UNAVAILABLE)throw std::runtime_error("typed exchange transport");
    const auto before_invalid_faction=client.payload;
    if(client.awardFaction({fixture_id,0,static_cast<fw::gw::progression::Faction>(4),100}).status!=FW_INVALID||client.awardFaction({fixture_id,0,fw::gw::progression::Faction::imperial,0}).status!=FW_INVALID||client.exchangeImperialFaction({fixture_id,0,fw::gw::progression::Faction::imperial,100}).status!=FW_INVALID||client.payload!=before_invalid_faction)throw std::runtime_error("invalid faction mutation reached transport");
    const auto old_faction_op=client.operation;const auto old_faction_payload=client.payload;
    client.accountFactions(fixture_id);
    if(client.operation!=old_faction_op||client.payload!=old_faction_payload||manager.client().accountFactions(fixture_id).status!=FW_UNAVAILABLE)throw std::runtime_error("typed faction transport failed");
    const auto old_progression_op=client.operation;const auto old_progression_payload=client.payload;
    client.characterProgression(inventory_owner);
    if(client.operation!=old_progression_op||client.payload!=old_progression_payload||manager.client().characterProgression(inventory_owner).status!=FW_UNAVAILABLE)throw std::runtime_error("typed progression transport failed");
    client.guildFaction(fixture_id);
    if(client.operation!=old_progression_op||client.payload!=old_progression_payload||manager.client().guildFaction(fixture_id).status!=FW_UNAVAILABLE)throw std::runtime_error("typed guild faction transport failed");
    client.tournamentPoints(fixture_id);
    if(client.operation!=old_progression_op||client.payload!=old_progression_payload||manager.client().tournamentPoints(fixture_id).status!=FW_UNAVAILABLE)throw std::runtime_error("typed tournament transport failed");
    const auto faction_profile_fixture=gwserver::storage_hex(fw::gw::progression::encode(fw::gw::progression::Factions{}));
    const auto character_profile_fixture=gwserver::storage_hex(fw::gw::progression::encode(fw::gw::progression::Character{}));
    if(gwserver::decode_factions({FW_OK,"1\t"+faction_profile_fixture}).status!=FW_OK||gwserver::decode_factions({FW_OK,"0"}).record||gwserver::decode_progression({FW_OK,"0\t"+character_profile_fixture}).record->revision!=0)throw std::runtime_error("progression absent/merged state lost");
    for(const auto& bad:std::vector<std::string>{"","1","1\tzz","0\t"+faction_profile_fixture,"1\t00"})
        if(gwserver::decode_factions({FW_OK,bad}).status!=FW_INTERNAL)throw std::runtime_error("malformed factions accepted");
    if(gwserver::decode_guild_faction({FW_OK,"1\t4294967295"}).value->points!=UINT32_MAX||gwserver::decode_guild_faction({FW_OK,"2\t0"}).status!=FW_INTERNAL||gwserver::decode_tournament_points({FW_OK,"0"}).value!=0||gwserver::decode_tournament_points({FW_OK,"4294967296"}).status!=FW_INTERNAL)throw std::runtime_error("progression balance bounds failed");
    const auto before_accountProfile_operation=client.operation;const auto before_accountProfile_payload=client.payload;
    if(client.accountProfile(fixture_id).status!=FW_NOT_FOUND||client.operation!=before_accountProfile_operation||client.payload!=before_accountProfile_payload||manager.client().accountProfile(fixture_id).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed account profile used transitional request");
    const auto before_gameAccountAlias_operation=client.operation;const auto before_gameAccountAlias_payload=client.payload;
    if(client.gameAccountAlias(fixture_id).status!=FW_NOT_FOUND||client.operation!=before_gameAccountAlias_operation||client.payload!=before_gameAccountAlias_payload||manager.client().gameAccountAlias(fixture_id).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed game alias used transitional request");
    std::vector<std::uint8_t> profile_fixture(39);profile_fixture[0]='F';profile_fixture[1]='W';profile_fixture[2]='A';profile_fixture[3]='P';profile_fixture[4]=7;profile_fixture[34]=26;
    const auto profile_response=gwserver::decode_account_profile({FW_OK,gwserver::storage_hex(profile_fixture)});
    if(profile_response.status!=FW_OK||!profile_response.value||profile_response.value->territory!=7||profile_response.value->agreement_version!=26||gwserver::decode_account_profile({FW_OK,"-"}).value)
        throw std::runtime_error("typed account profile lost fields/absence");
    for(const auto& bad:std::vector<std::string>{"","00","zz",std::string(480,'a')})
        if(gwserver::decode_account_profile({FW_OK,bad}).status!=FW_INTERNAL)throw std::runtime_error("malformed account profile accepted");
    if(gwserver::decode_game_alias({FW_OK,"test@example.test"}).value!="test@example.test"||gwserver::decode_game_alias({FW_OK,"bad\talias"}).status!=FW_INTERNAL)
        throw std::runtime_error("typed alias validation failed");
    const auto before_accountCampaigns_operation=client.operation;const auto before_accountCampaigns_payload=client.payload;
    if(client.accountCampaigns(fixture_id).status!=FW_NOT_FOUND||client.operation!=before_accountCampaigns_operation||client.payload!=before_accountCampaigns_payload||manager.client().accountCampaigns(fixture_id).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed campaigns used transitional request");
    const auto before_agreement_operation=client.operation;const auto before_agreement_payload=client.payload;
    if(client.accountAgreement(fixture_id).status!=FW_NOT_FOUND||client.operation!=before_agreement_operation||client.payload!=before_agreement_payload||manager.client().accountAgreement(fixture_id).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed agreement used transitional request");
    if(gwserver::decode_campaigns({FW_OK,"1,2,3,4"}).campaigns!=std::set<std::uint32_t>{1,2,3,4}||
       gwserver::decode_campaigns({FW_OK,""}).status!=FW_OK||gwserver::decode_agreement({FW_OK,"0"}).value!=false||gwserver::decode_agreement({FW_OK,"1"}).value!=true)
        throw std::runtime_error("valid account policy rejected");
    for(const std::string bad:{"0","5","1,1","1,","1,,2","01","1\t2"}){
        const auto decoded=gwserver::decode_campaigns({FW_OK,bad});
        if(decoded.status!=FW_INTERNAL||!decoded.campaigns.empty())throw std::runtime_error("malformed campaigns exposed partial entitlements");
    }
    for(const std::string bad:{"","2","true","01"})
        if(gwserver::decode_agreement({FW_OK,bad}).status!=FW_INTERNAL)throw std::runtime_error("malformed agreement accepted");
    if(gwserver::decode_agreement({FW_NOT_FOUND,"missing"}).value)throw std::runtime_error("missing agreement became accepted");
    const gwserver::RenameCharacter rename_request{inventory_owner,"Old Hero","New Hero"};
    const auto before_rename_op=client.operation;const auto before_rename_payload=client.payload;
    if(client.renameCharacter(rename_request).status!=FW_NOT_FOUND||client.operation!=before_rename_op||client.payload!=before_rename_payload||manager.client().renameCharacter(rename_request).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed rename used transitional request");
    const auto before_invalid_rename=client.payload;
    if(client.renameCharacter({inventory_owner,"Old Hero","New\tHero"}).status!=FW_INVALID||client.payload!=before_invalid_rename)
        throw std::runtime_error("invalid rename reached transport");
    const auto before_unlock_op=client.operation;const auto before_unlock_payload=client.payload;
    if(client.unlockDevelopmentCharacter(inventory_owner).status!=FW_NOT_FOUND||client.operation!=before_unlock_op||client.payload!=before_unlock_payload||manager.client().unlockDevelopmentCharacter(inventory_owner).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed unlock used transitional request");
    const auto unlock=gwserver::decode_development_unlock({FW_OK,"2\t3\t1329"});
    if(unlock.status!=FW_OK||unlock.skill_revision!=2||unlock.attribute_revision!=3||unlock.learned!=1329)throw std::runtime_error("unlock response decode failed");
    for(const std::string bad:{"2\t3","0\t3\t1","2\t3\t-1","2\t3\t3434","2\t3\t1\textra"})
        if(gwserver::decode_development_unlock({FW_OK,bad}).status!=FW_INTERNAL)throw std::runtime_error("malformed unlock response accepted");
    const auto previous_number_op=client.operation;const auto previous_number_payload=client.payload;
    client.characterExperience(inventory_owner);
    if(client.operation!=previous_number_op||client.payload!=previous_number_payload||manager.client().characterExperience(inventory_owner).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed experience bypassed transport");
    client.characterMap(inventory_owner);
    if(client.operation!=previous_number_op||client.payload!=previous_number_payload||manager.client().characterMap(inventory_owner).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed map read bypassed transport");
    const auto old_name_op=client.operation;const auto old_name_payload=client.payload;
    client.characterName(inventory_owner);
    if(client.operation!=old_name_op||client.payload!=old_name_payload||manager.client().characterName(inventory_owner).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed name read bypassed transport");
    if(gwserver::decode_character_number({FW_OK,"-"},false).value||gwserver::decode_character_number({FW_OK,"0"},false).value!=0||
       gwserver::decode_character_number({FW_OK,"4294967295"},false).value!=UINT32_MAX||gwserver::decode_character_number({FW_OK,"55"},true).value!=55)
        throw std::runtime_error("typed character values lost absent/zero/full bounds");
    for(const std::string bad:{"","-1","55x","4294967296","55\t1"})
        if(gwserver::decode_character_number({FW_OK,bad},false).status!=FW_INTERNAL)throw std::runtime_error("malformed experience accepted");
    for(const std::string bad:{"-","0","65536"})
        if(gwserver::decode_character_number({FW_OK,bad},true).status!=FW_INTERNAL)throw std::runtime_error("invalid map accepted");
    for(const auto& bad:std::vector<std::string>{"","bad\tname",std::string(20,'a'),std::string("\xc0\xaf",2)})
        if(gwserver::decode_character_name({FW_OK,bad}).status!=FW_INTERNAL)throw std::runtime_error("invalid name accepted");
    const auto previous_inventory_op=client.operation;const auto previous_inventory_payload=client.payload;
    client.characterInventory(inventory_owner);
    if(client.operation!=previous_inventory_op||client.payload!=previous_inventory_payload||
       manager.client().characterInventory(inventory_owner).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed inventory read bypassed transport");
    const auto previous_storage_op=client.operation;const auto previous_storage_payload=client.payload;
    client.accountStorage(fixture_id);
    if(client.operation!=previous_storage_op||client.payload!=previous_storage_payload||
       manager.client().accountStorage(fixture_id).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed storage read bypassed transport");
    const auto packed_inventory=fw::gw::inventory::encode(fw::gw::inventory::baseline());
    const auto packed_bank=fw::gw::inventory::encode(fw::gw::storage::baseline());
    const auto peer_id=gwserver::Identity::from_hex("fedcba9876543210fedcba9876543210");
    const gwserver::ExchangeTrade trade_request{{inventory_owner,1,{}},{{peer_id,peer_id},2,{}}};
    const auto old_trade_op=client.operation;const auto old_trade_payload=client.payload;
    client.exchangeTrade(trade_request);
    if(client.operation!=old_trade_op||client.payload!=old_trade_payload||
       manager.client().exchangeTrade(trade_request).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed trade bypassed transport");
    auto invalid_trade=trade_request;invalid_trade.right.owner=invalid_trade.left.owner;
    const auto before_invalid_trade=client.payload;
    if(client.exchangeTrade(invalid_trade).status!=FW_INVALID||client.payload!=before_invalid_trade)
        throw std::runtime_error("self trade reached transport");
    invalid_trade=trade_request;invalid_trade.left.offer.items={{1,1},{1,2}};
    if(client.exchangeTrade(invalid_trade).status!=FW_INVALID||client.payload!=before_invalid_trade)
        throw std::runtime_error("duplicate trade offer reached transport");
    const auto trade_response="2\t3\t"+gwserver::storage_hex(packed_inventory)+"\t"+gwserver::storage_hex(packed_inventory);
    const auto trade_decoded=gwserver::decode_trade({FW_OK,trade_response},trade_request);
    if(trade_decoded.status!=FW_OK||!trade_decoded.left||!trade_decoded.right||trade_decoded.left->revision!=2||trade_decoded.right->revision!=3)
        throw std::runtime_error("valid trade result rejected");
    for(const auto& malformed:std::vector<std::string>{"2\t3",trade_response+"\textra","2\t4\t"+gwserver::storage_hex(packed_inventory)+"\tzz"}){
        const auto result=gwserver::decode_trade({FW_OK,malformed},trade_request);
        if(result.status!=FW_INTERNAL||result.left||result.right)throw std::runtime_error("malformed trade exposed partial result");
    }
    const gwserver::SaveInventory save_request{inventory_owner,0,fw::gw::inventory::baseline()};
    const auto old_inv_save_op=client.operation;const auto old_inv_save_payload=client.payload;
    client.saveInventory(save_request);
    if(client.operation!=old_inv_save_op||client.payload!=old_inv_save_payload||
       manager.client().saveInventory(save_request).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed inventory save bypassed transport");
    const gwserver::TransferStorage transfer_request{inventory_owner,1,0,fw::gw::inventory::baseline(),fw::gw::storage::baseline()};
    const auto old_bank_transfer_op=client.operation;const auto old_bank_transfer_payload=client.payload;
    client.transferStorage(transfer_request);
    if(client.operation!=old_bank_transfer_op||client.payload!=old_bank_transfer_payload||
       manager.client().transferStorage(transfer_request).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed storage transfer bypassed transport");
    const auto before_invalid_inventory=client.payload;
    auto bad_inventory=save_request;bad_inventory.profile.gold=100001;
    auto bad_transfer=transfer_request;bad_transfer.storage_revision=INT64_MAX;
    if(client.saveInventory(bad_inventory).status!=FW_INVALID||client.transferStorage(bad_transfer).status!=FW_INVALID||client.payload!=before_invalid_inventory)
        throw std::runtime_error("invalid inventory mutation reached transport");
    for(const std::string response:{"2","2\t1\t3","3\t1","2\t0","2\t-1"})
        if(gwserver::decode_storage_transfer({FW_OK,response},transfer_request).status!=FW_INTERNAL)
            throw std::runtime_error("malformed storage transfer response accepted");
    const auto transfer_result=gwserver::decode_storage_transfer({FW_OK,"2\t1"},transfer_request);
    if(transfer_result.status!=FW_OK||transfer_result.inventory_revision!=2||transfer_result.storage_revision!=1)
        throw std::runtime_error("valid storage transfer response rejected");
    if(!gwserver::decode_inventory({FW_OK,"1\t"+gwserver::storage_hex(packed_inventory)},false).record||
       !gwserver::decode_inventory({FW_OK,"2\t"+gwserver::storage_hex(packed_bank)},true).record)
        throw std::runtime_error("valid inventory profiles rejected");
    if(gwserver::decode_inventory({FW_OK,"0"},false).record)
        throw std::runtime_error("absent inventory became a stored record");
    for(const auto& malformed:std::vector<std::string>{"0\t00","1\tzz","1\t0","-1\t00","1\t00\textra"})
        if(gwserver::decode_inventory({FW_OK,malformed},false).status!=FW_INTERNAL)
            throw std::runtime_error("malformed inventory response accepted");
    const gwserver::CreateCharacter creation{fixture_id,"Typed Hero",fw::gw::CharacterProfile{}};
    const auto before_creation_op=client.operation;const auto before_creation_payload=client.payload;
    if(client.createCharacter(creation).status!=FW_INVALID||client.operation!=before_creation_op||client.payload!=before_creation_payload||manager.client().createCharacter(creation).status!=FW_UNAVAILABLE)
        throw std::runtime_error("typed character creation used transitional request");
    const auto before_invalid=client.payload;
    if(client.createCharacter({fixture_id,"Tab\tName",{}}).status!=FW_INVALID||client.payload!=before_invalid)
        throw std::runtime_error("invalid creation name reached transport");
    const auto old_delete_op=client.operation;const auto old_delete_payload=client.payload;
    client.deleteCharacter({fixture_id,fixture_id});
    if(client.operation!=old_delete_op||client.payload!=old_delete_payload)
        throw std::runtime_error("typed deletion lost owner or character identity");
    client.characterRoster({fixture_id,6});
    if(client.operation!=old_delete_op||client.payload!=old_delete_payload)
        throw std::runtime_error("typed roster used transitional dispatch");
    for(unsigned offset:{1u,7u,9u,UINT32_MAX})
        if(client.characterRoster({fixture_id,offset}).status!=FW_INVALID)
            throw std::runtime_error("invalid roster page reached transport");
    client.selectMap({fixture_id,fixture_id,148,55});
    if(client.operation!=old_delete_op||client.payload!=old_delete_payload)
        throw std::runtime_error("typed map transition used transitional dispatch");
    const auto previous_transition=client.payload;
    for(const auto maps:std::vector<std::pair<unsigned,unsigned>>{{0,55},{148,0},{148,148},{65536,55},{148,65536}})
        if(client.selectMap({fixture_id,fixture_id,maps.first,maps.second}).status!=FW_INVALID||client.payload!=previous_transition)
            throw std::runtime_error("invalid transition reached transport");
    if(manager.client().selectMap({fixture_id,fixture_id,148,55}).status!=FW_UNAVAILABLE||
       gwserver::decode_map_transition({FW_OK,"unexpected"}).status!=FW_INTERNAL)
        throw std::runtime_error("typed transition hid unavailable or malformed response");
    const std::string roster_id="0123456789abcdef0123456789abcdef";
    const auto legacy=gwserver::decode_roster({FW_OK,"1\t1\t"+roster_id+"\tLegacy Hero\t148\t-"},0);
    if(legacy.status!=FW_OK||legacy.entries.size()!=1||legacy.entries[0].profile)
        throw std::runtime_error("legacy roster record lost");
    for(const auto& malformed:std::vector<std::string>{
        "1junk\t0", "9\t0", "1\t0", "0\t0\t", "1\t1\tbad\tHero\t148\t-",
        "1\t1\t"+roster_id+"\tHero\t148x\t-",
        "1\t1\t"+roster_id+"\tHero\t0\t-",
        "1\t1\t"+roster_id+"\tHero\t148\tzz",
        "2\t2\t"+roster_id+"\tHero\t148\t-\t"+roster_id+"\tOther Hero\t148\t-"}){
        const auto rejected=gwserver::decode_roster({FW_OK,malformed},0);
        if(rejected.status!=FW_INTERNAL||!rejected.entries.empty())
            throw std::runtime_error("malformed roster exposed records");
    }
    const auto unavailable=manager.client().characterRoster({fixture_id,0});
    if(unavailable.status!=FW_UNAVAILABLE||!unavailable.entries.empty())
        throw std::runtime_error("typed roster hid unavailable transport");
    const gwserver::CreateCharacter legacy_request{fixture_id,"Typed Hero",{}};
    const auto created_record=gwserver::decode_creation({FW_OK,roster_id+"\tTyped Hero\t148"},legacy_request);
    if(created_record.status!=FW_OK||!created_record.created||created_record.created->name!="Typed Hero")
        throw std::runtime_error("typed creation record lost");
    for(const auto& malformed:std::vector<std::string>{roster_id,roster_id+"\tOther Hero\t148",
        roster_id+"\tTyped Hero\t55",roster_id+"\tTyped Hero\t148junk",
        roster_id+"\tTyped Hero\t148\textra"}){
        const auto rejected=gwserver::decode_creation({FW_OK,malformed},legacy_request);
        if(rejected.status!=FW_INTERNAL||rejected.created)
            throw std::runtime_error("malformed creation exposed a record");
    }
    if(gwserver::decode_deletion({FW_OK,"deleted"}).status!=FW_OK||
       gwserver::decode_deletion({FW_OK,"unexpected"}).status!=FW_INTERNAL||
       gwserver::decode_deletion({FW_NOT_FOUND,"not owned"}).message!="not owned")
        throw std::runtime_error("typed deletion lost completion or error");
    const gwserver::IssueTicket issuance{gwserver::SessionToken::from_hex(std::string(64,'a'))};
    const auto before_issue_op=client.operation;const auto before_issue_payload=client.payload;
    const auto unavailable_issue=client.issueTicket(issuance);
    if(unavailable_issue.status!=FW_NOT_FOUND||unavailable_issue.ticket||client.operation!=before_issue_op||client.payload!=before_issue_payload||manager.client().issueTicket(issuance).status!=FW_UNAVAILABLE)throw std::runtime_error("typed issuance dispatch");
    const auto canonical_ticket=fw::gw::format_token(fw::gw::GameToken{});
    const auto decoded_ticket=gwserver::decode_ticket_issuance({FW_OK,canonical_ticket+"\t123456"});
    if(decoded_ticket.status!=FW_OK||!decoded_ticket.ticket||decoded_ticket.ticket->expires!=123456)throw std::runtime_error("typed issuance result");
    for(const auto& malformed:{canonical_ticket,canonical_ticket+"\t0",canonical_ticket+"\t-1",canonical_ticket+"\t123x",canonical_ticket+"\t18446744073709551615",canonical_ticket+"\t123\textra",std::string("bad\t123")}){
        const auto decoded=gwserver::decode_ticket_issuance({FW_OK,malformed});
        if(decoded.status!=FW_INTERNAL||decoded.ticket)throw std::runtime_error("invalid issuance exposed ticket");
    }
    for(const auto& bad:{std::string(63,'a'),std::string(65,'a'),std::string(64,'g')}){
        bool rejected=false;try{(void)gwserver::SessionToken::from_hex(bad);}catch(const std::invalid_argument&){rejected=true;}
        if(!rejected)throw std::runtime_error("malformed session token accepted");
    }
    const gwserver::AuthenticateTicket auth_request{{},gwserver::Identity::from_hex(std::string(32,'a')),"test",{}};
    const auto before_auth_op=client.operation;const auto before_auth_payload=client.payload;
    const auto auth_reply=client.authenticateTicket(auth_request);
    if(auth_reply.status!=FW_NOT_FOUND||auth_reply.identity||client.operation!=before_auth_op||client.payload!=before_auth_payload)throw std::runtime_error("typed ticket dispatch");
    const auto envelope=std::string(32,'a')+"\ttest\t123456\t\t\t148\t0\t"+std::string(64,'c');
    if(gwserver::decode_ticket_authentication({FW_OK,envelope},auth_request).status!=FW_OK)throw std::runtime_error("typed empty selection");
    for(const auto& malformed:{envelope+"\textra",std::string(32,'b')+envelope.substr(32),std::string(32,'a')+"\ttest\t0\t\t\t148\t0",std::string(32,'a')+"\ttest\t123456\t\tName\t148\t1",std::string(32,'a')+"\ttest\t123456\t\t\t0\t0"}){
        const auto decoded=gwserver::decode_ticket_authentication({FW_OK,malformed},auth_request);
        if(decoded.status!=FW_INTERNAL||decoded.identity)throw std::runtime_error("invalid ticket result exposed identity");
    }
    const std::string boundary_password="networktest";
    const gwserver::LoginAccount boundary_login{"TeSt",std::span(reinterpret_cast<const std::uint8_t*>(boundary_password.data()),boundary_password.size())};
    const auto before_login_op=client.operation;const auto before_login_payload=client.payload;
    const auto remote_login=client.login({boundary_name+"missing",boundary_login.password});
    if(remote_login.status!=FW_NOT_FOUND||remote_login.login||client.operation!=before_login_op||client.payload!=before_login_payload||manager.client().login(boundary_login).status!=FW_UNAVAILABLE)throw std::runtime_error("typed login dispatch");
    const auto login_envelope=std::string(32,'a')+"\ttest\t"+std::string(64,'b')+"\t123456";
    const auto valid_login=gwserver::decode_login({FW_OK,login_envelope},boundary_login);
    if(valid_login.status!=FW_OK||!valid_login.login||valid_login.login->identity.name!="test"||valid_login.login->identity.expires!=123456)throw std::runtime_error("typed login decode");
    for(const auto& malformed:{login_envelope+"\textra",std::string(32,'g')+login_envelope.substr(32),std::string(32,'a')+"\tother\t"+std::string(64,'b')+"\t123456",std::string(32,'a')+"\ttest\tshort\t123456",std::string(32,'a')+"\ttest\t"+std::string(64,'b')+"\t0"}){
        const auto decoded=gwserver::decode_login({FW_OK,malformed},boundary_login);
        if(decoded.status!=FW_INTERNAL||decoded.login)throw std::runtime_error("invalid login exposed session");
    }
    const auto typed_session=valid_login.login->session;
    const auto before_session_op=client.operation;const auto before_session_payload=client.payload;
    if(client.validateSession({typed_session}).status!=FW_NOT_FOUND||client.operation!=before_session_op||client.payload!=before_session_payload||manager.client().validateSession({typed_session}).status!=FW_UNAVAILABLE)throw std::runtime_error("native session crossed transitional transport");
    const auto session_envelope=std::string(32,'a')+"\ttest\t123456";
    if(gwserver::decode_session({FW_OK,session_envelope}).status!=FW_OK)throw std::runtime_error("typed session decode");
    for(const auto& malformed:{session_envelope+"\textra",std::string(32,'a')+"\tTest\t123456",std::string(32,'a')+"\ttest\t-1"}){
        const auto decoded=gwserver::decode_session({FW_OK,malformed});
        if(decoded.status!=FW_INTERNAL||decoded.identity)throw std::runtime_error("invalid session exposed account");
    }
    const auto before_logout_op=client.operation;const auto before_logout_payload=client.payload;
    if(client.logout({typed_session}).status!=FW_NOT_FOUND||client.operation!=before_logout_op||client.payload!=before_logout_payload||manager.client().logout({typed_session}).status!=FW_UNAVAILABLE)throw std::runtime_error("native logout crossed transitional transport");
    if(gwserver::decode_logout({FW_OK,"revoked"}).status!=FW_OK||gwserver::decode_logout({FW_OK,"unexpected"}).status!=FW_INTERNAL)throw std::runtime_error("typed logout completion");
    const auto before_invalid_credentials=client.payload;
    if(client.login({"bad\tname",boundary_login.password}).status!=FW_INVALID||client.payload!=before_invalid_credentials||client.login({"test",{}}).status!=FW_INVALID)throw std::runtime_error("invalid credentials reached dispatch");
    const gwserver::OwnedCharacter skill_owner{{},{}};
    const auto before_skill_operation=client.operation;const auto before_skill_payload=client.payload;
    if(client.characterSkills(skill_owner).status!=FW_NOT_FOUND||client.operation!=before_skill_operation||client.payload!=before_skill_payload)throw std::runtime_error("skills used string dispatch");
    const auto before_attr_op=client.operation;const auto before_attr_payload=client.payload;
    if(client.characterAttributes(skill_owner).status!=FW_NOT_FOUND||client.operation!=before_attr_op||client.payload!=before_attr_payload)throw std::runtime_error("attributes used string dispatch");
    fw::gw::skills::Profile sample_skills;sample_skills.primary=1;sample_skills.profession_mask=6;
    fw::gw::attributes::Profile sample_attributes;
    const auto sample_skill_hex=gwserver::storage_hex(fw::gw::skills::encode_profile(sample_skills));
    const auto sample_attribute_hex=gwserver::storage_hex(sample_attributes.encode());
    const auto decoded_skills=gwserver::decode_skills({FW_OK,"1\t"+sample_skill_hex});
    const auto decoded_attributes=gwserver::decode_attributes({FW_OK,"1\t"+sample_attribute_hex});
    if(decoded_skills.status!=FW_OK||!decoded_skills.record||decoded_skills.record->profile.primary!=1||decoded_attributes.status!=FW_OK||!decoded_attributes.record||decoded_attributes.record->revision!=1)throw std::runtime_error("typed skill/attribute profile decode");
    if(gwserver::decode_skills({FW_OK,"0"}).record||gwserver::decode_attributes({FW_OK,"0"}).record)throw std::runtime_error("missing profile became fabricated state");
    for(const auto& malformed:{std::string("0\t")+sample_skill_hex,std::string("1\tzz"),std::string("18446744073709551615\t")+sample_skill_hex,std::string("1\t")+sample_skill_hex+"\textra"}){
        const auto decoded=gwserver::decode_skills({FW_OK,malformed});if(decoded.status!=FW_INTERNAL||decoded.record)throw std::runtime_error("malformed skills exposed record");
    }
    for(const auto& malformed:{std::string("0\t")+sample_attribute_hex,std::string("1\t00"),std::string("1\t")+sample_attribute_hex+"\textra"}){
        const auto decoded=gwserver::decode_attributes({FW_OK,malformed});if(decoded.status!=FW_INTERNAL||decoded.record)throw std::runtime_error("malformed attributes exposed record");
    }
    const gwserver::SaveSkillbar bar_request{skill_owner,1,{}};
    const gwserver::SaveAttributes attribute_request{skill_owner,1,sample_attributes};
    const gwserver::SaveSecondaryProfession profession_request{skill_owner,1,1,sample_skills,sample_attributes};
    const auto before_bar_operation=client.operation;const auto before_bar_payload=client.payload;
    if(client.saveSkillbar(bar_request).status!=FW_NOT_FOUND||client.operation!=before_bar_operation||client.payload!=before_bar_payload)throw std::runtime_error("skillbar used string dispatch");
    const auto before_attr_save_op=client.operation;const auto before_attr_save_payload=client.payload;
    if(client.saveAttributes(attribute_request).status!=FW_NOT_FOUND||client.operation!=before_attr_save_op||client.payload!=before_attr_save_payload)throw std::runtime_error("attribute save used string dispatch");
    const auto prior_profession_op=client.operation;const auto prior_profession_payload=client.payload;
    if(client.saveSecondaryProfession(profession_request).status!=FW_NOT_FOUND||client.operation!=prior_profession_op||client.payload!=prior_profession_payload)throw std::runtime_error("profession used string dispatch");
    if(gwserver::decode_revision({FW_OK,"2"},1).revision!=2||gwserver::decode_revision({FW_OK,"3"},1).status!=FW_INTERNAL||gwserver::decode_profession_save({FW_OK,"2\t3"},profession_request).status!=FW_INTERNAL)throw std::runtime_error("save response did not match transaction revisions");
    auto& server=manager.server();
    gwserver::NetworkComponent& endpoint=server;
    std::string unique="Network ";for(unsigned i=0;i<10;++i)unique+=char('a'+randombytes_uniform(26));
    const std::string password_bytes="networktest";
    const auto password_span=std::span(reinterpret_cast<const std::uint8_t*>(password_bytes.data()),password_bytes.size());
    const auto registered=endpoint.registerAccount({unique.substr(8),password_span});
    if(registered.status!=FW_OK)throw std::runtime_error("network fixture registration failed");
    if(!registered.login)throw std::runtime_error("registration returned no identity");
    const auto account=gwserver::storage_hex(registered.login->identity.account.bytes);
    const auto policy_identity=gwserver::Identity::from_hex(account);
    if(endpoint.accountCampaigns(policy_identity).status!=FW_OK||endpoint.accountAgreement(policy_identity).status!=FW_OK||
       endpoint.accountCampaigns({}).status!=FW_NOT_FOUND||endpoint.accountAgreement({}).status!=FW_NOT_FOUND)
        throw std::runtime_error("typed policy lost account ownership checks");
    {
        fw::Accounts direct("build/tests/network-lifecycle.db");
        if(direct.accountProfile(policy_identity,[]{return true;}).value.has_value())throw std::runtime_error("absent imported profile was synthesized");
        auto full_profile=profile_fixture;full_profile.resize(239);
        full_profile[32]=200;full_profile[34]=0;
        for(unsigned i=0;i<200;++i)full_profile[34+i]=static_cast<std::uint8_t>(i);
        full_profile[234]=26;full_profile[235]=123;
        const auto encoded_profile=gwserver::storage_hex(full_profile);
        if(direct.execute(gwserver::Operation::maintenanceGwAccountProfileSet,account+"\t"+encoded_profile,[]{return true;}).status!=FW_OK)throw std::runtime_error("policy profile fixture failed");
        const auto imported=endpoint.accountProfile(policy_identity);
        if(imported.status!=FW_OK||!imported.value||imported.value->territory!=7||imported.value->agreement_version!=26||imported.value->attribute_count!=200||imported.value->attributes[199]!=199||imported.value->unknown4!=123)throw std::runtime_error("direct profile lost imported fields");
        if(endpoint.accountProfile({}).status!=FW_NOT_FOUND||endpoint.gameAccountAlias({}).status!=FW_NOT_FOUND)throw std::runtime_error("policy read crossed missing ownership");
        if(endpoint.accountCampaigns(policy_identity).campaigns!=std::set<std::uint32_t>{1,2,3,4}||endpoint.gameAccountAlias(policy_identity).value!=unique.substr(8))throw std::runtime_error("direct campaign or alias read lost stored policy");
        unsigned profile_checks=0,alias_checks=0,campaign_checks=0;
        const auto cancelled_profile=direct.accountProfile(policy_identity,[&]{return ++profile_checks<2;});
        const auto cancelled_alias=direct.gameAccountAlias(policy_identity,[&]{return ++alias_checks<2;});
        const auto cancelled_campaigns=direct.accountCampaigns(policy_identity,[&]{return ++campaign_checks<2;});
        if(cancelled_profile.status!=FW_TIMEOUT||cancelled_profile.value||cancelled_alias.status!=FW_TIMEOUT||cancelled_alias.value||cancelled_campaigns.status!=FW_TIMEOUT||!cancelled_campaigns.campaigns.empty())throw std::runtime_error("policy read published after cancellation");
        const auto policy=direct.accountAgreement(policy_identity,[]{return true;});
        if(policy.status!=FW_OK||!policy.value.has_value())throw std::runtime_error("typed agreement missing stored flag");
        unsigned checks=0;
        const auto cancelled=direct.accountAgreement(policy_identity,[&]{return ++checks<2;});
        if(cancelled.status!=FW_TIMEOUT||cancelled.value.has_value()||checks!=2)throw std::runtime_error("agreement published after cancellation");
        if(direct.execute(gwserver::Operation::maintenanceGwAccountEulaSet,account+"\t0",[]{return true;}).status!=FW_OK||endpoint.accountAgreement(policy_identity).value!=false)
            throw std::runtime_error("agreement false flag changed");
        if(direct.execute(gwserver::Operation::maintenanceGwAccountEulaSet,account+"\t1",[]{return true;}).status!=FW_OK||endpoint.accountAgreement(policy_identity).value!=true)
            throw std::runtime_error("agreement true flag changed");
    }
    const gwserver::LoginAccount credentials{unique.substr(8),std::span(reinterpret_cast<const std::uint8_t*>(password_bytes.data()),password_bytes.size())};
    const auto logged_in=endpoint.login(credentials);
    if(logged_in.status!=FW_OK)throw std::runtime_error("shared login failed");
    if(!logged_in.login)throw std::runtime_error("missing typed account login");
    const auto session=logged_in.login->session;
    {
      fw::Accounts direct("build/tests/native-login-registration.db");
      const gwserver::RegisterAccount request{unique.substr(8)+"-native",credentials.password};
      for(unsigned stop=1;stop<=4;++stop){unsigned checks=0;const auto result=direct.registerAccount(request,[&]{return ++checks<stop;});if(result.status!=FW_TIMEOUT||result.login||checks!=stop)throw std::runtime_error("cancelled registration published account/session");}
      const auto created=direct.registerAccount(request,[]{return true;});
      if(created.status!=FW_OK||!created.login||direct.validateSession({created.login->session},[]{return true;}).status!=FW_OK)throw std::runtime_error("registration rollback or native session token failed");
      const gwserver::LoginAccount login{request.name,request.password};
      for(unsigned stop=1;stop<=4;++stop){unsigned checks=0;const auto result=direct.login(login,[&]{return ++checks<stop;});if(result.status!=FW_TIMEOUT||result.login||checks!=stop||direct.validateSession({created.login->session},[]{return true;}).status!=FW_OK)throw std::runtime_error("cancelled login published/revoked session");}
      const auto again=direct.login(login,[]{return true;});
      if(again.status!=FW_OK||!again.login||again.login->session.bytes==created.login->session.bytes||direct.validateSession({again.login->session},[]{return true;}).status!=FW_OK)throw std::runtime_error("native login did not issue usable unique session");
      const std::string wrong_password="incorrectpw";
      const gwserver::LoginAccount wrong{request.name,std::span(reinterpret_cast<const std::uint8_t*>(wrong_password.data()),wrong_password.size())};
      const auto denied=direct.login(wrong,[]{return true;});if(denied.status!=FW_NOT_FOUND||denied.login)throw std::runtime_error("wrong password issued session");
      const auto duplicate=direct.registerAccount(request,[]{return true;});if(duplicate.status!=FW_CONFLICT||duplicate.login)throw std::runtime_error("duplicate registration accepted");
      const auto limited=direct.login(login,[]{return true;});if(limited.status!=FW_OVERLOADED||limited.login)throw std::runtime_error("native attempt limit bypassed");
      if(direct.logout({created.login->session},[]{return true;}).status!=FW_OK||direct.logout({again.login->session},[]{return true;}).status!=FW_OK)throw std::runtime_error("native fixture cleanup failed");
    }
    {
      fw::Accounts direct("build/tests/native-ticket-issuance.db");
      const gwserver::LoginAccount fixture{unique.substr(8)+"-ticket",credentials.password};
      const auto created=gwserver::decode_login(direct.execute(gwserver::Operation::registerAccount,gwserver::storage_payload(fixture),[]{return true;}),fixture);
      if(created.status!=FW_OK||!created.login)throw std::runtime_error("ticket fixture registration failed");
      const gwserver::IssueTicket request{created.login->session};
      for(unsigned stop=1;stop<=4;++stop){unsigned checks=0;const auto result=direct.issueTicket(request,[&]{return ++checks<stop;});if(result.status!=FW_TIMEOUT||result.ticket||checks!=stop)throw std::runtime_error("cancelled issuance exposed ticket");}
      std::set<std::string> tokens;
      for(unsigned n=0;n<4;++n){const auto result=direct.issueTicket(request,[]{return true;});if(result.status!=FW_OK||!result.ticket||!result.ticket->expires||result.ticket->expires>created.login->identity.expires||!tokens.insert(fw::gw::format_token(result.ticket->token)).second)throw std::runtime_error("native ticket capacity/uniqueness/expiry failed");}
      const auto full=direct.issueTicket(request,[]{return true;});if(full.status!=FW_OVERLOADED||full.ticket)throw std::runtime_error("ticket quota bypassed");
      const auto second=gwserver::decode_login(direct.execute(gwserver::Operation::login,gwserver::storage_payload(fixture),[]{return true;}),fixture);
      if(second.status!=FW_OK||!second.login||direct.issueTicket({second.login->session},[]{return true;}).status!=FW_OVERLOADED)throw std::runtime_error("ticket quota not shared across account sessions");
      if(direct.logout({request.session},[]{return true;}).status!=FW_OK||direct.issueTicket(request,[]{return true;}).status!=FW_NOT_FOUND||direct.issueTicket({gwserver::SessionToken{}},[]{return true;}).status!=FW_NOT_FOUND)throw std::runtime_error("revoked/unknown session issued ticket");
      const auto after_logout=direct.issueTicket({second.login->session},[]{return true;});
      if(after_logout.status!=FW_OK||!after_logout.ticket||direct.logout({second.login->session},[]{return true;}).status!=FW_OK)throw std::runtime_error("logout did not cascade tickets or affected other session");
    }
    {
      fw::Accounts direct("build/tests/network-lifecycle.db");
      const auto valid=direct.validateSession({session},[]{return true;});
      if(valid.status!=FW_OK||!valid.identity||valid.identity->account.bytes!=gwserver::Identity::from_hex(account).bytes||valid.identity->name!=logged_in.login->identity.name||valid.identity->expires!=logged_in.login->identity.expires)throw std::runtime_error("native session identity mismatch");
      for(unsigned stop=1;stop<=3;++stop){unsigned checks=0;const auto cancelled=direct.validateSession({session},[&]{return ++checks<stop;});if(cancelled.status!=FW_TIMEOUT||cancelled.identity||checks!=stop)throw std::runtime_error("cancelled session exposed identity");}
      const auto missing=direct.validateSession({gwserver::SessionToken{}},[]{return true;});
      if(missing.status!=FW_NOT_FOUND||missing.identity)throw std::runtime_error("unknown session exposed identity");
    }
    if(endpoint.validateSession({session}).status!=FW_OK||endpoint.gameAccountAlias(gwserver::Identity::from_hex(account)).status!=FW_OK||endpoint.issueTicket({session}).status!=FW_OK)
        throw std::runtime_error("shared portal session or ticket failed");
    {
      fw::Accounts direct("build/tests/network-lifecycle.db");
      for(unsigned stop=1;stop<=3;++stop){unsigned checks=0;const auto result=direct.logout({session},[&]{return ++checks<stop;});if(result.status!=FW_TIMEOUT||checks!=stop||direct.validateSession({session},[]{return true;}).status!=FW_OK)throw std::runtime_error("cancelled logout revoked session");}
      if(direct.logout({gwserver::SessionToken{}},[]{return true;}).status!=FW_NOT_FOUND)throw std::runtime_error("unknown logout accepted");
    }
    const auto other_session=endpoint.login(credentials);
    if(other_session.status!=FW_OK||!other_session.login)throw std::runtime_error("parallel session login failed");
    if(endpoint.logout({session}).status!=FW_OK||endpoint.validateSession({session}).status!=FW_NOT_FOUND)
        throw std::runtime_error("shared logout failed to revoke session");
    if(endpoint.validateSession({other_session.login->session}).status!=FW_OK||endpoint.logout({session}).status!=FW_NOT_FOUND||endpoint.logout({other_session.login->session}).status!=FW_OK)throw std::runtime_error("logout affected another session or replay succeeded");
    fw::gw::Summary2022 summary;summary.map=148;summary.level=1;summary.low_nibble=1;summary.appearance=1u<<20;
    const auto profile=summary.profile();
    {
        fw::Accounts direct("build/tests/network-lifecycle.db");
        auto gear_summary=summary;gear_summary.equipment={{91,7,19},{92,7,19},{94,7,19},{90,7,19}};
        const auto gear_profile=gear_summary.profile();
        const auto name=fw::gw::character_name_to_utf8(std::u16string(19,u'\uac00'));
        const auto before=endpoint.characterRoster({policy_identity,0});
        unsigned checkpoints=0;
        const auto cancelled=direct.createCharacter({policy_identity,name,gear_profile},[&]{return ++checkpoints<4;});
        if(cancelled.status!=FW_TIMEOUT||cancelled.created||checkpoints!=4||endpoint.characterRoster({policy_identity,0}).total!=before.total)throw std::runtime_error("typed creation failed precommit rollback");
        const auto unicode=endpoint.createCharacter({policy_identity,name,gear_profile});
        if(unicode.status!=FW_OK||!unicode.created||unicode.created->name!=name||unicode.created->map!=148)throw std::runtime_error("typed Unicode creation failed");
        const gwserver::OwnedCharacter owner{policy_identity,unicode.created->character};
        const auto gear=endpoint.characterInventory(owner);const auto skills=endpoint.characterSkills(owner);
        if(!gear.record||gear.record->profile.items.size()!=5||!skills.record||skills.record->profile.primary!=1||skills.record->profile.secondary!=0||!skills.record->profile.learned.empty())throw std::runtime_error("typed creation did not persist gear/profession atomically");
        for(const auto&[id,item]:gear.record->profile.items)if(item.customized!=std::u16string(19,u'\uac00')||(item.slot!=0&&(!item.reviewed_armor||item.reviewed_armor->base!=25)))throw std::runtime_error("typed creation changed starter armor owner/stats");
        if(endpoint.deleteCharacter({owner.account,owner.character}).status!=FW_OK||endpoint.characterInventory(owner).status!=FW_NOT_FOUND||endpoint.characterSkills(owner).status!=FW_NOT_FOUND)throw std::runtime_error("typed creation cleanup failed");
        auto invalid_profile=profile;invalid_profile.unknown1=8;
        if(endpoint.createCharacter({policy_identity,unique,invalid_profile}).status!=FW_INVALID||endpoint.createCharacter({{},unique,profile}).status!=FW_NOT_FOUND||endpoint.characterRoster({policy_identity,0}).total!=before.total)throw std::runtime_error("typed creation invalid profile/owner committed");
    }
    const auto created=endpoint.createCharacter({gwserver::Identity::from_hex(account),unique,profile});
    if(created.status!=FW_OK)throw std::runtime_error("shared create failed");
    if(!created.created)throw std::runtime_error("missing typed creation record");
    const auto character=gwserver::storage_hex(created.created->character.bytes),owned=account+"\t"+character;
    const gwserver::OwnedCharacter progression_owner{gwserver::Identity::from_hex(account),created.created->character};
    {
        fw::Accounts direct("build/tests/network-lifecycle.db");
        const auto replacement=unique.substr(0,13)+" Swap";
        unsigned rename_checks=0;
        const auto cancelled=direct.renameCharacter({progression_owner,unique,replacement},[&]{return ++rename_checks<3;});
        if(cancelled.status!=FW_TIMEOUT||cancelled.value||rename_checks!=3||endpoint.characterName(progression_owner).value!=unique)throw std::runtime_error("typed rename precommit rollback failed");
        if(endpoint.renameCharacter({progression_owner,unique,replacement}).value!=replacement||endpoint.renameCharacter({progression_owner,unique,replacement}).status!=FW_CONFLICT||endpoint.renameCharacter({progression_owner,replacement,unique}).value!=unique)throw std::runtime_error("typed rename CAS or restore failed");
        if(endpoint.unlockDevelopmentCharacter(progression_owner).status!=FW_INVALID)throw std::runtime_error("development unlocked level-one character");
        auto dev_summary=summary;dev_summary.level=20;
        const auto temporary=endpoint.createCharacter({policy_identity,unique.substr(0,13)+" Dev",dev_summary.profile()});
        if(!temporary.created)throw std::runtime_error("development typed fixture failed");
        const gwserver::OwnedCharacter dev_owner{policy_identity,temporary.created->character};
        const auto before=endpoint.characterSkills(dev_owner);unsigned unlock_checks=0;
        const auto cancelled_unlock=direct.unlockDevelopmentCharacter(dev_owner,[&]{return ++unlock_checks<3;});
        const auto after=endpoint.characterSkills(dev_owner);
        if(cancelled_unlock.status!=FW_TIMEOUT||unlock_checks!=3||!before.record||!after.record||after.record->revision!=before.record->revision||!after.record->profile.learned.empty()||endpoint.characterAttributes(dev_owner).record)throw std::runtime_error("development partial grant survived rollback");
        const auto granted=endpoint.unlockDevelopmentCharacter(dev_owner),again=endpoint.unlockDevelopmentCharacter(dev_owner);
        const auto learned=endpoint.characterSkills(dev_owner);const auto attrs=endpoint.characterAttributes(dev_owner);
        if(granted.status!=FW_OK||again.status!=FW_OK||granted.learned!=fw::gw::skills::obtainable_player_skills.size()||again.skill_revision!=granted.skill_revision||again.attribute_revision!=granted.attribute_revision||!learned.record||learned.record->profile.learned.size()!=granted.learned||!attrs.record||attrs.record->profile.total_points(20)!=200)throw std::runtime_error("typed development grant incomplete or non-idempotent");
        if(endpoint.deleteCharacter({dev_owner.account,dev_owner.character}).status!=FW_OK)throw std::runtime_error("development typed fixture cleanup failed");
    }
    const auto new_guild=endpoint.guildCommand({progression_owner,gwserver::GuildAction::create,unique+" | "+unique.substr(8,4)});
    if(new_guild.status!=FW_OK||new_guild.message.empty())throw std::runtime_error("typed guild creation failed");
    const auto created_guild=endpoint.guild(progression_owner.account);
    if(created_guild.status!=FW_OK||!created_guild.value||created_guild.value->name!=unique||created_guild.value->members.size()!=1||created_guild.value->members[0].account.bytes!=progression_owner.account.bytes)throw std::runtime_error("typed guild membership read failed");
    if(endpoint.guildCommand({progression_owner,gwserver::GuildAction::motd,"Native typed announcement"}).status!=FW_OK||endpoint.guild(progression_owner.account).value->announcement!="Native typed announcement"||endpoint.guildCommand({progression_owner,gwserver::GuildAction::leave,""}).status!=FW_OK||endpoint.guild(progression_owner.account).value)throw std::runtime_error("typed guild announcement/leave failed");
    {
        fw::Accounts direct("build/tests/network-lifecycle.db");
        if(endpoint.equipmentVisibility(progression_owner).value!=0)throw std::runtime_error("visibility baseline not hidden");
        unsigned checks=0;const auto cancelled=direct.saveEquipmentVisibility({progression_owner,0,3},[&]{return ++checks<3;});
        if(cancelled.status!=FW_TIMEOUT||cancelled.value||endpoint.equipmentVisibility(progression_owner).value!=0)throw std::runtime_error("cancelled visibility write committed");
        if(endpoint.saveEquipmentVisibility({progression_owner,0,3}).value!=3||endpoint.saveEquipmentVisibility({progression_owner,0,12}).status!=FW_CONFLICT||endpoint.equipmentVisibility(progression_owner).value!=3)throw std::runtime_error("visibility CAS lost state");
        if(endpoint.equipmentVisibility({fixture_id,progression_owner.character}).status!=FW_NOT_FOUND||endpoint.saveEquipmentVisibility({{fixture_id,progression_owner.character},3,0}).status!=FW_NOT_FOUND)throw std::runtime_error("visibility ownership bypassed");
        if(endpoint.saveEquipmentVisibility({progression_owner,3,256}).status!=FW_INVALID)throw std::runtime_error("visibility range accepted");
        if(endpoint.saveEquipmentVisibility({progression_owner,3,0}).value!=0)throw std::runtime_error("visibility restore failed");
        const std::array<std::uint8_t,10> request{0x55,0x80,0xc0,0,0,0,0xc0,0,0,0};
        const auto decoded=fw::gw::equipment::parse_visibility_2022(request);
        if(decoded.value!=192||decoded.mask!=192||fw::gw::equipment::visibility_2022(192,192)!=std::vector<std::uint8_t>({238,0,192,0,0,0,192,0,0,0}))throw std::runtime_error("native visibility framing differs");
        auto bad=request;bad[6]=1;try{fw::gw::equipment::parse_visibility_2022(bad);throw std::runtime_error("partial visibility mask accepted");}catch(const std::invalid_argument&){}
    }
    const gwserver::SaveProgression first_progression{progression_owner,0,{}};
    const gwserver::SaveQuests first_quests{progression_owner,0,*fw::gw::quests::accept({},101)};
    const auto quests_saved=endpoint.saveQuests(first_quests);
    const auto quests_loaded=endpoint.characterQuests(progression_owner);
    if(quests_saved.status!=FW_OK||quests_saved.revision!=1||endpoint.saveQuests(first_quests).status!=FW_CONFLICT||
        quests_loaded.status!=FW_OK||!quests_loaded.record||quests_loaded.record->profile!=first_quests.profile)
        throw std::runtime_error("typed quest persistence lost profile or CAS");
    if(manager.client().characterQuests(progression_owner).status!=FW_UNAVAILABLE||manager.client().saveQuests(first_quests).status!=FW_UNAVAILABLE)
        throw std::runtime_error("quest custom client bypassed transport");
    if(gwserver::decode_quests({FW_OK,"0"}).record||gwserver::decode_quests({FW_OK,"1\t00"}).status!=FW_INTERNAL)
        throw std::runtime_error("quest response validation failed");
    fw::gw::quests::Content quest_content;quest_content.entry.id=101;quest_content.description=u"";quest_content.objectives=u"";
    const fw::gw::quests::Catalog quest_catalog{{101,quest_content}};
    const auto quest_details=gwserver::readCharacterQuest(endpoint,progression_owner,quest_catalog,101,false);
    const auto quest_marker=gwserver::readCharacterQuest(endpoint,progression_owner,quest_catalog,101,true);
    if(quest_details.status!=FW_OK||!quest_details.packet||(*quest_details.packet)[0]!=75||quest_marker.status!=FW_OK||!quest_marker.packet||(*quest_marker.packet)[0]!=76||
        gwserver::readCharacterQuest(endpoint,progression_owner,quest_catalog,102,false).packet)
        throw std::runtime_error("owned quest query publication failed");
    const auto abandoned_quest=gwserver::abandonCharacterQuest(endpoint,progression_owner,101);
    if(abandoned_quest.status!=FW_OK||abandoned_quest.committed_cycle!=1||!abandoned_quest.packet||(*abandoned_quest.packet)[0]!=81||
        endpoint.characterQuests(progression_owner).record->profile.records.at(101).stage!=fw::gw::quests::Stage::abandoned||
        gwserver::abandonCharacterQuest(endpoint,progression_owner,101).packet)
        throw std::runtime_error("quest abandon failed durable commit/removal or duplicate denial");
    if(gwserver::readCharacterQuest(endpoint,progression_owner,quest_catalog,101,false).packet||gwserver::readCharacterQuest(endpoint,progression_owner,quest_catalog,101,true).packet)
        throw std::runtime_error("abandoned quest still published");
    const auto before_accept=endpoint.characterQuests(progression_owner).record->revision;
    struct ConflictingQuestServer:gwserver::NetworkServer {
        using NetworkServer::NetworkServer;
        gwserver::RevisionResult saveQuests(const gwserver::SaveQuests&) override{return {FW_CONFLICT,"injected competing revision"};}
    } conflicting_quests(manager);
    const auto conflicting_accept=gwserver::acceptCharacterQuest(conflicting_quests,progression_owner,quest_catalog,101,false);
    if(conflicting_accept.status!=FW_CONFLICT||!conflicting_accept.packets.empty()||endpoint.characterQuests(progression_owner).record->revision!=before_accept)
        throw std::runtime_error("failed quest commit published client packets");
    const auto missing_accept=gwserver::acceptCharacterQuest(endpoint,progression_owner,{},101,false);
    if(missing_accept.status!=FW_INVALID||!missing_accept.packets.empty()||endpoint.characterQuests(progression_owner).record->revision!=before_accept)
        throw std::runtime_error("missing quest content committed progress");
    auto malformed_quest=quest_catalog;malformed_quest.at(101).objectives=std::u16string(128,u'x');
    const auto malformed_accept=gwserver::acceptCharacterQuest(endpoint,progression_owner,malformed_quest,101,false);
    if(malformed_accept.status!=FW_INVALID||!malformed_accept.packets.empty()||endpoint.characterQuests(progression_owner).record->revision!=before_accept)
        throw std::runtime_error("invalid quest wire content committed progress");
    fw::gw::world::Instance objective_world;
    fw::gw::world::Instance::Npc objective_npc;objective_npc.id=100;objective_npc.definition.id=1;
    objective_npc.definition.visual_adjustment[3]=100;objective_npc.definition.level=1;
    fw::gw::combat::npcs::Definition objective_definition;
    objective_definition.binding={146,100,1,0,0,0,0,1};
    objective_definition.relation=fw::gw::combat::npcs::Relation::hostile_to_players;
    objective_definition.spawn_health=fw::gw::combat::Health{35,35};objective_definition.damageable=true;
    objective_definition.physical_armor=60; // Synthetic integration fixture, not retail creature AR.
    objective_npc.combat_definition=objective_definition;
    auto objective_box=std::make_shared<fw::gw::world::Outbox>();objective_world.join(42,"Objective Tester",0,0,objective_box);
    objective_world.set_vitals(42,{100,100,20,20},100);
    const auto objective_owner=*objective_world.effect_scope(42);
    if(objective_world.spawn_kill_encounter(objective_owner,101,2,{objective_npc},[&]{
        return gwserver::acceptCharacterQuest(conflicting_quests,progression_owner,quest_catalog,101,false).status==FW_OK;
    })||objective_world.effect_scope(60000)||endpoint.characterQuests(progression_owner).record->revision!=before_accept)
        throw std::runtime_error("failed acceptance left live encounter or durable mutation");
    gwserver::QuestAcceptanceResult reaccepted_quest{FW_UNAVAILABLE,{}, {}};
    if(!objective_world.spawn_kill_encounter(objective_owner,101,2,{objective_npc},[&]{
        reaccepted_quest=gwserver::acceptCharacterQuest(endpoint,progression_owner,quest_catalog,101,false);
        return reaccepted_quest.status==FW_OK;
    }))throw std::runtime_error("accepted encounter failed activation");
    const auto accepted_profile=endpoint.characterQuests(progression_owner);
    if(reaccepted_quest.status!=FW_OK||reaccepted_quest.packets.size()!=2||reaccepted_quest.packets[0][0]!=72||reaccepted_quest.packets[1][0]!=75||
        accepted_profile.record->revision!=before_accept+1||accepted_profile.record->profile.records.at(101).cycle!=2||
        accepted_profile.record->profile.records.at(101).stage!=fw::gw::quests::Stage::active)
        throw std::runtime_error("quest acceptance failed durable state and Add-before-Details");
    const auto duplicate_accept=gwserver::acceptCharacterQuest(endpoint,progression_owner,quest_catalog,101,false);
    if(duplicate_accept.status!=FW_CONFLICT||!duplicate_accept.packets.empty()||endpoint.characterQuests(progression_owner).record->revision!=before_accept+1)
        throw std::runtime_error("duplicate quest acceptance mutated progress");
    if(!gwserver::acceptCharacterQuest(manager.client(),progression_owner,quest_catalog,101,false).packets.empty())
        throw std::runtime_error("unavailable quest client published acceptance");
    const auto revision_before_completion=endpoint.characterQuests(progression_owner).record->revision;
    if(gwserver::completeCharacterQuest(endpoint,progression_owner,quest_catalog,101,1).status!=FW_CONFLICT||
        !gwserver::completeCharacterQuest(conflicting_quests,progression_owner,quest_catalog,101,2).packets.empty()||
        !gwserver::completeCharacterQuest(endpoint,progression_owner,{},101,2).packets.empty()||
        endpoint.characterQuests(progression_owner).record->revision!=revision_before_completion)
        throw std::runtime_error("stale/missing/failed quest completion committed or published");
    const std::array objective_targets{*objective_world.effect_scope(60000)};
    if(!objective_world.begin_kill_objective(objective_owner,101,2,objective_targets)||
        !objective_world.ordinary_weapon(objective_owner,fw::gw::combat::weapon::Owned{3,1,{20,20,2},12,false,false,27},20)||
        !objective_world.ordinary_attack(objective_owner,objective_targets[0].actor,100))
        throw std::runtime_error("ordinary attack did not start encounter combat");
    objective_world.tick(600);objective_world.tick(1930);
    if(objective_world.npc_combat_snapshot(objective_targets[0])->health->current!=0)
        throw std::runtime_error("ordinary weapon kill did not finish encounter");
    const auto pending_completion=objective_world.pending_quest_completions(objective_owner);
    if(pending_completion!=std::vector<fw::gw::world::Instance::QuestCompletion>{{101,2}}||
        gwserver::completeCharacterQuest(conflicting_quests,progression_owner,quest_catalog,101,2).status!=FW_CONFLICT||
        objective_world.pending_quest_completions(objective_owner)!=pending_completion||
        endpoint.characterQuests(progression_owner).record->revision!=revision_before_completion)
        throw std::runtime_error("failed completion lost encounter or changed durable state");
    const auto completed_quest=gwserver::completeCharacterQuest(endpoint,progression_owner,quest_catalog,
        pending_completion[0].quest,pending_completion[0].cycle);
    if(completed_quest.status!=FW_OK||completed_quest.packets.size()!=2||completed_quest.packets[0][0]!=83||completed_quest.packets[1][0]!=76||
        endpoint.characterQuests(progression_owner).record->profile.records.at(101).stage!=fw::gw::quests::Stage::completed||
        endpoint.characterQuests(progression_owner).record->revision!=revision_before_completion+1||
        !gwserver::completeCharacterQuest(endpoint,progression_owner,quest_catalog,101,2).packets.empty())
        throw std::runtime_error("quest completion update/CAS/duplicate fence failed");
    if(!objective_world.clear_kill_objective(objective_owner,101,2)||
        !objective_world.pending_quest_completions(objective_owner).empty())
        throw std::runtime_error("committed completion acknowledgment failed");
    const gwserver::RewardQuestExperience reward_request{progression_owner,revision_before_completion+1,101,2,100};
    if(gwserver::rewardCharacterQuestExperience(manager.client(),reward_request).packet||
       gwserver::rewardCharacterQuestExperience(endpoint,{progression_owner,0,101,2,100}).packet||
       gwserver::rewardCharacterQuestExperience(endpoint,{progression_owner,0,0,2,100}).status!=FW_INVALID)
        throw std::runtime_error("failed quest reward published removal");
    const auto progression_saved=endpoint.saveProgression(first_progression);
    struct RewardPacketServer:gwserver::NetworkServer {
        using NetworkServer::NetworkServer;
        bool fail=false,scale=false;unsigned commits=0;
        gwserver::InventoryResult characterInventory(const gwserver::OwnedCharacter&) override{
            auto profile=fw::gw::inventory::baseline();profile.gold=20;
            if(scale){fw::gw::inventory::Item item;item.bag=1;item.slot=0;item.definition.id=4096;item.definition.model=2566;item.definition.file=23704;item.definition.type=21;item.definition.quantity=1;item.definition.interaction=537399809;item.definition.encoded_name={9558,52505,35705,32767};profile.items.emplace(4096,item);profile.next_item=4097;}
            return {FW_OK,{},gwserver::VersionedProfile<fw::gw::inventory::Profile>{9,profile}};
        }
        gwserver::SkillsResult characterSkills(const gwserver::OwnedCharacter&) override{
            fw::gw::skills::Profile profile;profile.primary=6;profile.profession_mask=1u<<6;profile.learned.emplace(194,1);
            return {FW_OK,{},gwserver::VersionedProfile<fw::gw::skills::Profile>{7,profile}};
        }
        gwserver::RevisionResult rewardQuestExperience(const gwserver::RewardQuestExperience&) override{
            ++commits;return {static_cast<unsigned>(fail?FW_CONFLICT:FW_OK),{},8};
        }
    } reward_packets(manager);
    const gwserver::RewardQuestExperience skill_reward{progression_owner,1,101,1,100,7,{194,197}};
    const auto published_reward=gwserver::rewardCharacterQuest(reward_packets,skill_reward);
    if(published_reward.status!=FW_OK||published_reward.packets.size()!=4||published_reward.packets[1][0]!=220||
       published_reward.packets[2][0]!=219||published_reward.packets[3][0]!=81||reward_packets.commits!=1)
        throw std::runtime_error("quest skill learning/removal publication failed");
    reward_packets.fail=true;
    if(!gwserver::rewardCharacterQuest(reward_packets,skill_reward).packets.empty())
        throw std::runtime_error("failed skill reward published client state");
    reward_packets.fail=false;auto gold_reward=skill_reward;gold_reward.gold=50;gold_reward.expected_inventory_revision=9;
    const auto gold_published=gwserver::rewardCharacterQuest(reward_packets,gold_reward);
    if(gold_published.status!=FW_OK||gold_published.packets.size()!=5||gold_published.packets[0][0]!=63||
       gold_published.packets[0][1]!=1||gold_published.packets[0][4]!=50||gold_published.packets.back()[0]!=81)
        throw std::runtime_error("quest gold delta/removal publication failed");
    gold_reward.expected_inventory_revision=8;
    if(!gwserver::rewardCharacterQuest(reward_packets,gold_reward).packets.empty())
        throw std::runtime_error("stale quest inventory published gold");
    gold_reward.expected_inventory_revision=9;reward_packets.fail=true;
    if(!gwserver::rewardCharacterQuest(reward_packets,gold_reward).packets.empty())
        throw std::runtime_error("failed quest commit published gold");
    reward_packets.scale=true;reward_packets.fail=false;
    auto scale_reward=skill_reward;scale_reward.id=83;scale_reward.consume_shimmering_scale=true;scale_reward.expected_inventory_revision=9;
    const auto before_scale=reward_packets.commits;
    if(gwserver::rewardCharacterQuest(reward_packets,scale_reward).status!=FW_INVALID||reward_packets.commits!=before_scale)
        throw std::runtime_error("scale reward committed without session identities");
    fw::gw::inventory::identity::Session scale_ids;
    scale_ids.hydrate(reward_packets.characterInventory(progression_owner).record->profile,fw::gw::storage::baseline());
    const auto scale_wire=*scale_ids.wire_item(fw::gw::inventory::identity::Domain::character,4096);
    const auto scale_result=gwserver::rewardCharacterQuest(reward_packets,scale_reward,&scale_ids);
    const auto native_delete=fw::gw::items::delete_2022(scale_wire);
    if(scale_wire==4096||scale_result.status!=FW_OK||std::find(scale_result.packets.begin(),scale_result.packets.end(),native_delete)==scale_result.packets.end())
        throw std::runtime_error("scale reward leaked durable item identity to client");
    reward_packets.fail=true;
    if(!gwserver::rewardCharacterQuest(reward_packets,scale_reward,&scale_ids).packets.empty())
        throw std::runtime_error("failed scale reward published item deletion");
    reward_packets.fail=false;reward_packets.scale=false;
    {
      struct TimedServer:gwserver::NetworkServer {
        using NetworkServer::NetworkServer;fw::gw::quests::Profile profile;std::uint64_t revision=0;bool fail=false;
        gwserver::QuestsResult characterQuests(const gwserver::OwnedCharacter&) override{
          if(!revision)return {FW_OK,{},{}};
          return {FW_OK,{},gwserver::VersionedProfile<fw::gw::quests::Profile>{revision,profile}};
        }
        gwserver::RevisionResult saveQuests(const gwserver::SaveQuests& request) override{
          if(fail||request.expected_revision!=revision)return {FW_CONFLICT,"competing timed revision",revision};
          fw::gw::quests::validate(request.profile);profile=request.profile;return {FW_OK,{},++revision};
        }
      } timed(manager);
      const auto content=fw::gw::quests::load_catalog(std::filesystem::path("data/gw/quests/marksmanship.fwqc"));
      if(gwserver::acceptCharacterQuest(timed,progression_owner,content,42,false).status!=FW_INVALID||timed.revision)
        throw std::runtime_error("timed acceptance omitted authoritative start");
      const auto accepted=gwserver::acceptCharacterQuest(timed,progression_owner,content,42,false,{}, {},0,false,100000);
      if(accepted.status!=FW_OK||accepted.packets.size()!=2||timed.profile.records.at(42).timed->deadline_utc_ms!=190000)
        throw std::runtime_error("timed acceptance did not persist deadline");
      if(gwserver::creditCharacterQuest(timed,progression_owner,content,42,1,1,5).status!=FW_CONFLICT||timed.revision!=1)
        throw std::runtime_error("timed credit omitted event timestamp");
      const auto credited=gwserver::creditCharacterQuest(timed,progression_owner,content,42,1,1,5,110000);
      if(credited.status!=FW_OK||credited.packets.size()!=1||credited.packets[0][0]!=83||timed.revision!=2)
        throw std::runtime_error("timed credit did not publish captured update");
      timed.fail=true;const auto competing=gwserver::creditCharacterQuest(timed,progression_owner,content,42,1,2,5,120000);
      if(competing.status!=FW_CONFLICT||!competing.packets.empty()||timed.profile.records.at(42).objective_credits.size()!=1)
        throw std::runtime_error("failed timed credit published or mutated progress");
      timed.fail=false;
      if(gwserver::expireCharacterMarksmanship(timed,progression_owner,1,189999).status!=FW_CONFLICT)
        throw std::runtime_error("timed service expired before deadline");
      timed.fail=true;
      const auto failed_expiry=gwserver::expireCharacterMarksmanship(timed,progression_owner,1,190000);
      if(failed_expiry.status!=FW_CONFLICT||!failed_expiry.packets.empty()||timed.profile.records.at(42).timed->expired)
        throw std::runtime_error("failed expiry changed state or cleared client clock");
      timed.fail=false;
      const auto expired=gwserver::expireCharacterMarksmanship(timed,progression_owner,1,190000);
      if(expired.status!=FW_OK||expired.packets!=fw::gw::quests::Batch{fw::gw::arena::clear_clock_2022()}||
         !timed.profile.records.at(42).timed->expired)
        throw std::runtime_error("timed service expiry not persisted before clock clear");
      if(!gwserver::expireCharacterMarksmanship(timed,progression_owner,1,200000).packets.empty())
        throw std::runtime_error("duplicate expiry published twice");
    }
    struct CollectionServer:RewardPacketServer {
        using RewardPacketServer::RewardPacketServer;
        fw::gw::quests::Profile quests=*fw::gw::quests::accept({},83);
        unsigned saves=0;
        gwserver::QuestsResult characterQuests(const gwserver::OwnedCharacter&) override{
            return {FW_OK,{},gwserver::VersionedProfile<fw::gw::quests::Profile>{2,quests}};
        }
        gwserver::RevisionResult saveQuests(const gwserver::SaveQuests& request) override{
            ++saves;if(fail)return {FW_CONFLICT,{},2};
            quests=request.profile;return {FW_OK,{},3};
        }
    } collection(manager);
    fw::gw::quests::Content scale_content;scale_content.entry.id=83;
    scale_content.description=u"Test collection";scale_content.objectives=u"Return with the scale";
    fw::gw::quests::Catalog scale_catalog{{83,scale_content}};
    if(gwserver::completeElementalistTest(collection,progression_owner,scale_catalog,1).status!=FW_CONFLICT||collection.saves)
        throw std::runtime_error("collection completed without carried scale");
    collection.scale=true;
    auto missing_objectives=scale_catalog;missing_objectives.at(83).objectives.reset();
    if(gwserver::completeElementalistTest(collection,progression_owner,missing_objectives,1).status!=FW_INVALID||collection.saves)
        throw std::runtime_error("collection completion accepted missing objective publication");
    if(gwserver::completeElementalistTest(collection,progression_owner,{},1).status!=FW_INVALID||
       gwserver::completeElementalistTest(collection,progression_owner,scale_catalog,2).status!=FW_CONFLICT||collection.saves)
        throw std::runtime_error("collection ignored content or attempt fence");
    collection.fail=true;
    if(!gwserver::completeElementalistTest(collection,progression_owner,scale_catalog,1).packets.empty()||
       collection.quests.records.at(83).stage!=fw::gw::quests::Stage::active)
        throw std::runtime_error("failed collection save published completion");
    collection.fail=false;
    const auto collected=gwserver::completeElementalistTest(collection,progression_owner,scale_catalog,1);
    if(collected.status!=FW_OK||collected.packets.empty()||collection.quests.records.at(83).stage!=fw::gw::quests::Stage::completed||
       !collection.characterInventory(progression_owner).record->profile.items.contains(4096)||
       !gwserver::completeElementalistTest(collection,progression_owner,scale_catalog,1).packets.empty())
        throw std::runtime_error("collection completion consumed item or duplicated publication");
    if(progression_saved.status!=FW_OK||progression_saved.revision!=1||endpoint.saveProgression(first_progression).status!=FW_CONFLICT||endpoint.characterProgression(progression_owner).record->revision!=1)throw std::runtime_error("typed progression save lost CAS");
    const auto faction_saved=endpoint.saveFactions({progression_owner.account,0,{}});
    if(faction_saved.status!=FW_OK||faction_saved.revision!=1)throw std::runtime_error("typed faction initialization failed");
    const gwserver::AwardFaction earned{progression_owner.account,1,fw::gw::progression::Faction::imperial,100};
    if(endpoint.awardFaction(earned).revision!=2||endpoint.awardFaction(earned).status!=FW_CONFLICT||endpoint.accountFactions(progression_owner.account).record->profile.counters[3].current!=100)throw std::runtime_error("typed award lost CAS/balance");
    const auto converted=endpoint.exchangeImperialFaction({progression_owner.account,2,fw::gw::progression::Faction::balthazar,100});
    if(converted.status!=FW_OK||converted.revision!=3||endpoint.accountFactions(progression_owner.account).record->profile.counters[3].current!=0||endpoint.accountFactions(progression_owner.account).record->profile.counters[2].current!=33)throw std::runtime_error("typed Imperial exchange failed");
    if(endpoint.createCharacter({gwserver::Identity::from_hex(account),unique,profile}).status!=FW_CONFLICT)
        throw std::runtime_error("typed creation lost name conflict");
    const auto game_login=endpoint.login(credentials);
    for(unsigned primary:{1u,6u}){
        auto starter_summary=summary;starter_summary.equipment={{91,7,19},{92,7,19},{94,7,19},{90,7,19}};
        auto starter_profile=starter_summary.profile();
        auto appearance=std::uint32_t(starter_profile.extended[2])|(std::uint32_t(starter_profile.extended[3])<<16);
        appearance=(appearance&~(15u<<20))|(primary<<20);starter_profile.extended[2]=static_cast<std::uint16_t>(appearance);starter_profile.extended[3]=static_cast<std::uint16_t>(appearance>>16);
        starter_profile.extended[12]=static_cast<std::uint16_t>((starter_profile.extended[12]&~((31u<<4)|(1u<<9)))|(1u<<4));starter_profile.last_map_id=148;
        const auto name=unique.substr(0,10)+(primary==1?" Start W":" Start E");
        const auto created=endpoint.createCharacter({progression_owner.account,name,starter_profile});
        if(created.status!=FW_OK||!created.created)throw std::runtime_error("armed starter creation failed");
        const gwserver::OwnedCharacter owner{progression_owner.account,created.created->character};
        {fw::Accounts reopened("build/tests/network-lifecycle.db");const auto gear=reopened.characterInventory(owner,[]{return true;});const auto skills=reopened.characterSkills(owner,[]{return true;});
            if(!gear.record||!skills.record||skills.record->profile.primary!=primary||!gear.record->profile.weapons.sets[0].weapon)throw std::runtime_error("creation weapon/primary not durable");
            const auto& weapon=gear.record->profile.items.at(gear.record->profile.weapons.sets[0].weapon);
            if(weapon.bag!=2||weapon.slot!=0||weapon.definition.file!=(primary==1?39776u:94937u))throw std::runtime_error("wrong captured creation weapon");}
        if(endpoint.deleteCharacter({owner.account,owner.character}).status!=FW_OK)throw std::runtime_error("starter creation cleanup failed");
    }
    {
        // Isolate quest/item integration from the legacy empty-inventory roster
        // fixture. Production initializes inventory before NPC interactions.
        const auto name=unique.substr(0,10)+" Sword";
        const auto created_sword=endpoint.createCharacter({progression_owner.account,name,profile});
        if(created_sword.status!=FW_OK||!created_sword.created)throw std::runtime_error("quest item character fixture failed");
        const gwserver::OwnedCharacter owner{progression_owner.account,created_sword.created->character};
        if(endpoint.saveInventory({owner,0,fw::gw::inventory::baseline()}).status!=FW_OK)throw std::runtime_error("quest item baseline failed");
        const auto inventory=endpoint.characterInventory(owner);
        const auto proposed=*fw::gw::inventory::presearing::grant_starter_sword(inventory.record->profile,fw::gw::character_name_to_utf16(name));
        const auto& item=proposed.items.at(inventory.record->profile.next_item);
        auto packets=fw::gw::inventory::item_packets(item);
        packets.push_back(fw::gw::items::location_2022(proposed.inventory,item.definition.id,item.bag,item.slot));
        const gwserver::QuestInventoryGrant sword{{owner,inventory.record->revision,proposed},packets};
        fw::gw::world::Instance encounter;auto output=std::make_shared<fw::gw::world::Outbox>();
        encounter.join(42,name,0,0,output);encounter.set_vitals(42,{100,100,20,20},100);
        const auto actor=*encounter.effect_scope(42);
        auto stale=sword;++stale.durable.expected_revision;
        if(encounter.spawn_kill_encounter(actor,101,1,{objective_npc},[&]{
            const auto failed=gwserver::acceptCharacterQuest(endpoint,owner,quest_catalog,101,false,{},stale);
            if(failed.status!=FW_CONFLICT||!failed.packets.empty())throw std::runtime_error("stale item published quest acceptance");
            return failed.status==FW_OK;
        })||encounter.effect_scope(60000)||endpoint.characterQuests(owner).record||
           endpoint.characterInventory(owner).record->revision!=inventory.record->revision)
            throw std::runtime_error("stale item grant left encounter or partial persistence");
        gwserver::QuestAcceptanceResult accepted{FW_UNAVAILABLE,{}, {}};
        if(!encounter.spawn_kill_encounter(actor,101,1,{objective_npc},[&]{
            accepted=gwserver::acceptCharacterQuest(endpoint,owner,quest_catalog,101,false,{},sword);
            return accepted.status==FW_OK;
        })||accepted.packets.size()!=6||accepted.packets[0][0]!=72||accepted.packets[1][0]!=75||
           !std::equal(packets.begin(),packets.end(),accepted.packets.begin()+2)||
           fw::gw::inventory::encode(endpoint.characterInventory(owner).record->profile)!=fw::gw::inventory::encode(proposed)||
           endpoint.characterQuests(owner).record->revision!=1||!encounter.effect_scope(60000))
            throw std::runtime_error("atomic quest/item/encounter grant disagreed");
        encounter.leave(42);
        if(endpoint.deleteCharacter({owner.account,owner.character}).status!=FW_OK)throw std::runtime_error("quest item fixture cleanup failed");
    }
    {
        const auto name=unique.substr(0,10)+" Grawl";
        const auto created=endpoint.createCharacter({progression_owner.account,name,profile});
        if(created.status!=FW_OK||!created.created)throw std::runtime_error("Grawl character creation failed");
        const gwserver::OwnedCharacter owner{progression_owner.account,created.created->character};
        if(endpoint.saveInventory({owner,0,fw::gw::inventory::baseline()}).status!=FW_OK)throw std::runtime_error("Grawl inventory baseline failed");
        const auto content=fw::gw::quests::load_catalog("data/gw/quests/presearing.fwqc");
        const auto* rule=fw::gw::quests::presearing::find(75);
        if(!rule||!content.contains(75))throw std::runtime_error("installed Grawl quest missing");
        const auto inventory=endpoint.characterInventory(owner);
        const auto sword_profile=*fw::gw::inventory::presearing::grant_starter_sword(inventory.record->profile,fw::gw::character_name_to_utf16(name));
        const auto& item=sword_profile.items.at(inventory.record->profile.next_item);
        auto item_packets=fw::gw::inventory::item_packets(item);
        item_packets.push_back(fw::gw::items::location_2022(sword_profile.inventory,item.definition.id,item.bag,item.slot));
        const gwserver::QuestInventoryGrant sword_grant{{owner,inventory.record->revision,sword_profile},item_packets};
        auto stale_sword=sword_grant;++stale_sword.durable.expected_revision;
        const auto rejected_lesson=gwserver::acceptCharacterQuest(endpoint,owner,content,75,false,rule->acceptance_skills,stale_sword,42);
        const auto unmodified_lessons=endpoint.characterSkills(owner);
        if(rejected_lesson.status!=FW_CONFLICT||!rejected_lesson.packets.empty()||
            !unmodified_lessons.record||unmodified_lessons.record->profile.learned.contains(382)||
            unmodified_lessons.record->profile.bar[0].skill)
            throw std::runtime_error("failed quest item grant published or persisted skill assignments");
        // Combat positions/armor are synthetic fixtures; quest identity, grants
        // and reward policy are the installed captured/wiki content.
        std::vector<fw::gw::world::Instance::Npc> enemies;
        for(unsigned i=0;i<3;++i){auto npc=objective_npc;npc.id=100+i;npc.x=30.f*(i+1);
            npc.combat_definition->binding.actor=npc.id;npc.combat_definition->conditions_allowed=true;
            npc.combat_definition->fleshy=true;npc.combat_definition->fleshy_evidence="synthetic Grawl sword-condition fixture";enemies.push_back(npc);}
        fw::gw::world::Instance world;world.join(42,name,0,0,std::make_shared<fw::gw::world::Outbox>());
        world.set_vitals(42,{100,100,20,20},100);const auto actor=*world.effect_scope(42);
        gwserver::QuestAcceptanceResult grawl_acceptance;
        if(!world.spawn_kill_encounter(actor,75,1,enemies,[&]{
            grawl_acceptance=gwserver::acceptCharacterQuest(endpoint,owner,content,75,false,rule->acceptance_skills,sword_grant,42);
            return grawl_acceptance.status==FW_OK;}))
            throw std::runtime_error("Grawl atomic acceptance failed");
        const auto taught=endpoint.characterSkills(owner);
        if(!taught.record||taught.record->profile.bar[0]!=fw::gw::skills::Slot{382,0}||
            taught.record->profile.bar[1]!=fw::gw::skills::Slot{384,0}||taught.record->profile.bar[2]!=fw::gw::skills::Slot{1,0})
            throw std::runtime_error("Grawl acceptance did not persist usable taught skills");
        for(unsigned slot=0;slot<3;++slot){const auto expected=fw::gw::skills::slot_packet_2022(42,slot,taught.record->profile.bar[slot]);
            if(std::find(grawl_acceptance.packets.begin(),grawl_acceptance.packets.end(),expected)==grawl_acceptance.packets.end())
                throw std::runtime_error("Grawl acceptance did not publish persisted slot");}
        {fw::Accounts reopened("build/tests/network-lifecycle.db");const auto quests=reopened.characterQuests(owner,[]{return true;});const auto skills=reopened.characterSkills(owner,[]{return true;});const auto items=reopened.characterInventory(owner,[]{return true;});
            if(quests.record->profile.records.at(75).stage!=fw::gw::quests::Stage::active||!skills.record->profile.learned.contains(382)||!skills.record->profile.learned.contains(384)||!items.record->profile.items.contains(item.definition.id))throw std::runtime_error("Grawl acceptance lost quest/skills/sword on reopen");}
        const fw::gw::combat::weapon::Owned weapon{item.definition.id,1,{10,10,2},12,false,false,27};
        if(!world.ordinary_weapon(actor,weapon,20))throw std::runtime_error("Grawl combat equipment failed");
        for(unsigned i=0;i<3;++i){const auto target=*world.effect_scope(60000+i);
            if(!world.melee_attack(actor,target,weapon,*fw::gw::combat::weapon::attack_skill(382,12),100+i*10)||!world.melee_attack(actor,target,weapon,*fw::gw::combat::weapon::attack_skill(384,12),101+i*10))throw std::runtime_error("Grawl quest skills failed");
            if(i<2&&!world.pending_quest_completions(actor).empty())throw std::runtime_error("Grawl completed before all assigned kills");}
        if(world.pending_quest_completions(actor)!=std::vector<fw::gw::world::Instance::QuestCompletion>{{75,1}}||gwserver::completeCharacterQuest(endpoint,owner,content,75,1).status!=FW_OK)throw std::runtime_error("Grawl objective completion failed");
        const auto quests=endpoint.characterQuests(owner);
        const auto skills=endpoint.characterSkills(owner);const auto xp=endpoint.characterExperience(owner);
        const gwserver::RewardQuestExperience reward{owner,quests.record->revision,75,1,rule->experience,skills.record->revision,{}};
        if(gwserver::rewardCharacterQuest(endpoint,reward).status!=FW_OK||gwserver::rewardCharacterQuest(endpoint,reward).status==FW_OK)throw std::runtime_error("Grawl reward/duplicate fence failed");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");const auto restored=reopened.characterQuests(owner,[]{return true;});const auto experience=reopened.characterNumber(owner,false,[]{return true;});
            const auto learned=reopened.characterSkills(owner,[]{return true;});const auto items=reopened.characterInventory(owner,[]{return true;});
            if(restored.record->profile.records.at(75).stage!=fw::gw::quests::Stage::rewarded||experience.value!=*xp.value+rule->experience||
               !learned.record->profile.learned.contains(382)||!learned.record->profile.learned.contains(384)||!learned.record->profile.learned.contains(1)||
               !items.record->profile.items.contains(item.definition.id))throw std::runtime_error("Grawl reward lost quest/XP/skills/sword on reopen");}
        world.leave(42);if(endpoint.deleteCharacter({owner.account,owner.character}).status!=FW_OK)throw std::runtime_error("Grawl fixture cleanup failed");
    }
    {
        auto mesmer=profile;
        auto appearance=std::uint32_t(profile.extended[2])|(std::uint32_t(profile.extended[3])<<16);
        appearance=(appearance&~(15u<<20))|(5u<<20);
        mesmer.extended[2]=static_cast<std::uint16_t>(appearance);mesmer.extended[3]=static_cast<std::uint16_t>(appearance>>16);
        const auto mesmer_name=unique.substr(0,10)+" Mesmer";
        const auto created=endpoint.createCharacter({progression_owner.account,mesmer_name,mesmer});
        if(created.status!=FW_OK||!created.created)throw std::runtime_error("Mesmer quest fixture creation failed");
        const gwserver::OwnedCharacter owner{progression_owner.account,created.created->character};
        const auto content=fw::gw::quests::load_catalog("data/gw/quests/presearing.fwqc");
        std::ifstream npc_file("data/gw/quests/actors/mesmer-test-skale.txt"),combat_file("data/gw/quests/mesmer-test-development-combat.txt");
        auto population=fw::gw::world::load_npc_catalog(npc_file);
        const std::string sha="beac6399f6d52c52b5ef9acd88c39c7acdbae750214dc88a9309cede5f5fd396";
        fw::gw::world::bind_combat_catalog(population,fw::gw::combat::npcs::load_catalog(combat_file,sha),sha);
        const fw::gw::world::NpcCatalog::KillEncounter encounter{146,population.for_map(146),true};
        fw::gw::world::Instance world;world.join(42,mesmer_name,encounter.actors.front().x,encounter.actors.front().y,std::make_shared<fw::gw::world::Outbox>());
        world.set_vitals(42,{100,100,20,20},100);const auto actor=*world.effect_scope(42);
        if(!fw::gw::world::accept_kill_encounter(world,actor,146,81,1,encounter,[&]{return gwserver::acceptCharacterQuest(endpoint,owner,content,81,false).status==FW_OK;}))throw std::runtime_error("Mesmer atomic acceptance failed");
        if(!world.damage_npc(actor,*world.effect_scope(60000),{fw::gw::combat::Kind::armor_ignoring,200,20,194},1248,101).killed)throw std::runtime_error("Mesmer scoped kill failed");
        if(world.npc_combat_snapshot(*world.effect_scope(60001))->health->current!=10||world.pending_quest_completions(actor)!=std::vector<fw::gw::world::Instance::QuestCompletion>{{81,1}}||gwserver::completeCharacterQuest(endpoint,owner,content,81,1).status!=FW_OK)throw std::runtime_error("Mesmer one-kill completion failed");
        const auto before=endpoint.characterExperience(owner);const auto skills=endpoint.characterSkills(owner);
        gwserver::RewardQuestExperience reward{owner,endpoint.characterQuests(owner).record->revision,81,1,500,skills.record->revision,{26,40},0,0,false,true};
        auto stale=reward;++stale.expected_skill_revision;
        if(gwserver::rewardCharacterQuest(endpoint,stale,nullptr,42).status!=FW_CONFLICT||endpoint.characterExperience(owner).value!=before.value)throw std::runtime_error("Mesmer stale reward changed XP");
        const auto result=gwserver::rewardCharacterQuest(endpoint,reward,nullptr,42);
        if(result.status!=FW_OK||std::find(result.packets.begin(),result.packets.end(),fw::gw::skills::slot_packet_2022(42,0,{26,0}))==result.packets.end()||std::find(result.packets.begin(),result.packets.end(),fw::gw::skills::slot_packet_2022(42,1,{40,0}))==result.packets.end()||gwserver::rewardCharacterQuest(endpoint,reward,nullptr,42).status==FW_OK)throw std::runtime_error("Mesmer reward slot/duplicate fence failed");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");const auto quests=reopened.characterQuests(owner,[]{return true;});const auto learned=reopened.characterSkills(owner,[]{return true;});const auto xp=reopened.characterNumber(owner,false,[]{return true;});
            if(quests.record->profile.records.at(81).stage!=fw::gw::quests::Stage::rewarded||learned.record->profile.primary!=5||learned.record->profile.bar[0]!=fw::gw::skills::Slot{26,0}||learned.record->profile.bar[1]!=fw::gw::skills::Slot{40,0}||xp.value!=*before.value+500)throw std::runtime_error("Mesmer reward persistence failed");}
        {
            std::ifstream actors_file("data/gw/quests/actors/mesmer-burden-bull.txt"),armor_file("data/gw/quests/mesmer-burden-development-combat.txt");
            auto actors=fw::gw::world::load_npc_catalog(actors_file);
            const std::string bull_hash="403b4e1576f4f6eb456d311befc074d1f5bf4ec8c096257ccab0291cad956f60";
            fw::gw::world::bind_combat_catalog(actors,fw::gw::combat::npcs::load_catalog(armor_file,bull_hash),bull_hash);
            const fw::gw::world::NpcCatalog::KillEncounter bull{146,actors.for_map(146),false};
            fw::gw::world::Instance hunt;
            hunt.configure_navigation(std::make_shared<fw::gw::navigation::Mesh>(fw::gw::navigation::Mesh::load("data/gw/navigation/113021.fwnav")));
            hunt.join(42,mesmer_name,bull.actors.front().x,bull.actors.front().y,std::make_shared<fw::gw::world::Outbox>());
            hunt.set_vitals(42,{100,100,20,20},100);const auto hunter=*hunt.effect_scope(42);
            const std::array<std::uint32_t,3> acceptance_skills{31,40,76};
            if(!fw::gw::world::accept_kill_encounter(hunt,hunter,146,71,1,bull,[&]{return gwserver::acceptCharacterQuest(endpoint,owner,content,71,false,acceptance_skills,{},42).status==FW_OK;}))throw std::runtime_error("Althea atomic quest and skills acceptance failed");
            const auto granted=endpoint.characterSkills(owner);
            if(!granted.record->profile.learned.contains(31)||!granted.record->profile.learned.contains(76))throw std::runtime_error("Althea acceptance skills missing");
            if(!hunt.damage_npc(hunter,*hunt.effect_scope(60000),{fw::gw::combat::Kind::armor_ignoring,200,20,194},1248,100).killed||
               hunt.pending_quest_completions(hunter)!=std::vector<fw::gw::world::Instance::QuestCompletion>{{71,1}}||
               gwserver::completeCharacterQuest(endpoint,owner,content,71,1).status!=FW_OK)throw std::runtime_error("Captured Rogue Bull objective failed");
            const gwserver::RewardQuestExperience burden{owner,endpoint.characterQuests(owner).record->revision,71,1,250,granted.record->revision,{},0,0,false,false};
            if(gwserver::rewardCharacterQuest(endpoint,burden,nullptr,42).status!=FW_OK||gwserver::rewardCharacterQuest(endpoint,burden,nullptr,42).status==FW_OK)throw std::runtime_error("Althea reward duplicate fence failed");
            fw::Accounts reopened("build/tests/network-lifecycle.db");
            if(reopened.characterQuests(owner,[]{return true;}).record->profile.records.at(71).stage!=fw::gw::quests::Stage::rewarded||
               reopened.characterNumber(owner,false,[]{return true;}).value!=*before.value+750||
               !reopened.characterSkills(owner,[]{return true;}).record->profile.learned.contains(31))throw std::runtime_error("Althea reward and skill persistence failed");
        }
        {
            std::ifstream appearance("data/gw/quests/actors/domination-magic-brood.txt"),metadata("data/gw/quests/domination-magic-development-combat.txt");
            auto actors=fw::gw::world::load_npc_catalog(appearance);
            const std::string hash="4651fbc7b872b3b17a70cb9948db565ef681e309bfa432d19da9d3cd908553ec";
            fw::gw::world::bind_combat_catalog(actors,fw::gw::combat::npcs::load_catalog(metadata,hash),hash);
            const fw::gw::world::NpcCatalog::KillEncounter encounter{161,actors.for_map(161),false};
            const std::array<std::uint32_t,2> skills{28,27};
            // Acceptance is in Foible's Fair; the objective is restored in Wizard's Folly.
            if(!fw::gw::world::accept_kill_encounter(world,actor,165,49,1,encounter,[&]{return gwserver::acceptCharacterQuest(endpoint,owner,content,49,false,skills,{},42).status==FW_OK;}))throw std::runtime_error("Domination cross-zone acceptance failed");
            fw::gw::world::Instance hunt;
            hunt.join(42,mesmer_name,encounter.actors.front().x,encounter.actors.front().y,std::make_shared<fw::gw::world::Outbox>());
            hunt.set_vitals(42,{100,100,20,20},100);const auto hunter=*hunt.effect_scope(42);
            if(!fw::gw::world::accept_kill_encounter(hunt,hunter,161,49,1,encounter,[]{return true;}))throw std::runtime_error("Domination objective zone restoration failed");
            if(!hunt.damage_npc(hunter,*hunt.effect_scope(60000),{fw::gw::combat::Kind::armor_ignoring,200,20,194},1248,100).killed||
               hunt.pending_quest_completions(hunter)!=std::vector<fw::gw::world::Instance::QuestCompletion>{{49,1}}||
               gwserver::completeCharacterQuest(endpoint,owner,content,49,1).status!=FW_OK)throw std::runtime_error("Domination captured brood objective failed");
            const auto granted=endpoint.characterSkills(owner);
            const gwserver::RewardQuestExperience reward{owner,endpoint.characterQuests(owner).record->revision,49,1,250,granted.record->revision,{},0,0,false,false};
            if(gwserver::rewardCharacterQuest(endpoint,reward,nullptr,42).status!=FW_OK||gwserver::rewardCharacterQuest(endpoint,reward,nullptr,42).status==FW_OK)throw std::runtime_error("Domination reward duplicate fence failed");
            fw::Accounts reopened("build/tests/network-lifecycle.db");
            const auto learned=reopened.characterSkills(owner,[]{return true;});
            if(reopened.characterQuests(owner,[]{return true;}).record->profile.records.at(49).stage!=fw::gw::quests::Stage::rewarded||
               reopened.characterNumber(owner,false,[]{return true;}).value!=*before.value+1000||
               !learned.record->profile.learned.contains(28)||!learned.record->profile.learned.contains(27))throw std::runtime_error("Domination reward and skills persistence failed");
        }
        world.leave(42);if(endpoint.deleteCharacter({owner.account,owner.character}).status!=FW_OK)throw std::runtime_error("Mesmer fixture cleanup failed");
    }
    {
        auto necromancer=profile;auto appearance=std::uint32_t(profile.extended[2])|(std::uint32_t(profile.extended[3])<<16);
        appearance=(appearance&~(15u<<20))|(4u<<20);necromancer.extended[2]=static_cast<std::uint16_t>(appearance);necromancer.extended[3]=static_cast<std::uint16_t>(appearance>>16);
        const auto necromancer_name=unique.substr(0,10)+" Necro";
        const auto created=endpoint.createCharacter({progression_owner.account,necromancer_name,necromancer});
        if(created.status!=FW_OK||!created.created)throw std::runtime_error("Necromancer fixture creation failed");
        const gwserver::OwnedCharacter owner{progression_owner.account,created.created->character};const auto content=fw::gw::quests::load_catalog("data/gw/quests/presearing.fwqc");
        std::ifstream trainer_file("data/gw/quests/actors/necromancer-test-verata.txt"),trainer_metadata("data/gw/quests/necromancer-test-trainer-combat.txt");
        auto trainers=fw::gw::world::load_npc_catalog(trainer_file);const std::string trainer_hash="34e37c8c6184cd3846d03332af680de485bdcbc974cafdc4ee0b8f1e1a223435";
        fw::gw::world::bind_combat_catalog(trainers,fw::gw::combat::npcs::load_catalog(trainer_metadata,trainer_hash),trainer_hash);
        std::ifstream enemies_file("data/gw/quests/actors/necromancer-test-skale.txt"),enemy_metadata("data/gw/quests/necromancer-test-enemy-combat.txt");
        auto enemies=fw::gw::world::load_npc_catalog(enemies_file);const std::string enemy_hash="3123ceecab078f5eb150b2c96657bfe4e293b8c68858c4ec96930a1c9d15882a";
        fw::gw::world::bind_combat_catalog(enemies,fw::gw::combat::npcs::load_catalog(enemy_metadata,enemy_hash),enemy_hash);
        std::ifstream summon_file("data/gw/quests/actors/necromancer-test-bone.txt"),summon_metadata("data/gw/quests/necromancer-test-summon-combat.txt");
        auto summons=fw::gw::world::load_npc_catalog(summon_file);const std::string summon_hash="23d4859323bacf6d5f9d9fde0eeda587f99bb964ba3af5b1e4a29a120440a4d2";
        fw::gw::world::bind_combat_catalog(summons,fw::gw::combat::npcs::load_catalog(summon_metadata,summon_hash),summon_hash);
        const fw::gw::world::NpcCatalog::CorpseEncounter encounter{146,enemies.for_map(146),summons.for_map(146).front(),220,128};
        fw::gw::world::Instance world;
        world.configure_navigation(std::make_shared<fw::gw::navigation::Mesh>(fw::gw::navigation::Mesh::load(
            std::filesystem::path("data/gw/navigation")/(std::to_string(fw::gw::map_asset_file(146))+".fwnav"))));
        world.configure_npcs(trainers.for_map(146));world.join(42,necromancer_name,4605,581,std::make_shared<fw::gw::world::Outbox>());
        world.set_vitals(42,{100,100,20,20},100);const auto actor=*world.effect_scope(42),trainer=*world.effect_scope(trainers.for_map(146).front().id);
        if(!fw::gw::world::accept_corpse_encounter(world,actor,trainer,146,1,encounter,[&]{return gwserver::acceptCharacterQuest(endpoint,owner,content,82,false).status==FW_OK;}))throw std::runtime_error("Necromancer atomic acceptance failed");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");const auto restored=reopened.characterQuests(owner,[]{return true;});
            if(!restored.record||restored.record->profile.records.at(82)!=fw::gw::quests::Progress{1,fw::gw::quests::Stage::active})throw std::runtime_error("Necromancer active state lost on reopen");}
        if(!world.damage_npc(actor,*world.effect_scope(60001),{fw::gw::combat::Kind::armor_ignoring,200,20,194},1248,101).killed||!world.pending_quest_completions(actor).empty())throw std::runtime_error("Necromancer kill-only completed quest");
        if(fw::gw::world::advance_corpse_encounter(world,actor,1,encounter,102,[]{return true;})!=fw::gw::world::Instance::CorpseAdvance::advanced)throw std::runtime_error("Necromancer trainer could not route on Lakeside mesh");
        world.tick(120102);
        if(fw::gw::world::advance_corpse_encounter(world,actor,1,encounter,120103,[]{return true;})!=fw::gw::world::Instance::CorpseAdvance::completed||
           world.pending_quest_completions(actor)!=std::vector<fw::gw::world::Instance::QuestCompletion>{{82,1}}||
           gwserver::completeCharacterQuest(endpoint,owner,content,82,1).status!=FW_OK)throw std::runtime_error("Necromancer summon/completion persistence failed");
        {fw::gw::world::Instance restored_world;restored_world.configure_npcs(trainers.for_map(146));
            restored_world.join(42,necromancer_name,4605,581,std::make_shared<fw::gw::world::Outbox>());
            const auto restored_owner=*restored_world.effect_scope(42),restored_trainer=*restored_world.effect_scope(trainers.for_map(146).front().id);
            const auto durable=endpoint.characterQuests(owner);
            if(durable.record->profile.records.at(82).stage!=fw::gw::quests::Stage::completed||
               !restored_world.restore_completed_corpse_objective(restored_owner,restored_trainer,146,1)||restored_world.effect_scope(60000)||
               restored_world.pending_quest_completions(restored_owner)!=std::vector<fw::gw::world::Instance::QuestCompletion>{{82,1}})throw std::runtime_error("Necromancer completed reconnect lost reward readiness or recreated actors");
        }
        const auto before=endpoint.characterExperience(owner);const auto skills=endpoint.characterSkills(owner);
        gwserver::RewardQuestExperience reward{owner,endpoint.characterQuests(owner).record->revision,82,1,500,skills.record->revision,{153,105},0,0,false,true};
        auto stale=reward;++stale.expected_skill_revision;
        if(gwserver::rewardCharacterQuest(endpoint,stale,nullptr,42).status!=FW_CONFLICT||endpoint.characterExperience(owner).value!=before.value)throw std::runtime_error("Necromancer stale reward changed XP");
        const auto result=gwserver::rewardCharacterQuest(endpoint,reward,nullptr,42);
        if(result.status!=FW_OK||std::find(result.packets.begin(),result.packets.end(),fw::gw::skills::slot_packet_2022(42,0,{153,0}))==result.packets.end()||
           std::find(result.packets.begin(),result.packets.end(),fw::gw::skills::slot_packet_2022(42,1,{105,0}))==result.packets.end()||gwserver::rewardCharacterQuest(endpoint,reward,nullptr,42).status==FW_OK)throw std::runtime_error("Necromancer reward/duplicate fence failed");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");const auto quests=reopened.characterQuests(owner,[]{return true;});const auto learned=reopened.characterSkills(owner,[]{return true;});const auto xp=reopened.characterNumber(owner,false,[]{return true;});
            if(quests.record->profile.records.at(82).stage!=fw::gw::quests::Stage::rewarded||learned.record->profile.primary!=4||
               learned.record->profile.bar[0]!=fw::gw::skills::Slot{153,0}||learned.record->profile.bar[1]!=fw::gw::skills::Slot{105,0}||xp.value!=*before.value+500)throw std::runtime_error("Necromancer reward lost quest/skills/XP on reopen");}
        {
            std::ifstream appearance("data/gw/quests/actors/power-of-blood-fanatics.txt"),metadata("data/gw/quests/power-of-blood-development-combat.txt");
            auto actors=fw::gw::world::load_npc_catalog(appearance);
            const std::string hash="6483f41eb3a0da1ce05d0f3550a59d9cbf1d3472b5bd9af81ba928701dbd8e08";
            fw::gw::world::bind_combat_catalog(actors,fw::gw::combat::npcs::load_catalog(metadata,hash),hash);
            const fw::gw::world::NpcCatalog::KillEncounter encounter{145,actors.for_map(145),false,3};
            const std::array<std::uint32_t,2> acceptance{115,156};
            if(!fw::gw::world::accept_kill_encounter(world,actor,160,43,1,encounter,[&]{return gwserver::acceptCharacterQuest(endpoint,owner,content,43,false,acceptance,{},42).status==FW_OK;}))throw std::runtime_error("Blood counted quest acceptance failed");
            auto make_hunt=[&](unsigned credited){
                auto hunt=std::make_unique<fw::gw::world::Instance>();
                hunt->configure_navigation(std::make_shared<fw::gw::navigation::Mesh>(fw::gw::navigation::Mesh::load(
                    std::filesystem::path("data/gw/navigation")/(std::to_string(fw::gw::map_asset_file(145))+".fwnav"))));
                hunt->join(42,necromancer_name,encounter.actors.front().x,encounter.actors.front().y,std::make_shared<fw::gw::world::Outbox>());
                hunt->set_vitals(42,{100,100,20,20},100);
                if(!hunt->spawn_kill_encounter(*hunt->effect_scope(42),43,1,encounter.actors,[]{return true;},false,{},{},false,0,false,{},{},3,credited))throw std::runtime_error("Blood counted encounter restoration failed");
                return hunt;
            };
            auto hunt=make_hunt(0);std::uint64_t time=100;
            for(unsigned count=1;count<=3;++count){
                const auto target=60000+(count==3?1:0);const auto& npc=encounter.actors.at(count==3?1:0);
                hunt->move(42,npc.x,npc.y,time);time+=60000;hunt->tick(time);
                const auto hunter=*hunt->effect_scope(42);
                if(!hunt->damage_npc(hunter,*hunt->effect_scope(target),{fw::gw::combat::Kind::armor_ignoring,200,20,153},1248,time+1).killed)throw std::runtime_error("Blood scoped actor kill failed");
                const auto pending=hunt->pending_quest_credits(hunter);
                if(pending.size()!=1||!hunt->pending_quest_completions(hunter).empty()||hunt->acknowledge_quest_credit({0,42,0},pending.front()))throw std::runtime_error("Blood pending ownership/threshold failed");
                const auto saved=gwserver::creditCharacterQuest(endpoint,owner,content,43,1,pending.front().credit,3);
                if(saved.status!=FW_OK||saved.packets.size()!=(count==3?2:1))throw std::runtime_error("Blood counted credit save failed");
                if(saved.packets.front()[0]!=83||saved.packets.front()[1]!=0)throw std::runtime_error("Blood objective update emitted QuestAdded instead of objectives change");
                const auto revision=endpoint.characterQuests(owner).record->revision;
                const auto duplicate=gwserver::creditCharacterQuest(endpoint,owner,content,43,1,pending.front().credit,3);
                if(duplicate.status!=FW_OK||!duplicate.packets.empty()||endpoint.characterQuests(owner).record->revision!=revision)throw std::runtime_error("Blood repeated credit changed durable progress");
                if(!hunt->acknowledge_quest_credit(hunter,pending.front())||!hunt->pending_quest_credits(hunter).empty())throw std::runtime_error("Blood pending acknowledgement failed");
                fw::Accounts reopened("build/tests/network-lifecycle.db");
                const auto restored=reopened.characterQuests(owner,[]{return true;});
                if(restored.record->profile.records.at(43).objective_credits.size()!=count)throw std::runtime_error("Blood credit lost after reopening storage");
                const auto text=gwserver::readCharacterQuest(endpoint,owner,content,43,false);
                if(text.status!=FW_OK||!text.packet)throw std::runtime_error("Blood counter details unavailable");
                if(count==1){hunt=make_hunt(1);time=100;}
            }
            if(hunt->npc_combat_snapshot(*hunt->effect_scope(60002))->health->current!=120||endpoint.characterQuests(owner).record->profile.records.at(43).stage!=fw::gw::quests::Stage::completed)throw std::runtime_error("Blood completion incorrectly requires all fanatics");
            const gwserver::RewardQuestExperience reward{owner,endpoint.characterQuests(owner).record->revision,43,1,250};
            if(gwserver::rewardCharacterQuest(endpoint,reward,nullptr,42).status!=FW_OK||gwserver::rewardCharacterQuest(endpoint,reward,nullptr,42).status==FW_OK)throw std::runtime_error("Blood reward fence failed");
            fw::Accounts reopened("build/tests/network-lifecycle.db");
            if(reopened.characterQuests(owner,[]{return true;}).record->profile.records.at(43).stage!=fw::gw::quests::Stage::rewarded||reopened.characterNumber(owner,false,[]{return true;}).value!=*before.value+750)throw std::runtime_error("Blood reward did not persist");
        }
        world.leave(42);if(endpoint.deleteCharacter({owner.account,owner.character}).status!=FW_OK)throw std::runtime_error("Necromancer fixture cleanup failed");
    }
    {
        auto monk=profile;auto appearance=std::uint32_t(profile.extended[2])|(std::uint32_t(profile.extended[3])<<16);
        appearance=(appearance&~(15u<<20))|(3u<<20);monk.extended[2]=static_cast<std::uint16_t>(appearance);monk.extended[3]=static_cast<std::uint16_t>(appearance>>16);
        const auto monk_name=unique.substr(0,10)+" Monk";
        const auto created=endpoint.createCharacter({progression_owner.account,monk_name,monk});if(created.status!=FW_OK||!created.created)throw std::runtime_error("Monk fixture creation failed");
        const gwserver::OwnedCharacter owner{progression_owner.account,created.created->character};const auto content=fw::gw::quests::load_catalog("data/gw/quests/presearing.fwqc");
        std::ifstream giver_file("data/gw/quests/actors/monk-test-givers.txt"),giver_metadata("data/gw/quests/monk-test-giver-combat.txt");auto givers=fw::gw::world::load_npc_catalog(giver_file);
        const std::string giver_hash="14b6cc1b3b97e5ad5b4bcb922486890c0675036440f23e93ff38d6ccb2001b1f";
        fw::gw::world::bind_combat_catalog(givers,fw::gw::combat::npcs::load_catalog(giver_metadata,giver_hash),giver_hash);
        std::ifstream enemies_file("data/gw/quests/actors/monk-test-skale.txt"),enemy_metadata("data/gw/quests/monk-test-enemy-combat.txt");auto enemies=fw::gw::world::load_npc_catalog(enemies_file);
        const std::string enemy_hash="faa268470eba50f6fe4ba66517c51c6823d5ffc045afe51f0ee6665a5c02a5ec";
        fw::gw::world::bind_combat_catalog(enemies,fw::gw::combat::npcs::load_catalog(enemy_metadata,enemy_hash),enemy_hash);
        const fw::gw::world::NpcCatalog::EscortEncounter encounter{146,enemies.for_map(146),"Gwen","Ciglo the Monk",220,1248,128,128};
        fw::gw::world::Instance world;world.configure_navigation(std::make_shared<fw::gw::navigation::Mesh>(fw::gw::navigation::Mesh::load(
            std::filesystem::path("data/gw/navigation")/(std::to_string(fw::gw::map_asset_file(146))+".fwnav"))));world.configure_npcs(givers.for_map(146));
        world.join(42,monk_name,3802,-4559,std::make_shared<fw::gw::world::Outbox>());world.set_vitals(42,{100,100,20,20},100);
        const auto actor=*world.effect_scope(42);const auto roles=fw::gw::world::escort_participants(world,givers,encounter);
        if(!roles||!fw::gw::world::accept_escort_encounter(world,actor,roles->first,roles->second,146,1,encounter,[&]{return gwserver::acceptCharacterQuest(endpoint,owner,content,84,false).status==FW_OK;}))throw std::runtime_error("Monk atomic acceptance failed");
        if(!world.discover_escort(actor,1,1248,101))throw std::runtime_error("Monk captured Gwen discovery failed");
        world.move(42,4702,-2843,102);world.tick(20102);
        for(unsigned id=60000;id<60003;++id)if(!world.damage_npc(actor,*world.effect_scope(id),{fw::gw::combat::Kind::armor_ignoring,20,20,194},1248,20103).killed)throw std::runtime_error("Monk blocker kill failed");
        if(world.pending_quest_completions(actor)!=std::vector<fw::gw::world::Instance::QuestCompletion>{{84,1}}||world.escort_reward_ready(actor,1)||
           gwserver::completeCharacterQuest(endpoint,owner,content,84,1).status!=FW_OK)throw std::runtime_error("Monk completed stage/early arrival gate failed");
        world.follow_escort(actor,1,220,128,20104);world.tick(40104);world.move(42,5942,4172,40105);
        for(std::uint64_t now=41105;now<=160105;now+=1000){world.tick(now);world.follow_escort(actor,1,220,128,now);}
        if(!world.arrive_escort(actor,1,128,160106)||!world.escort_reward_ready(actor,1))throw std::runtime_error("Monk actual escort arrival failed");
        const auto before=endpoint.characterExperience(owner);const auto skills=endpoint.characterSkills(owner);
        gwserver::RewardQuestExperience reward{owner,endpoint.characterQuests(owner).record->revision,84,1,500,skills.record->revision,{288,252},0,0,false,true};
        auto stale=reward;++stale.expected_skill_revision;if(gwserver::rewardCharacterQuest(endpoint,stale,nullptr,42).status!=FW_CONFLICT||endpoint.characterExperience(owner).value!=before.value)throw std::runtime_error("Monk stale reward changed XP");
        const auto result=gwserver::rewardCharacterQuest(endpoint,reward,nullptr,42);
        if(result.status!=FW_OK||std::find(result.packets.begin(),result.packets.end(),fw::gw::skills::slot_packet_2022(42,0,{288,0}))==result.packets.end()||
           std::find(result.packets.begin(),result.packets.end(),fw::gw::skills::slot_packet_2022(42,1,{252,0}))==result.packets.end()||gwserver::rewardCharacterQuest(endpoint,reward,nullptr,42).status==FW_OK)throw std::runtime_error("Monk reward slots/duplicate fence failed");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");const auto quests=reopened.characterQuests(owner,[]{return true;});const auto learned=reopened.characterSkills(owner,[]{return true;});const auto xp=reopened.characterNumber(owner,false,[]{return true;});
            if(quests.record->profile.records.at(84).stage!=fw::gw::quests::Stage::rewarded||learned.record->profile.primary!=3||learned.record->profile.bar[0]!=fw::gw::skills::Slot{288,0}||
               learned.record->profile.bar[1]!=fw::gw::skills::Slot{252,0}||xp.value!=*before.value+500)throw std::runtime_error("Monk reward lost quest/skills/XP on reopen");}
        world.leave(42);if(endpoint.deleteCharacter({owner.account,owner.character}).status!=FW_OK)throw std::runtime_error("Monk fixture cleanup failed");
    }
    {
        const auto name=unique.substr(0,10)+" Trial";
        const auto created=endpoint.createCharacter({progression_owner.account,name,profile});
        if(created.status!=FW_OK||!created.created)throw std::runtime_error("trial fixture creation failed");
        const gwserver::OwnedCharacter owner{progression_owner.account,created.created->character};
        const auto content=fw::gw::quests::load_catalog("data/gw/quests/presearing.fwqc");
        const auto* rule=fw::gw::quests::presearing::find(73);
        const auto rejected=gwserver::acceptCharacterQuest(endpoint,owner,content,73,false,rule->acceptance_skills,{},0,true);
        if(rejected.status!=FW_INVALID||!rejected.packets.empty()||endpoint.characterQuests(owner).record)
            throw std::runtime_error("invalid trial mutated quest state");
        const auto accepted=gwserver::acceptCharacterQuest(endpoint,owner,content,73,false,rule->acceptance_skills,{},42,true);
        if(accepted.status!=FW_OK)throw std::runtime_error("trial acceptance transaction failed: "+accepted.message);
        {fw::Accounts reopened("build/tests/network-lifecycle.db");
            const auto state=reopened.characterQuests(owner,[]{return true;});
            const auto skills=reopened.characterSkills(owner,[]{return true;});
            if(!state.record||!skills.record||state.record->profile.records.at(73).stage!=fw::gw::quests::Stage::active||
                skills.record->profile.secondary||skills.record->profile.trial_skills!=std::set<std::uint32_t>{197,180,200}||
                skills.record->profile.bar[0]!=fw::gw::skills::Slot{197,0})
                throw std::runtime_error("trial quest/skill state did not survive reopen");
            if(fw::gw::skills::validate(skills.record->profile.bar,fw::gw::skills::policy(skills.record->profile,0x1e,true))!=fw::gw::skills::Reason::profession)
                throw std::runtime_error("trial permission leaked outside authorized pre session");}
        const auto active_trial=endpoint.characterQuests(owner);const auto trial_skills=endpoint.characterSkills(owner);
        gwserver::SaveQuests stale_cleanup{owner,active_trial.record->revision,
            *fw::gw::quests::abandon(active_trial.record->profile,73,1)};
        stale_cleanup.abandon_trial_quest=73;stale_cleanup.expected_skill_revision=trial_skills.record->revision+1;
        if(endpoint.saveQuests(stale_cleanup).status!=FW_CONFLICT||
            endpoint.characterQuests(owner).record->profile.records.at(73).stage!=fw::gw::quests::Stage::active||
            endpoint.characterSkills(owner).record->profile.trial_skills!=trial_skills.record->profile.trial_skills)
            throw std::runtime_error("stale trial cleanup partially committed");
        const auto abandoned=gwserver::abandonCharacterQuest(endpoint,owner,73,42);
        if(abandoned.status!=FW_OK||abandoned.skill_packets.empty())throw std::runtime_error("trial abandonment omitted slot reconciliation");
        if(std::find(abandoned.skill_packets.begin(),abandoned.skill_packets.end(),
            fw::gw::skills::slot_packet_2022(42,0,{}))==abandoned.skill_packets.end())
            throw std::runtime_error("trial abandonment omitted explicit native slot clear");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");
            const auto skills=reopened.characterSkills(owner,[]{return true;});const auto quests=reopened.characterQuests(owner,[]{return true;});
            if(!skills.record->profile.trial_skills.empty()||skills.record->profile.bar[0].skill||
                !skills.record->profile.learned.contains(197)||quests.record->profile.records.at(73).stage!=fw::gw::quests::Stage::abandoned)
                throw std::runtime_error("trial abandonment did not persist coherent learning/bar/quest state");}
        const auto retry=gwserver::acceptCharacterQuest(endpoint,owner,content,73,false,rule->acceptance_skills,{},42,true);
        if(retry.status!=FW_OK||endpoint.characterSkills(owner).record->profile.bar[0]!=fw::gw::skills::Slot{197,0})
            throw std::runtime_error("reaccepting a trial failed to restore granted skills");
        if(gwserver::completeCharacterQuest(endpoint,owner,content,73,2).status!=FW_OK)
            throw std::runtime_error("trial completion fixture failed");
        const auto ready=endpoint.characterQuests(owner);const auto before_unlock=endpoint.characterSkills(owner);
        gwserver::RewardQuestExperience trial_reward{owner,ready.record->revision,73,2,250,before_unlock.record->revision,{},0,0,false,false,true};
        auto stale_unlock=trial_reward;++stale_unlock.expected_skill_revision;
        if(gwserver::rewardCharacterQuest(endpoint,stale_unlock,nullptr,42).status!=FW_CONFLICT||
            endpoint.characterQuests(owner).record->profile.records.at(73).stage!=fw::gw::quests::Stage::completed||
            endpoint.characterSkills(owner).record->profile.profession_mask!=before_unlock.record->profile.profession_mask)
            throw std::runtime_error("stale trial reward partially unlocked profession");
        const auto unlocked=gwserver::rewardCharacterQuest(endpoint,trial_reward,nullptr,42);
        if(unlocked.status!=FW_OK)throw std::runtime_error("trial reward/unlock transaction failed");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");
            const auto state=reopened.characterQuests(owner,[]{return true;});const auto skills=reopened.characterSkills(owner,[]{return true;});
            if(state.record->profile.records.at(73).stage!=fw::gw::quests::Stage::rewarded||skills.record->profile.secondary||
                !(skills.record->profile.profession_mask&(1u<<6)))throw std::runtime_error("trial unlock did not survive reopen without selecting secondary");
            const auto unlock_packet=fw::gw::skills::profession_packets_2022(42,1,0,skills.record->profile.profession_mask)[1];
            if(std::find(unlocked.packets.begin(),unlocked.packets.end(),unlock_packet)==unlocked.packets.end())
                throw std::runtime_error("trial unlock omitted native profession mask");}
        const auto selectable=endpoint.characterSkills(owner);const auto allocation=endpoint.characterAttributes(owner);
        if(allocation.status!=FW_OK)throw std::runtime_error("trial allocation lookup failed");
        const auto selected=fw::gw::professions::propose({42,6,0},selectable.record->profile,
            allocation.record?allocation.record->profile:fw::gw::attributes::Profile{}, {42,3,0x1e,1,false,true});
        if(!selected.accepted())throw std::runtime_error("unlocked trial selection failed");
        const auto committed=endpoint.saveSecondaryProfession({owner,selectable.record->revision,
            allocation.record?allocation.record->revision:0,selected.skill_profile,selected.attribute_profile});
        if(committed.status!=FW_OK)throw std::runtime_error("trial selection persistence failed: "+committed.message);
        {fw::Accounts reopened("build/tests/network-lifecycle.db");const auto skills=reopened.characterSkills(owner,[]{return true;});
            if(skills.record->profile.secondary!=6||!skills.record->profile.trial_skills.empty()||
                skills.record->profile.bar[0]!=fw::gw::skills::Slot{197,0})
                throw std::runtime_error("selected secondary lost learning/bar or retained trial permissions on reopen");}
        // A selected profession before accepting79 reconciles from persisted
        // selection, not a preview; a missing primary test still rejects.
        namespace second=fw::gw::quests::second_profession;
        const second::Interaction armin{{1,10021,1},{1,10021,1},"Armin Saberlin",148,true};
        const auto story_catalog=second::bind_markers(content,148);
        const auto denied_story=gwserver::acceptSecondProfession(endpoint,owner,story_catalog,1,{true,true,1,6},armin);
        if(denied_story.status!=FW_CONFLICT||!denied_story.packets.empty())throw std::runtime_error("quest79 bypassed primary test");
        const auto before_story=endpoint.characterQuests(owner).record;
        auto prerequisite=before_story->profile;prerequisite.records.emplace(85,fw::gw::quests::Progress{1,fw::gw::quests::Stage::rewarded});
        if(endpoint.saveQuests({owner,before_story->revision,prerequisite}).status!=FW_OK)throw std::runtime_error("quest79 prerequisite fixture failed");
        auto stale_armin=armin;++stale_armin.current.generation;
        const auto stale_offer=gwserver::acceptSecondProfession(endpoint,owner,story_catalog,1,{true,true,1,6},stale_armin);
        if(stale_offer.status!=FW_CONFLICT||!stale_offer.packets.empty())throw std::runtime_error("quest79 stale NPC accepted");
        const auto story_accepted=gwserver::acceptSecondProfession(endpoint,owner,story_catalog,1,{true,true,1,6},armin);
        if(story_accepted.status!=FW_OK||story_accepted.packets.size()!=2||story_accepted.packets[0][0]!=72||story_accepted.packets[1][0]!=75)
            throw std::runtime_error("quest79 acceptance did not publish exactly add/details");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");
            if(reopened.characterQuests(owner,[]{return true;}).record->profile.records.at(79).stage!=fw::gw::quests::Stage::active||
               reopened.characterSkills(owner,[]{return true;}).record->profile.secondary!=6)
                throw std::runtime_error("quest79 preselected crash-recovery fixture lost state");}
        const auto stale_story=gwserver::completeSecondProfession(endpoint,owner,story_catalog,2,true,true);
        if(stale_story.status!=FW_CONFLICT||!stale_story.packets.empty())throw std::runtime_error("quest79 stale cycle published completion");
        const auto completed_story=gwserver::completeSecondProfession(endpoint,owner,story_catalog,1,true,true);
        if(completed_story.status!=FW_OK||completed_story.packets.size()!=2||completed_story.packets[0][0]!=83||completed_story.packets[1][0]!=76)
            throw std::runtime_error("quest79 completion must publish objectives/marker rather than QuestAdded");
        const auto duplicate_story=gwserver::completeSecondProfession(endpoint,owner,story_catalog,1,true,true);
        if(duplicate_story.status!=FW_OK||!duplicate_story.packets.empty())throw std::runtime_error("quest79 duplicate completion published again");
        const auto story_ready=endpoint.characterQuests(owner).record;
        const auto story_xp=endpoint.characterExperience(owner).value;
        gwserver::RewardQuestExperience story_reward{owner,story_ready->revision,79,1,250};
        auto stale_story_reward=story_reward;--stale_story_reward.expected_revision;
        const auto conflict=gwserver::rewardCharacterQuest(endpoint,stale_story_reward);
        if(conflict.status!=FW_CONFLICT||!conflict.packets.empty()||endpoint.characterExperience(owner).value!=story_xp)
            throw std::runtime_error("quest79 stale reward published or paid");
        const auto story_paid=gwserver::rewardCharacterQuest(endpoint,story_reward);
        if(story_paid.status!=FW_OK||story_paid.packets!=fw::gw::quests::Batch{fw::gw::quests::remove_2022(79)}||
           endpoint.characterExperience(owner).value!=*story_xp+250)
            throw std::runtime_error("quest79 exact reward transaction/publication failed");
        const auto replay_story=gwserver::rewardCharacterQuest(endpoint,story_reward);
        if(replay_story.status!=FW_CONFLICT||!replay_story.packets.empty())throw std::runtime_error("quest79 reward replay published");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");const auto story=reopened.characterQuests(owner,[]{return true;});
            if(story.record->profile.records.at(79).stage!=fw::gw::quests::Stage::rewarded||
               reopened.characterNumber(owner,false,[]{return true;}).value!=*story_xp+250)
                throw std::runtime_error("quest79 reward did not survive reopen");}
        if(endpoint.deleteCharacter({owner.account,owner.character}).status!=FW_OK)throw std::runtime_error("trial fixture cleanup failed");
    }
    {
        namespace message=fw::gw::quests::message_from_friend;
        const auto name=unique.substr(0,10)+" Message";
        const auto created=endpoint.createCharacter({progression_owner.account,name,profile});
        if(created.status!=FW_OK||!created.created)throw std::runtime_error("quest80 fixture creation failed");
        const gwserver::OwnedCharacter owner{progression_owner.account,created.created->character};
        const auto content=message::bind_markers(fw::gw::quests::bind_catalog_character(
            fw::gw::quests::load_catalog("data/gw/quests/presearing.fwqc"),fw::gw::character_name_to_utf16(name,19)),148);
        const message::Interaction crier{{1,10002,1},{1,10002,1},"Town Crier",148,true};
        const message::Interaction tydus{{1,10001,1},{1,10001,1},"Sir Tydus",148,true};
        const auto accepted=gwserver::acceptMessageFromFriend(endpoint,owner,content,1,{true,true},crier);
        if(accepted.status!=FW_OK||accepted.packets.size()!=2||accepted.packets[0][0]!=72||accepted.packets[1][0]!=75)
            throw std::runtime_error("quest80 acceptance publication differs");
        fw::gw::inventory::identity::Session ids;
        auto empty_character=fw::gw::inventory::baseline(),empty_bank=fw::gw::storage::baseline();
        std::uint64_t absent_revision=0;
        const auto missing_inventory=gwserver::rewardMessageFromFriend(endpoint,owner,content,1,{true,true},tydus,
            ids,empty_character,empty_bank,absent_revision);
        if(missing_inventory.status!=FW_NOT_FOUND||!missing_inventory.packets.empty()||absent_revision!=0)
            throw std::runtime_error("quest80 missing inventory was not rejected without publication");
        if(endpoint.saveInventory({owner,0,fw::gw::inventory::baseline()}).status!=FW_OK)
            throw std::runtime_error("quest80 synthetic inventory initialization failed");
        const auto inventory_result=endpoint.characterInventory(owner);
        if(inventory_result.status!=FW_OK||!inventory_result.record)
            throw std::runtime_error("quest80 synthetic inventory baseline unavailable");
        const auto inventory=inventory_result.record;
        auto carried=ids.hydrate(inventory->profile,fw::gw::storage::baseline());
        auto revision=inventory->revision;const auto original_gold=carried.character.gold;
        const auto xp=endpoint.characterExperience(owner).value;
        const auto wrong_npc=gwserver::rewardMessageFromFriend(endpoint,owner,content,1,{true,true},crier,ids,carried.character,carried.bank,revision);
        if(wrong_npc.status!=FW_CONFLICT||!wrong_npc.packets.empty()||carried.character.gold!=original_gold)
            throw std::runtime_error("quest80 wrong NPC mutated inventory/publication");
        ++revision;
        const auto stale=gwserver::rewardMessageFromFriend(endpoint,owner,content,1,{true,true},tydus,ids,carried.character,carried.bank,revision);
        --revision;
        if(stale.status!=FW_CONFLICT||!stale.packets.empty()||endpoint.characterExperience(owner).value!=xp)
            throw std::runtime_error("quest80 stale inventory published or paid");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");
            if(reopened.characterQuests(owner,[]{return true;}).record->profile.records.at(80).stage!=fw::gw::quests::Stage::active)
                throw std::runtime_error("quest80 active attempt did not survive reconnect");}
        // Invalid/missing/unbound publication content must fail BEFORE paying.
        for(unsigned fault=0;fault<5;++fault){auto broken=content;
            switch(fault){case 0:broken.erase(80);break;
                case 1:broken.at(80).captured_character_argument=u"Logger";break;
                case 2:broken.at(80).description=std::u16string(128,u'x');break;
                case 3:broken.at(80).entry.x=std::bit_cast<float>(0x7fc00000u);break;
                case 4:broken.at(80).objectives.reset();break;}
            const auto rejected=gwserver::rewardMessageFromFriend(endpoint,owner,broken,1,{true,true},tydus,ids,carried.character,carried.bank,revision);
            if(rejected.status!=FW_INVALID||!rejected.packets.empty()||carried.character.gold!=original_gold||
               revision!=inventory->revision||endpoint.characterExperience(owner).value!=xp||
               endpoint.characterInventory(owner).record->profile.gold!=original_gold||
               endpoint.characterQuests(owner).record->profile.records.at(80).stage!=fw::gw::quests::Stage::active)
                throw std::runtime_error("quest80 invalid catalog paid or published before validation");
        }
        struct MessageRewardConflict:gwserver::NetworkServer {
            gwserver::NetworkComponent& backing;unsigned attempts=0;
            MessageRewardConflict(gwserver::NetworkManager& manager,gwserver::NetworkComponent& source):NetworkServer(manager),backing(source){}
            gwserver::QuestsResult characterQuests(const gwserver::OwnedCharacter& request)override{return backing.characterQuests(request);}
            gwserver::InventoryResult characterInventory(const gwserver::OwnedCharacter& request)override{return backing.characterInventory(request);}
            gwserver::RevisionResult rewardQuestExperience(const gwserver::RewardQuestExperience&)override{
                ++attempts;return {FW_CONFLICT,"injected Message reward CAS conflict",0};}
        } conflict_endpoint(manager,endpoint);
        const auto failed_commit=gwserver::rewardMessageFromFriend(conflict_endpoint,owner,content,1,{true,true},tydus,
            ids,carried.character,carried.bank,revision);
        if(conflict_endpoint.attempts!=1||failed_commit.status!=FW_CONFLICT||!failed_commit.packets.empty()||
           carried.character.gold!=original_gold||revision!=inventory->revision||endpoint.characterExperience(owner).value!=xp||
           endpoint.characterInventory(owner).record->profile.gold!=original_gold)
            throw std::runtime_error("quest80 durable CAS failure published prepared prefix or mutated projection");
        const auto expected_gold=fw::gw::inventory::gold::delta_2022(319,carried.character.inventory,10);
        const auto paid=gwserver::rewardMessageFromFriend(endpoint,owner,content,1,{true,true},tydus,ids,carried.character,carried.bank,revision);
        if(paid.status!=FW_OK||paid.packets!=fw::gw::quests::Batch{fw::gw::quests::marker_2022(content.at(80).entry),
           fw::gw::quests::details_2022(80,*content.at(80).description,*content.at(80).objectives),fw::gw::quests::remove_2022(80),expected_gold}||
           carried.character.gold!=original_gold+10||revision!=inventory->revision+1||endpoint.characterExperience(owner).value!=*xp+100)
            throw std::runtime_error("quest80 exact atomic reward/publication failed");
        for(const auto& packet:paid.packets){const auto opcode=unsigned(packet[0])|(unsigned(packet[1])<<8);
            if(opcode==72||opcode==79||opcode==83)throw std::runtime_error("quest80 turn-in invented QuestAdded/completion UI");}
        const auto original=fw::gw::quests::load_catalog("data/gw/quests/message-from-friend-80.fwqc");
        if(paid.packets[1]==fw::gw::quests::details_2022(80,*original.at(80).description,*original.at(80).objectives))
            throw std::runtime_error("quest80 refresh leaked captured Logger name instead of session binding");
        const auto projection=ids.prepare(carried.character,carried.bank);
        if(fw::gw::inventory::encode(projection.durable.character)!=fw::gw::inventory::encode(endpoint.characterInventory(owner).record->profile))
            throw std::runtime_error("quest80 session identity projection was not rebased after gold reward");
        const auto replay=gwserver::rewardMessageFromFriend(endpoint,owner,content,1,{true,true},tydus,ids,carried.character,carried.bank,revision);
        if(replay.status!=FW_CONFLICT||!replay.packets.empty())throw std::runtime_error("quest80 replay published");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");
            const auto quests=reopened.characterQuests(owner,[]{return true;});
            if(!fw::gw::quests::restore_2022(quests.record->profile,content).empty()||
               reopened.characterInventory(owner,[]{return true;}).record->profile.gold!=original_gold+10||
               reopened.characterNumber(owner,false,[]{return true;}).value!=*xp+100)
                throw std::runtime_error("quest80 rewarded reconnect state differs");}
        if(endpoint.deleteCharacter({owner.account,owner.character}).status!=FW_OK)throw std::runtime_error("quest80 fixture cleanup failed");
    }
    {
        namespace glory=fw::gw::quests::path_to_glory;
        const auto name=unique.substr(0,10)+" Glory";
        const auto created=endpoint.createCharacter({progression_owner.account,name,profile});
        if(created.status!=FW_OK||!created.created)throw std::runtime_error("quest89 fixture creation failed");
        const gwserver::OwnedCharacter owner{progression_owner.account,created.created->character};
        const auto skills=endpoint.characterSkills(owner);
        if(skills.status!=FW_OK||!skills.record||skills.record->profile.primary<1||skills.record->profile.primary>6)
            throw std::runtime_error("quest89 fixture lacks core primary baseline");
        const auto primary=skills.record->profile.primary;
        const auto unbound=fw::gw::quests::load_catalog("data/gw/quests/presearing.fwqc");
        const auto content=glory::bind_markers(fw::gw::quests::bind_catalog_character(unbound,
            fw::gw::character_name_to_utf16(name,19)),148);
        const glory::Interaction armin{{1,10021,1},{1,10021,1},"Armin Saberlin",148,true};
        const glory::Interaction tydus{{1,10001,1},{1,10001,1},"Sir Tydus",148,true};
        const glory::Eligibility eligible{true,true,primary};
        const auto denied=gwserver::acceptPathToGlory(endpoint,owner,content,1,eligible,armin);
        if(denied.status!=FW_CONFLICT||!denied.packets.empty()||endpoint.characterQuests(owner).record)
            throw std::runtime_error("quest89 bypassed primary-test admission");
        constexpr unsigned tests[]{0,85,86,84,82,81,83};
        fw::gw::quests::Profile state;state.records.emplace(tests[primary],fw::gw::quests::Progress{1,fw::gw::quests::Stage::rewarded});
        if(endpoint.saveQuests({owner,0,state}).status!=FW_OK)throw std::runtime_error("quest89 prerequisite fixture failed");
        const auto unbound_offer=gwserver::acceptPathToGlory(endpoint,owner,unbound,1,eligible,armin);
        if(unbound_offer.status!=FW_INVALID||!unbound_offer.packets.empty())throw std::runtime_error("quest89 acceptance leaked unbound character name");
        const auto accepted=gwserver::acceptPathToGlory(endpoint,owner,content,1,eligible,armin);
        if(accepted.status!=FW_OK||accepted.packets.size()!=2||accepted.packets[0][0]!=72||accepted.packets[1][0]!=75)
            throw std::runtime_error("quest89 acceptance add/details differs");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");const auto quests=reopened.characterQuests(owner,[]{return true;});
            if(!quests.record||fw::gw::quests::restore_2022(quests.record->profile,content)!=accepted.packets)
                throw std::runtime_error("quest89 active reconnect changed native restoration");}
        const auto xp=endpoint.characterExperience(owner).value;const auto map=endpoint.characterMap(owner).value;
        if(!xp||!map)throw std::runtime_error("quest89 fixture lacks XP/map baseline");
        for(unsigned fault=0;fault<4;++fault){auto broken=content;
            switch(fault){case 0:broken.erase(89);break;case 1:broken.at(89).description=std::u16string(128,u'x');break;
                case 2:broken.at(89).entry.x=std::bit_cast<float>(0x7fc00000u);break;case 3:broken.at(89).captured_character_argument=u"Logger";break;}
            const auto bad=gwserver::rewardPathToGlory(endpoint,owner,broken,1,eligible,tydus);
            if(bad.status!=FW_INVALID||!bad.packets.empty()||endpoint.characterExperience(owner).value!=xp)
                throw std::runtime_error("quest89 invalid catalog paid before publication validation");
        }
        struct GloryRewardConflict:gwserver::NetworkServer {
            gwserver::NetworkComponent& backing;unsigned attempts=0;
            GloryRewardConflict(gwserver::NetworkManager& manager,gwserver::NetworkComponent& source):NetworkServer(manager),backing(source){}
            gwserver::QuestsResult characterQuests(const gwserver::OwnedCharacter& request)override{return backing.characterQuests(request);}
            gwserver::RevisionResult rewardQuestExperience(const gwserver::RewardQuestExperience&)override{
                ++attempts;return {FW_CONFLICT,"injected Glory reward CAS conflict",0};}
        } conflict_endpoint(manager,endpoint);
        const auto failed=gwserver::rewardPathToGlory(conflict_endpoint,owner,content,1,eligible,tydus);
        if(conflict_endpoint.attempts!=1||failed.status!=FW_CONFLICT||!failed.packets.empty()||endpoint.characterExperience(owner).value!=xp)
            throw std::runtime_error("quest89 durable CAS failure published prepared refresh");
        const auto paid=gwserver::rewardPathToGlory(endpoint,owner,content,1,eligible,tydus);
        const fw::gw::quests::Batch expected{fw::gw::quests::marker_2022(content.at(89).entry),
            fw::gw::quests::details_2022(89,*content.at(89).description,*content.at(89).objectives),fw::gw::quests::remove_2022(89)};
        if(paid.status!=FW_OK||paid.packets!=expected||endpoint.characterExperience(owner).value!=*xp+100)
            throw std::runtime_error("quest89 exact postcommit76/75/81 reward differs");
        if(paid.packets[1]==fw::gw::quests::details_2022(89,*unbound.at(89).description,*unbound.at(89).objectives))
            throw std::runtime_error("quest89 refresh leaked captured Logger name");
        const auto replay=gwserver::rewardPathToGlory(endpoint,owner,content,1,eligible,tydus);
        if(replay.status!=FW_CONFLICT||!replay.packets.empty())throw std::runtime_error("quest89 replay published or paid");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");const auto quests=reopened.characterQuests(owner,[]{return true;});
            const auto restored_skills=reopened.characterSkills(owner,[]{return true;});
            if(!quests.record||!fw::gw::quests::restore_2022(quests.record->profile,content).empty()||
               reopened.characterNumber(owner,false,[]{return true;}).value!=*xp+100||reopened.characterNumber(owner,true,[]{return true;}).value!=map||
               !restored_skills.record||fw::gw::skills::encode_profile(restored_skills.record->profile)!=fw::gw::skills::encode_profile(skills.record->profile)||
               quests.record->profile.records.contains(79)||reopened.characterInventory(owner,[]{return true;}).record)
                throw std::runtime_error("quest89 reconnect changed map/profession/inventory or required79");}
        if(endpoint.deleteCharacter({owner.account,owner.character}).status!=FW_OK)throw std::runtime_error("quest89 fixture cleanup failed");
    }
    {
        auto novice=profile;novice.extended[12]=(novice.extended[12]&~0x1f0u)|(1u<<4);
        const auto created_level=endpoint.createCharacter({progression_owner.account,unique.substr(0,10)+" Level",novice});
        if(created_level.status!=FW_OK||!created_level.created)throw std::runtime_error("level fixture creation failed");
        const gwserver::OwnedCharacter level_owner{progression_owner.account,created_level.created->character};
        auto level_quest=*fw::gw::quests::accept({},101);
        level_quest=*fw::gw::quests::advance(level_quest,101,1,fw::gw::quests::Stage::active,fw::gw::quests::Stage::completed);
        if(endpoint.saveQuests({level_owner,0,level_quest}).status!=FW_OK||
           endpoint.rewardQuestExperience({level_owner,1,101,1,2000}).status!=FW_OK)
            throw std::runtime_error("level boundary reward failed");
        bool found_level=false;
        for(unsigned offset=0;offset<endpoint.characterRoster({level_owner.account,0}).total;offset+=6){
            for(const auto& row:endpoint.characterRoster({level_owner.account,offset}).entries)
                if(row.character.bytes==level_owner.character.bytes){found_level=true;if(!row.profile||((row.profile->extended[12]>>4)&31)!=2)
                    throw std::runtime_error("reward level not persisted in roster");}
        }
        {fw::Accounts reopened("build/tests/network-lifecycle.db");
            if(reopened.characterNumber(level_owner,false,[]{return true;}).value!=2000)
                throw std::runtime_error("level reward XP lost on reopen");}
        if(!found_level||endpoint.deleteCharacter({level_owner.account,level_owner.character}).status!=FW_OK)
            throw std::runtime_error("level fixture cleanup failed");
        namespace wizard=fw::gw::quests::wayward_wizard;
        const auto created=endpoint.createCharacter({progression_owner.account,unique.substr(0,10)+" Wizard",profile});
        if(created.status!=FW_OK||!created.created)throw std::runtime_error("Wayward fixture creation failed");
        const gwserver::OwnedCharacter owner{progression_owner.account,created.created->character};
        if(endpoint.saveInventory({owner,0,fw::gw::inventory::baseline()}).status!=FW_OK)throw std::runtime_error("Wayward inventory baseline failed");
        const auto content=fw::gw::quests::load_catalog("data/gw/quests/wayward-wizard.fwqc");
        const wizard::Eligibility eligibility{true,true,2};
        const wizard::Interaction ralena{{1,3,1},{1,3,1},"Ralena Stormbringer",165,true};
        const wizard::Interaction orion{{2,41,1},{2,41,1},"Orion Elek",161,true};
        const wizard::Interaction sandre{{3,42,1},{3,42,1},"Sandre Elek",148,true};
        if(gwserver::acceptCharacterQuest(endpoint,owner,content,55,false).status!=FW_OK||
           gwserver::advanceWaywardWizard(endpoint,owner,content,1,eligibility,orion,wizard::Action::talk_orion).status==FW_OK)
            throw std::runtime_error("Wayward ordered dialogue failed");
        const auto ralena_result=gwserver::advanceWaywardWizard(endpoint,owner,content,1,eligibility,ralena,wizard::Action::talk_ralena);
        if(ralena_result.status!=FW_OK||ralena_result.packets.size()!=1||ralena_result.packets[0][0]!=83)
            throw std::runtime_error("Wayward active dialogue must publish details only");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");
            const auto saved=reopened.characterQuests(owner,[]{return true;});
            if(saved.record->profile.records.at(55).objective_credits!=std::set<std::uint64_t>{1})throw std::runtime_error("Wayward stage persistence failed");}
        if(gwserver::advanceWaywardWizard(endpoint,owner,content,1,eligibility,orion,wizard::Action::talk_orion).status!=FW_OK)
            throw std::runtime_error("Wayward completion failed");
        const auto quest=endpoint.characterQuests(owner);const auto inventory=endpoint.characterInventory(owner);
        const auto before=endpoint.characterExperience(owner);
        fw::gw::inventory::identity::Session ids;
        const auto wire=ids.hydrate(inventory.record->profile,fw::gw::storage::baseline());
        const auto proposed=wizard::reward(quest.record->profile,wire.character,1,eligibility,sandre);
        if(!proposed)throw std::runtime_error("Wayward reward proposal failed");
        gwserver::RewardQuestExperience request{owner,quest.record->revision,55,1,250};request.expected_inventory_revision=inventory.record->revision;
        const auto unchanged=[&]{return endpoint.characterQuests(owner).record->revision==quest.record->revision&&
            endpoint.characterInventory(owner).record->revision==inventory.record->revision&&endpoint.characterExperience(owner).value==before.value&&ids.revision()==1;};
        auto stale=request;stale.expected_inventory_revision=0;
        if(gwserver::rewardCharacterQuestItem(endpoint,stale,ids,proposed->inventory,wire.bank).status!=FW_CONFLICT||!unchanged())
            throw std::runtime_error("Wayward stale inventory reward mutated state");
        stale=request;--stale.expected_revision;
        const auto stale_result=gwserver::rewardCharacterQuestItem(endpoint,stale,ids,proposed->inventory,wire.bank);
        if(stale_result.status!=FW_CONFLICT||!stale_result.packets.empty()||!unchanged())throw std::runtime_error("Wayward stale quest published reward");
        auto changed_bank=wire.bank;changed_bank.gold=1;
        if(gwserver::rewardCharacterQuestItem(endpoint,request,ids,proposed->inventory,changed_bank).status!=FW_INVALID||!unchanged())
            throw std::runtime_error("Wayward reward changed uncommitted bank");
        auto prepared=ids.prepare(proposed->inventory,wire.bank);auto durable_request=request;durable_request.reward_inventory=prepared.durable.character;
        auto wrong_item=durable_request;wrong_item.reward_inventory->items.at(inventory.record->profile.next_item).definition.model=1;
        if(endpoint.rewardQuestExperience(wrong_item).status!=FW_INVALID||!unchanged())throw std::runtime_error("Wayward invented reward accepted");
        {fw::Accounts direct("build/tests/network-lifecycle.db");unsigned checks=0;
            if(direct.rewardQuestExperience(durable_request,[&]{return ++checks<3;}).status!=FW_TIMEOUT||checks!=3||!unchanged())
                throw std::runtime_error("Wayward XP/item/quest rollback failed");}
        const auto result=gwserver::rewardCharacterQuestItem(endpoint,request,ids,proposed->inventory,wire.bank);
        if(result.status!=FW_OK||!result.profiles||result.packets.size()!=3||result.packets.back()[0]!=81||ids.revision()!=2)
            throw std::runtime_error("Wayward item identity publication failed");
        if(gwserver::rewardCharacterQuestItem(endpoint,request,ids,proposed->inventory,wire.bank).status==FW_OK)
            throw std::runtime_error("Wayward duplicate reward accepted");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");
            const auto q=reopened.characterQuests(owner,[]{return true;});const auto inv=reopened.characterInventory(owner,[]{return true;});
            if(q.record->profile.records.at(55).stage!=fw::gw::quests::Stage::rewarded||inv.record->profile.items.size()!=1||
               inv.record->profile.items.begin()->second.definition.model!=2949||reopened.characterNumber(owner,false,[]{return true;}).value!=*before.value+250)
                throw std::runtime_error("Wayward reward persistence failed");}
        if(endpoint.deleteCharacter({owner.account,owner.character}).status!=FW_OK)throw std::runtime_error("Wayward cleanup failed");
        {
        namespace wizard=fw::gw::quests::unsettling_rumors;
        const auto created=endpoint.createCharacter({progression_owner.account,unique.substr(0,10)+" Rumors",profile});
        if(created.status!=FW_OK||!created.created)throw std::runtime_error("Unsettling fixture creation failed");
        const gwserver::OwnedCharacter owner{progression_owner.account,created.created->character};
        if(endpoint.saveInventory({owner,0,fw::gw::inventory::baseline()}).status!=FW_OK)throw std::runtime_error("Unsettling inventory baseline failed");
        const auto content=fw::gw::quests::load_catalog("data/gw/quests/unsettling-rumors.fwqc");
        const wizard::Eligibility eligibility{true,true,true};
        const wizard::Interaction ralena{{1,3,1},{1,3,1},"Meerak the Scribe",164,true};
        const wizard::Interaction orion{{2,41,1},{2,41,1},"Armin Saberlin",148,true};
        const wizard::Interaction sandre{{3,42,1},{3,42,1},"Armin Saberlin",148,true};
        if(gwserver::acceptCharacterQuest(endpoint,owner,content,62,false).status==FW_OK)
            throw std::runtime_error("Unsettling accepted before primary test");
        const auto learned=endpoint.characterSkills(owner);
        if(!learned.record)throw std::runtime_error("Unsettling fixture profession missing");
        constexpr std::array<unsigned,7> tests{0,85,86,84,82,81,83};
        const auto primary=learned.record->profile.primary;
        if(!primary||primary>6)throw std::runtime_error("Unsettling fixture invalid primary");
        fw::gw::quests::Profile prerequisite;prerequisite.records.emplace(tests[primary],fw::gw::quests::Progress{1,fw::gw::quests::Stage::rewarded});
        if(endpoint.saveQuests({owner,0,prerequisite}).status!=FW_OK)throw std::runtime_error("Unsettling fixture prerequisite failed");
        if(gwserver::acceptCharacterQuest(endpoint,owner,content,62,false).status!=FW_OK||
           gwserver::advanceUnsettlingRumors(endpoint,owner,content,1,eligibility,orion,wizard::Action::talk_armin).status==FW_OK)
            throw std::runtime_error("Unsettling ordered dialogue failed");
        const auto ralena_result=gwserver::advanceUnsettlingRumors(endpoint,owner,content,1,eligibility,ralena,wizard::Action::talk_meerak);
        if(ralena_result.status!=FW_OK||ralena_result.packets.size()!=2||ralena_result.packets[0][0]!=83)
            throw std::runtime_error("Unsettling active dialogue must publish objectives and marker");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");
            const auto saved=reopened.characterQuests(owner,[]{return true;});
            if(saved.record->profile.records.at(62).objective_credits!=std::set<std::uint64_t>{1})throw std::runtime_error("Unsettling stage persistence failed");}
        if(gwserver::advanceUnsettlingRumors(endpoint,owner,content,1,eligibility,orion,wizard::Action::talk_armin).status!=FW_OK)
            throw std::runtime_error("Unsettling completion failed");
        const auto quest=endpoint.characterQuests(owner);const auto inventory=endpoint.characterInventory(owner);
        const auto before=endpoint.characterExperience(owner);
        fw::gw::inventory::identity::Session ids;
        const auto wire=ids.hydrate(inventory.record->profile,fw::gw::storage::baseline());
        const auto proposed=wizard::reward(quest.record->profile,wire.character,1,eligibility,sandre);
        if(!proposed)throw std::runtime_error("Unsettling reward proposal failed");
        gwserver::RewardQuestExperience request{owner,quest.record->revision,62,1,250};request.expected_inventory_revision=inventory.record->revision;
        const auto unchanged=[&]{return endpoint.characterQuests(owner).record->revision==quest.record->revision&&
            endpoint.characterInventory(owner).record->revision==inventory.record->revision&&endpoint.characterExperience(owner).value==before.value&&ids.revision()==1;};
        auto stale=request;stale.expected_inventory_revision=0;
        if(gwserver::rewardCharacterQuestItem(endpoint,stale,ids,proposed->inventory,wire.bank).status!=FW_CONFLICT||!unchanged())
            throw std::runtime_error("Unsettling stale inventory reward mutated state");
        stale=request;--stale.expected_revision;
        const auto stale_result=gwserver::rewardCharacterQuestItem(endpoint,stale,ids,proposed->inventory,wire.bank);
        if(stale_result.status!=FW_CONFLICT||!stale_result.packets.empty()||!unchanged())throw std::runtime_error("Unsettling stale quest published reward");
        auto changed_bank=wire.bank;changed_bank.gold=1;
        if(gwserver::rewardCharacterQuestItem(endpoint,request,ids,proposed->inventory,changed_bank).status!=FW_INVALID||!unchanged())
            throw std::runtime_error("Unsettling reward changed uncommitted bank");
        auto prepared=ids.prepare(proposed->inventory,wire.bank);auto durable_request=request;durable_request.reward_inventory=prepared.durable.character;
        auto injected=quest.record->profile;injected.records.at(62).stage=fw::gw::quests::Stage::rewarded;
        if(endpoint.saveQuests({owner,quest.record->revision,injected}).status!=FW_INVALID||!unchanged())
            throw std::runtime_error("Unsettling direct reward bypass accepted");
        for(unsigned variant=0;variant<5;++variant){auto wrong=durable_request;
            switch(variant){case 0:wrong.experience=251;break;case 1:wrong.gold=1;break;
                case 2:wrong.reward_inventory.reset();break;case 3:wrong.learned_skills={1};break;case 4:wrong.unlock_trial=true;break;}
            if(endpoint.rewardQuestExperience(wrong).status!=FW_INVALID||!unchanged())throw std::runtime_error("Unsettling reward policy bypass accepted");
        }
        auto wrong_item=durable_request;wrong_item.reward_inventory->items.at(inventory.record->profile.next_item).definition.model=1;
        if(endpoint.rewardQuestExperience(wrong_item).status!=FW_INVALID||!unchanged())throw std::runtime_error("Unsettling invented reward accepted");
        {fw::Accounts direct("build/tests/network-lifecycle.db");unsigned checks=0;
            if(direct.rewardQuestExperience(durable_request,[&]{return ++checks<3;}).status!=FW_TIMEOUT||checks!=3||!unchanged())
                throw std::runtime_error("Unsettling XP/item/quest rollback failed");}
        const auto result=gwserver::rewardCharacterQuestItem(endpoint,request,ids,proposed->inventory,wire.bank);
        if(result.status!=FW_OK||!result.profiles||result.packets.size()!=3||result.packets.back()[0]!=81||ids.revision()!=2)
            throw std::runtime_error("Unsettling item identity publication failed");
        if(gwserver::rewardCharacterQuestItem(endpoint,request,ids,proposed->inventory,wire.bank).status==FW_OK)
            throw std::runtime_error("Unsettling duplicate reward accepted");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");
            const auto q=reopened.characterQuests(owner,[]{return true;});const auto inv=reopened.characterInventory(owner,[]{return true;});
            if(q.record->profile.records.at(62).stage!=fw::gw::quests::Stage::rewarded||inv.record->profile.items.size()!=1||
               inv.record->profile.items.begin()->second.definition.model!=2817||reopened.characterNumber(owner,false,[]{return true;}).value!=*before.value+250)
                throw std::runtime_error("Unsettling reward persistence failed");}
        if(endpoint.deleteCharacter({owner.account,owner.character}).status!=FW_OK)throw std::runtime_error("Unsettling cleanup failed");
        }
        {
            namespace op=fw::gw::quests::opposition;
            const auto created=endpoint.createCharacter({progression_owner.account,unique.substr(0,10)+" Royal",profile});
            if(created.status!=FW_OK||!created.created)throw std::runtime_error("Opposition fixture creation failed");
            const gwserver::OwnedCharacter owner{progression_owner.account,created.created->character};
            const auto catalog=fw::gw::quests::load_catalog("data/gw/quests/opposition-to-the-king.fwqc");
            const op::Eligibility eligible{true,true};
            if(gwserver::acceptCharacterQuest(endpoint,owner,catalog,78,false).status!=FW_OK)throw std::runtime_error("Opposition acceptance failed");
            auto accepted=endpoint.characterQuests(owner);auto injected=accepted.record->profile;
            injected.records.at(78).stage=fw::gw::quests::Stage::completed;injected.records.at(78).objective_credits={1,2,3};
            if(endpoint.saveQuests({owner,accepted.record->revision,injected}).status!=FW_INVALID)throw std::runtime_error("Opposition skipped witnesses accepted");
            // The mixed Green Hills/Estate path exercises both eligible maps.
            for(unsigned credit:{1u,4u,5u}){
                const auto& w=op::witnesses[credit-1];const op::Interaction npc{{7,credit,1},{7,credit,1},w.name,w.map,true};
                const auto result=gwserver::questionOppositionWitness(endpoint,owner,catalog,1,eligible,npc);
                if(result.status!=FW_OK||result.packets.size()!=2||result.packets[0][0]!=83||result.packets[1][0]!=76)
                    throw std::runtime_error("Opposition question publication failed");
                const auto duplicate=gwserver::questionOppositionWitness(endpoint,owner,catalog,1,eligible,npc);
                if(duplicate.status!=FW_CONFLICT||!duplicate.packets.empty())throw std::runtime_error("Opposition duplicated witness credit");
                fw::Accounts reopened("build/tests/network-lifecycle.db");
                if(!reopened.characterQuests(owner,[]{return true;}).record->profile.records.at(78).objective_credits.contains(credit))
                    throw std::runtime_error("Opposition witness credit lost on reopen");
            }
            const auto quest=endpoint.characterQuests(owner);const auto before=endpoint.characterExperience(owner);
            const auto unchanged=[&]{return endpoint.characterQuests(owner).record->revision==quest.record->revision&&endpoint.characterExperience(owner).value==before.value;};
            gwserver::RewardQuestExperience request{owner,quest.record->revision,78,1,500};
            for(unsigned variant=0;variant<6;++variant){auto wrong=request;
                switch(variant){case 0:wrong.experience=501;break;case 1:wrong.gold=1;break;case 2:wrong.learned_skills={1};break;
                    case 3:wrong.unlock_trial=true;break;case 4:wrong.fill_skillbar=true;break;case 5:wrong.reward_inventory=fw::gw::inventory::baseline();break;}
                if(endpoint.rewardQuestExperience(wrong).status!=FW_INVALID||!unchanged())throw std::runtime_error("Opposition reward policy bypass accepted");
            }
            auto stale=request;--stale.expected_revision;const auto rejected=gwserver::rewardCharacterQuest(endpoint,stale);
            if(rejected.status!=FW_CONFLICT||!rejected.packets.empty()||!unchanged())throw std::runtime_error("Opposition stale reward published");
            injected=quest.record->profile;injected.records.at(78).stage=fw::gw::quests::Stage::rewarded;
            if(endpoint.saveQuests({owner,quest.record->revision,injected}).status!=FW_INVALID||!unchanged())throw std::runtime_error("Opposition direct reward save accepted");
            {fw::Accounts direct("build/tests/network-lifecycle.db");unsigned checks=0;
                if(direct.rewardQuestExperience(request,[&]{return ++checks<3;}).status!=FW_TIMEOUT||!unchanged())throw std::runtime_error("Opposition XP rollback failed");}
            const auto rewarded=gwserver::rewardCharacterQuest(endpoint,request);
            if(rewarded.status!=FW_OK||rewarded.packets.size()!=1||rewarded.packets[0][0]!=81)throw std::runtime_error("Opposition reward publication failed");
            if(gwserver::rewardCharacterQuest(endpoint,request).status==FW_OK)throw std::runtime_error("Opposition duplicate reward accepted");
            {fw::Accounts reopened("build/tests/network-lifecycle.db");
                if(reopened.characterQuests(owner,[]{return true;}).record->profile.records.at(78).stage!=fw::gw::quests::Stage::rewarded||
                   reopened.characterNumber(owner,false,[]{return true;}).value!=*before.value+500)throw std::runtime_error("Opposition XP/reward lost on reopen");}
            if(endpoint.deleteCharacter({owner.account,owner.character}).status!=FW_OK)throw std::runtime_error("Opposition cleanup failed");
        }

    }
    {
        const auto name=unique.substr(0,10)+" Scale";
        auto elementalist_profile=profile;
        auto elementalist_appearance=std::uint32_t(profile.extended[2])|(std::uint32_t(profile.extended[3])<<16);
        elementalist_appearance=(elementalist_appearance&~(15u<<20))|(6u<<20);
        elementalist_profile.extended[2]=static_cast<std::uint16_t>(elementalist_appearance);
        elementalist_profile.extended[3]=static_cast<std::uint16_t>(elementalist_appearance>>16);
        const auto created_scale=endpoint.createCharacter({progression_owner.account,name,elementalist_profile});
        if(created_scale.status!=FW_OK||!created_scale.created)throw std::runtime_error("collection character creation failed");
        const gwserver::OwnedCharacter owner{progression_owner.account,created_scale.created->character};
        if(endpoint.saveInventory({owner,0,fw::gw::inventory::baseline()}).status!=FW_OK)throw std::runtime_error("collection inventory baseline failed");
        const auto content=fw::gw::quests::load_catalog("data/gw/quests/candidates/quest-83.fwqc");
        fw::gw::world::Instance world;auto output=std::make_shared<fw::gw::world::Outbox>();
        world.configure_navigation(std::make_shared<fw::gw::navigation::Mesh>(fw::gw::navigation::Mesh::load(
            std::filesystem::path("data/gw/navigation")/(std::to_string(fw::gw::map_asset_file(146))+".fwnav"))));
        auto scale_npc=objective_npc;scale_npc.x=4626.897f;scale_npc.y=-2924.479f;
        world.join(42,name,scale_npc.x,scale_npc.y,output);world.set_vitals(42,{100,100,20,20},100);const auto actor=*world.effect_scope(42);
        if(!world.spawn_loot_encounter(actor,83,1,{scale_npc},100,[&]{
            return gwserver::acceptCharacterQuest(endpoint,owner,content,83,false).status==FW_OK;
        }))throw std::runtime_error("persistent collection acceptance failed");
        const auto enemy=*world.effect_scope(60000);
        if(!world.damage_npc(actor,enemy,{fw::gw::combat::Kind::armor_ignoring,200,20,194},1248,101).killed)
            throw std::runtime_error("collection enemy did not die");
        const auto floor=world.loot_npc_death(actor,enemy,fw::gw::inventory::presearing::shimmering_scale_definition(),101,60000,83,1);
        if(!floor||gwserver::completeElementalistTest(endpoint,owner,content,1).status!=FW_CONFLICT)
            throw std::runtime_error("floor loot incorrectly counted as possession");
        fw::gw::inventory::identity::Session ids;
        auto carried=ids.hydrate(endpoint.characterInventory(owner).record->profile,fw::gw::storage::baseline());
        const auto credit=fw::gw::inventory::ground::credit(carried.character,*floor,1,true);
        if(!credit.accepted())throw std::runtime_error("captured scale credit plan rejected");
        auto projection=ids.prepare(credit.profile,carried.bank);
        bool save_attempted=false;
        if(world.pickup_ground_item(actor,floor->scope,1,160.f,102,[&](const auto&,unsigned){save_attempted=true;return false;})||
           !save_attempted||!world.ground_item(floor->scope.actor))throw std::runtime_error("failed scale save removed floor item or bypassed callback");
        const bool picked=world.pickup_ground_item(actor,floor->scope,1,160.f,103,[&](const auto&,unsigned){
            const auto saved=endpoint.saveInventory({owner,endpoint.characterInventory(owner).record->revision,projection.durable.character});
            return saved.status==FW_OK&&ids.commit(std::move(projection));
        });
        if(!picked||world.ground_item(floor->scope.actor))throw std::runtime_error("scale pickup persistence/removal failed");
        if(gwserver::completeElementalistTest(endpoint,owner,content,1).status!=FW_OK)
            throw std::runtime_error("persisted collection did not complete");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");
            const auto restored=reopened.characterQuests(owner,[]{return true;});
            if(restored.status!=FW_OK||restored.record->profile.records.at(83).stage!=fw::gw::quests::Stage::completed)
                throw std::runtime_error("collection completion lost on reopen");}
        const auto quests=endpoint.characterQuests(owner);const auto skills=endpoint.characterSkills(owner);
        const auto xp=endpoint.characterExperience(owner);
        if(!skills.record||!xp.value)throw std::runtime_error("collection reward baselines unavailable");
        const gwserver::RewardQuestExperience reward{owner,quests.record->revision,83,1,500,skills.record->revision,{194,180},endpoint.characterInventory(owner).record->revision,0,true,true};
        if(gwserver::rewardCharacterQuest(endpoint,reward,&ids).status!=FW_INVALID)throw std::runtime_error("reward slot publication accepted without actor");
        const auto published_reward=gwserver::rewardCharacterQuest(endpoint,reward,&ids,42);
        if(published_reward.status!=FW_OK||
           std::find(published_reward.packets.begin(),published_reward.packets.end(),fw::gw::skills::slot_packet_2022(42,0,{194,0}))==published_reward.packets.end()||
           endpoint.characterExperience(owner).value!=*xp.value+500||
           fw::gw::inventory::presearing::carried_shimmering_scale(endpoint.characterInventory(owner).record->profile)||
           gwserver::rewardCharacterQuest(endpoint,reward,&ids,42).status==FW_OK)
            throw std::runtime_error("persistent collection reward or duplicate fence failed");
        {fw::Accounts reopened("build/tests/network-lifecycle.db");
            const auto restored=reopened.characterQuests(owner,[]{return true;});
            const auto learned=reopened.characterSkills(owner,[]{return true;});
            const auto inventory=reopened.characterInventory(owner,[]{return true;});
            if(restored.record->profile.records.at(83).stage!=fw::gw::quests::Stage::rewarded||
               !learned.record->profile.learned.contains(194)||!learned.record->profile.learned.contains(180)||
               learned.record->profile.bar[0]!=fw::gw::skills::Slot{194,0}||learned.record->profile.bar[1]!=fw::gw::skills::Slot{180,0}||
               fw::gw::inventory::presearing::carried_shimmering_scale(inventory.record->profile))
                throw std::runtime_error("collection reward lost skills/item/quest consistency on reopen");}
        world.leave(42);
        if(endpoint.deleteCharacter({owner.account,owner.character}).status!=FW_OK)throw std::runtime_error("collection fixture cleanup failed");
    }
    if(game_login.status!=FW_OK||!game_login.login)throw std::runtime_error("game typed login failed");
    const auto game_session=game_login.login->session;
    const auto issued_ticket=endpoint.issueTicket({game_session});
    if(issued_ticket.status!=FW_OK||!issued_ticket.ticket)throw std::runtime_error("issued ticket encoding");
    gwserver::AuthenticateTicket game_request{issued_ticket.ticket->token,gwserver::Identity::from_hex(account),unique.substr(8),unique};
    auto wrong_claim=game_request;wrong_claim.account.bytes[0]^=1;
    if(endpoint.authenticateTicket(wrong_claim).status!=FW_NOT_FOUND)throw std::runtime_error("ticket accepted wrong account");
    {
     fw::Accounts direct("build/tests/network-lifecycle.db");
     auto unavailable_character=game_request;unavailable_character.character="No Such Character";
     const auto denied_character=direct.authenticateTicket(unavailable_character,[]{return true;});
     if(denied_character.status!=FW_NOT_FOUND||denied_character.identity)throw std::runtime_error("foreign/unknown character claim accepted");
     auto invalid_alias=game_request;invalid_alias.alias="bad\talias";
     const auto denied_alias=direct.authenticateTicket(invalid_alias,[]{return true;});
     if(denied_alias.status!=FW_INVALID||denied_alias.identity)throw std::runtime_error("invalid alias accepted");
     for(unsigned stop=1;stop<=5;++stop){unsigned checks=0;const auto result=direct.authenticateTicket(game_request,[&]{return ++checks<stop;});if(result.status!=FW_TIMEOUT||result.identity||checks!=stop)throw std::runtime_error("cancelled authentication consumed ticket or exposed identity");}
    }
    const auto game_identity=endpoint.authenticateTicket(game_request);
    if(game_identity.status!=FW_OK||!game_identity.identity||!game_identity.identity->character||game_identity.identity->character->bytes!=created.created->character.bytes||game_identity.identity->map!=148||game_identity.identity->roster_size!=1)throw std::runtime_error("typed world-entry identity failed");
    if(endpoint.authenticateTicket(game_request).status!=FW_NOT_FOUND)throw std::runtime_error("ticket replay accepted");
    if(manager.client().authenticateTicket(game_request).status!=FW_UNAVAILABLE)throw std::runtime_error("custom client locally authenticated");
    if(!game_identity.identity->lease)throw std::runtime_error("authenticated session has no cleanup lease");
    {
        fw::Accounts cancellation_store("build/tests/network-lifecycle.db");
        unsigned admission=0;
        if(cancellation_store.releaseGameSession(*game_identity.identity->lease,[&]{return ++admission<2;}).status!=FW_TIMEOUT||
           endpoint.validateSession({game_session}).status!=FW_OK)
            throw std::runtime_error("expired session cleanup committed instead of rolling back");
    }
    auto wrong_lease=*game_identity.identity->lease;wrong_lease.account.bytes[0]^=1;
    if(endpoint.releaseGameSession(wrong_lease).status!=FW_NOT_FOUND)throw std::runtime_error("foreign account session released");
    if(endpoint.releaseGameSession(*game_identity.identity->lease).status!=FW_OK||
       endpoint.releaseGameSession(*game_identity.identity->lease).status!=FW_NOT_FOUND)
        throw std::runtime_error("session release is not owned/one-shot");
    if(manager.client().releaseGameSession(*game_identity.identity->lease).status!=FW_UNAVAILABLE)
        throw std::runtime_error("custom client locally released session");
    if(endpoint.validateSession({game_session}).status!=FW_NOT_FOUND)
        throw std::runtime_error("released game session remains valid");
    // Respect the independent ten-attempt login rate limit in this test.
    for(unsigned attempt=0;attempt<6;++attempt){
        const auto repeat=endpoint.login(credentials);
        if(repeat.status!=FW_OK||!repeat.login)throw std::runtime_error("repeated launch exhausted account sessions");
        const auto ticket=endpoint.issueTicket({repeat.login->session});
        if(ticket.status!=FW_OK||!ticket.ticket)throw std::runtime_error("repeat ticket unavailable");
        auto claim=game_request;claim.ticket=ticket.ticket->token;
        const auto authenticated=endpoint.authenticateTicket(claim);
        if(authenticated.status!=FW_OK||!authenticated.identity||!authenticated.identity->lease||
           endpoint.releaseGameSession(*authenticated.identity->lease).status!=FW_OK)
            throw std::runtime_error("repeat session ownership/cleanup failed");
    }
    if(endpoint.logout({game_session}).status!=FW_NOT_FOUND)throw std::runtime_error("released game session accepted logout");
    const auto roster=endpoint.characterRoster({gwserver::Identity::from_hex(account),0});
    if(roster.status!=FW_OK||roster.total!=1||roster.entries.size()!=1||
       gwserver::storage_hex(roster.entries[0].character.bytes)!=character||!roster.entries[0].profile||
       roster.entries[0].profile->encode()!=profile.encode()||endpoint.characterProgression({gwserver::Identity::from_hex(account),gwserver::Identity::from_hex(character)}).status!=FW_OK)
        throw std::runtime_error("shared roster/progression lost created character");
    const gwserver::OwnedCharacter persisted_owner{gwserver::Identity::from_hex(account),gwserver::Identity::from_hex(character)};
    const auto renamed=endpoint.renameCharacter({persisted_owner,unique,unique.substr(0,13)+" New"});
    if(renamed.status!=FW_OK||renamed.value!=unique.substr(0,13)+" New"||
       endpoint.renameCharacter({persisted_owner,unique,"Other Hero"}).status!=FW_CONFLICT||
       endpoint.renameCharacter({{{},persisted_owner.character},*renamed.value,"Other Hero"}).status!=FW_NOT_FOUND||
       endpoint.renameCharacter({persisted_owner,*renamed.value,unique}).status!=FW_OK)
        throw std::runtime_error("typed rename lost ownership/name compare-and-swap");
    if(endpoint.characterName(persisted_owner).value!=unique||endpoint.characterMap(persisted_owner).value!=148||
       endpoint.characterExperience({{},persisted_owner.character}).status!=FW_NOT_FOUND||endpoint.characterName({{},persisted_owner.character}).status!=FW_NOT_FOUND)
        throw std::runtime_error("typed character state lost persistence/ownership");
    const auto empty_inventory=endpoint.characterInventory(persisted_owner);
    const auto empty_storage=endpoint.accountStorage(persisted_owner.account);
    if(empty_inventory.status!=FW_OK||empty_inventory.record||empty_storage.status!=FW_OK||empty_storage.record)
        throw std::runtime_error("new inventory/storage should be absent");
    if(endpoint.characterInventory({{},persisted_owner.character}).status!=FW_NOT_FOUND||endpoint.accountStorage({}).status!=FW_NOT_FOUND)
        throw std::runtime_error("typed inventory read lost ownership checks");
    auto sample_inventory=fw::gw::inventory::baseline();sample_inventory.gold=12345;
    auto sample_storage=fw::gw::storage::baseline();sample_storage.gold=54321;
    if(endpoint.saveInventory({persisted_owner,0,sample_inventory}).status!=FW_OK||
       endpoint.transferStorage({persisted_owner,1,0,sample_inventory,sample_storage}).status!=FW_OK)
        throw std::runtime_error("inventory persistence fixture failed");
    const auto loaded_inventory=endpoint.characterInventory(persisted_owner);
    const auto loaded_storage=endpoint.accountStorage(persisted_owner.account);
    if(!loaded_inventory.record||loaded_inventory.record->revision!=2||loaded_inventory.record->profile.gold!=12345||
       !loaded_storage.record||loaded_storage.record->revision!=1||loaded_storage.record->profile.gold!=54321)
        throw std::runtime_error("typed inventory/storage reads lost persisted revisions or profiles");
    auto conflicted_inventory=sample_inventory;conflicted_inventory.gold=111;
    auto conflicted_bank=sample_storage;conflicted_bank.gold=222;
    if(endpoint.saveInventory({persisted_owner,1,conflicted_inventory}).status!=FW_CONFLICT||
       endpoint.transferStorage({persisted_owner,2,0,conflicted_inventory,conflicted_bank}).status!=FW_CONFLICT||
       endpoint.characterInventory(persisted_owner).record->profile.gold!=12345||
       endpoint.characterInventory(persisted_owner).record->revision!=2||
       endpoint.accountStorage(persisted_owner.account).record->profile.gold!=54321||
       endpoint.accountStorage(persisted_owner.account).record->revision!=1)
        throw std::runtime_error("inventory conflict partially committed character or bank");
    const auto peer_registered=endpoint.registerAccount({unique.substr(8)+"peer",password_span});
    if(peer_registered.status!=FW_OK||!peer_registered.login)throw std::runtime_error("trade peer registration failed");
    const auto peer_account=peer_registered.login->identity.account;
    const auto peer_character=endpoint.createCharacter({peer_account,unique.substr(0,13)+" Peer",profile});
    if(peer_character.status!=FW_OK||!peer_character.created)throw std::runtime_error("trade peer creation failed");
    const gwserver::OwnedCharacter peer_owner{peer_account,peer_character.created->character};
    const gwserver::AddFriend persisted_friend{persisted_owner.account,1,unique.substr(0,13)+" Peer","Peer Alias"};
    {
        fw::Accounts direct("build/tests/network-lifecycle.db");unsigned checks=0;
        if(direct.addFriend(persisted_friend,[&]{return ++checks<3;}).status!=FW_TIMEOUT||!endpoint.friends(persisted_owner.account).entries.empty())
            throw std::runtime_error("cancelled friend addition committed");
        if(endpoint.addFriend({persisted_owner.account,1,*endpoint.characterName(persisted_owner).value,"Self"}).status!=FW_INVALID)
            throw std::runtime_error("friend self addition accepted");
    }
    const auto friend_added=endpoint.addFriend(persisted_friend);
    if(friend_added.status!=FW_OK||!friend_added.value||friend_added.value->target.bytes!=peer_account.bytes||friend_added.value->alias!="Peer Alias")throw std::runtime_error("typed friend addition did not resolve account");
    const auto persisted_friends=endpoint.friends(persisted_owner.account);
    if(persisted_friends.status!=FW_OK||persisted_friends.entries.size()!=1||persisted_friends.entries[0].alias!="Peer Alias"||endpoint.friendBlocked({persisted_owner.account,peer_account}).value!=false)throw std::runtime_error("typed friend state not persisted");
    {
        fw::Accounts direct("build/tests/network-lifecycle.db");unsigned checks=0;
        const auto cancelled=direct.friends(persisted_owner.account,[&]{return ++checks<3;});
        if(cancelled.status!=FW_TIMEOUT||!cancelled.entries.empty())throw std::runtime_error("cancelled friend list exposed partial records");
        const auto unicode=endpoint.addFriend({persisted_owner.account,1,persisted_friend.name,"\xec\xb9\x9c\xea\xb5\xac"});
        const auto listed=endpoint.friends(persisted_owner.account);
        if(unicode.status!=FW_OK||listed.status!=FW_OK||listed.entries.size()!=1||listed.entries[0].alias!="\xec\xb9\x9c\xea\xb5\xac"||listed.entries[0].character!=persisted_friend.name)
            throw std::runtime_error("native friend Unicode or character resolution lost");
    }
    auto ignored_friend=persisted_friend;ignored_friend.type=2;
    if(endpoint.addFriend(ignored_friend).status!=FW_OK||endpoint.friendBlocked({persisted_owner.account,peer_account}).value!=true)throw std::runtime_error("typed ignore conversion not persisted");
    {
        fw::Accounts direct("build/tests/network-lifecycle.db");
        unsigned checks=0;
        if(direct.removeFriend({persisted_owner.account,peer_account},[&]{return ++checks<3;}).status!=FW_TIMEOUT||endpoint.friendBlocked({persisted_owner.account,peer_account}).value!=true)
            throw std::runtime_error("cancelled friend removal committed");
        if(endpoint.friendBlocked({peer_account,persisted_owner.account}).value!=false)
            throw std::runtime_error("ignore relationship leaked to reverse owner");
        if(direct.friendBlocked({persisted_owner.account,peer_account},[]{return false;}).status!=FW_TIMEOUT)
            throw std::runtime_error("cancelled ignore read succeeded");
    }
    if(endpoint.removeFriend({persisted_owner.account,peer_account}).status!=FW_OK||!endpoint.friends(persisted_owner.account).entries.empty()||endpoint.friendBlocked({persisted_owner.account,peer_account}).value!=false)throw std::runtime_error("typed friend removal not persisted");
    if(gwserver::decode_friend_add({FW_OK,gwserver::storage_hex(peer_account.bytes)+"\t416c696173"},persisted_friend).status!=FW_INTERNAL)throw std::runtime_error("friend response alias mismatch accepted");
    const gwserver::ExchangeTrade gold_trade{{persisted_owner,2,{100,{}}},{peer_owner,0,{}}};
    const auto exchanged=endpoint.exchangeTrade(gold_trade);
    if(exchanged.status!=FW_OK||!exchanged.left||!exchanged.right||exchanged.left->revision!=3||exchanged.right->revision!=1||
       exchanged.left->profile.gold!=12245||exchanged.right->profile.gold!=100)
        throw std::runtime_error("typed trade failed atomic gold exchange");
    if(endpoint.exchangeTrade(gold_trade).status!=FW_CONFLICT||
       endpoint.characterInventory(persisted_owner).record->profile.gold!=12245||endpoint.characterInventory(peer_owner).record->profile.gold!=100)
        throw std::runtime_error("stale typed trade changed balances");
    auto foreign_trade=gold_trade;foreign_trade.left.owner.account=peer_account;foreign_trade.left.expected_revision=3;foreign_trade.right.expected_revision=1;
    // Distinct-account checks reject this forged same-account exchange before storage.
    if(endpoint.exchangeTrade(foreign_trade).status!=FW_INVALID)throw std::runtime_error("forged trade ownership accepted");
    foreign_trade.right.owner.account=persisted_owner.account;
    if(endpoint.exchangeTrade(foreign_trade).status!=FW_NOT_FOUND||
       endpoint.characterInventory(persisted_owner).record->profile.gold!=12245||endpoint.characterInventory(peer_owner).record->profile.gold!=100)
        throw std::runtime_error("foreign-owned trade reached inventory mutation");
    if(endpoint.deleteCharacter({peer_owner.account,peer_owner.character}).status!=FW_OK)throw std::runtime_error("trade fixture cleanup failed");
    if(endpoint.characterSkills(persisted_owner).status!=FW_OK||!endpoint.characterSkills(persisted_owner).record||endpoint.characterSkills(persisted_owner).record->revision!=1)throw std::runtime_error("new character profession should be durable");
    {
        fw::Accounts importer("build/tests/network-lifecycle.db");
        const auto imported=importer.execute("gw-character-skills-import",owned+"\t1\t"+sample_skill_hex+"\t"+std::string(64,'a'),[]{return true;});
        if(imported.status!=FW_OK)throw std::runtime_error("skill fixture import failed");
    }
    if(endpoint.characterSkills({{},persisted_owner.character}).status!=FW_NOT_FOUND||endpoint.characterAttributes({{},persisted_owner.character}).status!=FW_NOT_FOUND)throw std::runtime_error("skill reads bypassed ownership");
    const auto baseline_saved=endpoint.saveAttributes({persisted_owner,0,sample_attributes});
    if(baseline_saved.status!=FW_OK||baseline_saved.revision!=1)throw std::runtime_error("typed baseline attribute save failed");
    const auto bar_saved=endpoint.saveSkillbar({persisted_owner,2,{}});
    if(bar_saved.status!=FW_OK||bar_saved.revision!=3||endpoint.saveSkillbar({persisted_owner,2,{}}).status!=FW_CONFLICT)throw std::runtime_error("typed bar save lost compare-and-swap");
    auto changed_profession=sample_skills;changed_profession.secondary=2;
    if(endpoint.saveSecondaryProfession({persisted_owner,3,0,changed_profession,sample_attributes}).status!=FW_CONFLICT||endpoint.characterSkills(persisted_owner).record->revision!=3)throw std::runtime_error("profession conflict partially committed");
    const auto profession_saved=endpoint.saveSecondaryProfession({persisted_owner,3,1,changed_profession,sample_attributes});
    if(profession_saved.status!=FW_OK||profession_saved.skill_revision!=4||profession_saved.attribute_revision!=2)throw std::runtime_error("typed atomic profession save failed");
    const auto persisted_skills=endpoint.characterSkills(persisted_owner);
    const auto persisted_attributes=endpoint.characterAttributes(persisted_owner);
    if(!persisted_skills.record||persisted_skills.record->revision!=4||persisted_skills.record->profile.secondary!=2||!persisted_attributes.record||persisted_attributes.record->revision!=2)throw std::runtime_error("typed profession state not persisted");
    const auto account_identity=gwserver::Identity::from_hex(account);
    const auto character_identity=gwserver::Identity::from_hex(character);
    {
        fw::Accounts cancellation_store("build/tests/network-lifecycle.db");
        const auto before=endpoint.characterRoster({account_identity,0});
        unsigned admissions=0;
        if(cancellation_store.selectMap({account_identity,character_identity,148,55},[&]{return ++admissions<2;}).status!=FW_TIMEOUT)
            throw std::runtime_error("expired map transition not cancelled");
        const auto after=endpoint.characterRoster({account_identity,0});
        if(after.status!=FW_OK||after.entries[0].map!=148||after.entries[0].profile->encode()!=before.entries[0].profile->encode())
            throw std::runtime_error("expired map transition partially committed");
    }
    if(endpoint.selectMap({{},character_identity,148,55}).status!=FW_CONFLICT||
       endpoint.selectMap({account_identity,character_identity,55,148}).status!=FW_CONFLICT||
       endpoint.selectMap({account_identity,character_identity,148,55}).status!=FW_OK||
       endpoint.selectMap({account_identity,character_identity,148,55}).status!=FW_CONFLICT)
        throw std::runtime_error("map transition lost ownership or compare-and-swap");
    const auto transitioned=endpoint.characterRoster({account_identity,0});
    if(transitioned.status!=FW_OK||transitioned.entries.size()!=1||transitioned.entries[0].map!=55||
       !transitioned.entries[0].profile||transitioned.entries[0].profile->last_map_id!=55)
        throw std::runtime_error("map transition did not persist profile map atomically");
    if(endpoint.deleteCharacter({{},gwserver::Identity::from_hex(character)}).status!=FW_NOT_FOUND||
       endpoint.characterProgression({gwserver::Identity::from_hex(account),gwserver::Identity::from_hex(character)}).status!=FW_OK)
        throw std::runtime_error("typed deletion bypassed account ownership");
    if(endpoint.deleteCharacter({gwserver::Identity::from_hex(account),gwserver::Identity::from_hex(character)}).status!=FW_OK||endpoint.characterProgression({gwserver::Identity::from_hex(account),gwserver::Identity::from_hex(character)}).status!=FW_NOT_FOUND)
        throw std::runtime_error("shared deletion failed to remove owned character");
    if(endpoint.deleteCharacter({gwserver::Identity::from_hex(account),gwserver::Identity::from_hex(character)}).status!=FW_NOT_FOUND)
        throw std::runtime_error("typed deletion replay succeeded");
    if(endpoint.guildCape(gwserver::Identity::from_hex(std::string(32,'0'))).status!=FW_NOT_FOUND)
        throw std::runtime_error("shared guild API bypassed account ownership validation");
    gwserver::ServerSettings settings;
    std::istringstream configured("bind=127.0.0.2\npublic_host=127.0.0.2\nauth_port=26112\nportal_port=26601\n");
    gwserver::load_endpoints(configured,settings);
    {std::istringstream encounters("quest_encounters=data/gw/quests/encounters.fwqe\nquest_givers=data/gw/quests/givers.txt\nquest_waves=data/gw/quests/experiment-waves.txt\nquest_giver_combat=data/gw/quests/aziure-development-combat.txt\n");gwserver::load_endpoints(encounters,settings);
     if(settings.quest_giver_combat!=std::optional<std::filesystem::path>{"data/gw/quests/aziure-development-combat.txt"})throw std::runtime_error("quest giver combat setting lost path");
     if(settings.quest_waves!=std::optional<std::filesystem::path>{"data/gw/quests/experiment-waves.txt"})throw std::runtime_error("quest wave setting lost path");
     if(settings.quest_encounters!=std::optional<std::filesystem::path>{"data/gw/quests/encounters.fwqe"})throw std::runtime_error("quest encounter setting lost path");
     if(settings.quest_givers!=std::optional<std::filesystem::path>{"data/gw/quests/givers.txt"})throw std::runtime_error("quest giver setting lost path");
     std::istringstream disabled("quest_encounters=none\nquest_givers=none\nquest_waves=none\nquest_giver_combat=none\n");gwserver::load_endpoints(disabled,settings);
     if(settings.quest_encounters||settings.quest_givers||settings.quest_waves||settings.quest_giver_combat)throw std::runtime_error("quest encounter/giver/wave disable failed");}
    {
        settings.item_catalogs.consumables="inherited-missing-catalog.txt";
        std::istringstream item_config("identification_catalog=data/items/identify.txt\nsalvage_catalog=data/Reviewed Items/salvage.txt\nconsumable_catalog=none\n");
        gwserver::load_endpoints(item_config,settings);
        if(settings.item_catalogs.identification!=std::optional<std::filesystem::path>{"data/items/identify.txt"}||
           settings.item_catalogs.salvage!=std::optional<std::filesystem::path>{"data/Reviewed Items/salvage.txt"}||settings.item_catalogs.consumables)
            throw std::runtime_error("explicit item catalogue settings lost paths or disable policy");
        const auto before=settings.item_catalogs;
        for(const auto& text:{std::string("salvage_catalog=new.txt\nsalvage_catalog=other.txt"),
                             std::string("identification_catalog=new.txt\nunknown=value"),
                             std::string("consumable_catalog=")+std::string(1025,'x')}){
            std::istringstream invalid(text);bool rejected=false;
            try{gwserver::load_endpoints(invalid,settings);}catch(const std::invalid_argument&){rejected=true;}
            if(!rejected||settings.item_catalogs.identification!=before.identification||
               settings.item_catalogs.salvage!=before.salvage||settings.item_catalogs.consumables!=before.consumables)
                throw std::runtime_error("failed config partially changed catalogue settings");
        }
        std::istringstream disabled("identification_catalog=none\nsalvage_catalog=none\n");
        gwserver::load_endpoints(disabled,settings);
        if(settings.item_catalogs.identification||settings.item_catalogs.salvage||settings.item_catalogs.consumables)
            throw std::runtime_error("explicit catalogue disable failed");
    }
    for(const char* invalid:{"auth_port=65535", "portal_port=26113", "bind=127.0.0.1\nbind=0.0.0.0", "public_host=0.0.0.0", "auth_port=26112junk", "unrecognized=value"}){
        const auto before=settings;std::istringstream config(invalid);bool rejected=false;
        try{gwserver::load_endpoints(config,settings);}catch(const std::invalid_argument&){rejected=true;}
        if(!rejected||settings.bind!=before.bind||settings.public_host!=before.public_host||settings.auth_port!=before.auth_port||settings.portal_port!=before.portal_port)throw std::runtime_error("invalid endpoint config changed active settings");
    }
    settings.auth_port=26112;settings.portal_port=26601;
    // Listener lifecycle is independent of optional world content. NPC
    // placement validation requires the matching navigation catalog.
    settings.navigation.clear();settings.npcs.clear();settings.profile.clear();settings.portals.clear();
    auto rejects=[&](const gwserver::ServerSettings& invalid){
        bool rejected=false;
        try{server.start(invalid);}catch(const std::exception&){rejected=true;}
        if(!rejected||server.healthy())throw std::runtime_error("failed startup reported healthy");
    };
    for(unsigned port:{0u,65535u,65536u}){
        auto invalid=settings;invalid.auth_port=port;rejects(invalid);
    }
    for(unsigned port:{0u,65536u,settings.auth_port,settings.auth_port+1}){
        auto invalid=settings;invalid.portal_port=port;rejects(invalid);
    }
    {
        fw::tcp::Runtime sockets;
        fw::tcp::Listener occupied(settings.bind,static_cast<std::uint16_t>(settings.auth_port));
        rejects(settings);
        // The other listener must be gone after partial startup failure.
        fw::tcp::Listener released(settings.bind,static_cast<std::uint16_t>(settings.portal_port));
    }
    for(unsigned attempt=0;attempt<2;++attempt){
        server.start(settings);
        if(!server.healthy())throw std::runtime_error("listeners not healthy after restart");
        bool duplicate_rejected=false;
        try{server.start(settings);}catch(const std::exception&){duplicate_rejected=true;}
        if(!duplicate_rejected||!server.healthy())throw std::runtime_error("duplicate start damaged live server");
        {
            fw::tcp::Runtime sockets;
            for(unsigned port:{settings.auth_port,settings.auth_port+1,settings.portal_port}){
                auto connection=fw::tcp::connect(settings.bind,static_cast<std::uint16_t>(port),1000);
                if(!connection)throw std::runtime_error("listener not accepting connections");
            }
        }
        server.stop();server.stop();
        if(server.healthy())throw std::runtime_error("stopped server reported healthy");
        fw::tcp::Runtime sockets;
        fw::tcp::Listener auth(settings.bind,static_cast<std::uint16_t>(settings.auth_port));
        fw::tcp::Listener world(settings.bind,static_cast<std::uint16_t>(settings.auth_port+1));
        fw::tcp::Listener portal(settings.bind,static_cast<std::uint16_t>(settings.portal_port));
    }
    std::cout<<"PASS listener startup rollback, connectivity and restart\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

