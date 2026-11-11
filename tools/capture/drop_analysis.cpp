#include "../skills/capture_json.h"
#include "drop_analysis.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
using capture::Json;
std::string encoded(const Json& j){if(j.kind!=Json::array||j.items.size()>8192)return {};std::string result;for(auto& u:j.items){auto n=u.u32();if(n>65535)throw std::runtime_error("UTF16 bound");result+=std::to_string(n)+",";}return result;}
std::uint64_t number(const Json& j){if(j.kind!=Json::integer||j.number<0)throw std::runtime_error("positive integer required");return static_cast<std::uint64_t>(j.number);}
bool flag(const Json& j){if(j.kind!=Json::boolean)throw std::runtime_error("boolean required");return j.number!=0;}
int main(int argc,char**argv)try{
 if(argc!=3)throw std::runtime_error("usage: drop_analysis capture.jsonl report.txt");
 std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("capture unavailable");
 capture::drops::Reducer r;std::map<std::pair<std::string,std::string>,std::uint64_t> notifications;
 std::map<std::string,std::string> decoded;std::uint64_t lines=0,bad=0,unattributed=0,total=0,gaps=0;
 std::string line;char ch;
 while(input.get(ch)){if(++total>1024ull*1024*1024)throw std::runtime_error("capture exceeds1GiB");
  if(ch!='\n'){if(line.size()<262145)line+=ch;continue;}++lines;
  if(line.size()>262144){++bad;line.clear();r.actors.clear();continue;}
  try{auto row=capture::Parser(line).parse();auto event=row.at("event").str();
   if(event=="name_decoded"){decoded[encoded(row.at("name_encoded_utf16"))]=row.at("name").str();}
   else if(event=="npc_observations"){
    r.scope(number(row.at("instance_epoch")));for(auto& npc:row.at("npcs").items)
     r.metadata(npc.at("agent").u32(),npc.at("model").u32(),npc.at("allegiance").u32(),encoded(npc.at("name_encoded_utf16")));
   }else if(event=="npc_visibility_changed"){
    r.scope(number(row.at("instance_epoch")));for(auto& d:row.at("changes").items)
     r.observation(d.at("agent").u32(),d.at("model").u32(),flag(d.at("visible")),flag(d.at("observed_dead")),flag(d.at("status_changed_without_visibility_change")));
   }else if(event=="loot_notification"){
    if(row.at("confidence").str()!="report_name_only"){++unattributed;continue;}
    (void)number(row.at("timestamp_filetime"));
    auto source=encoded(row.at("source_reported_name").at("name_encoded_utf16")),item=encoded(row.at("item_reported_name").at("name_encoded_utf16"));
    if(source.empty()||item.empty()){++unattributed;continue;}++notifications[{source,item}];
   }else if(event=="collector_heartbeat"||event=="collector_stop"){
    if(auto it=row.members.find("dropped_samples");it!=row.members.end())gaps=(std::max)(gaps,number(it->second));
   }
  }catch(const std::exception&){++bad;r.actors.clear();}
  line.clear();
 }
 if(input.bad())throw std::runtime_error("read failed");if(!line.empty())++bad;
 std::ofstream out(argv[2]);if(!out)throw std::runtime_error("report unavailable");
 out<<"Observed loot audit\nsource="<<argv[1]<<"\nlines="<<lines<<" malformed_or_overbound="<<bad<<" reported_queue_drops="<<gaps
 <<"\nverified_observed_enemy_deaths="<<r.kills<<" initial_dead_excluded="<<r.excluded_initial_dead<<" visibility_losses_not_kills="<<r.visibility_losses<<" unidentified_transitions="<<r.unidentified<<" unattributed_notifications="<<unattributed
 <<"\nMethod: exact encoded source-name join to unique observed creature model. Notification counts / observed enemy-death counts are observational rates, NOT probabilities; may exceed1. No binomial confidence interval: per-kill trials and complete loot observation are not proven.\nBias: missed deaths/drops, party reservation and loot modes, compass visibility, sampling intervals, duplicate names/models and capture gaps. Quantity not exposed by current notification; counts are messages, not units. Ground items and weapon/rune modifier definitions are separate observations; source-item instance link not proven.\n";
 for(auto& [key,count]:notifications){auto it=r.name_models.find(key.first);out<<"\nsource="<<std::quoted(decoded.contains(key.first)?decoded[key.first]:key.first)<<" item="<<std::quoted(decoded.contains(key.second)?decoded[key.second]:key.second)<<" notifications="<<count;
  if(it==r.name_models.end()||it->second.size()!=1)out<<" model_join="<<(it==r.name_models.end()?"unknown":"ambiguous")<<" rate=unavailable";
  else{auto model=*it->second.begin();auto denominator=r.model_kills[model];out<<" model="<<model<<" observed_enemy_deaths="<<denominator;if(denominator)out<<" observed_notifications_per_observed_death="<<std::setprecision(8)<<double(count)/double(denominator);else out<<" rate=unavailable_zero_denominator";}
  out<<" binomial_interval=unavailable\n";
 }
 out.flush();if(!out)throw std::runtime_error("write failed");std::cout<<"Observed enemy deaths "<<r.kills<<", notification groups "<<notifications.size()<<"\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

