#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <sstream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fw::gw::navigation {

struct Point {
    float x = 0;
    float z = 0;
};
struct Surface {
    std::size_t plane=0,cell=0;
    auto operator<=>(const Surface&) const=default;
};
struct SurfaceLink { Surface a,b; };

struct Trapezoid {
    std::array<std::int32_t, 4> neighbours{};
    std::int32_t portal = -1;
    float yt = 0, yb = 0;
    float xtl = 0, xtr = 0, xbl = 0, xbr = 0;
};

struct Plane {
    std::vector<Trapezoid> trapezoids;
};

struct Route {
    std::vector<Point> waypoints;
    std::vector<std::size_t> cells;
    float length = 0;
};

struct CorridorPortal { Point left,right; };
// Pull a taut string through ordered portals. This minimizes geometric length
// inside the selected corridor; it does not change A*'s corridor selection.
inline std::vector<Point> pull_string(Point start,Point goal,std::vector<CorridorPortal> portals) {
    portals.insert(portals.begin(),{start,start});portals.push_back({goal,goal});
    const auto area=[](Point a,Point b,Point c){return (c.x-a.x)*(b.z-a.z)-(b.x-a.x)*(c.z-a.z);};
    const auto same=[](Point a,Point b){return std::hypot(a.x-b.x,a.z-b.z)<0.0001f;};
    std::vector<Point> result{start};Point apex=start,left=start,right=start;
    std::size_t apex_index=0,left_index=0,right_index=0;
    for(std::size_t i=1;i<portals.size();++i){
        const auto next_left=portals[i].left,next_right=portals[i].right;
        if(area(apex,right,next_right)<=0){
            if(same(apex,right)||area(apex,left,next_right)>0){right=next_right;right_index=i;}
            else {if(!same(result.back(),left))result.push_back(left);apex=left;apex_index=left_index;
                left=right=apex;left_index=right_index=apex_index;i=apex_index;continue;}
        }
        if(area(apex,left,next_left)>=0){
            if(same(apex,left)||area(apex,right,next_left)<0){left=next_left;left_index=i;}
            else {if(!same(result.back(),right))result.push_back(right);apex=right;apex_index=right_index;
                left=right=apex;left_index=right_index=apex_index;i=apex_index;continue;}
        }
    }
    if(!same(result.back(),goal))result.push_back(goal);
    return result;
}

class Mesh {
    std::uint32_t asset_hash_ = 0;
    bool has_bounds_ = false;
    std::array<float, 4> bounds_{}; // min x, min z, max x, max z
    std::vector<Plane> planes_;
    std::vector<SurfaceLink> surface_links_;

