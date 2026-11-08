#include "gw_deep_wound.h"
#include <iostream>
#include <stdexcept>
using fw::gw::combat::deep_wound::State;
void check(bool value){if(!value)throw std::runtime_error("Deep Wound regression");}
int main(){try{
    {State healing;double hp=500;healing.reset(1000);healing.capacity(hp,1000,true);
     healing.event(hp,100,true,.66*.8);check(hp==460); // Joint reduction capped at40%.
     healing.event(hp,100,false,.66);check(hp==560); // Health gain is not healing.
     healing.regeneration(hp,100);check(hp==660);}
    State s;double hp=50;s.capacity(hp,480,false);check(hp==50);
    s.capacity(hp,480,true);check(hp==1&&s.maximum()==384&&s.debt==47);
    s.capacity(hp,480,true);check(hp==1&&s.debt==47);
    s.capacity(hp,480,false);check(hp==50&&s.debt==0);
    for(int i=0;i<100;++i){s.capacity(hp,480,true);s.capacity(hp,480,false);}check(hp==50);
    s.capacity(hp,480,true);s.event(hp,50,true);check(hp==0);
    s.capacity(hp,480,false);check(hp==0&&s.maximum()==480);
    s.reset(480);hp=50;s.capacity(hp,480,true);s.event(hp,100,true);check(hp==34);
    s.reset(480);hp=50;s.capacity(hp,480,true);s.event(hp,100);check(hp==54);
    s.reset(480);hp=50;s.capacity(hp,480,true);s.regeneration(hp,40);check(hp==1&&s.debt==7);
    s.regeneration(hp,20);check(hp==14&&s.debt==0);s.capacity(hp,480,false);check(hp==110);
    s.reset(1000);hp=1000;s.capacity(hp,1000,true);check(hp==900&&s.maximum()==900);
    s.capacity(hp,1100,true);check(hp==1000&&s.maximum()==1000);
    s.event(hp,-10);check(hp==990);s.capacity(hp,1100,false);check(hp==1090);
    s.reset(480);hp=50;s.capacity(hp,480,true);s.event(hp,-.1);check(hp==0);
    s.reset(480);hp=50;s.capacity(hp,400,false);check(hp==1&&s.debt==0);s.event(hp,10,true);check(hp==11);
    s.reset(480);hp=50;s.capacity(hp,480,true);s.capacity(hp,400,true);check(hp==1&&s.debt==80);s.capacity(hp,400,false);check(hp==1&&s.debt==0);
    std::cout<<"Deep Wound capacity, delayed lethal debt, healing/gain/regen PASS\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
