#pragma once
#include <algorithm>
#include <cmath>
#include <optional>
#include <span>
#include <vector>

namespace fw::gw::combat::ai::motion {
struct Point {double x=0,y=0;};
struct Advance {
    Point point;double spent=0;bool reached=false;std::optional<double> facing;
    // Ordered endpoints of every nonzero traversed leg, including a partial
    // final leg. Publication must retain these corners instead of replacing
    // the integrated route with the chord from start to final position.
    std::vector<Point> path;
};
inline bool valid(Point p){return std::isfinite(p.x)&&std::isfinite(p.y);}
// Spend one authoritative movement interval along the whole navigation route.
// The stop region is a circle around the actual goal, not a distance subtracted
// from each detour segment. Speed/range and the route itself remain host policy.
inline std::optional<Advance> advance(Point from,std::span<const Point> route,Point goal,double stop,double budget){
    if(!valid(from)||!valid(goal)||!std::isfinite(stop)||stop<0||!std::isfinite(budget)||budget<0)return {};
    for(const auto p:route)if(!valid(p))return {};
    Advance out{from};
    if(std::hypot(from.x-goal.x,from.y-goal.y)<=stop){out.reached=true;return out;}
    for(const auto next:route){
        const double dx=next.x-out.point.x,dy=next.y-out.point.y,length=std::hypot(dx,dy);
        if(!std::isfinite(length))return {};
        if(length==0)continue;
        const double ux=dx/length,uy=dy/length;
        double step=std::min(length,std::max(0.0,budget-out.spent));
        // First entry along this segment, including a segment whose endpoint
        // lies beyond the circle again. This cannot cut across a route corner.
        const double gx=out.point.x-goal.x,gy=out.point.y-goal.y;
        const double along=-(gx*ux+gy*uy),cross=gx*uy-gy*ux;
        if(std::abs(cross)<=stop){
            const double entry=along-std::sqrt(std::max(0.0,stop*stop-cross*cross));
            if(entry>=0&&entry<=step){step=entry;out.reached=true;}
        }
        // Heading belongs to the last segment actually travelled. A tick may
        // round several corners; its net displacement is not the final facing.
        if(step>0)out.facing=std::atan2(dy,dx);
        out.point.x+=ux*step;out.point.y+=uy*step;out.spent+=step;
        if(step>0)out.path.push_back(out.point);
        if(out.reached||step<length||out.spent>=budget)return out;
    }
    return out;
}
}
