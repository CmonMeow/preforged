#include "guild_state.h"
#include "guild_commands.h"
#include "cape_state.h"
#include "accounts.h"
#include "common.h"
#include "gw_character_profile.h"
#include "gw_inventory.h"
#include "gw_guild_cape.h"
#include "gw_guild_trim.h"
#include "gw_guild_faction.h"
#include <chrono>
#include <sodium.h>
#include <winsqlite/winsqlite3.h>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <span>

namespace {
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
std::string hex(std::span<const std::uint8_t> bytes){
    std::string value(bytes.size()*2+1,'\0');
    sodium_bin2hex(value.data(),value.size(),bytes.data(),bytes.size());value.pop_back();return value;
}
std::string hex(std::string_view text){return hex(std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size()));}
std::string unhex(const std::string& text){
    std::string value(text.size()/2,'\0');std::size_t count=0;
    check(!(text.size()%2)&&!sodium_hex2bin(reinterpret_cast<unsigned char*>(value.data()),value.size(),text.data(),text.size(),nullptr,&count,nullptr)&&count==value.size(),"social reply hex invalid");
    return value;
}
// Each run owns a fresh database under the CTest scratch directory. No live
// database, captured identity, or process is involved, including on failure.
struct Scratch {
    std::filesystem::path path;
    explicit Scratch(const char* directory){
        auto root=std::filesystem::absolute(directory);std::filesystem::create_directories(root);
        std::array<std::uint8_t,16> token{};randombytes_buf(token.data(),token.size());
        path=root/("social-"+hex(token)+".db");check(!std::filesystem::exists(path),"test database already exists");
    }
    ~Scratch(){for(const auto* suffix:{"","-wal","-shm","-journal"}){std::error_code ignored;std::filesystem::remove(path.string()+suffix,ignored);}}
};
struct FixtureDb {
    sqlite3* db=nullptr;
    explicit FixtureDb(const std::filesystem::path& path){
        check(sqlite3_open_v2(path.string().c_str(),&db,SQLITE_OPEN_READWRITE|SQLITE_OPEN_FULLMUTEX,nullptr)==SQLITE_OK,"fixture database open failed");
        check(sqlite3_exec(db,"PRAGMA foreign_keys=ON",nullptr,nullptr,nullptr)==SQLITE_OK,"fixture foreign keys");
    }
    ~FixtureDb(){if(db)sqlite3_close(db);}
    void write(const char* sql,const std::vector<std::string>& fields={}){
        sqlite3_stmt* statement=nullptr;check(sqlite3_prepare_v2(db,sql,-1,&statement,nullptr)==SQLITE_OK,"fixture prepare failed");
        struct Finalize{sqlite3_stmt* statement;~Finalize(){sqlite3_finalize(statement);}} guard{statement};
        for(std::size_t i=0;i<fields.size();++i)check(sqlite3_bind_text(statement,static_cast<int>(i+1),fields[i].data(),static_cast<int>(fields[i].size()),SQLITE_TRANSIENT)==SQLITE_OK,"fixture binding failed");
        check(sqlite3_step(statement)==SQLITE_DONE,"fixture write failed");
    }
    std::int64_t scalar(const char* sql,const std::vector<std::string>& fields={}){
        sqlite3_stmt* statement=nullptr;check(sqlite3_prepare_v2(db,sql,-1,&statement,nullptr)==SQLITE_OK,"fixture count prepare failed");
        struct Finalize{sqlite3_stmt* statement;~Finalize(){sqlite3_finalize(statement);}} guard{statement};
        for(std::size_t i=0;i<fields.size();++i)check(sqlite3_bind_text(statement,static_cast<int>(i+1),fields[i].data(),static_cast<int>(fields[i].size()),SQLITE_TRANSIENT)==SQLITE_OK,"fixture count binding failed");
        check(sqlite3_step(statement)==SQLITE_ROW,"fixture count failed");return sqlite3_column_int64(statement,0);
    }
};
struct Account {std::string id,login,character,name;};
struct Friend {unsigned type=0;std::string alias,character;};
struct Guild {std::string id,name,tag,announcement,author;unsigned hall=0;std::map<std::string,std::pair<std::string,unsigned>> members;};
class Test {
    std::filesystem::path database_;
    std::unique_ptr<fw::Accounts> accounts_;
public:
    explicit Test(const std::filesystem::path& path):database_(path),accounts_(std::make_unique<fw::Accounts>(path.string())){}
    void reopen(){accounts_.reset();accounts_=std::make_unique<fw::Accounts>(database_.string());}
    std::string call(const std::string& operation,const std::string& payload,std::uint32_t expected=FW_OK){
        const auto result=accounts_->execute(operation,payload,[]{return true;});
        if(result.status!=expected)throw std::runtime_error(operation+" returned "+std::to_string(result.status)+" expected "+std::to_string(expected));
        return result.payload;
    }
    void expires(const std::string& operation,const std::string& payload,unsigned admitted_checks){
        unsigned checks=0;const auto result=accounts_->execute(operation,payload,[&]{return ++checks<=admitted_checks;});
        if(result.status!=FW_TIMEOUT)throw std::runtime_error(operation+" failed to reject cancelled operation");
        check(checks==admitted_checks+1,"cancelled operation did not observe request deadline");
    }
    Account create(std::string login,std::string name){
        auto fields=fw::split(call("register",login+'\t'+hex("synthetic-social-password")));
        check(fields.size()==4,"synthetic registration shape");Account result{fields[0],std::move(login),{},std::move(name)};
        result.character=import(result.login,result.name);return result;
    }
    std::string import(const std::string& login,const std::string& name){
        fw::gw::CharacterProfile profile;profile.unknown0=0x87654321;profile.extension_count=37;profile.unknown1=8;
        const auto packed=profile.encode();const auto fields=fw::split(call("gw-character-profile-import-utf8",login+'\t'+hex(name)+'\t'+hex(packed)));
        check(fields.size()==3&&fields[1]==name,"synthetic import shape or name changed");return fields[0];
    }
    std::string command(const Account& account,const std::string& action,const std::string& argument={},std::uint32_t expected=FW_OK){
        const auto parsed=gwserver::guild_action(action);
        const auto result=accounts_->guildCommand({{gwserver::Identity::from_hex(account.id),gwserver::Identity::from_hex(account.character)},parsed.value_or(static_cast<gwserver::GuildAction>(999)),argument},[]{return true;});
        if(result.status!=expected)throw std::runtime_error("native guild "+action+" status"+std::to_string(result.status));return result.message;
    }
    void expire_command(const Account& account,const std::string& action,const std::string& argument,unsigned admitted_checks){
        const auto parsed=gwserver::guild_action(action);check(parsed.has_value(),"test action unavailable");unsigned checks=0;
        const auto result=accounts_->guildCommand({{gwserver::Identity::from_hex(account.id),gwserver::Identity::from_hex(account.character)},*parsed,argument},[&]{return ++checks<=admitted_checks;});
        check(result.status==FW_TIMEOUT&&checks==admitted_checks+1,"native guild ignored cancellation");
    }
    std::string add(const Account& owner,const std::string& name,unsigned type=1,const std::string& alias={},std::uint32_t expected=FW_OK){
        return call("gw-friend-add",owner.id+'\t'+std::to_string(type)+'\t'+hex(name)+'\t'+hex(alias),expected);
    }
    std::map<std::string,Friend> friends(const Account& owner){
        const auto fields=fw::split(call("gw-friend-list",owner.id));const auto count=fw::number(fields[0],110);
        check(fields.size()==1+count*4,"friend reply shape");std::map<std::string,Friend> result;
        for(std::size_t at=1;at<fields.size();at+=4)check(result.emplace(fields[at],Friend{static_cast<unsigned>(fw::number(fields[at+1],2)),unhex(fields[at+2]),unhex(fields[at+3])}).second,"duplicate friend target");
        return result;
    }
    Guild guild(const Account& owner){
        const auto fields=fw::split(call("gw-guild-state",owner.id));if(fields.size()==1){check(fields[0]=="0","empty guild shape");return {};}
        check(fields.size()>=7,"guild reply header");const auto count=fw::number(fields[6],100);check(fields.size()==7+count*3,"guild roster shape");
        Guild result{fields[0],unhex(fields[1]),unhex(fields[2]),unhex(fields[3]),unhex(fields[4]),static_cast<unsigned>(fw::number(fields[5],65535)),{}};
        for(std::size_t at=7;at<fields.size();at+=3)check(result.members.emplace(fields[at],std::pair{unhex(fields[at+1]),static_cast<unsigned>(fw::number(fields[at+2],2))}).second,"duplicate guild account");
        return result;
    }
};
std::string fixture_id(unsigned number){
    // Distinct, valid raw IDs exist only inside this test database.
    std::array<std::uint8_t,16> bytes{};bytes[0]=0xef;
    for(unsigned n=0;n<4;++n)bytes[n+1]=static_cast<std::uint8_t>(number>>(n*8));return hex(bytes);
}
}

