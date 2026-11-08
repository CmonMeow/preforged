// Detached, unregistered: run only with reviewed candidate source applied.
#include "gw_pet_catalog.h"
#include "gw_npc_combat_catalog.h"
#include <fstream>
#include <stdexcept>
using namespace fw::gw;
static void check(bool value){if(!value)throw std::runtime_error("Stalker species flesh authority regression");}
int main(int argc,char** argv){
    check(argc==2);
    const auto traits=pets::reviewed_species_traits(pets::Species::melandrus_stalker);
    check(traits&&traits->fleshy&&std::string(traits->evidence).find("not capture")!=std::string::npos);
    check(!pets::reviewed_species_traits(pets::Species(0))&&!pets::reviewed_species_traits(pets::Species(2)));
    // Cosmetic names, level and evolution do not produce a different species fact.
    for(unsigned level:{5u,20u}){pets::Pet pet;pet.level=level;pet.name="Other Cat";
        check(pets::valid(pet)&&pets::reviewed_species_traits(pet.species)->fleshy);}
    std::ifstream input(argv[1]);check(bool(input));
    const auto catalog=combat::npcs::load_catalog(input,"b61455a1c495ed53ef1bfbaba92bf1addf3d151a297e9edf27b85441068fde76");
    const auto* wild=catalog.find(162,23000);check(wild&&wild->fleshy==std::optional(true));
    check(!wild->knockdown_allowed&& !wild->disease_immune&& !wild->energy_pips);
    combat::npcs::LiveActor original(*wild,{1,1,1,162,23000});
    auto preserved=original;const auto life=preserved.lifetime();
    check(preserved.fleshy(life)==std::optional(true)&&!preserved.knockdown_allowed(life));
    // Durable reconstruction uses the same provider, rather than name/model defaults.
    auto rebuilt=*wild;rebuilt.fleshy.reset();rebuilt.fleshy_evidence="-";
    rebuilt.fleshy=traits->fleshy;rebuilt.fleshy_evidence=traits->evidence;
    combat::npcs::LiveActor reentry(rebuilt,{1,1,2,162,23000});
    check(reentry.fleshy(reentry.lifetime())==std::optional(true));
    check(!reentry.fleshy(life)); // Wrong generation cannot borrow the species receipt.
}
