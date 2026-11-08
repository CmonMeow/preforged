#include "gw_npc_combat_catalog.h"
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace fw::gw::combat::npcs;
static void check(bool yes){if(!yes)throw std::runtime_error("Lightbringer taxonomy test failed");}
static bool rejects(const std::function<void()>& action){
    try{action();}catch(const std::invalid_argument&){return true;}
    return false;
}
int main()try{
    Definition definition;
    definition.binding.map=474;
    definition.binding.actor=definition.binding.definition=100;
    definition.binding.level=20;
    definition.relation=Relation::hostile_to_players;
    definition.spawn_health={100,100};
    definition.damageable=true;
    definition.servant_of_abaddon=true;
    check(rejects([&]{LiveActor unreviewed(definition,{1,1,1,474,100});}));
    definition.servant_of_abaddon_evidence="synthetic-review";
    LiveActor servant(definition,{1,1,1,474,100});
    check(servant.servant_of_abaddon(servant.lifetime())==true);
    check(!servant.servant_of_abaddon({1,1,2,474,100}));
    definition.servant_of_abaddon=false;
    LiveActor nonservant(definition,{1,1,1,474,100});
    check(nonservant.servant_of_abaddon(nonservant.lifetime())==false);
    definition.servant_of_abaddon.reset();
    definition.servant_of_abaddon_evidence="-";
    LiveActor unknown(definition,{1,1,1,474,100});
    check(!unknown.servant_of_abaddon(unknown.lifetime()));

    const std::string digest(64,'a');
    std::vector<std::string> row={"npc","474","100","100","1234","0","0","8","20",
        "hostile-to-players","-","100","100","-","1","0","relation","health","-","-","-"};
    row.resize(54,"-");row[27]="1";row[52]="1";row[53]="taxonomy";
    const auto catalog_text=[&](const std::vector<std::string>& fields){
        std::ostringstream out;
        out<<"gw-npc-combat 16 "<<digest<<'\n';
        out<<"source relation authored "<<digest<<" tests/synthetic-relation\n";
        out<<"source health authored "<<digest<<" tests/synthetic-health\n";
        out<<"source taxonomy authored "<<digest<<" tests/synthetic-taxonomy\n";
        for(const auto& field:fields)out<<std::quoted(field)<<' ';
        out<<'\n';return out.str();
    };
    auto encoded=catalog_text(row);std::istringstream input(encoded);
    const auto catalog=load_catalog(input,digest);
    const auto* parsed=catalog.find(474,100);
    check(parsed&&parsed->servant_of_abaddon==true&&
          parsed->servant_of_abaddon_evidence=="taxonomy");
    row[53]="-";encoded=catalog_text(row);
    check(rejects([&]{std::istringstream invalid(encoded);load_catalog(invalid,digest);}));
    row[52]="-";row[53]="taxonomy";encoded=catalog_text(row);
    check(rejects([&]{std::istringstream invalid(encoded);load_catalog(invalid,digest);}));
    std::cout<<"PASS evidence-backed demonic-servant trait, lifetime and catalog v16\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
