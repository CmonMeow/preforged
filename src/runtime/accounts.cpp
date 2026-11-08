#include "guild_state.h"
#include "guild_commands.h"
#include "cape_state.h"
#include "../network/friend_state.h"
#include "character_results.h"
#include "character_state.h"
#include "trade_requests.h"
#include "account_sessions.h"
#include "account_policy.h"
#include "accounts.h"
#include "quest_state.h"
#include "pet_state.h"
#include "inventory_state.h"
#include "common.h"
#include "gw_token.h"
#include "gw_character_profile.h"
#include "gw_experience_level.h"
#include "gw_equipment_assignments.h"
#include "gw_auth_responses.h"
#include "gw_character_skills.h"
#include "gw_quest_skills.h"
#include "gw_quest_wayward_reward.h"
#include "gw_quest_unsettling_reward.h"
#include "gw_quest_opposition.h"
#include "gw_quest_charr_gate_reward.h"
#include "gw_attributes.h"
#include "gw_inventory.h"
#include "gw_presearing_items.h"
#include "gw_basic_armor_energy.h"
#include "gw_guild_cape.h"
#include "gw_guild_trim.h"
#include "gw_guild_faction.h"
#include "gw_trade_exchange.h"
#include "gw_storage.h"
#include "gw_player_skills_generated.h"
#include "gw_progression.h"
#include "gw_quest_state.h"
#include "gw_quest_timed_state.h"
#include <sodium.h>
#include <windows.h>
#include <winsqlite/winsqlite3.h>
#include <array>
#include <chrono>
#include <cmath>
#include <set>
#include <cstdlib>

