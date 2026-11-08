// Current-production baseline: real hostile NPC, real owned Fragility and
// Blind expiry at the first actual Symbol field pulse. No Seed/proposed source.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <bit>
#include <cmath>
using namespace fw::gw;namespace effects=skills::effects;
static void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("NPC incoming order line "+std::to_string(at.line()));}
static unsigned word(const world::Packet& p,unsigned at){return unsigned(p[at])|(unsigned(p[at+1])<<8)|(unsigned(p[at+2])<<16)|(unsigned(p[at+3])<<24);}
int main()try{
 for(bool dense:{false,true}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
  d.holy_traits.light_sensitive=false;d.light_sensitive_evidence="authored order baseline";n.combat_definition=d;w.configure_npcs({n});
  auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);const auto frag=*effects::fragility(19,12);const auto field=*effects::damage_field(247,12);
  const std::uint64_t due=100+field.first_ms;check(field.damage!=frag.damage&&field.first_ms>0);
  check(w.degeneration_hex(source,target,*effects::degeneration_hex(19,12),100,20));
  check(w.apply_condition(source,target,combat::conditions::Kind::blind,100,field.first_ms));
  check(w.npc_combat_snapshot(target)->health->current==1000-frag.damage);
  check(w.damage_field(source,field,20,100));box->drain();
  if(dense)w.tick(due-1);w.tick(dense?due:due+1);
  const auto packets=box->drain();std::vector<double> losses;
  for(const auto& p:packets)if(p.size()>=18&&p[0]==162&&p[1]==0&&word(p,2)==55&&word(p,6)==100){
   const auto fraction=std::bit_cast<float>(word(p,14));if(fraction<0)losses.push_back(-double(fraction)*1000);
  }
  check(losses.size()==2);check(std::abs(losses[0]-frag.damage)<.001&&std::abs(losses[1]-field.damage)<.001);
  check(w.npc_combat_snapshot(target)->health->current==1000-2*frag.damage-field.damage);
  const auto hp=w.npc_combat_snapshot(target)->health->current;w.tick(due+2);check(w.npc_combat_snapshot(target)->health->current==hp);
  std::cout<<"{\"dense\":"<<(dense?"true":"false")<<",\"at\":"<<due<<",\"first_loss\":"<<losses[0]<<",\"second_loss\":"<<losses[1]<<",\"health\":"<<hp<<"}\n";
 }
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
