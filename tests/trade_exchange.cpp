#include "accounts.h"
#include "network.h"
#include "network_storage_adapter.h"
#include "common.h"
#include "framework/module.h"
#include "gw_trade_exchange.h"
#include <sodium.h>
#include <winsqlite/winsqlite3.h>
#include <filesystem>
#include <iostream>
using namespace fw;
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::string hex(std::span<const std::uint8_t> bytes){std::string value(bytes.size()*2+1,'\0');sodium_bin2hex(value.data(),value.size(),bytes.data(),bytes.size());value.pop_back();return value;}
void sql_fixture(const std::filesystem::path& path,const std::string& command){sqlite3* db=nullptr;check(sqlite3_open(path.string().c_str(),&db)==SQLITE_OK,"fixturedb");auto status=sqlite3_exec(db,command.c_str(),nullptr,nullptr,nullptr);sqlite3_close(db);check(status==SQLITE_OK,"fixturesql");}
int main(int argc,char** argv)try{
 check(argc==2&&sodium_init()>=0,"scratchdir required");auto directory=std::filesystem::absolute(argv[1]).lexically_normal();check(directory.filename()=="trade-exchange","testowneddirectory");std::filesystem::create_directories(directory);auto database=directory/"state.db";
 for(auto suffix:{"","-wal","-shm","-journal"}){std::error_code error;std::filesystem::remove(database.string()+suffix,error);check(!error,"fixture reset");}
 Accounts accounts(database.string());gwserver::NetworkManager manager(database.string()); TestStorageAdapter adapter(manager);
 auto call=[&](gwserver::Operation op,const std::string& payload,unsigned status=FW_OK){
     auto r=adapter.request(op,payload);if(r.status!=status)throw std::runtime_error(std::string(*gwserver::storage_operation(op))+" status"+std::to_string(r.status)+" "+r.payload);return r.payload;};
 auto a=split(call(gwserver::Operation::registerAccount,"TradeAlice\t74726164652d666978747572652d70617373776f7264"))[0],b=split(call(gwserver::Operation::registerAccount,"TradeBob\t74726164652d666978747572652d70617373776f7264"))[0];
 auto ca=split(call(gwserver::Operation::createCharacter,a+"\tTrade Alpha"))[0],cb=split(call(gwserver::Operation::createCharacter,b+"\tTrade Beta"))[0];
 auto left=gw::inventory::baseline(),right=left;left.gold=1000;right.gold=300;
 gw::inventory::Item item;item.definition.id=1;item.definition.file=1234;item.definition.type=11;item.definition.model=567;item.definition.quantity=100;item.definition.interaction=0x80000;item.definition.encoded_name=u"trade fixture";item.definition.modifiers={0x12345678};item.bag=1;left.items.emplace(1,item);left.next_item=2;
 auto other=item;other.definition.quantity=200;right.items.emplace(1,other);right.next_item=2;
 auto packed=[](const auto& p){return hex(gw::inventory::encode(p));};call(gwserver::Operation::saveInventory,a+"\t"+ca+"\t0\t"+packed(left));call(gwserver::Operation::saveInventory,b+"\t"+cb+"\t0\t"+packed(right));
 auto payload=a+"\t"+ca+"\t1\t100\t1:75\t"+b+"\t"+cb+"\t1\t200\t1:20";
 auto expect=gw::trade::exchange::plan(left,gw::trade::exchange::parse("100","1:75"),right,gw::trade::exchange::parse("200","1:20"));
 check(expect.left.gold==1100&&expect.right.gold==200&&expect.left.items.at(1).definition.quantity==45&&expect.right.items.at(1).definition.quantity==250&&expect.right.items.at(2).definition.quantity==5,"gold+stack conservation/collision remap");
 auto before_a=call(gwserver::Operation::characterInventory,a+"\t"+ca),before_b=call(gwserver::Operation::characterInventory,b+"\t"+cb);
 call(gwserver::Operation::exchangeTrade,b+"\t"+ca+"\t1\t0\t\t"+a+"\t"+cb+"\t1\t0\t",FW_NOT_FOUND);
 call(gwserver::Operation::exchangeTrade,a+"\t"+ca+"\t0\t0\t\t"+b+"\t"+cb+"\t1\t0\t",FW_CONFLICT);
 auto bad=payload;bad.replace(bad.find("1:75"),4,"1:101");call(gwserver::Operation::exchangeTrade,bad,FW_INVALID);
 const gwserver::ExchangeTrade typed{{{gwserver::Identity::from_hex(a),gwserver::Identity::from_hex(ca)},1,gw::trade::exchange::parse("100","1:75")},{{gwserver::Identity::from_hex(b),gwserver::Identity::from_hex(cb)},1,gw::trade::exchange::parse("200","1:20")}};
 unsigned typed_checks=0;
 check(accounts.exchangeTrade(typed,[&]{return ++typed_checks<3;}).status==FW_TIMEOUT,"typed precommit cancellation");
 check(call(gwserver::Operation::characterInventory,a+"\t"+ca)==before_a&&call(gwserver::Operation::characterInventory,b+"\t"+cb)==before_b,"typed cancelled trade partially committed");
 auto stale_typed=typed;stale_typed.right.expected_revision=0;
 check(accounts.exchangeTrade(stale_typed,[]{return true;}).status==FW_CONFLICT,"typed stale peer revision accepted");
 unsigned checkpoints=0;auto timeout=accounts.execute("gw-trade-exchange",payload,[&]{return ++checkpoints<3;});check(timeout.status==FW_TIMEOUT,"precommitcancel");
 check(call(gwserver::Operation::characterInventory,a+"\t"+ca)==before_a&&call(gwserver::Operation::characterInventory,b+"\t"+cb)==before_b,"cancel/stale/foreign/quantity mutate neitherowner");
 sql_fixture(database,"CREATE TRIGGER trade_second_failure BEFORE UPDATE ON fw_gw_inventory WHEN OLD.character='"+cb+"' BEGIN SELECT RAISE(ABORT,'fixture'); END;");call(gwserver::Operation::exchangeTrade,payload,FW_UNAVAILABLE);
 check(call(gwserver::Operation::characterInventory,a+"\t"+ca)==before_a&&call(gwserver::Operation::characterInventory,b+"\t"+cb)==before_b,"secondwritefailure rollbackbothowners");sql_fixture(database,"DROP TRIGGER trade_second_failure;");
 auto committed=split(call(gwserver::Operation::exchangeTrade,payload));check(committed.size()==4&&committed[0]=="2"&&committed[1]=="2"&&committed[2]==packed(expect.left)&&committed[3]==packed(expect.right),"atomicpersistedexchange exactresult");call(gwserver::Operation::exchangeTrade,payload,FW_CONFLICT);
 check(call(gwserver::Operation::characterInventory,a+"\t"+ca)=="2\t"+packed(expect.left)&&call(gwserver::Operation::characterInventory,b+"\t"+cb)=="2\t"+packed(expect.right),"committed inventories matchresult");
 auto full=right;full.bags.at(1).capacity=1;full.items.at(1).definition.model=99;auto rejected=[&](const auto& l,const auto& offer,const auto& r){bool denied=false;try{gw::trade::exchange::plan(l,offer,r,{});}catch(const std::invalid_argument&){denied=true;}check(denied,"invalidexchange denied");};rejected(left,gw::trade::exchange::parse("0","1:1"),full);
 auto custom=left;custom.items.at(1).customized=u"Owner";const auto custom_trade=gw::trade::exchange::plan(custom,gw::trade::exchange::parse("0","1:1"),gw::inventory::baseline(),{});check(custom_trade.right.items.at(1).customized==custom.items.at(1).customized,"customizeditem cantrade while retainingownerrestriction");
 {
  namespace exchange=gw::trade::exchange;
  check(exchange::offerable(custom,1)&&!exchange::offerable(custom,999),"offer admission allows customization but requires real owned identity");
  for(unsigned bit=0;bit<32;++bit){auto flags=left;flags.items.at(1).definition.interaction=1u<<bit;
   check(exchange::offerable(flags,1)==(bit!=8),"trade flag predicate rejects only reviewed interaction100");}
  for(unsigned type:{6u,20u,21u,44u,45u}){auto blocked=left;blocked.items.at(1).definition.type=static_cast<std::uint8_t>(type);check(!exchange::offerable(blocked,1),"existing forbidden trade item classes remain denied");}
  auto worn=left;worn.items.at(1).bag=2;worn.items.at(1).slot=0;
  check(!exchange::offerable(worn,1),"equipped item cannot enter offer");
  auto installed=left;installed.items.at(1).definition.type=3;installed.items.at(1).definition.quantity=1;
  installed.items.at(1).bag=0;installed.bags.at(1).container=1;
  check(!exchange::offerable(installed,1),"installed container cannot enter offer");
  for(unsigned side=0;side<2;++side){auto first=left,second=right;
   (side?second:first).items.at(1).definition.interaction|=0x100;
   const auto first_before=packed(first),second_before=packed(second);bool denied=false;
   try{exchange::plan(first,exchange::parse("10","1:1"),second,exchange::parse("20","1:1"));}
   catch(const std::invalid_argument&){denied=true;}
   check(denied&&packed(first)==first_before&&packed(second)==second_before,"either participant restricted item rejects without owned item or gold changes");
  }
 }
 auto capped=right;capped.gold=100000;rejected(left,gw::trade::exchange::parse("1",""),capped);
 check(packed(left)==before_a.substr(2)&&packed(right)==before_b.substr(2),"purefailedplan nevermutatesinputs");
 auto tiny_a=left,tiny_b=right;tiny_a.bags.at(1).capacity=tiny_b.bags.at(1).capacity=1;tiny_b.items.at(1).definition.model=99;
 auto swap=gw::trade::exchange::plan(tiny_a,gw::trade::exchange::parse("0","1:100"),tiny_b,gw::trade::exchange::parse("0","1:200"));
 check(swap.left.items.size()==1&&swap.right.items.size()==1&&swap.left.items.at(2).definition.model==99&&swap.right.items.at(2).definition.model==567,"full inventories swap via vacatedslots with colliding ids remapped");
 check(swap.left.items.at(2).definition.modifiers==other.definition.modifiers&&swap.left.items.at(2).definition.encoded_name==other.definition.encoded_name,"opaque metadata preserved acrossowners");
 auto quest=left;quest.items.at(1).definition.type=21;rejected(quest,gw::trade::exchange::parse("0","1:1"),right);
 bool duplicate=false;try{gw::trade::exchange::parse("0","1:1,1:2");}catch(const std::invalid_argument&){duplicate=true;}check(duplicate,"duplicateoffers rejected");
 auto armor=gw::inventory::baseline();auto plate=item;plate.definition.type=7;plate.definition.quantity=1;plate.definition.interaction=0;plate.profession=1;plate.reviewed_armor=gw::inventory::ReviewedArmor{80,20,0};armor.items.emplace(1,plate);armor.next_item=2;
 auto moved=gw::trade::exchange::plan(armor,gw::trade::exchange::parse("0","1:1"),gw::inventory::baseline(),{});
 check(moved.right.items.at(1).reviewed_armor==plate.reviewed_armor&&gw::inventory::decode(gw::inventory::encode(moved.right)).items.at(1).reviewed_armor==plate.reviewed_armor,"FWI6 reviewedmetadata copy anddurablecodec preserved");
 {
  auto recipient=full;recipient.bags.emplace(42,gw::inventory::Bag{42,1,4,5,0});
  const auto before=packed(recipient);rejected(left,gw::trade::exchange::parse("0","1:1"),recipient);
  check(packed(recipient)==before,"pack-only material trade capacity rejection preserves recipient");
  auto legacy=item;legacy.definition.id=2;legacy.bag=42;recipient.items.emplace(2,legacy);recipient.next_item=3;
  rejected(left,gw::trade::exchange::parse("0","1:1"),recipient);
  recipient.items.erase(2);
  auto gear=armor;gear.items.at(1).customized=u"Trade Beta";
  const auto offer=gw::trade::exchange::parse("0","1:1");
  const auto accepted=gw::trade::exchange::plan(gear,offer,recipient,{},u"Trade Alpha",u"Trade Beta");
  check(accepted.right.items.at(3).bag==42&&accepted.left.items.empty(),"trade admits recipient-owned customized equipment to pack");
  for(const auto owner:{u"Trade Alpha",u""}){
   bool denied=false;try{gw::trade::exchange::plan(gear,offer,recipient,{},u"Trade Alpha",owner);}catch(const std::invalid_argument&){denied=true;}
   check(denied,"trade cannot substitute sender or missing character for recipient owner");
  }
  gear.items.at(1).customized.reset();rejected(gear,offer,recipient);
  gear.items.at(1).customized=u"";
  check(gw::trade::exchange::plan(gear,offer,recipient,{}).right.items.at(3).bag==42,"explicit uncustomized equipment may trade into pack");
  // Existing accounts are at revision2. Exercise the authoritative name read
  // inside the same transaction as both inventories, not a caller-supplied name.
  gear.items.at(1).customized=u"Trade Alpha";
  call(gwserver::Operation::saveInventory,a+"\t"+ca+"\t2\t"+packed(gear));
  call(gwserver::Operation::saveInventory,b+"\t"+cb+"\t2\t"+packed(recipient));
  const auto stored_a=call(gwserver::Operation::characterInventory,a+"\t"+ca),stored_b=call(gwserver::Operation::characterInventory,b+"\t"+cb);
  call(gwserver::Operation::exchangeTrade,a+"\t"+ca+"\t3\t0\t1:1\t"+b+"\t"+cb+"\t3\t0\t",FW_INVALID);
  check(call(gwserver::Operation::characterInventory,a+"\t"+ca)==stored_a&&call(gwserver::Operation::characterInventory,b+"\t"+cb)==stored_b,"foreign owner pack denial rolls back both persisted trade inventories");
  gear.items.at(1).customized=u"Trade Beta";
  call(gwserver::Operation::saveInventory,a+"\t"+ca+"\t3\t"+packed(gear));
  const auto saved=split(call(gwserver::Operation::exchangeTrade,a+"\t"+ca+"\t4\t0\t1:1\t"+b+"\t"+cb+"\t3\t0\t"));
  const auto expected=gw::trade::exchange::plan(gear,offer,recipient,{},u"Trade Alpha",u"Trade Beta");
  check(saved.size()==4&&saved[0]=="5"&&saved[1]=="4"&&saved[3]==packed(expected.right),"database recipient name admits exact owned equipment without external name claim");
 }
 {
  unsigned revision_a=5,revision_b=4;
  for(unsigned side=0;side<2;++side){auto first=left,second=right;
   (side?second:first).items.at(1).definition.interaction|=0x100;
   call(gwserver::Operation::saveInventory,a+"\t"+ca+"\t"+std::to_string(revision_a)+"\t"+packed(first));++revision_a;
   call(gwserver::Operation::saveInventory,b+"\t"+cb+"\t"+std::to_string(revision_b)+"\t"+packed(second));++revision_b;
   const auto before_first=call(gwserver::Operation::characterInventory,a+"\t"+ca),before_second=call(gwserver::Operation::characterInventory,b+"\t"+cb);
   call(gwserver::Operation::exchangeTrade,a+"\t"+ca+"\t"+std::to_string(revision_a)+"\t10\t1:1\t"+b+"\t"+cb+"\t"+std::to_string(revision_b)+"\t20\t1:1",FW_INVALID);
   check(call(gwserver::Operation::characterInventory,a+"\t"+ca)==before_first&&call(gwserver::Operation::characterInventory,b+"\t"+cb)==before_second,
         "database trade restriction preserves both exact profiles gold and revisions for either restricted participant");
  }
 }
 std::cout<<"trade exchange PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


