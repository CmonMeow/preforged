#include "gw_energy_regeneration.h"
#include "gw_basic_armor_energy.h"
#include <iostream>

int main()try{
    using namespace fw::gw;auto check=[](bool b,const char* message){if(!b)throw std::runtime_error(message);};
    for(unsigned primary=1;primary<=10;++primary)check(energy::innate_pips(primary)==2,"all professions innate2pips");
    check(energy::basic_armor(1).pips==0&&energy::basic_armor(2).pips==1&&energy::basic_armor(9).pips==0,"martial armor pip table");
    check(energy::basic_armor(8).capacity==10&&energy::regeneration_pips(8,energy::basic_armor(8).pips)==4,"explicit caster armor adds2pips and10capacity");
    check(energy::regeneration_pips(8)==2,"armor never implicitly inferred from profession");
    for(unsigned primary=1;primary<=10;++primary){
        std::array<CharacterProfile::EquipmentAppearance,5> visual{};for(auto& piece:visual)piece.composite=1;
        auto owned=energy::private_starter_armor(primary,visual,u"Private Test");
        auto restored=inventory::decode(inventory::encode(owned));
        for(const auto&[id,item]:restored.items){
            auto definition=item.definition;definition.modifiers.push_back(0xa3c80000u|(unsigned(item.reviewed_armor->base)<<8));
            const auto bonus=energy::armor_piece(primary,item.slot);
            if(bonus.capacity)definition.modifiers.push_back(0x62c80000u|(bonus.capacity<<8));
            if(bonus.pips)definition.modifiers.push_back(0x62e80000u|(bonus.pips<<8));
            check(inventory::item_packets(item).front()==items::definition_2022(definition),"reviewed armor tooltip matches base and energy modifiers");
            auto carried=item;carried.bag=1;carried.slot=0;
            check(inventory::item_packets(carried).front()==inventory::item_packets(item).front(),"armor tooltip independent of equipped placement");
            auto captured=item;captured.definition.modifiers={0xa3c80500u};
            check(inventory::item_packets(captured).front()==items::definition_2022(captured.definition),"captured armor modifiers must not be replaced");
        }

        check(restored.items.at(1).reviewed_armor==owned.items.at(1).reviewed_armor,"reviewed private armor metadata persists independently of wiremodifiers");
        check(energy::fully_modeled_earth_armor(restored,primary)==energy::private_maximum_armor(primary).base+energy::private_maximum_armor(primary).elemental,"all modeled private pieces establish earth armor");
        auto explicit_ratings=restored;
        for(auto& [id,piece]:explicit_ratings.items)piece.definition.modifiers={0xa3c81900};
        check(energy::fully_modeled_physical_armor(explicit_ratings,primary)==25+energy::private_maximum_armor(primary).physical,"native explicit base overrides stored baseline in physical damage calculation");
        check(energy::fully_modeled_elemental_armor(explicit_ratings,primary)==25+energy::private_maximum_armor(primary).elemental,"native explicit base overrides stored baseline in elemental damage calculation");
        auto invalid_ratings=explicit_ratings;invalid_ratings.items.at(1).definition.modifiers.push_back(0xa3c83c00);
        check(!energy::fully_modeled_elemental_armor(invalid_ratings,primary),"ambiguous native base cannot fall back to a different private baseline");
        invalid_ratings=explicit_ratings;invalid_ratings.items.at(1).definition.type=7;
        check(!energy::fully_modeled_physical_armor(invalid_ratings,primary),"chest definition in head slot cannot establish body armor");
        invalid_ratings=explicit_ratings;invalid_ratings.items.emplace(6,invalid_ratings.items.at(1));invalid_ratings.items.at(6).definition.id=6;
        check(!energy::fully_modeled_physical_armor(invalid_ratings,primary),"duplicate physical armor slot cannot silently overwrite rating");
        auto sum=energy::owned_basic_armor(owned,primary),expected=energy::basic_armor(primary);
        check(sum.capacity==expected.capacity&&sum.pips==expected.pips,"per-piece armor sum");
        owned.items.erase(4);auto missing=energy::owned_basic_armor(owned,primary);
        check(!energy::fully_modeled_earth_armor(owned,primary),"partial armor is unknown until weighted model exists");
        check(missing.pips==sum.pips-(primary==1||primary==9?0:1),"removed legs lose their recovery");
    }
    check(energy::owned_basic_armor(inventory::baseline(),6).pips==0,"empty inventory has no armor recovery");
    {
        const std::array<int,5> locations{10,20,30,40,50};
        const std::array<int,8> expected{10,10,10,20,20,30,40,50};
        for(unsigned bucket=0;bucket<8;++bucket)check(energy::physical_armor_hit(locations,bucket)==expected[bucket],"wiki hit-location weight changed");
        check(!energy::physical_armor_hit(locations,8),"invalid armor roll admitted");
        std::array<CharacterProfile::EquipmentAppearance,5> visual{};for(auto& piece:visual)piece.composite=1;
        auto starter=energy::creation_starter_armor(6,1,visual,u"No Headgear");
        std::erase_if(starter.items,[](const auto& pair){return pair.second.slot==4;});
        check(!energy::fully_modeled_physical_armor(starter,6),"partial armor became a uniform scalar");
        check(energy::physical_armor_locations(starter,6)==std::array<int,5>{5,5,0,5,5},"missing headgear did not become unarmored head only");
        starter.items.begin()->second.reviewed_armor.reset();
        check(!energy::physical_armor_locations(starter,6),"unknown equipped armor treated as naked");
        check(energy::physical_armor_locations(inventory::baseline(),6)==std::array<int,5>{},"naked body rejected");
    }
    for(unsigned campaign=1;campaign<=3;++campaign)for(unsigned primary=1;primary<=10;++primary){
        std::array<CharacterProfile::EquipmentAppearance,5> visual{};for(auto& piece:visual)piece.composite=1;
        const auto starter=energy::creation_starter_armor(primary,campaign,visual,u"Starter Test");
        const auto restored=inventory::decode(inventory::encode(starter));
        constexpr std::array<unsigned,11> minimum{0,25,10,5,5,5,5,10,5,20,10};
        const auto physical=primary==1?(campaign==1?20:10):0;
        check(energy::fully_modeled_physical_armor(restored,primary)==minimum[primary]+physical,"campaign starter physical armor persists");
        check(energy::fully_modeled_elemental_armor(restored,primary)==minimum[primary]+(primary==2?30:0),"starter elemental armor persists");
        const auto bonus=energy::owned_basic_armor(restored,primary),expected=energy::basic_armor(primary);
        check(bonus.capacity==expected.capacity&&bonus.pips==expected.pips,"lower starter armor preserves inherent energy bonuses");
        check(energy::owned_basic_health(restored,primary)==(primary==10?25:0),"only reviewed Dervish chest grants inherent health");
        if(primary==10){
            auto unequipped=restored;auto& chest=unequipped.items.at(2);chest.bag=1;chest.slot=0;
            check(energy::owned_basic_health(unequipped,10)==0,"backpack chest grants no health");
            chest=restored.items.at(2);chest.reviewed_armor.reset();
            check(energy::owned_basic_health(unequipped,10)==0,"appearance-only captured chest grants no inferred health");
            chest=restored.items.at(2);chest.profession=6;
            check(energy::owned_basic_health(unequipped,10)==0,"foreign profession chest grants no health");
        }
    }
    check(energy::maximum_energy(6,12,20)==56&&energy::maximum_energy(6,16,30)==78,"EnergyStorage adds3pereffectiveprimaryrank");
    check(energy::maximum_energy(8,12,20)==20,"secondary/nonElementalist getsnoEnergyStoragegrant");
    {
        vitals::Resources capacity{100,100,10,20};std::uint32_t debt=0;
        energy::resize(capacity,56,debt);
        check(capacity.energy==46&&capacity.max_energy==56&&!debt,"capacity increase preserves missing ten energy");
        capacity.energy=5;energy::resize(capacity,20,debt);
        check(capacity.energy==0&&debt==31,"capacity decrease retains negative energy as UI-clamped debt");
        energy::Regeneration repayment;repayment.advance(capacity,30000,2,debt);
        check(capacity.energy==0&&debt==11,"regeneration first repays negative energy");
        repayment.advance(capacity,18000,2,debt);
        check(capacity.energy==1&&!debt,"remaining regeneration becomes spendable after debt cleared");
        capacity.energy=0;debt=31;energy::resize(capacity,56,debt);
        check(capacity.energy==5&&!debt,"restoring capacity reverses the exact deficit without refill exploit");
        energy::resize(capacity,56,debt);check(capacity.energy==5,"same capacity never refills the actor");
        debt=1;bool invalid=false;try{energy::resize(capacity,20,debt);}catch(const std::invalid_argument&){invalid=true;}
        check(invalid&&capacity.energy==5,"inconsistent debt state is rejected without mutation");
    }
    energy::Regeneration small,large;vitals::Resources a{100,100,0,100},b=a;
    for(unsigned i=0;i<3000;++i)small.advance(a,1,2);large.advance(b,3000,2);
    check(a.energy==2&&a.energy==b.energy&&small.remainder()==large.remainder(),"varying tick cadence preserves exact fractions");
    energy::Regeneration odd;vitals::Resources c{100,100,0,100};unsigned total=0;
    for(unsigned i=0;i<100;++i){const unsigned elapsed=i%2?17:29;total+=elapsed;odd.advance(c,elapsed,3);}
    check(c.energy==total*3/3000&&odd.remainder()==total*3%3000,"irregular frames preserve pip-ms remainder");
    energy::Regeneration cap;vitals::Resources full{100,100,19,20};auto step=cap.advance(full,30000,10);
    check(full.energy==20&&step.delta==1&&cap.remainder()==0,"never regenerate beyond capacity or bank full-barcredit");
    full.energy-=5;cap.advance(full,299,10);check(full.energy==15,"new spend cannotconsume discarded full-barcredit");cap.advance(full,1,10);check(full.energy==16,"fractional energy grants at exact3000pip-msinterval");
    energy::Regeneration dead;vitals::Resources corpse{0,100,0,20};step=dead.advance(corpse,3000,2);
    check(corpse.energy==0&&!step.changed&&dead.remainder()==0,"dead actor gainsnoenergy");corpse.health=100;corpse.max_energy=0;
    dead.advance(corpse,3000,2);check(corpse.energy==0,"zero capacity gainsnothing");
    energy::Regeneration stall;vitals::Resources paused{100,100,0,1000000};step=stall.advance(paused,UINT64_MAX,10);
    check(paused.energy==100&&step.used_ms==30000&&step.discarded_ms==UINT64_MAX-30000,"unbounded stall cannotoverflow orgrantunboundedburst");
    energy::Regeneration drain;vitals::Resources draining{100,100,20,20};
    auto drained=drain.advance(draining,1000,-1);
    check(drained.changed&&draining.energy==19&&drain.remainder()==2000,"negative pips spend fractional Energy even at full capacity");
    drain.advance(draining,2000,-1);
    check(draining.energy==19&&drain.remainder()==0,"negative pip cadence preserves exact loss");
    drain.advance(draining,30000,-10);
    check(draining.energy==0&&drain.remainder()==0,"degeneration stops at zero without capacity debt");
    std::uint32_t preserved_debt=3;drain.advance(draining,3000,-2,preserved_debt);
    check(draining.energy==0&&preserved_debt==3,"degeneration cannot deepen existing debt");
    bool rejected=false;try{stall.advance(paused,1000,-11);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"out-of-range negative pips rejected");
    paused.energy=paused.max_energy+1;rejected=false;try{stall.advance(paused,1000,2);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"invalid overspent capacity rejected");
    std::cout<<"Native fractional energy regeneration/capacity tests passed.\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<"\n";return 1;}
