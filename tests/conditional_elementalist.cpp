#include "gw_elemental_damage.h"
#include "gw_skill_particle_wire.h"
#include "gw_skill_casting.h"
#include <fstream>
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("conditional elementalist regression");}
int main(){using namespace fw::gw;using namespace skills::effects;using combat::conditions::Kind;
 std::ifstream fixture(FIXTURE_FILE);check(bool(fixture));const auto compare=[&](combat::Packet packet){std::string line;check(bool(std::getline(fixture,line)));std::vector<unsigned char> expected;for(std::size_t at=0;at<line.size();at+=2)expected.push_back(static_cast<unsigned char>(std::stoul(line.substr(at,2),nullptr,16)));packet[1]|=0x80;check(packet==expected);};
 for(unsigned skill:{884u,2192u,1379u})compare(*skills::particles::target_body_2022(43,skill));compare(*skills::casting::success_body_effect(42,217));
 for(const auto& row:skills::skill_catalog_2022){const auto particle=skills::particles::target_body_2022(43,row.id);check(bool(particle)==(row.target_body_resource<2077));if(particle)check(*particle==combat::detail::integer(21,43,row.target_body_resource));}
 check(!skills::particles::target_body_2022(0,884)&&!skills::particles::target_body_2022(43,3433));
 combat::Actor target{2,1,combat::Health{480,480},{},0,true,{},60,60};combat::conditions::State burning;check(burning.apply(Kind::burning,1,100,20000));
 const auto searing=*elemental(884,12,13);check(valid_elemental(searing)&&searing.burn_or_damage&&searing.radius==252&&searing.damage==82&&searing.burning_ms==6000);
 auto clean=target;clean.fire_armor.reset();auto first=elemental_target(searing,clean,{},20,1,100);check(first.accepted&&first.hits.empty()&&first.actor.health->current==480&&first.conditions.has(Kind::burning,6099)&&!first.conditions.has(Kind::burning,6100));
 auto invalid=clean;invalid.damageable=false;check(!elemental_target(searing,invalid,{},20,1,100).accepted);invalid=clean;invalid.health->current=481;check(!elemental_target(searing,invalid,{},20,1,100).accepted);
 auto second=elemental_target(searing,target,burning,20,1,100);check(second.accepted&&second.hits.size()==1&&second.actor.health->current==398&&second.conditions.lookup(Kind::burning)->expires==20100&&second.additions.empty());
 for(const auto& p:combat::damage_packets_2022(1,second.hits[0]))compare(p);
 check(!elemental_target(searing,clean,burning,20,1,100).accepted);
 const auto glowing=*elemental(2192,12,13);auto ice=elemental_target(glowing,target,{},20,1,100,true);check(ice.accepted&&ice.energy==11&&ice.hits[0].damage==41);for(const auto& p:combat::damage_packets_2022(1,ice.hits[0]))compare(p);
 check(elemental_target(glowing,target,{},20,1,100,false).energy==0);
 compare(skills::casting::effect_add(2,480,0,0x80000000,6));compare(skills::casting::effect_remove(2,0x80000000));
 Scope owner{1,2,1};Registry registry(owner);check(registry.apply(owner,{1,1,211,12,Category::hex,100,2100},Replace::same_skill).accepted);check(water_hex(registry,owner,100)&&!water_hex(registry,owner,2100)&&!water_hex(registry,{1,2,2},100));
 Registry other(owner);check(other.apply(owner,{1,1,31,12,Category::hex,100,2100},Replace::same_skill).accepted);check(!water_hex(other,owner,100));
 auto forged=glowing;forged.energy_if_water_hex=999;check(!valid_elemental(forged)&&!elemental(2192,21));
}
