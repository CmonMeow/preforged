#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <span>
namespace fw::gw::skills::effects {
// Pure piecewise-linear trajectory calculation. World must provide all turn
// boundaries and authoritative collision-resolved paths before using this to
// retire an effect. It does not extrapolate raw requested destinations.
struct MaintainedRangePoint {std::uint64_t at;double x,y;};
struct MaintainedRangeExit {std::optional<std::uint64_t> first_outside;};
inline std::optional<MaintainedRangeExit> maintained_range_exit(
        std::span<const MaintainedRangePoint> caster,std::span<const MaintainedRangePoint> target,
        std::uint64_t from,std::uint64_t through,double radius=5000.){
    if(from>through||!std::isfinite(radius)||radius<=0||!std::isfinite(radius*radius))return {};
    const auto valid=[&](auto path){
        if(path.empty()||path.front().at>from||path.back().at<through)return false;
        for(std::size_t i=0;i<path.size();++i)
            if(!std::isfinite(path[i].x)||!std::isfinite(path[i].y)||(i&&path[i-1].at>=path[i].at))return false;
        return true;
    };
    if(!valid(caster)||!valid(target))return {};
    std::size_t a=0,b=0;
    const auto advance=[](auto path,std::size_t& index,std::uint64_t at){while(index+1<path.size()&&path[index+1].at<=at)++index;};
    const auto point=[](auto path,std::size_t index,std::uint64_t at){
        const auto& p=path[index];if(index+1==path.size()||p.at==at)return p;
        const auto& q=path[index+1];const double t=double(at-p.at)/double(q.at-p.at);
        return MaintainedRangePoint{at,std::lerp(p.x,q.x,t),std::lerp(p.y,q.y,t)};
    };
    const auto outside=[&](std::uint64_t at){const auto p=point(caster,a,at),q=point(target,b,at);return std::hypot(p.x-q.x,p.y-q.y)>radius;};
    auto at=from;advance(caster,a,at);advance(target,b,at);
    if(outside(at))return MaintainedRangeExit{at};
    while(at<through){
        auto end=through;
        if(a+1<caster.size())end=std::min(end,caster[a+1].at);
        if(b+1<target.size())end=std::min(end,target[b+1].at);
        // Relative motion is linear in this interval. A circle is convex:
        // inside endpoints prove the whole segment inside. If the end is
        // outside,there is one exit after the known-inside start.
        if(outside(end)){
            auto low=at,high=end;
            while(high-low>1){const auto middle=low+(high-low)/2;if(outside(middle))high=middle;else low=middle;}
            return MaintainedRangeExit{high};
        }
        at=end;advance(caster,a,at);advance(target,b,at);
    }
    return MaintainedRangeExit{};
}
}
