// Included after the database helpers, inside accounts.cpp's private namespace.
// All calls remain trusted internal account operations; public handlers derive
// the owner from an authenticated session, never from a request field.
void social_schema(sqlite3* db){
    sql(db,"CREATE TABLE IF NOT EXISTS fw_gw_equipment_visibility(character TEXT PRIMARY KEY REFERENCES fw_gw_characters(id) ON DELETE CASCADE,value INTEGER NOT NULL CHECK(value BETWEEN 0 AND 255));"
           "CREATE TABLE IF NOT EXISTS fw_gw_friends(owner TEXT NOT NULL REFERENCES fw_accounts(id) ON DELETE CASCADE,"
           "target TEXT NOT NULL REFERENCES fw_accounts(id) ON DELETE CASCADE,type INTEGER NOT NULL CHECK(type IN (1,2)),"
           "alias TEXT NOT NULL,PRIMARY KEY(owner,target),CHECK(owner<>target));"
           "CREATE TABLE IF NOT EXISTS fw_gw_guilds(id TEXT PRIMARY KEY,name TEXT NOT NULL COLLATE NOCASE UNIQUE,"
           "tag TEXT NOT NULL COLLATE NOCASE UNIQUE,announcement TEXT NOT NULL DEFAULT '',author TEXT NOT NULL DEFAULT '',hall INTEGER NOT NULL DEFAULT 0);"
           "CREATE TABLE IF NOT EXISTS fw_gw_guild_members(account TEXT PRIMARY KEY REFERENCES fw_accounts(id) ON DELETE CASCADE,"
           "guild TEXT NOT NULL REFERENCES fw_gw_guilds(id) ON DELETE CASCADE,alias TEXT NOT NULL,rank INTEGER NOT NULL CHECK(rank IN (0,1,2)));"
           "CREATE TABLE IF NOT EXISTS fw_gw_guild_capes(guild TEXT PRIMARY KEY REFERENCES fw_gw_guilds(id) ON DELETE CASCADE,design BLOB NOT NULL CHECK(length(design)=8));"
           "CREATE TABLE IF NOT EXISTS fw_gw_guild_faction(guild TEXT PRIMARY KEY REFERENCES fw_gw_guilds(id) ON DELETE CASCADE,allegiance INTEGER NOT NULL CHECK(allegiance IN(0,1)),balance INTEGER NOT NULL CHECK(balance BETWEEN 0 AND 4294967295),decay_day INTEGER NOT NULL CHECK(decay_day>=0));"
           "CREATE TABLE IF NOT EXISTS fw_gw_tournament_points(account TEXT PRIMARY KEY REFERENCES fw_accounts(id) ON DELETE CASCADE,balance INTEGER NOT NULL CHECK(balance>=0 AND balance<=4294967295));"
           "CREATE INDEX IF NOT EXISTS fw_gw_guild_roster ON fw_gw_guild_members(guild);"
           "CREATE TABLE IF NOT EXISTS fw_gw_guild_invites(target TEXT PRIMARY KEY REFERENCES fw_accounts(id) ON DELETE CASCADE,"
           "guild TEXT NOT NULL REFERENCES fw_gw_guilds(id) ON DELETE CASCADE,inviter TEXT NOT NULL REFERENCES fw_accounts(id) ON DELETE CASCADE,expires INTEGER NOT NULL);");
}
bool social_id(std::string_view value){return value.size()==32&&value.find_first_not_of("0123456789abcdef")==value.npos;}
std::string social_text(const std::string& hex,unsigned units,bool empty=false){
    if(hex.size()%2||hex.size()>units*8)throw std::invalid_argument("social text bound");
    std::string value(hex.size()/2,'\0');size_t count=0;
    if(!hex.empty()&&sodium_hex2bin(reinterpret_cast<unsigned char*>(value.data()),value.size(),hex.data(),hex.size(),nullptr,&count,nullptr))throw std::invalid_argument("social text encoding");
    if(value.empty()&&!empty)throw std::invalid_argument("empty social text");
    if(!value.empty())fw::gw::character_name_to_utf16(value,units);return value;
}
std::string social_hex(const std::string& text){return hex_bytes(std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size()));}
bool guild_tag_letters(const std::string& text){
 const auto wide=fw::gw::character_name_to_utf16(text,4);
 std::vector<WORD> classes(wide.size());
 return GetStringTypeW(CT_CTYPE1,reinterpret_cast<const wchar_t*>(wide.data()),static_cast<int>(wide.size()),classes.data())&&
  std::all_of(classes.begin(),classes.end(),[](WORD value){return (value&C1_ALPHA)!=0;});
}
std::optional<std::string> social_target(sqlite3* db,const std::string& name){
    Query target(db,"SELECT DISTINCT account FROM fw_gw_characters WHERE name=? COLLATE NOCASE LIMIT 2");target.text(1,name);
    if(!target.row())return {};auto account=target.text(0);if(target.row())return {};return account;
}
#include "accounts_friends.inc"
#include "accounts_guild_reads.inc"
#include "accounts_guild_purchases.inc"
#include "accounts_guild_commands.inc"
std::optional<AccountResult> social_execute(sqlite3* db,gwserver::Operation operation,const std::vector<std::string>& fields,const std::function<bool()>& permitted){
    if(operation!=gwserver::Operation::friends&&operation!=gwserver::Operation::addFriend&&operation!=gwserver::Operation::removeFriend&&operation!=gwserver::Operation::friendBlocked&&operation!=gwserver::Operation::guild&&operation!=gwserver::Operation::guildCommand&&operation!=gwserver::Operation::guildFaction&&operation!=gwserver::Operation::guildCape&&operation!=gwserver::Operation::purchaseGuildCape&&operation!=gwserver::Operation::tournamentPoints&&operation!=gwserver::Operation::purchaseGuildTrim&&operation!=gwserver::Operation::maintenanceGwGuildIdentitySave)return {};
    if(fields.empty()||!social_id(fields[0]))return AccountResult{FW_INVALID,"invalid social account"};
    Query owner(db,"SELECT 1 FROM fw_accounts WHERE id=?");owner.text(1,fields[0]);if(!owner.row())return AccountResult{FW_NOT_FOUND,"social account not found"};
    if(operation==gwserver::Operation::guildFaction){
        if(fields.size()!=1)return AccountResult{FW_INVALID,"guild faction fields"};
        const auto result=read_guild_faction(db,gwserver::Identity::from_hex(fields[0]),permitted);
        return AccountResult{result.status,result.status==FW_OK?std::to_string(result.value->allegiance)+"\t"+std::to_string(result.value->points):result.message};
    }
    if(operation==gwserver::Operation::maintenanceGwGuildIdentitySave){
        if(fields.size()!=4||!social_id(fields[1]))return AccountResult{FW_INVALID,"guild identity fields"};
        const auto name=social_text(fields[2],31),tag=social_text(fields[3],4);
        if(fw::gw::character_name_to_utf16(name,31).size()<3||!guild_tag_letters(tag))return AccountResult{FW_INVALID,"guild name3..31 characters; tag1..4 letters"};
        Transaction tx(db);Query member(db,"SELECT guild,rank FROM fw_gw_guild_members WHERE account=?");member.text(1,fields[0]);
        if(!member.row()||member.text(0)!=fields[1]||member.number(1)!=0)return AccountResult{FW_CONFLICT,"guild leader identity changed"};
        Query duplicate(db,"SELECT 1 FROM fw_gw_guilds WHERE id<>? AND (name=? COLLATE NOCASE OR tag=? COLLATE NOCASE)");duplicate.text(1,fields[1]);duplicate.text(2,name);duplicate.text(3,tag);
        if(duplicate.row())return AccountResult{FW_CONFLICT,"guild name or tag in use"};
        allowed(permitted);Query write(db,"UPDATE fw_gw_guilds SET name=?,tag=? WHERE id=?");write.text(1,name);write.text(2,tag);write.text(3,fields[1]);write.row();
        allowed(permitted);tx.commit();return AccountResult{FW_OK,"Guild name and tag saved."};
    }
    if(operation==gwserver::Operation::friends){
        if(fields.size()!=1)return AccountResult{FW_INVALID,"friend list fields"};
        const auto result=read_friends(db,gwserver::Identity::from_hex(fields[0]),permitted);
        if(result.status!=FW_OK)return AccountResult{result.status,result.message};
        std::string encoded=std::to_string(result.entries.size());
        for(const auto& row:result.entries)encoded+='\t'+gwserver::storage_hex(row.account.bytes)+"\t"+std::to_string(row.type)+"\t"+social_hex(row.alias)+"\t"+social_hex(row.character);
        return AccountResult{FW_OK,encoded};
    }
    if(operation==gwserver::Operation::addFriend){
        if(fields.size()!=4||(fields[1]!="1"&&fields[1]!="2"))return AccountResult{FW_INVALID,"friend add fields"};
        const auto result=add_friend(db,{gwserver::Identity::from_hex(fields[0]),fields[1]=="1"?1u:2u,social_text(fields[2],19),social_text(fields[3],19,true)},permitted);
        if(result.status!=FW_OK)return AccountResult{result.status,result.message};
        return AccountResult{FW_OK,gwserver::storage_hex(result.value->target.bytes)+"\t"+social_hex(result.value->alias)};
    }
    if(operation==gwserver::Operation::removeFriend||operation==gwserver::Operation::friendBlocked){
        if(fields.size()!=2||!social_id(fields[1]))return AccountResult{FW_INVALID,"friend target fields"};
        const gwserver::FriendTarget request{gwserver::Identity::from_hex(fields[0]),gwserver::Identity::from_hex(fields[1])};
        if(operation==gwserver::Operation::removeFriend){const auto result=remove_friend(db,request,permitted);return AccountResult{result.status,result.status==FW_OK?"removed":result.message};}
        const auto result=friend_blocked(db,request,permitted);return AccountResult{result.status,result.status==FW_OK?(*result.value?"1":"0"):result.message};
    }
    if(operation==gwserver::Operation::tournamentPoints){
        if(fields.size()!=1)return AccountResult{FW_INVALID,"tournament balance fields"};
        const auto result=read_tournament_points(db,gwserver::Identity::from_hex(fields[0]),permitted);
        return AccountResult{result.status,result.status==FW_OK?std::to_string(*result.value):result.message};
    }
    if(operation==gwserver::Operation::purchaseGuildTrim){
        if(fields.size()!=4||!social_id(fields[1]))return AccountResult{FW_INVALID,"trim purchase fields"};
        const auto design=fw::gw::guild::cape::decode(gwserver::profile_bytes(fields[3],8));
        const auto result=buy_guild_trim(db,{{gwserver::Identity::from_hex(fields[0]),gwserver::Identity::from_hex(fields[1])},static_cast<unsigned>(number(fields[2],255)),design},permitted);
        return AccountResult{result.status,result.status==FW_OK?std::to_string(*result.value):result.message};
    }
    if(operation==gwserver::Operation::guildCape){
        if(fields.size()!=1)return AccountResult{FW_INVALID,"cape query fields"};
        const auto result=read_guild_cape(db,gwserver::Identity::from_hex(fields[0]),permitted);
        return AccountResult{result.status,result.status==FW_OK?(result.value?hex_bytes(fw::gw::guild::cape::encode(*result.value)):"0"):result.message};
    }
    if(operation==gwserver::Operation::purchaseGuildCape){
        if(fields.size()!=5||!social_id(fields[1]))return AccountResult{FW_INVALID,"cape purchase fields"};
        const auto design=fw::gw::guild::cape::decode(gwserver::profile_bytes(fields[4],8));
        const auto result=buy_guild_cape(db,{{gwserver::Identity::from_hex(fields[0]),gwserver::Identity::from_hex(fields[1])},number(fields[2],INT64_MAX-1),static_cast<unsigned>(number(fields[3],UINT32_MAX)),design},permitted);
        return AccountResult{result.status,result.status==FW_OK?std::to_string(result.inventory_revision)+"\t"+std::to_string(result.gold):result.message};
    }
    if(operation==gwserver::Operation::guild){
        if(fields.size()!=1)return AccountResult{FW_INVALID,"guild state fields"};
        const auto result=read_guild(db,gwserver::Identity::from_hex(fields[0]),permitted);
        if(result.status!=FW_OK)return AccountResult{result.status,result.message};
        if(!result.value)return AccountResult{FW_OK,"0"};const auto& state=*result.value;
        std::string encoded=gwserver::storage_hex(state.id.bytes)+"\t"+social_hex(state.name)+"\t"+social_hex(state.tag)+"\t"+social_hex(state.announcement)+"\t"+social_hex(state.author)+"\t"+std::to_string(state.hall)+"\t"+std::to_string(state.members.size());
        for(const auto& row:state.members)encoded+="\t"+gwserver::storage_hex(row.account.bytes)+"\t"+social_hex(row.alias)+"\t"+std::to_string(row.rank);
        return AccountResult{FW_OK,encoded};
    }
    if(operation==gwserver::Operation::guildCommand){
        if(fields.size()!=4||!social_id(fields[1]))return AccountResult{FW_INVALID,"guild command fields"};
        const auto action=gwserver::guild_action(fields[2]);if(!action)return AccountResult{FW_INVALID,"unknown guild command"};
        const auto result=mutate_guild(db,{{gwserver::Identity::from_hex(fields[0]),gwserver::Identity::from_hex(fields[1])},*action,social_text(fields[3],255,true)},permitted);
        return AccountResult{result.status,result.message};
    }
    return AccountResult{FW_INVALID,"unknown social operation"};
}