    static std::uint32_t read_u32(std::istream& in) {
        std::array<unsigned char, 4> b{};
        if (!in.read(reinterpret_cast<char*>(b.data()), b.size()))
            throw std::runtime_error("truncated FWNAV001 asset");
        return std::uint32_t(b[0]) | (std::uint32_t(b[1]) << 8) |
               (std::uint32_t(b[2]) << 16) | (std::uint32_t(b[3]) << 24);
    }
    static float read_f32(std::istream& in) {
        return std::bit_cast<float>(read_u32(in));
    }
    static float distance(Point a, Point b) { return std::hypot(a.x - b.x, a.z - b.z); }
    static Point center(const Trapezoid& t) {
        return {(t.xtl + t.xtr + t.xbl + t.xbr) * 0.25f, (t.yt + t.yb) * 0.5f};
    }
    static bool contains(const Trapezoid& t, Point p) {
        constexpr float epsilon = 0.01f;
        const auto low_z = std::min(t.yt, t.yb) - epsilon;
        const auto high_z = std::max(t.yt, t.yb) + epsilon;
        if (p.z < low_z || p.z > high_z) return false;
        const float dz = t.yb - t.yt;
        if (std::abs(dz) < 1e-6f)
            return p.x >= std::min({t.xtl,t.xtr,t.xbl,t.xbr})-epsilon &&
                   p.x <= std::max({t.xtl,t.xtr,t.xbl,t.xbr})+epsilon;
        const float u = std::abs(dz) < 1e-6f ? 0.f : std::clamp((p.z - t.yt) / dz, 0.f, 1.f);
        const float left = t.xtl + (t.xbl - t.xtl) * u;
        const float right = t.xtr + (t.xbr - t.xtr) * u;
        return p.x >= std::min(left, right) - epsilon && p.x <= std::max(left, right) + epsilon;
    }
    static bool shared_portal(const Trapezoid& a, const Trapezoid& b, Point& midpoint, CorridorPortal* endpoints=nullptr) {
        constexpr float epsilon = 0.05f;
        const float atop = std::min(a.yt, a.yb), abottom = std::max(a.yt, a.yb);
        const float btop = std::min(b.yt, b.yb), bbottom = std::max(b.yt, b.yb);
        const auto left_edge = [](const Trapezoid& t, float z) {
            const float dz = t.yb - t.yt;
            if (std::abs(dz)<1e-6f) return std::min({t.xtl,t.xtr,t.xbl,t.xbr});
            const float u = std::abs(dz) < 1e-6f ? 0.f : std::clamp((z - t.yt) / dz, 0.f, 1.f);
            return t.xtl + (t.xbl - t.xtl) * u;
        };
        const auto right_edge = [](const Trapezoid& t, float z) {
            const float dz = t.yb - t.yt;
            if (std::abs(dz)<1e-6f) return std::max({t.xtl,t.xtr,t.xbl,t.xbr});
            const float u = std::abs(dz) < 1e-6f ? 0.f : std::clamp((z - t.yt) / dz, 0.f, 1.f);
            return t.xtr + (t.xbr - t.xtr) * u;
        };
        const float overlap_top = std::max(atop, btop), overlap_bottom = std::min(abottom, bbottom);
        if (overlap_bottom - overlap_top > epsilon) {
            const float z = (overlap_top + overlap_bottom) * 0.5f;
            const float candidates[][2] = {{right_edge(a,z), left_edge(b,z)},
                {left_edge(a,z), right_edge(b,z)}};
            for (unsigned side=0;side<2;++side) {const auto& c=candidates[side];if (std::abs(c[0]-c[1]) <= epsilon) {
                if(endpoints){
                    const auto edge=[&](float y){return side==0?(right_edge(a,y)+left_edge(b,y))*.5f:(left_edge(a,y)+right_edge(b,y))*.5f;};
                    *endpoints={{edge(overlap_top),overlap_top},{edge(overlap_bottom),overlap_bottom}};
                }
                midpoint = {(c[0]+c[1])*0.5f,z}; return true;
            }}
        }
        const float shared_z = std::abs(abottom-btop) <= epsilon ? (abottom+btop)*0.5f :
                               std::abs(bbottom-atop) <= epsilon ? (bbottom+atop)*0.5f :
                               std::numeric_limits<float>::quiet_NaN();
        if (std::isfinite(shared_z)) {
            const float left = std::max(left_edge(a,shared_z),left_edge(b,shared_z));
            const float right = std::min(right_edge(a,shared_z),right_edge(b,shared_z));
            if (right-left > epsilon) { if(endpoints)*endpoints={{left,shared_z},{right,shared_z}};midpoint={(left+right)*0.5f,shared_z}; return true; }
        }
        return false;
    }
public:
    std::optional<std::array<float,4>> source_bounds() const {
        return has_bounds_?std::optional(bounds_):std::nullopt;
    }
    static Mesh load(const std::filesystem::path& path) {
        std::error_code ec;
        const auto bytes = std::filesystem::file_size(path, ec);
        if (ec || bytes < 52 || bytes > 512ull * 1024 * 1024)
            throw std::runtime_error("invalid FWNAV001 asset size");
        std::ifstream in(path, std::ios::binary);
        if (!in) throw std::runtime_error("could not open FWNAV001 asset");
        std::array<char, 8> magic{};
        if (!in.read(magic.data(), magic.size()) || magic != std::array<char,8>{'F','W','N','A','V','0','0','1'})
            throw std::runtime_error("invalid FWNAV001 magic");
        if (read_u32(in) != 1) throw std::runtime_error("unsupported FWNAV001 version");

        Mesh result;
        result.asset_hash_ = read_u32(in);
        if (read_u32(in) != 0xEEFE704Cu) throw std::runtime_error("invalid PATH_INFO signature");
        const auto path_version = read_u32(in);
        (void)read_u32(in); // source sequence
        if (!path_version) throw std::runtime_error("invalid PATH_INFO version");
        const auto has_bounds = read_u32(in);
        if (has_bounds > 1) throw std::runtime_error("invalid FWNAV001 bounds flag");
        result.has_bounds_ = has_bounds != 0;
        for (auto& value : result.bounds_) {
            value = read_f32(in);
            if (!std::isfinite(value)) throw std::runtime_error("non-finite FWNAV001 bounds");
        }
        if (result.has_bounds_ && (result.bounds_[0] >= result.bounds_[2] || result.bounds_[1] >= result.bounds_[3]))
            throw std::runtime_error("invalid FWNAV001 bounds");

        const auto plane_count = read_u32(in);
        if (!plane_count || plane_count > 4096) throw std::runtime_error("FWNAV001 plane count limit");
        result.planes_.reserve(plane_count);
        std::uint64_t total_trapezoids = 0;
        for (std::uint32_t pi = 0; pi < plane_count; ++pi) {
            const auto count = read_u32(in);
            total_trapezoids += count;
            if (!count || count > 1'000'000 || total_trapezoids > 2'000'000)
                throw std::runtime_error("FWNAV001 trapezoid count limit");
            Plane plane;
            plane.trapezoids.reserve(count);
            for (std::uint32_t i = 0; i < count; ++i) {
                Trapezoid t;
                for (auto& neighbour : t.neighbours) {
                    neighbour = std::bit_cast<std::int32_t>(read_u32(in));
                    if (neighbour < -1 || neighbour >= static_cast<std::int32_t>(count))
                        throw std::runtime_error("FWNAV001 neighbor index out of range");
                }
                t.portal = std::bit_cast<std::int32_t>(read_u32(in));
                t.yt = read_f32(in); t.yb = read_f32(in);
                t.xtl = read_f32(in); t.xtr = read_f32(in);
                t.xbl = read_f32(in); t.xbr = read_f32(in);
                for (float value : {t.yt,t.yb,t.xtl,t.xtr,t.xbl,t.xbr})
                    if (!std::isfinite(value)) throw std::runtime_error("non-finite FWNAV001 trapezoid");
                if (std::abs(t.yb - t.yt) < 1e-6f &&
                    std::abs(t.xtr - t.xtl) < 1e-6f && std::abs(t.xbr - t.xbl) < 1e-6f)
                    throw std::runtime_error("degenerate FWNAV001 trapezoid");
                plane.trapezoids.push_back(t);
            }
            result.planes_.push_back(std::move(plane));
        }
        if (in.peek() != std::char_traits<char>::eof()) throw std::runtime_error("trailing FWNAV001 data");
        auto links_path=path;links_path += ".links";
        std::ifstream links(links_path);
        if(links){
            std::string magic;std::uint32_t asset;std::size_t planes,count;
            if(!(links>>magic>>asset>>planes>>count)||magic!="FWLINK01"||asset!=result.asset_hash_||planes!=result.planes_.size()||count>2'000'000)
                throw std::runtime_error("invalid navigation links header");
            std::set<std::pair<Surface,Surface>> unique;
            for(std::size_t i=0;i<count;++i){
                SurfaceLink link;Point midpoint;
                if(!(links>>link.a.plane>>link.a.cell>>link.b.plane>>link.b.cell)||link.a==link.b||
                   link.a.plane>=planes||link.b.plane>=planes||
                   link.a.cell>=result.trapezoid_count(link.a.plane)||link.b.cell>=result.trapezoid_count(link.b.plane)||
                   !result.portal_midpoint(link.a.plane,link.a.cell,link.b.plane,link.b.cell,midpoint)||
                   !unique.emplace(link.a,link.b).second)
                    throw std::runtime_error("invalid navigation portal link");
                result.surface_links_.push_back(link);
            }
            for(auto [a,b]:unique)if(!unique.contains({b,a}))throw std::runtime_error("navigation link missing reciprocal");
            std::string extra;if(links>>extra)throw std::runtime_error("trailing navigation links data");
        }
        return result;
    }

