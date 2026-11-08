#include "gw_item_transactions.h"
#include "gw_armor_upgrades.h"
#include "gw_dye_transactions.h"
#include "gw_salvage_catalog.h"
#include "gw_consumables.h"
#include "gw_morale_continuity.h"
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

int main(int argc,char** argv)try{
    using namespace fw::gw::item_transactions;using fw::gw::items::Packet;
    auto check=[](bool okay,const char* message){if(!okay)throw std::runtime_error(message);};
    for(unsigned word:{0x27e802eau,0x27e902eau,0x27ea02eau}){
        const auto rune=fw::gw::inventory::upgrades::component(word,1);
        check(rune&&rune->kind==fw::gw::inventory::upgrades::Class::rune,"Absorption recognized for real upgrade slot");
        check(!fw::gw::inventory::upgrades::component(word,6),"Absorption foreign profession cannot be installed");
    }    {
        using fw::gw::consumables::continuity::Morale;using fw::gw::skills::effects::Scope;
        Morale state;const Scope first{1,1,1},next{2,1,2},outpost{3,1,3};
        check(state.claim(first,false)==100,"fresh character session morale starts neutral");
        {auto guard=state.guard(first);check(guard.owned(),"owned world generation obtains morale guard");guard.publish(105);}
        check(state.claim(next,false)==105,"PvE transfer and reconnect preserve session morale");
        {auto stale=state.guard(first);check(!stale.owned(),"old actor generation loses continuity ownership");stale.publish(110);}
        check(state.claim(next,false)==105,"stale publisher cannot overwrite new actor state");
        check(state.claim(outpost,true)==100,"outpost entry resets morale explicitly");
        Morale other;check(other.claim(first,false)==100,"separate character session never inherits old morale");
    }
    {
        namespace inv=fw::gw::inventory;namespace con=fw::gw::consumables;
        auto template_profile=inv::baseline();template_profile.next_item=2;inv::Item item;
        item.definition.id=1;item.definition.file=1234;item.definition.model=26784;item.definition.type=15;item.definition.interaction=0x80000;item.definition.encoded_name=u"Synthetic consumable fixture";item.bag=1;template_profile.items.emplace(1,item);
        const auto path=std::filesystem::temp_directory_path()/"gw-reviewed-consumable-test.txt";
        auto hex=[](const inv::Packet& p){std::string s;for(auto b:p){s+="0123456789abcdef"[b>>4];s+="0123456789abcdef"[b&15];}return s;};
        {std::ofstream f(path);f<<std::string(64,'c')<<'\t'<<hex(inv::encode(template_profile))<<'\n';}
        con::Catalog catalog;catalog.load(path);auto owned=template_profile;owned.items.at(1).definition.quantity=2;
        // Signatures ignore owned identity, placement and quantity while
        // retaining all item metadata. Distinct recipes coexist in the index.
        auto second_recipe=template_profile;second_recipe.items.at(1).definition.file=4321;
        {std::ofstream f(path);f<<std::string(64,'c')<<'\t'<<hex(inv::encode(template_profile))<<'\n'
            <<std::string(64,'d')<<'\t'<<hex(inv::encode(second_recipe))<<'\n';}
        catalog.load(path);check(catalog.size()==2,"multiple exact consumable signatures coexist");
        auto relocated=second_recipe.items.at(1);relocated.definition.id=88;relocated.definition.quantity=7;relocated.slot=9;
        check(catalog.plan(relocated)&&catalog.plan(relocated)->increase==5,"indexed lookup normalizes actual identity placement and count");
        relocated.customized=u"Unexpected owner";check(!catalog.plan(relocated),"indexed lookup retains customization metadata");
        {std::ofstream f(path);f<<std::string(64,'e')<<'\t'<<hex(inv::encode(second_recipe))<<'\n'
            <<std::string(64,'f')<<'\t'<<hex(inv::encode(second_recipe))<<'\n';}
        bool duplicate_rejected=false;try{catalog.load(path);}catch(const std::invalid_argument&){duplicate_rejected=true;}
        check(duplicate_rejected&&catalog.size()==2&&catalog.plan(template_profile.items.at(1))&&catalog.plan(second_recipe.items.at(1)),
            "duplicate signature rejects replacement without losing prior index");
        auto proposed=con::debit(owned,1,catalog,true);check(proposed.accepted&&proposed.profile.items.at(1).definition.quantity==1&&owned.items.at(1).definition.quantity==2,"reviewed consumable proposal consumes one actual unit without live mutation");
        check(!con::debit(owned,1,catalog,false).accepted,"mode rejection preserves actual owned consumable");
        auto changed=owned;changed.items.at(1).definition.file++;check(!con::debit(changed,1,catalog,true).accepted,"model alone cannot authorize forged consumable definition");
        proposed=con::debit(template_profile,1,catalog,true);check(proposed.accepted&&proposed.profile.items.empty()&&proposed.packets.size()==2,"last consumable detaches and deletes actual source");
        check(con::morale_next(108)==110&&con::capacity(480,105,20)==524&&con::capacity(56,105,10)==68,"reviewed morale cap and equipment exclusion");
        auto elixir=template_profile;elixir.items.at(1).definition.model=21227;
        {std::ofstream f(path);f<<std::string(64,'d')<<'\t'<<hex(inv::encode(elixir))<<'\n';}
        con::Catalog elixirs;elixirs.load(path);
        check(elixirs.plan(elixir.items.at(1))->increase==10&&con::debit(elixir,1,elixirs,true).accepted,"exact reviewed Elixir gives ten-percent owned debit");
        check(!elixirs.plan(template_profile.items.at(1))&&!con::debit(elixir,1,catalog,true).accepted,"different reviewed consumable cannot substitute by common effect");
        check(con::morale_next(40,10)==50&&con::morale_next(105,10)==110,"Elixir restores relative death penalty and caps morale");
        auto seal=elixir;seal.items.at(1).definition.model=30211;
        {std::ofstream f(path);f<<std::string(64,'e')<<'\t'<<hex(inv::encode(seal))<<'\n';}
        con::Catalog seals;seals.load(path);const auto seal_plan=seals.plan(seal.items.at(1));
        check(seal_plan&&seal_plan->increase==10&&!seal_plan->party&&seal_plan->recharge&&con::debit(seal,1,seals,true).accepted,"Seal exact definition carries self-only full recharge behavior");
        check(!con::debit(seal,1,seals,false).accepted&&!con::valid({5,false,true})&&!con::valid({10,true,true}),"PvP or unreviewed plan semantics cannot debit consumable");
        std::filesystem::remove(path);
    }
    {
        namespace sv=fw::gw::salvage;namespace inv=fw::gw::inventory;
        auto profile=inv::baseline();profile.next_item=3;
        inv::Item target,kit;target.definition.id=1;target.definition.file=1;target.definition.model=1;target.definition.type=7;target.definition.encoded_name=u"Synthetic armor";target.bag=1;target.slot=0;
        kit.definition.id=2;kit.definition.file=2;kit.definition.model=2;kit.definition.type=29;kit.definition.encoded_name=u"Synthetic kit";kit.definition.modifiers={0x25e80300,0x24580002};kit.bag=1;kit.slot=1;
        profile.items={{1,target},{2,kit}};
        inv::Item output;output.definition.file=3;output.definition.model=3;output.definition.type=11;output.definition.quantity=3;output.definition.encoded_name=u"Synthetic material";
        sv::Outcome outcome;outcome.expected_inventory=inv::encode(profile);outcome.reviewed=true;outcome.outputs={output};
        fw::gw::identification::Policy policy{true,true,false};
        auto plan=sv::apply(profile,2,1,outcome,policy);check(plan.accepted(),"reviewed material salvage proposal succeeds");
        check(!plan.profile.items.contains(1)&&plan.profile.items.at(2).definition.modifiers[1]==0x24580001&&plan.profile.items.at(3).definition.quantity==3,"salvage atomically consumes source and one actual charge with output");
        check(profile.items.contains(1)&&profile.items.at(2).definition.modifiers[1]==0x24580002,"salvage proposal does not mutate owned inventory");
        {
            // Synthetic fixtures exercise the wiki equipment-pack rule without
            // asserting that these definitions are captured salvage recipes.
            for(const auto mode:{sv::Mode::materials,sv::Mode::component}){
                auto packed=profile;packed.bags.emplace(42,inv::Bag{42,1,4,5,0});
                if(mode==sv::Mode::component)packed.items.at(2).definition.modifiers[0]=0x25e80200;
                auto recipe=outcome;recipe.mode=mode;
                if(mode==sv::Mode::component){recipe.outputs[0].definition.type=8;recipe.outputs[0].definition.quantity=1;}
                packed.items.at(1).bag=42;
                recipe.expected_inventory=inv::encode(packed);
                auto rejected=sv::apply(packed,2,1,recipe,policy);
                check(rejected.status==sv::Reason::target&&rejected.packets.empty()&&inv::encode(rejected.profile)==recipe.expected_inventory,
                    "equipment pack target rejects salvage with no kit debit item deletion or packets");
                packed.items.at(1).bag=1;packed.bags.at(1).capacity=2;
                recipe.expected_inventory=inv::encode(packed);
                rejected=sv::apply(packed,2,1,recipe,policy);
                check(rejected.status==sv::Reason::capacity&&rejected.packets.empty()&&inv::encode(rejected.profile)==recipe.expected_inventory,
                    "empty equipment pack cannot supply material or component salvage output capacity");
                // IDs deliberately do not match native indexes: pack42 must
                // be skipped even when it sorts before the ordinary bag43.
                packed.bags.emplace(43,inv::Bag{43,1,2,5,0});
                recipe.expected_inventory=inv::encode(packed);
                const auto accepted=sv::apply(packed,2,1,recipe,policy);
                check(accepted.accepted()&&accepted.profile.items.at(3).bag==43&&accepted.profile.items.at(3).slot==0&&
                    accepted.profile.items.at(2).definition.modifiers[1]==0x24580001&&!accepted.profile.items.contains(1),
                    "salvage skips pack and atomically places actual output in later ordinary bag");
                check(inv::encode(packed)==recipe.expected_inventory&&inv::decode(inv::encode(accepted.profile)).items.at(3).bag==43,
                    "salvage proposal preserves input and output placement survives reconnect codec");
                packed.items.at(1).bag=43;
                recipe.expected_inventory=inv::encode(packed);
                check(sv::apply(packed,2,1,recipe,policy).accepted(),"ordinary bag contents remain salvageable");
            }
        }
        auto weapon_profile=profile;weapon_profile.items.at(1).definition.type=5;
        weapon_profile.weapons.sets[0].weapon=1;weapon_profile.weapons.sets[3].weapon=1;
        auto weapon_outcome=outcome;weapon_outcome.expected_inventory=inv::encode(weapon_profile);
        auto weapon_salvage=sv::apply(weapon_profile,2,1,weapon_outcome,policy);
        check(weapon_salvage.accepted()&&!weapon_salvage.profile.items.contains(1)&&
            !weapon_salvage.profile.weapons.sets[0].weapon&&!weapon_salvage.profile.weapons.sets[3].weapon,
            "salvaging carried weapon removes active and inactive set references");
        check(weapon_profile.weapons.sets[0].weapon==1&&weapon_profile.weapons.sets[3].weapon==1,
            "weapon salvage proposal preserves caller weapon sets");
        const auto& published=weapon_salvage.packets;
        const auto memberships=inv::weapon_packets(weapon_salvage.profile);
        check(published.size()>=memberships.size()&&std::equal(memberships.rbegin(),memberships.rend(),published.rbegin()),
            "salvage publishes restored weapon memberships after item removal and output placement");
        auto changed=profile;changed.gold=1;check(!sv::apply(changed,2,1,outcome,policy).accepted(),"outcome bound to exact inventory snapshot");
        outcome.rare_material=true;check(!sv::apply(profile,2,1,outcome,policy).accepted(),"basic kit cannot yield rare material");outcome.rare_material=false;
        outcome.reviewed=false;check(!sv::apply(profile,2,1,outcome,policy).accepted(),"unknown recipe cannot debit kit");outcome.reviewed=true;
        profile.items.at(2).definition.modifiers={0x25e80600,0x24580001};outcome.expected_inventory=inv::encode(profile);
        check(!sv::apply(profile,2,1,outcome,policy).accepted(),"perfect kit cannot salvage materials");
        outcome.mode=sv::Mode::component;outcome.outputs[0].definition.type=8;outcome.outputs[0].definition.quantity=1;outcome.retained=target;
        plan=sv::apply(profile,2,1,outcome,policy);check(plan.accepted()&&plan.profile.items.contains(1)&&!plan.profile.items.contains(2),"perfect extraction retains target and destroys exhausted kit");
        auto retained_weapon=profile;retained_weapon.items.at(1).definition.type=5;retained_weapon.weapons.sets[2].weapon=1;
        auto retained_outcome=outcome;retained_outcome.expected_inventory=inv::encode(retained_weapon);retained_outcome.retained=retained_weapon.items.at(1);
        const auto extracted_weapon=sv::apply(retained_weapon,2,1,retained_outcome,policy);
        check(extracted_weapon.accepted()&&extracted_weapon.profile.weapons==retained_weapon.weapons,
            "component extraction retaining weapon preserves all weapon memberships");
        outcome.retained->definition.file=99;check(!sv::apply(profile,2,1,outcome,policy).accepted(),"retention cannot substitute source identity");
        {
            auto recipe=inv::baseline();recipe.next_item=4;
            auto original=target;original.definition.modifiers={0x21e80901};recipe.items.emplace(1,original);
            auto retained=target;retained.definition.id=2;retained.slot=1;recipe.items.emplace(2,retained);
            auto extracted=outcome.outputs[0];extracted.definition.id=3;extracted.bag=1;extracted.slot=2;recipe.items.emplace(3,extracted);
            const auto path=std::filesystem::temp_directory_path()/"gw-reviewed-salvage-test.txt";
            auto hex=[](const inv::Packet& packet){std::string text;for(auto b:packet){text+="0123456789abcdef"[b>>4];text+="0123456789abcdef"[b&15];}return text;};
            {std::ofstream file(path);file<<std::string(64,'a')<<'\t'<<hex(inv::encode(recipe))<<'\n';}
            sv::Catalog catalog;catalog.load(path);check(catalog.size()==1,"validated reviewed component catalog loads");
            auto actual=profile;actual.items.at(1)=original;auto selected=catalog.component(actual,1);
            check(selected&&selected->retained&&selected->retained->definition.id==1&&selected->outputs[0].definition.id==0,"provider binds real target and leaves output identity for owned allocator");
            actual.items.at(1).definition.file=99;check(!catalog.component(actual,1),"different source asset cannot select recipe");
            auto second=recipe;second.items.at(1).definition.file=99;second.items.at(2).definition.file=99;second.items.at(3).definition.file=123;
            {std::ofstream file(path);file<<std::string(64,'a')<<'\t'<<hex(inv::encode(recipe))<<'\n'
                <<std::string(64,'b')<<'\t'<<hex(inv::encode(second))<<'\n';}catalog.load(path);
            selected=catalog.component(actual,1);
            check(catalog.size()==2&&selected&&selected->outputs[0].definition.file==123&&selected->retained->definition.file==99,
                "salvage signature index selects exact source among multiple recipes");
            auto modified=actual;modified.items.at(1).customized=u"Other Owner";
            check(!catalog.component(modified,1),"salvage signature retains explicit customization");
            {std::ofstream file(path);file<<"missing provenance"<<'\t'<<hex(inv::encode(recipe))<<'\n';}
            bool rejected=false;try{catalog.load(path);}catch(const std::invalid_argument&){rejected=true;}check(rejected&&catalog.size()==2&&catalog.component(actual,1),"invalid catalog rejected atomically");
            {std::ofstream file(path);for(unsigned i=0;i<2;++i)file<<std::string(64,'a')<<'\t'<<hex(inv::encode(recipe))<<'\n';}
            rejected=false;try{catalog.load(path);}catch(const std::invalid_argument&){rejected=true;}check(rejected&&catalog.size()==2&&catalog.component(actual,1),"duplicate catalog source rejects entire replacement index");
            {std::ofstream file(path);file<<std::string(sv::Catalog::maximum_line+1,'a');}
            rejected=false;try{catalog.load(path);}catch(const std::invalid_argument&){rejected=true;}check(rejected&&catalog.size()==2,"oversize catalog line denied before decode");
            std::filesystem::resize_file(path,sv::Catalog::maximum_file+1);
            rejected=false;try{catalog.load(path);}catch(const std::invalid_argument&){rejected=true;}check(rejected&&catalog.size()==2,"oversize catalog file denied before reading");
            std::filesystem::remove(path);
        }
        sv::Pending pending{2,1,4,100};check(pending.current(4,99)&&!pending.current(5,99)&&!pending.current(4,100),"salvage revision and expiry gate");pending.clear();check(!pending.current(4,99),"salvage cancellation invalidates pending state");
    }
    {
        namespace up=fw::gw::inventory::upgrades;namespace inv=fw::gw::inventory;
        auto profile=inv::baseline();profile.next_item=3;
        inv::Item armor,source;armor.definition.id=1;armor.definition.file=1;armor.definition.model=1;armor.definition.type=7;armor.definition.encoded_name=u"a";armor.bag=2;armor.slot=2;armor.profession=6;
        armor.definition.modifiers={0x253003ca,0x21e80901,0x12345678};
        source.definition.id=2;source.definition.file=2;source.definition.model=2;source.definition.type=8;source.definition.encoded_name=u"r";source.definition.quantity=2;source.definition.modifiers={0x25b80000,0x21e80903};source.definition.interaction=0x10000;source.profession=11;source.bag=1;
        profile.items={{1,armor},{2,source}};inv::validate_character(profile);
        {
            auto warrior=profile;warrior.items.at(1).profession=1;warrior.items.at(1).definition.modifiers={0x253003ca,0x27e802ea,0x12345678};
            auto& insignia=warrior.items.at(2);insignia.definition.modifiers={0x25b80000,0xa53003f4};insignia.definition.interaction=0x100000;insignia.profession=1;
            const auto before=inv::encode(warrior);const auto installed=up::apply(warrior,2,1);
            check(installed&&installed->profile.items.at(2).definition.quantity==1&&installed->profile.items.at(1).definition.modifiers==
                std::vector<std::uint32_t>({0x27e802ea,0x12345678,0xa53003f4}),"Dreadnought replaces only insignia and consumes exactly one source");
            check(inv::encode(warrior)==before,"Dreadnought proposal preserves caller armor and source");
            auto replacement=installed->profile;replacement.items.at(2).definition.modifiers={0x25b80000,0x253003cc};
            const auto survivor=up::apply(replacement,2,1);
            check(survivor&&!survivor->profile.items.contains(2)&&survivor->profile.items.at(1).definition.modifiers==
                std::vector<std::uint32_t>({0x27e802ea,0x12345678,0x253003cc}),"replacement removes Dreadnought without leaving hidden armor bonus");
            auto wrong=warrior;wrong.items.at(1).profession=6;wrong.items.at(1).definition.modifiers={0x253003ca};wrong.items.at(2).profession=11;
            check(!up::apply(wrong,2,1),"generic source metadata cannot install Warrior Dreadnought on other profession");
            check(!up::component(0x253003f4,1),"unproven Dreadnought bit variant is not silently accepted");
            for(unsigned profession=1;profession<=10;++profession){
                auto universal=profile;universal.items.at(1).profession=static_cast<std::uint8_t>(profession);
                universal.items.at(1).definition.modifiers={0x253003ca,0x25300425,0x12345678};
                universal.items.at(2).definition.modifiers={0x25b80000,0xa53003ce};universal.items.at(2).definition.interaction=0x100000;
                const auto before=inv::encode(universal);const auto stalwart=up::apply(universal,2,1);
                check(stalwart&&stalwart->profile.items.at(2).definition.quantity==1&&stalwart->profile.items.at(1).definition.modifiers==
                    std::vector<std::uint32_t>({0x25300425,0x12345678,0xa53003ce})&&inv::encode(universal)==before,
                    "Stalwart installs for all professions preserving rune opaque fields and caller state");
            }
            auto exchange=installed->profile;exchange.items.at(2).definition.modifiers={0x25b80000,0xa53003ce};
            const auto stalwart=up::apply(exchange,2,1);
            check(stalwart&&!stalwart->profile.items.contains(2)&&stalwart->profile.items.at(1).definition.modifiers==
                std::vector<std::uint32_t>({0x27e802ea,0x12345678,0xa53003ce}),"Stalwart replaces Dreadnought and consumes last source without stacking armor channels");
            check(!up::component(0x253003ce,1),"unproven Stalwart word variant is not accepted");
        }
        auto plan=up::apply(profile,2,1);check(bool(plan),"owned reviewed rune applies to matching armor");
        check(profile.items.at(2).definition.quantity==2&&profile.items.at(1).definition.modifiers==armor.definition.modifiers,"proposal never mutates live inventory");
        check(plan->profile.items.at(2).definition.quantity==1,"consumes exactly one stacked rune");
        check(plan->profile.items.at(1).definition.modifiers==std::vector<std::uint32_t>({0x253003ca,0x12345678,0x21e80903}),"replaces rune preserves insignia and opaque armor properties");
        source.definition.modifiers={0x25b80000,0x253003cc};source.definition.interaction=0x100000;profile.items.at(2)=source;
        plan=up::apply(profile,2,1);check(plan&&plan->profile.items.at(1).definition.modifiers==std::vector<std::uint32_t>({0x21e80901,0x12345678,0x253003cc}),"insignia replaces only insignia");
        auto unproven=profile;unproven.items.at(1).definition.modifiers={0x21e80901};
        check(bool(up::apply(unproven,2,1)),"native applicability permits first insignia on eligible armor");
        for(auto flags:{0x8000u}){auto blocked=unproven;blocked.items.at(1).definition.interaction=flags;check(!up::apply(blocked,2,1),"native forbidden prefix flag rejects transaction");}
        auto invalid=profile;invalid.items.at(2).profession=7;check(!up::apply(invalid,2,1),"source profession permission enforced");
        invalid=profile;invalid.items.at(2).definition.modifiers={0x253003cc};check(!up::apply(invalid,2,1),"missing native applicability marker rejects transaction");
        invalid=profile;invalid.items.at(2).definition.modifiers={0x25b81b00,0x253003cc};check(!up::apply(invalid,2,1),"wrong native target applicability rejects transaction");
        invalid=profile;invalid.items.at(2).definition.interaction=0x110000;check(!up::apply(invalid,2,1),"ambiguous native source category rejected");
        invalid=profile;invalid.items.at(2).definition.interaction=0x10000;invalid.items.at(1).definition.interaction=0x4000;check(!up::apply(invalid,2,1),"native forbidden suffix flag rejects transaction");
        profile.items.at(2).definition.quantity=1;plan=up::apply(profile,2,1);check(plan&&!plan->profile.items.contains(2),"last upgrade removed only in proposal");
        check(!up::apply(profile,1,1)&&!up::apply(profile,77,1),"missing or same source rejected");
        profile.items.at(2).definition.modifiers={0x21e81101};check(!up::apply(profile,2,1),"wrong profession attribute rune rejected");
        profile.items.at(2).definition.modifiers={0x253003cc,0x253003ca};check(!up::apply(profile,2,1),"ambiguous source rejected");
        up::Pending pending;auto bank=inv::Profile{};check(pending.preview.reserve(profile,bank),"preview reservation succeeds");
        const auto preview=pending.preview.id();check(preview>inv::identity::ground_last&&!profile.items.contains(preview),"preview outsideowned/floor pools and never persisted");
        pending.target=1;pending.revision=4;pending.expires=100;
        check(pending.current(4,99)&&!pending.current(5,99)&&!pending.current(4,100),"revision and expiry gate confirmation");
        pending.clear();check(!pending.current(4,99)&&!pending.preview.id(),"cancel clears reservation and pending source");
    }
    {
        namespace dye=fw::gw::inventory::dye;namespace inv=fw::gw::inventory;
        auto profile=inv::baseline();profile.next_item=4;inv::Item target,vial,other;
        target.definition.id=1;target.definition.file=1;target.definition.model=1;target.definition.type=7;target.definition.encoded_name=u"a";target.definition.interaction=0x20000006;target.definition.dye=12;target.bag=2;target.slot=2;target.profession=6;
        vial.definition.id=2;vial.definition.file=2;vial.definition.model=146;vial.definition.type=10;vial.definition.encoded_name=u"d";vial.definition.quantity=3;vial.definition.modifiers={0x24e80002};vial.definition.dye=13;vial.bag=1;vial.slot=0;
        other=vial;other.definition.id=3;other.definition.modifiers={0x24e80003};other.definition.quantity=1;other.slot=1;
        profile.items={{1,target},{2,vial},{3,other}};inv::validate_character(profile);
        {
            const std::array<std::uint32_t,1> single{2};const std::array<std::uint32_t,2> mixture{2,3};
            auto container=profile;auto& bag_item=container.items.at(1);bag_item.definition.type=3;bag_item.bag=0;bag_item.slot=0;
            container.bags.emplace(42,inv::Bag{42,1,2,5,1});
            auto contents=other;contents.definition.id=4;contents.definition.type=11;contents.bag=42;contents.slot=0;
            container.items.emplace(4,contents);container.next_item=5;const auto before=inv::encode(container);
            const auto dyed=dye::apply(container,1,single);
            check(dyed&&dyed->color==2&&dyed->profile.items.at(1).bag==0&&dyed->profile.bags.at(42).container==1&&
                dyed->profile.bags.at(42).capacity==5&&dyed->profile.items.at(4).bag==42&&dyed->profile.items.at(2).definition.quantity==2,
                "installed owned container accepts single dye preserving identity capacity linkage and contents");
            check(inv::encode(container)==before&&inv::decode(inv::encode(dyed->profile)).items.at(1).definition.dye==2,
                "installed container dye proposal is immutable and survives reconnect codec");
            check(!dye::apply(container,1,mixture)&&inv::encode(container)==before,
                "installed container mixture rejects without any vial debit");
            auto carried=container;carried.bags.erase(42);carried.items.erase(4);carried.items.at(1).bag=1;carried.items.at(1).slot=2;
            check(dye::apply(carried,1,single)&&!dye::apply(carried,1,mixture),"carried container also accepts only a single color");
            container.items.at(2).definition.modifiers={0x24e80023};
            check(!dye::apply(container,1,single),"single legacy premixed vial cannot bypass container single-color rule");
            container.items.at(2)=vial;container.items.at(2).definition.quantity=1;
            const auto last=dye::apply(container,1,single);
            check(last&&!last->profile.items.contains(2)&&last->profile.items.contains(1)&&last->profile.items.contains(4),
                "last dye vial is consumed without removing installed container or its contents");
            container.items.at(1).definition.interaction|=0x200;
            check(!dye::apply(container,1,single),"installed containers retain native forbidden-dye restriction");
            auto unlinked=bag_item;unlinked.definition.interaction&=~0x200u;
            check(!dye::eligible(profile,unlinked),"unplaced container without owned installation link is not eligible");
        }
        const std::array<std::uint32_t,3> mixed{2,2,3};auto plan=dye::apply(profile,1,mixed);
        check(plan&&plan->color==0x223,"native nibble order and repeated source stack consumption");
        check(plan->profile.items.at(2).definition.quantity==1&&!plan->profile.items.contains(3),"each selected vial debited atomically in proposal");
        check(profile.items.at(1).definition.dye==12&&profile.items.at(2).definition.quantity==3&&profile.items.contains(3),"preview/proposal never modifies real possessions");
        check(plan->profile.items.at(1).definition.unknown1==target.definition.unknown1&&plan->profile.items.at(1).profession==target.profession,"dye preserves tint/profession/identity");
        const std::array<std::uint32_t,2> same{2,2};check(!dye::apply(profile,1,same),"multiple identical colors denied");
        profile.items.at(1).definition.interaction|=0x200;check(!dye::apply(profile,1,mixed),"native forbidden-dye flag honored");profile.items.at(1)=target;
        profile.items.at(1).definition.dye=0x322;check(!dye::apply(profile,1,mixed),"same color multiset no-op consumes nothing");profile.items.at(1)=target;
        profile.items.at(2).definition.quantity=1;check(!dye::apply(profile,1,mixed),"repeated stack overdraw denied");profile.items.at(2)=vial;
        profile.items.at(2).definition.modifiers={0x24e80023};check(!dye::apply(profile,1,mixed),"legacy premixed with modern single denied");
        const std::array<std::uint32_t,1> legacy{2};plan=dye::apply(profile,1,legacy);check(plan&&plan->color==0x32,"native legacy vial nibble expansion retained");
        profile.items.at(2).definition.modifiers={0x24e80001};check(!dye::apply(profile,1,legacy),"invalid color denied");
        profile.items.at(2).definition.modifiers={0x24e80002,0x24e80003};check(!dye::apply(profile,1,legacy),"ambiguous color field denied");
        const std::array<std::uint32_t,1> absent{77};check(!dye::apply(profile,1,absent)&&!dye::apply(profile,99,legacy),"unowned source/target denied");
    }
    check(argc==2,"native item transaction fixture path required");
    std::ifstream input(argv[1]);check(bool(input),"native fixtures missing");
    std::map<std::string,Packet> fixtures;std::string label,hex;
    while(input>>label>>hex){check(hex.size()%2==0,"invalid fixture hex");Packet bytes;
        for(std::size_t at=0;at<hex.size();at+=2)bytes.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(at,2),nullptr,16)));
        check(fixtures.emplace(label,std::move(bytes)).second,"duplicate fixture label");}
    auto matches=[&](const char* name,Packet bytes){bytes[1]|=0x80;check(bytes==fixtures.at(name),"differs from original2022 serializer");};
    const std::array<std::uint32_t,3> choices{43,44,0};
    matches("smsg354",salvage_choices_2022(1,42,choices));
    matches("smsg355",salvage_cancel_2022());matches("smsg356",salvage_done_2022());
    matches("smsg357",salvage_success_2022());matches("smsg358",salvage_kept_2022(1));
    matches("smsg360",upgrade_ready_2022(1,43));matches("smsg361",upgrade_choices_2022(1,42,choices));
    matches("smsg362",upgrade_cancel_2022());matches("smsg363",upgrade_done_2022());
    const std::map<std::string,Kind> kinds{{"cmsg106",Kind::identify},{"cmsg117",Kind::salvage_start},
        {"cmsg118",Kind::salvage_cancel},{"cmsg119",Kind::salvage_ack},{"cmsg120",Kind::salvage_materials},
        {"cmsg121",Kind::salvage_component},{"cmsg125",Kind::upgrade_select},{"cmsg126",Kind::upgrade_query},
        {"cmsg127",Kind::upgrade_cancel},{"cmsg128",Kind::upgrade_confirm},
        {"cmsg104-count1",Kind::dye},{"cmsg104-count4",Kind::dye}};
    for(const auto& [name,kind]:kinds){
        auto packet=fixtures.at(name);const auto parsed=request_2022(packet);
        check(parsed&&parsed->kind==kind,"native request not recognized");
        packet[1]&=0x7f;check(request_2022(packet).has_value(),"transport framing changed semantic request");
        packet.push_back(0);check(!request_2022(packet),"trailing fields accepted");
        packet=fixtures.at(name);packet.pop_back();check(!request_2022(packet),"truncated fields accepted");
    }
    const auto identify=*request_2022(fixtures.at("cmsg106"));
    check(identify.source==43&&identify.target==42,"kit and target exchanged");
    const auto salvage=*request_2022(fixtures.at("cmsg117"));
    check(salvage.inventory==1&&salvage.source==43&&salvage.target==42,"salvage inventory/kit/target fields shifted");
    const auto query=*request_2022(fixtures.at("cmsg126"));
    check(query.inventory==1&&query.target==42,"upgrade inventory and target exchanged");
    const auto select=*request_2022(fixtures.at("cmsg125"));
    check(select.selection==1&&select.source==43,"upgrade selector BYTE treated as DWORD");
    const auto dye1=*request_2022(fixtures.at("cmsg104-count1"));
    const auto dye4=*request_2022(fixtures.at("cmsg104-count4"));
    check(dye1.target==42&&dye1.dye_count==1&&dye1.dyes==std::array<std::uint32_t,4>{43,0,0,0},"short dye prefix corrupted");
    check(dye4.dye_count==4&&dye4.dyes==std::array<std::uint32_t,4>{43,44,43,44},"duplicate stack dye references not preserved");
    for(auto count:{0u,5u,0x101u}){auto packet=fixtures.at("cmsg104-count1");packet[6]=std::uint8_t(count);packet[7]=std::uint8_t(count>>8);
        check(!request_2022(packet),"invalid WORD dye count admitted");}
    for(auto name:{"cmsg106","cmsg117","cmsg126"}){
        auto packet=fixtures.at(name);packet[2]=packet[3]=0;check(!request_2022(packet),"zero source/inventory admitted");}
    {auto packet=fixtures.at("cmsg121");packet[2]=3;check(!request_2022(packet),"materials index3 admitted as component index");}
    {auto packet=fixtures.at("cmsg125");packet[2]=4;check(!request_2022(packet),"invalid upgrade slot admitted");}
    check(!request_2022({})&&!request_2022({1})&&!request_2022({0,0}),"unknown/truncated opcode accepted");
    auto rejects=[&](auto make,const char* message){bool rejected=false;try{(void)make();}catch(const std::invalid_argument&){rejected=true;}check(rejected,message);};
    rejects([]{return salvage_choices_2022(0,42,{});},"zero inventory published");
    rejects([]{return upgrade_choices_2022(1,0,{});},"zero preview published");
    rejects([]{const std::array<std::uint32_t,4> values{1,2,3,4};return salvage_choices_2022(1,42,values);},"four component slots published");
    rejects([]{return terminal_2022(358);},"field-bearing kept reply emitted without required BYTE");
    const auto no_choices=salvage_choices_2022(1,42,{});
    check(no_choices.size()==10&&no_choices[8]==0&&no_choices[9]==0,"empty choice list includes invented items");
    std::cout<<"PASS native2022 dye identify salvage upgrade syntax and reply serializer fixtures\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
