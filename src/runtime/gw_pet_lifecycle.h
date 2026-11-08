#pragma once
#include "gw_pet_catalog.h"
#include "gw_effect_registry.h"
#include <functional>
#include <map>
#include <limits>

namespace fw::gw::pets {
using Scope=skills::effects::Scope;
inline bool valid_scope(Scope value){return value.instance&&value.actor&&value.actor<=65535&&value.generation;}
// All facts come from the current locked World, never from client parameters.
struct CharmFacts {
    Scope owner,target;
    unsigned owner_level=0,animal_level=0;
    bool owner_alive=false,publication_ready=false,target_alive=false;
    bool reviewed_charmable=false,already_owned=false,in_range=false,same_plane=false,line_of_sight=false;
    bool owner_has_pet=false;
    std::uint64_t clock=0;
};
inline bool charmable(const CharmFacts& facts,std::uint64_t now){
    return valid_scope(facts.owner)&&valid_scope(facts.target)&&facts.owner!=facts.target&&
        facts.owner.instance==facts.target.instance&&facts.owner_alive&&facts.publication_ready&&
        facts.target_alive&&facts.reviewed_charmable&&!facts.already_owned&&!facts.owner_has_pet&&
        facts.in_range&&facts.same_plane&&facts.line_of_sight&&facts.clock<=now&&
        charm_level_allowed(facts.owner_level,facts.animal_level);
}
struct AdoptionWrite {
    std::uint64_t expected_revision=0;
    Profile expected,replacement;
    bool operator==(const AdoptionWrite&)const=default;
};
// The trusted storage adapter performs authenticated exact-revision CAS of the
// complete expected profile. Success means durable commit, not a staged write.
// Existing-pet replacement/release is a separate transaction, never implicit.
enum class CommitStatus {committed,conflict,unavailable};
using AdoptionCommit=std::function<CommitStatus(const AdoptionWrite&)>;
struct CharmAttempt {
    std::uint64_t serial=0,cast_sequence=0,starts=0,completes=0;
    Scope owner,target;
    AdoptionWrite write;
    bool operator==(const CharmAttempt&)const=default;
};
// Pre-payment reservation: no cast clock, hostility or action mutation yet.
struct CharmReservation {
    std::uint64_t serial=0;Scope owner,target;
    bool operator==(const CharmReservation&)const=default;
};
enum class CharmCompletion {invalid,deferred,retired,conflict,unavailable,adopted};
// Exact admission/action receipts; no server instance/actor identity persisted.
// World must prepare its complete conversion/publication before calling finish,
// then apply the prepared no-fail World swap immediately after adopted. The
// storage callback must not reenter World while its instance lock is held.
class CharmLedger {
    std::map<Scope,CharmAttempt> attempts_;
    struct Pending {CharmReservation receipt;Profile durable;std::uint64_t revision=0,created=0;};
    std::map<Scope,Pending> pending_;
    std::uint64_t serial_=0,reservation_serial_=0;
public:
    std::optional<CharmReservation> reserve(const CharmFacts& facts,const Profile& durable,
            std::uint64_t revision,std::uint64_t now){
        if(!charmable(facts,now)||durable.pet||revision>=INT64_MAX||reservation_serial_==UINT64_MAX||
           attempts_.contains(facts.owner)||pending_.contains(facts.owner))return {};
        Pet pet;pet.level=facts.animal_level;if(!valid(pet)||!base_stats(pet))return {};
        for(const auto& [owner,attempt]:attempts_)if(attempt.target==facts.target)return {};
        for(const auto& [owner,pending]:pending_)if(pending.receipt.target==facts.target)return {};
        CharmReservation receipt{++reservation_serial_,facts.owner,facts.target};
        pending_.emplace(facts.owner,Pending{receipt,durable,revision,now});return receipt;
    }
    bool cancel(const CharmReservation& receipt){
        const auto found=pending_.find(receipt.owner);
        if(found==pending_.end()||found->second.receipt!=receipt)return false;
        pending_.erase(found);return true;
    }
    std::optional<CharmAttempt> accept(const CharmReservation& receipt,const CharmFacts& facts,
            const Profile& durable,std::uint64_t revision,std::uint64_t cast_sequence,
            std::uint64_t now,std::uint64_t duration=10000){
        const auto found=pending_.find(receipt.owner);
        if(found==pending_.end()||found->second.receipt!=receipt)return {};
        const auto prepared=found->second;pending_.erase(found); // Failure cannot replay payment.
        if(now<prepared.created||facts.owner!=receipt.owner||facts.target!=receipt.target||
           durable!=prepared.durable||revision!=prepared.revision)return {};
        return start(facts,durable,revision,cast_sequence,now,duration);
    }
    std::optional<CharmAttempt> start(const CharmFacts& facts,const Profile& durable,
            std::uint64_t revision,std::uint64_t cast_sequence,std::uint64_t now,std::uint64_t duration=10000){
        if(!charmable(facts,now)||durable.pet||revision>=INT64_MAX||!cast_sequence||!duration||
           duration>60000||now>UINT64_MAX-duration||serial_==UINT64_MAX||attempts_.contains(facts.owner))return {};
        if(pending_.contains(facts.owner))return {};
        for(const auto& [owner,pending]:pending_)if(pending.receipt.target==facts.target)return {};
        for(const auto& [owner,attempt]:attempts_)if(attempt.target==facts.target)return {};
        Pet pet;pet.level=facts.animal_level;
        if(!valid(pet)||!base_stats(pet))return {};
        CharmAttempt attempt{++serial_,cast_sequence,now,now+duration,facts.owner,facts.target,{revision,durable,Profile{pet}}};
        attempts_.emplace(facts.owner,attempt);return attempt;
    }
    bool cancel(const CharmAttempt& receipt){
        const auto found=attempts_.find(receipt.owner);
        if(found==attempts_.end()||found->second!=receipt)return false;
        attempts_.erase(found);return true;
    }
    void retire(Scope scope){
        std::erase_if(attempts_,[&](const auto& pair){return pair.first==scope||pair.second.target==scope;});
        std::erase_if(pending_,[&](const auto& pair){return pair.first==scope||pair.second.receipt.target==scope;});
    }
    std::optional<CharmAttempt> current(Scope owner)const{
        const auto found=attempts_.find(owner);return found==attempts_.end()?std::nullopt:std::optional(found->second);
    }
    CharmCompletion finish(const CharmAttempt& receipt,const CharmFacts& facts,
            bool action_current,bool world_prepared,const AdoptionCommit& commit,std::uint64_t now){
        const auto found=attempts_.find(receipt.owner);
        if(found==attempts_.end()||found->second!=receipt||now<receipt.starts)return CharmCompletion::invalid;
        if(now<receipt.completes||facts.clock>now)return CharmCompletion::deferred;
        // A completed action is consumed even when admission changed. A fresh
        // paid cast is required after conflict, cancellation or recipient death.
        attempts_.erase(found);
        if(!action_current||!world_prepared||!commit||facts.owner!=receipt.owner||facts.target!=receipt.target||
           !charmable(facts,now)||facts.animal_level!=receipt.write.replacement.pet->level)return CharmCompletion::retired;
        switch(commit(receipt.write)){
        case CommitStatus::committed:return CharmCompletion::adopted;
        case CommitStatus::conflict:return CharmCompletion::conflict;
        default:return CharmCompletion::unavailable;
        }
    }
};
struct ComfortFacts {
    Scope owner,pet;
    bool owner_alive=false,publication_ready=false,current_owned_pet=false,pet_alive=false;
    bool same_plane=false,in_range=false;
    std::uint64_t clock=0;
};
inline bool comfort_target(const ComfortFacts& facts,std::uint64_t now){
    return valid_scope(facts.owner)&&valid_scope(facts.pet)&&facts.owner!=facts.pet&&facts.owner.instance==facts.pet.instance&&
        facts.owner_alive&&facts.publication_ready&&facts.current_owned_pet&&facts.in_range&&facts.clock<=now;
}
}
