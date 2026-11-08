#include "gw_shield_bash.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){
 if(!b)throw std::runtime_error("Shield Bash line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned r=0;r<=20;++r){const auto p=shield_bash(r);check(p&&p->rank==r&&p->duration==1000ull*interpolate(5,11,r)&&p->energy==5&&p->recharge==20000&&p->additional_disable_ms==15000);}check(!shield_bash(21));
 check(shield_bash_attack_skill(0)==false&&shield_bash_attack_skill(340)==true&&shield_bash_attack_skill(399)==true&&shield_bash_attack_skill(281)==false&&!shield_bash_attack_skill(3433));
 for(const auto& row:skill_catalog_2022)if(row.type==20)check(shield_bash_attack_skill(row.id)==false);
 const Scope owner{1,25,1},attacker{1,26,1};Registry registry(owner);
 check(registry.apply(owner,{110,25,363,0,Category::other,100,5100},Replace::same_skill).accepted);
 const ShieldEquipment shield{1,123},none{2,0};
 check(shield_bash_protection(registry,owner,100,shield,340)==ShieldBashProtection{110});
 check(shield_bash_protection(registry,owner,100,none,340)==ShieldBashProtection{});
 check(!shield_bash_protection(registry,owner,100,{},340));
 check(!shield_bash_protection(registry,{1,25,2},100,shield,340));
 for(auto at:{99ull,5100ull})check(shield_bash_protection(registry,owner,at,{},340)==ShieldBashProtection{});
 check(shield_bash_protection(registry,owner,100,{},0)==ShieldBashProtection{});
 ShieldBashContact c{{attacker,owner,1,90,100,combat::weapon::AttackOutcome::block,{},combat::weapon::AttackDelivery::melee},{340,7},110,false};
 const auto p=prepare_shield_bash_contact(registry,owner,shield,c);
 check(p&&p->knockdown&&p->additional_disable_ms==15000&&p->skill==Slot{340,7}&&p->event==1&&p->attacker==attacker);
 check(registry.lookup(owner,110).has_value()&&!p->registry.lookup(owner,110));
 check(!prepare_shield_bash_contact(p->registry,owner,shield,c));
 check(!prepare_shield_bash_contact(registry,owner,none,c));
 auto bad=c;bad.attributed_effect=111;check(!prepare_shield_bash_contact(registry,owner,shield,bad));
 bad=c;bad.attack.target.generation++;check(!prepare_shield_bash_contact(registry,owner,shield,bad));
 bad=c;bad.attack.attacker.generation=0;check(!prepare_shield_bash_contact(registry,owner,shield,bad));
 bad=c;bad.skill={0,0};check(!prepare_shield_bash_contact(registry,owner,shield,bad));
 bad=c;bad.skill.copy=65536;check(!prepare_shield_bash_contact(registry,owner,shield,bad));
 for(auto outcome:{combat::weapon::AttackOutcome::hit,combat::weapon::AttackOutcome::miss,combat::weapon::AttackOutcome::dodge,combat::weapon::AttackOutcome::obstruction,combat::weapon::AttackOutcome::cancelled}){bad=c;bad.attack.outcome=outcome;check(!prepare_shield_bash_contact(registry,owner,shield,bad));}
 auto ranged=c;ranged.skill={399,2};ranged.attack.delivery=combat::weapon::AttackDelivery::ranged;
 const auto rp=prepare_shield_bash_contact(registry,owner,shield,ranged);check(rp&&!rp->knockdown&&!rp->additional_disable_ms);
 bad=c;bad.skill={399,2};check(!prepare_shield_bash_contact(registry,owner,shield,bad));
 bad=ranged;bad.skill={340,7};check(!prepare_shield_bash_contact(registry,owner,shield,bad));
 auto bypass=c;bypass.attributed_effect=0;bypass.unblockable_melee=true;bypass.attack.outcome=combat::weapon::AttackOutcome::hit;bypass.attack.before_damage=100;
 check(prepare_shield_bash_contact(registry,owner,shield,bypass)->knockdown);
 bypass.attack.delivery=combat::weapon::AttackDelivery::ranged;check(!prepare_shield_bash_contact(registry,owner,shield,bypass));
 bypass=c;bypass.unblockable_melee=true;check(!prepare_shield_bash_contact(registry,owner,shield,bypass));
 std::cout<<"PASS Shield Bash native policy, shield/skill/lifetime admission, exact-copy block consumption and melee/bypass proposals\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
