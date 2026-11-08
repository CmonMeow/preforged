#include "gw_pet_cast_lease.h"
#include <optional>
#include <stdexcept>
#include <iostream>
using fw::gw::pets::CastLease;
void check(bool v){if(!v)throw std::runtime_error("pet cast lease cleanup");}
int main(){
 int cancellations=0;
 {CastLease reservation([&]{++cancellations;});}
 check(cancellations==1);
 {CastLease reservation([&]{++cancellations;});CastLease pending(std::move(reservation));}
 check(cancellations==2);
 {CastLease completed([&]{++cancellations;});completed.consumed();}
 check(cancellations==2);
 try{CastLease reservation([&]{++cancellations;});throw std::runtime_error("payment failed");}catch(const std::runtime_error&){}
 check(cancellations==3);
 std::optional<CastLease> pending;pending.emplace([&]{++cancellations;});pending.reset();check(cancellations==4);
 {CastLease failed_cleanup([]{throw std::runtime_error("cleanup failure");});}
 std::cout<<"Pet cast lease rejection, transfer, completion, reset and exception cleanup passed\n";
}
