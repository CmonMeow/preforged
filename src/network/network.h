#pragma once
#include "accounts.h"
#include "operations.h"
#include "character_requests.h"
#include "character_results.h"
#include "ticket_auth.h"
#include "account_sessions.h"
#include "skill_state.h"
#include "inventory_state.h"
#include "trade_requests.h"
#include "character_state.h"
#include "account_policy.h"
#include "friend_state.h"
#include "progression_state.h"
#include "quest_state.h"
#include "pet_state.h"
#include "cape_state.h"
#include "guild_commands.h"
#include "guild_state.h"
#include "gw_item_catalogs.h"
#include "gw_quest_catalog.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
namespace gwserver {
class NetworkManager;
class NetworkComponent;
struct ServerSettings {
    std::optional<std::filesystem::path> quest_catalog;
    std::optional<std::filesystem::path> quest_encounters;
    std::optional<std::filesystem::path> quest_waves;
    std::optional<std::filesystem::path> quest_corpses;
    std::optional<std::filesystem::path> quest_escorts;
    std::optional<std::filesystem::path> quest_givers;
    std::optional<std::filesystem::path> quest_giver_combat;
    fw::gw::items::CatalogPaths item_catalogs=fw::gw::items::CatalogPaths::environment();
    std::string portals="config/map-portals.txt";
    std::string database = "data/state.db";
    std::string key = "data/gw/transport-key-a6fb3c51.bin";
    std::string navigation = "data/gw/navigation/catalog.txt";
    std::string npcs = "data/gw/npcs.txt";
    std::string npc_combat = "data/gw/npc-combat.txt";
    std::string profile = "data/gw-official-import/main/account-profile.bin";
    std::string bind = "127.0.0.1", public_host = "127.0.0.1";
    unsigned auth_port = 6112, portal_port = 6601, build = 37130;
};
// Shared application operations dispatch through the concrete component.
class NetworkComponent {
protected:
    NetworkManager& manager_;
    explicit NetworkComponent(NetworkManager& manager) : manager_(manager) {}
public:
    virtual ~NetworkComponent() = default;
    virtual LoginResult registerAccount(const RegisterAccount& request) = 0;
    virtual SessionResult validateSession(const ValidateSession& request) = 0;
    virtual TicketIssuanceResult issueTicket(const IssueTicket& request) = 0;
    virtual GameAliasResult gameAccountAlias(const Identity& account) = 0;
    virtual LoginResult login(const LoginAccount& request) = 0;
    virtual LogoutResult logout(const LogoutAccount& request) = 0;
    virtual LogoutResult releaseGameSession(const GameSessionLease& lease) = 0;
    virtual CreationResult createCharacter(const CreateCharacter& request) = 0;
    virtual DeletionResult deleteCharacter(const DeleteCharacter& request) = 0;
    virtual RosterResult characterRoster(const CharacterRoster& request) = 0;
    virtual MapTransitionResult selectMap(const SelectMap& request) = 0;
    virtual TicketAuthenticationResult authenticateTicket(const AuthenticateTicket& request) = 0;
    virtual AccountProfileResult accountProfile(const Identity& account) = 0;
    virtual CampaignsResult accountCampaigns(const Identity& account) = 0;
    virtual AgreementResult accountAgreement(const Identity& account) = 0;
    virtual CharacterNumberResult characterExperience(const OwnedCharacter& request) = 0;
    virtual CharacterNumberResult characterMap(const OwnedCharacter& request) = 0;
    virtual CharacterNameResult characterName(const OwnedCharacter& request) = 0;
    virtual CharacterNumberResult equipmentVisibility(const OwnedCharacter& owner) = 0;
    virtual CharacterNumberResult saveEquipmentVisibility(const SaveEquipmentVisibility& request) = 0;
    virtual CharacterNameResult renameCharacter(const RenameCharacter& request) = 0;
    virtual DevelopmentUnlockResult unlockDevelopmentCharacter(const OwnedCharacter& request) = 0;
    virtual SkillsResult characterSkills(const OwnedCharacter& request) = 0;
    virtual ProfessionSaveResult saveSecondaryProfession(const SaveSecondaryProfession& request) = 0;
    virtual RevisionResult saveSkillbar(const SaveSkillbar& request) = 0;
    virtual AttributesResult characterAttributes(const OwnedCharacter& request) = 0;
    virtual RevisionResult saveAttributes(const SaveAttributes& request) = 0;
    virtual InventoryResult characterInventory(const OwnedCharacter& request) = 0;
    virtual RevisionResult saveInventory(const SaveInventory& request) = 0;
    virtual TradeResult exchangeTrade(const ExchangeTrade& request) = 0;
    virtual InventoryResult accountStorage(const Identity& account) = 0;
    virtual StorageTransferResult transferStorage(const TransferStorage& request) = 0;
    virtual FriendsResult friends(const Identity& account) = 0;
    virtual FriendAddResult addFriend(const AddFriend& request) = 0;
    virtual FriendRemoveResult removeFriend(const FriendTarget& request) = 0;
    virtual FriendBlockedResult friendBlocked(const FriendTarget& request) = 0;
    virtual GuildResult guild(const Identity& account) = 0;
    virtual GuildCommandResult guildCommand(const GuildCommand& request) = 0;
    virtual FactionsResult accountFactions(const Identity& account) = 0;
    virtual RevisionResult saveFactions(const SaveFactions& request) = 0;
    virtual RevisionResult awardFaction(const AwardFaction& request) = 0;
    virtual RevisionResult exchangeImperialFaction(const ExchangeImperialFaction& request) = 0;
    virtual ProgressionResult characterProgression(const OwnedCharacter& request) = 0;
    virtual RevisionResult saveProgression(const SaveProgression& request) = 0;
    virtual QuestsResult characterQuests(const OwnedCharacter& request) = 0;
    virtual PetsResult characterPets(const OwnedCharacter&) {return {FW_UNAVAILABLE,"Pet storage unavailable",{}};}
    virtual RevisionResult adoptPet(const AdoptPet&) {return {FW_UNAVAILABLE,"Pet storage unavailable"};}
    virtual RevisionResult saveQuests(const SaveQuests& request) = 0;
    virtual RevisionResult rewardQuestExperience(const RewardQuestExperience&) {return {FW_UNAVAILABLE,"Quest reward storage unavailable"};}
    virtual GuildFactionResult guildFaction(const Identity& account) = 0;
    virtual CapeResult guildCape(const Identity& account) = 0;
    virtual CapePurchaseResult purchaseGuildCape(const PurchaseGuildCape& request) = 0;
    virtual TournamentPointsResult tournamentPoints(const Identity& account) = 0;
    virtual TournamentPointsResult purchaseGuildTrim(const PurchaseGuildTrim& request) = 0;

};
// Native listeners and application execution stay inside this server process.
class NetworkServer : public NetworkComponent {
    std::thread auth_, portal_;
    std::atomic<bool> auth_finished_{true}, portal_finished_{true};
public:
    explicit NetworkServer(NetworkManager& manager) : NetworkComponent(manager) {}
    ~NetworkServer() override;
    LoginResult registerAccount(const RegisterAccount& request) override;
    SessionResult validateSession(const ValidateSession& request) override;
    TicketIssuanceResult issueTicket(const IssueTicket& request) override;
    GameAliasResult gameAccountAlias(const Identity& account) override;
    LoginResult login(const LoginAccount& request) override;
    LogoutResult logout(const LogoutAccount& request) override;
    LogoutResult releaseGameSession(const GameSessionLease& lease) override;
    CreationResult createCharacter(const CreateCharacter& request) override;
    DeletionResult deleteCharacter(const DeleteCharacter& request) override;
    RosterResult characterRoster(const CharacterRoster& request) override;
    MapTransitionResult selectMap(const SelectMap& request) override;
    TicketAuthenticationResult authenticateTicket(const AuthenticateTicket& request) override;
    AccountProfileResult accountProfile(const Identity& account) override;
    CampaignsResult accountCampaigns(const Identity& account) override;
    AgreementResult accountAgreement(const Identity& account) override;
    CharacterNumberResult characterExperience(const OwnedCharacter& request) override;
    CharacterNumberResult characterMap(const OwnedCharacter& request) override;
    CharacterNameResult characterName(const OwnedCharacter& request) override;
    CharacterNumberResult equipmentVisibility(const OwnedCharacter& owner) override;
    CharacterNumberResult saveEquipmentVisibility(const SaveEquipmentVisibility& request) override;
    CharacterNameResult renameCharacter(const RenameCharacter& request) override;
    DevelopmentUnlockResult unlockDevelopmentCharacter(const OwnedCharacter& request) override;
    SkillsResult characterSkills(const OwnedCharacter& request) override;
    ProfessionSaveResult saveSecondaryProfession(const SaveSecondaryProfession& request) override;
    RevisionResult saveSkillbar(const SaveSkillbar& request) override;
    AttributesResult characterAttributes(const OwnedCharacter& request) override;
    RevisionResult saveAttributes(const SaveAttributes& request) override;
    InventoryResult characterInventory(const OwnedCharacter& request) override;
    RevisionResult saveInventory(const SaveInventory& request) override;
    TradeResult exchangeTrade(const ExchangeTrade& request) override;
    InventoryResult accountStorage(const Identity& account) override;
    StorageTransferResult transferStorage(const TransferStorage& request) override;
    FriendsResult friends(const Identity& account) override;
    FriendAddResult addFriend(const AddFriend& request) override;
    FriendRemoveResult removeFriend(const FriendTarget& request) override;
    FriendBlockedResult friendBlocked(const FriendTarget& request) override;
    GuildResult guild(const Identity& account) override;
    GuildCommandResult guildCommand(const GuildCommand& request) override;
    FactionsResult accountFactions(const Identity& account) override;
    RevisionResult saveFactions(const SaveFactions& request) override;
    RevisionResult awardFaction(const AwardFaction& request) override;
    RevisionResult exchangeImperialFaction(const ExchangeImperialFaction& request) override;
    ProgressionResult characterProgression(const OwnedCharacter& request) override;
    RevisionResult saveProgression(const SaveProgression& request) override;
    QuestsResult characterQuests(const OwnedCharacter& request) override;
    PetsResult characterPets(const OwnedCharacter& owner) override;
    RevisionResult adoptPet(const AdoptPet& request) override;
    RevisionResult saveQuests(const SaveQuests& request) override;
    RevisionResult rewardQuestExperience(const RewardQuestExperience& request) override;
    GuildFactionResult guildFaction(const Identity& account) override;
    CapeResult guildCape(const Identity& account) override;
    CapePurchaseResult purchaseGuildCape(const PurchaseGuildCape& request) override;
    TournamentPointsResult tournamentPoints(const Identity& account) override;
    TournamentPointsResult purchaseGuildTrim(const PurchaseGuildTrim& request) override;
public:
    void start(const ServerSettings& settings);
    void stop();
    bool healthy() const;
};
// Future custom clients supply transport; unavailable is never local success.
class NetworkClient : public NetworkComponent {
public:
    explicit NetworkClient(NetworkManager& manager) : NetworkComponent(manager) {}
    LoginResult registerAccount(const RegisterAccount& request) override;
    SessionResult validateSession(const ValidateSession& request) override;
    TicketIssuanceResult issueTicket(const IssueTicket& request) override;
    GameAliasResult gameAccountAlias(const Identity& account) override;
    LoginResult login(const LoginAccount& request) override;
    LogoutResult logout(const LogoutAccount& request) override;
    LogoutResult releaseGameSession(const GameSessionLease& lease) override;
    CreationResult createCharacter(const CreateCharacter& request) override;
    DeletionResult deleteCharacter(const DeleteCharacter& request) override;
    RosterResult characterRoster(const CharacterRoster& request) override;
    MapTransitionResult selectMap(const SelectMap& request) override;
    TicketAuthenticationResult authenticateTicket(const AuthenticateTicket& request) override;
    AccountProfileResult accountProfile(const Identity& account) override;
    CampaignsResult accountCampaigns(const Identity& account) override;
    AgreementResult accountAgreement(const Identity& account) override;
    CharacterNumberResult characterExperience(const OwnedCharacter& request) override;
    CharacterNumberResult characterMap(const OwnedCharacter& request) override;
    CharacterNameResult characterName(const OwnedCharacter& request) override;
    CharacterNumberResult equipmentVisibility(const OwnedCharacter& owner) override;
    CharacterNumberResult saveEquipmentVisibility(const SaveEquipmentVisibility& request) override;
    CharacterNameResult renameCharacter(const RenameCharacter& request) override;
    DevelopmentUnlockResult unlockDevelopmentCharacter(const OwnedCharacter& request) override;
    SkillsResult characterSkills(const OwnedCharacter& request) override;
    ProfessionSaveResult saveSecondaryProfession(const SaveSecondaryProfession& request) override;
    RevisionResult saveSkillbar(const SaveSkillbar& request) override;
    AttributesResult characterAttributes(const OwnedCharacter& request) override;
    RevisionResult saveAttributes(const SaveAttributes& request) override;
    InventoryResult characterInventory(const OwnedCharacter& request) override;
    RevisionResult saveInventory(const SaveInventory& request) override;
    TradeResult exchangeTrade(const ExchangeTrade& request) override;
    InventoryResult accountStorage(const Identity& account) override;
    StorageTransferResult transferStorage(const TransferStorage& request) override;
    FriendsResult friends(const Identity& account) override;
    FriendAddResult addFriend(const AddFriend& request) override;
    FriendRemoveResult removeFriend(const FriendTarget& request) override;
    FriendBlockedResult friendBlocked(const FriendTarget& request) override;
    GuildResult guild(const Identity& account) override;
    GuildCommandResult guildCommand(const GuildCommand& request) override;
    FactionsResult accountFactions(const Identity& account) override;
    RevisionResult saveFactions(const SaveFactions& request) override;
    RevisionResult awardFaction(const AwardFaction& request) override;
    RevisionResult exchangeImperialFaction(const ExchangeImperialFaction& request) override;
    ProgressionResult characterProgression(const OwnedCharacter& request) override;
    RevisionResult saveProgression(const SaveProgression& request) override;
    QuestsResult characterQuests(const OwnedCharacter& request) override;
    PetsResult characterPets(const OwnedCharacter& owner) override;
    RevisionResult adoptPet(const AdoptPet& request) override;
    RevisionResult saveQuests(const SaveQuests& request) override;
    RevisionResult rewardQuestExperience(const RewardQuestExperience& request) override;
    GuildFactionResult guildFaction(const Identity& account) override;
    CapeResult guildCape(const Identity& account) override;
    CapePurchaseResult purchaseGuildCape(const PurchaseGuildCape& request) override;
    TournamentPointsResult tournamentPoints(const Identity& account) override;
    TournamentPointsResult purchaseGuildTrim(const PurchaseGuildTrim& request) override;
};
class NetworkManager {
    friend class NetworkServer;
    friend struct OfflineStorageAdapter;
    fw::AccountResult execute(Operation operation,const std::string& payload,unsigned timeout_ms = 3000);
    ProfessionSaveResult saveSecondaryProfession(const SaveSecondaryProfession& request);
    AttributesResult characterAttributes(const OwnedCharacter& owner);
    RevisionResult saveAttributes(const SaveAttributes& request);
    SkillsResult characterSkills(const OwnedCharacter& owner);
    RevisionResult saveSkillbar(const SaveSkillbar& request);
    ProgressionResult characterProgression(const OwnedCharacter& owner);
    RevisionResult saveProgression(const SaveProgression& request);
    FactionsResult accountFactions(const Identity& owner);
    RevisionResult saveFactions(const SaveFactions& request);
    RevisionResult awardFaction(const AwardFaction& request);
    RevisionResult exchangeImperialFaction(const ExchangeImperialFaction& request);
    MapTransitionResult selectMap(const SelectMap& request);
    CharacterNameResult renameCharacter(const RenameCharacter& request);
    DevelopmentUnlockResult unlockDevelopmentCharacter(const OwnedCharacter& request);
    CreationResult createCharacter(const CreateCharacter& request);
    AccountProfileResult accountProfile(const Identity& account);
    GameAliasResult gameAccountAlias(const Identity& account);
    CampaignsResult accountCampaigns(const Identity& account);
    GuildCommandResult guildCommand(const GuildCommand& request);
    LoginResult login(const LoginAccount& request);
    LoginResult registerAccount(const RegisterAccount& request);
    TicketAuthenticationResult authenticateTicket(const AuthenticateTicket& request);
    TicketIssuanceResult issueTicket(const IssueTicket& request);
    LogoutResult logout(const LogoutAccount& request);
    SessionResult validateSession(const ValidateSession& request);
    AgreementResult accountAgreement(const Identity& account);
    RosterResult characterRoster(const CharacterRoster& request);
    LogoutResult releaseGameSession(const GameSessionLease& lease);
    DeletionResult deleteCharacter(const DeleteCharacter& request);
    CharacterNumberResult characterNumber(const OwnedCharacter& request,bool map);
    CharacterNameResult characterName(const OwnedCharacter& request);
    TradeResult exchangeTrade(const ExchangeTrade& request);
    RevisionResult saveInventory(const SaveInventory& request);
    StorageTransferResult transferStorage(const TransferStorage& request);
    InventoryResult characterInventory(const OwnedCharacter& owner);
    InventoryResult accountStorage(const Identity& owner);
    QuestsResult characterQuests(const OwnedCharacter& owner);
    PetsResult characterPets(const OwnedCharacter& owner);
    RevisionResult adoptPet(const AdoptPet& request);
    RevisionResult saveQuests(const SaveQuests& request);
    RevisionResult rewardQuestExperience(const RewardQuestExperience& request);
    FriendRemoveResult removeFriend(const FriendTarget& request);
    FriendBlockedResult friendBlocked(const FriendTarget& request);
    FriendsResult friends(const Identity& owner);
    FriendAddResult addFriend(const AddFriend& request);
    CapeResult guildCape(const Identity& owner);
    GuildFactionResult guildFaction(const Identity& owner);
    TournamentPointsResult tournamentPoints(const Identity& owner);
    GuildResult guild(const Identity& owner);
    CapePurchaseResult purchaseGuildCape(const PurchaseGuildCape& request);
    TournamentPointsResult purchaseGuildTrim(const PurchaseGuildTrim& request);
    CharacterNumberResult equipmentVisibility(const OwnedCharacter& owner);
    CharacterNumberResult saveEquipmentVisibility(const SaveEquipmentVisibility& request);
    const fw::gw::items::Catalogs item_catalogs_;
    const fw::gw::quests::Catalog quest_catalog_;
    fw::Accounts accounts_;
    std::timed_mutex database_mutex_;
    NetworkServer server_;
    NetworkClient client_;
    static std::atomic<NetworkManager*> current_;
public:
    explicit NetworkManager(const std::string& database, fw::gw::items::CatalogPaths paths=fw::gw::items::CatalogPaths::environment(),std::optional<std::filesystem::path> quests={});
    ~NetworkManager();
    NetworkManager(const NetworkManager&) = delete;
    static NetworkManager& current();
    const fw::gw::items::Catalogs& item_catalogs()const{return item_catalogs_;}
    const fw::gw::quests::Catalog& quest_catalog()const{return quest_catalog_;}
    NetworkServer& server() { return server_; }
    NetworkClient& client() { return client_; }

};
}
