#include "../tools/capture/transition_tracker.h"
#include <iostream>
#include <limits>
#include <stdexcept>
void check(bool b){if(!b)throw std::runtime_error("transition tracker regression");}
int main()try{
    using namespace capture;
    TransitionTracker tracker;
    TransitionPoint a{148,10000,0,1,0,10,20,1},b{146,30,2,2,0,30,40,2};
    check(!tracker.observe(true,a,1000)); // late attachment is not an arrival
    a.x=15;check(!tracker.observe(true,a,1100));
    check(!tracker.observe(false,{},1120));
    check(!tracker.observe(false,{},1200));
    check(!tracker.observe(true,{},1300)); // loading complete, agent not ready
    auto event=tracker.observe(true,b,1400);
    check(event&&event->from.x==15&&event->to.x==30&&event->loading_observed&&event->departure_age_ms==20);
    check(!tracker.observe(true,b,1500));
    b.epoch=3;b.instance_ms=5; // same map, different instance
    event=tracker.observe(true,b,1600);check(event&&!event->loading_observed&&event->departure_age_ms==100);
    b.x=std::numeric_limits<float>::quiet_NaN();b.epoch=4;
    check(!tracker.observe(true,b,1700));b.x=40;
    event=tracker.observe(true,b,1800);check(event&&event->from.epoch==3);
    auto json=transition_json(*event);
    check(json.find("\"travel_method\":\"unknown\"")!=std::string::npos&&json.find("\"verified_portal\":false")!=std::string::npos);
    std::cout<<"PASS map transition pairing, loading gaps, instance changes and observation labels\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
