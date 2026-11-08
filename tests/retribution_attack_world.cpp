#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Retribution attack line "+std::to_string(at.line()));
}
int main()try{unsigned cases=0;
 {world::Outbox box,other;world::Outbox::DeferredQueue outer,inner,grandchild;
  {world::Outbox::Capture parent(box,outer);box.enqueue({{1}});
   {world::Outbox::SubCapture child(box,inner);box.enqueue({{2}});other.enqueue({{9}});
    bool rejected=false;try{world::Outbox::Capture forbidden(box,grandchild);}catch(const std::logic_error&){rejected=true;}check(rejected);
    {world::Outbox::SubCapture grand(box,grandchild);box.enqueue({{3}});}
    box.enqueue(std::move(grandchild.packets));box.enqueue({{4}});
   }
   box.enqueue({{5}});check(box.drain().empty());
  }
  check(outer.packets==std::vector<world::Packet>{{1},{5}}&&inner.packets==std::vector<world::Packet>{{2},{3},{4}});
  check(other.drain()==std::vector<world::Packet>{{9}}&&!box.captured_on_this_thread());
  box.enqueue(std::move(inner.packets));box.enqueue(std::move(outer.packets));
  check(box.drain()==std::vector<world::Packet>{{2},{3},{4},{1},{5}});
 }
 for(unsigned skill:{320u,394u,330u,396u})for(unsigned rank:{0u,12u,20u})for(unsigned mode=0;mode<8;++mode){
  ++cases;warrior_test::Fixture f(skill,false);auto& w=f.world;
  w.configure_attack_critical_draws([]{return .999999;});
  if(f.weapon.item_type==5){f.weapon.damage.raw_damage_type=1;check(w.ordinary_weapon(f.source,f.weapon,20));}
  check(w.player_holy_equipment(f.source,mode==7?6:0));w.set_vitals(1,{mode==6?1u:500u,1000,100,100},100);
  w.join(3,"Monk",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(3,{1000,1000,100,100},100);const auto caster=*w.effect_scope(3);
  check(w.ordinary_weapon(caster,std::nullopt,20));
  check(w.party_action(3,158,2,100)&&w.party_action(2,154,3,100));
  if(mode==4)w.set_vitals(2,{1,1000,100,100},100);
  check(w.stage_retribution(caster,f.target,*skills::effects::retribution(rank),100));
  if(mode==1)check(w.status_mutation(f.target,307,81,12,100,8100,true));
  if(mode==2)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,20000));
  if(mode==3)check(w.status_mutation(f.target,259,82,12,100,9100,true));
  if(mode==5)check(w.stage_retribution(f.source,f.source,*skills::effects::retribution(rank),100));
  f.box->drain();w.configure_attack_block_draws([]{return 0.;});
  const auto accepted=w.melee_attack(f.source,skill==330?f.source:f.target,f.weapon,*combat::weapon::attack_skill(skill,12),500);
  check(accepted);w.tick(900);const auto packets=f.box->drain();
  unsigned returns=0,bases=0;double total=0;std::size_t first_base=packets.size(),first_return=packets.size();
  auto u32=[](const world::Packet& p,unsigned n){return std::uint32_t(p[n])|std::uint32_t(p[n+1])<<8|std::uint32_t(p[n+2])<<16|std::uint32_t(p[n+3])<<24;};
  for(std::size_t i=0;i<packets.size();++i){const auto& p=packets[i];if(p.size()!=18||p[0]!=162||p[1]!=0)continue;
   const auto target=u32(p,6),cause=u32(p,10);const auto damage=-double(std::bit_cast<float>(u32(p,14)))*1000;
   if(target==2&&cause==1&&damage>0){++bases;first_base=std::min(first_base,i);total+=std::min<double>(5+rank,std::max(1.,std::floor((damage+1e-4)*.33)))+(mode==7?6:0);}
   if(target==1&&cause==3&&damage>0){++returns;first_return=std::min(first_return,i);}
  }
  const bool none=mode==2||mode==3;const unsigned expected=mode==6?0u:500u-unsigned(none?0:total);
  if(w.retribution_damage_failures()||w.retribution_pending_damage_contacts()||w.resources(1)->health!=expected||returns!=(none?0:bases)){
   std::cerr<<"skill "<<skill<<" rank "<<rank<<" mode "<<mode<<" hp "<<w.resources(1)->health<<" expected "<<500-total<<" base "<<bases<<" return "<<returns<<" fail "<<w.retribution_damage_failures()<<" pending "<<w.retribution_pending_damage_contacts()<<'\n';check(false);
  }
  if(returns)check(first_return<first_base);
 }
 // Autonomous hostile NPC attacks use the same committed damage consumer.
 for(unsigned rank:{0u,12u,20u})for(bool sensitive:{false,true}){
  ++cases;world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;
  d.holy_traits={sensitive,false};d.light_sensitive_evidence="synthetic reviewed test trait";d.summoned_evidence="synthetic reviewed test trait";
  n.combat_definition=d;w.configure_npcs({n});
  auto box=std::make_shared<world::Outbox>();w.join(1,"Monk",100,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto target=*w.effect_scope(1),npc=*w.effect_scope(100);check(w.physical_armor(target,60));
  check(w.ordinary_weapon(target,std::nullopt,20)&&w.stage_retribution(target,target,*skills::effects::retribution(rank),100));
  combat::ai::FlarePolicy p;p.source=npc;p.level=20;p.speed=220;p.acquire=1000;p.follow=1500;p.leash=2000;p.flight={1000,32};p.evidence="synthetic Retribution NPC contact";
  combat::ai::ordinary::Reviewed attack{20,20,12,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},p.evidence};
  check(w.configure_npc_hostile_ordinary(p,attack,1000));w.tick(1000);w.tick(1750);
  const auto expected=std::min(5+rank,6u)*(sensitive?2:1);
  check(w.resources(1)->health==980&&w.npc_combat_snapshot(npc)->health->current==500-expected);
  check(w.retribution_damage_failures()==0&&w.retribution_pending_damage_contacts()==0);
 }
 std::cout<<"PASS "<<cases<<" Retribution player attack cases: melee/ranged/area/dual, conversion, blind, block, lethal and reciprocal protection; no paid binding\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
