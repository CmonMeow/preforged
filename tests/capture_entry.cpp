#include "../tools/capture/entry_tracker.h"
#include "../tools/capture/local_observation_gate.h"
#include <limits>
#include <stdexcept>
#include <iostream>
int main()try{
 const auto accept=[](bool observing,unsigned id,unsigned model,float x,float y,float facing){return capture::valid_local_observation(observing,id,static_cast<std::uint16_t>(model),x,y,facing);};
 if(!accept(false,42,0x3000,1,2,3))throw std::runtime_error("normal controlled actor rejected");
 if(accept(true,42,0x3000,1,2,3)||accept(false,0,0x3000,1,2,3)||accept(false,42,0x2000,1,2,3))throw std::runtime_error("observer, absent actor, or NPC admitted");
 const float bad=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
 if(accept(false,42,0x3000,bad,2,3)||accept(false,42,0x3000,1,inf,3)||accept(false,42,0x3000,1,2,bad))throw std::runtime_error("invalid local coordinates admitted");
 capture::EntryTracker t;
 if(t.observe(true,true,false,100))throw std::runtime_error("mid-map attach captured");
 t.observe(false,false,false,200);
 if(t.observe(true,false,false,300)||!t.observe(true,true,false,301)||t.observe(true,true,false,302))throw std::runtime_error("entry must be captured once when local character arrives");
 t.observe(false,false,false,400);
 if(t.observe(true,true,true,500)||t.observe(true,true,false,501))throw std::runtime_error("movement must disarm capture");
 t.observe(false,false,false,600);t.observe(true,false,false,700);
 if(t.observe(true,true,false,2701))throw std::runtime_error("late position accepted");
 t.observe(false,false,false,3000);
 if(!t.observe(true,true,false,3100))throw std::runtime_error("same-map reentry not armed");
 std::cout<<"PASS local entry gating\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
