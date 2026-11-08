#include "gw_passive_recovery.h"
#include "gw_health_regeneration.h"
#include <iostream>
int main()try{
 using namespace fw::gw;using namespace recovery;
 auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 // Test schedule is deliberately a fixture, not asserted official timing.
 const Schedule fixture{{3000,4000,5000,6000,7000,8000,9000}};
 State state(fixture,100);check(state.pips(3099)==0&&state.pips(3100)==1&&state.pips(9100)==7,"ramp exact boundaries and seven cap");
 auto slices=state.segments(10100);check(slices.size()==8&&slices.front().pips==0&&slices.back().pips==7,"delayed tick splits every threshold");
 double health=100;skills::effects::Scope owner{1,2,3};skills::regeneration::State timed(owner,100);
 for(auto s:slices)check(timed.advance(owner,s.end,health,480,int(s.pips)),"timed-source integration");
 check(health==156,"piecewise automatic health contribution44+14");
 // Natural recovery is seven; the combined effective cap is ten, after
 // regeneration and degeneration have both contributed.
 skills::regeneration::State combined(owner,0);
 check(combined.apply(owner,{1,2,288,3,0,10000}),"additional three-pip source");
 double combined_health=100;
 check(combined.advance(owner,1000,combined_health,480,7)&&combined_health==120,"natural seven plus three gives ten pips");
 check(combined.apply(owner,{2,2,446,6,1000,10000}),"additional regeneration above cap");
 check(combined.advance(owner,2000,combined_health,480,7)&&combined_health==140,"combined regeneration caps at ten");
 check(combined.advance(owner,3000,combined_health,480,7-8)&&combined_health==156,"degeneration offsets all sources before cap");
 check(!state.record(Event::attack,10100),"event cannot erase unintegrated history");check(state.commit(10100)&&state.record(Event::attack,10100)&&state.pips(10100)==0,"attack reset after integration");
 check(state.commit(13100)&&state.pips(13100)==1,"recovery after combat quiet");
 for(auto event:{Event::self_heal,Event::pbaoe,Event::enemy_shout,Event::zero_damage_received})check(state.record(event,13100)&&state.pips(13100)==1,"noncombat event preserves automatic ramp");
 for(auto event:{Event::health_loss,Event::attack,Event::target_enemy_spell_or_signet,Event::targeted_by_enemy_spell_or_signet}){check(state.record(event,13100)&&state.pips(13100)==0,"combat event resets even canceled targeted spell");}
 check(state.commit(17100)&&state.inhibit(17100)&&state.pips(17100)==0,"ongoing health loss inhibits recovery");
 bool rejected=false;try{State bad(Schedule{{1,1,2,3,4,5,6}},0);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"schedule strictly increasing");
 rejected=false;try{state.segments(0);}catch(const std::invalid_argument&){rejected=true;}check(rejected&&!state.commit(0),"monotonic clock");
 State large(fixture,UINT64_MAX-2);check(large.segments(UINT64_MAX).size()==1&&large.pips(UINT64_MAX)==0,"clock overflow cannot create early ramp");
 std::cout<<"passive recovery PASS\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
