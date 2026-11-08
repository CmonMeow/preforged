#pragma once
#include "gw_shrine_wipe.h"
#include <cmath>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
namespace fw::gw::shrine {
struct Place {double x=0,y=0;unsigned plane=0,cell=0;bool operator==(const Place&)const=default;};
inline bool valid(Place p){return std::isfinite(p.x)&&std::isfinite(p.y);}
enum class Basis {unknown,captured,reviewed,authored,inferred,fixture};
struct Evidence {Basis basis=Basis::unknown;std::string reference;bool operator==(const Evidence&)const=default;};
inline bool documented(const Evidence& e){return e.basis!=Basis::unknown&&!e.reference.empty()&&e.reference.size()<=4096;}
enum class ActivationKind {approach,entry};
struct Activation {
    ActivationKind kind=ActivationKind::approach;std::optional<Place> center;double radius=0;
    std::string entry_identity;Evidence evidence;
    bool operator==(const Activation&)const=default;
};
struct Site {unsigned id=0;Place destination;Evidence destination_evidence;std::optional<Activation> activation;bool operator==(const Site&)const=default;};
struct CatalogStamp {unsigned map=0,nav_asset=0;std::uint64_t revision=0;bool operator==(const CatalogStamp&)const=default;};
enum class Review {rejected,destination_only,enabled,unchanged};
// No bundled coordinates. The trusted content adapter supplies provenance and
// validates every supplied Place against the exact map/nav asset. A captured
// recovery destination alone never manufactures an activation center/radius.
class SiteCatalog {
    CatalogStamp stamp_;bool fixtures_=false;std::map<unsigned,Site> sites_;
    bool evidence_allowed(const Evidence& e,bool destination)const{
        if(!documented(e)||((e.basis==Basis::fixture)&&!fixtures_))return false;
        return !destination||e.basis==Basis::captured||e.basis==Basis::reviewed||(fixtures_&&e.basis==Basis::fixture);
    }
public:
    using Geometry=std::function<bool(Place)>;
    SiteCatalog(unsigned map,unsigned asset,bool allow_fixtures=false):stamp_{map,asset,1},fixtures_(allow_fixtures){}
    CatalogStamp stamp()const{return stamp_;}
    const Site* find(unsigned id)const{const auto it=sites_.find(id);return it==sites_.end()?nullptr:&it->second;}
    bool has_enabled()const{for(const auto& [id,s]:sites_)if(s.activation)return true;return false;}
    const std::map<unsigned,Site>& sites()const{return sites_;}
    Review review(Site site,const Geometry& geometry){
        if(!stamp_.map||!stamp_.nav_asset||!site.id||!valid(site.destination)||!geometry||
           !evidence_allowed(site.destination_evidence,true)||!geometry(site.destination))return Review::rejected;
        if(site.activation){const auto& a=*site.activation;
            if(!evidence_allowed(a.evidence,false))return Review::rejected;
            if(a.kind==ActivationKind::approach){
                if(!a.center||!valid(*a.center)||!geometry(*a.center)||!std::isfinite(a.radius)||a.radius<=0||!a.entry_identity.empty())return Review::rejected;
            }else if(a.kind==ActivationKind::entry){if(a.entry_identity.empty()||a.center||a.radius!=0)return Review::rejected;}
            else return Review::rejected;
        }
        if(const auto* old=find(site.id);old&&*old==site)return Review::unchanged;
        if(stamp_.revision==UINT64_MAX)return Review::rejected;
        const bool enabled=site.activation.has_value();sites_[site.id]=std::move(site);++stamp_.revision;
        return enabled?Review::enabled:Review::destination_only;
    }
};
struct ActiveSites {
    Party party;std::vector<Scope> members;CatalogStamp catalog;std::uint64_t revision=0;
    std::set<unsigned> sites;Time observed=0;
};
// Instance-owned activation observations. World must supply each real movement
// segment/corner's authoritative entry pose; this helper never invents a swept
// path, activation location, or party membership from a pet/NPC actor.
class SiteActivation {
    std::map<std::pair<std::uint64_t,unsigned>,ActiveSites> states_;std::uint64_t revision_=0;
public:
    bool observe(const SiteCatalog& catalog,const Snapshot& facts,Scope actor,Place pose,std::string_view entry={}){
        if(!valid(pose)||!facts.party.instance||!facts.party.id||!facts.party.epoch||facts.players.empty())return false;
        std::vector<Scope> members;std::set<unsigned> actors;bool living_member=false;
        for(const auto& p:facts.players){if(p.scope.instance!=facts.party.instance||!p.scope.actor||p.scope.actor>65535||!p.scope.generation||!actors.insert(p.scope.actor).second)return false;
            members.push_back(p.scope);if(p.scope==actor&&p.alive==std::optional(true))living_member=true;}
        if(!living_member)return false;std::sort(members.begin(),members.end());
        const auto key=std::pair{facts.party.instance,facts.party.id};auto it=states_.find(key);
        if(it!=states_.end()&&(facts.party.epoch<it->second.party.epoch||facts.at<it->second.observed||
           (catalog.stamp().map==it->second.catalog.map&&catalog.stamp().nav_asset==it->second.catalog.nav_asset&&catalog.stamp().revision<it->second.catalog.revision)))return false;
        if(it==states_.end()||facts.party!=it->second.party||catalog.stamp()!=it->second.catalog){
            if(revision_==UINT64_MAX)return false;
            it=states_.insert_or_assign(key,ActiveSites{facts.party,members,catalog.stamp(),++revision_,{},facts.at}).first;
        }else if(it->second.members!=members)return false; // Host must advance roster epoch.
        auto next=it->second.sites;
        for(const auto& [id,site]:catalog.sites())if(site.activation){const auto& a=*site.activation;
            const bool activated=a.kind==ActivationKind::entry?!entry.empty()&&entry==std::string_view(a.entry_identity):
                a.center&&pose.plane==a.center->plane&&std::hypot(pose.x-a.center->x,pose.y-a.center->y)<=a.radius;
            if(activated)next.insert(id);
        }
        if(next!=it->second.sites){if(revision_==UINT64_MAX)return false;it->second.sites=std::move(next);it->second.revision=++revision_;}
        it->second.observed=facts.at;return true;
    }
    std::optional<ActiveSites> snapshot(Party party)const{
        const auto it=states_.find({party.instance,party.id});return it!=states_.end()&&it->second.party==party?std::optional(it->second):std::nullopt;
    }
};
struct SelectionOrigin {Scope source;Place pose;Evidence policy;};
struct SiteRecoveryPlan {
    Attempt wipe;CatalogStamp catalog;std::uint64_t activation_revision=0,navigation_revision=0;
    unsigned site=0;Place destination;SelectionOrigin origin;
};
enum class SiteDecision {invalid,catalog_unavailable,activation_unreviewed,return_to_last_outpost_required,ready};
struct Selection {SiteDecision decision=SiteDecision::invalid;std::optional<SiteRecoveryPlan> plan;};
// A destination prerequisite ONLY, not authority to resurrect or claim a wipe.
// Selection origin/tie-breaking is explicit host policy, not recovered retail:
// choose nearest activated destination to supplied exact member pose; ties byID.
inline Selection prepare_site_recovery(const SiteCatalog& catalog,const ActiveSites& active,
        const Attempt& wipe,SelectionOrigin origin,std::uint64_t navigation_revision){
    if(!navigation_revision||!wipe.serial||wipe.party!=active.party||wipe.members!=active.members||
       active.catalog!=catalog.stamp()||!active.revision||!valid(origin.pose)||!documented(origin.policy)||
       std::find(wipe.members.begin(),wipe.members.end(),origin.source)==wipe.members.end())return {};
    if(catalog.sites().empty())return {SiteDecision::catalog_unavailable,{}};
    if(!catalog.has_enabled())return {SiteDecision::activation_unreviewed,{}};
    if(active.sites.empty())return {SiteDecision::return_to_last_outpost_required,{}};
    const Site* selected=nullptr;double distance=0;
    for(const auto id:active.sites){const auto* site=catalog.find(id);if(!site||!site->activation)return {};
        const double d=std::hypot(origin.pose.x-site->destination.x,origin.pose.y-site->destination.y);
        if(!std::isfinite(d))return {};
        if(!selected||d<distance||(d==distance&&id<selected->id)){selected=site;distance=d;}
    }
    if(!selected)return {};
    return {SiteDecision::ready,SiteRecoveryPlan{wipe,catalog.stamp(),active.revision,navigation_revision,selected->id,selected->destination,std::move(origin)}};
}
inline bool current_site_plan(const SiteRecoveryPlan& plan,const SiteCatalog& catalog,
        const ActiveSites& active,const Attempt& wipe,std::uint64_t navigation_revision){
    if(plan.wipe!=wipe||plan.catalog!=catalog.stamp()||active.catalog!=plan.catalog||active.party!=wipe.party||
       active.members!=wipe.members||plan.activation_revision!=active.revision||plan.navigation_revision!=navigation_revision||
       !active.sites.contains(plan.site))return false;
    const auto selected=prepare_site_recovery(catalog,active,wipe,plan.origin,navigation_revision);
    return selected.decision==SiteDecision::ready&&selected.plan->site==plan.site&&selected.plan->destination==plan.destination;
}
}
