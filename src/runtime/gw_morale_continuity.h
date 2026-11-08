#pragma once
#include "gw_effect_registry.h"
#include <mutex>
#include <stdexcept>
namespace fw::gw::consumables::continuity {
class Morale {
    std::mutex mutex_;skills::effects::Scope owner_{};unsigned value_=100;
public:
    class Guard {
        Morale& state_;std::unique_lock<std::mutex> lock_;bool owned_;
    public:
        Guard(Morale& state,skills::effects::Scope scope):state_(state),lock_(state.mutex_),owned_(state.owner_==scope){}
        Guard(Guard&&)=default;
        bool owned()const{return owned_;}
        void publish(unsigned value)noexcept{if(owned_)state_.value_=value;}
    };
    unsigned claim(skills::effects::Scope scope,bool reset){
        if(!scope.instance||!scope.actor||!scope.generation)throw std::invalid_argument("morale continuity owner scope required");
        std::lock_guard lock(mutex_);owner_=scope;if(reset)value_=100;return value_;
    }
    Guard guard(skills::effects::Scope scope){return Guard(*this,scope);}
};
}
