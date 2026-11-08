#include "gw_conditions.h"
#include <iostream>
#include <stdexcept>
int main()try{
    using namespace fw::gw::combat::conditions;
    auto check=[](bool b,const char* s){if(!b)throw std::runtime_error(s);};
    State state;check(state.apply(Kind::weakness,1,100,8000),"condition apply");
    check(state.apply(Kind::weakness,2,200,1000)&&state.has(Kind::weakness,8099),"short reapply preserves longest remainder");
    check(!state.has(Kind::weakness,8100),"exact condition expiration");
    check(state.apply(Kind::weakness,2,8000,2000)&&state.has(Kind::weakness,9999),"long reapply extends timer");
    check(state.apply(Kind::cracked_armor,1,0,5000),"cracked armor applied");
    check(state.earth_armor(100,100)==80&&state.earth_armor(70,100)==60&&state.earth_armor(40,100)==40,"cracked armor minimum60 never boosts low armor");
    check(state.presentation(0x880,100)==0x882,"condition presentation preserves enchantment/hex flags");
    state.apply(Kind::crippled,1,0,2000);state.apply(Kind::poison,1,0,2000);
    check((state.presentation(0,100)&0x4a)==0x4a,"cripple composite and poison bits");
    state.expire(10000);check(state.presentation(0x880,10000)==0x880,"expiry clears only owned condition bits");
    check(!state.apply(Kind::blind,0,0,1000)&&!state.apply(Kind::blind,1,UINT64_MAX,1000),"invalid cause and overflow denied");
    State ordered;check(ordered.apply(Kind::poison,1,100,1000)&&ordered.apply(Kind::blind,2,100,2000),"same-time different conditions");
    check(ordered.newest(100)==Kind::blind&&ordered.count(100)==2,"removal order follows actual insertion");
    check(ordered.apply(Kind::poison,3,200,100)&&ordered.lookup(Kind::poison)->expires==1100&&
        ordered.lookup(Kind::poison)->cause==1&&ordered.newest(200)==Kind::poison,"short reapplication covers without changing longer original source");
    check(ordered.expiry_boundaries(100,3000)==std::vector<std::uint64_t>{1100,2100},"exact sorted expiry partitions");
    check(ordered.degeneration_pips(1099)==-4&&ordered.degeneration_pips(1100)==0,"poison stops on exact expiry boundary");
    check(ordered.apply(Kind::burning,1,2100,1000)&&ordered.apply(Kind::bleeding,1,2100,1000)&&
        ordered.apply(Kind::disease,1,2100,1000)&&ordered.apply(Kind::poison,1,2100,1000)&&ordered.degeneration_pips(2100)==-18,
        "all raw degeneration sources sum before shared health cap");
    check(ordered.count(2000)==1&&!ordered.has(Kind::burning,2000),"future application not active before its source time");
    check(ordered.remove(Kind::poison)&&ordered.degeneration_pips(2100)==-14,"removal stops actual degeneration contribution");
    State casting;
    check(casting.healing_signet(100,2100),"Healing Signet casting interval accepted");
    check(casting.armor(80,99)==80&&casting.armor(80,100)==40&&casting.earth_armor(60,2099)==20&&casting.armor(80,2100)==80,"minus40 only during exact casting interval");
    check(casting.count(100)==0&&casting.presentation(0,100)==0,"casting penalty is not a condition");
    casting.remove(Kind::cracked_armor);check(casting.armor(80,101)==40,"cleansing cannot remove casting penalty");
    check(casting.healing_signet(102,{})&&casting.armor(80,102)==80,"cancel immediately clears casting penalty");
    check(!casting.healing_signet(100,100)&&!casting.healing_signet(100,60101),"invalid casting deadlines denied");
    check(casting.healing_signet(100,2100),"casting reset fixture");casting.clear();check(casting.armor(80,101)==80,"death clears casting penalty");
    std::cout<<"Condition durations, cover order, exact expiry, degeneration, presentation and armor tests passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
