#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
namespace fw::gw::overcast {
// Signed authoritative resource arithmetic. Scheduling/UI projection belongs
// to the world; this primitive makes no assumption about recovery phase.
class Pool {
    std::int64_t nominal_,current_,points_=0;
    static std::int64_t add(std::int64_t a,std::int64_t b){
        if((b>0&&a>INT64_MAX-b)||(b<0&&a<INT64_MIN-b))throw std::overflow_error("Overcast arithmetic overflow");
        const auto result=a+b;if(result==INT64_MIN)throw std::overflow_error("Overcast signed minimum unsupported");return result;
    }
public:
    static constexpr std::uint64_t recovery_interval_ms=3000;
    Pool(std::int64_t nominal,std::int64_t current):nominal_(nominal),current_(current){
        if(nominal<0||current>nominal||current==INT64_MIN)throw std::invalid_argument("invalid Overcast pool");
    }
    std::int64_t current()const{return current_;}
    std::int64_t maximum()const{return nominal_-points_;}
    std::int64_t nominal()const{return nominal_;}
    std::int64_t points()const{return points_;}
    bool active()const{return points_!=0;}
    std::uint64_t visible_current()const{return current_>0?static_cast<std::uint64_t>(current_):0;}
    bool can_pay(std::int64_t amount)const{return amount>=0&&(amount==0||current_>=amount);}
    bool pay(std::int64_t amount){if(!can_pay(amount))return false;current_-=amount;return true;}
    void apply(std::int64_t amount){
        if(amount<0)throw std::invalid_argument("negative Overcast");
        const auto next=add(points_,amount);points_=next;current_=std::min(current_,maximum());
    }
    bool start(std::int64_t cost,std::int64_t amount){
        if(amount<0||!can_pay(cost))return false;
        auto next=*this;next.pay(cost);next.apply(amount);*this=next;return true;
    }
    std::uint64_t recover(std::uint64_t events){
        const auto recovered=std::min(events,static_cast<std::uint64_t>(points_));
        points_-=static_cast<std::int64_t>(recovered);return recovered;
    }
    void gain(std::int64_t amount){
        if(amount<0)throw std::invalid_argument("negative Energy gain");
        if(current_>=maximum())return;
        if(maximum()<0){current_+=std::min(amount,maximum()-current_);return;}
        if(current_<0){const auto paid=std::min(amount,-current_);current_+=paid;amount-=paid;}
        if(amount&&current_<maximum())current_+=std::min(amount,maximum()-current_);
    }
    void resize_nominal(std::int64_t nominal){
        if(nominal<0)throw std::invalid_argument("negative nominal capacity");
        const auto next=add(current_,nominal-nominal_);nominal_=nominal;current_=next;
    }
    void zone(){points_=0;}
};
class RecoveryClock {
    std::uint64_t phase_;
public:
    explicit RecoveryClock(std::uint64_t phase):phase_(phase){if(phase>=3000)throw std::invalid_argument("invalid recovery phase");}
    std::uint64_t remainder()const{return phase_;}
    void set_phase(std::uint64_t phase){if(phase>=3000)throw std::invalid_argument("invalid recovery phase");phase_=phase;}
    std::uint64_t advance(std::uint64_t elapsed){
        const auto tail=phase_+elapsed%3000;phase_=tail%3000;return elapsed/3000+tail/3000;
    }
};
}