    std::uint32_t asset_hash() const { return asset_hash_; }
    const std::vector<SurfaceLink>& surface_links() const {return surface_links_;}
    std::size_t plane_count() const { return planes_.size(); }
    std::size_t trapezoid_count(std::size_t plane) const { return planes_.at(plane).trapezoids.size(); }
    const Trapezoid& trapezoid(std::size_t plane, std::size_t index) const {
        return planes_.at(plane).trapezoids.at(index);
    }
    bool portal_midpoint(std::size_t plane,std::size_t cell,std::size_t peer_plane,
                         std::size_t peer_cell,Point& point) const {
        return shared_portal(trapezoid(plane,cell),trapezoid(peer_plane,peer_cell),point);
    }
    std::optional<CorridorPortal> portal(Surface from,Surface to) const {
        Point midpoint;CorridorPortal edge;
        if(!shared_portal(trapezoid(from.plane,from.cell),trapezoid(to.plane,to.cell),midpoint,&edge))return {};
        const auto a=center(trapezoid(from.plane,from.cell)),b=center(trapezoid(to.plane,to.cell));
        const auto cross=(b.x-a.x)*(edge.left.z-midpoint.z)-(b.z-a.z)*(edge.left.x-midpoint.x);
        if(cross<0)std::swap(edge.left,edge.right);
        return edge;
    }
    std::optional<std::size_t> locate_plane(Point point) const {
        if (!std::isfinite(point.x) || !std::isfinite(point.z)) return std::nullopt;
        if (has_bounds_ && (point.x < bounds_[0] || point.z < bounds_[1] ||
                            point.x > bounds_[2] || point.z > bounds_[3])) return std::nullopt;
        for (std::size_t plane = 0; plane < planes_.size(); ++plane)
            for (const auto& cell : planes_[plane].trapezoids)
                if (contains(cell, point)) return plane;
        return std::nullopt;
    }
    bool walkable(std::size_t plane, Point point) const {
        if (!std::isfinite(point.x) || !std::isfinite(point.z) || plane >= planes_.size()) return false;
        if (has_bounds_ && (point.x < bounds_[0] || point.z < bounds_[1] || point.x > bounds_[2] || point.z > bounds_[3]))
            return false;
        for (const auto& t : planes_[plane].trapezoids) if (contains(t, point)) return true;
        return false;
    }
    std::optional<std::size_t> locate_cell(std::size_t plane, Point point) const {
        if (plane >= planes_.size() || !std::isfinite(point.x) || !std::isfinite(point.z)) return std::nullopt;
        if (has_bounds_ && (point.x < bounds_[0] || point.z < bounds_[1] ||
                            point.x > bounds_[2] || point.z > bounds_[3])) return std::nullopt;
        const auto& cells = planes_[plane].trapezoids;
        for (std::size_t i = 0; i < cells.size(); ++i) if (contains(cells[i], point)) return i;
        return std::nullopt;
    }
    bool route_stays_walkable(std::size_t plane, const Route& route) const {
        if (plane >= planes_.size() || route.waypoints.size() < 2 || route.cells.empty()) return false;
        for (std::size_t i = 1; i < route.waypoints.size(); ++i) {
            const auto a = route.waypoints[i-1], b = route.waypoints[i];
            const auto requested_steps = std::ceil(distance(a,b) / 10.f);
            const auto steps = static_cast<int>(std::clamp(requested_steps, 1.f, 100000.f));
            for (int s = 0; s <= steps; ++s) {
                const float t = float(s)/float(steps);
                const Point sample{a.x+(b.x-a.x)*t,a.z+(b.z-a.z)*t};
                const bool in_path_cell = std::any_of(route.cells.begin(), route.cells.end(), [&](std::size_t cell) {
                    return cell < planes_[plane].trapezoids.size() && contains(planes_[plane].trapezoids[cell], sample);
                });
                if (!in_path_cell) return false;
            }
        }
        return true;
    }
    std::size_t discard_unverified_portals() {
        std::size_t discarded=0;
        for(auto& plane:planes_)for(std::size_t i=0;i<plane.trapezoids.size();++i){
            auto& cell=plane.trapezoids[i];
            for(auto& neighbour:cell.neighbours){
                if(neighbour<0)continue;
                Point portal{};
                if(static_cast<std::size_t>(neighbour)>=plane.trapezoids.size()||
                   static_cast<std::size_t>(neighbour)==i||
                   !shared_portal(cell,plane.trapezoids[neighbour],portal)||
                   !contains(cell,portal)||!contains(plane.trapezoids[neighbour],portal)){
                    neighbour=-1;++discarded;
                }
            }
        }
        return discarded;
    }
    bool validate_topology() const {
        for (const auto& plane : planes_) {
            for (std::size_t i = 0; i < plane.trapezoids.size(); ++i) {
                const auto& cell = plane.trapezoids[i];
                for (auto neighbour : cell.neighbours) {
                    if (neighbour < 0) continue;
                    if (static_cast<std::size_t>(neighbour) >= plane.trapezoids.size()) continue;
                    if (i == static_cast<std::size_t>(neighbour)) return false;
                    const auto& other = plane.trapezoids[static_cast<std::size_t>(neighbour)];
                    Point portal{};
                    if (!shared_portal(cell, other, portal)) return false;
                    if (!contains(cell, portal) || !contains(other, portal)) return false;
                }
            }
        }
        return true;
    }
    std::optional<Route> find_path(std::size_t plane_index, Point start, Point goal) const {
        if (!walkable(plane_index, start) || !walkable(plane_index, goal)) return std::nullopt;
        const auto& cells = planes_[plane_index].trapezoids;
        const auto locate = [&](Point p) -> std::size_t {
            for (std::size_t i = 0; i < cells.size(); ++i) if (contains(cells[i], p)) return i;
            return cells.size();
        };
        const auto from = locate(start);
        const auto to = locate(goal);
        if (from == cells.size() || to == cells.size()) return std::nullopt;
        Route route;
        if (from == to) {
            route.waypoints = {start, goal}; route.cells = {from}; route.length = distance(start, goal); return route;
        }

        struct Open { float score; std::size_t cell; };
        struct Later { bool operator()(const Open& a, const Open& b) const {
            return a.score == b.score ? a.cell > b.cell : a.score > b.score;
        }};
        std::priority_queue<Open, std::vector<Open>, Later> open;
        std::vector<float> cost(cells.size(), std::numeric_limits<float>::infinity());
        std::vector<std::size_t> parent(cells.size(), cells.size());
        cost[from] = 0;
        open.push({distance(center(cells[from]), center(cells[to])), from});
        while (!open.empty()) {
            const auto current = open.top().cell;
            open.pop();
            if (current == to) break;
            for (const auto neighbour : cells[current].neighbours) {
                if (neighbour < 0) continue;
                const auto next = static_cast<std::size_t>(neighbour);
                if (next >= cells.size()) continue;
                const float next_cost = cost[current] + distance(center(cells[current]), center(cells[next]));
                if (next_cost >= cost[next]) continue;
                cost[next] = next_cost;
                parent[next] = current;
                open.push({next_cost + distance(center(cells[next]), center(cells[to])), next});
            }
        }
        if (!std::isfinite(cost[to])) return std::nullopt;
        std::vector<std::size_t> reversed;
        for (auto cell = to; cell != from; cell = parent[cell]) {
            if (cell >= cells.size() || parent[cell] >= cells.size()) return std::nullopt;
            reversed.push_back(cell);
        }
        reversed.push_back(from);
        std::reverse(reversed.begin(), reversed.end());
        route.cells = reversed;
        std::vector<CorridorPortal> corridor;
        for (std::size_t i = 1; i < reversed.size(); ++i) {
            const auto a = reversed[i - 1], b = reversed[i];
            auto edge=portal({plane_index,a},{plane_index,b});if(!edge)return {};
            corridor.push_back(*edge);
        }
        route.waypoints=pull_string(start,goal,std::move(corridor));
        for (std::size_t i = 1; i < route.waypoints.size(); ++i)
            route.length += distance(route.waypoints[i - 1], route.waypoints[i]);
        return route;
    }
};

