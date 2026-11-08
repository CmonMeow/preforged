#include "../tools/capture/quest_interaction_join.h"
#include <stdexcept>
int main(){
 using namespace capture::quests;
 auto check=[](bool value){if(!value)throw std::runtime_error("quest interaction scope");};
 std::vector<Observation> rows{{1,148,90,5,true},{2,148,95,5,true},{1,148,110,5,false},{1,148,120,5,true},{1,148,130,5,false}};
 auto joined=select(rows,{1,148,100,125});check(joined.baseline==0&&joined.changes==std::vector<std::size_t>{2,3});
 rows.push_back({1,148,99,6,false});joined=select(rows,{1,148,100,125});check(joined.context_changed&&!joined.baseline&&joined.changes.empty());rows.pop_back();
 rows.push_back({1,146,99,6,true});joined=select(rows,{1,148,100,125});check(!joined.baseline&&joined.changes.size()==2);
 rows.push_back({1,148,124,7,false});joined=select(rows,{1,148,100,125});check(joined.context_changed&&!joined.baseline&&joined.changes.empty());
 bool rejected=false;try{select(rows,{0,148,100,125});}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 rejected=false;try{select(rows,{1,148,125,100});}catch(const std::invalid_argument&){rejected=true;}check(rejected);
}
