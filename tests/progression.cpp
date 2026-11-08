#include "gw_progression.h"
#include "accounts.h"
#include "progression_state.h"
#include "common.h"
#include "framework/module.h"
#include <sodium.h>
#include <winsqlite/winsqlite3.h>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace fw;
namespace p=fw::gw::progression;
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
std::string hex(std::span<const std::uint8_t> b){std::string s(b.size()*2+1,'\0');sodium_bin2hex(s.data(),s.size(),b.data(),b.size());s.pop_back();return s;}
template<class F>void invalid(F f){bool caught=false;try{f();}catch(const std::invalid_argument&){caught=true;}check(caught,"invalid progression rejected");}
int main(int argc,char** argv)try{
 p::Character allegiance;check(p::allegiance_rank(allegiance,1951)==0&&p::allegiance_rank(allegiance,1953)==0&&p::allegiance_rank(allegiance,2094)==0&&p::allegiance_rank(allegiance,2096)==0&&!p::allegiance_rank(allegiance,396),"allegiance empty tracks");
 allegiance.ranks.emplace(1,p::Rank{0,6,u"Rank"});allegiance.tracks[5].fields[2]=1;
 check(p::allegiance_rank(allegiance,1951)==6&&p::allegiance_rank(allegiance,1953)==6&&p::allegiance_rank(allegiance,2094)==0&&p::allegiance_rank(allegiance,2096)==0,"allegiance faction isolation");
 check(hex(p::tournament_account(100000,true))=="300000000000000000000000000000000000000000000000a0860100","native tournament initialization serializer");
 check(hex(p::tournament_account(100000,false))=="32000000000000000000000000000000000000000000a0860100","native tournament update serializer");
 check(argc==3&&sodium_init()>=0,"fixture and scratch paths");p::Factions f;check(f.counters[3].maximum==20000,"Imperial cap sum");f=p::earn(f,p::Faction::kurzick,12000);check(f.counters[0].current==10000&&f.counters[0].total==10000,"earn clamps at cap");f=p::spend(f,p::Faction::kurzick,5000);check(f.counters[0].current==5000&&f.counters[0].total==10000,"spend retains lifetime");invalid([&]{p::spend(f,p::Faction::kurzick,5001);});check(p::encode(p::decode_factions(p::encode(f)))==p::encode(f),"faction codec");
 auto imperial=p::earn(p::Factions{},p::Faction::imperial,10001);
 auto exchanged=p::exchange_imperial(imperial,p::Faction::balthazar,10001);
 check(exchanged.counters[3].current==0&&exchanged.counters[3].total==10001&&exchanged.counters[2].current==3333,"Imperial exchange rounding/lifetime");
 for(auto target:{p::Faction::kurzick,p::Faction::luxon}){
  auto capped=imperial;capped=p::earn(capped,target,9999);capped=p::exchange_imperial(capped,target,10001);
  check(capped.counters[unsigned(target)].current==10000&&capped.counters[unsigned(target)].total==10000&&capped.counters[3].current==0,"Imperial exchange discards credit above cap");
 }
 invalid([&]{p::exchange_imperial(imperial,p::Faction::imperial,1);});
 invalid([&]{p::exchange_imperial(imperial,p::Faction::balthazar,0);});
 invalid([&]{p::exchange_imperial(imperial,p::Faction::balthazar,10002);});
 p::Character character;character.skill_points=11;character.skill_points_total=12;character.hero_points=7;character.hero_points_total=8;character.ranks.emplace(1,p::Rank{2,1,u"Rank"});character.tracks.emplace(0,p::Track{{2,100,1,100,0,0,1,1},u"Points",u"Hint"});character.active=0;
 check(p::encode(p::decode_character(p::encode(character)))==p::encode(character),"title/currency codec");
 auto select=p::select_title(character,p::Packet{86,0,0,0,0,0});check(select&&select->active==0,"select earned title");check(p::select_title(character,p::Packet{87,0})->active==255,"hide title");check(!p::select_title(character,p::Packet{86,0,1,0,0,0}),"unearned track denied");auto bad=character;bad.tracks[0].fields[1]=99;invalid([&]{p::validate(bad);});
 check(p::ebon_vanguard_rank(character)==0,"absent Vanguard track uses rank zero");
 auto vanguard=character;vanguard.ranks.emplace(2,p::Rank{2,7,u"Vngrd"});vanguard.tracks.emplace(40,p::Track{{0,100,2,100,0,0,0,0},u"Points",u"Hint"});
 check(p::ebon_vanguard_rank(vanguard)==7,"Vanguard tier read without selecting display title");
 vanguard.tracks.at(40).fields[2]=999;check(!p::ebon_vanguard_rank(vanguard),"missing title tier must not grant rank");
 check(p::deldrimor_rank(character)==0,"absent Deldrimor track uses rank zero");
 auto deldrimor=character;deldrimor.ranks.emplace(5,p::Rank{2,8,u"Deldr"});deldrimor.tracks.emplace(39,p::Track{{0,100,5,100,0,0,0,0},u"Points",u"Hint"});
 check(p::deldrimor_rank(deldrimor)==8,"Deldrimor tier read without selecting display title");
 deldrimor.tracks.at(39).fields[2]=999;check(!p::deldrimor_rank(deldrimor),"missing Deldrimor tier must not grant rank");
 check(p::norn_rank(character)==0,"absent Norn track uses rank zero");
 auto norn=character;norn.ranks.emplace(3,p::Rank{2,10,u"Norn"});norn.tracks.emplace(41,p::Track{{0,100,3,100,0,0,0,0},u"Points",u"Hint"});
 check(p::norn_rank(norn)==10,"Norn tier read without selecting display title");
 norn.tracks.at(41).fields[2]=999;check(!p::norn_rank(norn),"missing Norn tier must not grant rank");
 check(p::sunspear_rank(character)==0,"absent Sunspear track uses rank zero");
 auto sunspear=character;sunspear.ranks.emplace(4,p::Rank{2,7,u"Sunsp"});sunspear.tracks.emplace(17,p::Track{{0,100,4,100,0,0,0,0},u"Points",u"Hint"});
 check(p::sunspear_rank(sunspear)==7,"earned Sunspear tier is independent of selected title");
 sunspear.tracks.at(17).fields[2]=999;check(!p::sunspear_rank(sunspear),"missing Sunspear rank fails closed");
 std::map<std::string,std::string> fixtures;std::ifstream source(argv[1]);std::string label,value;while(source>>label>>value)fixtures[label]=value;check(fixtures.size()==11,"all native fixtures present");
 p::Factions wire;wire.counters[0].current=1;wire.counters[0].total=2;wire.counters[1].current=3;wire.counters[1].total=4;wire.counters[3].current=5;wire.counters[3].total=6;wire.counters[2].current=9;wire.counters[2].total=10;auto packets=p::restore(wire,character,100,20,100,42);
 const std::array<const char*,8> names{"stats","max-kurzick","max-luxon","max-balthazar","max-imperial","title-rank","title-track","title-display"};check(packets.size()==names.size(),"restore wire packet count");for(unsigned i=0;i<packets.size();++i){auto b=packets[i];b[1]|=0x80;check(hex(b)==fixtures.at(names[i]),"original client serializer fixture");}
 const auto dir=std::filesystem::absolute(argv[2]).lexically_normal();check(dir.filename()=="progression-accounts","scratch guard");std::filesystem::create_directories(dir);auto db=dir/"state.db";for(auto suffix:{"","-wal","-shm","-journal"}){std::error_code e;std::filesystem::remove(db.string()+suffix,e);check(!e,"test reset");}
 Accounts accounts(db.string());auto call=[&](const std::string& op,const std::string& body,unsigned expected=FW_OK){auto result=accounts.execute(op,body,[]{return true;});check(result.status==expected,(op+" status "+std::to_string(result.status)).c_str());return result.payload;};std::string password="progression-fixture";auto owner=split(call("register","ProgressionOwner\t"+hex({reinterpret_cast<const std::uint8_t*>(password.data()),password.size()})))[0];auto other=split(call("register","ProgressionOther\t"+hex({reinterpret_cast<const std::uint8_t*>(password.data()),password.size()})))[0];auto id=split(call("gw-character-create",owner+"\tProgression Alpha"))[0];const auto owned=owner+"\t"+id;
 check(call("gw-account-faction-get",owner)=="0"&&call("gw-character-progression-get",owned)=="0","absent earned data explicit");call("gw-character-progression-get",other+"\t"+id,FW_NOT_FOUND);check(call("gw-account-faction-save",owner+"\t0\t"+hex(p::encode(f)))=="1","account faction durable");call("gw-account-faction-save",owner+"\t0\t"+hex(p::encode(f)),FW_CONFLICT);check(call("gw-account-faction-get",other)=="0","account factions isolated");call("gw-character-progression-save",owned+"\t0\t"+hex(p::encode(character)));const auto before=call("gw-character-progression-get",owned);auto hidden=character;hidden.active=255;unsigned checks=0;auto cancelled=accounts.execute("gw-character-progression-save",owned+"\t1\t"+hex(p::encode(hidden)),[&]{return ++checks<3;});check(cancelled.status==FW_TIMEOUT&&checks==3&&call("gw-character-progression-get",owned)==before,"precommit timeout rollback");call("gw-character-progression-save",other+"\t"+id+"\t1\t"+hex(p::encode(hidden)),FW_NOT_FOUND);call("gw-character-delete",owned);check(call("gw-account-faction-get",owner)=="1\t"+hex(p::encode(f)),"account faction survives character deletion");call("gw-character-progression-get",owned,FW_NOT_FOUND);
 check(call("gw-account-faction-save",other+"\t0\t"+hex(p::encode(imperial)))=="1","exchange fixture saved");
 const auto exchange_before=call("gw-account-faction-get",other);
 unsigned direct_exchange_checks=0;
 check(accounts.exchangeImperialFaction({gwserver::Identity::from_hex(other),1,p::Faction::luxon,10001},[&]{return ++direct_exchange_checks<3;}).status==FW_TIMEOUT&&direct_exchange_checks==3,"typed exchange final deadline");
 check(call("gw-account-faction-get",other)==exchange_before,"typed exchange rolls back debit and credit");
 unsigned exchange_checks=0;auto expired=accounts.execute("gw-account-faction-exchange",other+"\t1\t2\t10001",[&]{return ++exchange_checks<3;});
 check(expired.status==FW_TIMEOUT&&call("gw-account-faction-get",other)==exchange_before,"expired faction exchange rolls back both balances");
 check(call("gw-account-faction-exchange",other+"\t1\t2\t10001")=="2","exchange commits revision");
 check(call("gw-account-faction-get",other)=="2\t"+hex(p::encode(exchanged)),"exchange saved both balances");
 call("gw-account-faction-exchange",other+"\t1\t2\t10001",FW_CONFLICT);
 call("gw-account-faction-exchange",other+"\t2\t2\t1",FW_CONFLICT);
 check(call("gw-account-faction-get",other)=="2\t"+hex(p::encode(exchanged)),"failed exchange leaves account unchanged");
 const auto reward_owner=split(call("register","FactionRewardOwner\t"+hex({reinterpret_cast<const std::uint8_t*>(password.data()),password.size()})))[0];
 auto reward_profile=p::Factions{};
 for(unsigned kind=0;kind<4;++kind){
  const auto request=reward_owner+"\t"+std::to_string(kind)+"\t"+std::to_string(kind)+"\t123";
  check(call("gw-account-faction-award",request)==std::to_string(kind+1),"faction award revision");
  reward_profile=p::earn(reward_profile,static_cast<p::Faction>(kind),123);
  check(call("gw-account-faction-get",reward_owner)==std::to_string(kind+1)+"\t"+hex(p::encode(reward_profile)),"all four faction awards durable");
  call("gw-account-faction-award",request,FW_CONFLICT);
 }
 const auto reward_before=call("gw-account-faction-get",reward_owner);unsigned award_checks=0;
 unsigned direct_award_checks=0;
 check(accounts.awardFaction({gwserver::Identity::from_hex(reward_owner),4,p::Faction::balthazar,999},[&]{return ++direct_award_checks<3;}).status==FW_TIMEOUT&&direct_award_checks==3,"typed award final deadline");
 check(call("gw-account-faction-get",reward_owner)==reward_before,"typed award rollback");
 const auto cancelled_award=accounts.execute("gw-account-faction-award",reward_owner+"\t4\t0\t999",[&]{return ++award_checks<3;});
 check(cancelled_award.status==FW_TIMEOUT&&call("gw-account-faction-get",reward_owner)==reward_before,"faction award rollback");
 call("gw-account-faction-award",reward_owner+"\t4\t0\t0",FW_INVALID);
 check(call("gw-account-faction-get",reward_owner)==reward_before,"zero award leaves balances unchanged");
 const auto import_character=split(call("gw-character-create",reward_owner+"\tProgression Import"))[0];
 const auto import_owned=reward_owner+"\t"+import_character;
 auto captured_character=character;captured_character.active=255;
 captured_character.ranks.emplace(31,p::Rank{0,0,std::u16string{char16_t(0x101)}});
 captured_character.tracks.emplace(4,p::Track{{4,15,31,0,1,25,12,32},u"Points",u"Hint"});
 captured_character.tracks.emplace(7,p::Track{{0,0,31,0,1,25,6,32},u"Points",u"Hint"});
 p::validate(captured_character);
 check(!p::select_title(captured_character,p::Packet{86,0,4,0,0,0}),"rank-zero placeholder cannot be displayed as earned");
 auto maxed=captured_character;maxed.tracks[0].fields[4]=4095;maxed.tracks[0].fields[5]=UINT32_MAX;p::validate(maxed);
 maxed.ranks.emplace(185,p::Rank{1,1000,u"Carto"});maxed.ranks.emplace(186,p::Rank{1,UINT32_MAX,std::u16string{char16_t(0x101)}});p::validate(maxed);
 auto invalid_rank=maxed;invalid_rank.ranks.at(185).number=1001;invalid([&]{p::validate(invalid_rank);});
 invalid_rank=maxed;invalid_rank.ranks.at(186).name=u"False";invalid([&]{p::validate(invalid_rank);});
 const auto import_request=import_owned+"\t4\t0\t"+hex(p::encode(reward_profile))+"\t"+hex(p::encode(captured_character))+"\t"+std::string(64,'a');
 unsigned import_checks=0;const auto rolled_back=accounts.execute("gw-character-progression-import",import_request,[&]{return ++import_checks<3;});
 check(rolled_back.status==FW_TIMEOUT&&call("gw-account-faction-get",reward_owner)==reward_before&&call("gw-character-progression-get",import_owned)=="0","combined import cancellation rolls back both profiles");
 call("gw-character-progression-import",other+"\t"+import_character+"\t4\t0\t"+hex(p::encode(reward_profile))+"\t"+hex(p::encode(captured_character))+"\t"+std::string(64,'a'),FW_NOT_FOUND);
 check(call("gw-character-progression-import",import_request)=="5\t1","combined progression import revisions");
 check(call("gw-account-faction-get",reward_owner)=="5\t"+hex(p::encode(reward_profile))&&call("gw-character-progression-get",import_owned)=="1\t"+hex(p::encode(captured_character)),"combined import persists captured profiles");
 call("gw-character-progression-import",import_request,FW_CONFLICT);
 // Recreate the pre-account-title schema state without changing imported data.
 sqlite3* legacy=nullptr;check(sqlite3_open(db.string().c_str(),&legacy)==SQLITE_OK,"legacy database open");
 check(sqlite3_exec(legacy,"DELETE FROM fw_gw_account_titles",nullptr,nullptr,nullptr)==SQLITE_OK,"legacy account-title absence");sqlite3_close(legacy);
 const auto seed=import_owned+"\t1";
 call("gw-account-titles-seed",other+"\t"+import_character+"\t1",FW_NOT_FOUND);
 call("gw-account-titles-seed",import_owned+"\t0",FW_CONFLICT);
 unsigned seed_checks=0;const auto seed_cancelled=accounts.execute("gw-account-titles-seed",seed,[&]{return ++seed_checks<3;});
 check(seed_cancelled.status==FW_TIMEOUT,"title seed cancellation");
 check(call("gw-account-titles-seed",seed)=="1","legacy imported titles initialized");
 call("gw-account-titles-seed",seed,FW_CONFLICT);
 check(call("gw-character-progression-get",import_owned)=="1\t"+hex(p::encode(captured_character))&&call("gw-account-faction-get",reward_owner)=="5\t"+hex(p::encode(reward_profile)),"title seed preserves character selection, currencies and faction revisions");
 const auto sibling=split(call("gw-character-create",reward_owner+"\tProgression Sibling"))[0];
 const auto sibling_owned=reward_owner+"\t"+sibling;
 const auto shared=split(call("gw-character-progression-get",sibling_owned));
 check(shared.size()==2&&shared[0]=="0","new sibling gets shared titles without character mutation");
 std::vector<std::uint8_t> shared_bytes(shared[1].size()/2);check(!sodium_hex2bin(shared_bytes.data(),shared_bytes.size(),shared[1].data(),shared[1].size(),nullptr,nullptr,nullptr),"shared profile hex");
 const auto shared_profile=p::decode_character(shared_bytes);
 check(shared_profile.tracks.contains(0)&&shared_profile.tracks.contains(4)&&!shared_profile.tracks.contains(7)&&shared_profile.skill_points==0&&shared_profile.hero_points==0,"account titles share without sharing character titles/currencies");
 auto altered_shared=shared_profile;altered_shared.tracks[0].fields[1]=101;
 call("gw-character-progression-save",sibling_owned+"\t0\t"+hex(p::encode(altered_shared)));
 const auto sibling_after=split(call("gw-character-progression-get",sibling_owned));
 shared_bytes.resize(sibling_after[1].size()/2);check(!sodium_hex2bin(shared_bytes.data(),shared_bytes.size(),sibling_after[1].data(),sibling_after[1].size(),nullptr,nullptr,nullptr),"shared reload hex");
 check(p::decode_character(shared_bytes).tracks.at(0).fields[1]==100,"character save cannot overwrite authoritative account title points");
 call("gw-character-delete",import_owned);
 check(call("gw-character-progression-get",sibling_owned)==sibling_after[0]+"\t"+sibling_after[1],"account titles survive deletion of imported character");
 const auto updated_xp=p::restore(wire,character,600,20,100,42);
 auto expected_stats=packets.front();for(unsigned i=0;i<4;++i)expected_stats[2+i]=static_cast<std::uint8_t>(600u>>(8*i));
 check(updated_xp.front()==expected_stats,"quest XP snapshot changed unrelated counters");
 for(unsigned i=1;i<packets.size();++i)check(updated_xp[i]==packets[i],"quest XP snapshot changed title/faction state");
 std::cout<<"PASS pinned2022 progression wire, caps, earned title selection, persistence ownership/CAS/rollback and Imperial exchange\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
