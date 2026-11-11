#include "../skills/capture_json.h"
#include "quest_interaction_join.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <algorithm>
using capture::Json;
std::uint64_t number(const Json& j){if(j.kind!=Json::integer||j.number<0)throw std::runtime_error("nonnegative integer required");return static_cast<std::uint64_t>(j.number);}
template<class F> void read(const char* path,F consume){
 std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("input unavailable");
 std::string line;std::uint64_t total=0;char c;
 while(input.get(c)){
  if(++total>1024ull*1024*1024)throw std::runtime_error("input exceeds 1GiB");
  if(c!='\n'){if(line.size()>=262144)throw std::runtime_error("input row exceeds parser bound");line+=c;continue;}
  if(!line.empty())consume(capture::Parser(line).parse(),line);line.clear();
 }
 if(input.bad()||!line.empty())throw std::runtime_error("input read failed or final row incomplete");
}
int main(int argc,char** argv)try{
 if(argc!=4)throw std::runtime_error("usage: quest_interaction_join world.jsonl npc-interaction.jsonl output.jsonl");
 const auto output_path=std::filesystem::weakly_canonical(argv[3]);
 if(output_path==std::filesystem::weakly_canonical(argv[1])||output_path==std::filesystem::weakly_canonical(argv[2]))throw std::runtime_error("output must not overwrite evidence");
 capture::quests::Window window;bool start=false,packets=false;
 std::vector<std::string> interaction_rows;std::size_t interaction_bytes=0;
 read(argv[2],[&](const Json& row,const std::string& line){const auto event=row.at("event").str();
  interaction_bytes+=line.size()+1;
  if(interaction_bytes>64ull*1024*1024)throw std::runtime_error("interaction evidence exceeds 64MiB");
  interaction_rows.push_back(line);
  if(event=="interaction_start"){
   if(start)throw std::runtime_error("multiple interaction starts");start=true;
   window.process=row.at("process_id").u32();window.map=row.at("map").u32();
   window.start=window.end=number(row.at("tick_ms"));
  }else if(event=="interaction_packets_begin")packets=true;
  else if(event=="packet"&&packets){
   if(row.at("process_id").u32()!=window.process)throw std::runtime_error("mixed-process packet evidence");
   window.end=(std::max)(window.end,number(row.at("tick_ms")));
  }
 });
 if(!start||!packets)throw std::runtime_error("interaction markers missing");
 std::vector<capture::quests::Observation> rows;std::vector<std::string> raw;std::size_t retained=0,legacy=0;
 read(argv[1],[&](const Json& row,const std::string& line){const auto event=row.at("event").str();
  if(event!="quest_snapshot_changed"&&event!="npc_visibility_changed")return;
  if(!row.members.contains("process_id")||!row.members.contains("tick_ms")){++legacy;return;}
  retained+=line.size();if(retained>256ull*1024*1024)throw std::runtime_error("retained observations exceed 256MiB");
  rows.push_back({row.at("process_id").u32(),row.at("map").u32(),number(row.at("tick_ms")),number(row.at("instance_epoch")),event=="quest_snapshot_changed"});raw.push_back(line);
 });
 auto selected=capture::quests::select(rows,window);
 std::ofstream out(argv[3],std::ios::binary);if(!out)throw std::runtime_error("output unavailable");
 out<<"{\"event\":\"quest_interaction_join\",\"process_id\":"<<window.process<<",\"map\":"<<window.map<<",\"start_tick_ms\":"<<window.start<<",\"end_tick_ms\":"<<window.end
 <<",\"correlation_only\":true,\"quest_trigger_not_proven\":true,\"legacy_rows_excluded\":"<<legacy<<",\"context_changed\":"<<(selected.context_changed?"true":"false")<<",\"baseline_found\":"<<(selected.baseline?"true":"false")<<"}\n";
 // Preserve NPC identity, schemas, bounded prelude and exact responses in their
 // original order. These are interaction evidence, not attributed quest effects.
 out<<"{\"event\":\"interaction_evidence_begin\",\"correlation_only\":true,\"rows\":"<<interaction_rows.size()<<"}\n";
 for(const auto& line:interaction_rows)out<<line<<'\n';
 out<<"{\"event\":\"interaction_evidence_end\"}\n";
 if(selected.baseline)out<<"{\"event\":\"baseline_begin\",\"context_only\":true}\n"<<raw[*selected.baseline]<<'\n';
 out<<"{\"event\":\"observed_changes_begin\",\"correlation_only\":true}\n";
 std::stable_sort(selected.changes.begin(),selected.changes.end(),[&](auto a,auto b){return rows[a].tick<rows[b].tick;});
 for(auto i:selected.changes)out<<raw[i]<<'\n';
 out.flush();if(!out)throw std::runtime_error("output write failed");
 std::cout<<"Joined "<<selected.changes.size()<<" observations; baseline "<<bool(selected.baseline)<<"; context changed "<<selected.context_changed<<"; legacy excluded "<<legacy<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
