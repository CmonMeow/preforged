#pragma once
#include "gw_navigation.h"
#include <set>
#include <span>

namespace fw::gw::navigation {
// A surface is a specific cell on a specific plane. XY overlap never creates
// a connection. Cross-plane edges must come from reciprocal DAT portal pairs.
struct SurfacePoint { Point point; Surface surface; };
struct Motion {
    SurfacePoint end;bool clipped=false,blocked=false;
    // Final straight-segment rate, scaled to the caller's whole input interval.
    // Unlike end-start this excludes the earlier free/contact prefixes and
    // their geometry tolerance. It is not a world-units-per-second velocity.
    Point terminal_delta{};
};
struct MotionPoint {float fraction;SurfacePoint position;};
struct RouteDiagnostics { bool taut=false,visibility_improved=false;std::size_t failed_portal=0,portal_count=0,visibility_nodes=0; };
class LayeredMesh {
    const Mesh& mesh_;
    std::map<Surface,std::vector<Surface>> edges_;
    static float distance(Point a,Point b){return std::hypot(a.x-b.x,a.z-b.z);}
    Point center(Surface s) const {
        const auto& t=mesh_.trapezoid(s.plane,s.cell);
        return {(t.xtl+t.xtr+t.xbl+t.xbr)*.25f,(t.yt+t.yb)*.5f};
    }
    bool interval(Surface s,Point a,Point b,float& enter,float& leave,double epsilon=.01) const {
        const auto& t=mesh_.trapezoid(s.plane,s.cell);enter=0;leave=1;
        // DAT bridge entrances include zero-height cells. They are explicit
        // connector segments, not open area and not invalid geometry.
        const bool connector=std::abs(t.yb-t.yt)<1e-6f;
        const double leftSlope=connector?0:(double(t.xbl)-t.xtl)/(double(t.yb)-t.yt),
            rightSlope=connector?0:(double(t.xbr)-t.xtr)/(double(t.yb)-t.yt);
        const double leftIntercept=connector?std::min({t.xtl,t.xtr,t.xbl,t.xbr}):t.xtl-leftSlope*t.yt,
            rightIntercept=connector?std::max({t.xtl,t.xtr,t.xbl,t.xbr}):t.xtr-rightSlope*t.yt;
        // Clip the segment to the four half-planes of this convex trapezoid.
        const auto clip=[&](double start,double end){
            if(std::abs(start)<=epsilon)start=0;
            if(std::abs(end)<=epsilon)end=0;
            if(start< -epsilon&&end< -epsilon)return false;
            const double delta=end-start;
            if(std::abs(delta)<1e-8f)return start>=-epsilon;
            const float crossing=float(-start/delta);
            if(delta>0)enter=std::max(enter,crossing);else leave=std::min(leave,crossing);
            return enter<=leave+1e-6f;
        };
        return clip(a.z-std::min(t.yt,t.yb),b.z-std::min(t.yt,t.yb))&&
            clip(std::max(t.yt,t.yb)-a.z,std::max(t.yt,t.yb)-b.z)&&
            clip(a.x-leftSlope*a.z-leftIntercept,b.x-leftSlope*b.z-leftIntercept)&&
            clip(rightSlope*a.z+rightIntercept-a.x,rightSlope*b.z+rightIntercept-b.x);
    }
    // A steep shared edge can move farther than the collision epsilon when
    // its independently rounded X/Y coordinates are converted to floats. Keep
    // float Y first, then intersect both physical X intervals at that Y. This
    // changes the represented point, never the cells or collision tolerance.
    std::optional<Point> shared_crossing(Surface a,Surface b,Point point)const{
        float enter,leave;
        if(interval(a,point,point,enter,leave)&&interval(b,point,point,enter,leave))return point;
        const auto& first=mesh_.trapezoid(a.plane,a.cell);const auto& second=mesh_.trapezoid(b.plane,b.cell);
        const double low_y=std::max(double(std::min(first.yt,first.yb)),double(std::min(second.yt,second.yb))),
            high_y=std::min(double(std::max(first.yt,first.yb)),double(std::max(second.yt,second.yb)));
        if(low_y>high_y)return {};
        const auto ulp=[](float value){return std::max(double(std::nextafter(value,std::numeric_limits<float>::infinity()))-value,
            double(value)-std::nextafter(value,-std::numeric_limits<float>::infinity()));};
        const Point original=point;point.z=float(std::clamp(double(point.z),low_y,high_y));
        if(std::abs(double(point.z)-original.z)>ulp(original.z))return {};
        double low_x=-std::numeric_limits<double>::infinity(),high_x=std::numeric_limits<double>::infinity(),slope=0;
        for(const auto* t:{&first,&second}){
            double left,right;
            if(std::abs(double(t->yb)-t->yt)<1e-6){left=std::min({t->xtl,t->xtr,t->xbl,t->xbr});right=std::max({t->xtl,t->xtr,t->xbl,t->xbr});}
            else{
                const double u=(double(point.z)-t->yt)/(double(t->yb)-t->yt);
                left=t->xtl+(double(t->xbl)-t->xtl)*u;right=t->xtr+(double(t->xbr)-t->xtr)*u;
                slope=std::max({slope,std::abs((double(t->xbl)-t->xtl)/(double(t->yb)-t->yt)),
                    std::abs((double(t->xbr)-t->xtr)/(double(t->yb)-t->yt))});
            }
            low_x=std::max(low_x,left);high_x=std::min(high_x,right);
        }
        if(low_x>high_x)return {};
        point.x=float(std::clamp(double(point.x),low_x,high_x));
        // One float quantum per coordinate bounds coupled rounding; this is
        // not permission to project an unrelated point onto a distant edge.
        if(std::abs(double(point.x)-original.x)>ulp(original.x)+slope*ulp(original.z)||
            !interval(a,point,point,enter,leave)||!interval(b,point,point,enter,leave))return {};
        return point;
    }
public:
    explicit LayeredMesh(const Mesh& mesh,std::span<const SurfaceLink> links={}) : mesh_(mesh) {
        if(links.empty())links=mesh.surface_links();
        for(std::size_t p=0;p<mesh.plane_count();++p)for(std::size_t c=0;c<mesh.trapezoid_count(p);++c){
            auto& edges=edges_[{p,c}];
            for(auto n:mesh.trapezoid(p,c).neighbours)if(n>=0){
                Point portal;
                if(mesh.portal_midpoint(p,c,p,n,portal))edges.push_back({p,std::size_t(n)});
            }
        }
        std::set<std::pair<Surface,Surface>> unique;
        for(auto link:links){
            Point portal;
            if(link.a==link.b||!mesh.portal_midpoint(link.a.plane,link.a.cell,link.b.plane,link.b.cell,portal))
                throw std::runtime_error("layer link has no shared geometric edge");
            if(unique.emplace(link.a,link.b).second)edges_[link.a].push_back(link.b);
            if(unique.emplace(link.b,link.a).second)edges_[link.b].push_back(link.a);
        }
    }
    // Reachability within a source-centered disk, not a bound on path length.
    // Each cell and its disk intersection are convex. Adjacent intersections
    // connect iff their shared portal intersects the disk. This query avoids
    // rejecting a valid in-range corridor merely because an unconstrained
    // shortest-route search happened to choose a route outside the disk.
    // Optional closed portal edges are supplied by the authoritative gate owner;
    // this does not infer dynamic gates or carve obstacles inside a cell.
    std::optional<bool> reachable_within(Surface start,Point a,Surface goal,Point b,float radius,
                                        std::span<const SurfaceLink> blocked={})const{
        const auto valid=[&](Surface v){return v.plane<mesh_.plane_count()&&v.cell<mesh_.trapezoid_count(v.plane);};
        if(!valid(start)||!valid(goal)||!std::isfinite(a.x)||!std::isfinite(a.z)||!std::isfinite(b.x)||!std::isfinite(b.z)||
           !std::isfinite(radius)||radius<=0)return {};
        for(const auto& edge:blocked)if(!valid(edge.a)||!valid(edge.b)||edge.a==edge.b)return {};
        float enter,leave;if(!interval(start,a,a,enter,leave,0)||!interval(goal,b,b,enter,leave,0))return {};
        const double r2=double(radius)*radius;
        const auto inside=[&](double x,double z){const double dx=x-a.x,dz=z-a.z;return dx*dx+dz*dz<=r2;};
        if(!inside(b.x,b.z))return false;
        std::set<Surface> seen{start};std::queue<Surface> pending;pending.push(start);
        while(!pending.empty()){
            const auto current=pending.front();pending.pop();if(current==goal)return true;
            const auto edges=edges_.find(current);if(edges==edges_.end())continue;
            for(const auto next:edges->second){
                if(seen.contains(next)||std::any_of(blocked.begin(),blocked.end(),[&](const auto& e){return (e.a==current&&e.b==next)||(e.a==next&&e.b==current);}))continue;
                const auto portal=mesh_.portal(current,next);if(!portal)return {};
                const auto u=portal->left,v=portal->right;
                const double dx=double(v.x)-u.x,dz=double(v.z)-u.z,len=dx*dx+dz*dz;
                const auto t=len?std::clamp(((double(a.x)-u.x)*dx+(double(a.z)-u.z)*dz)/len,0.,1.):0.;
                if(!inside(double(u.x)+t*dx,double(u.z)+t*dz))continue;
                seen.insert(next);pending.push(next);
            }
        }
        return false;
    }
    // A visibility edge must reach the requested surface, not whichever
    // overlapping floor a trace happens to visit first. Record real portal
    // crossings so a straight route can traverse stairs at any point along
    // their width; floor connector endpoints are not mandatory waypoints.
    std::optional<std::vector<SurfacePoint>> straight_route(Surface start,Point a,Surface goal,Point b) const {
        return straight_route_impl(start,a,goal,b,true);
    }
private:
    std::optional<std::vector<SurfacePoint>> straight_route_impl(Surface start,Point a,Surface goal,Point b,bool crossings) const {
        if(!std::isfinite(a.x)||!std::isfinite(a.z)||!std::isfinite(b.x)||!std::isfinite(b.z))return {};
        float enter,leave;
        if(!interval(start,a,a,enter,leave)||!interval(goal,b,b,enter,leave))return {};
        struct Visit {Surface surface;float at;std::size_t parent;};
        std::vector<Visit> visits{{start,0,0}};std::set<Surface> seen{start};
        for(std::size_t i=0;i<visits.size();++i){
            const auto state=visits[i];
            if(!interval(state.surface,a,b,enter,leave)||enter>state.at+1e-5f||leave<state.at-1e-5f)continue;
            if(state.surface==goal&&leave>=1-1e-6f){
                std::vector<std::size_t> chain;
                for(auto index=i;;index=visits[index].parent){chain.push_back(index);if(index==0)break;}
                if(!crossings)return std::vector<SurfacePoint>{{a,start},{b,goal}};
                std::reverse(chain.begin(),chain.end());std::vector<SurfacePoint> points{{a,start}};bool adjusted=false;
                for(std::size_t k=1;k<chain.size();++k){
                    const auto& next=visits[chain[k]];const auto& previous=visits[chain[k-1]];
                    if(previous.surface.plane==next.surface.plane)continue;
                    const Point requested{a.x+(b.x-a.x)*next.at,a.z+(b.z-a.z)*next.at};
                    const auto crossing=shared_crossing(previous.surface,next.surface,requested);if(!crossing)return {};
                    adjusted|=crossing->x!=requested.x||crossing->z!=requested.z;
                    points.push_back({*crossing,previous.surface});points.push_back({*crossing,next.surface});
                }
                points.push_back({b,goal});
                // Quantization may make the represented polyline non-collinear.
                // Recheck its real branches so a corrected corner never cuts a wall.
                if(adjusted)for(std::size_t k=1;k<points.size();++k)
                    if(!straight_route_impl(points[k-1].surface,points[k-1].point,points[k].surface,points[k].point,false))return {};
                return points;
            }
            const auto edges=edges_.find(state.surface);if(edges==edges_.end())continue;
            for(const auto next:edges->second){
                float next_enter,next_leave;
                if(!seen.contains(next)&&interval(next,a,b,next_enter,next_leave)&&
                    next_enter<=leave+1e-5f&&next_leave>=leave-1e-6f){
                    seen.insert(next);visits.push_back({next,std::max(state.at,next_enter),i});
                }
            }
        }
        return {};
    }
public:
    // Validate actual keyboard motion, including narrow cells, without sampling
    // or substituting an A* detour around a wall. A blocked step changes nothing.
    std::optional<Surface> trace(Surface start,Point a,Point b,bool same_plane_only=false)const{
        return trace_impl(start,a,b,same_plane_only,{},false);
    }
    // Return the SAME validated collinear branch as trace(), including cell/
    // plane entry fractions. No midpoint routing or endpoint-plane inference.
    // Edge ties use this branch; independent endpoint queries may admit both
    // cells within the existing .01 geometry tolerance.
    std::optional<std::vector<MotionPoint>> trace_path(Surface start,Point a,Point b,bool same_plane_only=false)const{
        std::vector<MotionPoint> result;
        if(!trace_impl(start,a,b,same_plane_only,{},false,&result))return {};
        return result;
    }
private:
    std::optional<Surface> trace_impl(Surface start,Point a,Point b,bool same_plane_only,
                                     std::span<const SurfaceLink> blocked,bool strict_endpoint,std::vector<MotionPoint>* trajectory=nullptr) const {
        if(!std::isfinite(a.x)||!std::isfinite(a.z)||!std::isfinite(b.x)||!std::isfinite(b.z))return {};
        float enter,leave;if(!interval(start,a,b,enter,leave)||enter>1e-6f)return {};
        struct State{Surface surface;float at,entered;std::size_t parent;};
        constexpr auto no_parent=std::numeric_limits<std::size_t>::max();
        std::vector<State> pending{{start,0,0,no_parent}},ancestry;std::set<Surface> seen;
        while(!pending.empty()){
            const auto state=pending.back();pending.pop_back();
            if(!seen.insert(state.surface).second)continue;
            if(!interval(state.surface,a,b,enter,leave)||enter>state.at+1e-5f||leave<state.at-1e-5f)continue;
            const auto parent=ancestry.size();if(trajectory)ancestry.push_back(state);
            if(leave>=1-1e-6f){
                float endEnter,endLeave;
                // Parametric epsilon scales with segment length. A long span
                // can leave a world-space gap larger than geometry tolerance.
                if(interval(state.surface,b,b,endEnter,endLeave,strict_endpoint?0.:.01)){
                    if(trajectory){
                        std::vector<State> branch;
                        for(auto node=parent;node!=no_parent;node=ancestry[node].parent)branch.push_back(ancestry[node]);
                        std::reverse(branch.begin(),branch.end());trajectory->clear();
                        for(const auto& entry:branch){const float fraction=std::clamp(entry.entered,0.f,1.f);
                            trajectory->push_back({fraction,{{float(std::lerp(double(a.x),double(b.x),double(fraction))),
                                float(std::lerp(double(a.z),double(b.z),double(fraction)))},entry.surface}});
                        }
                        trajectory->push_back({1,{b,state.surface}});
                    }
                    return state.surface;
                }
            }
            const auto it=edges_.find(state.surface);if(it==edges_.end())continue;
            for(auto next:it->second){
                if(std::any_of(blocked.begin(),blocked.end(),[&](const auto& e){return (e.a==state.surface&&e.b==next)||(e.b==state.surface&&e.a==next);}))continue;
                if(same_plane_only&&next.plane!=start.plane)continue;
                float nextEnter,nextLeave;
                // Several connector cells can occupy the same crossing point.
                // The visited set bounds these zero-progress transitions.
                if(interval(next,a,b,nextEnter,nextLeave)&&nextEnter<=leave+1e-5f&&nextLeave>=leave-1e-6f)
                    // Search reachability retains its original candidate time.
                    // Observed plane ownership changes when the old cell is
                    // exited, not merely when an overlapping neighbor begins.
                    pending.push_back({next,std::max(nextEnter,state.at),
                        std::max(state.entered,std::min(leave,nextLeave)),trajectory?parent:no_parent});
            }
        }
        return {};
    }
public:
    // Longest validated collinear prefix. Unlike slide(), this never spends
    // unused distance along a wall or selects a route around an obstruction.
    // Explicit closed edges are checked during every prefix probe. Endpoint
    // containment is strict: trace's crossing tolerance is not clearance.
    std::optional<Motion> straight_prefix(Surface start,Point a,Point b,
                                         std::span<const SurfaceLink> blocked={})const{
        const auto valid=[&](Surface v){return v.plane<mesh_.plane_count()&&v.cell<mesh_.trapezoid_count(v.plane);};
        if(!valid(start)||!std::isfinite(a.x)||!std::isfinite(a.z)||!std::isfinite(b.x)||!std::isfinite(b.z))return {};
        for(const auto& edge:blocked)if(!valid(edge.a)||!valid(edge.b)||edge.a==edge.b)return {};
        float enter,leave;if(!interval(start,a,a,enter,leave,0))return {};
        if(const auto end=trace_impl(start,a,b,false,blocked,true))return Motion{{b,*end},false,false,{}};
        float low=0,high=1;SurfacePoint end{a,start};
        for(unsigned n=0;n<32;++n){
            const auto middle=(low+high)*.5f;if(middle==low||middle==high)break;
            const Point p{float(double(a.x)+(double(b.x)-a.x)*middle),float(double(a.z)+(double(b.z)-a.z)*middle)};
            if(const auto surface=trace_impl(start,a,p,false,blocked,true)){low=middle;end={p,*surface};}else high=middle;
        }
        return Motion{end,true,std::hypot(end.point.x-a.x,end.point.z-a.z)<.001f,{}};
    }
    // Shadow steps use bounded topology, not line of sight. A reachable
    // destination may have a wall between the endpoints. An unreachable one
    // falls back to a straight prefix through the source's connected topology.
    // This is geometry only: callers still own targeting, bundles and state.
    std::optional<Motion> shadow_destination(Surface start,Point a,Surface goal,Point b,
                                             float compass,std::span<const SurfaceLink> blocked={})const{
        const auto reachable=reachable_within(start,a,goal,b,compass,blocked);
        if(!reachable)return {};
        // Out-of-range is invalid targeting, not a blocked-path opportunity
        // to move beyond the very disk that constrained reachability.
        if(std::hypot(double(b.x)-a.x,double(b.z)-a.z)>compass)return {};
        if(*reachable)return Motion{{b,goal},false,false,{}};
        auto fallback=straight_prefix(start,a,b,blocked);
        if(fallback){
            // A clear XY ray on another floor still did not reach goal.
            fallback->clipped=true;
            fallback->blocked=std::hypot(double(fallback->end.point.x)-a.x,double(fallback->end.point.z)-a.z)<.001;
        }
        return fallback;
    }
    // Sweep to the first wall, then spend only the remaining distance along
    // its tangent. Every prefix still goes through trace, so overlap never
    // becomes a connection and explicit connector/plane transitions survive.
    // This is local collision response, not a route search around obstacles.
    // A caller may bound the speed lost on a genuine tangent projection. The
    // default keeps ordinary orthogonal sliding; keyboard prediction in the
    // 2022 client clamps its queued tangent speed ratio to [.33,1]. This never
    // chooses a direction for head-on contact or adds to the input budget.
    // Renewed held input may add the native initial-path .01 ratio bias;
    // ordinary projection and queued-path callers retain the zero default.
    std::optional<Motion> slide(Surface start,Point a,Point b,float minimum_tangent_ratio=0.f,float tangent_bias=0.f,bool condition_axes=false,std::vector<MotionPoint>* trajectory=nullptr) const {
        if(trajectory)trajectory->clear();
        if(!trace(start,a,a)||!std::isfinite(b.x)||!std::isfinite(b.z)||
            !std::isfinite(minimum_tangent_ratio)||minimum_tangent_ratio<0||minimum_tangent_ratio>1||!std::isfinite(tangent_bias)||tangent_bias<0||tangent_bias>1)return {};
        if(condition_axes){
            // Original 6b4b01..6b4bbc conditions the normalized path ray,
            // including a positive minor component for tiny negative input.
            // Keep this out of ordinary projection/click-route callers.
            const float length=std::hypot(b.x-a.x,b.z-a.z);
            if(length>0){
                float dx=(b.x-a.x)/length,dz=(b.z-a.z)/length;
                if(std::abs(dz)<.001f){dx=dx>0?.9999989867210388f:-.9999989867210388f;dz=.001f;}
                else if(std::abs(dx)<.001f){dx=.001f;dz=dz>0?.9999989867210388f:-.9999989867210388f;}
                b={a.x+dx*length,a.z+dz*length};
            }
        }
        struct Prefix { SurfacePoint end; float fraction; };
        const auto prefix=[&](SurfacePoint from,Point to){
            if(const auto surface=trace(from.surface,from.point,to))return Prefix{{to,*surface},1};
            float low=0,high=1;SurfacePoint end=from;
            // Fixed work bound, finer than geometry tolerance even for a long
            // input interval. No endpoint outside the mesh is accepted.
            for(unsigned n=0;n<24;++n){
                const float middle=(low+high)*.5f;
                if(middle==low||middle==high)break;
                const Point point{from.point.x+(to.x-from.point.x)*middle,
                                  from.point.z+(to.z-from.point.z)*middle};
                if(const auto surface=trace(from.surface,from.point,point)){low=middle;end={point,*surface};}
                else high=middle;
            }
            // trace's tolerance is useful for floating-point edge crossings,
            // but it must not become physical clearance past a wall tip. A
            // padded endpoint can start beyond a zero-width connector on the
            // next step, stranding the actor on the old navigation layer.
            const auto& cell=mesh_.trapezoid(end.surface.plane,end.surface.cell);
            float physical_enter,physical_leave;
            if(interval(end.surface,from.point,to,physical_enter,physical_leave,0))
                low=std::min(low,physical_leave);
            else if(end.surface==from.surface)low=0;
            Point contact{from.point.x+(to.x-from.point.x)*low,from.point.z+(to.z-from.point.z)*low};
            contact.z=std::clamp(contact.z,std::min(cell.yt,cell.yb),std::max(cell.yt,cell.yb));
            if(std::abs(cell.yb-cell.yt)>1e-6f){
                const double t=(double(contact.z)-cell.yt)/(double(cell.yb)-cell.yt);
                const double left=cell.xtl+(double(cell.xbl)-cell.xtl)*t,
                    right=cell.xtr+(double(cell.xbr)-cell.xtr)*t;
                const double low_x=std::min(left,right),high_x=std::max(left,right);
                contact.x=float(std::clamp(double(contact.x),low_x,high_x));
                // Rounding a sloped edge outward by one float ULP leaves
                // the next tangent parallel to, but outside, the physical
                // cell. Round the contact inward rather than relying on
                // trace's wider tolerance as clearance.
                if(double(contact.x)>high_x)contact.x=std::nextafter(contact.x,-std::numeric_limits<float>::infinity());
                if(double(contact.x)<low_x)contact.x=std::nextafter(contact.x,std::numeric_limits<float>::infinity());
            }else contact.x=std::clamp(contact.x,std::min({cell.xtl,cell.xtr,cell.xbl,cell.xbr}),
                std::max({cell.xtl,cell.xtr,cell.xbl,cell.xbr}));
            if(const auto surface=trace(from.surface,from.point,contact))end={contact,*surface};
            else return Prefix{from,0};
            return Prefix{end,low};
        };
        SurfacePoint current{a,start};const Point input{b.x-a.x,b.z-a.z};Point remaining=input;bool clipped=false;
        if(trajectory)trajectory->push_back({0,current});
        const auto finish=[&](Motion result){if(trajectory)trajectory->push_back({1,result.end});return result;};
        float remaining_fraction=1;
        for(unsigned collision=0;collision<4;++collision){
            const auto previous_surface=current.surface;
            const auto step=prefix(current,{current.point.x+remaining.x,current.point.z+remaining.z});
            current=step.end;
            if(trajectory&&step.fraction<1)trajectory->push_back({1-remaining_fraction*(1-step.fraction),current});
            if(step.fraction==1)return finish(Motion{current,clipped,false,
                {remaining.x/remaining_fraction,remaining.z/remaining_fraction}});
            clipped=true;remaining.x*=1-step.fraction;remaining.z*=1-step.fraction;
            remaining_fraction*=1-step.fraction;
            // A slide can reach an explicit floor connector or a wall tip.
            // Once the original input ray is clear on that connected surface,
            // spend the remaining interval in the requested direction instead
            // of repeatedly projecting the old wall tangent at the corner.
            const Point resumed{input.x*remaining_fraction,input.z*remaining_fraction};
            const Point resumed_end{current.point.x+resumed.x,current.point.z+resumed.z};
            if(minimum_tangent_ratio>0&&current.surface!=previous_surface&&std::hypot(resumed.x,resumed.z)>.0001f){
                if(const auto surface=trace(current.surface,current.point,resumed_end))
                    return finish(Motion{{resumed_end,*surface},true,false,input});
            }
            // At a shared corner the usable tangent can belong to the next
            // cell rather than the starting layer. Inspect only explicitly
            // connected cells containing this contact, with a fixed local
            // work bound. Candidate movement still traces from the original
            // surface; XY overlap alone never creates a crossing.
            std::vector<Surface> contacts{current.surface};
            for(std::size_t index=0;index<contacts.size();++index){
                const auto edges=edges_.find(contacts[index]);if(edges==edges_.end())continue;
                for(const auto next:edges->second){
                    float enter,leave;
                    if(contacts.size()<16&&std::find(contacts.begin(),contacts.end(),next)==contacts.end()&&
                        interval(next,current.point,current.point,enter,leave))contacts.push_back(next);
                }
            }
            Point tangent{};double best=0;
            for(const auto contact:contacts){
            const auto& t=mesh_.trapezoid(contact.plane,contact.cell);
            const bool connector=std::abs(t.yb-t.yt)<1e-6f;
            const double left=connector?0:(double(t.xbl)-t.xtl)/(double(t.yb)-t.yt),
                right=connector?0:(double(t.xbr)-t.xtr)/(double(t.yb)-t.yt);
            const double li=connector?std::min({t.xtl,t.xtr,t.xbl,t.xbr}):t.xtl-left*t.yt,
                ri=connector?std::max({t.xtl,t.xtr,t.xbl,t.xbr}):t.xtr-right*t.yt;
            // Inward normals of the same four half-planes as interval().
            const double walls[][3]={{0,1,-std::min(t.yt,t.yb)},
                {0,-1,std::max(t.yt,t.yb)},{1,-left,-li},{-1,right,ri}};
            for(const auto& wall:walls){
                const double squared=wall[0]*wall[0]+wall[1]*wall[1];
                const double clearance=(wall[0]*current.point.x+wall[1]*current.point.z+wall[2])/std::sqrt(squared);
                const double outward=wall[0]*remaining.x+wall[1]*remaining.z;
                if(clearance>.03||outward>=-1e-7)continue;
                Point projected{float(remaining.x-wall[0]*outward/squared),
                                float(remaining.z-wall[1]*outward/squared)};
                const float projected_length=std::hypot(projected.x,projected.z);
                const float length=std::hypot(remaining.x,remaining.z);
                // Held-input stability policy: within two degrees of head-on,
                // stop rather than magnifying tiny alternating tangent signs
                // to the retail .33 speed floor. Ordinary projection callers
                // keep their existing response, and glancing input still slides.
                if(projected_length<=.0001f||
                    (minimum_tangent_ratio>0&&projected_length<=length*.034899497f))continue;
                const float tangent_length=std::min(length,std::max(projected_length+tangent_bias*length,minimum_tangent_ratio*length));
                projected.x*=tangent_length/projected_length;
                projected.z*=tangent_length/projected_length;
                const auto candidate=prefix(current,{current.point.x+projected.x,current.point.z+projected.z});
                const double dx=candidate.end.point.x-current.point.x,dz=candidate.end.point.z-current.point.z;
                // A second wall must not project the first tangent backwards
                // against the held input. That creates a repeatable two-wall
                // bounce and makes authoritative goals mimic reverse/strafe
                // keys. Rank and accept progress against the original request.
                const double progress=dx*input.x+dz*input.z;
                if(std::hypot(dx,dz)>.0001&&progress>best){best=progress;tangent=projected;}
            }
            }
            if(best==0)return finish(Motion{current,true,true});
            remaining=tangent;
        }
        return finish(Motion{current,true,true});
    }
    std::optional<std::vector<SurfacePoint>> route(Surface start,Point a,Surface goal,Point b,RouteDiagnostics* diagnostics=nullptr) const {
        if(diagnostics)*diagnostics={};
        float enter,leave;
        if(!interval(start,a,a,enter,leave)||!interval(goal,b,b,enter,leave))return {};
        if(auto direct=straight_route(start,a,goal,b))return direct;
        struct Open{float score;Surface surface;};
        struct Later{bool operator()(const Open&a,const Open&b)const{return a.score>b.score;}};
        std::priority_queue<Open,std::vector<Open>,Later> open;
        std::map<Surface,float> costs;std::map<Surface,Surface> parent;
        costs[start]=0;open.push({0,start});
        while(!open.empty()){
            const auto current=open.top().surface;open.pop();if(current==goal)break;
            const auto it=edges_.find(current);if(it==edges_.end())continue;
            for(auto next:it->second){
                const auto cost=costs.at(current)+distance(center(current),center(next));
                if(costs.contains(next)&&costs.at(next)<=cost)continue;
                costs[next]=cost;parent[next]=current;open.push({cost+distance(center(next),center(goal)),next});
            }
        }
        if(!costs.contains(goal))return {};
        std::vector<Surface> path{goal};while(path.back()!=start)path.push_back(parent.at(path.back()));
        std::reverse(path.begin(),path.end());std::vector<SurfacePoint> result{{a,start}};
        for(std::size_t i=1;i<path.size();++i){
            Point midpoint;const auto prev=path[i-1],next=path[i];
            if(!mesh_.portal_midpoint(prev.plane,prev.cell,next.plane,next.cell,midpoint))return {};
            const auto shared=shared_crossing(prev,next,midpoint);if(!shared)return {};
            // Duplicate boundary position records the plane transition explicitly.
            result.push_back({*shared,prev});result.push_back({*shared,next});
        }
        result.push_back({b,goal});
        // Pull across the entire corridor, including floor connectors. Keeping
        // connector midpoints forced actors toward the middle of wide steps.
        // Split the taut line at actual ordered portal crossings afterwards.
        std::vector<SurfacePoint> taut{{a,start}};
        bool valid_taut=true;
        std::vector<CorridorPortal> corridor;
        for(std::size_t k=1;k<path.size();++k){auto edge=mesh_.portal(path[k-1],path[k]);if(!edge){valid_taut=false;break;}corridor.push_back(*edge);}
        const auto points=valid_taut?pull_string(a,b,corridor):std::vector<Point>{};
        std::size_t segment=1;Point cursor=a;Surface current=start;
        const auto cross=[](Point u,Point v){return double(u.x)*v.z-double(u.z)*v.x;};
        for(std::size_t k=0;valid_taut&&k<corridor.size();++k){
            if(diagnostics)diagnostics->failed_portal=k;
            bool found=false;Point crossing;
            while(segment<points.size()){
                const Point d{points[segment].x-cursor.x,points[segment].z-cursor.z};
                const Point e{corridor[k].right.x-corridor[k].left.x,corridor[k].right.z-corridor[k].left.z};
                const Point offset{corridor[k].left.x-cursor.x,corridor[k].left.z-cursor.z};
                const auto denominator=cross(d,e);
                if(std::abs(denominator)>1e-8){
                    const auto t=cross(offset,e)/denominator,u=cross(offset,d)/denominator;
                    if(t>=-1e-5&&t<=1.00001&&u>=-1e-5&&u<=1.00001){const auto clipped=std::clamp(t,0.,1.);crossing={cursor.x+float(clipped)*d.x,cursor.z+float(clipped)*d.z};found=true;}
                }else{
                    // Consecutive portals can meet at the same obstacle corner.
                    const auto len=e.x*e.x+e.z*e.z;
                    const auto u=len>1e-10?((cursor.x-corridor[k].left.x)*e.x+(cursor.z-corridor[k].left.z)*e.z)/len:0.f;
                    if(std::abs(cross(offset,e))<.001&&u>=-1e-5&&u<=1.00001&&
                        (len>1e-10||distance(cursor,corridor[k].left)<.01f)){crossing=cursor;found=true;}
                }
                if(found)break;
                if(!trace(current,cursor,points[segment])){valid_taut=false;break;}
                taut.push_back({points[segment],current});cursor=points[segment++];
            }
            if(found&&valid_taut){const auto shared=shared_crossing(path[k],path[k+1],crossing);
                if(shared)crossing=*shared;else valid_taut=false;}
            float entry,exit;
            if(!found||!valid_taut||!interval(path[k],cursor,crossing,entry,exit)||entry>1e-4||exit<.9999||
                !interval(path[k+1],crossing,crossing,entry,exit)){valid_taut=false;break;}
            taut.push_back({crossing,path[k]});taut.push_back({crossing,path[k+1]});
            cursor=crossing;current=path[k+1];
        }
        if(valid_taut){
            float entry,exit;
            if(!interval(goal,cursor,b,entry,exit)||entry>1e-4||exit<.9999)valid_taut=false;
            else taut.push_back({b,goal});
        }
        if(diagnostics){diagnostics->taut=valid_taut;diagnostics->portal_count=corridor.size();}
        if(valid_taut)result=std::move(taut);
        // Collapse visible runs on a single plane. Retain explicit plane
        // transitions: XY visibility must never jump onto an overlapping floor.
        std::vector<SurfacePoint> smooth{result.front()};
        for(std::size_t i=0;i+1<result.size();){
            std::size_t best=i+1;
            for(std::size_t j=i+1;j<result.size();++j){
                if(result[j].surface.plane!=result[i].surface.plane)break;
                const auto reached=trace(result[i].surface,result[i].point,result[j].point);
                if(reached&&*reached==result[j].surface)best=j;
            }
            smooth.push_back(result[best]);i=best;
        }
        // Search geometry across the entire map, independently of the initial
        // cell corridor. Its valid length is only an upper bound: an obstacle
        // vertex outside this ellipse cannot belong to any shorter path.
        // There is no neighbor-ring cutoff or node-count fallback.
        const auto length=[](const std::vector<SurfacePoint>& points){float total=0;for(std::size_t i=1;i<points.size();++i)total+=distance(points[i-1].point,points[i].point);return total;};
        const auto previous_length=length(smooth);
        std::vector<SurfacePoint> nodes{{a,start},{b,goal}};
        std::map<Surface,std::vector<Point>> surface_points;
        surface_points[start].push_back(a);surface_points[goal].push_back(b);
        const auto add=[&](Point point,Surface surface){
            if(distance(a,point)+distance(point,b)>=previous_length+.01f)return;
            float entry,exit;if(!interval(surface,point,point,entry,exit))return;
            auto& existing=surface_points[surface];
            for(const auto& candidate:existing)if(distance(candidate,point)<.001f)return;
            existing.push_back(point);
            nodes.push_back({point,surface});
        };
        for(const auto& [surface,neighbors]:edges_){
            const auto& cell=mesh_.trapezoid(surface.plane,surface.cell);
            for(Point point: {Point{cell.xtl,cell.yt},Point{cell.xtr,cell.yt},Point{cell.xbl,cell.yb},Point{cell.xbr,cell.yb}})add(point,surface);
            for(auto next:neighbors)if(next.plane!=surface.plane){
                if(auto edge=mesh_.portal(surface,next))for(auto point:{edge->left,edge->right})
                    if(const auto shared=shared_crossing(surface,next,point)){add(*shared,surface);add(*shared,next);}
            }
        }
        if(diagnostics)diagnostics->visibility_nodes=nodes.size();
        {
            struct Candidate{float score;std::size_t index;};
            struct Compare{bool operator()(Candidate a,Candidate b)const{return a.score>b.score;}};
            std::priority_queue<Candidate,std::vector<Candidate>,Compare> frontier;
            std::vector<float> costs(nodes.size(),std::numeric_limits<float>::infinity());
            std::vector<std::size_t> parents(nodes.size(),nodes.size());std::vector<bool> closed(nodes.size());
            costs[0]=0;frontier.push({distance(a,b),0});
            while(!frontier.empty()){
                const auto i=frontier.top().index;frontier.pop();if(closed[i])continue;closed[i]=true;if(i==1)break;
                for(std::size_t j=0;j<nodes.size();++j){
                    if(closed[j]||i==j)continue;
                    const auto step=distance(nodes[i].point,nodes[j].point),cost=costs[i]+step;
                    if(cost>=costs[j]||cost+distance(nodes[j].point,b)>std::min(previous_length,costs[1])+1e-5f)continue;
                    if(!straight_route(nodes[i].surface,nodes[i].point,nodes[j].surface,nodes[j].point))continue;
                    costs[j]=cost;parents[j]=i;frontier.push({cost+distance(nodes[j].point,b),j});
                }
            }
            if(std::isfinite(costs[1])&&costs[1]<previous_length-1e-5f){
                std::vector<SurfacePoint> improved;
                for(std::size_t i=1;;i=parents[i]){improved.push_back(nodes[i]);if(i==0)break;}
                std::reverse(improved.begin(),improved.end());
                std::vector<SurfacePoint> expanded{improved.front()};
                for(std::size_t i=1;i<improved.size();++i){
                    const auto edge=straight_route(improved[i-1].surface,improved[i-1].point,improved[i].surface,improved[i].point);
                    if(!edge)throw std::runtime_error("visibility route lost a verified edge");
                    expanded.insert(expanded.end(),edge->begin()+1,edge->end());
                }
                smooth=std::move(expanded);
                if(diagnostics)diagnostics->visibility_improved=true;
            }
        }
        return smooth;
    }
};
}
