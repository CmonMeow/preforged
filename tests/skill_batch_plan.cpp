#include "../tools/skills/batch_template.h"
#include "../src/runtime/gw_player_skills_generated.h"
#include "../src/runtime/gw_skill_catalog_generated.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <vector>
void require(bool b,const char* message){if(!b)throw std::runtime_error(message);}
std::vector<std::string> split(const std::string& s){std::vector<std::string> out;std::istringstream in(s);std::string v;while(std::getline(in,v,'\t'))out.push_back(v);return out;}
int main(int argc,char** argv){try{
    require(argc==2,"output directory required");std::filesystem::path dest=argv[1];std::ifstream mf(dest/"skill-manifest.tsv"),cf(dest/"ALL-OBTAINABLE-BUILD-CODES.txt"),lf(dest/"capture-batches.tsv");require(bool(mf)&&bool(cf)&&bool(lf),"output files missing");
    std::map<unsigned,bool> pve,learned;std::string line;std::getline(mf,line);unsigned rows=0;while(std::getline(mf,line)){auto c=split(line);require(c.size()==13,"manifest columns");unsigned id=std::stoul(c[0]);pve[id]=c[5]=="1";learned[id]=c[3]=="yes";++rows;}require(rows==3495,"modern manifest row count");
    std::set<unsigned> expected(fw::gw::skills::obtainable_player_skills.begin(),fw::gw::skills::obtainable_player_skills.end()),actual,actual_learned;
    unsigned batches=0;while(std::getline(cf,line))if(line.starts_with("[ ] Batch ")){auto code=line.substr(line.rfind(" | ")+3);auto build=skillbatch::decode(code);unsigned elite=0,pve_count=0,faction=0;++batches;
        for(auto id:build.skills)if(id){require(expected.contains(id),"nonobtainable ID planned");require(actual.insert(id).second,"duplicate ID planned");auto& row=fw::gw::skills::skill_catalog_2022[id];require(row.profession==0||row.profession==build.primary||row.profession==build.secondary,"incompatible profession");elite+=row.elite();pve_count+=pve.at(id);unsigned side=(id>=1948&&id<=1955)||id==1957||id==2051?6:(id>=2091&&id<=2100?5:0);require(!side||!faction||side==faction,"mixed allegiance skills");if(side)faction=side;}
        require(elite<=1,"multiple elites");require(pve_count<=3,"too many PvE skills");
    }require(actual==expected,"full obtainable coverage mismatch");
    std::getline(lf,line);while(std::getline(lf,line)){auto c=split(line);require(c.size()==15,"learned batch columns");auto b=skillbatch::decode(c[6]);for(auto id:b.skills)if(id){require(learned.at(id),"unlearned ID in learned-only plan");require(actual_learned.insert(id).second,"duplicate learned skill");}}
    std::set<unsigned> expected_learned;for(auto id:expected)if(learned.at(id))expected_learned.insert(id);require(expected_learned==actual_learned,"learned coverage mismatch");
    std::cout<<"planner tests passed: "<<actual.size()<<" obtainable IDs in "<<batches<<" bars, "<<actual_learned.size()<<" learned-only IDs\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
