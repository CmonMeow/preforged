#include "network.h"
#include "gw_listeners.h"
#include "common.h"
#include "transport.h"
#include <chrono>
#include <sodium.h>
namespace gwserver {
std::atomic<NetworkManager*> NetworkManager::current_{nullptr};
NetworkManager::NetworkManager(const std::string& database, fw::gw::items::CatalogPaths paths,std::optional<std::filesystem::path> quests)
    : item_catalogs_(std::move(paths)), quest_catalog_(fw::gw::quests::load_catalog(quests)),accounts_(database), server_(*this), client_(*this) {
    NetworkManager* expected = nullptr;
    if (!current_.compare_exchange_strong(expected, this)) throw std::runtime_error("Server manager already exists");
}
NetworkManager::~NetworkManager() { server_.stop(); current_ = nullptr; }
NetworkManager& NetworkManager::current() {
    auto* value = current_.load();
    if (!value) throw std::runtime_error("Server manager unavailable");
    return *value;
}
fw::AccountResult NetworkManager::execute(Operation operation,
                                         const std::string& payload, unsigned timeout_ms) {
    // Maintenance identities remain offline-only, outside the game contract.
    if(static_cast<unsigned>(operation)>static_cast<unsigned>(Operation::purchaseGuildTrim))
        return {FW_NOT_FOUND,"Unknown application operation"};
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(std::clamp(timeout_ms,1u,30000u));
    std::unique_lock lock(database_mutex_, std::defer_lock);
    if (!lock.try_lock_until(deadline)) return {FW_TIMEOUT,"Database admission expired"};
    return accounts_.execute(operation,payload,[deadline] { return std::chrono::steady_clock::now() < deadline; });
}
NetworkServer::~NetworkServer() { stop(); }
MapTransitionResult NetworkManager::selectMap(const SelectMap& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.selectMap(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
RosterResult NetworkManager::characterRoster(const CharacterRoster& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.characterRoster(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
LogoutResult NetworkManager::releaseGameSession(const GameSessionLease& lease){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.releaseGameSession(lease,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
LogoutResult NetworkServer::releaseGameSession(const GameSessionLease& lease){return manager_.releaseGameSession(lease);}
LogoutResult NetworkClient::releaseGameSession(const GameSessionLease&){return {FW_UNAVAILABLE,"Custom-client transport is not implemented"};}
PetsResult NetworkManager::characterPets(const OwnedCharacter& owner){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired",{}};
 return accounts_.characterPets(owner,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
RevisionResult NetworkManager::adoptPet(const AdoptPet& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.adoptPet(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
PetsResult NetworkServer::characterPets(const OwnedCharacter& owner){return manager_.characterPets(owner);}
RevisionResult NetworkServer::adoptPet(const AdoptPet& request){return manager_.adoptPet(request);}
PetsResult NetworkClient::characterPets(const OwnedCharacter&){return {FW_UNAVAILABLE,"Custom-client transport is not implemented",{}};}
RevisionResult NetworkClient::adoptPet(const AdoptPet&){return {FW_UNAVAILABLE,"Custom-client transport is not implemented"};}
QuestsResult NetworkManager::characterQuests(const OwnedCharacter& owner){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired",{}};
    return accounts_.characterQuests(owner,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
RevisionResult NetworkManager::rewardQuestExperience(const RewardQuestExperience& request){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
    return accounts_.rewardQuestExperience(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
RevisionResult NetworkManager::saveQuests(const SaveQuests& request){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
    return accounts_.saveQuests(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
FactionsResult NetworkManager::accountFactions(const Identity& owner){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired",{}};
    return accounts_.accountFactions(owner,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
RevisionResult NetworkManager::saveFactions(const SaveFactions& request){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
    return accounts_.saveFactions(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
ProgressionResult NetworkManager::characterProgression(const OwnedCharacter& owner){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired",{}};
    return accounts_.characterProgression(owner,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
RevisionResult NetworkManager::saveProgression(const SaveProgression& request){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
    return accounts_.saveProgression(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
SkillsResult NetworkManager::characterSkills(const OwnedCharacter& owner){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired",{}};
    return accounts_.characterSkills(owner,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
RevisionResult NetworkManager::saveSkillbar(const SaveSkillbar& request){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
    return accounts_.saveSkillbar(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
AttributesResult NetworkManager::characterAttributes(const OwnedCharacter& owner){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired",{}};
    return accounts_.characterAttributes(owner,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
RevisionResult NetworkManager::saveAttributes(const SaveAttributes& request){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
    return accounts_.saveAttributes(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
RevisionResult NetworkManager::awardFaction(const AwardFaction& request){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
    return accounts_.awardFaction(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
RevisionResult NetworkManager::exchangeImperialFaction(const ExchangeImperialFaction& request){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
    return accounts_.exchangeImperialFaction(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
InventoryResult NetworkManager::characterInventory(const OwnedCharacter& owner){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired",{}};
    return accounts_.characterInventory(owner,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
InventoryResult NetworkManager::accountStorage(const Identity& owner){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired",{}};
    return accounts_.accountStorage(owner,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
RevisionResult NetworkManager::saveInventory(const SaveInventory& request){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
    return accounts_.saveInventory(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
StorageTransferResult NetworkManager::transferStorage(const TransferStorage& request){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
    return accounts_.transferStorage(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
void NetworkServer::start(const ServerSettings& s) {
    if (auth_.joinable() || portal_.joinable()) throw std::runtime_error("Server already started");
    if(s.auth_port<1024||s.auth_port>=65535||s.portal_port<1024||s.portal_port>65535||
       s.portal_port==s.auth_port||s.portal_port==s.auth_port+1)
        throw std::invalid_argument("Auth, world and portal need distinct valid ports");
    fw_gw_auth_prepare(); fw_gw_portal_prepare(); auth_finished_=false; portal_finished_=false;
    try {
        portal_=std::thread([this,s] {
            fw_gw_portal_run(s); portal_finished_=true;
        });
        auth_=std::thread([this,s] {
            fw_gw_auth_run(s); auth_finished_=true;
        });
        auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        while (!auth_finished_ && !portal_finished_ && !(fw_gw_auth_ready() && fw_gw_portal_ready()) && std::chrono::steady_clock::now()<deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        if (!healthy()) throw std::runtime_error("Network listeners failed to start; see preceding diagnostic");
    } catch (...) { stop(); throw; }
}
void NetworkServer::stop() {
    fw_gw_auth_stop(); fw_gw_portal_stop();
    if(auth_.joinable()) auth_.join();
    if(portal_.joinable()) portal_.join();
}
bool NetworkServer::healthy() const {
    return !auth_finished_ && !portal_finished_ && fw_gw_auth_ready() && fw_gw_portal_ready();
}
LoginResult NetworkManager::registerAccount(const RegisterAccount& value){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.registerAccount(value,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
LoginResult NetworkServer::registerAccount(const RegisterAccount& value){return manager_.registerAccount(value);}
SessionResult NetworkManager::validateSession(const ValidateSession& value) {
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.validateSession(value,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
SessionResult NetworkServer::validateSession(const ValidateSession& value) {return manager_.validateSession(value);}
TicketIssuanceResult NetworkManager::issueTicket(const IssueTicket& value) {
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.issueTicket(value,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
TicketIssuanceResult NetworkServer::issueTicket(const IssueTicket& value) {return manager_.issueTicket(value);}
GameAliasResult NetworkManager::gameAccountAlias(const Identity& account){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired",{}};
    return accounts_.gameAccountAlias(account,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
GameAliasResult NetworkServer::gameAccountAlias(const Identity& account) { return manager_.gameAccountAlias(account); }
LoginResult NetworkManager::login(const LoginAccount& value){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.login(value,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
LoginResult NetworkServer::login(const LoginAccount& value){return manager_.login(value);}
LogoutResult NetworkManager::logout(const LogoutAccount& value) {
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.logout(value,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
LogoutResult NetworkServer::logout(const LogoutAccount& value) {return manager_.logout(value);}
CreationResult NetworkManager::createCharacter(const CreateCharacter& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired",{}};
 return accounts_.createCharacter(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
CreationResult NetworkServer::createCharacter(const CreateCharacter& value) { return manager_.createCharacter(value); }
DeletionResult NetworkManager::deleteCharacter(const DeleteCharacter& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.deleteCharacter(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
DeletionResult NetworkServer::deleteCharacter(const DeleteCharacter& value) { return manager_.deleteCharacter(value); }
RosterResult NetworkServer::characterRoster(const CharacterRoster& value) {
    return manager_.characterRoster(value);
}
MapTransitionResult NetworkServer::selectMap(const SelectMap& value) {
    return manager_.selectMap(value);
}
TicketAuthenticationResult NetworkManager::authenticateTicket(const AuthenticateTicket& value){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.authenticateTicket(value,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
TicketAuthenticationResult NetworkServer::authenticateTicket(const AuthenticateTicket& value){return manager_.authenticateTicket(value);}
AccountProfileResult NetworkManager::accountProfile(const Identity& account){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired",{}};
    return accounts_.accountProfile(account,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
AccountProfileResult NetworkServer::accountProfile(const Identity& account) { return manager_.accountProfile(account); }
CampaignsResult NetworkManager::accountCampaigns(const Identity& account){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired",{}};
    return accounts_.accountCampaigns(account,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
CampaignsResult NetworkServer::accountCampaigns(const Identity& account) { return manager_.accountCampaigns(account); }
AgreementResult NetworkManager::accountAgreement(const Identity& account){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired",{}};
    return accounts_.accountAgreement(account,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
AgreementResult NetworkServer::accountAgreement(const Identity& account) { return manager_.accountAgreement(account); }
CharacterNumberResult NetworkServer::characterExperience(const OwnedCharacter& value) { return manager_.characterNumber(value,false); }
CharacterNumberResult NetworkServer::characterMap(const OwnedCharacter& value) { return manager_.characterNumber(value,true); }
CharacterNumberResult NetworkManager::characterNumber(const OwnedCharacter& owner,bool map){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.characterNumber(owner,map,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
CharacterNameResult NetworkManager::characterName(const OwnedCharacter& owner){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.characterName(owner,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
CharacterNameResult NetworkServer::characterName(const OwnedCharacter& value) { return manager_.characterName(value); }
CharacterNameResult NetworkManager::renameCharacter(const RenameCharacter& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.renameCharacter(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
CharacterNameResult NetworkServer::renameCharacter(const RenameCharacter& request){return manager_.renameCharacter(request);}
DevelopmentUnlockResult NetworkManager::unlockDevelopmentCharacter(const OwnedCharacter& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.unlockDevelopmentCharacter(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
DevelopmentUnlockResult NetworkServer::unlockDevelopmentCharacter(const OwnedCharacter& request){return manager_.unlockDevelopmentCharacter(request);}
SkillsResult NetworkServer::characterSkills(const OwnedCharacter& value) { return manager_.characterSkills(value); }
ProfessionSaveResult NetworkManager::saveSecondaryProfession(const SaveSecondaryProfession& request){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    std::unique_lock lock(database_mutex_,std::defer_lock);
    if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
    return accounts_.saveSecondaryProfession(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
ProfessionSaveResult NetworkServer::saveSecondaryProfession(const SaveSecondaryProfession& value) { return manager_.saveSecondaryProfession(value); }
RevisionResult NetworkServer::saveSkillbar(const SaveSkillbar& value) { return manager_.saveSkillbar(value); }
AttributesResult NetworkServer::characterAttributes(const OwnedCharacter& value) { return manager_.characterAttributes(value); }
RevisionResult NetworkServer::saveAttributes(const SaveAttributes& value) { return manager_.saveAttributes(value); }
InventoryResult NetworkServer::characterInventory(const OwnedCharacter& value) { return manager_.characterInventory(value); }
RevisionResult NetworkServer::saveInventory(const SaveInventory& value) { return manager_.saveInventory(value); }
TradeResult NetworkManager::exchangeTrade(const ExchangeTrade& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.exchangeTrade(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
TradeResult NetworkServer::exchangeTrade(const ExchangeTrade& value) { return manager_.exchangeTrade(value); }
InventoryResult NetworkServer::accountStorage(const Identity& account) { return manager_.accountStorage(account); }
StorageTransferResult NetworkServer::transferStorage(const TransferStorage& value) { return manager_.transferStorage(value); }
FriendsResult NetworkServer::friends(const Identity& p) { return manager_.friends(p); }
FriendAddResult NetworkServer::addFriend(const AddFriend& p) { return manager_.addFriend(p); }
FriendRemoveResult NetworkServer::removeFriend(const FriendTarget& p) { return manager_.removeFriend(p); }
FriendBlockedResult NetworkServer::friendBlocked(const FriendTarget& p) { return manager_.friendBlocked(p); }
GuildResult NetworkServer::guild(const Identity& p) { return manager_.guild(p); }
GuildCommandResult NetworkManager::guildCommand(const GuildCommand& value){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.guildCommand(value,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
GuildCommandResult NetworkServer::guildCommand(const GuildCommand& value){return manager_.guildCommand(value);}
FactionsResult NetworkServer::accountFactions(const Identity& p) { return manager_.accountFactions(p); }
RevisionResult NetworkServer::saveFactions(const SaveFactions& p) { return manager_.saveFactions(p); }
RevisionResult NetworkServer::awardFaction(const AwardFaction& p) { return manager_.awardFaction(p); }
RevisionResult NetworkServer::exchangeImperialFaction(const ExchangeImperialFaction& p) { return manager_.exchangeImperialFaction(p); }
ProgressionResult NetworkServer::characterProgression(const OwnedCharacter& p) { return manager_.characterProgression(p); }
QuestsResult NetworkServer::characterQuests(const OwnedCharacter& p) { return manager_.characterQuests(p); }
RevisionResult NetworkServer::saveQuests(const SaveQuests& p) {
    return manager_.saveQuests(p);
}
RevisionResult NetworkServer::rewardQuestExperience(const RewardQuestExperience& p){return manager_.rewardQuestExperience(p);}
RevisionResult NetworkClient::rewardQuestExperience(const RewardQuestExperience&){return {FW_UNAVAILABLE,"Custom-client transport is not implemented"};}
RevisionResult NetworkServer::saveProgression(const SaveProgression& p) { return manager_.saveProgression(p); }
GuildFactionResult NetworkServer::guildFaction(const Identity& p) { return manager_.guildFaction(p); }
CapeResult NetworkServer::guildCape(const Identity& p) { return manager_.guildCape(p); }
CapePurchaseResult NetworkServer::purchaseGuildCape(const PurchaseGuildCape& p) { return manager_.purchaseGuildCape(p); }
TournamentPointsResult NetworkServer::tournamentPoints(const Identity& p) { return manager_.tournamentPoints(p); }
TournamentPointsResult NetworkServer::purchaseGuildTrim(const PurchaseGuildTrim& p) { return manager_.purchaseGuildTrim(p); }

LoginResult NetworkClient::registerAccount(const RegisterAccount&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
SessionResult NetworkClient::validateSession(const ValidateSession&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
TicketIssuanceResult NetworkClient::issueTicket(const IssueTicket&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
GameAliasResult NetworkClient::gameAccountAlias(const Identity&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
LoginResult NetworkClient::login(const LoginAccount&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
LogoutResult NetworkClient::logout(const LogoutAccount&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
CreationResult NetworkClient::createCharacter(const CreateCharacter&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
DeletionResult NetworkClient::deleteCharacter(const DeleteCharacter&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
RosterResult NetworkClient::characterRoster(const CharacterRoster&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
MapTransitionResult NetworkClient::selectMap(const SelectMap&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
TicketAuthenticationResult NetworkClient::authenticateTicket(const AuthenticateTicket&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
AccountProfileResult NetworkClient::accountProfile(const Identity&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
CampaignsResult NetworkClient::accountCampaigns(const Identity&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
AgreementResult NetworkClient::accountAgreement(const Identity&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
CharacterNumberResult NetworkClient::characterExperience(const OwnedCharacter&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
CharacterNumberResult NetworkClient::characterMap(const OwnedCharacter&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
CharacterNameResult NetworkClient::characterName(const OwnedCharacter&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
CharacterNameResult NetworkClient::renameCharacter(const RenameCharacter&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
DevelopmentUnlockResult NetworkClient::unlockDevelopmentCharacter(const OwnedCharacter&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
SkillsResult NetworkClient::characterSkills(const OwnedCharacter&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
ProfessionSaveResult NetworkClient::saveSecondaryProfession(const SaveSecondaryProfession&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
RevisionResult NetworkClient::saveSkillbar(const SaveSkillbar&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
AttributesResult NetworkClient::characterAttributes(const OwnedCharacter&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
RevisionResult NetworkClient::saveAttributes(const SaveAttributes&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
InventoryResult NetworkClient::characterInventory(const OwnedCharacter&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
RevisionResult NetworkClient::saveInventory(const SaveInventory&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
TradeResult NetworkClient::exchangeTrade(const ExchangeTrade&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
InventoryResult NetworkClient::accountStorage(const Identity&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
StorageTransferResult NetworkClient::transferStorage(const TransferStorage&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
FriendsResult NetworkClient::friends(const Identity&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
FriendAddResult NetworkClient::addFriend(const AddFriend&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
FriendRemoveResult NetworkClient::removeFriend(const FriendTarget&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
FriendBlockedResult NetworkClient::friendBlocked(const FriendTarget&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
GuildResult NetworkClient::guild(const Identity&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
GuildCommandResult NetworkClient::guildCommand(const GuildCommand&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
FactionsResult NetworkClient::accountFactions(const Identity&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
RevisionResult NetworkClient::saveFactions(const SaveFactions&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
RevisionResult NetworkClient::awardFaction(const AwardFaction&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
RevisionResult NetworkClient::exchangeImperialFaction(const ExchangeImperialFaction&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
ProgressionResult NetworkClient::characterProgression(const OwnedCharacter&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
QuestsResult NetworkClient::characterQuests(const OwnedCharacter&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
RevisionResult NetworkClient::saveQuests(const SaveQuests&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
RevisionResult NetworkClient::saveProgression(const SaveProgression&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
GuildFactionResult NetworkClient::guildFaction(const Identity&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
CapeResult NetworkClient::guildCape(const Identity&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
CapePurchaseResult NetworkClient::purchaseGuildCape(const PurchaseGuildCape&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
TournamentPointsResult NetworkClient::tournamentPoints(const Identity&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
TournamentPointsResult NetworkClient::purchaseGuildTrim(const PurchaseGuildTrim&) { return {FW_UNAVAILABLE,"Custom-client transport is not implemented"}; }
}

namespace gwserver {
FriendRemoveResult NetworkManager::removeFriend(const FriendTarget& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.removeFriend(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
}

namespace gwserver {
FriendBlockedResult NetworkManager::friendBlocked(const FriendTarget& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.friendBlocked(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
}

namespace gwserver {
FriendsResult NetworkManager::friends(const Identity& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.friends(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
}

namespace gwserver {
FriendAddResult NetworkManager::addFriend(const AddFriend& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.addFriend(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
}

namespace gwserver {
CapeResult NetworkManager::guildCape(const Identity& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.guildCape(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
}

namespace gwserver {
GuildFactionResult NetworkManager::guildFaction(const Identity& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.guildFaction(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
}

namespace gwserver {
TournamentPointsResult NetworkManager::tournamentPoints(const Identity& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.tournamentPoints(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
}

namespace gwserver {
GuildResult NetworkManager::guild(const Identity& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.guild(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
}

namespace gwserver {
CapePurchaseResult NetworkManager::purchaseGuildCape(const PurchaseGuildCape& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.purchaseGuildCape(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
}

namespace gwserver {
TournamentPointsResult NetworkManager::purchaseGuildTrim(const PurchaseGuildTrim& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.purchaseGuildTrim(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
}

namespace gwserver {
CharacterNumberResult NetworkServer::equipmentVisibility(const OwnedCharacter& request){return manager_.equipmentVisibility(request);}
CharacterNumberResult NetworkClient::equipmentVisibility(const OwnedCharacter&){return {FW_UNAVAILABLE,"Custom-client transport is not implemented"};}
CharacterNumberResult NetworkManager::equipmentVisibility(const OwnedCharacter& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.equipmentVisibility(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
}

namespace gwserver {
CharacterNumberResult NetworkServer::saveEquipmentVisibility(const SaveEquipmentVisibility& request){return manager_.saveEquipmentVisibility(request);}
CharacterNumberResult NetworkClient::saveEquipmentVisibility(const SaveEquipmentVisibility&){return {FW_UNAVAILABLE,"Custom-client transport is not implemented"};}
CharacterNumberResult NetworkManager::saveEquipmentVisibility(const SaveEquipmentVisibility& request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);std::unique_lock lock(database_mutex_,std::defer_lock);
 if(!lock.try_lock_until(deadline))return {FW_TIMEOUT,"Database admission expired"};
 return accounts_.saveEquipmentVisibility(request,[deadline]{return std::chrono::steady_clock::now()<deadline;});
}
}
