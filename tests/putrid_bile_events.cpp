#include "gw_putrid_bile_events.h"
#include <iostream>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Putrid Bile immutable death ownership");}
int main()try{
 const Scope caster{1,1,1},dead{1,2,2},recipient{1,3,3};
 const PutridBileLink link{caster,dead,80,12,20,100,1000};
 PutridBileLinks links;PutridBileEvents events;check(links.assign(link));
 const auto owned=links.take_death(dead,999);check(owned.has_value());
 const auto event=events.reserve(*owned,0,10,20,999,{recipient});check(event&&events.size()==1);
 links.expire(5000);check(!links.lookup(dead)&&events.lookup(event->serial)==event);
 check(events.reserve(*owned,0,10,20,999,{recipient})==event&&events.size()==1);
 check(!events.first_due(998)&&events.first_due(5000)==event);
 check(!events.first_due(5000,{event->serial}));
 auto forged=*event;++forged.link.target.generation;check(!events.consume(forged));
 check(events.lookup(event->serial)==event&&events.consume(*event)&&!events.consume(*event));
 check(!events.reserve(link,0,10,20,1000,{recipient}));
 check(!events.reserve(link,0,10,20,99,{recipient}));
 check(!events.reserve(link,0,10,20,999,{recipient,recipient}));
 const auto second=events.reserve(link,0,10,20,999,{recipient});check(second&&second->serial!=event->serial);
 auto replacement=caster;++replacement.generation;events.retire_caster(replacement);check(events.size()==1);
 events.retire_caster(caster);check(!events.size());
 std::cout<<"PASS Putrid Bile death reservation, original expiry, scoped ownership and one-shot consumption\n";
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}
