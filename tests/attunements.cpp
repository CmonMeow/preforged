#include "gw_attunements.h"
#include "gw_skill_casting.h"
#include <fstream>
#include <string>
#include <stdexcept>
void check(bool ok){if(!ok)throw std::runtime_error("attunement regression");}
int main(){using namespace fw::gw::skills;using namespace effects;
 check(attunement(169,12)->duration==55000&&attunement(184,12)->attribute==10&&attunement(208,12)->attribute==11&&attunement(225,12)->attribute==8&&attunement(164,12)->attribute_bonus==2&&attunement(164,12)->duration==53000&&!attunement(164,21)&&!attunement(180));
 check(standard_attunement_refund(5)==2&&standard_attunement_refund(15)==6&&standard_attunement_refund(25)==8);
 std::vector<Entry> active{{1,50,184,12,Category::enchantment,100,55100},{2,50,164,12,Category::enchantment,100,53100}};
 check(attunement_cast_refund(active,*find_skill(191),100)==9&&attunement_cast_refund(active,*find_skill(1379),100)==4&&attunement_cast_refund(active,*find_skill(169),100)==5&&attunement_cast_refund(active,*find_skill(153),100)==0);
 check(elemental_attribute_bonus(active,100)==2&&elemental_attribute_bonus(active,53100)==0&&attunement_cast_refund(active,*find_skill(191),55100)==0&&attunement_cast_refund(active,*find_skill(191),99)==0);
 active.push_back(active[0]);check(attunement_cast_refund(active,*find_skill(191),100)==9);

 for(unsigned id:{169u,184u,208u,225u,164u}){
  casting::Runtime runtime(50);Policy policy;policy.primary=6;policy.secondary=7;policy.campaigns=0x1e;policy.can_change=true;policy.learned={{id,1}};Slots bar{};bar[0]={id,0};fw::gw::attributes::Ranks ranks{};ranks[8]=ranks[9]=ranks[10]=ranks[11]=ranks[12]=12;
  fw::gw::vitals::Resources self{480,480,60,60};Registry registry({1,50,1});unsigned refunded=0;casting::Hooks hooks;
  hooks.resources=[&](unsigned actor)->std::optional<fw::gw::vitals::Resources>{return actor==50?std::optional(self):std::nullopt;};hooks.target_valid=[](unsigned actor,float,bool){return actor==50;};hooks.consume_energy=[&](unsigned n){self.energy-=n;return true;};hooks.restore_energy=[&](unsigned n){refunded+=n;self.energy+=n;return true;};hooks.events=[](const auto&){};
  hooks.status_mutation=[&](unsigned skill,unsigned effect,unsigned rank,std::uint64_t now,std::optional<std::uint64_t> expires,bool add){if(add)return registry.apply({1,50,1},{effect,50,skill,rank,Category::enchantment,now,expires},Replace::same_skill).accepted;registry.remove({1,50,1},effect);return true;};
  hooks.effect_snapshot=[&](std::uint64_t now)->std::optional<std::vector<Entry>>{return registry.active({1,50,1},now);};
  check(runtime.start(0,{id,0,999},bar,policy,ranks,true,hooks)==casting::Reason::accepted);runtime.update(1000,hooks);
  check(refunded==(id==164?0u:4u)&&registry.active({1,50,1},1000).size()==1);
  auto entry=registry.active({1,50,1},1000).front();registry.remove({1,50,1},entry.instance);runtime.update(2000,hooks);
  check(registry.active({1,50,1},2000).empty()&&registry.active({1,50,2},2000).empty());
  check(attunement_cast_refund(*hooks.effect_snapshot(2000),*find_skill(id),2000)==0);
 }
 std::ifstream fixture(FIXTURE_FILE);check(bool(fixture));unsigned instance=0;for(unsigned id:{169u,184u,208u,225u,164u}){++instance;for(auto packet:{casting::effect_add(50,id,12,instance,float(attunement(id,12)->duration)/1000),casting::effect_remove(50,instance)}){std::string line;check(bool(std::getline(fixture,line)));std::vector<unsigned char> expected;for(std::size_t at=0;at<line.size();at+=2)expected.push_back(static_cast<unsigned char>(std::stoul(line.substr(at,2),nullptr,16)));packet[1]|=0x80;check(packet==expected);}}
}
