#include "gw_combat.h"
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
using namespace fw::gw::combat;
void require(bool value,const char* text){if(!value)throw std::runtime_error(text);}
void near(double a,double b,const char* text){require(std::abs(a-b)<1e-8,text);}
int main(int argc,char** argv)try{
    if(argc!=2)throw std::runtime_error("retail combat fixture path required");
    Actor enemy{42,7,Health{480,480},60,0,true};const Damage earth{Kind::earth,100,20,194};
    for(const auto kind:{Kind::earth,Kind::lightning,Kind::fire,Kind::cold,Kind::armor_ignoring}){
        auto vulnerable=enemy;vulnerable.lightning_armor=60;vulnerable.fire_armor=60;vulnerable.cold_armor=60;vulnerable.incoming_damage_multiplier=2;
        const auto doubled=apply(vulnerable,{kind,100,20,194},true);
        require(doubled.reason==Reason::accepted,"direct damage vulnerability rejected");near(doubled.damage,200,"direct damage multiplier lost");near(vulnerable.health->current,280,"multiplied health mismatch");
        require(!damage_packets_2022(1,doubled).empty(),"multiplied hit not serializable");
    }
    // Physical touch damage uses reviewed physical armor, never Earth armor.
    auto physical=enemy;physical.earth_armor=100;
    const Damage piercing{Kind::piercing,100,20,2213};
    require(apply(physical,piercing,true).reason==Reason::unknown_armor&&
            physical.health->current==480,"unknown physical armor changed Health");
    physical.physical_armor=60;
    const auto pierced=apply(physical,piercing,true);
    require(pierced.reason==Reason::accepted,"known physical armor rejected");
    near(pierced.damage,100,"piercing borrowed unrelated elemental armor");
    near(physical.health->current,380,"piercing Health mismatch");
    require(!damage_packets_2022(1,pierced).empty(),"piercing hit lacks wire publication");
    auto slash_target=enemy;slash_target.earth_armor=100;
    const Damage slashing{Kind::slashing,150,20,1324};
    require(apply(slash_target,slashing,true).reason==Reason::unknown_armor&&
            slash_target.health->current==480,
            "slashing borrowed elemental armor without reviewed physical armor");
    slash_target.physical_armor=60;
    const auto slashed=apply(slash_target,slashing,true);
    require(slashed.reason==Reason::accepted&&slashed.kind==Kind::slashing,
            "reviewed slashing damage rejected");
    near(slashed.damage,150,"slashing physical-armor calculation changed");
    near(slash_target.health->current,330,"slashing Health mismatch");
    require(!damage_packets_2022(1,slashed).empty(),
            "slashing hit lacks wire publication");
    physical=enemy;physical.physical_armor=100;
    physical.elemental_protection=Kind::earth;physical.elemental_multiplier=.5;
    const auto resisted=apply(physical,piercing,true);
    near(resisted.damage,50,"physical armor/protection ordering incorrect");
    physical=enemy;physical.physical_armor=60;physical.incoming_reduction=20;
    const auto reduced=apply(physical,piercing,true);
    near(reduced.damage,80,"flat reduction not applied to piercing");
    for(const double invalid:{0.,-1.,17.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
        auto unchanged=enemy;unchanged.incoming_damage_multiplier=invalid;
        require(apply(unchanged,earth,true).reason==Reason::invalid&&unchanged.health->current==480,"invalid multiplier mutated health");
    }
    auto overkill=enemy;overkill.incoming_damage_multiplier=2;
    const auto amplified_lethal=apply(overkill,{Kind::armor_ignoring,300,20,194},true);
    require(amplified_lethal.killed&&amplified_lethal.damage==600&&amplified_lethal.applied==480&&overkill.health->current==0&&!damage_packets_2022(1,amplified_lethal).empty(),"multiplier overkill accounting failed");
    require(amount(earth,60).has_value(),"known earth armor missing");
    near(*amount(earth,60),100,"earth baseline");near(*amount(earth,100),50,"+40 armor must halve earth damage");
    near(*amount(earth,20),200,"-40 armor must double earth damage");
    near(*amount(Damage{Kind::earth,100,10,194},30),100,"skill strike level must use caster level");
    near(*amount(Damage{Kind::earth,100,28,194},84),100,"level28 NPC uses its actual strike level rather than player cap");
    require(!amount(earth,std::nullopt),"unknown earth armor fabricated");
    near(*amount(Damage{Kind::armor_ignoring,120,20,194},std::nullopt),120,"armor-ignore incorrectly requires armor");
    const auto initial=health_packets_2022(enemy);const auto hit=apply(enemy,earth,true);
    require(hit.reason==Reason::accepted&&!hit.killed&&hit.generation==7,"earth hit rejected or lifetime lost");
    near(enemy.health->current,380,"authoritative damage not applied");
    auto packets=initial;const auto damage=damage_packets_2022(43,hit);packets.insert(packets.end(),damage.begin(),damage.end());
    const auto ignore=apply(enemy,{Kind::armor_ignoring,120,20,194},true);
    const auto ignored=damage_packets_2022(43,ignore);packets.insert(packets.end(),ignored.begin(),ignored.end());
    near(enemy.health->current,260,"armor-ignore health outcome");
    Actor fragile{42,9,Health{20,480},60,0x2000,true};const auto lethal=apply(fragile,{Kind::earth,500,20,194},true);
    require(lethal.killed&&fragile.effects==0x2010&&fragile.health->current==0,"lethal result lost effects/death");
    const auto killed=damage_packets_2022(43,lethal);packets.insert(packets.end(),killed.begin(),killed.end());
    require(killed.size()==4,"death packet missing");near(lethal.damage,500,"raw overkill damage truncated");near(lethal.applied,20,"applied overkill damage not clamped");
    const auto repeated=apply(fragile,earth,true);require(repeated.reason==Reason::dead&&damage_packets_2022(43,repeated).empty(),"dead target hit again");
    std::ifstream fixtures(argv[1]);require(bool(fixtures),"retail fixtures missing");
    for(auto packet:packets){
        packet[1]|=0x80;std::string hex;require(bool(fixtures>>hex),"retail fixture missing");Packet expected;
        for(std::size_t i=0;i<hex.size();i+=2)expected.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
        require(packet==expected,"pinned retail combat serializer mismatch");
    }
    std::string extra;require(!(fixtures>>extra),"unexpected fixture count");
    auto unchanged=[&](Actor actor,Damage request,bool hostile,Reason reason){
        const auto before=actor;const auto rejected=apply(actor,request,hostile);
        require(rejected.reason==reason&&actor.health.has_value()==before.health.has_value()&&actor.effects==before.effects,"rejected damage mutated state");
        if(before.health)require(std::bit_cast<std::uint64_t>(actor.health->current)==std::bit_cast<std::uint64_t>(before.health->current)&&actor.health->maximum==before.health->maximum,"rejected damage changed health");
    };
    unchanged(enemy,earth,false,Reason::not_hostile);
    auto unknown=enemy;unknown.health.reset();unchanged(unknown,earth,true,Reason::unknown_health);
    unknown=enemy;unknown.earth_armor.reset();unchanged(unknown,earth,true,Reason::unknown_armor);
    unknown=enemy;unknown.damageable=false;unchanged(unknown,earth,true,Reason::not_damageable);
    unknown=enemy;unknown.health=Health{481,480};unchanged(unknown,earth,true,Reason::invalid);
    unknown=enemy;unknown.health=Health{std::numeric_limits<double>::quiet_NaN(),480};unchanged(unknown,earth,true,Reason::invalid);
    for(auto bad: {Damage{Kind::earth,std::numeric_limits<double>::quiet_NaN(),20,194},Damage{Kind::earth,-1,20,194},
                  Damage{Kind::earth,1000001,20,194},Damage{Kind::earth,100,0,194},Damage{Kind::earth,100,256,194},Damage{Kind::earth,100,20,3433}})
        unchanged(enemy,bad,true,Reason::invalid);
    for(auto ar:{-101,maximum_effective_armor+1}){unknown=enemy;unknown.earth_armor=ar;unchanged(unknown,earth,true,Reason::invalid);}
    unknown=enemy;unknown.earth_armor=-100;
    unchanged(unknown,{Kind::earth,1000000,255,194},true,Reason::invalid);
    Actor fraction{44,1,Health{10,10},60,0,true};
    for(unsigned i=0;i<10;++i)require(apply(fraction,{Kind::earth,.25,20,194},true).reason==Reason::accepted,"fractional hit rejected");
    near(fraction.health->current,7.5,"fractional damage rounded away or accumulated incorrectly");
    auto zero=apply(fraction,{Kind::earth,0,20,194},true);require(zero.reason==Reason::accepted&&damage_packets_2022(43,zero).empty(),"zero damage emitted an event");
    // A successful cast over an empty target set belongs to the AoE caller:
    // these per-target helpers never manufacture a victim to produce success.
    std::cout<<"PASS pinned retail combat fixtures, fractional earth/armor-ignore damage, unknown-state gates, hostile ownership and death/overkill bounds\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
