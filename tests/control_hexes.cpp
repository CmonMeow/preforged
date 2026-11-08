#include "gw_control_hexes.h"
#include <stdexcept>
void check(bool v){if(!v)throw std::runtime_error("control hex regression");}
int main(){using namespace fw::gw::skills;using namespace effects;
 for(unsigned id:{35u,56u}){
    const auto zero=*control_hex(id,0),twelve=*control_hex(id,12),max=*control_hex(id,20);
    check(zero.duration==8000&&twelve.duration==18000&&max.duration==24000);
    check(twelve.radius==(id==56?166.f:0.f)&&twelve.range==1248);
    check(!control_hex(id,21));
 }
 check(!control_hex(36));
 const auto& signet=*find_skill(132);const auto& spell=*find_skill(281);
 const auto& attack=*find_skill(397);
 std::vector<Entry> entries{{1,2,35,12,Category::hex,100,18100}};
 check(!signet_forbidden(signet,entries,99));
 check(signet_forbidden(signet,entries,100));
 check(!signet_forbidden(spell,entries,100)&&!signet_forbidden(attack,entries,100));
 check(!signet_forbidden(signet,entries,18100));
 check(!adrenaline_forbidden(entries,100));
 entries.push_back({2,2,56,12,Category::hex,100,18100});
 check(adrenaline_forbidden(entries,100)&&!adrenaline_forbidden(entries,18100));
 entries[1].kind=Category::enchantment;check(!adrenaline_forbidden(entries,100));
 // Vocal Minority admission: exact lifetime boundaries and both vocal types.
 for(unsigned rank=0;rank<=20;++rank){
    const auto plan=control_hex(883,rank);
    check(plan&&plan->duration==1000ull*(5+rank)&&plan->radius==252.f&&plan->range==1248.f);
 }
 check(!control_hex(883,21));
 const auto& shout=*find_skill(2216);const auto& chant=*find_skill(1553);
 check(shout.type==15&&chant.type==27);
 entries={{3,2,883,12,Category::hex,100,17100}};
 for(const auto* vocal:{&shout,&chant}){
    check(!control_skill_forbidden(*vocal,entries,99));
    check(control_skill_forbidden(*vocal,entries,100));
    check(control_skill_forbidden(*vocal,entries,17099));
    check(!control_skill_forbidden(*vocal,entries,17100));
 }
 for(const auto* ordinary:{&signet,&spell,&attack})check(!control_skill_forbidden(*ordinary,entries,100));
 auto invalid=entries;
 invalid[0].kind=Category::enchantment;check(!vocal_forbidden(shout,invalid,100));
 invalid=entries;invalid[0].cause=0;check(!vocal_forbidden(shout,invalid,100));
 invalid=entries;invalid[0].rank=21;check(!vocal_forbidden(shout,invalid,100));
 invalid=entries;invalid[0].expires.reset();check(!vocal_forbidden(shout,invalid,100));
 entries.clear();check(!control_skill_forbidden(shout,entries,100));
 entries={{4,2,35,12,Category::hex,100,18100}};
 check(control_skill_forbidden(signet,entries,100));
 check(!control_skill_forbidden(shout,entries,100)&&!control_skill_forbidden(chant,entries,100));
}
