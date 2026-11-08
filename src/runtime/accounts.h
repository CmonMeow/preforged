#pragma once
#include <cstdint>
#include "operations.h"
#include <functional>
#include <memory>
#include <string>
namespace fw::gw::guild::cape {struct Design;}
namespace fw::gw::auth {struct AccountInfoProfile;}
namespace fw::gw::quests {struct Profile;}
namespace gwserver {struct RewardQuestExperience;}
namespace fw::gw::inventory {struct Profile;}
namespace fw::gw::progression {struct Character;struct Factions;}
namespace fw::gw::skills {struct Profile;}
namespace fw::gw::pets {struct Profile;}
namespace gwserver {struct AdoptPet;}
namespace fw::gw::attributes {struct Profile;}
namespace gwserver {struct CampaignsResult;struct CreateCharacter;struct CreationResult;struct RenameCharacter;struct SaveEquipmentVisibility;struct DevelopmentUnlockResult;struct SelectMap;struct MapTransitionResult;struct CharacterRoster;struct RosterResult;struct GameSessionLease;struct ValidateSession;struct LogoutAccount;struct IssueTicket;struct AuthenticateTicket;struct RegisterAccount;struct GuildCommand;struct GuildCommandResult;struct LoginAccount;struct LoginResult;struct TicketAuthenticationResult;struct TicketIssuanceResult;struct SessionResult;struct LogoutResult;template<class>struct CharacterStateResult;using CharacterNameResult=CharacterStateResult<std::string>;using CharacterNumberResult=CharacterStateResult<std::uint32_t>;struct DeleteCharacter;struct DeletionResult;struct ExchangeTrade;struct TradeResult;struct OwnedCharacter;struct SaveQuests;struct SaveProgression;struct SaveFactions;struct SaveInventory;struct TransferStorage;struct StorageTransferResult;struct AwardFaction;struct ExchangeImperialFaction;struct Identity;struct SaveSkillbar;struct SaveAttributes;struct SaveSecondaryProfession;struct ProfessionSaveResult;struct RevisionResult;template<class>struct ProfileResult;}
namespace gwserver {struct FriendTarget;struct AddFriend;struct FriendAddition;struct FriendsResult;using FriendAddResult=CharacterStateResult<FriendAddition>;struct FriendRemoveResult;using FriendBlockedResult=CharacterStateResult<bool>;}
namespace gwserver {struct PurchaseGuildCape;struct PurchaseGuildTrim;struct CapePurchaseResult;struct GuildState;using GuildResult=CharacterStateResult<GuildState>;struct GuildFaction;using GuildFactionResult=CharacterStateResult<GuildFaction>;using CapeResult=CharacterStateResult<fw::gw::guild::cape::Design>;using TournamentPointsResult=CharacterStateResult<std::uint32_t>;}
namespace fw {
struct AccountResult { uint32_t status; std::string payload; };
// Trusted internal API. NetworkManager serializes access to this connection;
// public protocol handlers authenticate and validate requests before dispatch.
class Accounts {
    struct Impl;
    std::unique_ptr<Impl> impl_;
public:
    gwserver::GuildCommandResult guildCommand(const gwserver::GuildCommand&,const std::function<bool()>&);
    gwserver::LoginResult login(const gwserver::LoginAccount&,const std::function<bool()>&);
    gwserver::LoginResult registerAccount(const gwserver::RegisterAccount&,const std::function<bool()>&);
    gwserver::TicketAuthenticationResult authenticateTicket(const gwserver::AuthenticateTicket&,const std::function<bool()>&);
    gwserver::TicketIssuanceResult issueTicket(const gwserver::IssueTicket&,const std::function<bool()>&);
    gwserver::LogoutResult logout(const gwserver::LogoutAccount&,const std::function<bool()>&);
    gwserver::SessionResult validateSession(const gwserver::ValidateSession&,const std::function<bool()>&);
    gwserver::CharacterNumberResult equipmentVisibility(const gwserver::OwnedCharacter&,const std::function<bool()>&);
    gwserver::CharacterNumberResult saveEquipmentVisibility(const gwserver::SaveEquipmentVisibility&,const std::function<bool()>&);
    gwserver::CapePurchaseResult purchaseGuildCape(const gwserver::PurchaseGuildCape&,const std::function<bool()>&);
    gwserver::TournamentPointsResult purchaseGuildTrim(const gwserver::PurchaseGuildTrim&,const std::function<bool()>&);
    gwserver::GuildResult guild(const gwserver::Identity&,const std::function<bool()>&);
    gwserver::CapeResult guildCape(const gwserver::Identity&,const std::function<bool()>&);
    gwserver::GuildFactionResult guildFaction(const gwserver::Identity&,const std::function<bool()>&);
    gwserver::TournamentPointsResult tournamentPoints(const gwserver::Identity&,const std::function<bool()>&);
    gwserver::FriendsResult friends(const gwserver::Identity&,const std::function<bool()>&);
    gwserver::FriendAddResult addFriend(const gwserver::AddFriend&,const std::function<bool()>&);
    gwserver::FriendRemoveResult removeFriend(const gwserver::FriendTarget&,const std::function<bool()>&);
    gwserver::FriendBlockedResult friendBlocked(const gwserver::FriendTarget&,const std::function<bool()>&);
    gwserver::CharacterStateResult<bool> accountAgreement(const gwserver::Identity& account,const std::function<bool()>& permitted);
    gwserver::CharacterStateResult<gw::auth::AccountInfoProfile> accountProfile(const gwserver::Identity& account,const std::function<bool()>& permitted);
    gwserver::CharacterStateResult<std::string> gameAccountAlias(const gwserver::Identity& account,const std::function<bool()>& permitted);
    gwserver::CampaignsResult accountCampaigns(const gwserver::Identity& account,const std::function<bool()>& permitted);
    gwserver::CreationResult createCharacter(const gwserver::CreateCharacter& request,const std::function<bool()>& permitted);
    gwserver::CharacterNameResult renameCharacter(const gwserver::RenameCharacter& request,const std::function<bool()>& permitted);
    gwserver::DevelopmentUnlockResult unlockDevelopmentCharacter(const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted);
    explicit Accounts(const std::string& database);
    ~Accounts();
    gwserver::ProfileResult<gw::quests::Profile> characterQuests(const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted);
    gwserver::ProfileResult<gw::pets::Profile> characterPets(const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted);
    gwserver::RevisionResult adoptPet(const gwserver::AdoptPet& request,const std::function<bool()>& permitted);
    gwserver::RevisionResult saveQuests(const gwserver::SaveQuests& request,const std::function<bool()>& permitted);
    gwserver::RevisionResult rewardQuestExperience(const gwserver::RewardQuestExperience& request,const std::function<bool()>& permitted);
    gwserver::ProfileResult<gw::skills::Profile> characterSkills(const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted);
    gwserver::RevisionResult saveSkillbar(const gwserver::SaveSkillbar& request,const std::function<bool()>& permitted);
    gwserver::ProfileResult<gw::attributes::Profile> characterAttributes(const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted);
    gwserver::RevisionResult saveAttributes(const gwserver::SaveAttributes& request,const std::function<bool()>& permitted);
    gwserver::ProfessionSaveResult saveSecondaryProfession(const gwserver::SaveSecondaryProfession& request,const std::function<bool()>& permitted);
    gwserver::ProfileResult<gw::progression::Character> characterProgression(const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted);
    gwserver::RevisionResult saveProgression(const gwserver::SaveProgression& request,const std::function<bool()>& permitted);
    gwserver::ProfileResult<gw::progression::Factions> accountFactions(const gwserver::Identity& owner,const std::function<bool()>& permitted);
    gwserver::RevisionResult saveFactions(const gwserver::SaveFactions& request,const std::function<bool()>& permitted);
    gwserver::RevisionResult awardFaction(const gwserver::AwardFaction& request,const std::function<bool()>& permitted);
    gwserver::RevisionResult exchangeImperialFaction(const gwserver::ExchangeImperialFaction& request,const std::function<bool()>& permitted);
    gwserver::ProfileResult<gw::inventory::Profile> characterInventory(const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted);
    gwserver::ProfileResult<gw::inventory::Profile> accountStorage(const gwserver::Identity& account,const std::function<bool()>& permitted);
    gwserver::RevisionResult saveInventory(const gwserver::SaveInventory& request,const std::function<bool()>& permitted);
    gwserver::StorageTransferResult transferStorage(const gwserver::TransferStorage& request,const std::function<bool()>& permitted);
    gwserver::DeletionResult deleteCharacter(const gwserver::DeleteCharacter& request,const std::function<bool()>& permitted);
    gwserver::CharacterNumberResult characterNumber(const gwserver::OwnedCharacter& owner,bool map,const std::function<bool()>& permitted);
    gwserver::CharacterNameResult characterName(const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted);
    gwserver::TradeResult exchangeTrade(const gwserver::ExchangeTrade& request,const std::function<bool()>& permitted);
    gwserver::MapTransitionResult selectMap(const gwserver::SelectMap& request,const std::function<bool()>& permitted);
    gwserver::RosterResult characterRoster(const gwserver::CharacterRoster& request,const std::function<bool()>& permitted);
    gwserver::LogoutResult releaseGameSession(const gwserver::GameSessionLease& lease,const std::function<bool()>& permitted);
    AccountResult execute(gwserver::Operation operation, const std::string& payload,
                          const std::function<bool()>& permitted);
    // Existing offline tools resolve names here, never in production dispatch.
    AccountResult execute(const std::string& operation, const std::string& payload,
                          const std::function<bool()>& permitted);
};
}

