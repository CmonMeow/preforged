#pragma once
#include "gw_npc_pursuit_motion.h"
namespace fw::gw::combat::ai::motion {
struct LeashBudget {double budget=0;bool boundary=false;};
// Authored home-circle enforcement, not a recovered retail radius. Restrict
// pursuit along its actual route at the first exit; a later re-entry cannot
// authorize travel through the outside region. Return-home motion is exempt.
inline std::optional<LeashBudget> leash_budget(Point from,std::span<const Point> route,
        Point home,double radius,double requested){
    if(!valid(from)||!valid(home)||!std::isfinite(radius)||radius<=0||
       !std::isfinite(requested)||requested<0)return {};
    for(const auto p:route)if(!valid(p))return {};
    if(std::hypot(from.x-home.x,from.y-home.y)>radius)return LeashBudget{0,true};
    double travelled=0;auto cursor=from;
    for(const auto next:route){
        const double dx=next.x-cursor.x,dy=next.y-cursor.y,length=std::hypot(dx,dy);
        if(!std::isfinite(length))return {};
        if(length==0)continue;
        const double ux=dx/length,uy=dy/length;
        const double x=cursor.x-home.x,y=cursor.y-home.y;
        const double along=x*ux+y*uy,cross=x*uy-y*ux;
        const double exit=-along+std::sqrt(std::max(0.,radius*radius-cross*cross));
        if(exit<=length&&travelled+exit<=requested)
            return LeashBudget{std::max(0.,travelled+exit),true};
        if(length>=requested-travelled)return LeashBudget{requested,false};
        travelled+=length;cursor=next;
    }
    return LeashBudget{requested,false};
}
}
