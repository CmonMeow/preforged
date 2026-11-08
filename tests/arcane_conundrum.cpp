#include "gw_arcane_conundrum.h"
#include "gw_activation_modifiers.h"
#include <stdexcept>
void check(bool v){if(!v)throw std::runtime_error("Arcane Conundrum lifecycle regression");}
int main(){using namespace fw::gw::skills::effects;
 const auto zero=*arcane_conundrum(0),twelve=*arcane_conundrum(12),max=*arcane_conundrum(20);
 check(zero.duration==5000&&zero.energy==1);
 check(twelve.duration==13000&&twelve.energy==6&&twelve.radius==166&&twelve.range==1248);
 check(max.duration==18000&&max.energy==9&&!arcane_conundrum(21));
 ArcaneConundrumReturns returns;
 Scope caster{1,1,4},first{1,2,8},second{1,3,9};
 check(returns.add({caster,first,10,6,100,13100}));
 check(!returns.add({caster,first,10,6,100,13100}));
 check(returns.add({caster,second,11,6,100,13100}));
 check(!returns.end(first,10,99,true)&&returns.size()==2);
 check(!returns.end({1,2,9},10,13100,true)&&returns.size()==2);
 const auto reward=returns.end(first,10,13100,true);
 check(reward&&reward->caster==caster&&reward->energy==6);
 check(!returns.end(first,10,13100,true));
 // A separate affected foe earns its own reward, exactly once.
 const auto other=returns.end(second,11,1000,true);
 check(other&&other->caster==caster&&other->energy==6&&returns.size()==0);
 check(returns.add({caster,first,12,6,100,13100}));
 check(!returns.end(first,12,200,false)&&returns.size()==0);
 check(!returns.add({caster,{2,2,8},13,6,100,13100}));
 check(!returns.add({caster,first,13,6,100,100}));
 check(!returns.add({caster,first,13,10,100,13100}));
 check(returns.add({caster,first,14,6,100,13100}));
 returns.discard({1,1,5});check(returns.size()==1);
 returns.discard(caster);check(returns.size()==0);
 fw::gw::combat::conditions::State conditions;
 std::vector<Entry> hexes{{1,1,36,12,Category::hex,100,13100}};
 const auto* spell=fw::gw::skills::find_skill(281);
 const auto* signet=fw::gw::skills::find_skill(132);
 const auto* touch=fw::gw::skills::find_skill(154);
 const auto* attack=fw::gw::skills::find_skill(397);
 check(activation_multiplier(*spell,hexes,conditions,100)==2);
 check(activation_multiplier(*signet,hexes,conditions,100)==1);
 check(activation_multiplier(*touch,hexes,conditions,100)==1);
 check(activation_multiplier(*attack,hexes,conditions,100)==1);
 check(activation_multiplier(*spell,hexes,conditions,99)==1);
 check(activation_multiplier(*spell,hexes,conditions,13100)==1);
 // Repeated same-name instances are one modifier, while a distinct Migraine
 // combines multiplicatively and then respects the shared activation cap.
 hexes.push_back({2,2,36,0,Category::hex,100,13100});
 check(activation_multiplier(*spell,hexes,conditions,100)==2);
 hexes.push_back({3,3,53,12,Category::hex,100,13100});
 check(activation_multiplier(*spell,hexes,conditions,100)==2.5);
 check(activation_multiplier(*signet,hexes,conditions,100)==2);
 check(activation_multiplier(*attack,hexes,conditions,100)==1);
}
