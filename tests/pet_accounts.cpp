#include "pet_actions.h"
#include "common.h"
#include <sodium.h>
#include <winsqlite/winsqlite3.h>
#include <barrier>
#include <filesystem>
#include <iostream>
#include <source_location>
using namespace fw;using namespace gwserver;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Pet persistence line "+std::to_string(at.line()));
}
int main(int argc,char** argv)try{
 check(argc==2);const auto dir=std::filesystem::absolute(argv[1]).lexically_normal();check(dir.filename()=="pet-accounts");
 std::filesystem::create_directories(dir);const auto db=dir/"state.db";
 for(auto suffix:{"","-wal","-shm","-journal"}){std::error_code error;std::filesystem::remove(db.string()+suffix,error);check(!error);}
 Accounts accounts(db.string());auto call=[&](const std::string& op,const std::string& payload){const auto r=accounts.execute(op,payload,[]{return true;});check(r.status==FW_OK);return split(r.payload)[0];};
 const auto owner=call("register","PetAlice\t71756573742d666978747572652d70617373776f7264");
 const auto other=call("register","PetBob\t71756573742d666978747572652d70617373776f7264");
 const auto character=call("gw-character-create",owner+"\tPet Owner");
 const OwnedCharacter owned{Identity::from_hex(owner),Identity::from_hex(character)},foreign{Identity::from_hex(other),owned.character};
 const fw::gw::pets::Profile absent{},adopted{fw::gw::pets::Pet{}};
 const AdoptPet request{owned,0,absent,adopted};
 const auto empty=[&]{const auto r=accounts.characterPets(owned,[]{return true;});check(r.status==FW_OK&&r.record&&r.record->revision==0&&r.record->profile==absent);};empty();
 check(accounts.characterPets(foreign,[]{return true;}).status==FW_NOT_FOUND);
 auto wrong=request;wrong.owner=foreign;check(accounts.adoptPet(wrong,[]{return true;}).status==FW_NOT_FOUND);empty();
 wrong=request;wrong.expected_revision=1;check(accounts.adoptPet(wrong,[]{return true;}).status==FW_CONFLICT);empty();
 wrong=request;wrong.expected=adopted;check(accounts.adoptPet(wrong,[]{return true;}).status==FW_INVALID);empty();
 for(unsigned mode=0;mode<6;++mode){wrong=request;
  if(mode==0)wrong.profile={};if(mode==1)wrong.profile.pet->level=20;if(mode==2)wrong.profile.pet->experience=1;
  if(mode==3)wrong.profile.pet->evolution=fw::gw::pets::Evolution::hearty;if(mode==4)wrong.profile.pet->name="Buddy";
  if(mode==5)wrong.profile.pet->species=static_cast<fw::gw::pets::Species>(999);
  check(accounts.adoptPet(wrong,[]{return true;}).status==FW_INVALID);empty();
 }
 for(unsigned boundary=1;boundary<=3;++boundary){unsigned calls=0;
  check(accounts.adoptPet(request,[&]{return ++calls<boundary;}).status==FW_TIMEOUT);empty();
  Accounts reopened(db.string());check(reopened.characterPets(owned,[]{return true;}).record->profile==absent);
 }
 check(accounts.adoptPet(request,[]{return true;}).revision==1);
 check(accounts.adoptPet(request,[]{return true;}).status==FW_CONFLICT);
 wrong=request;wrong.expected_revision=1;check(accounts.adoptPet(wrong,[]{return true;}).status==FW_CONFLICT);
 {Accounts reopened(db.string());const auto r=reopened.characterPets(owned,[]{return true;});check(r.record&&r.record->revision==1&&r.record->profile==adopted);}
 const auto race=call("gw-character-create",owner+"\tPet Race");const OwnedCharacter racing{owned.account,Identity::from_hex(race)};
 Accounts first(db.string()),second(db.string());std::barrier start(2);std::uint32_t outcomes[2]{};
 std::thread a([&]{start.arrive_and_wait();outcomes[0]=first.adoptPet({racing,0,absent,adopted},[]{return true;}).status;});
 std::thread b([&]{start.arrive_and_wait();outcomes[1]=second.adoptPet({racing,0,absent,adopted},[]{return true;}).status;});a.join();b.join();
 check((outcomes[0]==FW_OK&&outcomes[1]==FW_CONFLICT)||(outcomes[1]==FW_OK&&outcomes[0]==FW_CONFLICT));
 check(accounts.characterPets(racing,[]{return true;}).record->revision==1);
 {NetworkManager manager(db.string());check(manager.server().characterPets(owned).record->profile==adopted);
  check(manager.client().characterPets(owned).status==FW_UNAVAILABLE);
  check(manager.client().adoptPet(request).status==FW_UNAVAILABLE);
  const auto commit=pet_adoption_commit(manager.server(),owned);
  check(commit({0,absent,adopted})==fw::gw::pets::CommitStatus::conflict);
  const auto third=call("gw-character-create",owner+"\tPet Adapter");const OwnedCharacter adapter{owned.account,Identity::from_hex(third)};
  check(pet_adoption_commit(manager.server(),adapter)({0,absent,adopted})==fw::gw::pets::CommitStatus::committed);
  check(manager.server().characterPets(adapter).record->revision==1);
 }
 check(accounts.execute("gw-character-delete",owner+"\t"+character,[]{return true;}).status==FW_OK);
 check(accounts.characterPets(owned,[]{return true;}).status==FW_NOT_FOUND);
 sqlite3* probe=nullptr;check(sqlite3_open_v2(db.string().c_str(),&probe,SQLITE_OPEN_READONLY,nullptr)==SQLITE_OK);
 sqlite3_stmt* query=nullptr;check(sqlite3_prepare_v2(probe,"SELECT count(*) FROM fw_gw_pets WHERE character=?",-1,&query,nullptr)==SQLITE_OK);
 sqlite3_bind_text(query,1,character.c_str(),-1,SQLITE_TRANSIENT);check(sqlite3_step(query)==SQLITE_ROW&&sqlite3_column_int(query,0)==0);sqlite3_finalize(query);sqlite3_close(probe);
 std::cout<<"PASS pet ownership/revision CAS, exact profile, all timeout rollback boundaries, concurrent adoption, reopen, typed network adapter and deletion cascade; no Charm binding\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
