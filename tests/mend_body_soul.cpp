#include "gw_mend_body_soul.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>

using namespace fw::gw::skills::effects;

void check(bool condition,std::source_location where=std::source_location::current()){
    if(!condition)throw std::runtime_error("Mend Body and Soul line " +
                                           std::to_string(where.line()));
}

int main()try{
    check(mend_body_soul(0)->heal==20);
    check(mend_body_soul(12)->heal==96);
    check(mend_body_soul(15)->heal==115);
    check(!mend_body_soul(21));
    const auto plan=*mend_body_soul(12);
    check(quote_mend_body_soul(plan,0,3)->conditions_to_remove==0);
    check(quote_mend_body_soul(plan,2,3)->conditions_to_remove==2);
    check(quote_mend_body_soul(plan,5,3)->conditions_to_remove==3);
    check(quote_mend_body_soul(plan,2,3)->heal==96);
    check(!quote_mend_body_soul(plan,std::nullopt,3));
    check(!quote_mend_body_soul(plan,2,11));
    auto forged=plan;++forged.heal;
    check(!quote_mend_body_soul(forged,2,3));
    std::cout<<"PASS Mend Body and Soul rank and typed-spirit quote; no source binding\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