int main(int argc,char** argv)try{
    check(argc==2,"social test scratch directory required");check(sodium_init()>=0,"sodium initialization failed");
    Scratch scratch(argv[1]);Test test(scratch.path);
    auto alice=test.create("alice@example.test","Alice Hero");
    auto bob=test.create("bob","Bob Hero");auto carol=test.create("carol","Carol Hero");
    auto dora=test.create("dora","Dora Hero");auto erin=test.create("erin","Erin Hero");
    check(test.friends(alice).empty()&&test.guild(alice).id.empty(),"opaque character capture invented official social state");
    test.call("gw-friend-list","bad",FW_INVALID);test.call("gw-friend-list",std::string(32,'f'),FW_NOT_FOUND);
    test.call("gw-friend-list",alice.id+"\textra",FW_INVALID);test.call("gw-friend-remove",alice.id+"\tbad",FW_INVALID);
    test.call("gw-guild-state",alice.id+"\textra",FW_INVALID);
    test.call("gw-guild-command",alice.id+'\t'+bob.character+"\tcreate\t"+hex("Stolen Guild | BAD"),FW_NOT_FOUND);
    test.add(alice,alice.name,1,{},FW_INVALID);test.add(alice,bob.name,0,{},FW_INVALID);test.add(alice,bob.name,3,{},FW_INVALID);
    test.add(alice,"bob",1,{},FW_NOT_FOUND);test.add(alice,"alice@example.test",1,{},FW_NOT_FOUND);
    test.add(alice,"bOb HeRo");
    auto list=test.friends(alice);check(list.size()==1&&list.at(bob.id).alias=="bOb HeRo"&&list.at(bob.id).character==bob.name,"friend target casing, default alias or display name wrong");
    check(list.at(bob.id).alias.find('@')==std::string::npos&&list.at(bob.id).character.find('@')==std::string::npos,"friend reply exposed account email");
    test.add(bob,alice.name);test.call("gw-friend-remove",alice.id+'\t'+alice.id);
    check(test.friends(bob).contains(alice.id),"another owner's removal erased a friendship");
    test.call("gw-friend-remove",alice.id+'\t'+bob.id);check(test.friends(alice).empty()&&test.friends(bob).size()==1,"friend removal crossed account boundary");
    test.add(alice,bob.name,2,"Case Safe Alias");check(test.call("gw-friend-blocked",alice.id+'\t'+bob.id)=="1"&&test.call("gw-friend-blocked",bob.id+'\t'+alice.id)=="0","ignore relation became reciprocal");
    test.add(alice,bob.name,1,"Alias Changed");list=test.friends(alice);
    check(list.size()==1&&list.at(bob.id).type==1&&list.at(bob.id).alias=="Alias Changed"&&test.call("gw-friend-blocked",alice.id+'\t'+bob.id)=="0","friend/ignore conversion duplicated or leaked relation");
    const auto before_duplicate=test.call("gw-friend-list",alice.id);
    test.add(alice,carol.name,1,"aLiAs ChAnGeD",FW_CONFLICT);
    test.add(alice,carol.name,2,"ALIAS CHANGED",FW_CONFLICT);
    check(test.call("gw-friend-list",alice.id)==before_duplicate,"duplicate alias changed another target/type");
    test.add(bob,carol.name,1,"Alias Changed");check(test.friends(bob).size()==2,"alias uniqueness wrongly crossed account ownership");
    test.add(alice,bob.name,1,"aLiAs ChAnGeD");check(test.friends(alice).at(bob.id).alias=="aLiAs ChAnGeD","same target cannot update alias case");
    const std::string unicode_alias="\xC3\x89"+std::string(18,'x');test.add(alice,bob.name,1,unicode_alias);
    check(test.friends(alice).at(bob.id).alias==unicode_alias,"UTF-16 bounded alias was counted as UTF-8 bytes");
    const auto before_bad=test.call("gw-friend-list",alice.id);
    for(const auto& invalid:{hex(std::string(20,'x')),std::string("0"),std::string("zz"),std::string("00"),std::string("0a"),std::string("eda080")})
        test.call("gw-friend-add",alice.id+"\t1\t"+hex(bob.name)+'\t'+invalid,FW_INVALID);
    check(test.call("gw-friend-list",alice.id)==before_bad,"invalid alias altered existing relation");
    test.import(bob.login,"Shared Hero");test.import(carol.login,"Shared Hero");
    test.add(alice,"Shared Hero",1,{},FW_NOT_FOUND);check(test.call("gw-friend-list",alice.id)==before_bad,"ambiguous character targeted a random account");
    test.expires("gw-friend-add",alice.id+"\t2\t"+hex(bob.name)+"\t",2);
    test.expires("gw-friend-remove",alice.id+'\t'+bob.id,2);
    check(test.call("gw-friend-list",alice.id)==before_bad,"cancelled friend mutation committed");
    for(const auto& operation:{"gw-friend-list","gw-guild-state"})test.expires(operation,alice.id,1);
    test.expires("gw-friend-blocked",alice.id+'\t'+bob.id,1);

    // Limit fixtures seed only synthetic accounts, avoiding 112 expensive and
    // irrelevant password hashes. Every list operation still uses Accounts.
    std::vector<Account> pool;pool.reserve(112);
    {
        FixtureDb fixture(scratch.path);fixture.write("BEGIN IMMEDIATE");
        for(unsigned i=0;i<112;++i){
            const auto id=fixture_id(i+1),character=fixture_id(i+1001),login="pool"+std::to_string(i),name="Pool Hero "+std::to_string(i);
            fixture.write("INSERT INTO fw_accounts(id,name,password,eula_accepted,game_alias) VALUES(?,?,'synthetic-unused-password',1,?)",{id,login,login});
            fixture.write("INSERT INTO fw_gw_characters(id,account,name) VALUES(?,?,?)",{character,id,name});pool.push_back({id,login,character,name});
        }
        fixture.write("COMMIT");
    }
    for(unsigned i=0;i<100;++i)test.add(erin,pool[i].name);
    test.add(erin,pool[100].name,1,{},FW_CONFLICT);test.add(erin,pool[0].name,1,"Full List Update");
    for(unsigned i=100;i<110;++i)test.add(erin,pool[i].name,2);
    check(test.friends(erin).size()==110,"friend/ignore caps did not admit exactly100+10");
    const auto full=test.call("gw-friend-list",erin.id);
    test.add(erin,pool[110].name,2,{},FW_CONFLICT);test.add(erin,pool[0].name,2,{},FW_CONFLICT);
    test.add(erin,pool[100].name,1,{},FW_CONFLICT);check(test.call("gw-friend-list",erin.id)==full,"full-list conversion erased previous type/alias");
    test.call("gw-friend-remove",erin.id+'\t'+pool[109].id);test.add(erin,pool[0].name,2);
    check(test.friends(erin).size()==109&&test.friends(erin).at(pool[0].id).type==2,"conversion after freed ignore slot lost target");

    test.command(alice,"join",{},FW_NOT_FOUND);test.command(alice,"leave",{},FW_NOT_FOUND);
    for(const auto& name:{"ab | OK","Valid Guild | A1","Valid Guild | ABCDE","Valid Guild","Valid Guild | TAG | EXTRA"})test.command(alice,"create",name,FW_INVALID);
    for(unsigned checks:{2u,3u})test.expire_command(alice,"create","Cancelled Guild | NONE",checks);
    check(test.guild(alice).id.empty(),"cancelled create committed a member");
    {FixtureDb fixture(scratch.path);check(fixture.scalar("SELECT count(*) FROM fw_gw_guilds")==0,"cancelled create left orphan guild");}
    check(test.command(alice,"create","  Private Guild | TEST  ").find("private server")!=std::string::npos,"guild creation falsely claimed official membership");
    auto guild=test.guild(alice);const auto guild_id=guild.id;
    using fw::gw::guild::decay_faction;
    check(fw::gw::guild::faction_day(86399)==0&&fw::gw::guild::faction_day(86400)==1,"midnight UTC boundary");
    check(decay_faction(10000,10,10).balance==10000,"same day taxed");
    check(decay_faction(10000,10,11).balance==9000&&decay_faction(10000,10,12).balance==8100,"daily compounding");
    check(decay_faction(10000,10,9).day==10,"backward clock rewound decay marker");
    check(decay_faction(0,10,100000).balance==0&&decay_faction(1,10,100000).balance==1,"empty/positive minimum");
    check(decay_faction(UINT32_MAX,10,11).balance==3865470565u,"decay overflow");
    check(test.call("gw-guild-faction-get",alice.id)=="0\t0","new guild faction default");
    const auto today=fw::gw::guild::faction_day(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    {FixtureDb fixture(scratch.path);fixture.write("INSERT INTO fw_gw_guild_faction VALUES(?,1,10000,?)",{guild_id,std::to_string(today-2)});}
    test.expires("gw-guild-faction-get",alice.id,1);
    {FixtureDb fixture(scratch.path);check(fixture.scalar("SELECT balance FROM fw_gw_guild_faction WHERE guild=?",{guild_id})==10000,"cancelled decay persisted");}
    {
        fw::Accounts direct(scratch.path.string());const auto owner=gwserver::Identity::from_hex(alice.id);unsigned checks=0;
        const auto cancelled=direct.guildFaction(owner,[&]{return ++checks<3;});
        check(cancelled.status==FW_TIMEOUT&&!cancelled.value,"native cancelled faction read exposed balance");
        FixtureDb fixture(scratch.path);check(fixture.scalar("SELECT balance FROM fw_gw_guild_faction WHERE guild=?",{guild_id})==10000,"native cancelled decay committed");
        const auto outsider=direct.guildFaction(gwserver::Identity::from_hex(bob.id),[]{return true;});
        check(outsider.status==FW_OK&&outsider.value->points==0,"native faction leaked to nonmember");
    }
    check(test.call("gw-guild-faction-get",bob.id)=="0\t0","nonmember faction leak");
    check(test.call("gw-guild-faction-get",alice.id)=="1\t8100","offline daily decay");
    test.reopen();check(test.call("gw-guild-faction-get",alice.id)=="1\t8100","restart double taxed");

    check(guild.name=="Private Guild"&&guild.tag=="TEST"&&guild.hall==0&&guild.members.size()==1&&guild.members.at(alice.id)==std::pair{alice.name,0u},"private guild identity/leader initialized incorrectly");
    {
     auto inventory=fw::gw::inventory::baseline();inventory.gold=5000;
     check(test.call("gw-character-inventory-save",alice.id+'\t'+alice.character+"\t0\t"+hex(fw::gw::inventory::encode(inventory)))=="1","cape fixture inventory revision");
     const auto design=hex(fw::gw::guild::cape::encode({240,177,101,3,10,0,35}));
     const auto buy=alice.id+'\t'+alice.character+"\t1\t2000\t"+design;
     check(test.call("gw-guild-cape-get",alice.id)=="0","new guild has unpurchased cape design");
     test.call("gw-guild-cape-buy",bob.id+'\t'+alice.character+"\t1\t2000\t"+design,FW_CONFLICT);
     test.call("gw-guild-cape-buy",alice.id+'\t'+alice.character+"\t1\t0\t"+design,FW_INVALID);
     auto earned=fw::gw::guild::cape::Design{240,177,101,3,10,1,35};
     test.call("gw-guild-cape-buy",alice.id+'\t'+alice.character+"\t1\t2000\t"+hex(fw::gw::guild::cape::encode(earned)),FW_CONFLICT);
     test.expires("gw-guild-cape-buy",buy,3);
     check(test.call("gw-guild-cape-get",alice.id)=="0","cancelled cape purchase committed design");
     {
        fw::Accounts direct(scratch.path.string());const gwserver::PurchaseGuildCape request{{gwserver::Identity::from_hex(alice.id),gwserver::Identity::from_hex(alice.character)},1,2000,{240,177,101,3,10,0,35}};
        unsigned checks=0;const auto cancelled=direct.purchaseGuildCape(request,[&]{return ++checks<4;});
        check(cancelled.status==FW_TIMEOUT&&cancelled.inventory_revision==0,"native cancelled cape purchase returned success data");
        check(!direct.guildCape(request.owner.account,[]{return true;}).value,"native cancelled cape purchase committed design");
        const auto unchanged=direct.characterInventory(request.owner,[]{return true;});
        check(unchanged.record&&unchanged.record->revision==1&&unchanged.record->profile.gold==5000,"native cancelled cape purchase debited gold or revision");
        const auto bought=direct.purchaseGuildCape(request,[]{return true;});
        check(bought.status==FW_OK&&bought.inventory_revision==2&&bought.gold==3000,"native cape price not atomically debited");
     }

     test.call("gw-guild-cape-buy",buy,FW_CONFLICT);
     test.reopen();check(test.call("gw-guild-cape-get",alice.id)==design,"cape design lost after database reopen");
     {
        fw::Accounts direct(scratch.path.string());const auto owner=gwserver::Identity::from_hex(alice.id);
        const auto native=direct.guildCape(owner,[]{return true;});
        check(native.status==FW_OK&&native.value&&hex(fw::gw::guild::cape::encode(*native.value))==design,"native cape bytes differ from offline adapter");
        const auto absent=direct.guildCape(gwserver::Identity::from_hex(bob.id),[]{return true;});
        check(absent.status==FW_OK&&!absent.value,"native cape leaked to nonmember");
        unsigned checks=0;const auto cancelled=direct.guildCape(owner,[&]{return ++checks<2;});
        check(cancelled.status==FW_TIMEOUT&&!cancelled.value,"native cancelled cape read exposed design");
     }

     auto poor=inventory;poor.gold=1999;
     check(test.call("gw-character-inventory-save",alice.id+'\t'+alice.character+"\t2\t"+hex(fw::gw::inventory::encode(poor)))=="3","cape poor fixture revision");
     test.call("gw-guild-cape-buy",alice.id+'\t'+alice.character+"\t3\t2000\t"+design,FW_CONFLICT);
     check(test.call("gw-guild-cape-get",alice.id)==design,"denied cape purchase mutated design");
     const auto trim_buy=alice.id+'\t'+alice.character+"\t1\t"+design;
     check(test.call("gw-guild-trim-balance",alice.id)=="0","new account granted tournament points");
     test.call("gw-guild-trim-buy",trim_buy,FW_CONFLICT);
     {FixtureDb fixture(scratch.path);fixture.write("INSERT INTO fw_gw_tournament_points VALUES(?,10000)",{alice.id});}
     {
        fw::Accounts direct(scratch.path.string());const auto owner=gwserver::Identity::from_hex(alice.id);
        check(direct.tournamentPoints(owner,[]{return true;}).value==10000,"native tournament points lost balance");
        check(direct.tournamentPoints(gwserver::Identity::from_hex(bob.id),[]{return true;}).value==0,"native tournament points leaked to another account");
        unsigned checks=0;const auto cancelled=direct.tournamentPoints(owner,[&]{return ++checks<2;});
        check(cancelled.status==FW_TIMEOUT&&!cancelled.value,"native cancelled points read exposed balance");
        check(direct.tournamentPoints(gwserver::Identity::from_hex(std::string(32,'0')),[]{return true;}).status==FW_NOT_FOUND,"native tournament read accepted absent owner");
     }

     test.call("gw-guild-trim-buy",bob.id+'\t'+alice.character+"\t1\t"+design,FW_CONFLICT);
     test.call("gw-guild-trim-buy",alice.id+'\t'+alice.character+"\t2\t"+design,FW_CONFLICT);
     test.call("gw-guild-trim-buy",alice.id+'\t'+alice.character+"\t1\t0000000000000000",FW_CONFLICT);
     for(unsigned checks:{2u,3u}){
       test.expires("gw-guild-trim-buy",trim_buy,checks);
       check(test.call("gw-guild-cape-get",alice.id)==design&&test.call("gw-guild-trim-balance",alice.id)=="10000","cancelled trim purchase committed one side");
     }
     {
        fw::Accounts direct(scratch.path.string());const gwserver::PurchaseGuildTrim request{{gwserver::Identity::from_hex(alice.id),gwserver::Identity::from_hex(alice.character)},1,{240,177,101,3,10,0,35}};
        unsigned checks=0;const auto cancelled=direct.purchaseGuildTrim(request,[&]{return ++checks<4;});
        check(cancelled.status==FW_TIMEOUT&&!cancelled.value,"native cancelled trim returned balance");
        const auto cape=direct.guildCape(request.owner.account,[]{return true;});
        check(cape.value&&hex(fw::gw::guild::cape::encode(*cape.value))==design&&direct.tournamentPoints(request.owner.account,[]{return true;}).value==10000,"native cancelled trim committed cape or points");
        check(direct.purchaseGuildTrim(request,[]{return true;}).value==2500,"native trim price not debited");
     }

     test.call("gw-guild-trim-buy",trim_buy,FW_CONFLICT);
     auto silver=fw::gw::guild::cape::decode(fw::gw::guild::cape::encode({240,177,101,3,10,1,35}));
     const auto silver_hex=hex(fw::gw::guild::cape::encode(silver));
     test.reopen();check(test.call("gw-guild-cape-get",alice.id)==silver_hex&&test.call("gw-guild-trim-balance",alice.id)=="2500","trim/balance not durable");
     test.call("gw-guild-trim-buy",alice.id+'\t'+alice.character+"\t1\t"+silver_hex,FW_CONFLICT);
     check(test.call("gw-guild-trim-buy",alice.id+'\t'+alice.character+"\t4\t"+silver_hex)=="0","exact-balance trim purchase rejected");
     check(test.call("gw-guild-trim-balance",alice.id)=="0","duplicate confirmation debited points twice");
     const auto before_gold=test.call("gw-guild-cape-get",alice.id);
     {FixtureDb fixture(scratch.path);fixture.write("UPDATE fw_gw_tournament_points SET balance=20000 WHERE account=?",{alice.id});}
     check(test.call("gw-guild-trim-buy",alice.id+'\t'+alice.character+"\t2\t"+before_gold)=="5000","gold trim must cost15000");
     auto gold=fw::gw::guild::cape::decode(fw::gw::guild::cape::encode({240,177,101,3,10,2,35}));
     check(test.call("gw-guild-cape-get",alice.id)==hex(fw::gw::guild::cape::encode(gold)),"gold purchase did not save trim2");
     test.call("gw-guild-trim-buy",alice.id+'\t'+alice.character+"\t2\t"+before_gold,FW_CONFLICT);
     check(test.call("gw-guild-trim-balance",alice.id)=="5000","repeated gold purchase debited points");
    }
    test.command(alice,"create","Other Guild | OTHR",FW_CONFLICT);
    test.command(dora,"create","private guild | NEW",FW_CONFLICT);test.command(dora,"create","Different Guild | test",FW_CONFLICT);
    test.command(dora,"create","Dora Guild | DORA");const auto dora_before=test.call("gw-guild-state",dora.id);
    test.command(alice,"kick",dora.name,FW_CONFLICT);check(test.call("gw-guild-state",dora.id)==dora_before,"foreign guild membership altered");
    test.expire_command(alice,"invite",bob.name,2);test.command(bob,"join",{},FW_NOT_FOUND);
    test.command(alice,"invite",bob.name);test.command(bob,"join");
    test.command(bob,"invite",carol.name,FW_CONFLICT);test.command(bob,"motd","Unauthorized",FW_CONFLICT);
    test.command(bob,"promote",alice.name,FW_CONFLICT);test.command(bob,"kick",alice.name,FW_CONFLICT);
    const auto before_promote=test.call("gw-guild-state",alice.id);test.expire_command(alice,"promote",bob.name,2);
    check(test.call("gw-guild-state",alice.id)==before_promote,"cancelled promotion changed rank");
    test.command(alice,"promote",bob.name);test.command(bob,"invite",carol.name);
    test.expire_command(carol,"join",{},2);check(test.guild(carol).id.empty(),"cancelled join partially committed");
    test.command(carol,"join");test.command(carol,"join",{},FW_CONFLICT);
    test.command(bob,"leader",carol.name,FW_CONFLICT);test.command(bob,"promote",carol.name,FW_CONFLICT);
    test.command(bob,"kick",alice.name,FW_CONFLICT);test.command(alice,"leader",alice.name,FW_NOT_FOUND);
    test.command(bob,"motd","UTF-8 \xC3\x89 announcement");guild=test.guild(alice);
    check(guild.announcement=="UTF-8 \xC3\x89 announcement"&&guild.author==bob.name,"announcement author or UTF-8 changed");
    {
        fw::Accounts direct(scratch.path.string());const auto owner=gwserver::Identity::from_hex(alice.id);
        const auto native=direct.guild(owner,[]{return true;});
        check(native.status==FW_OK&&native.value&&native.value->members.size()==3&&native.value->announcement==guild.announcement&&native.value->author==bob.name,"native guild snapshot lost roster or announcement");
        const auto foreign=direct.guild(gwserver::Identity::from_hex(dora.id),[]{return true;});
        check(foreign.status==FW_OK&&foreign.value&&foreign.value->id.bytes!=native.value->id.bytes,"native guild snapshot crossed ownership");
        unsigned checks=0;const auto cancelled=direct.guild(owner,[&]{return ++checks<3;});
        check(cancelled.status==FW_TIMEOUT&&!cancelled.value,"cancelled guild snapshot exposed partial roster");
        check(direct.guild(gwserver::Identity::from_hex(std::string(32,'0')),[]{return true;}).status==FW_NOT_FOUND,"native guild read accepted missing account");
        {
            FixtureDb fixture(scratch.path);fixture.write("UPDATE fw_gw_guild_members SET rank=0 WHERE account=?",{bob.id});
            const auto corrupt=direct.guild(owner,[]{return true;});
            check(corrupt.status==FW_INTERNAL&&!corrupt.value,"native guild read exposed ambiguous leadership");
            fixture.write("UPDATE fw_gw_guild_members SET rank=1 WHERE account=?",{bob.id});
        }

    }
    const auto unchanged=test.call("gw-guild-state",alice.id);
    test.command(alice,"motd",std::string(256,'x'),FW_INVALID);
    test.expire_command(alice,"motd","Cancelled announcement",2);
    test.command(alice,"unknown",{},FW_INVALID);test.command(alice,"leave",{},FW_CONFLICT);
    check(test.call("gw-guild-state",alice.id)==unchanged,"invalid/cancelled command changed guild state");
    for(unsigned checks:{2u,3u})test.expire_command(alice,"leader",carol.name,checks);
    check(test.call("gw-guild-state",alice.id)==unchanged,"cancelled leadership transfer partially changed ranks");
    {FixtureDb fixture(scratch.path);check(fixture.scalar("SELECT count(*) FROM fw_gw_guild_members WHERE guild=? AND rank=0",{guild_id})==1,"guild lost unique leader after transfer rollback");}
    test.command(alice,"leader",carol.name);guild=test.guild(alice);
    check(guild.members.at(carol.id).second==0&&guild.members.at(alice.id).second==1,"successful leadership transfer wrong ranks");
    test.command(alice,"demote",bob.name,FW_CONFLICT);
    const auto before_demote=test.call("gw-guild-state",carol.id);test.expire_command(carol,"demote",bob.name,2);
    check(test.call("gw-guild-state",carol.id)==before_demote,"cancelled demotion changed rank");test.command(carol,"demote",bob.name);
    test.command(bob,"motd","Rejected member change",FW_CONFLICT);
    const auto before_kick=test.call("gw-guild-state",carol.id);test.expire_command(alice,"kick",bob.name,2);
    check(test.call("gw-guild-state",carol.id)==before_kick,"cancelled removal changed membership");
    test.command(alice,"kick",bob.name);check(test.guild(bob).id.empty(),"officer could not remove lower ranked member");
    test.command(alice,"invite",bob.name);
    {FixtureDb fixture(scratch.path);fixture.write("UPDATE fw_gw_guild_invites SET expires=0 WHERE target=?",{bob.id});}
    test.command(bob,"join",{},FW_NOT_FOUND);test.command(alice,"invite",bob.name);test.command(bob,"join");

    // Full roster rejects the join atomically and preserves its invitation.
    test.command(carol,"invite",pool[111].name);
    {
        FixtureDb fixture(scratch.path);fixture.write("BEGIN IMMEDIATE");
        for(unsigned i=0;i<97;++i)fixture.write("INSERT INTO fw_gw_guild_members(account,guild,alias,rank) VALUES(?,?,?,2)",{pool[i].id,guild_id,pool[i].name});
        fixture.write("COMMIT");
    }
    {
        fw::Accounts direct(scratch.path.string());const auto native=direct.guild(gwserver::Identity::from_hex(carol.id),[]{return true;});
        check(native.status==FW_OK&&native.value&&native.value->members.size()==100,"native full guild roster truncated");
    }
    check(test.guild(carol).members.size()==100,"guild cap fixture size incorrect");test.command(pool[111],"join",{},FW_CONFLICT);
    check(test.guild(pool[111]).id.empty(),"full guild join partially committed");
    test.command(carol,"kick",pool[0].name);test.command(pool[111],"join");
    check(test.guild(carol).members.size()==100&&test.guild(carol).members.contains(pool[111].id),"full-guild refusal consumed invitation");
    test.command(erin,"create",std::string(32,'x')+" | X",FW_INVALID);
    test.command(erin,"create",std::string(31,'x')+" | X");test.command(erin,"motd",std::string(255,'m'));
    check(test.guild(erin).name.size()==31&&test.guild(erin).tag=="X"&&test.guild(erin).announcement.size()==255,"guild text boundary was truncated");
    test.command(erin,"leave");check(test.friends(erin).size()==109,"guild removal erased unrelated account friends");
    const std::string korean="\xec\x9d\x8fvs\xec\x9d\x8f";
    test.command(erin,"create","Unicode Guild | "+korean);
    check(test.guild(erin).tag==korean,"four UTF16 Korean guild tag preserved");test.command(erin,"leave");
    const auto identity_before=test.call("gw-guild-state",carol.id);
    const auto identity_request=carol.id+"\t"+guild_id+"\t"+hex(korean)+"\t"+hex(korean);
    test.call("gw-guild-identity-save",alice.id+"\t"+guild_id+"\t"+hex(korean)+"\t"+hex(korean),FW_CONFLICT);
    test.expires("gw-guild-identity-save",identity_request,2);
    check(test.call("gw-guild-state",carol.id)==identity_before,"cancelled guild identity change rolls back");
    test.call("gw-guild-identity-save",identity_request);
    check(test.guild(carol).name==korean&&test.guild(carol).tag==korean&&test.guild(carol).members.size()==100,"Unicode identity edit preserves guild membership");

    const auto friend_snapshot=test.call("gw-friend-list",alice.id),guild_snapshot=test.call("gw-guild-state",carol.id);
    test.reopen();check(test.call("gw-friend-list",alice.id)==friend_snapshot&&test.call("gw-guild-state",carol.id)==guild_snapshot&&test.call("gw-guild-state",dora.id)==dora_before,"social state changed across database reopen");
    // Character IDs/names belong to the roster; social relationships belong to
    // accounts. Removing a character never removes its account membership.
    test.call("gw-character-delete",dora.id+'\t'+bob.character,FW_NOT_FOUND);
    test.call("gw-character-delete",bob.id+'\t'+bob.character);
    check(test.guild(carol).members.at(bob.id).first==bob.name&&test.friends(alice).at(bob.id).alias==unicode_alias,"character deletion erased account social identity");
    test.command(bob,"leave",{},FW_NOT_FOUND);bob.character=test.import(bob.login,"Bob Replacement");bob.name="Bob Replacement";
    const auto before_leave=test.call("gw-guild-state",carol.id);test.expire_command(bob,"leave",{},2);
    check(test.call("gw-guild-state",carol.id)==before_leave,"cancelled member leave committed");test.command(bob,"leave");
    test.command(carol,"leave",{},FW_CONFLICT);test.command(dora,"invite",pool[110].name);
    test.expire_command(dora,"leave",{},2);check(test.call("gw-guild-state",dora.id)==dora_before,"cancelled sole-leader leave committed");
    test.command(dora,"leave");test.command(pool[110],"join",{},FW_NOT_FOUND);
    check(test.guild(dora).id.empty(),"sole-leader guild was not dissolved");
    {FixtureDb fixture(scratch.path);check(fixture.scalar("SELECT count(*) FROM fw_gw_guild_invites WHERE target=?",{pool[110].id})==0,"dissolved guild left invitation");}
    test.reopen();check(test.guild(dora).id.empty()&&test.guild(carol).members.size()==99&&test.friends(erin).size()==109,"final social state did not persist");
    std::cout<<"PASS social ownership, character-name resolution, UTF-16 bounds, friend/ignore limits, private guild roles, invite expiry, atomic leader transfer, cancellation, reopen and character deletion preservation\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