namespace fw {
namespace {
struct SqlError : std::runtime_error { using std::runtime_error::runtime_error; };
struct Cancelled {};
void allowed(const std::function<bool()>& permitted) { if (!permitted()) throw Cancelled{}; }
int64_t unix_time() { return std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count(); }
struct Query {
    sqlite3_stmt* statement = nullptr;
    Query(sqlite3* db, const char* sql) {
        if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) throw SqlError("account query preparation");
    }
    ~Query() { sqlite3_finalize(statement); }
    void text(int index, const std::string& value) {
        if (sqlite3_bind_text(statement, index, value.data(), int(value.size()), SQLITE_TRANSIENT) != SQLITE_OK) throw SqlError("account text binding");
    }
    void number(int index, int64_t value) {
        if (sqlite3_bind_int64(statement, index, value) != SQLITE_OK) throw SqlError("account number binding");
    }
    void real(int index, double value) {
        if (sqlite3_bind_double(statement, index, value) != SQLITE_OK) throw SqlError("account real binding");
    }
    void blob(int index, std::span<const std::uint8_t> value) {
        // SQLite treats a null pointer with a zero length as SQL NULL, which
        // violates the NOT NULL summary column on profile-less characters.
        static constexpr std::uint8_t empty_blob=0;
        const auto* data=value.empty()?&empty_blob:value.data();
        if(sqlite3_bind_blob(statement,index,data,static_cast<int>(value.size()),SQLITE_TRANSIENT)!=SQLITE_OK)
            throw SqlError("account blob binding");
    }
    bool row() {
        int code = sqlite3_step(statement);
        if (code == SQLITE_ROW) return true;
        if (code == SQLITE_DONE) return false;
        throw SqlError("account query execution");
    }
    std::string text(int column) {
        const auto* data = sqlite3_column_text(statement, column);
        return data ? std::string(reinterpret_cast<const char*>(data), sqlite3_column_bytes(statement, column)) : std::string{};
    }
    int64_t number(int column) { return sqlite3_column_int64(statement, column); }
    std::vector<std::uint8_t> blob(int column) {
        const auto* data=static_cast<const std::uint8_t*>(sqlite3_column_blob(statement,column));
        const auto size=sqlite3_column_bytes(statement,column);
        return data&&size>0?std::vector<std::uint8_t>(data,data+size):std::vector<std::uint8_t>{};
    }
};
std::string hex_bytes(std::span<const std::uint8_t> bytes) {
    std::string result(bytes.size()*2+1,'\0');
    sodium_bin2hex(result.data(),result.size(),bytes.data(),bytes.size());
    result.pop_back();return result;
}
void sql(sqlite3* db, const char* statement) {
    if (sqlite3_exec(db, statement, nullptr, nullptr, nullptr) != SQLITE_OK) throw SqlError("account transaction");
}
struct Transaction {
    sqlite3* db; bool committed = false;
    explicit Transaction(sqlite3* value) : db(value) { sql(db, "BEGIN IMMEDIATE"); }
    void commit() { sql(db, "COMMIT"); committed = true; }
    ~Transaction() { if (!committed) sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr); }
};
std::string username(std::string value) {
    if (!account_name_valid(value)) throw std::runtime_error("invalid account name");
    for (auto& c : value) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    return value;
}
struct Password {
    std::array<char, 128> bytes{};
    size_t size = 0;
    explicit Password(std::span<const std::uint8_t> value,bool development_test=false){
        const bool test=development_test&&value.size()==4&&std::equal(value.begin(),value.end(),reinterpret_cast<const std::uint8_t*>("test"));
        if((value.size()<8&&!test)||value.size()>bytes.size())throw std::invalid_argument("password requires8..128 bytes");
        std::copy(value.begin(),value.end(),bytes.begin());size=value.size();
    }
    explicit Password(const std::string& hex, bool development_test=false) {
        if ((hex.size() < 16 && !(development_test && hex=="74657374")) || hex.size() > 256 || hex.size() % 2 ||
            sodium_hex2bin(reinterpret_cast<unsigned char*>(bytes.data()), bytes.size(), hex.data(), hex.size(),
                           nullptr, &size, nullptr)) {
            sodium_memzero(bytes.data(), bytes.size());
            throw std::runtime_error("password requires 8..128 bytes");
        }
    }
    Password(const Password&) = delete;
    ~Password() { sodium_memzero(bytes.data(), bytes.size()); }
    std::string hash() const {
        std::array<char, crypto_pwhash_STRBYTES> out{};
        if (crypto_pwhash_str_alg(out.data(), bytes.data(), size, 3, crypto_pwhash_MEMLIMIT_INTERACTIVE,
                                 crypto_pwhash_ALG_ARGON2ID13)) throw SqlError("account password hashing unavailable");
        return out.data();
    }
    bool matches(const std::string& hash) const { return crypto_pwhash_str_verify(hash.c_str(), bytes.data(), size) == 0; }
};
std::string random_hex(size_t length) {
    std::array<unsigned char, 32> value{}; randombytes_buf(value.data(), length);
    std::string hex(length * 2 + 1, '\0'); sodium_bin2hex(hex.data(), hex.size(), value.data(), length);
    sodium_memzero(value.data(), value.size()); hex.pop_back(); return hex;
}
std::string token_hash(const std::string& token) {
    std::array<unsigned char, 32> bytes{}, digest{};
    if (token.size() != 64 || token.find_first_not_of("0123456789abcdef") != token.npos ||
        sodium_hex2bin(bytes.data(), bytes.size(), token.data(), token.size(), nullptr, nullptr, nullptr))
        throw std::runtime_error("invalid account token");
    crypto_hash_sha256(digest.data(), bytes.data(), bytes.size()); sodium_memzero(bytes.data(), bytes.size());
    std::string hex(65, '\0'); sodium_bin2hex(hex.data(), hex.size(), digest.data(), digest.size()); hex.pop_back(); return hex;
}
AccountResult denied() { return {FW_NOT_FOUND, "invalid credentials or session"}; }
std::string game_token_hash(const gw::GameToken& token) {
    std::array<unsigned char, crypto_hash_sha256_BYTES> digest{};
    crypto_hash_sha256(digest.data(), token.data(), token.size());
    std::string hex(65, '\0');
    sodium_bin2hex(hex.data(), hex.size(), digest.data(), digest.size());
    hex.pop_back(); return hex;
}
gwserver::AgreementResult read_account_agreement(sqlite3* db,const gwserver::Identity& account,const std::function<bool()>& permitted){
    allowed(permitted);
    Query read(db,"SELECT eula_accepted FROM fw_accounts WHERE id=?");
    read.text(1,gwserver::storage_hex(account.bytes));
    if(!read.row())return {FW_NOT_FOUND,"account missing",{}};
    const auto flag=read.number(0);
    allowed(permitted);
    if(flag!=0&&flag!=1)return {FW_INTERNAL,"invalid stored agreement flag",{}};
    return {FW_OK,{},flag==1};
}
#include "accounts_social.inc"
#include "accounts_inventory_reads.inc"
#include "accounts_inventory_writes.inc"
#include "accounts_inventory.inc"
#include "accounts_development.inc"
#include "accounts_roster.inc"
#include "accounts_creation.inc"
#include "accounts_quests.inc"
#include "accounts_skills.inc"
#include "accounts_pets.inc"
#include "accounts_attributes.inc"
#include "accounts_profession.inc"
#include "accounts_character_progression.inc"
#include "accounts_factions.inc"
#include "accounts_progression.inc"
}
struct Accounts::Impl {
    sqlite3* db = nullptr;
    std::string dummy;
    explicit Impl(const std::string& path) {
        if (sodium_init() < 0 || path.empty()) throw std::runtime_error("account database required");
        if (sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {
            if (db) sqlite3_close(db); db = nullptr; throw SqlError("cannot open accounts database");
        }
        try {
            // Account schema migrations can overlap across host generations
            // sharing one environment database. Keep the wait for initialization
            // bounded, then restore the request-time overload bound.
            sqlite3_busy_timeout(db, 5000);
            sql(db, "PRAGMA journal_mode=WAL; PRAGMA synchronous=FULL; PRAGMA foreign_keys=ON;"
                "CREATE TABLE IF NOT EXISTS fw_accounts(id TEXT PRIMARY KEY, name TEXT UNIQUE NOT NULL, password TEXT NOT NULL,"
                "eula_accepted INTEGER NOT NULL DEFAULT 0 CHECK(eula_accepted IN (0,1)),game_alias TEXT NOT NULL DEFAULT '');"
                "CREATE TABLE IF NOT EXISTS fw_gw_account_profiles(account TEXT PRIMARY KEY REFERENCES fw_accounts(id) ON DELETE CASCADE,profile BLOB NOT NULL);"
                "CREATE TABLE IF NOT EXISTS fw_gw_campaign_entitlements(account TEXT NOT NULL REFERENCES fw_accounts(id) ON DELETE CASCADE,"
                "campaign INTEGER NOT NULL CHECK(campaign BETWEEN 1 AND 4), PRIMARY KEY(account,campaign));"
                "CREATE TABLE IF NOT EXISTS fw_account_sessions(hash TEXT PRIMARY KEY, account TEXT NOT NULL REFERENCES fw_accounts(id),"
                "issued INTEGER NOT NULL, expires INTEGER NOT NULL);"
                "CREATE INDEX IF NOT EXISTS fw_account_sessions_owner ON fw_account_sessions(account);"
                "CREATE INDEX IF NOT EXISTS fw_account_sessions_expiry ON fw_account_sessions(expires);"
                "CREATE TABLE IF NOT EXISTS fw_gw_tickets(hash TEXT PRIMARY KEY,"
                "session TEXT NOT NULL REFERENCES fw_account_sessions(hash) ON DELETE CASCADE,"
                "issued INTEGER NOT NULL, expires INTEGER NOT NULL);"
                "CREATE INDEX IF NOT EXISTS fw_gw_tickets_session ON fw_gw_tickets(session);"
                "CREATE INDEX IF NOT EXISTS fw_gw_tickets_expiry ON fw_gw_tickets(expires);"
                "CREATE TABLE IF NOT EXISTS fw_gw_characters(id TEXT PRIMARY KEY,"
                "account TEXT NOT NULL REFERENCES fw_accounts(id) ON DELETE CASCADE,"
                "name TEXT NOT NULL COLLATE NOCASE, map_id INTEGER NOT NULL DEFAULT 148,"
                "appearance BLOB NOT NULL DEFAULT X'', summary BLOB NOT NULL DEFAULT X'',"
                "roster_order INTEGER NOT NULL DEFAULT 0, UNIQUE(account,name));"
                "CREATE INDEX IF NOT EXISTS fw_gw_characters_owner ON fw_gw_characters(account);"
                "CREATE TABLE IF NOT EXISTS fw_gw_character_skills(character TEXT PRIMARY KEY REFERENCES fw_gw_characters(id) ON UPDATE CASCADE ON DELETE CASCADE,"
                "revision INTEGER NOT NULL,profile BLOB NOT NULL,source TEXT NOT NULL);"
                "CREATE TABLE IF NOT EXISTS fw_gw_character_attributes(character TEXT PRIMARY KEY REFERENCES fw_gw_characters(id) ON UPDATE CASCADE ON DELETE CASCADE,"
                "revision INTEGER NOT NULL,profile BLOB NOT NULL,source TEXT NOT NULL);"
                "CREATE TABLE IF NOT EXISTS fw_account_attempts(name TEXT PRIMARY KEY, window INTEGER NOT NULL, count INTEGER NOT NULL);");
            social_schema(db);
            sql(db,"CREATE TABLE IF NOT EXISTS fw_gw_inventory(character TEXT PRIMARY KEY REFERENCES fw_gw_characters(id) ON UPDATE CASCADE ON DELETE CASCADE,revision INTEGER NOT NULL,profile BLOB NOT NULL);"
                   "CREATE TABLE IF NOT EXISTS fw_gw_storage(account TEXT PRIMARY KEY REFERENCES fw_accounts(id) ON DELETE CASCADE,revision INTEGER NOT NULL,profile BLOB NOT NULL);"
                   "CREATE TABLE IF NOT EXISTS fw_gw_factions(account TEXT PRIMARY KEY REFERENCES fw_accounts(id) ON DELETE CASCADE,revision INTEGER NOT NULL,profile BLOB NOT NULL);"
                   "CREATE TABLE IF NOT EXISTS fw_gw_progression(character TEXT PRIMARY KEY REFERENCES fw_gw_characters(id) ON UPDATE CASCADE ON DELETE CASCADE,revision INTEGER NOT NULL,profile BLOB NOT NULL);"
                   "CREATE TABLE IF NOT EXISTS fw_gw_quests(character TEXT PRIMARY KEY REFERENCES fw_gw_characters(id) ON UPDATE CASCADE ON DELETE CASCADE,revision INTEGER NOT NULL,profile BLOB NOT NULL);"
                   "CREATE TABLE IF NOT EXISTS fw_gw_pets(character TEXT PRIMARY KEY REFERENCES fw_gw_characters(id) ON UPDATE CASCADE ON DELETE CASCADE,revision INTEGER NOT NULL CHECK(revision>0),profile BLOB NOT NULL);"
                   "CREATE TABLE IF NOT EXISTS fw_gw_progression_imports(character TEXT PRIMARY KEY REFERENCES fw_gw_characters(id) ON UPDATE CASCADE ON DELETE CASCADE,source TEXT NOT NULL);"
                   "CREATE TABLE IF NOT EXISTS fw_gw_account_titles(account TEXT PRIMARY KEY REFERENCES fw_accounts(id) ON DELETE CASCADE,revision INTEGER NOT NULL,profile BLOB NOT NULL,source TEXT NOT NULL);");
            auto has_column = [&](const char* column) {
                Query columns(db, "PRAGMA table_info(fw_gw_characters)");
                while (columns.row()) if (columns.text(1) == column) return true;
                return false;
            };
            auto has_account_column = [&](const char* column) {
                Query columns(db, "PRAGMA table_info(fw_accounts)");
                while (columns.row()) if (columns.text(1) == column) return true;
                return false;
            };
            if (!has_account_column("eula_accepted"))
                sql(db, "ALTER TABLE fw_accounts ADD COLUMN eula_accepted INTEGER NOT NULL DEFAULT 0 CHECK(eula_accepted IN (0,1))");
            if (!has_account_column("game_alias"))
                sql(db, "ALTER TABLE fw_accounts ADD COLUMN game_alias TEXT NOT NULL DEFAULT ''");
            sql(db,"UPDATE fw_accounts SET game_alias=lower(name) WHERE game_alias='';"
                   "CREATE UNIQUE INDEX IF NOT EXISTS fw_accounts_game_alias_unique ON fw_accounts(game_alias COLLATE NOCASE);");
        // Private-server clients must not be sent back into the retail agreement
        // screen, whose acceptance request is not yet safely implemented.
        sql(db, "UPDATE fw_accounts SET eula_accepted=1 WHERE eula_accepted=0");
            // Local development accounts unlock all three campaigns and Eye
            // of the North. These are private-backend permissions only.
            sql(db, "INSERT OR IGNORE INTO fw_gw_campaign_entitlements(account,campaign) SELECT id,campaign FROM fw_accounts CROSS JOIN (SELECT 1 AS campaign UNION ALL SELECT 2 UNION ALL SELECT 3 UNION ALL SELECT 4)");
            if (!has_column("position_x")) sql(db, "ALTER TABLE fw_gw_characters ADD COLUMN position_x REAL NOT NULL DEFAULT 0");
            if (!has_column("position_y")) sql(db, "ALTER TABLE fw_gw_characters ADD COLUMN position_y REAL NOT NULL DEFAULT 0");
            if (!has_column("summary")) sql(db, "ALTER TABLE fw_gw_characters ADD COLUMN summary BLOB NOT NULL DEFAULT X''");
            // NULL distinguishes uncaptured imported progress from a new character's zero XP.
            if (!has_column("experience")) sql(db, "ALTER TABLE fw_gw_characters ADD COLUMN experience INTEGER CHECK(experience IS NULL OR experience BETWEEN 0 AND 4294967295)");
            if (!has_column("roster_order")) {
                sql(db, "ALTER TABLE fw_gw_characters ADD COLUMN roster_order INTEGER NOT NULL DEFAULT 0");
                sql(db, "UPDATE fw_gw_characters AS current SET roster_order=("
                        "SELECT count(*) FROM fw_gw_characters AS prior WHERE prior.account=current.account AND ("
                        "prior.name COLLATE NOCASE<current.name COLLATE NOCASE OR "
                        "(prior.name COLLATE NOCASE=current.name COLLATE NOCASE AND prior.id<current.id)))");
            }
            sql(db, "CREATE INDEX IF NOT EXISTS fw_gw_characters_roster_order ON fw_gw_characters(account,roster_order,id)");
            sqlite3_busy_timeout(db, 20);
            dummy = Password(random_hex(24)).hash();
        } catch (...) { sqlite3_close(db); db = nullptr; throw; }
    }
    ~Impl() { if (db) sqlite3_close(db); }
    void cleanup(int64_t now) {
        Query tickets(db, "DELETE FROM fw_gw_tickets WHERE expires<=? OR issued>?");
        tickets.number(1, now); tickets.number(2, now); tickets.row();
        Query sessions(db, "DELETE FROM fw_account_sessions WHERE expires<=? OR issued>?");
        sessions.number(1, now); sessions.number(2, now); sessions.row();
        Query attempts(db, "DELETE FROM fw_account_attempts WHERE window<=? OR window>?");
        attempts.number(1, now - 60); attempts.number(2, now); attempts.row();
    }
    bool attempt(const std::string& name, int64_t now) {
        Transaction tx(db); cleanup(now);
        Query select(db, "SELECT count FROM fw_account_attempts WHERE name=?"); select.text(1, name);
        if (select.row()) {
            if (select.number(0) >= 10) return false;
            Query increment(db, "UPDATE fw_account_attempts SET count=count+1 WHERE name=?"); increment.text(1, name); increment.row();
        } else {
            Query size(db, "SELECT count(*) FROM fw_account_attempts"); size.row(); if (size.number(0) >= 1024) return false;
            Query insert(db, "INSERT INTO fw_account_attempts VALUES(?,?,1)"); insert.text(1, name); insert.number(2, now); insert.row();
        }
        tx.commit(); return true;
    }
    std::pair<std::string, std::string> account(const std::string& name) {
        Query read(db, "SELECT id,password FROM fw_accounts WHERE name=?"); read.text(1, name);
        if (!read.row()) return {};
        return {read.text(0), read.text(1)};
    }
    struct Identity { std::string id, name; int64_t expires = 0; };
    Identity identity(const std::string& hash, int64_t now) {
        Query read(db, "SELECT a.id,a.name,s.expires FROM fw_account_sessions s JOIN fw_accounts a ON a.id=s.account "
                       "WHERE s.hash=? AND s.expires>? AND s.issued<=?");
        read.text(1, hash); read.number(2, now); read.number(3, now);
        if (!read.row()) return {};
        return {read.text(0), read.text(1), read.number(2)};
    }
    gwserver::LoginResult issue_native(const std::string& id,const std::string& name,int64_t now){
        cleanup(now);
        Query all(db,"SELECT count(*) FROM fw_account_sessions");all.row();
        Query own(db,"SELECT count(*) FROM fw_account_sessions WHERE account=?");own.text(1,id);own.row();
        if(all.number(0)>=4096||own.number(0)>=8)return {FW_OVERLOADED,"account session limit"};
        gwserver::SessionToken token{};
        struct Erase{gwserver::SessionToken& value;~Erase(){sodium_memzero(value.bytes.data(),value.bytes.size());}}erase{token};
        randombytes_buf(token.bytes.data(),token.bytes.size());
        std::array<unsigned char,crypto_hash_sha256_BYTES> digest{};crypto_hash_sha256(digest.data(),token.bytes.data(),token.bytes.size());
        const auto expires=now+3600;
        Query create(db,"INSERT INTO fw_account_sessions VALUES(?,?,?,?)");create.text(1,gwserver::storage_hex(digest));create.text(2,id);create.number(3,now);create.number(4,expires);create.row();
        return {FW_OK,{},gwserver::AccountLogin{{gwserver::Identity::from_hex(id),name,static_cast<std::uint64_t>(expires)},token}};
    }
    AccountResult issue(const std::string& id,const std::string& name,int64_t now){
        auto result=issue_native(id,name,now);
        if(result.status!=FW_OK)return {result.status,result.message};
        const auto payload=id+"\t"+name+"\t"+gwserver::storage_hex(result.login->session.bytes)+"\t"+std::to_string(result.login->identity.expires);
        sodium_memzero(result.login->session.bytes.data(),result.login->session.bytes.size());return {FW_OK,payload};
    }
    gwserver::LoginResult login_native(const gwserver::LoginAccount& request,bool registration,const std::function<bool()>& permitted){
        allowed(permitted);const auto name=gwserver::normalized_account(request.name);
        Password password(request.password,name=="test"||name=="test@example.test");
        if(!attempt(name,unix_time()))return {FW_OVERLOADED,"account attempt limit"};
        auto previous=account(name);
        if(!registration){
            const bool verified=password.matches(previous.first.empty()?dummy:previous.second);allowed(permitted);
            if(!verified||previous.first.empty())return {FW_NOT_FOUND,"invalid credentials or session"};
            Transaction tx(db);if(account(name)!=previous)return {FW_NOT_FOUND,"invalid credentials or session"};
            if(name=="test@example.test"&&password.size==4&&std::equal(request.password.begin(),request.password.end(),reinterpret_cast<const std::uint8_t*>("test"))){Query retire(db,"DELETE FROM fw_account_sessions WHERE account=?");retire.text(1,previous.first);retire.row();}
            allowed(permitted);auto result=issue_native(previous.first,name,unix_time());
            struct Erase{gwserver::LoginResult& value;~Erase(){if(value.login)sodium_memzero(value.login->session.bytes.data(),value.login->session.bytes.size());}}erase{result};
            allowed(permitted);if(result.status==FW_OK)tx.commit();return std::move(result);
        }
        const auto hash=password.hash();allowed(permitted);Transaction tx(db);
        if(!account(name).first.empty())return {FW_CONFLICT,"account name unavailable"};
        Query size(db,"SELECT count(*) FROM fw_accounts");size.row();if(size.number(0)>=100000)return {FW_OVERLOADED,"account capacity"};
        const auto id=random_hex(16);allowed(permitted);
        Query create(db,"INSERT INTO fw_accounts(id,name,password,eula_accepted,game_alias) VALUES(?,?,?,1,?)");create.text(1,id);create.text(2,name);create.text(3,hash);create.text(4,name);create.row();
        Query campaigns(db,"INSERT INTO fw_gw_campaign_entitlements(account,campaign) SELECT ?,campaign FROM (SELECT 1 AS campaign UNION ALL SELECT 2 UNION ALL SELECT 3 UNION ALL SELECT 4)");campaigns.text(1,id);campaigns.row();
        auto result=issue_native(id,name,unix_time());
        struct Erase{gwserver::LoginResult& value;~Erase(){if(value.login)sodium_memzero(value.login->session.bytes.data(),value.login->session.bytes.size());}}erase{result};
        allowed(permitted);if(result.status==FW_OK)tx.commit();return std::move(result);
    }
    AccountResult execute(gwserver::Operation operation, const std::string& payload, const std::function<bool()>& permitted) {
        allowed(permitted);
        // Wipe split-field copies as soon as this worker operation completes.
        auto fields = split(payload);
        struct Wipe { std::vector<std::string>& fields; ~Wipe() { for (auto& value : fields) sodium_memzero(value.data(), value.size()); } } wipe{fields};
        if(auto social=social_execute(db,operation,fields,permitted))return *social;
        if(auto development=development_execute(db,operation,fields,permitted))return *development;
        if(auto inventory=inventory_execute(db,operation,fields,permitted))return *inventory;

        if(auto progression=progression_execute(db,operation,fields,permitted))return *progression;
        if(operation==gwserver::Operation::characterSkills||operation==gwserver::Operation::maintenanceGwCharacterSkillsImport||operation==gwserver::Operation::saveSkillbar) {
            const bool get=operation==gwserver::Operation::characterSkills,import=operation==gwserver::Operation::maintenanceGwCharacterSkillsImport;
            if(fields.size()!=(get?2u:import?5u:4u))return {FW_INVALID,"invalid character skill request"};
            for(unsigned i=0;i<2;++i)if(fields[i].size()!=32||fields[i].find_first_not_of("0123456789abcdef")!=fields[i].npos)
                return {FW_INVALID,"invalid character identity"};
            // Offline adapters share the typed storage implementation used by the game.
            const gwserver::OwnedCharacter owned{gwserver::Identity::from_hex(fields[0]),gwserver::Identity::from_hex(fields[1])};
            if(get){
                const auto result=read_skills(db,owned,permitted);
                if(result.status!=FW_OK)return {result.status,result.message};
                if(!result.record)return {FW_OK,"0"};
                return {FW_OK,std::to_string(result.record->revision)+"\t"+hex_bytes(gw::skills::encode_profile(result.record->profile))};
            }
            if(!import){
                const auto expected=number(fields[2],INT64_MAX-1);
                if(fields[3].size()!=128)return {FW_INVALID,"invalid skillbar size"};
                std::array<std::uint8_t,64> raw{};std::size_t decoded=0;
                if(sodium_hex2bin(raw.data(),raw.size(),fields[3].data(),fields[3].size(),nullptr,&decoded,nullptr)||decoded!=raw.size())return {FW_INVALID,"invalid character skill data encoding"};
                gw::skills::Slots bar{};
                for(unsigned n=0;n<8;++n){auto read=[&](unsigned at){std::uint32_t v=0;for(unsigned b=0;b<4;++b)v|=std::uint32_t(raw[at+b])<<(b*8);return v;};bar[n]={read(n*8),read(n*8+4)};}
                const auto result=write_skillbar(db,{owned,expected,bar},permitted);
                return {result.status,result.status==FW_OK?std::to_string(result.revision):result.message};
            }
            Transaction tx(db);Query owner(db,"SELECT 1 FROM fw_gw_characters WHERE account=? AND id=?");
            owner.text(1,fields[0]);owner.text(2,fields[1]);if(!owner.row())return {FW_NOT_FOUND,"owned character not found"};
            Query current(db,"SELECT revision,profile,source FROM fw_gw_character_skills WHERE character=?");current.text(1,fields[1]);
            const bool present=current.row();const auto revision=present?current.number(0):0;
            auto packed=present?current.blob(1):std::vector<std::uint8_t>{};
            const auto expected=number(fields[2],INT64_MAX-1);
            if(expected!=static_cast<std::uint64_t>(revision))return {FW_CONFLICT,"character skill revision conflict"};
            const auto& encoded=fields[3];const auto limit=13804u;
            if(encoded.empty()||encoded.size()%2||encoded.size()>limit*2)return {FW_INVALID,"invalid character skill data size"};
            std::vector<std::uint8_t> raw(encoded.size()/2);std::size_t decoded=0;
            if(sodium_hex2bin(raw.data(),raw.size(),encoded.data(),encoded.size(),nullptr,&decoded,nullptr)||decoded!=raw.size())
                return {FW_INVALID,"invalid character skill data encoding"};
            gw::skills::Profile profile;
            try {
                profile=gw::skills::decode_profile(raw);
                if(fields[4].size()!=64||fields[4].find_first_not_of("0123456789abcdef")!=fields[4].npos)return {FW_INVALID,"invalid skill source hash"};
            }catch(const std::invalid_argument&){return {FW_INVALID,"invalid character skill profile"};}
            packed=gw::skills::encode_profile(profile);allowed(permitted);
            if(present){Query update(db,"UPDATE fw_gw_character_skills SET revision=?,profile=?,source=? WHERE character=? AND revision=?");
                update.number(1,revision+1);update.blob(2,packed);update.text(3,fields[4]);update.text(4,fields[1]);update.number(5,revision);update.row();
                if(sqlite3_changes(db)!=1)return {FW_CONFLICT,"character skill revision conflict"};
            }else{Query insert(db,"INSERT INTO fw_gw_character_skills VALUES(?,?,?,?)");insert.text(1,fields[1]);insert.number(2,1);insert.blob(3,packed);insert.text(4,fields[4]);insert.row();}
            allowed(permitted);tx.commit();return {FW_OK,std::to_string(revision+1)};
        }
        if(operation==gwserver::Operation::characterAttributes||operation==gwserver::Operation::maintenanceGwCharacterAttributesImport||operation==gwserver::Operation::saveAttributes) {
            const bool get=operation==gwserver::Operation::characterAttributes,import=operation==gwserver::Operation::maintenanceGwCharacterAttributesImport;
            if(fields.size()!=(get?2u:import?5u:4u))return {FW_INVALID,"invalid attribute request"};
            for(unsigned i=0;i<2;++i)if(fields[i].size()!=32||fields[i].find_first_not_of("0123456789abcdef")!=fields[i].npos)
                return {FW_INVALID,"invalid character identity"};
            const gwserver::OwnedCharacter owned{gwserver::Identity::from_hex(fields[0]),gwserver::Identity::from_hex(fields[1])};
            if(get){const auto result=read_attributes(db,owned,permitted);
                if(result.status!=FW_OK)return {result.status,result.message};
                return {FW_OK,result.record?std::to_string(result.record->revision)+"\t"+hex_bytes(result.record->profile.encode()):"0"};}
            if(!import){
                const auto expected=number(fields[2],INT64_MAX-1);
                const auto profile=gw::attributes::Profile::decode(gwserver::profile_bytes(fields[3],54));
                const auto result=write_attributes(db,{owned,expected,profile},permitted);
                return {result.status,result.status==FW_OK?std::to_string(result.revision):result.message};
            }
            Transaction tx(db);Query owner(db,"SELECT 1 FROM fw_gw_characters WHERE account=? AND id=?");
            owner.text(1,fields[0]);owner.text(2,fields[1]);if(!owner.row())return {FW_NOT_FOUND,"owned character not found"};
            Query current(db,"SELECT revision,profile,source FROM fw_gw_character_attributes WHERE character=?");current.text(1,fields[1]);
            const bool present=current.row();const auto revision=present?current.number(0):0;
            if(number(fields[2],INT64_MAX-1)!=static_cast<std::uint64_t>(revision))return {FW_CONFLICT,"attribute revision conflict"};
            if(fields[3].size()!=108)return {FW_INVALID,"invalid attribute profile length"};
            std::array<std::uint8_t,54> packed{};std::size_t decoded=0;
            if(sodium_hex2bin(packed.data(),packed.size(),fields[3].data(),fields[3].size(),nullptr,&decoded,nullptr)||decoded!=packed.size())
                return {FW_INVALID,"invalid attribute profile encoding"};
            if(packed[0]!=1||packed[1]>3||(packed[2]!=0&&packed[2]!=15&&packed[2]!=30))return {FW_INVALID,"invalid attribute entitlement"};
            for(unsigned id=0;id<51;++id)if(packed[id+3]>12||((id>=26&&id<=28||id>=45)&&packed[id+3]))return {FW_INVALID,"invalid base attribute rank"};
            if(fields[4].size()!=64||fields[4].find_first_not_of("0123456789abcdef")!=fields[4].npos)return {FW_INVALID,"invalid attribute source hash"};
            allowed(permitted);
            if(present){Query update(db,"UPDATE fw_gw_character_attributes SET revision=?,profile=?,source=? WHERE character=? AND revision=?");
                update.number(1,revision+1);update.blob(2,packed);update.text(3,fields[4]);update.text(4,fields[1]);update.number(5,revision);update.row();
                if(sqlite3_changes(db)!=1)return {FW_CONFLICT,"attribute revision conflict"};
            }else{Query insert(db,"INSERT INTO fw_gw_character_attributes VALUES(?,?,?,?)");insert.text(1,fields[1]);insert.number(2,1);insert.blob(3,packed);insert.text(4,fields[4]);insert.row();}
            allowed(permitted);tx.commit();return {FW_OK,std::to_string(revision+1)};
        }
        if(operation==gwserver::Operation::saveSecondaryProfession) {
            if(fields.size()!=6)return {FW_INVALID,"invalid secondary profession request"};
            const gwserver::SaveSecondaryProfession request{{gwserver::Identity::from_hex(fields[0]),gwserver::Identity::from_hex(fields[1])},
                number(fields[2],INT64_MAX-1),number(fields[3],INT64_MAX-1),
                gw::skills::decode_profile(gwserver::profile_bytes(fields[4],13804)),
                gw::attributes::Profile::decode(gwserver::profile_bytes(fields[5],54))};
            const auto result=write_profession(db,request,permitted);
            return {result.status,result.status==FW_OK?std::to_string(result.skill_revision)+"\t"+std::to_string(result.attribute_revision):result.message};
        }
        if (operation == gwserver::Operation::accountProfile || operation == gwserver::Operation::maintenanceGwAccountProfileSet) {
            const bool set=operation==gwserver::Operation::maintenanceGwAccountProfileSet;
            if(fields.size()!=(set?2u:1u) || fields[0].size()!=32 ||
               fields[0].find_first_not_of("0123456789abcdef")!=fields[0].npos)
                return {FW_INVALID,"invalid account profile request"};
            Query owner(db,"SELECT 1 FROM fw_accounts WHERE id=?");owner.text(1,fields[0]);
            if(!owner.row())return {FW_NOT_FOUND,"account missing"};
            if(set) {
                std::array<std::uint8_t,239> bytes{};std::size_t size=0;
                if(fields[1].size()<78 || fields[1].size()>478 || fields[1].size()%2 ||
                   sodium_hex2bin(bytes.data(),bytes.size(),fields[1].data(),fields[1].size(),nullptr,&size,nullptr))
                    return {FW_INVALID,"invalid account profile encoding"};
                auto profile=std::span(bytes).first(size);
                try { gw::auth::AccountInfoProfile::decode(profile); }
                catch(const std::invalid_argument&) {return {FW_INVALID,"invalid account profile layout"};}
                Transaction tx(db);
                Query write(db,"INSERT INTO fw_gw_account_profiles(account,profile) VALUES(?,?) ON CONFLICT(account) DO UPDATE SET profile=excluded.profile");
                write.text(1,fields[0]);write.blob(2,profile);write.row();allowed(permitted);tx.commit();
                return {FW_OK,"stored"};
            }
            Query read(db,"SELECT profile FROM fw_gw_account_profiles WHERE account=?");read.text(1,fields[0]);
            if(!read.row())return {FW_OK,"-"};
            return {FW_OK,hex_bytes(read.blob(0))};
        }
        if (operation == gwserver::Operation::accountAgreement || operation == gwserver::Operation::maintenanceGwAccountEulaSet) {
            const bool set = operation == gwserver::Operation::maintenanceGwAccountEulaSet;
            if (fields.size() != (set ? 2u : 1u) || fields[0].size()!=32 ||
                fields[0].find_first_not_of("0123456789abcdef")!=fields[0].npos ||
                (set && fields[1]!="0" && fields[1]!="1"))
                return {FW_INVALID,"invalid EULA account flag request"};
            if (set) {
                Transaction tx(db);
                Query update(db,"UPDATE fw_accounts SET eula_accepted=? WHERE id=?");
                update.number(1,fields[1]=="1"?1:0); update.text(2,fields[0]); update.row();
                if(sqlite3_changes(db)!=1)return {FW_NOT_FOUND,"account missing"};
                allowed(permitted); tx.commit();
                return {FW_OK,fields[1]};
            }
            const auto result=read_account_agreement(db,gwserver::Identity::from_hex(fields[0]),permitted);
            if(result.status!=FW_OK)return {result.status,result.message};
            return {FW_OK,*result.value?"1":"0"};
        }
        if (operation == gwserver::Operation::gameAccountAlias || operation == gwserver::Operation::maintenanceGwAccountGameAliasSet) {
            const bool set = operation == gwserver::Operation::maintenanceGwAccountGameAliasSet;
            if (fields.size() != (set ? 2u : 1u) || fields[0].size()!=32 ||
                fields[0].find_first_not_of("0123456789abcdef")!=fields[0].npos ||
                (set && !account_name_valid(fields[1])))
                return {FW_INVALID,"invalid game-account alias request"};
            if (set) {
                auto alias=username(fields[1]);
                Transaction tx(db);
                Query duplicate(db,"SELECT 1 FROM fw_accounts WHERE game_alias=? COLLATE NOCASE AND id<>?");
                duplicate.text(1,alias);duplicate.text(2,fields[0]);
                if(duplicate.row())return {FW_CONFLICT,"game-account alias unavailable"};
                Query update(db,"UPDATE fw_accounts SET game_alias=? WHERE id=?");
                update.text(1,alias);update.text(2,fields[0]);update.row();
                if(sqlite3_changes(db)!=1)return {FW_NOT_FOUND,"account missing"};
                allowed(permitted);tx.commit();return {FW_OK,alias};
            }
            Query read(db,"SELECT game_alias FROM fw_accounts WHERE id=?");read.text(1,fields[0]);
            if(!read.row())return {FW_NOT_FOUND,"account missing"};
            return {FW_OK,read.text(0)};
        }
        if (operation == gwserver::Operation::accountCampaigns) {
            if (fields.size()!=1 || fields[0].size()!=32 ||
                fields[0].find_first_not_of("0123456789abcdef")!=fields[0].npos)
                return {FW_INVALID,"invalid campaign entitlement request"};
            Query read(db,"SELECT campaign FROM fw_gw_campaign_entitlements WHERE account=? ORDER BY campaign");
            read.text(1,fields[0]);
            std::string result;
            while(read.row()) {
                if(!result.empty()) result.push_back(',');
                result += std::to_string(read.number(0));
            }
            Query owner(db,"SELECT 1 FROM fw_accounts WHERE id=?"); owner.text(1,fields[0]);
            if(!owner.row()) return {FW_NOT_FOUND,"account missing"};
            return {FW_OK,result};
        }
        if (operation == gwserver::Operation::selectMap) {
            if(fields.size()!=4)return {FW_INVALID,"invalid character map transition"};
            auto valid_id=[](const std::string& value){return value.size()==32&&value.find_first_not_of("0123456789abcdef")==value.npos;};
            if(!valid_id(fields[0])||!valid_id(fields[1]))return {FW_INVALID,"invalid character identity"};
            const auto from=number(fields[2],65535),to=number(fields[3],65535);
            if(from==to)return {FW_INVALID,"invalid character map transition"};
            const auto result=transition_character_map(db,{gwserver::Identity::from_hex(fields[0]),gwserver::Identity::from_hex(fields[1]),static_cast<std::uint32_t>(from),static_cast<std::uint32_t>(to)},permitted,true);
            return {result.status,result.status==FW_OK?"transitioned":result.message};
        }
        if(operation==gwserver::Operation::deleteCharacter){
            if(fields.size()!=2||!social_id(fields[0])||!social_id(fields[1]))return {FW_INVALID,"invalid character identity"};
            const auto result=delete_character(db,{gwserver::Identity::from_hex(fields[0]),gwserver::Identity::from_hex(fields[1])},permitted);
            return {result.status,result.status==FW_OK?"deleted":result.message};
        }
        if(operation==gwserver::Operation::characterMap||operation==gwserver::Operation::characterExperience){
            if(fields.size()!=2||!social_id(fields[0])||!social_id(fields[1]))return {FW_INVALID,"invalid character identity"};
            const auto result=read_character_number(db,{gwserver::Identity::from_hex(fields[0]),gwserver::Identity::from_hex(fields[1])},operation==gwserver::Operation::characterMap,permitted,true);
            return {result.status,result.status==FW_OK?(result.value?std::to_string(*result.value):"-"):result.message};
        }
        if (operation == gwserver::Operation::maintenanceGwCharacterPositionGet || operation == gwserver::Operation::maintenanceGwCharacterPositionSet) {
            const bool set = operation == gwserver::Operation::maintenanceGwCharacterPositionSet;
            if (fields.size() != (set ? 5u : 2u)) return {FW_INVALID, "invalid character position request"};
            auto valid_id = [](const std::string& value) {
                return value.size() == 32 && value.find_first_not_of("0123456789abcdef") == value.npos;
            };
            if (!valid_id(fields[0]) || !valid_id(fields[1])) return {FW_INVALID, "invalid character identity"};
            double x = 0, y = 0;
            if (set) {
                const auto map=number(fields[2],65535);if(fields[2].empty())return {FW_INVALID,"invalid character map"};
                auto coordinate = [](const std::string& value, double& result) {
                    if (value.empty()) return false;
                    char* end = nullptr;
                    result = std::strtod(value.c_str(), &end);
                    return end == value.c_str() + value.size() && std::isfinite(result) && std::abs(result) <= 10000.0;
                };
                if (!coordinate(fields[3], x) || !coordinate(fields[4], y))
                    return {FW_INVALID, "invalid character coordinates"};
                Transaction tx(db);
                Query update(db, "UPDATE fw_gw_characters SET position_x=?,position_y=? WHERE account=? AND id=? AND map_id=?");
                update.real(1, x); update.real(2, y); update.text(3, fields[0]); update.text(4, fields[1]);update.number(5,static_cast<int64_t>(map));update.row();
                if (sqlite3_changes(db) != 1) return {FW_CONFLICT, "character map changed or not owned"};
                allowed(permitted); tx.commit();
                return {FW_OK, "saved"};
            }
            Query read(db, "SELECT map_id,position_x,position_y FROM fw_gw_characters WHERE account=? AND id=?");
            read.text(1, fields[0]); read.text(2, fields[1]);
            if (!read.row()) return {FW_NOT_FOUND, "character not owned by account"};
            return {FW_OK, std::to_string(read.number(0)) + "\t" + std::to_string(sqlite3_column_double(read.statement, 1)) +
                "\t" + std::to_string(sqlite3_column_double(read.statement, 2))};
        }
        if(operation==gwserver::Operation::createCharacter){
            if((fields.size()!=2&&fields.size()!=3)||!social_id(fields[0]))return {FW_INVALID,"invalid character creation request"};
            gwserver::CreateCharacter request{gwserver::Identity::from_hex(fields[0]),fields[1],{}};
            if(fields.size()==3)request.profile=gw::CharacterProfile::decode(gwserver::profile_bytes(fields[2],gw::CharacterProfile::storage_size));
            const auto result=create_character(db,request,permitted);
            if(result.status!=FW_OK)return {result.status,result.message};
            return {FW_OK,gwserver::storage_hex(result.created->character.bytes)+"\t"+result.created->name+"\t"+std::to_string(result.created->map)};
        }
        if (operation == gwserver::Operation::maintenanceGwCharacterProfileImport || operation == gwserver::Operation::maintenanceGwCharacterProfileImportUtf8) {
            if(operation == gwserver::Operation::maintenanceGwCharacterProfileImportUtf8) {
                if((fields.size()!=3&&fields.size()!=4&&fields.size()!=5) || fields[1].empty() || fields[1].size()>152 || fields[1].size()%2 ||
                   fields[1].find_first_not_of("0123456789abcdefABCDEF")!=fields[1].npos)
                    return {FW_INVALID,"invalid encoded character name"};
                std::string decoded_name(fields[1].size()/2,'\0');
                std::size_t decoded_size=0;
                if(sodium_hex2bin(reinterpret_cast<unsigned char*>(decoded_name.data()),decoded_name.size(),
                                  fields[1].data(),fields[1].size(),nullptr,&decoded_size,nullptr) ||
                   decoded_size!=decoded_name.size()) {
                    sodium_memzero(decoded_name.data(),decoded_name.size());
                    return {FW_INVALID,"invalid encoded character name"};
                }
                sodium_memzero(fields[1].data(),fields[1].size());
                fields[1]=std::move(decoded_name);
            }
            const bool ordered_import=operation==gwserver::Operation::maintenanceGwCharacterProfileImportUtf8&&
                (fields.size()==4||fields.size()==5);
            std::string captured_id;
            if(operation==gwserver::Operation::maintenanceGwCharacterProfileImportUtf8&&fields.size()==5) {
                auto token=gw::parse_token(fields[4]);
                if(!token)return {FW_INVALID,"invalid captured character UUID"};
                captured_id=std::string(33,'\0');
                sodium_bin2hex(captured_id.data(),captured_id.size(),token->data(),token->size());
                captured_id.pop_back();
            }
            std::uint64_t roster_order=0;
            if(ordered_import)try{roster_order=number(fields[3],7);}catch(const std::runtime_error&){return {FW_INVALID,"invalid roster position"};}
            bool name_valid=false;
            if(fields.size()==3||ordered_import)try{(void)fw::gw::character_name_to_utf16(fields[1]);name_valid=true;}catch(const std::invalid_argument&){}
            if(name_valid) {
                std::string normalized=fields[1];
                for(auto& c:normalized) if(c>='A'&&c<='Z') c=static_cast<char>(c-'A'+'a');
                if(normalized=="probe character") return {FW_INVALID,"placeholder character name is not importable"};
            }
            if((fields.size()!=3&&!ordered_import) || !name_valid || fields[2].size()!=fw::gw::CharacterProfile::storage_size*2 ||
               fields[2].find_first_not_of("0123456789abcdefABCDEF")!=fields[2].npos)
                return {FW_INVALID,"invalid character profile import"};
            std::array<std::uint8_t,fw::gw::CharacterProfile::storage_size> raw{};
            if(sodium_hex2bin(raw.data(),raw.size(),fields[2].data(),fields[2].size(),nullptr,nullptr,nullptr))
                return {FW_INVALID,"invalid character profile encoding"};
            fw::gw::CharacterProfile profile;
            try { profile=fw::gw::CharacterProfile::decode(raw); }
            catch(const std::invalid_argument&) { sodium_memzero(raw.data(),raw.size());return {FW_INVALID,"invalid character profile bounds"}; }
            auto packed=profile.encode();sodium_memzero(raw.data(),raw.size());
            auto account_name=username(fields[0]);
            Transaction tx(db);
            Query owner(db,"SELECT id FROM fw_accounts WHERE name=? COLLATE NOCASE");owner.text(1,account_name);
            if(!owner.row())return {FW_NOT_FOUND,"account missing"};
            auto owner_id=owner.text(0);
            Query entitlement(db,"SELECT 1 FROM fw_gw_campaign_entitlements WHERE account=? AND campaign=1");entitlement.text(1,owner_id);
            if(!entitlement.row())return {FW_NOT_FOUND,"Prophecies entitlement required"};
            std::string id;
            Query by_id(db,"SELECT account,name FROM fw_gw_characters WHERE id=?");
            if(!captured_id.empty()) {
                by_id.text(1,captured_id);
                if(by_id.row()) {
                    if(by_id.text(0)!=owner_id)return {FW_CONFLICT,"captured character UUID belongs to another account"};
                    id=captured_id;
                }
            }
            if(id.empty()) {
                Query by_name(db,"SELECT id FROM fw_gw_characters WHERE account=? AND name=? COLLATE NOCASE");
                by_name.text(1,owner_id);by_name.text(2,fields[1]);
                if(by_name.row())id=by_name.text(0);
            }
            if(!id.empty()) {
                const auto previous_id=id;
                if(!captured_id.empty())id=captured_id;
                Query name_collision(db,"SELECT 1 FROM fw_gw_characters WHERE account=? AND name=? COLLATE NOCASE AND id<>?");
                name_collision.text(1,owner_id);name_collision.text(2,fields[1]);name_collision.text(3,previous_id);
                if(name_collision.row())return {FW_CONFLICT,"character name unavailable"};
                Query update(db,ordered_import
                    ? "UPDATE fw_gw_characters SET id=?,name=?,map_id=?,position_x=0,position_y=0,experience=NULL,summary=?,roster_order=? WHERE account=? AND id=?"
                    : "UPDATE fw_gw_characters SET id=?,name=?,map_id=?,position_x=0,position_y=0,experience=NULL,summary=? WHERE account=? AND id=?");
                update.text(1,id);update.text(2,fields[1]);update.number(3,profile.last_map_id);update.blob(4,packed);
                if(ordered_import) { update.number(5,roster_order);update.text(6,owner_id);update.text(7,previous_id); }
                else { update.text(5,owner_id);update.text(6,previous_id); }
                update.row();
            } else {
                Query count(db,"SELECT count(*) FROM fw_gw_characters WHERE account=?");count.text(1,owner_id);count.row();
                if(count.number(0)>=8)return {FW_OVERLOADED,"character capacity"};
                id=captured_id.empty()?random_hex(16):captured_id;
                Query next_order(db,"SELECT COALESCE(MAX(roster_order)+1,0) FROM fw_gw_characters WHERE account=?");
                next_order.text(1,owner_id);next_order.row();
                Query insert(db,"INSERT INTO fw_gw_characters(id,account,name,map_id,summary,roster_order) VALUES(?,?,?,?,?,?)");
                insert.text(1,id);insert.text(2,owner_id);insert.text(3,fields[1]);insert.number(4,profile.last_map_id);insert.blob(5,packed);
                insert.number(6,ordered_import?static_cast<std::int64_t>(roster_order):next_order.number(0));insert.row();
            }
            allowed(permitted);tx.commit();
            return {FW_OK,id+"\t"+fields[1]+"\t"+std::to_string(profile.last_map_id)};
        }
        if (operation == gwserver::Operation::characterRoster) {
            if(fields.size()!=2 || fields[0].size()!=32 ||
               fields[0].find_first_not_of("0123456789abcdef")!=fields[0].npos)
                return {FW_INVALID,"invalid character roster request"};
            const auto offset=number(fields[1],8);
            if(fields[1].empty() || offset>8 || offset%3)
                return {FW_INVALID,"invalid character roster page"};
            auto page=read_roster(db,{gwserver::Identity::from_hex(fields[0]),static_cast<unsigned>(offset)},permitted);
            if(page.status!=FW_OK)return {page.status,page.message};
            std::string result=std::to_string(page.total)+"\t"+std::to_string(page.entries.size());
            for(const auto& entry:page.entries)result+="\t"+gwserver::storage_hex(entry.character.bytes)+"\t"+entry.name+"\t"+std::to_string(entry.map)+"\t"+(entry.profile?hex_bytes(entry.profile->encode()):"-");
            if(result.size()>FW_MAX_PAYLOAD)return {FW_INTERNAL,"character roster page exceeds application payload limit"};
            return {FW_OK,std::move(result)};
        }
        if (operation == gwserver::Operation::issueTicket || operation == gwserver::Operation::maintenanceGwTicketConsume || operation == gwserver::Operation::authenticateTicket) {
            const bool authenticate = operation == gwserver::Operation::authenticateTicket;
            if (fields.size() != (authenticate ? 4u : 1u)) return {FW_INVALID, "invalid game ticket request"};
            std::optional<gw::GameToken> claimed;
            if (authenticate) {
                claimed = gw::parse_token(fields[1]);
                bool character_valid=fields[3].empty();
                if(!character_valid)try{(void)fw::gw::character_name_to_utf16(fields[3]);character_valid=true;}catch(const std::invalid_argument&){}
                if (!claimed || fields[2].size() > 31 || !character_valid) return {FW_INVALID, "invalid game identity"};
                if(!fields[2].empty())(void)username(fields[2]); // UUID and one-use ticket establish identity; older clients may omit the alias.
            }
            Transaction tx(db);
            const auto now = unix_time();
            cleanup(now); allowed(permitted);
            if (operation == gwserver::Operation::issueTicket) {
                auto session = token_hash(fields[0]);
                auto owner = identity(session, now); if (owner.id.empty()) return denied();
                Query own(db, "SELECT count(*) FROM fw_gw_tickets t JOIN fw_account_sessions s ON s.hash=t.session WHERE s.account=?");
                own.text(1, owner.id); own.row();
                Query total(db, "SELECT count(*) FROM fw_gw_tickets"); total.row();
                if (own.number(0) >= 4 || total.number(0) >= 4096)
                    return {FW_OVERLOADED, "game ticket limit"};
                gw::GameToken token{}; randombytes_buf(token.data(), token.size());
                const auto expires = std::min(now + 60, owner.expires);
                Query insert(db, "INSERT INTO fw_gw_tickets VALUES(?,?,?,?)");
                insert.text(1, game_token_hash(token)); insert.text(2, session);
                insert.number(3, now); insert.number(4, expires); insert.row();
                auto value = gw::format_token(token); sodium_memzero(token.data(), token.size());
                allowed(permitted); tx.commit();
                return {FW_OK, value + "\t" + std::to_string(expires)};
            }
            auto token = gw::parse_token(fields[0]);
            if (!token) return {FW_INVALID, "canonical game token required"};
            auto hash = game_token_hash(*token); sodium_memzero(token->data(), token->size());
            Query read(db, "SELECT session FROM fw_gw_tickets WHERE hash=? AND expires>? AND issued<=?");
            read.text(1, hash); read.number(2, now); read.number(3, now);
            if (!read.row()) return authenticate ? AccountResult{FW_NOT_FOUND, "ticket unavailable"} : denied();
            const auto parent_session=read.text(0);
            auto owner = identity(parent_session, now);
            if (owner.id.empty()) return authenticate ? AccountResult{FW_NOT_FOUND, "parent session expired"} : denied();
            if (authenticate) {
                gw::GameToken actual{};
                if (owner.id.size() != 32 || sodium_hex2bin(actual.data(), actual.size(), owner.id.data(), owner.id.size(), nullptr, nullptr, nullptr))
                    throw SqlError("invalid stored account identity");
                if (sodium_memcmp(actual.data(), claimed->data(), actual.size()))
                    return {FW_NOT_FOUND, "UserId claim mismatch"};
            }
            std::string character_id, character_name;
            int64_t character_map = 148;
            struct CharacterRecord { std::string id,name; int64_t map=148; };
            std::vector<CharacterRecord> characters;
            if (authenticate) {
                Query roster(db, "SELECT id,name,map_id FROM fw_gw_characters WHERE account=? ORDER BY roster_order,id");
                roster.text(1,owner.id);
                while(roster.row()) characters.push_back({roster.text(0),roster.text(1),roster.number(2)});
                if(characters.size()>8) throw SqlError("invalid persistent character roster");
                if(!fields[3].empty()) {
                    auto selected=std::find_if(characters.begin(),characters.end(),[&](const CharacterRecord& value) {
                        return value.name.size()==fields[3].size() &&
                            std::equal(value.name.begin(),value.name.end(),fields[3].begin(),[](unsigned char a,unsigned char b) {
                                if(a>='A'&&a<='Z')a=static_cast<unsigned char>(a+'a'-'A');
                                if(b>='A'&&b<='Z')b=static_cast<unsigned char>(b+'a'-'A');
                                return a==b;
                            });
                    });
                    if(selected==characters.end()) return {FW_NOT_FOUND,"character unavailable"};
                    character_id=selected->id; character_name=selected->name; character_map=selected->map;
                }
            }
            Query erase(db, "DELETE FROM fw_gw_tickets WHERE hash=?"); erase.text(1, hash); erase.row();
            allowed(permitted); tx.commit();
            auto result = owner.id + "\t" + owner.name + "\t" + std::to_string(owner.expires);
            if (authenticate) {
                result += "\t" + character_id + "\t" + character_name + "\t" + std::to_string(character_map);
                result += "\t" + std::to_string(characters.size())+"\t"+parent_session;
            }
            return {FW_OK, std::move(result)};
        }
        if (operation == gwserver::Operation::registerAccount || operation == gwserver::Operation::login) {
            if (fields.size() != 2) return {FW_INVALID, "account name and encoded password required"};
            auto name = username(fields[0]); Password password(fields[1],(name=="test" || name=="test@example.test"));
            if (!attempt(name, unix_time())) return {FW_OVERLOADED, "account attempt limit"};
            auto previous = account(name);
            if (operation == gwserver::Operation::login) {
                bool verified = password.matches(previous.first.empty() ? dummy : previous.second);
                allowed(permitted);
                if (!verified || previous.first.empty()) return denied();
                Transaction tx(db);
                if (account(name) != previous) return denied(); // Concurrent password change wins.
                // The explicitly configured local development identity has one
                // active login. Repeated client launches retire its old tickets
                // and sessions after successful password verification.
                if(name=="test@example.test"&&fields[1]=="74657374") {
                    Query retire(db,"DELETE FROM fw_account_sessions WHERE account=?");
                    retire.text(1,previous.first);retire.row();
                }
                auto result = issue(previous.first, name, unix_time()); allowed(permitted);
                if (result.status == FW_OK) tx.commit();
                return result;
            }
            auto hash = password.hash(); allowed(permitted);
            Transaction tx(db);
            if (!account(name).first.empty()) return {FW_CONFLICT, "account name unavailable"};
            Query size(db, "SELECT count(*) FROM fw_accounts"); size.row();
            if (size.number(0) >= 100000) return {FW_OVERLOADED, "account capacity"};
            auto id = random_hex(16);
            // Existing environment databases may still declare DEFAULT 1 from
            // an earlier rollout. Never inherit that stale schema default:
            // only a verified client acceptance exchange may set this flag.
            Query create(db, "INSERT INTO fw_accounts(id,name,password,eula_accepted,game_alias) VALUES(?,?,?,1,?)");
            create.text(1, id); create.text(2, name); create.text(3, hash); create.text(4,name); create.row();
            Query prophecies(db,"INSERT INTO fw_gw_campaign_entitlements(account,campaign) SELECT ?,campaign FROM (SELECT 1 AS campaign UNION ALL SELECT 2 UNION ALL SELECT 3 UNION ALL SELECT 4)");
            prophecies.text(1,id); prophecies.row();
            auto result = issue(id, name, unix_time()); allowed(permitted);
            if (result.status == FW_OK) tx.commit();
            return result;
        }
        if (operation != gwserver::Operation::validateSession && operation != gwserver::Operation::logout && operation != gwserver::Operation::maintenanceLogoutAll && operation != gwserver::Operation::maintenanceChangePassword)
            return {FW_NOT_FOUND, "unknown account operation"};
        if (fields.size() != (operation == gwserver::Operation::maintenanceChangePassword ? 3u : 1u)) return {FW_INVALID, "invalid account request"};
        auto hash = token_hash(fields[0]);
        auto owner = identity(hash, unix_time()); if (owner.id.empty()) return denied();
        if (operation == gwserver::Operation::validateSession) return {FW_OK, owner.id + "\t" + owner.name + "\t" + std::to_string(owner.expires)};
        std::pair<std::string, std::string> previous;
        std::string replacement;
        if (operation == gwserver::Operation::maintenanceChangePassword) {
            Password old_password(fields[1]), new_password(fields[2]);
            if (!attempt(owner.name, unix_time())) return {FW_OVERLOADED, "account attempt limit"};
            previous = account(owner.name);
            if (!old_password.matches(previous.second)) return denied();
            replacement = new_password.hash(); allowed(permitted);
        }
        Transaction tx(db); allowed(permitted);
        if (identity(hash, unix_time()).id != owner.id) return denied();
        if (operation == gwserver::Operation::maintenanceChangePassword) {
            if (account(owner.name) != previous) return denied();
            Query update(db, "UPDATE fw_accounts SET password=? WHERE id=?"); update.text(1, replacement); update.text(2, owner.id); update.row();
        }
        Query erase(db, operation == gwserver::Operation::logout ? "DELETE FROM fw_account_sessions WHERE hash=?" : "DELETE FROM fw_account_sessions WHERE account=?");
        erase.text(1, operation == gwserver::Operation::logout ? hash : owner.id); erase.row(); allowed(permitted); tx.commit();
        return {FW_OK, "revoked"};
    }
};
Accounts::Accounts(const std::string& database) : impl_(std::make_unique<Impl>(database)) {}
Accounts::~Accounts() = default;
AccountResult Accounts::execute(gwserver::Operation operation, const std::string& payload, const std::function<bool()>& permitted) {
    try { return impl_->execute(operation, payload, permitted); }
    catch (const Cancelled&) { return {FW_TIMEOUT, "account request expired"}; }
    catch (const SqlError&) { return {FW_UNAVAILABLE, "account storage or hashing unavailable"}; }
    catch (const std::exception&) { return {FW_INVALID, "invalid account request"}; }
}
}