inline std::filesystem::path catalog_asset_path(std::string value,const std::filesystem::path& asset_directory = {}) {
    if(!value.empty()&&value.back()=='\r')value.pop_back();
    std::filesystem::path path(value);
    if(path.empty())throw std::runtime_error("empty navmesh asset path");
    if(!path.is_absolute()) {
        if(asset_directory.empty()||!asset_directory.is_absolute())
            throw std::runtime_error("relative navmesh assets require an absolute catalog directory");
        path=(asset_directory/path).lexically_normal();
    }
    return path;
}
inline std::map<std::uint32_t, std::shared_ptr<const Mesh>> load_catalog(std::istream& input,
        const std::filesystem::path& asset_directory = {}) {
    std::map<std::uint32_t, std::shared_ptr<const Mesh>> result;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const auto split = line.find('=');
        if (split == std::string::npos || !split || split + 1 == line.size())
            throw std::runtime_error("invalid navmesh catalog row");
        std::uint32_t map_id = 0;
        const auto* first = line.data();
        const auto* last = first + split;
        const auto parsed = std::from_chars(first, last, map_id);
        if (parsed.ec != std::errc{} || parsed.ptr != last || map_id > 65535)
            throw std::runtime_error("invalid navmesh logical map id");
        const auto path=catalog_asset_path(line.substr(split+1),asset_directory);
        auto mesh = std::make_shared<Mesh>(Mesh::load(path));
        // Preserve DAT geometry; ambiguous links become blocked edges rather
        // than permitting traversal through a portal we cannot verify.
        mesh->discard_unverified_portals();
        if (!mesh->validate_topology()) throw std::runtime_error("invalid navmesh neighbor topology");
        if (!result.emplace(map_id, std::move(mesh)).second)
            throw std::runtime_error("duplicate navmesh logical map id");
    }
    if (result.empty()) throw std::runtime_error("navmesh map catalog is empty");
    return result;
}

} // namespace fw::gw::navigation
