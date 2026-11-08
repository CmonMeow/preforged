#include "../src/runtime/gw_adrenaline.h"
#include <stdexcept>
using namespace fw::gw;
static void check(bool v){if(!v)throw std::runtime_error("adrenaline regression");}
int main(){
 combat::adrenaline::State state;skills::Slots bar{};bar[0]={335,0};bar[1]={336,0};bar[2]={335,1};
 check(state.configure(bar,0));check(state.pools().size()==3&&state.pools()[0].cost==80&&state.pools()[1].cost==200);
 for(unsigned n=0;n<3;++n)state.hit(n+1);check(!state.ready({335,0},3));state.damage(24,480,4);check(state.ready({335,0},4));
 check(state.spend({335,1},4));check(state.pools()[0].units==55&&state.pools()[1].units==55&&state.pools()[2].units==0);
 check(!state.spend({335,0},4));for(unsigned n=0;n<20;++n)state.hit(5+n);check(state.pools()[0].units==80&&state.pools()[1].units==200);
 std::swap(bar[0],bar[1]);check(state.configure(bar,25));check(state.pools()[0].slot.skill==336&&state.pools()[0].units==200);
 check(state.disable({336,0},1000,26));state.hit(27);check(state.pools()[0].units==0);state.hit(1000);check(state.pools()[0].units==25);
 state.tick(25999);check(state.pools()[0].units==25);state.tick(26000);check(state.pools()[0].units==0);
 state.hit(27000);state.tick(100);check(state.pools()[0].units==25);state.reset();check(state.pools()[0].units==0);
 state.gain(200,30000);const auto old=state.pools();bar[3]=bar[0];check(!state.configure(bar,30000));check(state.pools()[0].units==old[0].units);
 check(!state.damage(-1,480,30000)&&!state.damage(10,0,30000));
 const skills::Packet native208={0xd0,0,0x2a,0,0,0,0x19,1,1,0,0,0,0xc8,0,0,0};
 const skills::Packet native209={0xd1,0,0x2a,0,0,0,0x19,1,1,0,0,0};
 check(combat::adrenaline::snapshot_2022(42,{{281,1},200,200})==native208);
 check(combat::adrenaline::spend_2022(42,{281,1})==native209);
 combat::adrenaline::State hammer;skills::Slots hammer_bar{};
 hammer_bar[0]={331,0};hammer_bar[1]={331,1};hammer_bar[2]={336,0};
 check(hammer.configure(hammer_bar,0)&&hammer.pools()[0].cost==130);
 hammer.gain(125,100);check(!hammer.spend_all({331,0},100));
 check(hammer.pools()[0].units==125&&hammer.pools()[1].units==125&&hammer.pools()[2].units==125);
 hammer.gain(5,101);check(hammer.disable({336,0},5000,102));
 check(hammer.spend_all({331,1},103));
 for(const auto& pool:hammer.pools())check(pool.units==0);
 check(hammer.pools()[2].disabled_until==5000);
 hammer.hit(104);check(hammer.pools()[0].units==25&&hammer.pools()[1].units==25&&hammer.pools()[2].units==0);
 check(hammer.configure(hammer_bar,105)&&hammer.pools()[0].units==25);
 hammer.hit(5000);check(hammer.pools()[2].units==25);
}