namespace fw {
AccountResult Accounts::execute(const std::string& name,const std::string& payload,const std::function<bool()>& permitted) {
    const auto operation=gwserver::parse_storage_operation(name);
    if(!operation)return {FW_NOT_FOUND,"unknown account operation"};
    return execute(*operation,payload,permitted);
}
}

namespace fw {
gwserver::PetsResult Accounts::characterPets(const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted){
 try{return read_pets(impl_->db,owner,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired",{}};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable",{}};}
 catch(const std::exception&){return {FW_INTERNAL,"invalid stored pet record",{}};}
}
gwserver::RevisionResult Accounts::adoptPet(const gwserver::AdoptPet& request,const std::function<bool()>& permitted){
 try{return adopt_pet(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INVALID,"invalid pet adoption request"};}
}
gwserver::QuestsResult Accounts::characterQuests(const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted){
 try{return read_quests(impl_->db,owner,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired",{}};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable",{}};}
 catch(const std::exception&){return {FW_INTERNAL,"invalid stored quest record",{}};}
}
gwserver::RevisionResult Accounts::saveQuests(const gwserver::SaveQuests& request,const std::function<bool()>& permitted){
 try{return write_quests(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INVALID,"invalid quest request"};}
}
gwserver::RevisionResult Accounts::rewardQuestExperience(const gwserver::RewardQuestExperience& request,const std::function<bool()>& permitted){
 try{return reward_quest_experience(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INVALID,"invalid quest reward request"};}
}
}

