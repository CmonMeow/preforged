#pragma once
#include "gw_navigation.h"
#include <functional>
#include <string_view>

namespace fw::gw::map_portals {
using Point = navigation::Point; // z is the Guild Wars world y coordinate.
struct Box { float min_x=0,min_z=0,max_x=0,max_z=0; };
struct Arrival { std::uint32_t map=0,plane=0;Point point{};float facing=0; };
struct Route {
    std::string id;
    std::uint32_t source=0,destination=0,source_asset=0,destination_asset=0,source_plane=0;
    Box trigger{};Arrival arrival{};Point outward{};
    std::uint32_t cooldown_ms=2500;
    // This catalog deliberately does not claim that the client supplied the
    // trigger shape. Observed endpoints + wiki adjacency establish candidates.
    std::string provenance;
};
inline bool finite(Point p){return std::isfinite(p.x)&&std::isfinite(p.z);}
inline bool contains(Box b,Point p){return p.x>=b.min_x&&p.x<=b.max_x&&p.z>=b.min_z&&p.z<=b.max_z;}
inline std::optional<float> enter(Box b,Point a,Point end){
    // Require a positive traversal interval. Closed transverse boundaries
    // allow the genuine corridor's outer navigation-edge lanes. Detector
    // separately requires crossing the inward face in the outward direction;
    // travelling along that face or a single corner touch never transfers.
    double lo=0,hi=1;
    const auto slab=[&](double start,double delta,double low,double high){
        if(std::abs(delta)<1e-12)return start>=low&&start<=high;
        double l=(low-start)/delta,h=(high-start)/delta;
        if(l>h)std::swap(l,h);lo=std::max(lo,l);hi=std::min(hi,h);return lo<hi;
    };
    if(!slab(a.x,double(end.x)-a.x,b.min_x,b.max_x)||
       !slab(a.z,double(end.z)-a.z,b.min_z,b.max_z)||hi-lo<=1e-8)return {};
    return static_cast<float>(lo);
}
inline bool crosses_inward_face(const Route& r,Point a,Point b){
    if(std::abs(r.outward.x)>=std::abs(r.outward.z))return r.outward.x>0?
        a.x<=r.trigger.min_x&&b.x>r.trigger.min_x:a.x>=r.trigger.max_x&&b.x<r.trigger.max_x;
    return r.outward.z>0?a.z<=r.trigger.min_z&&b.z>r.trigger.min_z:
        a.z>=r.trigger.max_z&&b.z<r.trigger.max_z;
}
inline bool past_inward_face(const Route& r,Point p){
    if(!contains(r.trigger,p))return false;
    return std::abs(r.outward.x)>=std::abs(r.outward.z)?
        p.x>r.trigger.min_x&&p.x<r.trigger.max_x:p.z>r.trigger.min_z&&p.z<r.trigger.max_z;
}
inline bool inside_cell(const navigation::Trapezoid& t,Point p){
    if(p.z<std::min(t.yt,t.yb)||p.z>std::max(t.yt,t.yb)||std::abs(t.yb-t.yt)<1e-6f)return false;
    const double u=(double(p.z)-t.yt)/(double(t.yb)-t.yt);
    const double a=t.xtl+(double(t.xbl)-t.xtl)*u,b=t.xtr+(double(t.xbr)-t.xtr)*u;
    return p.x>=std::min(a,b)&&p.x<=std::max(a,b);
}
inline bool covered_by_connected_cells(Box b,const navigation::Mesh& mesh,std::uint32_t plane){
    // Prove full area coverage by the exact trapezoid union, rather than only
    // testing corners or a grid of points. The active component uses genuine
    // DAT neighbour edges on this exact plane; overlapping floors never fill
    // a gap. Zero-height connector cells participate in connectivity only.
    const auto seed=mesh.locate_cell(plane,{(b.min_x+b.max_x)*.5f,(b.min_z+b.max_z)*.5f});
    if(!seed)return false;
    std::set<std::size_t> reachable{*seed};std::vector<std::size_t> pending{*seed};
    while(!pending.empty()){
        const auto c=pending.back();pending.pop_back();
        for(auto n:mesh.trapezoid(plane,c).neighbours)if(n>=0&&std::size_t(n)<mesh.trapezoid_count(plane)){
            Point edge;if(mesh.portal_midpoint(plane,c,plane,std::size_t(n),edge)&&reachable.insert(std::size_t(n)).second)
                pending.push_back(std::size_t(n));
        }
    }
    struct Line {double slope,intercept,low,high;};
    struct Strip {Line a,b;};
    std::vector<Strip> cells;std::vector<Line> lines;
    std::vector<double> events{b.min_z,b.max_z};
    for(auto c:reachable){
        const auto& t=mesh.trapezoid(plane,c);
        const double low=std::max(double(b.min_z),double(std::min(t.yt,t.yb))),
                     high=std::min(double(b.max_z),double(std::max(t.yt,t.yb)));
        if(low>high||std::abs(double(t.yb)-t.yt)<1e-6||
           std::max({t.xtl,t.xtr,t.xbl,t.xbr})<b.min_x||std::min({t.xtl,t.xtr,t.xbl,t.xbr})>b.max_x)continue;
        const double ma=(double(t.xbl)-t.xtl)/(double(t.yb)-t.yt),mb=(double(t.xbr)-t.xtr)/(double(t.yb)-t.yt);
        const Line a{ma,t.xtl-ma*t.yt,low,high},right{mb,t.xtr-mb*t.yt,low,high};
        cells.push_back({a,right});lines.push_back(a);lines.push_back(right);events.push_back(low);events.push_back(high);
        if(cells.size()>256)return false; // bounded developer catalog, fail closed
    }
    if(cells.empty())return false;
    lines.push_back({0,b.min_x,b.min_z,b.max_z});lines.push_back({0,b.max_x,b.min_z,b.max_z});
    // Between vertex heights and line-order changes, every covered interval
    // endpoint is linear and its ordering is fixed. Checking both ends and
    // the interior of each such band therefore proves the entire rectangle.
    for(std::size_t i=0;i<lines.size();++i)for(std::size_t j=0;j<i;++j){
        const auto& a=lines[i];const auto& other=lines[j];
        const double low=std::max(a.low,other.low),high=std::min(a.high,other.high),delta=a.slope-other.slope;
        if(low>high||std::abs(delta)<1e-12)continue;
        const double y=(other.intercept-a.intercept)/delta;
        if(y>low&&y<high)events.push_back(y);
    }
    std::sort(events.begin(),events.end());events.erase(std::unique(events.begin(),events.end()),events.end());
    const auto covered=[&](double y){
        std::vector<std::pair<double,double>> intervals;
        for(const auto& c:cells)if(y>=c.a.low&&y<=c.a.high){
            const double a=c.a.slope*y+c.a.intercept,right=c.b.slope*y+c.b.intercept;
            const double low=std::max(double(b.min_x),std::min(a,right)),high=std::min(double(b.max_x),std::max(a,right));
            if(low<=high)intervals.emplace_back(low,high);
        }
        std::sort(intervals.begin(),intervals.end());double cursor=b.min_x;
        for(const auto [low,high]:intervals){if(low>cursor+.01)return false;cursor=std::max(cursor,high);if(cursor>=double(b.max_x)-.01)return true;}
        return false;
    };
    for(std::size_t i=0;i<events.size();++i){
        if(!covered(events[i]))return false;
        if(i&& !covered((events[i-1]+events[i])*.5))return false;
    }
    return true;
}
inline void validate(const Route& r,const navigation::Mesh& source,const navigation::Mesh& destination){
    if(r.id.empty()||r.id.size()>80||r.provenance.empty()||r.provenance.size()>120||
       !r.source||!r.destination||r.source==r.destination||r.source>4095||r.destination>4095||
       !r.source_asset||!r.destination_asset||source.asset_hash()!=r.source_asset||
       destination.asset_hash()!=r.destination_asset||r.source_plane>=source.plane_count()||
       r.arrival.map!=r.destination||r.arrival.plane>=destination.plane_count()||
       !finite(r.arrival.point)||!std::isfinite(r.arrival.facing)||!finite(r.outward)||
       std::hypot(r.outward.x,r.outward.z)<.99f||std::hypot(r.outward.x,r.outward.z)>1.01f||
       !r.cooldown_ms||r.cooldown_ms>30'000)
        throw std::runtime_error("invalid map portal identity/asset/arrival: "+r.id);
    const auto b=r.trigger;
    if(!finite({b.min_x,b.min_z})||!finite({b.max_x,b.max_z})||b.min_x>=b.max_x||b.min_z>=b.max_z||
       b.max_x-b.min_x>2000||b.max_z-b.min_z>2000)
        throw std::runtime_error("invalid map portal trigger bounds: "+r.id);
    const std::array<Point,4> vertices{{{b.min_x,b.min_z},{b.max_x,b.min_z},{b.max_x,b.max_z},{b.min_x,b.max_z}}};
    // Prefer the cheap convex-cell proof. A whole physical doorway may span
    // several connected DAT cells; its full union is proven analytically.
    bool contained=false;
    for(std::size_t c=0;c<source.trapezoid_count(r.source_plane);++c){
        const auto& t=source.trapezoid(r.source_plane,c);
        if(std::all_of(vertices.begin(),vertices.end(),[&](Point p){return inside_cell(t,p);})){contained=true;break;}
    }
    if(!contained)contained=covered_by_connected_cells(b,source,r.source_plane);
    if(!contained||!destination.walkable(r.arrival.plane,r.arrival.point))
        throw std::runtime_error("map portal trigger/arrival outside pinned navigation: "+r.id);
}
class Catalog {
    std::vector<Route> routes_;
public:
    static Catalog read(const std::filesystem::path& path){
        std::ifstream in(path);if(!in)throw std::runtime_error("map portal catalog unavailable");
        Catalog result;std::set<std::string> ids;std::string line;bool header=false;std::size_t line_number=0;
        while(std::getline(in,line)){
            ++line_number;if(line.size()>2048)throw std::runtime_error("map portal catalog line too long");
            const auto first=line.find_first_not_of(" \t\r");if(first==std::string::npos||line[first]=='#')continue;
            std::istringstream row(line);if(!header){std::string magic,extra;
                if(!(row>>magic)||magic!="FWGATE01"||(row>>extra))throw std::runtime_error("invalid map portal catalog header");
                header=true;continue;}
            Route r;std::string extra;
            if(!(row>>r.id>>r.source>>r.destination>>r.source_asset>>r.destination_asset>>r.source_plane>>
                 r.trigger.min_x>>r.trigger.min_z>>r.trigger.max_x>>r.trigger.max_z>>r.arrival.plane>>
                 r.arrival.point.x>>r.arrival.point.z>>r.arrival.facing>>r.outward.x>>r.outward.z>>r.cooldown_ms>>r.provenance)||
               (row>>extra)||!ids.insert(r.id).second||result.routes_.size()>=256)
                throw std::runtime_error("invalid map portal catalog row "+std::to_string(line_number));
            r.arrival.map=r.destination;result.routes_.push_back(std::move(r));
        }
        if(in.bad()||!header)throw std::runtime_error("map portal catalog read failed");
        return result;
    }
    const std::vector<Route>& routes()const{return routes_;}
    // The caller resolves logical map -> pinned DAT mesh. Validate once after
    // loading; an unavailable or wrong destination never becomes a fallback.
    void validate_all(const std::function<const navigation::Mesh*(std::uint32_t)>& lookup)const{
        for(std::size_t i=0;i<routes_.size();++i){const auto& r=routes_[i];
            const auto* source=lookup(r.source);const auto* destination=lookup(r.destination);
            if(!source||!destination)throw std::runtime_error("map portal navigation unavailable: "+r.id);
            validate(r,*source,*destination);
            for(std::size_t j=0;j<i;++j){const auto& p=routes_[j];
                if(p.source==r.source&&p.source_plane==r.source_plane&&
                   std::max(p.trigger.min_x,r.trigger.min_x)<std::min(p.trigger.max_x,r.trigger.max_x)&&
                   std::max(p.trigger.min_z,r.trigger.min_z)<std::min(p.trigger.max_z,r.trigger.max_z))
                    throw std::runtime_error("ambiguous overlapping map portal triggers: "+r.id);
            }
        }
    }
};
struct Sample {std::uint32_t map=0,plane=0;std::uint64_t epoch=0,monotonic_ms=0;Point point{};};
struct Crossing {std::string id;std::uint32_t source=0,destination=0,destination_asset=0;std::uint64_t epoch=0;
    Arrival arrival{};std::uint32_t cooldown_ms=0;};
class Detector {
    std::optional<Sample> previous_;
    std::map<std::string,bool> armed_;
    std::optional<Crossing> pending_;
    std::uint64_t cooldown_until_=0;
    void seed(const Catalog& catalog,Sample s){
        resolve(s.monotonic_ms);
        previous_=s;armed_.clear();pending_.reset();
        for(const auto& r:catalog.routes())if(r.source==s.map)
            // The inward face is a valid starting point for an outward step,
            // just as it is between ordinary samples. Suppress only a spawn
            // already beyond that face; inclusive contains() trapped reseeds
            // on the face until the player backed out of the whole trigger.
            armed_[r.id]=r.source_plane!=s.plane||!past_inward_face(r,s.point);
    }
public:
    // Call on map/world initialization, after explicit repositioning, or any
    // movement discontinuity. Spawning inside a gate must never bounce maps.
    void reseed(const Catalog& catalog,Sample s){seed(catalog,s);}
    bool pending()const{return pending_.has_value();}
    // Resolve success AND failure; retries require leaving and re-entering.
    void resolve(std::uint64_t monotonic_ms){
        if(pending_){const auto delay=pending_->cooldown_ms;
            cooldown_until_=monotonic_ms>std::numeric_limits<std::uint64_t>::max()-delay?
                std::numeric_limits<std::uint64_t>::max():monotonic_ms+delay;pending_.reset();}
    }
    std::optional<Crossing> observe(const Catalog& catalog,Sample s,bool authoritative_step=true){
        if(!s.map||!finite(s.point)){previous_.reset();armed_.clear();pending_.reset();return {};}
        if(!previous_||previous_->map!=s.map||previous_->epoch!=s.epoch||
           previous_->monotonic_ms>s.monotonic_ms){seed(catalog,s);return {};}
        const auto prior=*previous_;previous_=s;
        if(!authoritative_step||s.monotonic_ms-prior.monotonic_ms>3000||
           std::hypot(s.point.x-prior.point.x,s.point.z-prior.point.z)>900){seed(catalog,s);return {};}
        std::optional<Crossing> found;float earliest=2;
        for(const auto& r:catalog.routes())if(r.source==s.map){
            const bool was_inside=prior.plane==r.source_plane&&past_inward_face(r,prior.point);
            const bool now_inside=s.plane==r.source_plane&&contains(r.trigger,s.point);
            auto& armed=armed_[r.id];
            if(!now_inside&&!armed)armed=true;
            if(pending_||s.monotonic_ms<cooldown_until_||!armed||was_inside||s.plane!=r.source_plane)continue;
            // A collision-validated step may enter the gate's plane through
            // a real DAT connector. If its start was already inside the XY
            // box beyond its inward face on another floor, that plane change
            // is not a gate entry. A genuine connector at the entry face is
            // still eligible for an outward, authoritative crossing.
            if(prior.plane!=r.source_plane&&past_inward_face(r,prior.point))continue;
            const Point delta{s.point.x-prior.point.x,s.point.z-prior.point.z};
            if(double(delta.x)*r.outward.x+double(delta.z)*r.outward.z<=0)continue;
            if(!crosses_inward_face(r,prior.point,s.point))continue;
            if(const auto at=enter(r.trigger,prior.point,s.point);at&&*at<earliest){
                earliest=*at;found=Crossing{r.id,r.source,r.destination,r.destination_asset,s.epoch,r.arrival,r.cooldown_ms};}
        }
        if(found){armed_[found->id]=false;pending_=found;}
        return found;
    }
};
} // namespace fw::gw::map_portals
