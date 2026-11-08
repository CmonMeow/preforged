#include "gw_dwarven_battle_stance.h"
#include "gw_dolyak_signet.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool b){if(!b)throw std::runtime_error("Dwarven policy");}
int main()try{
 for(unsigned r=0;r<=20;++r){auto p=dwarven_battle_stance(r);check(p&&p->armor==40&&p->attack_speed_percent==33&&p->duration==1000ull*interpolate(5,11,r));}check(!dwarven_battle_stance(21));
 std::vector<Entry> entries{{123,25,375,12,Category::stance,100,10100}};
 check(dwarven_battle_bonuses(entries,true,100)==DwarvenBattleBonuses{true,40});
 check(dwarven_battle_bonuses(entries,false,100)==DwarvenBattleBonuses{});
 check(!dwarven_battle_bonuses(entries,{},100));
 check(dwarven_battle_bonuses(entries,{},99)==DwarvenBattleBonuses{});
 check(dwarven_battle_bonuses(entries,{},10100)==DwarvenBattleBonuses{});
 check(dwarven_attack_interrupt(entries,true,331,100)==true);
 check(dwarven_attack_interrupt(entries,true,0,100)==false);
 check(dwarven_attack_interrupt(entries,false,331,100)==false);
 check(dwarven_attack_interrupt(entries,true,281,100)==false);
 check(dwarven_attack_interrupt(entries,true,437,100)==false);
 check(!dwarven_attack_interrupt(entries,true,3433,100));
 check(stacked_skill_armor(40,20)==40&&stacked_skill_armor(40,60)==60);
 entries[0].rank=21;check(!dwarven_battle_bonuses(entries,true,100));
 std::cout<<"PASS Dwarven pinned policy,conditional hammer authority,expiry,armor stacking and attack-skill interrupt eligibility\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