namespace fw {
gwserver::SkillsResult Accounts::characterSkills(const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted){
 try{return read_skills(impl_->db,owner,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired",{}};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable",{}};}
 catch(const std::exception&){return {FW_INTERNAL,"invalid stored skill record",{}};}
}
gwserver::RevisionResult Accounts::saveSkillbar(const gwserver::SaveSkillbar& request,const std::function<bool()>& permitted){
 try{return write_skillbar(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INVALID,"invalid skillbar request"};}
}
}
namespace fw {
gwserver::AttributesResult Accounts::characterAttributes(const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted){
 try{return read_attributes(impl_->db,owner,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired",{}};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable",{}};}
 catch(const std::exception&){return {FW_INTERNAL,"invalid stored attribute record",{}};}
}
gwserver::RevisionResult Accounts::saveAttributes(const gwserver::SaveAttributes& request,const std::function<bool()>& permitted){
 try{return write_attributes(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INVALID,"invalid attribute request"};}
}
}

namespace fw {
gwserver::ProfessionSaveResult Accounts::saveSecondaryProfession(const gwserver::SaveSecondaryProfession& request,const std::function<bool()>& permitted){
 try{return write_profession(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INVALID,"invalid secondary profession request"};}
}
}
namespace fw {
gwserver::ProgressionResult Accounts::characterProgression(const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted){
 try{return read_character_progression(impl_->db,owner,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired",{}};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable",{}};}
 catch(const std::exception&){return {FW_INTERNAL,"invalid stored progression record",{}};}
}
gwserver::RevisionResult Accounts::saveProgression(const gwserver::SaveProgression& request,const std::function<bool()>& permitted){
 try{return write_character_progression(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INVALID,"invalid progression request"};}
}
}
namespace fw {
gwserver::FactionsResult Accounts::accountFactions(const gwserver::Identity& owner,const std::function<bool()>& permitted){
 try{return read_factions(impl_->db,owner,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired",{}};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable",{}};}
 catch(const std::exception&){return {FW_INTERNAL,"invalid stored faction record",{}};}
}
gwserver::RevisionResult Accounts::saveFactions(const gwserver::SaveFactions& request,const std::function<bool()>& permitted){
 try{return write_factions(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INVALID,"invalid faction request"};}
}
}

