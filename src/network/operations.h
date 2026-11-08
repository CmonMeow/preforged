#pragma once
#include <optional>
#include <string_view>
namespace gwserver {
// Application operations, not retail opcodes or a serialized wire ABI.
enum class Operation {
    registerAccount,
    validateSession,
    issueTicket,
    gameAccountAlias,
    login,
    logout,
    createCharacter,
    deleteCharacter,
    characterRoster,
    selectMap,
    authenticateTicket,
    accountProfile,
    accountCampaigns,
    accountAgreement,
    characterExperience,
    characterMap,
    characterName,
    renameCharacter,
    unlockDevelopmentCharacter,
    characterSkills,
    saveSecondaryProfession,
    saveSkillbar,
    characterAttributes,
    saveAttributes,
    characterInventory,
    saveInventory,
    exchangeTrade,
    accountStorage,
    transferStorage,
    friends,
    addFriend,
    removeFriend,
    friendBlocked,
    guild,
    guildCommand,
    accountFactions,
    saveFactions,
    awardFaction,
    exchangeImperialFaction,
    characterProgression,
    saveProgression,
    characterQuests,
    saveQuests,
    guildFaction,
    guildCape,
    purchaseGuildCape,
    tournamentPoints,
    purchaseGuildTrim,
    // Trusted offline maintenance operations; no retail or custom-client API.
    maintenanceChangePassword,
    maintenanceGwAccountEulaSet,
    maintenanceGwAccountGameAliasSet,
    maintenanceGwAccountProfileSet,
    maintenanceGwAccountTitlesSeed,
    maintenanceGwCharacterAttributesImport,
    maintenanceGwCharacterPositionGet,
    maintenanceGwCharacterPositionSet,
    maintenanceGwCharacterProfileImport,
    maintenanceGwCharacterProfileImportUtf8,
    maintenanceGwCharacterProgressionImport,
    maintenanceGwCharacterSkillsImport,
    maintenanceGwGuildIdentitySave,
    maintenanceGwTicketConsume,
    maintenanceLogoutAll,
};
// Diagnostic/maintenance names only; production storage dispatch uses this enum.
// Payloads still await migration to request structs.
inline constexpr std::optional<std::string_view> storage_operation(Operation operation) {
    switch(operation) {
    case Operation::registerAccount: return "register";
    case Operation::validateSession: return "validate";
    case Operation::issueTicket: return "gw-ticket-issue";
    case Operation::gameAccountAlias: return "gw-account-game-alias-get";
    case Operation::login: return "login";
    case Operation::logout: return "logout";
    case Operation::createCharacter: return "gw-character-create";
    case Operation::deleteCharacter: return "gw-character-delete";
    case Operation::characterRoster: return "gw-character-roster-page";
    case Operation::selectMap: return "gw-character-map-transition";
    case Operation::authenticateTicket: return "gw-ticket-authenticate";
    case Operation::accountProfile: return "gw-account-profile-get";
    case Operation::accountCampaigns: return "gw-account-campaigns-get";
    case Operation::accountAgreement: return "gw-account-eula-get";
    case Operation::characterExperience: return "gw-character-experience-get";
    case Operation::characterMap: return "gw-character-map-get";
    case Operation::characterName: return "gw-character-name-get";
    case Operation::renameCharacter: return "gw-character-development-rename";
    case Operation::unlockDevelopmentCharacter: return "gw-character-development-unlock";
    case Operation::characterSkills: return "gw-character-skills-get";
    case Operation::saveSecondaryProfession: return "gw-character-secondary-save";
    case Operation::saveSkillbar: return "gw-character-bar-save";
    case Operation::characterAttributes: return "gw-character-attributes-get";
    case Operation::saveAttributes: return "gw-character-attributes-save";
    case Operation::characterInventory: return "gw-character-inventory-get";
    case Operation::saveInventory: return "gw-character-inventory-save";
    case Operation::exchangeTrade: return "gw-trade-exchange";
    case Operation::accountStorage: return "gw-account-storage-get";
    case Operation::transferStorage: return "gw-storage-transfer-save";
    case Operation::friends: return "gw-friend-list";
    case Operation::addFriend: return "gw-friend-add";
    case Operation::removeFriend: return "gw-friend-remove";
    case Operation::friendBlocked: return "gw-friend-blocked";
    case Operation::guild: return "gw-guild-state";
    case Operation::guildCommand: return "gw-guild-command";
    case Operation::accountFactions: return "gw-account-faction-get";
    case Operation::saveFactions: return "gw-account-faction-save";
    case Operation::awardFaction: return "gw-account-faction-award";
    case Operation::exchangeImperialFaction: return "gw-account-faction-exchange";
    case Operation::characterProgression: return "gw-character-progression-get";
    case Operation::saveProgression: return "gw-character-progression-save";
    case Operation::characterQuests: return "gw-character-quests-get";
    case Operation::saveQuests: return "gw-character-quests-save";
    case Operation::guildFaction: return "gw-guild-faction-get";
    case Operation::guildCape: return "gw-guild-cape-get";
    case Operation::purchaseGuildCape: return "gw-guild-cape-buy";
    case Operation::tournamentPoints: return "gw-guild-trim-balance";
    case Operation::purchaseGuildTrim: return "gw-guild-trim-buy";
    case Operation::maintenanceChangePassword: return "change-password";
    case Operation::maintenanceGwAccountEulaSet: return "gw-account-eula-set";
    case Operation::maintenanceGwAccountGameAliasSet: return "gw-account-game-alias-set";
    case Operation::maintenanceGwAccountProfileSet: return "gw-account-profile-set";
    case Operation::maintenanceGwAccountTitlesSeed: return "gw-account-titles-seed";
    case Operation::maintenanceGwCharacterAttributesImport: return "gw-character-attributes-import";
    case Operation::maintenanceGwCharacterPositionGet: return "gw-character-position-get";
    case Operation::maintenanceGwCharacterPositionSet: return "gw-character-position-set";
    case Operation::maintenanceGwCharacterProfileImport: return "gw-character-profile-import";
    case Operation::maintenanceGwCharacterProfileImportUtf8: return "gw-character-profile-import-utf8";
    case Operation::maintenanceGwCharacterProgressionImport: return "gw-character-progression-import";
    case Operation::maintenanceGwCharacterSkillsImport: return "gw-character-skills-import";
    case Operation::maintenanceGwGuildIdentitySave: return "gw-guild-identity-save";
    case Operation::maintenanceGwTicketConsume: return "gw-ticket-consume";
    case Operation::maintenanceLogoutAll: return "logout-all";
    }
    return {};
}

// Compatibility at the offline maintenance-tool boundary only.
inline constexpr std::optional<Operation> parse_storage_operation(std::string_view name) {
    if(name=="register")return Operation::registerAccount;
    if(name=="validate")return Operation::validateSession;
    if(name=="gw-ticket-issue")return Operation::issueTicket;
    if(name=="gw-account-game-alias-get")return Operation::gameAccountAlias;
    if(name=="login")return Operation::login;
    if(name=="logout")return Operation::logout;
    if(name=="gw-character-create")return Operation::createCharacter;
    if(name=="gw-character-delete")return Operation::deleteCharacter;
    if(name=="gw-character-roster-page")return Operation::characterRoster;
    if(name=="gw-character-map-transition")return Operation::selectMap;
    if(name=="gw-ticket-authenticate")return Operation::authenticateTicket;
    if(name=="gw-account-profile-get")return Operation::accountProfile;
    if(name=="gw-account-campaigns-get")return Operation::accountCampaigns;
    if(name=="gw-account-eula-get")return Operation::accountAgreement;
    if(name=="gw-character-experience-get")return Operation::characterExperience;
    if(name=="gw-character-map-get")return Operation::characterMap;
    if(name=="gw-character-name-get")return Operation::characterName;
    if(name=="gw-character-development-rename")return Operation::renameCharacter;
    if(name=="gw-character-development-unlock")return Operation::unlockDevelopmentCharacter;
    if(name=="gw-character-skills-get")return Operation::characterSkills;
    if(name=="gw-character-secondary-save")return Operation::saveSecondaryProfession;
    if(name=="gw-character-bar-save")return Operation::saveSkillbar;
    if(name=="gw-character-attributes-get")return Operation::characterAttributes;
    if(name=="gw-character-attributes-save")return Operation::saveAttributes;
    if(name=="gw-character-inventory-get")return Operation::characterInventory;
    if(name=="gw-character-inventory-save")return Operation::saveInventory;
    if(name=="gw-trade-exchange")return Operation::exchangeTrade;
    if(name=="gw-account-storage-get")return Operation::accountStorage;
    if(name=="gw-storage-transfer-save")return Operation::transferStorage;
    if(name=="gw-friend-list")return Operation::friends;
    if(name=="gw-friend-add")return Operation::addFriend;
    if(name=="gw-friend-remove")return Operation::removeFriend;
    if(name=="gw-friend-blocked")return Operation::friendBlocked;
    if(name=="gw-guild-state")return Operation::guild;
    if(name=="gw-guild-command")return Operation::guildCommand;
    if(name=="gw-account-faction-get")return Operation::accountFactions;
    if(name=="gw-account-faction-save")return Operation::saveFactions;
    if(name=="gw-account-faction-award")return Operation::awardFaction;
    if(name=="gw-account-faction-exchange")return Operation::exchangeImperialFaction;
    if(name=="gw-character-progression-get")return Operation::characterProgression;
    if(name=="gw-character-progression-save")return Operation::saveProgression;
    if(name=="gw-character-quests-get")return Operation::characterQuests;
    if(name=="gw-character-quests-save")return Operation::saveQuests;
    if(name=="gw-guild-faction-get")return Operation::guildFaction;
    if(name=="gw-guild-cape-get")return Operation::guildCape;
    if(name=="gw-guild-cape-buy")return Operation::purchaseGuildCape;
    if(name=="gw-guild-trim-balance")return Operation::tournamentPoints;
    if(name=="gw-guild-trim-buy")return Operation::purchaseGuildTrim;
    if(name=="change-password")return Operation::maintenanceChangePassword;
    if(name=="gw-account-eula-set")return Operation::maintenanceGwAccountEulaSet;
    if(name=="gw-account-game-alias-set")return Operation::maintenanceGwAccountGameAliasSet;
    if(name=="gw-account-profile-set")return Operation::maintenanceGwAccountProfileSet;
    if(name=="gw-account-titles-seed")return Operation::maintenanceGwAccountTitlesSeed;
    if(name=="gw-character-attributes-import")return Operation::maintenanceGwCharacterAttributesImport;
    if(name=="gw-character-position-get")return Operation::maintenanceGwCharacterPositionGet;
    if(name=="gw-character-position-set")return Operation::maintenanceGwCharacterPositionSet;
    if(name=="gw-character-profile-import")return Operation::maintenanceGwCharacterProfileImport;
    if(name=="gw-character-profile-import-utf8")return Operation::maintenanceGwCharacterProfileImportUtf8;
    if(name=="gw-character-progression-import")return Operation::maintenanceGwCharacterProgressionImport;
    if(name=="gw-character-skills-import")return Operation::maintenanceGwCharacterSkillsImport;
    if(name=="gw-guild-identity-save")return Operation::maintenanceGwGuildIdentitySave;
    if(name=="gw-ticket-consume")return Operation::maintenanceGwTicketConsume;
    if(name=="logout-all")return Operation::maintenanceLogoutAll;
    return {};
}
}
