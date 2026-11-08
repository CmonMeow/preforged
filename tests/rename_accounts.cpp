#include "character_results.h"
#include "character_state.h"
#include "accounts.h"
#include "common.h"
#include "framework/module.h"
#include "gw_inventory.h"
#include "gw_character_skills.h"
#include "gw_attributes.h"
#include <sodium.h>
#include <winsqlite/winsqlite3.h>
#include <filesystem>
#include <iostream>
using namespace fw;
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::string hex(std::span<const std::uint8_t> bytes){std::string out(bytes.size()*2+1,'\0');sodium_bin2hex(out.data(),out.size(),bytes.data(),bytes.size());out.pop_back();return out;}
int main(int argc,char** argv)try{
 check(argc==2&&sodium_init()>=0,"scratchdir");const auto directory=std::filesystem::absolute(argv[1]).lexically_normal();check(directory.filename()=="rename-accounts","testowneddirectory");std::filesystem::create_directories(directory);auto database=directory/"state.db";
 for(auto suffix:{"","-wal","-shm","-journal"}){std::error_code error;std::filesystem::remove(database.string()+suffix,error);check(!error,"fixture reset");}
 std::string owner,character,inventory;
 {Accounts accounts(database.string());auto call=[&](const std::string& op,const std::string& payload,unsigned expected=FW_OK){auto reply=accounts.execute(op,payload,[]{return true;});if(reply.status!=expected)throw std::runtime_error(op+" status"+std::to_string(reply.status)+" "+reply.payload);return reply.payload;};
 owner=split(call("register","RenameAlice\t72656e616d652d666978747572652d70617373776f7264"))[0];auto other=split(call("register","RenameBob\t72656e616d652d666978747572652d70617373776f7264"))[0];
 character=split(call("gw-character-create",owner+"\tOld Character"))[0];auto other_character=split(call("gw-character-create",other+"\tOccupied Name"))[0];
 const gwserver::OwnedCharacter typed_owner{gwserver::Identity::from_hex(owner),gwserver::Identity::from_hex(character)};
 const auto typed_roster=accounts.characterRoster({typed_owner.account,0},[]{return true;});
 check(typed_roster.status==FW_OK&&typed_roster.total==1&&typed_roster.entries.size()==1&&typed_roster.entries[0].name=="Old Character","typed owned roster");
 check(accounts.characterRoster({typed_owner.account,3},[]{return true;}).entries.empty(),"typed empty roster page");
 check(accounts.characterRoster({typed_owner.account,1},[]{return true;}).status==FW_INVALID,"typed invalid roster offset");
 check(accounts.characterRoster({{},0},[]{return true;}).status==FW_NOT_FOUND,"missing account roster");
 unsigned roster_checks=0;const auto expired_roster=accounts.characterRoster({typed_owner.account,0},[&]{return ++roster_checks<3;});
 check(expired_roster.status==FW_TIMEOUT&&expired_roster.entries.empty()&&expired_roster.total==0,"expired roster exposes no partial records");
 check(accounts.characterName(typed_owner,[]{return true;}).value=="Old Character","typed character name read");
 check(accounts.characterName({gwserver::Identity::from_hex(other),typed_owner.character},[]{return true;}).status==FW_NOT_FOUND,"typed foreign name read");
 unsigned name_checks=0;const auto expired_name=accounts.characterName(typed_owner,[&]{return ++name_checks<2;});
 check(expired_name.status==FW_TIMEOUT&&!expired_name.value,"expired name read exposes no result");
 check(accounts.characterNumber(typed_owner,true,[]{return true;}).value.has_value(),"typed map read");
 check(accounts.characterNumber({gwserver::Identity::from_hex(other),typed_owner.character},false,[]{return true;}).status==FW_NOT_FOUND,"foreign experience read");
 for(bool map:{false,true}){unsigned checks=0;const auto expired=accounts.characterNumber(typed_owner,map,[&]{return ++checks<2;});check(expired.status==FW_TIMEOUT&&!expired.value,"expired character number exposes no result");}
 const auto roster_before=call("gw-character-roster-page",owner+"\t0");
 call("gw-character-create",owner+"\toccupied name",FW_CONFLICT);
 check(call("gw-character-roster-page",owner+"\t0")==roster_before,"cross-account creation collision preserves roster");
 call("gw-character-create",other+"\tOLD CHARACTER",FW_CONFLICT);
 const auto temporary=split(call("gw-character-create",other+"\tReleased Name"))[0];
 unsigned delete_checks=0;
 const auto cancelled_delete=accounts.deleteCharacter({gwserver::Identity::from_hex(other),gwserver::Identity::from_hex(temporary)},[&]{return ++delete_checks<2;});
 check(cancelled_delete.status==FW_TIMEOUT,"typed deletion precommit cancellation");
 check(accounts.characterName({gwserver::Identity::from_hex(other),gwserver::Identity::from_hex(temporary)},[]{return true;}).value=="Released Name","cancelled deletion removed character");
 call("gw-character-delete",other+"\t"+temporary);
 const auto reused=split(call("gw-character-create",owner+"\treleased name"))[0];
 call("gw-character-delete",owner+"\t"+reused);
 call("gw-character-inventory-save",owner+"\t"+character+"\t0\t"+hex(gw::inventory::encode(gw::inventory::baseline())));inventory=call("gw-character-inventory-get",owner+"\t"+character);
 gw::skills::Profile skills;skills.primary=6;skills.learned.emplace(171,2);skills.bar[0]={171,0};call("gw-character-skills-import",owner+"\t"+character+"\t0\t"+hex(gw::skills::encode_profile(skills))+"\t"+std::string(64,'a'));
 const auto skill_snapshot=call("gw-character-skills-get",owner+"\t"+character);
 gw::attributes::Profile attributes;call("gw-character-attributes-save",owner+"\t"+character+"\t0\t"+hex(attributes.encode()));const auto attributes_snapshot=call("gw-character-attributes-get",owner+"\t"+character);
 const auto rename=owner+"\t"+character+"\tOld Character\tNew Character";
 call("gw-character-development-rename",other+"\t"+character+"\tOld Character\tNew Character",FW_NOT_FOUND);
 call("gw-character-development-rename",owner+"\t"+character+"\tWrong Old Name\tNew Character",FW_CONFLICT);
 call("gw-character-development-rename",owner+"\t"+character+"\tOld Character\toccupied name",FW_CONFLICT);
 call("gw-character-development-rename",owner+"\t"+character+"\tOld Character\tThis Name Is Too Long Now",FW_INVALID);
 unsigned calls=0;auto cancelled=accounts.execute("gw-character-development-rename",rename,[&]{return ++calls<3;});check(cancelled.status==FW_TIMEOUT,"final precommit cancellation");
 check(call("gw-character-name-get",owner+"\t"+character)=="Old Character","failed rename leavesoldname");
 check(call("gw-character-development-rename",rename)=="New Character"&&call("gw-character-name-get",owner+"\t"+character)=="New Character","durable rename succeeds");
 call("gw-character-development-rename",rename,FW_CONFLICT);
 check(call("gw-character-inventory-get",owner+"\t"+character)==inventory,"rename preserves GUIDboundinventory and revision");
 check(call("gw-character-skills-get",owner+"\t"+character)==skill_snapshot&&call("gw-character-attributes-get",owner+"\t"+character)==attributes_snapshot,"rename preserves learned skill copies bar and attributes");
 const auto roster=call("gw-character-roster-page",owner+"\t0");check(roster.find(character)!=std::string::npos&&roster.find("New Character")!=std::string::npos,"roster carries sameGUID andnewname");
 }
 {Accounts reopened(database.string());auto name=reopened.execute("gw-character-name-get",owner+"\t"+character,[]{return true;});check(name.status==FW_OK&&name.payload=="New Character","rename persists restart");}
 std::cout<<"rename accounts PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