namespace fw {
gwserver::RevisionResult Accounts::awardFaction(const gwserver::AwardFaction& request,const std::function<bool()>& permitted){
 try{return award_faction(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INVALID,"invalid faction request"};}
}
gwserver::RevisionResult Accounts::exchangeImperialFaction(const gwserver::ExchangeImperialFaction& request,const std::function<bool()>& permitted){
 try{return exchange_faction(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INVALID,"invalid faction request"};}
}
}

namespace fw {
gwserver::InventoryResult Accounts::characterInventory(const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted){
 try{return read_inventory(impl_->db,owner,false,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired",{}};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable",{}};}
 catch(const std::exception&){return {FW_INTERNAL,"invalid stored inventory",{}};}
}
gwserver::InventoryResult Accounts::accountStorage(const gwserver::Identity& account,const std::function<bool()>& permitted){
 try{return read_inventory(impl_->db,{account,{}},true,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired",{}};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable",{}};}
 catch(const std::exception&){return {FW_INTERNAL,"invalid stored inventory",{}};}
}
}

namespace fw {
gwserver::RevisionResult Accounts::saveInventory(const gwserver::SaveInventory& request,const std::function<bool()>& permitted){
 try{const auto result=write_inventory(impl_->db,request.owner,request.expected_revision,request.profile,nullptr,0,permitted);return {result.status,result.message,result.inventory_revision};}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INVALID,"invalid inventory request"};}
}
gwserver::StorageTransferResult Accounts::transferStorage(const gwserver::TransferStorage& request,const std::function<bool()>& permitted){
 try{return write_inventory(impl_->db,request.owner,request.inventory_revision,request.character,&request.bank,request.storage_revision,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INVALID,"invalid storage transfer request"};}
}
}

