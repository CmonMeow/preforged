// Detached pet motion acceptance; resource-authority admission gates stay intact.
#include "gw_world_simulation.h"
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Pet maintained motion line "+std::to_string(at.line()));}
unsigned u32(const world::Packet& p,unsigned at){return unsigned(p.at(at))|unsigned(p.at(at+1))<<8|unsigned(p.at(at+2))<<16|unsigned(p.at(at+3))<<24;}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope owner,pet;float pet_x=500,pet_y=50;unsigned pet_plane=0;
 explicit Fixture(std::shared_ptr<const navigation::Mesh> mesh={}){
  if(mesh)w.configure_navigation(mesh);w.join(1,"Pet owner",500,50,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);owner=*w.effect_scope(1);
  check(w.restore_pet_profile(owner,1,pets::Profile{pets::Pet{}},1));check(w.pet_owner_combat(owner,1,0,1000)&&w.pet_instance_mode(owner,false,1000));
  check(w.reconcile_pet_bar_mode(owner,162,true,false,std::array{436u},1000));pet=*w.owned_pet_scope(owner);box->drain();
 }
 void tick(std::uint64_t at){w.tick(at);for(const auto& packet:box->drain())if(packet.size()==16&&packet[0]==44&&packet[1]==0&&u32(packet,2)==pet.actor){
  pet_x=std::bit_cast<float>(u32(packet,6));pet_y=std::bit_cast<float>(u32(packet,10));pet_plane=unsigned(packet[14])|unsigned(packet[15])<<8;}}
 void admission_gates(std::uint64_t at){
  // Default level5 pet has captured30Energy, but NO captured regeneration pips.
  // Motion support must never manufacture that separate maintained authority.
  const auto energy=w.npc_energy_snapshot(pet);check(energy&&energy->current==30&&energy->maximum==30);
  check(!w.mending_target_valid(owner,pet,0,at));check(!w.stage_mending(owner,pet,*mending(12),at)); // Mending is player-recipient only.
  check(!w.succor_target_valid(owner,pet,0,at));check(!w.complete_succor(owner,pet,0,at));
  check(!w.stage_retribution(owner,pet,*retribution(12),at));check(w.maintained_enchantment_count(owner,at)==0);
 }
};
std::shared_ptr<const navigation::Mesh> layered_fixture(const std::filesystem::path& file){
 std::ofstream out(file,std::ios::binary);auto u=[&](unsigned value){for(unsigned i=0;i<4;++i)out.put(char(value>>(8*i)));};auto f=[&](float value){u(std::bit_cast<unsigned>(value));};
 out.write("FWNAV001",8);for(unsigned value:{1u,7u,0xeefe704cu,12u,1u,1u})u(value);f(0);f(0);f(3000);f(100);u(2);
 const auto cell=[&](float left,float right){for(unsigned i=0;i<5;++i)u(0xffffffff);f(0);f(100);f(left);f(right);f(left);f(right);};
 u(2);cell(0,1000);cell(2000,3000);u(1);cell(1000,2000);out.close();
 {std::ofstream links(file.string()+".links");links<<"FWLINK01 7 2 4\n0 0 1 0\n1 0 0 0\n1 0 0 1\n0 1 1 0\n";}
 return std::make_shared<const navigation::Mesh>(navigation::Mesh::load(file));
}
int main(int argc,char** argv)try{
 if(argc!=2)throw std::runtime_error("isolated build navigation fixture path required");
 {Fixture f;const auto still=f.w.maintained_range_path(f.pet,1000,2000);check(still&&still->front().x==500&&still->back().x==500);f.admission_gates(1000);
  f.w.move(1,2500,50,1000);f.tick(1000);f.tick(2000);const auto before=f.w.npc_packets();
  const auto motion=f.w.maintained_range_path(f.pet,2000,2500);check(motion&&motion->back().x>motion->front().x);check(f.box->drain().empty()&&f.w.npc_packets()==before);
  Fixture oracle;oracle.w.move(1,2500,50,1000);oracle.tick(1000);oracle.tick(2000);oracle.tick(2500);
  check(std::abs(motion->back().x-oracle.pet_x)<.0001&&std::abs(motion->back().y-oracle.pet_y)<.0001);f.admission_gates(2000);
 }
 const std::filesystem::path file=argv[1];const auto mesh=layered_fixture(file);bool checked=false;
 {Fixture f(mesh);f.admission_gates(1000);f.w.move(1,2500,50,1000);
  for(std::uint64_t at=1000;at<=8000;at+=100){f.tick(at);if(f.w.plane(1)==f.pet_plane)continue;
   const auto before=f.w.npc_packets();const auto projected=f.w.maintained_range_path(f.pet,at,at+200);
   check(bool(projected));check(f.box->drain().empty()&&f.w.npc_packets()==before);
   Fixture oracle(mesh);oracle.w.move(1,2500,50,1000);for(std::uint64_t step=1000;step<=at;step+=100)oracle.tick(step);oracle.tick(at+200);
   check(std::abs(projected->back().x-oracle.pet_x)<.0001&&std::abs(projected->back().y-oracle.pet_y)<.0001);
   f.admission_gates(at);auto stale=f.pet;++stale.generation;check(!f.w.maintained_range_path(stale,at,at+200));checked=true;break;
  }
 }
 check(checked);std::filesystem::remove(file.string()+".links");std::filesystem::remove(file);
 std::cout<<"PASS static/planar/layered real pet projection against actual World motion;248/Succor unknown-pips and Mending NPC gates preserved\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
