// Shipped supplemental content + pinned map162 mesh + production enrollment.
// Owner resources/storage callback are isolated mechanics fixtures, not a paid
// Runtime or rendered-client acceptance claim.
#include "gw_stalker_population.h"
#include "gw_npc_catalog.h"
#include <sodium.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
#include <sstream>
using namespace fw::gw;
namespace {
unsigned checks=0;
void check(bool ok,const std::source_location where=std::source_location::current()){
    ++checks;if(!ok)throw std::runtime_error("shipped Stalker population line "+std::to_string(where.line()));
}
std::string read(const std::filesystem::path& path){
    std::ifstream input(path,std::ios::binary);check(bool(input));
    return {(std::istreambuf_iterator<char>(input)),{}};
}
std::string sha256(std::string_view bytes){
    std::array<unsigned char,crypto_hash_sha256_BYTES> hash{};
    crypto_hash_sha256(hash.data(),reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size());
    std::array<char,crypto_hash_sha256_BYTES*2+1> hex{};sodium_bin2hex(hex.data(),hex.size(),hash.data(),hash.size());
    return hex.data();
}
bool pet_add(const world::Packet& packet){return packet.size()>=2&&packet[0]==177&&packet[1]==0;}
}
int main(int argc,char** argv)try{
    check(argc==3&&sodium_init()>=0);
    const auto root=std::filesystem::absolute(argv[1]).lexically_normal();
    const auto generated=std::filesystem::absolute(argv[2]).lexically_normal();
    check(generated.filename()=="stalker-population-world");
    const auto relative=generated.lexically_relative(root/"build");
    check(!relative.empty()&&*relative.begin()!="..");
    std::filesystem::create_directories(generated);
    const auto appearance_bytes=read(root/"data/gw/pets/presearing-stalker-npcs.txt");
    const auto appearance_hash=sha256(appearance_bytes);
    std::istringstream appearance(appearance_bytes);
    auto population=world::load_npc_catalog(appearance);
    std::istringstream combat_file(read(root/"data/gw/pets/presearing-stalker-combat.txt"));
    const auto combat_catalog=combat::npcs::load_catalog(combat_file,appearance_hash);
    world::bind_combat_catalog(population,combat_catalog,appearance_hash);
    check(population.maps.size()==1&&population.for_map(162).size()==1&&combat_catalog.size()==1);
    const auto wild=population.for_map(162).front();
    check(wild.id==23000&&wild.definition.id==23000&&wild.captured_name=="Melandru's Stalker");
    check(wild.definition.model_file==128406&&wild.definition.flags==13&&wild.definition.primary==2&&wild.definition.level==5);
    check(wild.model_files==std::vector<std::uint32_t>{129924}&&wild.plane==0&&wild.combat_definition.has_value());
    const auto& metadata=*wild.combat_definition;
    check(metadata.relation==combat::npcs::Relation::neutral&&metadata.packed_allegiance==std::optional<std::uint32_t>{2});
    check(metadata.spawn_health&&metadata.spawn_health->current==180&&metadata.spawn_health->maximum==180);
    check(metadata.damageable&&metadata.conditions_allowed&&metadata.healing_allowed==std::optional(true));
    for(const auto armor:{metadata.physical_armor,metadata.general_armor,metadata.earth_armor,
                         metadata.fire_armor,metadata.cold_armor,metadata.lightning_armor})check(armor==std::optional(35));
    std::istringstream baseline_input(read(root/"data/gw/npcs.txt"));
    const auto baseline=world::load_npc_catalog(baseline_input);
    for(const auto& npc:baseline.for_map(162)){
        check(npc.id!=wild.id&&npc.definition.id!=wild.definition.id);
        check(npc.definition.model_file!=wild.definition.model_file);
    }
    auto mutable_mesh=navigation::Mesh::load(root/"data/gw/navigation/113437.fwnav");
    mutable_mesh.discard_unverified_portals();
    const auto cell=mutable_mesh.locate_cell(0,{wild.x,wild.y});check(cell&&*cell==2784);
    const navigation::Point owner_position{wild.x+8,wild.y};
    check(mutable_mesh.walkable(0,owner_position));
    auto mesh=std::make_shared<const navigation::Mesh>(std::move(mutable_mesh));

    auto instance=std::make_unique<world::Instance>();
    instance->configure_navigation(mesh);instance->configure_npcs(population.for_map(162));
    check(world::enroll_presearing_stalker(*instance,wild,100));
    const auto observed_animal=instance->effect_scope(wild.id);check(observed_animal.has_value());
    const auto animal=*observed_animal;
    const auto initial=instance->npc_combat_snapshot(animal);check(initial&&initial->health&&initial->health->current==180);
    auto output=std::make_shared<world::Outbox>();
    instance->join(1,"Stalker Charmer",owner_position.x,owner_position.z,output,0,{},0,100,1);
    instance->set_vitals(1,{10000,10000,40,40},100); // Morale100, not timestamp.
    const auto owner_scope=instance->effect_scope(1);check(owner_scope.has_value());const auto owner=*owner_scope;
    check(instance->physical_armor(owner,0));
    check(instance->restore_pet_profile(owner,0,{},1));
    check(instance->pet_instance_mode(owner,false,100));
    check(instance->pet_owner_combat(owner,1,0,100));
    output->drain();
    for(unsigned now=200;now<=900;now+=100){instance->tick(now);output->drain();}
    const auto neutral_owner=instance->resources(1);check(neutral_owner&&neutral_owner->health==10000);
    check(!instance->npc_flare_ai_snapshot(animal));
    const auto reservation=instance->reserve_pet_charm(owner,animal,1000);check(reservation.has_value());
    check(!instance->npc_flare_ai_snapshot(animal)); // Reservation alone is not hostility.
    const auto attempt=instance->accept_pet_charm(*reservation,1,1000,10000);check(attempt.has_value());
    check(instance->npc_flare_ai_snapshot(animal).has_value());
    for(unsigned now=1100;now<=10900;now+=100){instance->tick(now);output->drain();}
    const auto attacked_owner=instance->resources(1);check(attacked_owner&&attacked_owner->health>0&&attacked_owner->health<10000);

    pets::Profile durable;std::uint64_t revision=0;unsigned writes=0;
    const auto profile_file=generated/"adopted.pet";
    const pets::AdoptionCommit save=[&](const pets::AdoptionWrite& write){
        check(write.expected_revision==revision&&write.expected==durable&&!durable.pet&&write.replacement.pet.has_value());
        const auto bytes=pets::encode(write.replacement);
        std::ofstream file(profile_file,std::ios::binary|std::ios::trunc);
        if(!file)return pets::CommitStatus::unavailable;
        file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));file.close();
        if(!file)return pets::CommitStatus::unavailable;
        durable=write.replacement;++revision;++writes;return pets::CommitStatus::committed;
    };
    const auto adopted=instance->complete_pet_charm(*attempt,true,save,11000);
    check(adopted.status==pets::CharmCompletion::adopted&&writes==1&&revision==1);
    check(std::any_of(adopted.packets.begin(),adopted.packets.end(),pet_add));
    check(instance->owned_pet_scope(owner)==animal);
    const pets::wire::Command heel{pets::wire::Command::Kind::behavior,wild.id,2};
    check(instance->command_pet(owner,heel,11001).has_value()); // Default owned AI was installed, not manually configured.
    check(!instance->reserve_pet_charm(owner,animal,11002));
    check(instance->complete_pet_charm(*attempt,true,save,11003).status!=pets::CharmCompletion::adopted&&writes==1);
    const auto serialized=read(profile_file);
    const auto restored=pets::decode(std::span(reinterpret_cast<const std::uint8_t*>(serialized.data()),serialized.size()));
    check(restored==durable&&restored.pet&&restored.pet->level==5);
    instance->leave(1);check(!instance->owned_pet_scope(owner));instance.reset();

    // Fresh World restores only durable profile; old actor/owner lifetimes are
    // not stored or reused. This is codec reopen, not an SQLite account claim.
    auto reopened=std::make_unique<world::Instance>();
    reopened->configure_navigation(mesh);reopened->configure_npcs(population.for_map(162));
    check(world::enroll_presearing_stalker(*reopened,wild,20000));
    auto reopened_output=std::make_shared<world::Outbox>();
    reopened->join(1,"Restored Charmer",owner_position.x,owner_position.z,reopened_output,0,{},0,20000,1);
    reopened->set_vitals(1,{10000,10000,40,40},100);
    const auto fresh_owner_scope=reopened->effect_scope(1);check(fresh_owner_scope.has_value());const auto fresh_owner=*fresh_owner_scope;
    check(fresh_owner!=owner&&reopened->restore_pet_profile(fresh_owner,revision,restored,1));
    check(reopened->pet_owner_combat(fresh_owner,1,0,20000));
    check(reopened->reconcile_pet_bar_mode(fresh_owner,162,true,false,std::array{411u},20000));
    const auto companion=reopened->owned_pet_scope(fresh_owner);check(companion&&*companion!=animal&&companion->actor!=wild.id);
    const auto health=reopened->npc_combat_snapshot(*companion);check(health&&health->health&&health->health->current==180);
    check(reopened->command_pet(fresh_owner,{pets::wire::Command::Kind::behavior,companion->actor,2},20001).has_value());
    const auto fresh_wild=reopened->effect_scope(wild.id);check(fresh_wild.has_value());
    check(!reopened->reserve_pet_charm(fresh_owner,*fresh_wild,20002)); // Stored pet cannot be replaced implicitly.
    check(!reopened->command_pet(owner,heel,20003));
    const auto published=reopened->npc_packets();check(std::any_of(published.begin(),published.end(),pet_add));
    reopened->leave(1);check(!reopened->effect_scope(companion->actor));
    std::cout<<"PASS "<<checks<<" shipped Stalker content/nav/enrollment/retaliation/adoption/auto-AI/codec-reopen checks\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
