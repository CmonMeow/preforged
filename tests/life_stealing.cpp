#include "gw_life_stealing.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
void check(bool ok){if(!ok)throw std::runtime_error("life-stealing regression");}
int main(){using namespace fw::gw::skills::effects;
 check(life_steal(153,12)->amount==52&&life_steal(156,12)->amount==65&&life_steal(1077,12)->amount==65&&life_steal(156)->range==144&&!life_steal(153,21)&&!life_steal(152));
 auto hit=steal_life(*life_steal(153,12),{100,480},{480,480});check(hit&&hit->stolen==52&&hit->source.current==152&&hit->target.current==428&&!hit->killed);
 auto lethal=steal_life(*life_steal(156,12),{100,480},{30,480});check(lethal&&lethal->stolen==31&&lethal->source.current==131&&lethal->target.current==0&&lethal->killed);
 auto capped=steal_life(*life_steal(156,12),{479,480},{480,480});check(capped&&capped->stolen==65&&capped->gained==1&&capped->target.current==415);
 auto full=steal_life(*life_steal(156,12),{480,480},{480,480});check(full&&full->gained==0&&full->target.current==415&&life_steal_packets(1,2,*full).size()==3);
 auto forged=*life_steal(153);forged.amount=9999;check(!steal_life(forged,{100,480},{480,480})&&!steal_life(*life_steal(153),{0,480},{480,480})&&!steal_life(*life_steal(153),{100,480},{0,480}));
 std::ifstream fixtures(FIXTURE_FILE);check(bool(fixtures));for(unsigned index=0;index<3;++index){auto result=steal_life(*life_steal(index?156:153,12),{100,480},{index==2?30.0:480.0,480});check(bool(result));for(auto packet:life_steal_packets(1,2,*result)){std::string line;check(bool(std::getline(fixtures,line)));std::vector<unsigned char> expected;check(line.size()%2==0);for(std::size_t at=0;at<line.size();at+=2)expected.push_back(static_cast<unsigned char>(std::stoul(line.substr(at,2),nullptr,16)));packet[1]|=0x80;check(packet==expected);}}
}
