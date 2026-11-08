#include "accounts.h"
#include "common.h"
#include "framework/module.h"
#include "gw_summary_2022.h"
#include "gw_character_skills.h"
#include "gw_attributes.h"
#include "gw_player_skills_generated.h"
#include <sodium.h>
#include <filesystem>
#include <iostream>
using namespace fw;
namespace {
void check(bool v,const char* why){if(!v)throw std::runtime_error(why);}
std::string hex(std::span<const std::uint8_t> b){std::string s(b.size()*2+1,'\0');sodium_bin2hex(s.data(),s.size(),b.data(),b.size());s.pop_back();return s;}
std::vector<std::uint8_t> unhex(const std::string& s){std::vector<std::uint8_t> b(s.size()/2);check(!sodium_hex2bin(b.data(),b.size(),s.data(),s.size(),nullptr,nullptr,nullptr),"hex decode");return b;}
}
int main(int argc,char** argv)try{
 check(argc==2&&sodium_init()>=0,"dedicated development test directory");const auto dir=std::filesystem::absolute(argv[1]).lexically_normal();check(dir.filename()=="development-accounts","test directory guard");std::filesystem::create_directories(dir);const auto db=dir/"state.db";
 for(const auto suffix:{"","-wal","-shm","-journal"}){std::error_code e;std::filesystem::remove(db.string()+suffix,e);check(!e,"test database cleanup");}
 Accounts accounts(db.string());auto call=[&](const std::string& op,const std::string& body,std::uint32_t expected=FW_OK){auto r=accounts.execute(op,body,[]{return true;});check(r.status==expected,(op+" status "+std::to_string(r.status)).c_str());return r.payload;};
 const std::string password="development-fixture";const auto pwd=hex({reinterpret_cast<const std::uint8_t*>(password.data()),password.size()});
 const auto owner=split(call("register","DevelopmentOwner\t"+pwd))[0],foreign=split(call("register","DevelopmentOther\t"+pwd))[0];
 gw::Summary2022 summary;summary.map=148;summary.level=20;summary.appearance=7u<<20;summary.secondary_profession=3;
 const auto character=split(call("gw-character-create",owner+"\tDevelopment Alpha\t"+hex(summary.profile().encode())))[0],owned=owner+"\t"+character;
 call("gw-character-development-unlock",foreign+"\t"+character,FW_NOT_FOUND);
 gw::skills::Profile prior;prior.primary=7;prior.secondary=3;prior.profession_mask=1u<<3;prior.learned.emplace(281,2);prior.bar[0]={281,0};
 call("gw-character-skills-import",owned+"\t1\t"+hex(gw::skills::encode_profile(prior))+"\t"+std::string(64,'a'));
 gw::attributes::Profile allocation;allocation.ranks[13]=3;call("gw-character-attributes-save",owned+"\t0\t"+hex(allocation.encode()));
 const auto oldskills=call("gw-character-skills-get",owned),oldattrs=call("gw-character-attributes-get",owned);
 unsigned checkpoints=0;const auto rejected=accounts.execute("gw-character-development-unlock",owned,[&]{return ++checkpoints<3;});check(rejected.status==FW_TIMEOUT&&checkpoints==3,"cancel after both writes");
 check(call("gw-character-skills-get",owned)==oldskills&&call("gw-character-attributes-get",owned)==oldattrs,"atomic rollback on final deadline");
 check(call("gw-character-development-unlock",owned)=="3\t2\t1329","all verified player skills granted");
 auto skills=gw::skills::decode_profile(unhex(split(call("gw-character-skills-get",owned))[1]));auto attrs=gw::attributes::Profile::decode(unhex(split(call("gw-character-attributes-get",owned))[1]));
 check(skills.primary==7&&skills.secondary==3&&skills.bar[0].skill==281&&skills.learned.at(281)==2,"grant preserves professions/bar/copies");
 check(skills.profession_mask==(0x7feu&~(1u<<7)),"all owned campaign secondary choices");
 check(!skills.learned.contains(1319)&&!skills.learned.contains(562)&&!skills.learned.contains(3068),"NPC and Codex-only excluded");
 for(auto id:gw::skills::obtainable_player_skills)check(skills.learned.contains(id),"full player whitelist");
 check(attrs.ranks==allocation.ranks&&attrs.total_points(20)==200&&attrs.source==gw::attributes::PointsSource::development,"explicit development max points preserves base ranks");
 check(call("gw-character-development-unlock",owned)=="3\t2\t1329","repeat idempotent revisions");
 summary.level=1;const auto low=split(call("gw-character-create",owner+"\tDevelopment Beta\t"+hex(summary.profile().encode())))[0];call("gw-character-development-unlock",owner+"\t"+low,FW_INVALID);
 check(gw::skills::decode_profile(unhex(split(call("gw-character-skills-get",owner+"\t"+low))[1])).learned.empty(),"invalid target leaves creation learning unchanged");
 std::cout<<"PASS targeted development skill unlock, provenance, ownership, preserved state, idempotency and atomic rollback\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
