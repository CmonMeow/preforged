#include "gw_session_lifetime.h"
#include "gw_auth_session.h"
#include <iostream>
#include <stdexcept>
int main()try{
    using fw::gw::AuthenticatedLifetime;
    constexpr auto minute=std::uint64_t{60000};
    AuthenticatedLifetime quiet(10);
    if(quiet.expired(5*minute+9)||!quiet.expired(5*minute+10))
        throw std::runtime_error("idle session deadline boundary");
    AuthenticatedLifetime active(0);
    // Exercise a whole day without waiting in wall-clock time. This crosses
    // the former 30-minute lifetime cap while retaining idle enforcement.
    for(std::uint64_t now=minute;now<=24*60*minute;now+=minute){
        if(active.expired(now))throw std::runtime_error("active session expired");
        active.activity(now);
    }
    const auto last=24*60*minute;
    if(active.expired(last+5*minute-1)||!active.expired(last+5*minute))
        throw std::runtime_error("abandoned long session did not expire");
    active.activity(0);
    if(active.deadline()!=last+5*minute)throw std::runtime_error("clock regression shortened deadline");
    AuthenticatedLifetime overflow(std::numeric_limits<std::uint64_t>::max()-1);
    if(overflow.deadline()!=std::numeric_limits<std::uint64_t>::max())throw std::runtime_error("deadline overflow");
    using State=fw::gw::auth::Session::State;
    fw::gw::auth::PendingLifetime pending(0);
    pending.observed(1000,State::seed,State::seed,0,0);
    if(pending.deadline()!=minute)throw std::runtime_error("partial seed renewed authentication timeout");
    pending.observed(2000,State::seed,State::hello,0,0);
    if(pending.deadline()!=minute+2000)throw std::runtime_error("validated handshake progress did not renew timeout");
    unsigned pings=0;
    for(std::uint64_t now=30000;now<=24*60*minute;now+=30000){
        if(now>=pending.deadline())throw std::runtime_error("live login screen expired");
        pending.observed(now,State::hello,State::hello,pings,pings+1);++pings;
    }
    const auto idle_end=pending.deadline();
    pending.observed(idle_end-1,State::hello,State::hello,pings,pings);
    pending.observed(idle_end-1,State::hello,State::failed,pings,pings+1);
    if(pending.deadline()!=idle_end)throw std::runtime_error("partial/rejected input renewed pending login");
    pending.observed(0,State::hello,State::hello,pings,pings+1);
    if(pending.deadline()!=idle_end)throw std::runtime_error("pending clock regression changed deadline");
    std::cout<<"PASS active/pending session longevity, validated activity and idle expiry\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
