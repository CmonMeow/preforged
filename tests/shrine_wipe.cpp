#include <string>
#include <stdexcept>
#include "gw_shrine_wipe.h"
#include <iostream>
#include <stdexcept>
#include <source_location>
using namespace fw::gw::shrine;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("shrine wipe scheduler line "+std::to_string(at.line()));
}
int main()try{
    const Scope a{1,1,4},b{1,2,7};const Party party{1,1,3};
    Snapshot facts{party,{{a,false},{b,false}},1000,2};WipeScheduler scheduler;
    const auto first=scheduler.observe(facts);check(first.code==Code::armed&&first.attempt->due==11000);
    check(scheduler.observe(facts).attempt==first.attempt); // Duplicate same-time observation.
    facts.living_pets=99;facts.at=10999;check(scheduler.observe(facts).code==Code::waiting);
    check(!scheduler.claim(*first.attempt,facts));facts.at=11000;
    check(scheduler.observe(facts).code==Code::ready);check(scheduler.claim(*first.attempt,facts));
    check(!scheduler.claim(*first.attempt,facts)&&scheduler.observe(facts).code==Code::already_claimed);
    facts.at=12000;check(scheduler.observe(facts).code==Code::already_claimed);
    facts.players[0].alive=true;check(scheduler.observe(facts).code==Code::alive);
    facts.players[0].alive=false;facts.at=12500;const auto second=scheduler.observe(facts);
    check(second.code==Code::armed&&second.attempt->serial!=first.attempt->serial&&second.attempt->due==22500);
    facts.at=22499;facts.players[1].alive=true;check(scheduler.observe(facts).code==Code::alive&&!scheduler.pending(party));
    facts.players[1].alive=false;facts.at=22500;const auto third=scheduler.observe(facts);check(third.attempt->due==32500);
    check(!scheduler.claim(*second.attempt,facts));
    auto historical=facts;historical.at=22000;check(scheduler.observe(historical).code==Code::invalid&&scheduler.pending(party)==third.attempt);
    // Unknown player liveness is never guessed dead, and a valid party epoch is
    // not replaced by pet presence or by snapshots from another party.
    facts.players[0].alive.reset();facts.at=23000;check(scheduler.observe(facts).code==Code::unknown_liveness&&!scheduler.pending(party));
    facts.players[0].alive=false;facts.at=24000;const auto fourth=scheduler.observe(facts);check(fourth.attempt->due==34000);
    auto unrelated=facts;unrelated.party.id=8;check(scheduler.observe(unrelated).code==Code::armed&&scheduler.pending(party)==fourth.attempt);
    // Mere ordering is not roster mutation. Reused actor generation IS mutation.
    std::swap(facts.players[0],facts.players[1]);facts.at=25000;check(scheduler.observe(facts).attempt==fourth.attempt);
    ++facts.players[0].scope.generation;facts.at=26000;check(scheduler.observe(facts).code==Code::roster_changed&&!scheduler.pending(party));
    check(!scheduler.claim(*fourth.attempt,facts));
    check(scheduler.observe(facts).code==Code::invalid);
    check(!scheduler.pending(party));
    ++facts.party.epoch;facts.at=27000;const auto fresh=scheduler.observe(facts);check(fresh.code==Code::armed&&fresh.attempt->due==37000);
    auto old=facts;old.party=party;old.at=28000;check(scheduler.observe(old).code==Code::invalid&&scheduler.pending(facts.party)==fresh.attempt);
    check(!scheduler.retire(party)&&scheduler.retire(facts.party)&&!scheduler.pending(facts.party));
    ++facts.party.epoch;facts.at=29000;check(scheduler.observe(facts).code==Code::armed);
    {WipeScheduler empty;auto roster=facts;const auto pending=empty.observe(roster);check(pending.code==Code::armed);
        roster.players.clear();++roster.at;check(empty.observe(roster).code==Code::roster_changed&&!empty.pending(roster.party));
        check(!empty.claim(*pending.attempt,roster));}
    WipeScheduler invalid;auto bad=facts;bad.players[1].scope=bad.players[0].scope;check(invalid.observe(bad).code==Code::invalid);
    bad=facts;bad.players[0].scope.instance=2;check(invalid.observe(bad).code==Code::invalid);
    bad=facts;bad.party.epoch=0;check(invalid.observe(bad).code==Code::invalid);
    bad=facts;bad.at=UINT64_MAX-9999;check(invalid.observe(bad).code==Code::invalid);
    std::cout<<"Typed party wipe10sec scheduling, pet exclusion, revival/roster cancellation, clocks and replay passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
