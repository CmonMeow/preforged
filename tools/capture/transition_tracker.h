#pragma once
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>

namespace capture {
struct TransitionPoint {
    std::uint32_t map=0,instance_ms=0,plane=0;
    std::uint64_t epoch=0,observed_ms=0;
    float x=0,y=0,facing=0;
};
struct Transition {
    TransitionPoint from,to;
    bool loading_observed=false;
    std::uint64_t departure_age_ms=0;
};
class TransitionTracker {
    std::optional<TransitionPoint> last_;
    std::optional<std::uint64_t> loading_at_;
public:
    std::optional<Transition> observe(bool loaded,std::optional<TransitionPoint> point,std::uint64_t now){
        if(!loaded){if(last_&&!loading_at_)loading_at_=now;return {};}
        if(!point||!point->map||!std::isfinite(point->x)||!std::isfinite(point->y)||!std::isfinite(point->facing))return {};
        point->observed_ms=now;
        std::optional<Transition> result;
        if(last_&&now>=last_->observed_ms&&point->epoch!=last_->epoch){
            const auto departure=loading_at_.value_or(now);
            result=Transition{*last_,*point,loading_at_.has_value(),
                departure>=last_->observed_ms?departure-last_->observed_ms:0};
        }
        last_=point;loading_at_.reset();return result;
    }
};
inline std::string transition_json(const Transition& t){
    auto point=[](const TransitionPoint& p){return std::string("{\"map\":")+std::to_string(p.map)+
        ",\"instance_epoch\":"+std::to_string(p.epoch)+",\"instance_ms\":"+std::to_string(p.instance_ms)+
        ",\"observed_ms\":"+std::to_string(p.observed_ms)+",\"x\":"+std::to_string(p.x)+
        ",\"y\":"+std::to_string(p.y)+",\"plane\":"+std::to_string(p.plane)+",\"facing\":"+std::to_string(p.facing)+"}";};
    return "{\"schema\":1,\"event\":\"local_map_transition_observed\",\"from\":"+point(t.from)+",\"to\":"+point(t.to)+
        ",\"loading_observed\":"+(t.loading_observed?"true":"false")+
        ",\"departure_sample_age_ms\":"+std::to_string(t.departure_age_ms)+
        ",\"travel_method\":\"unknown\",\"verified_portal\":false,\"verified_spawn\":false}";
}
}
