#include "gw_world_simulation.h"
#include "gw_arena_death_time.h"
#include <stdexcept>
using namespace fw::gw;
void check(bool value){if(!value)throw std::runtime_error("arena World death time");}
int main(){
 world::Instance w;
 for(unsigned id:{1,2}){w.join(id,"Player",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{100,100,10,10},100);}
 const auto one=*w.effect_scope(1),two=*w.effect_scope(2);
 check(!w.player_death_times({1,2})[0]);
 check(w.kill(two,1000));check(!w.kill(two,9000));
 check(w.kill(one,12000));
 const auto deaths=w.player_death_times({1,2});check(deaths[0]==12000&&deaths[1]==1000);
 arena::MatchLifecycle match({1,2});check(arena::record_deaths(match,{1,2},deaths)==1);
 check(match.return_deadline()==11000);const auto result=match.poll(15000);check(result&&result->requeue==1);
 check(w.resurrect(two,16000));check(!w.player_death_times({1,2})[1]);
 check(w.kill(two,17000));check(w.player_death_times({1,2})[1]==17000);
 w.leave(2);check(!w.player_death_times({1,2})[1]);
}

