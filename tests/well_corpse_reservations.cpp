#include "gw_corpse_reservations.h"
#include <stdexcept>
using fw::gw::quests::CorpseReservations;
template<class T> void check(const T& value){if(!static_cast<bool>(value))throw std::runtime_error("Well test failed");}
int main(){
    using Scope=CorpseReservations::Scope;
    const Scope corpse{1,70,4},owner{1,25,2},quest_owner{1,40,3};
    CorpseReservations ledger;
    check(ledger.available(corpse));
    check(!ledger.reserve_well(corpse,owner,92,0));
    check(!ledger.reserve_well(corpse,owner,90,1));
    check(!ledger.reserve_well(corpse,owner,95,1));
    check(ledger.reserve_well(corpse,owner,92,7));
    check(!ledger.available(corpse));
    check(!ledger.reserve(corpse,{quest_owner,82,1}));
    check(!ledger.reserve_well(corpse,owner,92,8));
    check(!ledger.reserve_well(corpse,owner,93,7));
    check(ledger.owns_well(corpse,owner,92,7));
    check(!ledger.consume_well(corpse,owner,92,8));
    check(ledger.consume_well(corpse,owner,92,7));
    check(!ledger.consume_well(corpse,owner,92,7));
    check(!ledger.available(corpse));
    check(!ledger.reserve(corpse,{quest_owner,82,1}));
    check(!ledger.reserve_well(corpse,owner,92,9));
    auto recycled=corpse;++recycled.generation;
    check(ledger.available(recycled));
    check(ledger.reserve(recycled,{quest_owner,82,1}));
    check(!ledger.reserve_well(recycled,owner,91,10));
    check(ledger.release(recycled,{quest_owner,82,1}));
    check(ledger.reserve_well(recycled,owner,91,10));
    check(ledger.release(recycled,{owner,0,0,91,10}));
    check(ledger.available(recycled));
    ledger.retire(corpse);check(ledger.available(corpse));
}