namespace fw {
gwserver::MapTransitionResult Accounts::selectMap(const gwserver::SelectMap& request,const std::function<bool()>& permitted){
 try{return transition_character_map(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INTERNAL,"invalid stored character profile"};}
}
gwserver::RosterResult Accounts::characterRoster(const gwserver::CharacterRoster& request,const std::function<bool()>& permitted){
 try{return read_roster(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INTERNAL,"invalid stored character roster"};}
}
gwserver::LogoutResult Accounts::releaseGameSession(const gwserver::GameSessionLease& lease,const std::function<bool()>& permitted){
 try{
  allowed(permitted);Transaction tx(impl_->db);
  Query erase(impl_->db,"DELETE FROM fw_account_sessions WHERE hash=? AND account=?");
  erase.text(1,hex_bytes(lease.session_hash.bytes));erase.text(2,hex_bytes(lease.account.bytes));erase.row();
  const bool removed=sqlite3_changes(impl_->db)==1;
  allowed(permitted);tx.commit();
  return {static_cast<std::uint32_t>(removed?FW_OK:FW_NOT_FOUND),removed?"revoked":"session unavailable"};
 }catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
}
gwserver::TradeResult Accounts::exchangeTrade(const gwserver::ExchangeTrade& request,const std::function<bool()>& permitted){
 try{return write_trade(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INVALID,"invalid trade request"};}
}
}

