#include "gw_aegis.h"
#include "gw_block_attribution.h"
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Aegis policy");}
int main(){
 for(unsigned r=0;r<=20;++r){const auto p=aegis(r);check(p&&p->duration==1000ull*interpolate(5,11,r)&&p->block==.5);}
 check(!aegis(21));const auto p=*aegis(12);const Scope owner{1,1,1};
 std::array<AegisMember,5> members{{{owner,0,true,true},{{1,2,1},1012,true,true},
  {{1,3,1},1012.01,true,true},{{1,4,1},10,true,false},{{1,5,1},10,false,true}}};
 auto result=aegis_members(p,owner,members);check(result&&result->size()==2&&result->front()==owner);
 auto changed=members;changed[1].scope=owner;check(!aegis_members(p,owner,changed));
 changed=members;changed[0].scope.generation=2;check(!aegis_members(p,owner,changed));
 changed=members;changed[0].same_party=false;check(!aegis_members(p,owner,changed));
 changed=members;changed[1].distance=INFINITY;check(!aegis_members(p,owner,changed));
 auto forged=p;forged.range=5000;check(!aegis_members(forged,owner,members));
 Registry registry(owner);check(registry.apply(owner,{100,1,257,12,Category::enchantment,100,10100},Replace::same_skill).accepted);
 auto entries=registry.active(owner,100);
 check(reviewed_block_probability(entries,false,100)==.5);
 const auto sources=reviewed_block_sources(entries,100);check(sources&&sources->size()==1&&sources->front().skill==257);
 entries.push_back({101,1,258,0,Category::enchantment,100,2100});
 check(reviewed_block_probability(entries,false,100)==.75);
 check(reviewed_block_probability(entries,true,100)==0);
 entries.push_back({102,2,137,0,Category::hex,100,8100});
 check(reviewed_block_probability(entries,false,100)==0);
 check(reviewed_block_probability(registry.active(owner,10100),false,10100)==0);
}
