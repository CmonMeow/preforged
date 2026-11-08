#include "gw_pet_lifecycle.h"
#include "gw_pet_wire.h"
#include <array>
#include <iostream>
#include <source_location>
using namespace fw::gw::pets;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("pet lifecycle line "+std::to_string(at.line()));
}
int main()try{
    for(const Profile profile:{Profile{},Profile{Pet{}},Profile{Pet{Species::melandrus_stalker,20,1234,Evolution::elder,"Little Cat"}}}){
        const auto bytes=encode(profile);check(decode(bytes)==profile);
        for(std::size_t n=0;n<bytes.size();++n){bool rejected=false;try{decode(std::span(bytes).first(n));}catch(const std::invalid_argument&){rejected=true;}check(rejected);}
        auto extra=bytes;extra.push_back(0);bool rejected=false;try{decode(extra);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
    }
    check(!valid_name("A")&&!valid_name("Bad  Cat")&&!valid_name(" Cat")&&!valid_name("Bad1Cat")&&valid_name("Good Cat"));
    for(unsigned level=5;level<=20;++level){Pet p;p.level=level;check(base_stats(p)==std::optional(BaseStats{20*level+80,3*level+20}));}
    check(base_stats(Pet{})==std::optional(BaseStats{180,35}));Pet evolved;evolved.evolution=Evolution::dire;check(!base_stats(evolved));
    check(comfort(0)==std::optional(Comfort{0,20,10})&&comfort(12)==std::optional(Comfort{12,87,48})&&comfort(15)==std::optional(Comfort{15,104,58})&&!comfort(21));
    check(brings_pet(std::array{411u})&&brings_pet(std::array{436u})&&!brings_pet(std::array{789u}));
    check(charm_level_allowed(1,5)&&!charm_level_allowed(1,6));
    const Scope owner{1,2,3},animal{1,100,4};
    CharmFacts facts{owner,animal,1,5,true,true,true,true,false,true,true,true,false,100};
    const Profile empty;
    check(comfort_target(ComfortFacts{owner,animal,true,true,true,false,false,true,100},100)); // Own pet, range valid, different layer.

    {auto current=facts;current.animal_level=6;CharmLedger ledger;
        check(!ledger.reserve(current,{},0,100));current.owner_level=2;
        const auto leveled=ledger.reserve(current,{},0,100);check(bool(leveled));check(ledger.cancel(*leveled));}

    for(unsigned failure=0;failure<9;++failure){
        CharmLedger ledger;auto attempt=ledger.start(facts,empty,7,101,100);check(bool(attempt));
        check(!ledger.start(facts,empty,7,102,101));auto another=facts;another.owner.actor=3;check(!ledger.start(another,empty,7,102,101));
        unsigned commits=0;const AdoptionCommit write=[&](const AdoptionWrite& request){++commits;check(request.expected_revision==7&&!request.expected.pet&&request.replacement.pet==std::optional(Pet{}));return failure==7?CommitStatus::conflict:failure==8?CommitStatus::unavailable:CommitStatus::committed;};
        check(ledger.finish(*attempt,facts,true,true,write,10099)==CharmCompletion::deferred&&commits==0);
        auto current=facts;
        if(failure==1)current.target_alive=false;
        if(failure==2)++current.target.generation;
        if(failure==3)current.owner_alive=false;
        if(failure==4)current.owner_has_pet=true;
        const auto result=ledger.finish(*attempt,current,failure!=5,failure!=6,write,10100);
        check(result==(failure==0?CharmCompletion::adopted:failure<7?CharmCompletion::retired:failure==7?CharmCompletion::conflict:CharmCompletion::unavailable));
        check(commits==(failure==0||failure>=7?1u:0u));
        check(ledger.finish(*attempt,facts,true,true,write,10100)==CharmCompletion::invalid);
    }
    for(unsigned failure=0;failure<8;++failure){
        CharmLedger ledger;const auto reserved=ledger.reserve(facts,empty,7,100);check(bool(reserved));
        check(!ledger.start(facts,empty,7,11,100)&&!ledger.reserve(facts,empty,7,100));
        auto competing=facts;++competing.owner.actor;check(!ledger.reserve(competing,empty,7,100));
        auto wrong=*reserved;++wrong.serial;check(!ledger.cancel(wrong));
        auto current=facts;auto profile=empty;auto revision=7u;auto sequence=11u;
        if(failure==1)current.owner_alive=false;
        if(failure==2)current.target_alive=false;
        if(failure==3)++current.target.generation;
        if(failure==4)current.in_range=false;
        if(failure==5)++revision;
        if(failure==6)profile.pet=Pet{};
        if(failure==7)sequence=0;
        const auto active=ledger.accept(*reserved,current,profile,revision,sequence,101,10000);
        check(bool(active)==(failure==0));
        check(!ledger.accept(*reserved,facts,empty,7,12,102));check(!ledger.cancel(*reserved));
        if(active)check(ledger.cancel(*active));
        const auto retry=ledger.reserve(facts,empty,7,103);check(retry&&retry->serial!=reserved->serial);
        check(!ledger.cancel(*reserved));check(ledger.cancel(*retry));
    }
    {
        CharmLedger ledger;const auto reserved=*ledger.reserve(facts,empty,0,100);
        ledger.retire(owner);check(!ledger.cancel(reserved));
        auto next=facts;++next.owner.generation;
        const auto retry=ledger.reserve(next,empty,0,101);check(bool(retry));
        check(!ledger.cancel(reserved)&&ledger.cancel(*retry));
        const auto target_reserved=*ledger.reserve(facts,empty,0,102);ledger.retire(animal);
        check(!ledger.accept(target_reserved,facts,empty,0,1,103));
    }
    CharmLedger cancelled;const auto attempt=*cancelled.start(facts,empty,0,1,100);check(cancelled.cancel(attempt));
    check(cancelled.finish(attempt,facts,true,true,[](const auto&){throw std::runtime_error("cancelled CAS");return CommitStatus::committed;},10100)==CharmCompletion::invalid);
    const auto add=wire::add(181,15,u"\x108\x10a\xf0d\xa74a\xbbf9\x249a\x1",128406,129924,wire::Behavior::guard);
    const std::string expected="b100b50000000f000000070008010a010d0f4aa7f9bb9a24010096f5010084fb010001000000";
    std::string actual;const char digits[]="0123456789abcdef";for(auto b:add){actual+=digits[b>>4];actual+=digits[b&15];}check(actual==expected);
    check(wire::behavior(181,wire::Behavior::avoid_combat)==wire::Packet({97,0,181,0,0,0,2,0,0,0}));
    check(wire::locked_target(181,43)==wire::Packet({98,0,181,0,0,0,43,0,0,0}));
    check(wire::locked_target(181,0)==wire::Packet({98,0,181,0,0,0,0,0,0,0}));
    for(unsigned opcode:{19u,20u})for(unsigned value:{0u,1u,2u}){
        std::array<std::uint8_t,10> bytes{std::uint8_t(opcode),0,181,0,0,0,std::uint8_t(value),0,0,0};
        const auto command=wire::command(bytes);check(command&&command->pet==181&&command->value==value);
        check(!wire::command(std::span(bytes).first(9)));
    }
    std::cout<<"Pet durable schema, exact adoption CAS receipts and native wires passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
