#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <sstream>
#include <source_location>
using namespace fw::gw::combat::npcs;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC flesh trait line "+std::to_string(at.line()));
}
std::string fixture(unsigned version,std::string value="-",std::string evidence="-",std::string type="-",std::string type_evidence="-",std::string immune="-",std::string immune_evidence="-"){
 std::vector<std::string> row(version>=14?52:version==13?50:version==12?48:46,"-");
 row[0]="npc";row[1]="146";row[2]=row[3]="100";
 for(unsigned i=4;i<=8;++i)row[i]="0";
 row[9]="hostile-to-players";row[11]=row[12]="100";row[14]=row[15]="1";
 row[16]=row[17]=row[20]="e";row[27]="1";
 if(version>=12){row[46]=value;row[47]=evidence;}
 if(version>=13){row[48]=type;row[49]=type_evidence;}
 if(version>=14){row[50]=immune;row[51]=immune_evidence;}
 std::string out="gw-npc-combat "+std::to_string(version)+" "+std::string(64,'a')+"\nsource e authored "+std::string(64,'b')+" fixture\n";
 for(const auto& field:row)out+=field+" ";return out+"\n";
}
int main()try{
 for(unsigned mode=0;mode<4;++mode){
  const auto text=fixture(mode==0?11:12,mode<2?"-":mode==2?"0":"1",mode<2?"-":"e");
  std::istringstream input(text);const auto catalog=load_catalog(input,std::string(64,'a'));const auto* d=catalog.find(146,100);check(d);
  check(d->fleshy.has_value()==(mode>=2));if(mode>=2)check(*d->fleshy==(mode==3));
  const Lifetime life{1,1,1,146,100};LiveActor live(*d,life);
  check(live.fleshy(life)==d->fleshy);auto stale=life;++stale.actor_generation;check(!live.fleshy(stale));
  if(mode>=2){auto bad=*d;bad.fleshy_evidence="-";bool rejected=false;try{LiveActor invalid(bad,life);}catch(const std::invalid_argument&){rejected=true;}check(rejected);}
 }
 for(const auto& pair:std::vector<std::pair<std::string,std::string>>{{"1","-"},{"0","missing"},{"-","e"},{"2","e"}}){
  std::istringstream input(fixture(12,pair.first,pair.second));bool rejected=false;try{auto invalid=load_catalog(input,std::string(64,'a'));}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 }
 for(const auto& type:std::vector<std::string>{"-","human","norn"}){
  std::istringstream input(fixture(13,"1","e",type,type=="-"?"-":"e"));
  const auto catalog=load_catalog(input,std::string(64,'a'));const auto* d=catalog.find(146,100);check(d);
  check(d->creature_type.has_value()==(type!="-"));const Lifetime life{1,1,1,146,100};LiveActor live(*d,life);
  check(live.creature_type(life)==d->creature_type);auto stale=life;++stale.actor_generation;check(!live.creature_type(stale));
 }
 for(const auto& pair:std::vector<std::pair<std::string,std::string>>{{"human","-"},{"human","missing"},{"-","e"},{"Human","e"}}){
  std::istringstream input(fixture(13,"1","e",pair.first,pair.second));bool rejected=false;try{auto invalid=load_catalog(input,std::string(64,'a'));}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 }
 for(const auto& value:std::vector<std::string>{"-","0","1"}){
  std::istringstream input(fixture(14,"1","e","human","e",value,value=="-"?"-":"e"));
  const auto catalog=load_catalog(input,std::string(64,'a'));const auto* d=catalog.find(146,100);check(d);
  check(d->disease_immune.has_value()==(value!="-"));
  const Lifetime life{1,1,1,146,100};LiveActor live(*d,life);
  check(live.disease_immune(life,100)==d->disease_immune);
  auto stale=life;++stale.actor_generation;check(!live.disease_immune(stale,100));
  if(value!="-"){auto bad=*d;bad.disease_immunity_evidence="-";bool rejected=false;try{LiveActor invalid(bad,life);}catch(const std::invalid_argument&){rejected=true;}check(rejected);}
 }
 for(const auto& pair:std::vector<std::pair<std::string,std::string>>{{"1","-"},{"0","missing"},{"-","e"},{"2","e"}}){
  std::istringstream input(fixture(14,"1","e","human","e",pair.first,pair.second));bool rejected=false;
  try{auto invalid=load_catalog(input,std::string(64,'a'));}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 }
 for(unsigned version:{14u,15u}){
  auto text=fixture(version,"1","e","human","e","0","e");const auto token=text.find("hostile-to-players");text.replace(token,18,"allied-to-players");
  std::istringstream input(text);bool accepted=false;
  try{const auto catalog=load_catalog(input,std::string(64,'a'));const auto* d=catalog.find(146,100);check(d&&d->relation==Relation::allied_to_players);
   const Lifetime life{1,1,1,146,100};LiveActor live(*d,life);check(live.healable_ally(life));accepted=true;
  }catch(const std::invalid_argument&){}
  check(accepted==(version==15));
 }
 std::cout<<"PASS NPC trait schemas11-15,legacy unknown,false vs true,evidence integrity and stale lifetime\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