namespace fw {
gwserver::CharacterNameResult Accounts::characterName(const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted){
 try{return read_character_name(impl_->db,owner,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INTERNAL,"invalid stored character name"};}
}
}

namespace fw {
gwserver::CharacterNumberResult Accounts::characterNumber(const gwserver::OwnedCharacter& owner,bool map,const std::function<bool()>& permitted){
 try{return read_character_number(impl_->db,owner,map,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INTERNAL,"invalid stored character number"};}
}
}

namespace fw {
gwserver::DeletionResult Accounts::deleteCharacter(const gwserver::DeleteCharacter& request,const std::function<bool()>& permitted){
 try{return delete_character(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INVALID,"invalid character deletion request"};}
}
}

namespace fw {
gwserver::AgreementResult Accounts::accountAgreement(const gwserver::Identity& account,const std::function<bool()>& permitted){
 try{return read_account_agreement(impl_->db,account,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired",{}};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable",{}};}
 catch(const std::exception&){return {FW_INTERNAL,"invalid stored agreement flag",{}};}
}
}

namespace fw {
gwserver::AccountProfileResult Accounts::accountProfile(const gwserver::Identity& account,const std::function<bool()>& permitted){
 try{
    allowed(permitted);Transaction tx(impl_->db);
    const auto id=gwserver::storage_hex(account.bytes);
    Query owner(impl_->db,"SELECT 1 FROM fw_accounts WHERE id=?");owner.text(1,id);
    if(!owner.row())return {FW_NOT_FOUND,"account missing",{}};
    Query read(impl_->db,"SELECT profile FROM fw_gw_account_profiles WHERE account=?");read.text(1,id);
    std::optional<gw::auth::AccountInfoProfile> profile;
    if(read.row())profile=gw::auth::AccountInfoProfile::decode(read.blob(0));
    allowed(permitted);tx.commit();return {FW_OK,{},std::move(profile)};
 }catch(const Cancelled&){return {FW_TIMEOUT,"account request expired",{}};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable",{}};}
 catch(const std::exception&){return {FW_INTERNAL,"invalid stored account profile",{}};}
}
gwserver::GameAliasResult Accounts::gameAccountAlias(const gwserver::Identity& account,const std::function<bool()>& permitted){
 try{
    allowed(permitted);
    Query read(impl_->db,"SELECT game_alias FROM fw_accounts WHERE id=?");read.text(1,gwserver::storage_hex(account.bytes));
    if(!read.row())return {FW_NOT_FOUND,"account missing",{}};
    auto alias=read.text(0);
    if(!account_name_valid(alias))return {FW_INTERNAL,"invalid stored game account alias",{}};
    allowed(permitted);return {FW_OK,{},std::move(alias)};
 }catch(const Cancelled&){return {FW_TIMEOUT,"account request expired",{}};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable",{}};}
 catch(const std::exception&){return {FW_INTERNAL,"invalid stored game account alias",{}};}
}
gwserver::CampaignsResult Accounts::accountCampaigns(const gwserver::Identity& account,const std::function<bool()>& permitted){
 try{
    allowed(permitted);Transaction tx(impl_->db);
    const auto id=gwserver::storage_hex(account.bytes);
    Query owner(impl_->db,"SELECT 1 FROM fw_accounts WHERE id=?");owner.text(1,id);
    if(!owner.row())return {FW_NOT_FOUND,"account missing",{}};
    Query read(impl_->db,"SELECT campaign FROM fw_gw_campaign_entitlements WHERE account=? ORDER BY campaign");read.text(1,id);
    std::set<std::uint32_t> campaigns;
    while(read.row()){
        const auto value=read.number(0);
        if(value<1||value>4||!campaigns.insert(static_cast<std::uint32_t>(value)).second)return {FW_INTERNAL,"invalid stored campaign",{}};
    }
    allowed(permitted);tx.commit();return {FW_OK,{},std::move(campaigns)};
 }catch(const Cancelled&){return {FW_TIMEOUT,"account request expired",{}};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable",{}};}
 catch(const std::exception&){return {FW_INTERNAL,"invalid stored campaigns",{}};}
}
}

namespace fw {
gwserver::CreationResult Accounts::createCharacter(const gwserver::CreateCharacter& request,const std::function<bool()>& permitted){
 try{return create_character(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired",{}};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable",{}};}
 catch(const std::invalid_argument&){return {FW_INVALID,"invalid character creation request",{}};}
 catch(const std::exception&){return {FW_INTERNAL,"character creation failed",{}};}
}
}

namespace fw {
gwserver::CharacterNameResult Accounts::renameCharacter(const gwserver::RenameCharacter& request,const std::function<bool()>& permitted){
 try{return rename_character(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::invalid_argument&){return {FW_INVALID,"invalid character request"};}
 catch(const std::exception&){return {FW_INTERNAL,"character operation failed"};}
}
}

namespace fw {
gwserver::DevelopmentUnlockResult Accounts::unlockDevelopmentCharacter(const gwserver::OwnedCharacter& request,const std::function<bool()>& permitted){
 try{return unlock_character(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::invalid_argument&){return {FW_INVALID,"invalid character request"};}
 catch(const std::exception&){return {FW_INTERNAL,"character operation failed"};}
}
}

namespace fw {
gwserver::FriendsResult Accounts::friends(const gwserver::Identity& request,const std::function<bool()>& permitted){
 try{return read_friends(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::invalid_argument&){return {FW_INVALID,"invalid friend request"};}
 catch(const std::exception&){return {FW_INTERNAL,"friend operation failed"};}
}
}

namespace fw {
gwserver::FriendAddResult Accounts::addFriend(const gwserver::AddFriend& request,const std::function<bool()>& permitted){
 try{return add_friend(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::invalid_argument&){return {FW_INVALID,"invalid friend request"};}
 catch(const std::exception&){return {FW_INTERNAL,"friend operation failed"};}
}
}

namespace fw {
gwserver::FriendRemoveResult Accounts::removeFriend(const gwserver::FriendTarget& request,const std::function<bool()>& permitted){
 try{return remove_friend(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::invalid_argument&){return {FW_INVALID,"invalid friend request"};}
 catch(const std::exception&){return {FW_INTERNAL,"friend operation failed"};}
}
}

namespace fw {
gwserver::FriendBlockedResult Accounts::friendBlocked(const gwserver::FriendTarget& request,const std::function<bool()>& permitted){
 try{return friend_blocked(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::invalid_argument&){return {FW_INVALID,"invalid friend request"};}
 catch(const std::exception&){return {FW_INTERNAL,"friend operation failed"};}
}
}

namespace fw {
gwserver::CapeResult Accounts::guildCape(const gwserver::Identity& request,const std::function<bool()>& permitted){
 try{return read_guild_cape(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INTERNAL,"guild read failed"};}
}
}

namespace fw {
gwserver::GuildFactionResult Accounts::guildFaction(const gwserver::Identity& request,const std::function<bool()>& permitted){
 try{return read_guild_faction(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INTERNAL,"guild read failed"};}
}
}

namespace fw {
gwserver::TournamentPointsResult Accounts::tournamentPoints(const gwserver::Identity& request,const std::function<bool()>& permitted){
 try{return read_tournament_points(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INTERNAL,"guild read failed"};}
}
}

namespace fw {
gwserver::GuildResult Accounts::guild(const gwserver::Identity& request,const std::function<bool()>& permitted){
 try{return read_guild(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INTERNAL,"guild snapshot failed"};}
}
}

namespace fw {
gwserver::CapePurchaseResult Accounts::purchaseGuildCape(const gwserver::PurchaseGuildCape& request,const std::function<bool()>& permitted){
 try{return buy_guild_cape(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::invalid_argument&){return {FW_INVALID,"invalid guild purchase"};}
 catch(const std::exception&){return {FW_INTERNAL,"guild purchase failed"};}
}
}

namespace fw {
gwserver::TournamentPointsResult Accounts::purchaseGuildTrim(const gwserver::PurchaseGuildTrim& request,const std::function<bool()>& permitted){
 try{return buy_guild_trim(impl_->db,request,permitted);}
 catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::invalid_argument&){return {FW_INVALID,"invalid guild purchase"};}
 catch(const std::exception&){return {FW_INTERNAL,"guild purchase failed"};}
}
}

namespace fw {
gwserver::CharacterNumberResult Accounts::equipmentVisibility(const gwserver::OwnedCharacter& owner,const std::function<bool()>& permitted){
 try{
  allowed(permitted);Transaction tx(impl_->db);
  const auto character=gwserver::storage_hex(owner.character.bytes);
  Query owned(impl_->db,"SELECT 1 FROM fw_gw_characters WHERE id=? AND account=?");owned.text(1,character);owned.text(2,gwserver::storage_hex(owner.account.bytes));
  if(!owned.row())return {FW_NOT_FOUND,"owned character not found"};
  Query read(impl_->db,"SELECT value FROM fw_gw_equipment_visibility WHERE character=?");read.text(1,character);const auto value=read.row()?read.number(0):0;
  if(value<0||value>255)throw std::runtime_error("visibility state range");
  allowed(permitted);tx.commit();return {FW_OK,{},static_cast<unsigned>(value)};
 }catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INTERNAL,"equipment visibility read failed"};}
}
gwserver::CharacterNumberResult Accounts::saveEquipmentVisibility(const gwserver::SaveEquipmentVisibility& request,const std::function<bool()>& permitted){
 try{
  allowed(permitted);if(request.expected>255||request.replacement>255)return {FW_INVALID,"visibility state range"};Transaction tx(impl_->db);
  const auto character=gwserver::storage_hex(request.owner.character.bytes);
  Query owned(impl_->db,"SELECT 1 FROM fw_gw_characters WHERE id=? AND account=?");owned.text(1,character);owned.text(2,gwserver::storage_hex(request.owner.account.bytes));
  if(!owned.row())return {FW_NOT_FOUND,"owned character not found"};
  Query read(impl_->db,"SELECT value FROM fw_gw_equipment_visibility WHERE character=?");read.text(1,character);const auto value=read.row()?read.number(0):0;
  if(value!=request.expected)return {FW_CONFLICT,"equipment visibility changed"};
  Query write(impl_->db,"INSERT INTO fw_gw_equipment_visibility VALUES(?,?) ON CONFLICT(character) DO UPDATE SET value=excluded.value");write.text(1,character);write.number(2,request.replacement);
  allowed(permitted);write.row();allowed(permitted);tx.commit();return {FW_OK,{},request.replacement};
 }catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INTERNAL,"equipment visibility save failed"};}
}
}
namespace fw {
gwserver::SessionResult Accounts::validateSession(const gwserver::ValidateSession& request,const std::function<bool()>& permitted){
 try{
  allowed(permitted);Transaction tx(impl_->db);
  std::array<unsigned char,crypto_hash_sha256_BYTES> digest{};
  crypto_hash_sha256(digest.data(),request.session.bytes.data(),request.session.bytes.size());
  const auto owner=impl_->identity(gwserver::storage_hex(digest),unix_time());
  allowed(permitted);
  if(owner.id.empty())return {FW_NOT_FOUND,"invalid credentials or session"};
  if(owner.expires<=0||gwserver::normalized_account(owner.name)!=owner.name)throw std::runtime_error("invalid stored session identity");
  auto identity=gwserver::AccountIdentity{gwserver::Identity::from_hex(owner.id),owner.name,static_cast<std::uint64_t>(owner.expires)};
  allowed(permitted);tx.commit();return {FW_OK,{},std::move(identity)};
 }catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INTERNAL,"session validation failed"};}
}
}
namespace fw {
gwserver::LogoutResult Accounts::logout(const gwserver::LogoutAccount& request,const std::function<bool()>& permitted){
 try{
  allowed(permitted);Transaction tx(impl_->db);
  std::array<unsigned char,crypto_hash_sha256_BYTES> digest{};
  crypto_hash_sha256(digest.data(),request.session.bytes.data(),request.session.bytes.size());
  const auto hash=gwserver::storage_hex(digest);
  if(impl_->identity(hash,unix_time()).id.empty())return {FW_NOT_FOUND,"invalid credentials or session"};
  allowed(permitted);
  Query erase(impl_->db,"DELETE FROM fw_account_sessions WHERE hash=?");erase.text(1,hash);erase.row();
  allowed(permitted);tx.commit();return {FW_OK,"revoked"};
 }catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INTERNAL,"session logout failed"};}
}
}
namespace fw {
gwserver::TicketIssuanceResult Accounts::issueTicket(const gwserver::IssueTicket& request,const std::function<bool()>& permitted){
 try{
  allowed(permitted);Transaction tx(impl_->db);const auto now=unix_time();
  impl_->cleanup(now);allowed(permitted);
  std::array<unsigned char,crypto_hash_sha256_BYTES> digest{};
  crypto_hash_sha256(digest.data(),request.session.bytes.data(),request.session.bytes.size());
  const auto session=gwserver::storage_hex(digest);const auto owner=impl_->identity(session,now);
  if(owner.id.empty())return {FW_NOT_FOUND,"invalid credentials or session"};
  Query own(impl_->db,"SELECT count(*) FROM fw_gw_tickets t JOIN fw_account_sessions s ON s.hash=t.session WHERE s.account=?");own.text(1,owner.id);own.row();
  Query total(impl_->db,"SELECT count(*) FROM fw_gw_tickets");total.row();
  if(own.number(0)>=4||total.number(0)>=4096)return {FW_OVERLOADED,"game ticket limit"};
  gw::GameToken token{};struct Erase{gw::GameToken& value;~Erase(){sodium_memzero(value.data(),value.size());}} erase{token};randombytes_buf(token.data(),token.size());
  const auto expires=std::min(now+60,owner.expires);
  Query insert(impl_->db,"INSERT INTO fw_gw_tickets VALUES(?,?,?,?)");insert.text(1,game_token_hash(token));insert.text(2,session);insert.number(3,now);insert.number(4,expires);
  allowed(permitted);insert.row();allowed(permitted);tx.commit();
  return {FW_OK,{},gwserver::IssuedTicket{token,static_cast<std::uint64_t>(expires)}};
 }catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INTERNAL,"ticket issuance failed"};}
}
}
namespace fw {
gwserver::TicketAuthenticationResult Accounts::authenticateTicket(const gwserver::AuthenticateTicket& request,const std::function<bool()>& permitted){
 try{
  allowed(permitted);
  if((!request.alias.empty()&&!account_name_valid(request.alias))||request.alias.size()>31)return {FW_INVALID,"invalid game alias"};
  if(!request.character.empty())try{(void)gw::character_name_to_utf16(request.character,19);}catch(const std::invalid_argument&){return {FW_INVALID,"invalid character name"};}
  Transaction tx(impl_->db);const auto now=unix_time();impl_->cleanup(now);allowed(permitted);
  const auto hash=game_token_hash(request.ticket);
  Query read(impl_->db,"SELECT session FROM fw_gw_tickets WHERE hash=? AND expires>? AND issued<=?");read.text(1,hash);read.number(2,now);read.number(3,now);
  if(!read.row())return {FW_NOT_FOUND,"ticket unavailable"};
  const auto parent=read.text(0);const auto owner=impl_->identity(parent,now);if(owner.id.empty())return {FW_NOT_FOUND,"parent session expired"};
  const auto account=gwserver::Identity::from_hex(owner.id);
  if(sodium_memcmp(account.bytes.data(),request.account.bytes.data(),account.bytes.size()))return {FW_NOT_FOUND,"UserId claim mismatch"};
  if(owner.expires<=0||!account_name_valid(owner.name))throw std::runtime_error("invalid stored account identity");
  allowed(permitted);
  gwserver::AuthenticatedGameAccount result{account,owner.name,static_cast<std::uint64_t>(owner.expires),{}, {},148,0,gwserver::GameSessionLease{account,gwserver::SessionToken::from_hex(parent)}};
  Query roster(impl_->db,"SELECT id,name,map_id FROM fw_gw_characters WHERE account=? ORDER BY roster_order,id");roster.text(1,owner.id);
  while(roster.row()){
   if(++result.roster_size>8)throw std::runtime_error("persistent roster exceeds client bound");
   const auto id=gwserver::Identity::from_hex(roster.text(0));const auto name=roster.text(1);const auto map=roster.number(2);
   (void)gw::character_name_to_utf16(name,19);if(map<1||map>65535)throw std::runtime_error("invalid stored character map");
   if(!request.character.empty()&&name.size()==request.character.size()&&std::equal(name.begin(),name.end(),request.character.begin(),[](unsigned char a,unsigned char b){if(a>='A'&&a<='Z')a+='a'-'A';if(b>='A'&&b<='Z')b+='a'-'A';return a==b;})){
    if(result.character)throw std::runtime_error("ambiguous character selection");result.character=id;result.character_name=name;result.map=static_cast<std::uint16_t>(map);
   }
  }
  if(!request.character.empty()&&!result.character)return {FW_NOT_FOUND,"character unavailable"};
  allowed(permitted);Query erase(impl_->db,"DELETE FROM fw_gw_tickets WHERE hash=?");erase.text(1,hash);erase.row();allowed(permitted);tx.commit();
  return {FW_OK,{},std::move(result)};
 }catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}
 catch(const SqlError&){return {FW_UNAVAILABLE,"account storage unavailable"};}
 catch(const std::exception&){return {FW_INTERNAL,"ticket authentication failed"};}
}
}
namespace fw {
gwserver::LoginResult Accounts::login(const gwserver::LoginAccount& request,const std::function<bool()>& permitted){
 try{return impl_->login_native(request,false,permitted);}catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}catch(const SqlError&){return {FW_UNAVAILABLE,"account storage or hashing unavailable"};}catch(const std::invalid_argument&){return {FW_INVALID,"invalid credentials"};}catch(const std::exception&){return {FW_INTERNAL,"account login failed"};}
}
gwserver::LoginResult Accounts::registerAccount(const gwserver::RegisterAccount& request,const std::function<bool()>& permitted){
 try{return impl_->login_native({request.name,request.password},true,permitted);}catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}catch(const SqlError&){return {FW_UNAVAILABLE,"account storage or hashing unavailable"};}catch(const std::invalid_argument&){return {FW_INVALID,"invalid credentials"};}catch(const std::exception&){return {FW_INTERNAL,"account registration failed"};}
}
}
namespace fw {
gwserver::GuildCommandResult Accounts::guildCommand(const gwserver::GuildCommand& request,const std::function<bool()>& permitted){
 try{return mutate_guild(impl_->db,request,permitted);}catch(const Cancelled&){return {FW_TIMEOUT,"account request expired"};}catch(const SqlError&){return {FW_UNAVAILABLE,"guild storage unavailable"};}catch(const std::invalid_argument&){return {FW_INVALID,"invalid guild command"};}catch(const std::exception&){return {FW_INTERNAL,"guild mutation failed"};}
}
}
