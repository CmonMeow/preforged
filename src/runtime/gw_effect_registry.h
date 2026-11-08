#pragma once
#include "gw_skill_catalog_generated.h"
#include <map>
#include <optional>
#include <vector>
#include <cstdint>
#include <compare>
#include <cstddef>
#include <set>
#include <utility>

namespace fw::gw::skills::effects {
using Time=std::uint64_t;
struct Scope {std::uint64_t instance=0;std::uint32_t actor=0;std::uint64_t generation=0;auto operator<=>(const Scope&)const=default;};
enum class Category {enchantment,hex,stance,weapon_spell,preparation,glyph,chant,echo,form,other};
inline Category category(const SkillRecord& row){switch(row.type){
case 6:return Category::enchantment;case 4:return Category::hex;case 3:return Category::stance;
case 25:return Category::weapon_spell;case 19:return Category::preparation;case 12:return Category::glyph;
case 27:return Category::chant;case 28:return Category::echo;case 26:return Category::form;
default:return Category::other;
}}
struct Entry {std::uint32_t instance=0,cause=0;unsigned skill=0,rank=0;Category kind=Category::other;Time applied=0;std::optional<Time> expires;};
enum class Replace {independent,same_skill,category};
struct Applied {bool accepted=false;std::vector<Entry> removed;};
// Transaction-local application observations. A copied registry carries these
// records; discarding a speculative copy discards its events as well. Consumers
// drain only committed registries and retain the sequence per target lifetime.
struct ApplicationEvent {Scope target;std::uint64_t sequence;Entry applied;std::vector<Entry> observers;};
// Authoritative status storage only: applying a record is NOT executing its
// mechanics or emitting65. The reviewed skill handler chooses replacement and
// natural-expiry vs removal behavior; no generic skill success is invented.
class Registry {
    Scope owner_;
    std::map<std::uint32_t,Entry> entries_;
    std::map<std::uint32_t,std::uint64_t> order_;
    std::uint64_t next_order_=1;
    std::set<unsigned> application_observers_;
    std::vector<ApplicationEvent> applications_;
    bool owns(Scope caller)const{return caller==owner_&&owner_.instance&&owner_.actor&&owner_.generation;}
public:
    explicit Registry(Scope owner):owner_(owner){}
    void swap(Registry&other)noexcept{
        static_assert(noexcept(entries_.swap(other.entries_))&&noexcept(order_.swap(other.order_))&&noexcept(application_observers_.swap(other.application_observers_))&&noexcept(applications_.swap(other.applications_)));
        using std::swap;swap(owner_,other.owner_);swap(next_order_,other.next_order_);
        entries_.swap(other.entries_);order_.swap(other.order_);application_observers_.swap(other.application_observers_);applications_.swap(other.applications_);
    }
    friend void swap(Registry&a,Registry&b)noexcept{a.swap(b);}
    bool matches(Scope caller)const{return owns(caller);}
    bool observe_applications(Scope caller,unsigned skill){
        if(!owns(caller)||!skill||!find_skill(skill))return false;
        if(application_observers_.contains(skill))return true;
        if(application_observers_.size()>=16)return false;
        application_observers_.insert(skill);return true;
    }
    std::vector<ApplicationEvent> take_applications(Scope caller){
        if(!owns(caller))return {};auto result=std::move(applications_);applications_.clear();return result;
    }
    Applied apply(Scope caller,Entry entry,Replace replace){
        const auto* row=find_skill(entry.skill);
        if(!owns(caller)||next_order_==UINT64_MAX||!entry.instance||!entry.cause||!entry.skill||!row||row->type==8||entry.rank>20||
           entry.kind!=category(*row)||(entry.expires&&*entry.expires<=entry.applied)||
           (replace!=Replace::independent&&replace!=Replace::same_skill&&replace!=Replace::category))return {};
        Applied result;
        for(const auto& [id,old]:entries_)if(id==entry.instance||
            (replace==Replace::same_skill&&old.skill==entry.skill)||
            (replace==Replace::category&&old.kind==entry.kind))result.removed.push_back(old);
        // Defensive memory cap; not a claimed retail effect-slot limit.
        if(entries_.size()-result.removed.size()>=128)return {};
        ApplicationEvent event{caller,next_order_,entry,{}};
        if(entry.kind==Category::hex||entry.kind==Category::enchantment)
            for(auto skill:application_observers_)if(const auto observer=newest_skill(caller,skill,entry.applied))event.observers.push_back(*observer);
        // Defensive queue cap, not a retail effect limit. Never mutate a
        // registry when its application consumer has stopped draining events.
        if(!event.observers.empty()&&applications_.size()>=128)return {};
        for(const auto& old:result.removed){entries_.erase(old.instance);order_.erase(old.instance);}
        entries_[entry.instance]=entry;order_[entry.instance]=next_order_++;
        if(!event.observers.empty())applications_.push_back(std::move(event));
        result.accepted=true;return result;
    }
    std::optional<Entry> remove(Scope caller,std::uint32_t instance){
        if(!owns(caller))return {};const auto it=entries_.find(instance);if(it==entries_.end())return {};
        auto old=it->second;entries_.erase(it);order_.erase(instance);return old;
    }
    std::vector<Entry> expire(Scope caller,Time now){
        std::vector<Entry> removed;if(!owns(caller))return removed;
        for(auto it=entries_.begin();it!=entries_.end();)if(it->second.expires&&now>=*it->second.expires){removed.push_back(it->second);order_.erase(it->first);it=entries_.erase(it);}else ++it;
        return removed;
    }
    std::vector<Entry> active(Scope caller,Time now)const{
        std::vector<Entry> result;if(!owns(caller))return result;
        for(const auto& [id,e]:entries_)if(now>=e.applied&&(!e.expires||now<*e.expires))result.push_back(e);return result;
    }
    unsigned count(Scope caller,Category kind,Time now)const{
        if(!owns(caller))return 0;unsigned result=0;for(const auto& [id,e]:entries_)if(e.kind==kind&&now>=e.applied&&(!e.expires||now<*e.expires))++result;return result;
    }
    bool has(Scope caller,Category kind,Time now)const{
        if(!owns(caller))return false;for(const auto& [id,e]:entries_)if(e.kind==kind&&now>=e.applied&&(!e.expires||now<*e.expires))return true;return false;
    }
    std::optional<Entry> newest(Scope caller,Category kind,Time now,unsigned profession=0)const{
        if(!owns(caller))return {};std::optional<Entry> selected;std::uint64_t order=0;
        for(const auto& [id,e]:entries_)if(e.kind==kind&&(!profession||find_skill(e.skill)->profession==profession)&&now>=e.applied&&(!e.expires||now<*e.expires)&&order_.at(id)>order){selected=e;order=order_.at(id);}
        return selected;
    }
    std::optional<Entry> lookup(Scope caller,std::uint32_t effect)const{
        if(!owns(caller))return {};const auto it=entries_.find(effect);
        return it==entries_.end()?std::nullopt:std::optional(it->second);
    }
    // Transaction token for this exact application,not merely its reusable
    // wire effect ID. Replacement with identical fields still invalidates it.
    std::optional<std::uint64_t> entry_sequence(Scope caller,std::uint32_t effect)const{
        if(!owns(caller))return {};const auto found=order_.find(effect);
        return found==order_.end()?std::nullopt:std::optional(found->second);
    }
    std::optional<Entry> newest_skill(Scope caller,unsigned skill,Time now,unsigned alternate=0)const{
        if(!owns(caller))return {};std::optional<Entry> selected;std::uint64_t order=0;
        for(const auto& [id,e]:entries_)if((e.skill==skill||(alternate&&e.skill==alternate))&&now>=e.applied&&(!e.expires||now<*e.expires)&&order_.at(id)>order){selected=e;order=order_.at(id);}
        return selected;
    }
    std::uint32_t presentation(Scope caller,std::uint32_t other,Time now)const{
        if(!owns(caller))return other;auto result=other&~std::uint32_t(0x8880);
        if(has(caller,Category::enchantment,now))result|=0x80;
        if(has(caller,Category::hex,now))result|=0x800;
        if(has(caller,Category::weapon_spell,now))result|=0x8000;
        return result;
    }
    std::vector<Entry> clear(Scope caller){
        if(!owns(caller))return {};std::vector<Entry> removed;
        for(const auto& [id,e]:entries_)removed.push_back(e);
        entries_.clear();order_.clear();applications_.clear();return removed;
    }
    std::size_t size()const{return entries_.size();}
};
}
