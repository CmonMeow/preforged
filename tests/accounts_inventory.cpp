#include "accounts.h"
#include "network.h"
#include "network_storage_adapter.h"
#include "common.h"
#include "framework/module.h"
#include "gw_storage.h"
#include "gw_inventory_identity.h"
#include "gw_inventory_gold.h"
#include "gw_summary_2022.h"
#include <sodium.h>
#include <winsqlite/winsqlite3.h>
#include <filesystem>
#include <iostream>
#include <fstream>

using namespace fw;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::string hex(std::span<const std::uint8_t> bytes){std::string value(bytes.size()*2+1,'\0');sodium_bin2hex(value.data(),value.size(),bytes.data(),bytes.size());value.pop_back();return value;}
std::string encoded(const gw::inventory::Profile& profile){return hex(gw::inventory::encode(profile));}
void test_sql(const std::filesystem::path& path,const char* sql){sqlite3* connection=nullptr;check(sqlite3_open(path.string().c_str(),&connection)==SQLITE_OK,"test-owned SQLite connection");const auto status=sqlite3_exec(connection,sql,nullptr,nullptr,nullptr);sqlite3_close(connection);check(status==SQLITE_OK,"test-owned fixture SQL");}
unsigned count_inventory(const std::filesystem::path& path,const std::string& character){sqlite3* connection=nullptr;check(sqlite3_open_v2(path.string().c_str(),&connection,SQLITE_OPEN_READONLY,nullptr)==SQLITE_OK,"readonly cascade verification");sqlite3_stmt* statement=nullptr;check(sqlite3_prepare_v2(connection,"SELECT count(*) FROM fw_gw_inventory WHERE character=?",-1,&statement,nullptr)==SQLITE_OK,"cascade count query");sqlite3_bind_text(statement,1,character.c_str(),-1,SQLITE_TRANSIENT);check(sqlite3_step(statement)==SQLITE_ROW,"cascade count result");auto value=sqlite3_column_int(statement,0);sqlite3_finalize(statement);sqlite3_close(connection);return static_cast<unsigned>(value);}
}
int main(int argc,char** argv)try{
 check(argc==2,"dedicated inventory-accounts scratch directory required");check(sodium_init()>=0,"sodium initialization");
 const auto directory=std::filesystem::absolute(argv[1]).lexically_normal();check(directory.filename()=="inventory-accounts","test-owned directory name required");std::filesystem::create_directories(directory);const auto database=directory/"state.db";
 for(const auto suffix:{"","-wal","-shm","-journal"}){std::error_code error;std::filesystem::remove(database.string()+suffix,error);check(!error,"reset test-owned database");}
 std::string alice,bob,a,b,bob_character,final_bank;
 {
 Accounts accounts(database.string());auto call=[&](const std::string& operation,const std::string& payload,std::uint32_t expected=FW_OK){const auto result=accounts.execute(operation,payload,[]{return true;});if(result.status!=expected)throw std::runtime_error(operation+" returned "+std::to_string(result.status)+" expected "+std::to_string(expected));return result.payload;};
 const std::string password="inventory-fixture-password";const auto password_hex=hex({reinterpret_cast<const std::uint8_t*>(password.data()),password.size()});
 alice=split(call("register","InventoryAlice\t"+password_hex))[0];bob=split(call("register","InventoryBob\t"+password_hex))[0];
 a=split(call("gw-character-create",alice+"\tInventory Alpha"))[0];b=split(call("gw-character-create",alice+"\tInventory Beta"))[0];bob_character=split(call("gw-character-create",bob+"\tInventory Other"))[0];
 const auto owned=alice+"\t"+a;check(call("gw-character-inventory-get",owned)=="0","owned uncaptured inventory absent");check(call("gw-account-storage-get",alice)=="0","uncaptured account bank absent");call("gw-character-inventory-get",bob+"\t"+a,FW_NOT_FOUND);
 auto initial=gw::inventory::baseline();gw::inventory::Item item;item.definition.id=1;item.definition.file=1234;item.definition.type=11;item.definition.model=567;item.definition.quantity=100;item.definition.interaction=0x80000;item.definition.encoded_name=u"Synthetic fixture";item.definition.modifiers={0x12345678};item.bag=1;initial.items.emplace(1,item);initial.next_item=2;
 check(call("gw-character-inventory-save",owned+"\t0\t"+encoded(initial))=="1","first durable inventory revision");const auto original=call("gw-character-inventory-get",owned);check(original=="1\t"+encoded(initial),"stored opaque item definition roundtrip");
 const gwserver::OwnedCharacter typed_owner{gwserver::Identity::from_hex(alice),gwserver::Identity::from_hex(a)};
 const auto typed_inventory=accounts.characterInventory(typed_owner,[]{return true;});
 check(typed_inventory.status==FW_OK&&typed_inventory.record&&typed_inventory.record->revision==1&&encoded(typed_inventory.record->profile)==encoded(initial),"typed inventory exact profile");
 check(accounts.characterInventory({gwserver::Identity::from_hex(bob),typed_owner.character},[]{return true;}).status==FW_NOT_FOUND,"typed foreign inventory denied");
 check(accounts.characterInventory(typed_owner,[]{return false;}).status==FW_TIMEOUT,"typed inventory deadline");
 check(accounts.accountStorage(typed_owner.account,[]{return true;}).status==FW_OK&&!accounts.accountStorage(typed_owner.account,[]{return true;}).record,"typed missing bank");
 check(accounts.accountStorage({},[]{return true;}).status==FW_NOT_FOUND,"typed missing bank account denied");
 check(accounts.accountStorage(typed_owner.account,[]{return false;}).status==FW_TIMEOUT,"typed bank deadline");
 call("gw-character-inventory-save",bob+"\t"+a+"\t1\t"+encoded(initial),FW_NOT_FOUND);call("gw-character-inventory-save",owned+"\t0\t"+encoded(gw::inventory::baseline()),FW_CONFLICT);check(call("gw-character-inventory-get",owned)==original,"stale and foreign save preserve inventory");
 unsigned typed_save_checks=0;
 check(accounts.saveInventory({typed_owner,1,initial},[&]{return ++typed_save_checks<3;}).status==FW_TIMEOUT&&typed_save_checks==3,"typed inventory final precommit cancellation");
 check(accounts.characterInventory(typed_owner,[]{return true;}).record->revision==1,"typed inventory save rollback");
 unsigned typed_transfer_checks=0;
 check(accounts.transferStorage({typed_owner,1,0,initial,gw::storage::baseline()},[&]{return ++typed_transfer_checks<3;}).status==FW_TIMEOUT&&typed_transfer_checks==3,"typed transfer final precommit cancellation");
 check(accounts.characterInventory(typed_owner,[]{return true;}).record->revision==1&&!accounts.accountStorage(typed_owner.account,[]{return true;}).record,"typed transfer rolls back inventory and initial bank");
 auto bank=gw::storage::baseline();auto proposal=gw::storage::transfer(initial,bank,{gw::inventory::Change::split,1,25,100,0},{true,true,true});check(proposal.accepted(),"deposit proposal");
 const auto transfer=owned+"\t1\t0\t"+encoded(proposal.character)+"\t"+encoded(proposal.bank);check(call("gw-storage-transfer-save",transfer)=="2\t1","atomic deposit commits both revisions");
 const auto char_before=call("gw-character-inventory-get",owned),bank_before=call("gw-account-storage-get",alice);check(bank_before=="1\t"+encoded(proposal.bank),"bank bound to account");check(call("gw-account-storage-get",bob)=="0","different account bank isolated");
 call("gw-storage-transfer-save",owned+"\t2\t0\t"+encoded(initial)+"\t"+encoded(bank),FW_CONFLICT);call("gw-storage-transfer-save",owned+"\t1\t1\t"+encoded(initial)+"\t"+encoded(bank),FW_CONFLICT);
 call("gw-storage-transfer-save",bob+"\t"+a+"\t2\t1\t"+encoded(initial)+"\t"+encoded(bank),FW_NOT_FOUND);
 call("gw-storage-transfer-save",owned+"\t2\t1\t"+encoded(initial)+"\t"+encoded(bank)+"00",FW_INVALID);
 auto duplicate=proposal.bank;duplicate.items.clear();item.definition.quantity=25;item.bag=100;duplicate.items.emplace(1,item);duplicate.next_item=3;duplicate.bags.at(100).type=1;duplicate.bags.at(100).index=0;
 call("gw-storage-transfer-save",owned+"\t2\t1\t"+encoded(proposal.character)+"\t"+encoded(duplicate),FW_INVALID);
 check(call("gw-character-inventory-get",owned)==char_before&&call("gw-account-storage-get",alice)==bank_before,"invalid/stale/foreign transfer changes neither side");
 unsigned checkpoints=0;auto cancelled=accounts.execute("gw-storage-transfer-save",owned+"\t2\t1\t"+encoded(initial)+"\t"+encoded(bank),[&]{return ++checkpoints<3;});check(cancelled.status==FW_TIMEOUT&&checkpoints==3,"deadline expires at final precommit checkpoint");
 check(call("gw-character-inventory-get",owned)==char_before&&call("gw-account-storage-get",alice)==bank_before,"deadline rolls back both already-written records");
 test_sql(database,"CREATE TRIGGER inventory_fixture_storage_failure BEFORE UPDATE ON fw_gw_storage BEGIN SELECT RAISE(ABORT,'fixture storage write failure'); END;");
 call("gw-storage-transfer-save",owned+"\t2\t1\t"+encoded(initial)+"\t"+encoded(bank),FW_UNAVAILABLE);
 check(call("gw-character-inventory-get",owned)==char_before&&call("gw-account-storage-get",alice)==bank_before,"second record SQL failure rolls back first inventory write");test_sql(database,"DROP TRIGGER inventory_fixture_storage_failure;");
 auto withdrawal=gw::storage::transfer(gw::inventory::baseline(),proposal.bank,{gw::inventory::Change::move,2,0,1,0},{true,true,true});check(withdrawal.accepted(),"same account alternate character withdrawal");check(call("gw-storage-transfer-save",alice+"\t"+b+"\t0\t1\t"+encoded(withdrawal.character)+"\t"+encoded(withdrawal.bank))=="1\t2","account shared bank moves into alternate inventory");
 check(call("gw-character-inventory-get",owned)==char_before,"alternate withdrawal leaves first character inventory intact");check(call("gw-account-storage-get",alice)=="2\t"+encoded(withdrawal.bank),"alternate sees same account storage revision");
 check(count_inventory(database,b)==1,"alternate durable inventory exists before deletion");call("gw-character-delete",alice+"\t"+b);call("gw-character-inventory-get",alice+"\t"+b,FW_NOT_FOUND);check(count_inventory(database,b)==0,"character deletion cascades physical inventory row");check(call("gw-account-storage-get",alice)=="2\t"+encoded(withdrawal.bank),"character deletion preserves account bank");
 auto second=gw::storage::transfer(proposal.character,withdrawal.bank,{gw::inventory::Change::split,1,25,100,0},{true,true,true});check(second.accepted(),"second deposit before deletion");check(call("gw-storage-transfer-save",owned+"\t2\t2\t"+encoded(second.character)+"\t"+encoded(second.bank))=="3\t3","shared bank subsequent CAS");final_bank="3\t"+encoded(second.bank);call("gw-character-delete",owned);check(call("gw-account-storage-get",alice)==final_bank,"deletion cannot destroy deposited account property");
 }
 {
 Accounts reopened(database.string());auto result=reopened.execute("gw-account-storage-get",alice,[]{return true;});check(result.status==FW_OK&&result.payload==final_bank,"account bank survives connection restart");result=reopened.execute("gw-character-inventory-get",alice+"\t"+a,[]{return true;});check(result.status==FW_NOT_FOUND,"deleted owned character remains unavailable after restart");result=reopened.execute("gw-character-inventory-save",alice+"\t"+a+"\t0\t"+encoded(gw::inventory::baseline()),[]{return true;});check(result.status==FW_NOT_FOUND,"inventory save cannot resurrect deleted character");
 }
 {
 auto recipe=gw::inventory::baseline();recipe.next_item=3;
 gw::inventory::Item source;source.definition.id=1;source.definition.file=1234;source.definition.type=27;source.definition.model=567;source.definition.encoded_name=u"Synthetic catalog fixture";source.bag=1;
 auto revealed=source;revealed.definition.id=2;revealed.definition.interaction|=1;revealed.definition.price=100;revealed.slot=1;
 recipe.items={{1,source},{2,revealed}};const auto catalog_path=directory/"identification.txt";
 {std::ofstream file(catalog_path);file<<std::string(64,'b')<<'\t'<<encoded(recipe)<<'\n';}
 gw::items::CatalogPaths paths;paths.identification=catalog_path;
 gwserver::NetworkManager manager(database.string(),paths); TestStorageAdapter adapter(manager);
 check(manager.item_catalogs().identification().size()==1&&manager.item_catalogs().salvage().size()==0,"manager loads configured and unconfigured providers once");
 {std::ofstream file(catalog_path);file<<"broken catalog";}
 check(manager.item_catalogs().identification().reveal(source).has_value(),"file mutation cannot change active host catalog snapshot");
 std::filesystem::remove(catalog_path);
 check(manager.item_catalogs().identification().reveal(source).has_value(),"session provider survives source file removal");
 const auto payload=bob+"\t"+bob_character+"\t0\t0\t"+encoded(gw::inventory::baseline())+"\t"+encoded(gw::storage::baseline());
 auto result=adapter.request(static_cast<gwserver::Operation>(9999),payload,3000);check(result.status==FW_NOT_FOUND,"invalid operation cannot reach the storage transaction");
 result=adapter.request(gwserver::Operation::maintenanceGwAccountProfileSet,payload,3000);check(result.status==FW_NOT_FOUND,"offline maintenance cannot enter the game operation contract");
 result=adapter.request(gwserver::Operation::transferStorage,payload,3000);check(result.status==FW_OK&&result.payload=="1\t1","NetworkManager routes owned storage transfer through Accounts");
 result=adapter.request(gwserver::Operation::accountStorage,bob,3000);check(result.status==FW_OK&&result.payload=="1\t"+encoded(gw::storage::baseline()),"NetworkManager account storage get resolves same durable bank");
 result=adapter.request(gwserver::Operation::transferStorage,payload,3000);check(result.status==FW_CONFLICT,"NetworkManager storage route preserves CAS rejection");
 }
 {
 Accounts accounts(database.string());auto character=gw::inventory::baseline();auto bank=gw::storage::baseline();
 gw::inventory::Item item;item.definition.id=1;item.definition.file=1234;item.definition.type=11;item.definition.quantity=1;item.bag=1;
 character.items.emplace(1,item);character.next_item=2;character.gold=1000;item.bag=100;item.definition.model=456;bank.items.emplace(1,item);bank.next_item=2;bank.gold=2000;
 const auto payload=bob+"\t"+bob_character+"\t1\t1\t"+encoded(character)+"\t"+encoded(bank);
 auto result=accounts.execute("gw-storage-transfer-save",payload,[]{return true;});check(result.status==FW_OK&&result.payload=="2\t2","same numeric identity allowed in separate durable ownership domains");
 check(accounts.execute("gw-character-inventory-get",bob+"\t"+bob_character,[]{return true;}).payload=="2\t"+encoded(character),"overlapping ID preserves exact character item");
 check(accounts.execute("gw-account-storage-get",bob,[]{return true;}).payload=="2\t"+encoded(bank),"overlapping ID preserves distinct bank item");
 gw::inventory::identity::Session ids;auto wire=ids.hydrate(character,bank);
 const auto source=*ids.wire_item(gw::inventory::identity::Domain::character,1);
 const auto destination=*ids.wire_bag(gw::inventory::identity::Domain::bank,100);
 auto moved=gw::storage::transfer(wire.character,wire.bank,{gw::inventory::Change::move,source,0,destination,1},{true,true,true});
 check(moved.accepted(),"native deposit works with overlapping durable identities");
 auto prepared=ids.prepare(moved.character,moved.bank);
 const auto save=bob+"\t"+bob_character+"\t2\t2\t"+encoded(prepared.durable.character)+"\t"+encoded(prepared.durable.bank);
 const auto before=ids.revision();result=accounts.execute("gw-storage-transfer-save",bob+"\t"+bob_character+"\t1\t2\t"+encoded(prepared.durable.character)+"\t"+encoded(prepared.durable.bank),[]{return true;});
 check(result.status==FW_CONFLICT&&ids.revision()==before,"failed database CAS leaves session mapping uncommitted");
 result=accounts.execute("gw-storage-transfer-save",save,[]{return true;});check(result.status==FW_OK&&ids.commit(std::move(prepared)),"both durable records commit before session mapping");
 check(prepared.durable.character.items.empty()&&prepared.durable.bank.items.size()==2&&prepared.durable.bank.items.at(1).definition.model==456&&prepared.durable.bank.items.at(2).definition.model==0,"deposit allocates new bank identity without overwriting existing item");
 check(accounts.execute("gw-account-storage-get",bob,[]{return true;}).payload=="3\t"+encoded(prepared.durable.bank),"projected bank persisted exactly");
 gw::inventory::identity::Session reconnect;auto restored=reconnect.hydrate(prepared.durable.character,prepared.durable.bank);
 check(restored.bank.items.size()==2&&restored.character.items.empty(),"reconnect retains both items after colliding identity deposit");
 auto gold=gw::inventory::gold::propose({restored.character.gold,restored.bank.gold},{500,2500},{true,true,true});
 check(gold.accepted()&&gold.amount==500,"gold transfer conserves persisted character and bank balance");
 restored.character.gold=gold.balances.carried;restored.bank.gold=gold.balances.stored;auto gold_save=reconnect.prepare(restored.character,restored.bank);
 const auto gold_payload=bob+"\t"+bob_character+"\t3\t3\t"+encoded(gold_save.durable.character)+"\t"+encoded(gold_save.durable.bank);
 result=accounts.execute("gw-storage-transfer-save",gold_payload,[]{return true;});check(result.status==FW_OK&&result.payload=="4\t4"&&reconnect.commit(std::move(gold_save)),"gold commits both records atomically before session state");
 check(accounts.execute("gw-character-inventory-get",bob+"\t"+bob_character,[]{return true;}).payload=="4\t"+encoded(gold_save.durable.character)&&accounts.execute("gw-account-storage-get",bob,[]{return true;}).payload=="4\t"+encoded(gold_save.durable.bank),"gold and possessions persist together in FWI4");
 }
 {
 // Actual material proposals commit two owned records under the same CAS as
 // ordinary storage. Fixtures are synthetic; no official possessions granted.
 Accounts accounts(database.string());auto call=[&](const std::string& operation,const std::string& payload,std::uint32_t expected=FW_OK){const auto result=accounts.execute(operation,payload,[]{return true;});check(result.status==expected,"material Accounts operation result");return result.payload;};
 const std::string password="material-fixture-password";const auto account=split(call("register","InventoryMaterial\t"+hex({reinterpret_cast<const std::uint8_t*>(password.data()),password.size()})))[0];const auto character_id=split(call("gw-character-create",account+"\tMaterial Fixture"))[0],owned=account+"\t"+character_id;
 auto character=gw::inventory::baseline(),bank=gw::storage::baseline();character.gold=12345;bank.gold=543210;bank.material_quota=2500;
 gw::inventory::Item bone;bone.definition.id=1;bone.definition.file=1234;bone.definition.type=11;bone.definition.model=921;bone.definition.quantity=250;bone.definition.interaction=0x80000;bone.definition.modifiers={0x25080000};bone.profession=6;bone.customized=u"Exact material fixture";bone.bag=1;character.items.emplace(1,bone);
 gw::inventory::Item container;container.definition.id=2;container.definition.file=1234;container.definition.type=3;container.definition.quantity=1;container.profession=6;character.items.emplace(2,container);character.bags.at(1).container=2;
 auto weapon=container;weapon.definition.id=3;weapon.definition.type=5;weapon.bag=1;weapon.slot=1;character.items.emplace(3,weapon);character.weapons.sets[0].weapon=3;character.weapons.sets[3].weapon=3;character.weapons.active=3;character.next_item=4;
 auto stored=bone;stored.definition.quantity=2250;stored.bag=101;stored.slot=0;bank.items.emplace(1,stored);bank.next_item=2;
 check(call("gw-storage-transfer-save",owned+"\t0\t0\t"+encoded(character)+"\t"+encoded(bank))=="1\t1","material initial reviewed snapshot persists account quota/type5");
 const auto initial_character=call("gw-character-inventory-get",owned),initial_bank=call("gw-account-storage-get",account);
 auto wrong_domain=gw::storage::baseline();wrong_domain.bags.erase(100);wrong_domain.gold=0;call("gw-character-inventory-save",owned+"\t1\t"+encoded(wrong_domain),FW_INVALID);call("gw-storage-transfer-save",owned+"\t1\t1\t"+encoded(wrong_domain)+"\t"+encoded(bank),FW_INVALID);
 auto wrong_quota=character;wrong_quota.material_quota=500;call("gw-character-inventory-save",owned+"\t1\t"+encoded(wrong_quota),FW_INVALID);
 check(call("gw-character-inventory-get",owned)==initial_character&&call("gw-account-storage-get",account)==initial_bank,"material bank/quota cannot be forged into character domain");
 gw::inventory::identity::Session ids;auto wire=ids.hydrate(character,bank);const auto source=*ids.wire_item(gw::inventory::identity::Domain::character,1),target=*ids.wire_item(gw::inventory::identity::Domain::bank,1);const auto material_bag=*ids.wire_bag(gw::inventory::identity::Domain::bank,101);
 auto deposit=gw::storage::transfer(wire.character,wire.bank,{gw::inventory::Change::move,source,0,material_bag,0},{true,true,true});check(deposit.accepted(),"material250 merge proposal uses distinct session item1 identities");auto prepared=ids.prepare(deposit.character,deposit.bank);const auto projection=encoded(prepared.durable.character)+"\t"+encoded(prepared.durable.bank),payload=owned+"\t1\t1\t"+projection;const auto revision=ids.revision();
 call("gw-storage-transfer-save",owned+"\t1\t0\t"+projection,FW_CONFLICT);call("gw-storage-transfer-save",owned+"\t0\t1\t"+projection,FW_CONFLICT);call("gw-storage-transfer-save",bob+"\t"+character_id+"\t1\t1\t"+projection,FW_NOT_FOUND);
 unsigned checkpoints=0;const auto cancelled=accounts.execute("gw-storage-transfer-save",payload,[&]{return ++checkpoints<3;});check(cancelled.status==FW_TIMEOUT&&checkpoints==3,"material timeout occurs after both writes before commit");
 test_sql(database,"CREATE TRIGGER material_fixture_storage_failure BEFORE UPDATE ON fw_gw_storage BEGIN SELECT RAISE(ABORT,'material fixture storage write failure'); END;");call("gw-storage-transfer-save",payload,FW_UNAVAILABLE);test_sql(database,"DROP TRIGGER material_fixture_storage_failure;");
 check(call("gw-character-inventory-get",owned)==initial_character&&call("gw-account-storage-get",account)==initial_bank&&ids.revision()==revision&&ids.item(source)==gw::inventory::identity::Origin{gw::inventory::identity::Domain::character,1},"stale/foreign/timeout/SQLfailure rolls back both material records and session owner mapping");
 check(call("gw-storage-transfer-save",payload)=="2\t2"&&ids.commit(std::move(prepared)),"material merge commits both records before session registry");
 check(prepared.durable.character.items.size()==2&&prepared.durable.character.items.at(2).bag==0&&prepared.durable.character.weapons==character.weapons&&prepared.durable.bank.items.at(1).definition.quantity==2500&&prepared.durable.bank.material_quota==2500&&prepared.durable.character.gold==12345&&prepared.durable.bank.gold==543210,"material merge preserves actual installed container/sets/gold and account-owned capacity");
 wire=std::move(prepared.wire);const auto carry_bag=*ids.wire_bag(gw::inventory::identity::Domain::character,1);auto withdrawal=gw::storage::transfer(wire.character,wire.bank,{gw::inventory::Change::move,target,0,carry_bag,0},{true,true,true});check(withdrawal.accepted(),"material whole-stack withdrawal produces bounded carrying split");auto withdrawn=ids.prepare(withdrawal.character,withdrawal.bank);
 check(call("gw-storage-transfer-save",owned+"\t2\t2\t"+encoded(withdrawn.durable.character)+"\t"+encoded(withdrawn.durable.bank))=="3\t3"&&ids.commit(std::move(withdrawn)),"partial withdrawal persists both revisions atomically");
 check(withdrawn.durable.character.items.at(4).definition.quantity==250&&withdrawn.durable.character.items.at(4).profession==bone.profession&&withdrawn.durable.character.items.at(4).customized==bone.customized&&withdrawn.durable.bank.items.at(1).definition.quantity==2250&&withdrawn.durable.bank.material_quota==2500,"persisted withdrawal conserves2500 units and complete material metadata");
 Accounts reopen(database.string());check(reopen.execute("gw-character-inventory-get",owned,[]{return true;}).payload=="3\t"+encoded(withdrawn.durable.character)&&reopen.execute("gw-account-storage-get",account,[]{return true;}).payload=="3\t"+encoded(withdrawn.durable.bank),"FWI5 material quota/counts/identity survive independent Accounts connection restart");
 gw::inventory::identity::Session reconnect;const auto recovered=reconnect.hydrate(gw::inventory::decode(gw::inventory::encode(withdrawn.durable.character)),gw::inventory::decode(gw::inventory::encode(withdrawn.durable.bank)));const auto restored=reconnect.prepare(recovered.character,recovered.bank);
 check(encoded(restored.durable.character)==encoded(withdrawn.durable.character)&&encoded(restored.durable.bank)==encoded(withdrawn.durable.bank),"client reconnect restores material ownership and count without duplicating or regranting quota");
 }
 {
 Accounts accounts(database.string());
 auto call=[&](const std::string& operation,const std::string& payload){const auto result=accounts.execute(operation,payload,[]{return true;});check(result.status==FW_OK,"roster fixture operation");return result.payload;};
 const std::string password="roster-fixture-password";
 const auto account=split(call("register","RosterFixture\t"+hex({reinterpret_cast<const std::uint8_t*>(password.data()),password.size()})))[0];
 gw::Summary2022 imported;imported.map=148;imported.appearance=0x123456;imported.level=20;imported.low_nibble=1;
 imported.equipment={{33,4,6}};
 const auto character=split(call("gw-character-profile-import","RosterFixture\tRoster Alpha\t"+hex(imported.profile().encode())))[0];
 auto roster=[&]{const auto fields=split(call("gw-character-roster-page",account+"\t0"));check(fields.size()==6,"single owned roster entry");std::array<std::uint8_t,70> bytes{};check(sodium_hex2bin(bytes.data(),bytes.size(),fields[5].data(),fields[5].size(),nullptr,nullptr,nullptr)==0,"roster profile encoding");return gw::CharacterProfile::decode(bytes);};
 check(roster().equipment_2022()[0].composite==33,"uncaptured inventory preserves original summary");
 auto inventory=gw::inventory::baseline();gw::inventory::Item chest;chest.definition.id=1;chest.definition.file=544;chest.definition.type=7;chest.bag=2;chest.slot=2;inventory.items.emplace(1,chest);inventory.next_item=2;
 const auto owned=account+"\t"+character;
 call("gw-character-inventory-save",owned+"\t0\t"+encoded(inventory));
 const auto equipped=roster();check(equipped.equipment_2022().size()==5&&equipped.equipment_2022()[1].composite==544&&!equipped.equipment_2022()[0].composite,"roster projects saved equipped armor instead of captured gear");
 inventory.items.at(1).bag=1;inventory.items.at(1).slot=0;
 call("gw-character-inventory-save",owned+"\t1\t"+encoded(inventory));
 const auto removed=roster();check(!removed.equipment_2022()[1].composite&&removed.extended[2]==equipped.extended[2]&&removed.extended[12]==equipped.extended[12],"roster removes backpack armor without changing identity or campaign flags");
 }
 std::cout<<"PASS native inventory persistence, ownership, shared bank, CAS, atomic timeout rollback, materials, roster projection and restart\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
