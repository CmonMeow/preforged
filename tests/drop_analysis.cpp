#include "../tools/capture/drop_analysis.h"
#include <stdexcept>
int main(){auto check=[](bool b){if(!b)throw std::runtime_error("drop denominator");};
 capture::drops::Reducer r;r.scope(1);r.metadata(4,10,3,"Foe");
 r.observation(4,10,true,true,false);check(r.kills==0&&r.excluded_initial_dead==1);
 r.observation(4,10,true,false,true);r.observation(4,10,true,true,true);check(r.kills==1);
 r.observation(4,10,true,true,true);check(r.kills==1);
 r.observation(4,10,false,true,false);r.observation(4,10,true,true,false);check(r.kills==1);
 r.scope(2);r.observation(4,10,true,true,true);check(r.kills==1);
 r.metadata(8,12,1,"Friend");r.observation(8,12,true,false,false);r.observation(8,12,true,true,true);check(r.kills==1);
 r.metadata(9,13,3,"Foe");check(r.name_models["Foe"].size()==2);
 auto [lo,hi]=capture::drops::wilson(0,1000);check(lo>=-1e-12&&hi>0&&hi<0.01);
 bool rejected=false;try{capture::drops::wilson(2,1);}catch(...){rejected=true;}check(rejected);
}
