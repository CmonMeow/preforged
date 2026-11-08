#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace effects=skills::effects;
using Relation=combat::npcs::Relation;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Necrotic World line "+std::to_string(at.line()));
}
std::shared_ptr<const navigation::Mesh> mesh(const std::filesystem::path& p,bool blocked=false){
 std::filesystem::create_directories(p.parent_path());std::ofstream out(p,std::ios::binary);check(bool(out));
 auto word=[&](unsigned n){for(unsigned i=0;i<4;++i)out.put(char(n>>(8*i)));};
 auto real=[&](float n){word(std::bit_cast<unsigned>(n));};
 out.write("FWNAV001",8);for(unsigned n:{1u,7u,0xeefe704cu,12u,1u,1u})word(n);
 for(float n:{-2000.f,-2000.f,2000.f,2000.f})real(n);word(1);word(blocked?2:1);
 auto cell=[&](float left,float right){for(unsigned i=0;i<5;++i)word(0xffffffff);for(float n:{-2000.f,2000.f,left,right,left,right})real(n);};
 if(blocked){cell(-2000,200);cell(400,2000);}else cell(-2000,2000);
 out.close();return std::make_shared<const navigation::Mesh>(navigation::Mesh::load(p));
}
world::Instance::Npc body(unsigned id,float x,double hp=100,std::optional<bool> flesh=true,Relation relation=Relation::hostile_to_players){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
 d.relation=relation;d.spawn_health={hp,1000};d.damageable=d.conditions_allowed=true;d.fleshy=flesh;if(flesh.has_value())d.fleshy_evidence="synthetic Necrotic";
 n.combat_definition=d;return n;
}
void corpse(world::Instance& w,unsigned caster,unsigned target,unsigned at){
 check(w.damage_npc(*w.effect_scope(caster),*w.effect_scope(target),{combat::Kind::armor_ignoring,10000,20,42},1248,at).killed);
}
void player(world::Instance& w,unsigned id,float x,unsigned team,unsigned hp=500,std::shared_ptr<world::Outbox> box={}){
 w.join(id,"Actor",x,0,box?box:std::make_shared<world::Outbox>(),0,{}, {},100,team);w.set_vitals(id,{hp,1000,100,100},100);check(w.observe_condition_equipment(*w.effect_scope(id),equipment::condition_equipment(inventory::baseline(),0),100));
}
void hold(world::Instance& w,unsigned id,unsigned at,bundles::Disposition disposition=bundles::Disposition::ground){
 const auto owner=*w.effect_scope(id);items::Definition item;item.id=42;item.file=1234;item.type=6;item.model=456;item.encoded_name=u"Bundle";
 const auto wire=w.reserve_ground_item_id();check(bool(wire));
 const auto floor=w.drop_ground_item(owner,item,1,*wire,at,true,[](const auto&){return true;});check(bool(floor));
 check(w.pickup_ground_bundle(owner,floor->scope,1,disposition,at,at+10));check(w.held_bundle(owner).has_value());
}
inventory::Profile poison_gear(bool prefix,bool rune,bool inscription){
 auto p=inventory::baseline();
 const auto add=[&](unsigned id,unsigned type,unsigned slot,unsigned modifier){
  inventory::Item item;item.definition.id=id;item.definition.type=type;item.definition.quantity=1;
  item.bag=2;item.slot=slot;item.definition.modifiers={modifier};p.items.emplace(id,item);
 };
 if(prefix){add(10,26,0,0x246801e4);p.weapons.sets[0].weapon=10;}
 if(inscription){add(11,24,1,0x247801e4);p.weapons.sets[0].offhand=11;}
 if(rune)add(12,7,2,0x27780605);
 return p;
}
int main(int argc,char** argv)try{
 check(argc==2);const auto open=mesh(argv[1]),closed=mesh(std::string(argv[1])+"-blocked",true);
 const auto plan=*effects::necrotic_traversal(12);
 {
  world::Instance w;w.configure_navigation(open);w.configure_npcs({body(100,600)});
  player(w,1,0,1);player(w,2,650,2);const auto owner=*w.effect_scope(1),old_foe=*w.effect_scope(2),dead=*w.effect_scope(100);
  corpse(w,1,100,150);
  check(w.observe_condition_equipment(owner,equipment::condition_equipment(poison_gear(true,false,false),10),200));
  check(w.observe_condition_equipment(owner,std::nullopt,210));
  // Losing equipment authority must not erase the last durable revision.
  check(!w.observe_condition_equipment(owner,equipment::condition_equipment(inventory::baseline(),9),220));
  check(w.observe_condition_equipment(owner,equipment::condition_equipment(poison_gear(true,false,false),10),220));
  check(!w.observe_condition_equipment(owner,equipment::condition_equipment(inventory::baseline(),11),219));
  w.leave(2);w.join(2,"Replacement",650,0,std::make_shared<world::Outbox>(),0,{}, {},230,2);
  w.set_vitals(2,{500,1000,100,100},100);const auto fresh=*w.effect_scope(2);check(fresh!=old_foe);
  check(!w.observe_condition_equipment(old_foe,equipment::condition_equipment(inventory::baseline(),99),230));
  check(!w.complete_necrotic_traversal(owner,plan,1,240)&&!w.traversal_corpse_snapshot(dead,240)->consumed);
  check(w.observe_condition_equipment(fresh,equipment::condition_equipment(inventory::baseline(),0),241));
  check(w.complete_necrotic_traversal(owner,plan,2,241).has_value());
  check(w.has_condition(fresh,combat::conditions::Kind::poison,23240)&&!w.has_condition(fresh,combat::conditions::Kind::poison,23241));
 }
 for(unsigned mode=0;mode<6;++mode){
  world::Instance w;w.configure_navigation(open);w.configure_npcs({body(100,600)});
  player(w,1,0,1);player(w,2,650,2);const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2),dead=*w.effect_scope(100);
  corpse(w,1,100,150);
  check(w.observe_condition_equipment(owner,equipment::condition_equipment(poison_gear(mode==0||mode==5,false,false),1),200));
  check(w.observe_condition_equipment(foe,mode==4?std::nullopt:equipment::condition_equipment(poison_gear(false,mode==1||mode==2||mode==5,mode==2||mode==3),1),200));
  const auto result=w.complete_necrotic_traversal(owner,plan,1,200);
  if(mode>=4){
   check(!result&&!w.traversal_corpse_snapshot(dead,200)->consumed&&w.position(1,200).first==0);
   check(!w.has_condition(foe,combat::conditions::Kind::poison,200));continue;
  }
  const unsigned duration=mode==0?23000:mode==2?11000:14000;
  check(result&&w.has_condition(foe,combat::conditions::Kind::poison,200+duration-1));
  check(!w.has_condition(foe,combat::conditions::Kind::poison,200+duration));
 }

 {
  world::Instance w;w.configure_navigation(open);w.configure_npcs({body(100,600),body(101,800),body(102,900,1000),body(103,900,1000,false)});
  auto box=std::make_shared<world::Outbox>();player(w,1,0,1,500,box);player(w,2,850,2);player(w,3,850,1);player(w,4,1200,2);
  const auto source=*w.effect_scope(1),near=*w.effect_scope(100),far=*w.effect_scope(101),foe=*w.effect_scope(2),npc=*w.effect_scope(102),immune=*w.effect_scope(103);
  unsigned draws=0;w.configure_necrotic_corpse_draws([&](std::size_t count){++draws;check(count==2);return 1u;});
  check(w.necrotic_traversal_available(source,plan,120)==0&&draws==0);
  corpse(w,1,100,130);corpse(w,1,101,131);
  check(w.necrotic_traversal_available(source,plan,140)==2&&draws==0);
  check(w.necrotic_traversal_available(source,plan,140)==2&&draws==0);
  check(!w.traversal_corpse_snapshot(near,140)->consumed&&!w.traversal_corpse_snapshot(far,140)->consumed);
  w.move(1,200,0,150);box->drain();std::function<void()> publication;
  const auto result=w.complete_necrotic_traversal(source,plan,1,200,&publication);
  check(result&&result->corpse.corpse.scope==far&&result->destination.point.x==800&&publication&&box->drain().empty());
  check(w.position(1,200).first==800&&w.traversal_corpse_snapshot(far,200)->consumed&&!w.traversal_corpse_snapshot(near,200)->consumed);
  check(w.has_condition(foe,combat::conditions::Kind::poison,200)&&w.has_condition(npc,combat::conditions::Kind::poison,200));
  check(!w.has_condition(*w.effect_scope(3),combat::conditions::Kind::poison,200)&&!w.has_condition(*w.effect_scope(4),combat::conditions::Kind::poison,200));
  check(!w.has_condition(immune,combat::conditions::Kind::poison,200));
  publication();const auto sent=box->drain();
  const auto word=[](const world::Packet& p,unsigned at){return unsigned(p[at])|(unsigned(p[at+1])<<8)|(unsigned(p[at+2])<<16)|(unsigned(p[at+3])<<24);};
  const auto position=std::find_if(sent.begin(),sent.end(),[&](const auto& p){return p.size()==16&&p[0]==44&&p[1]==0&&word(p,2)==1;});
  const auto poison=std::find_if(sent.begin(),sent.end(),[&](const auto& p){return p.size()==10&&p[0]==240&&p[1]==0&&word(p,2)==102&&(word(p,6)&0x40);});
  check(position!=sent.end()&&poison!=sent.end()&&poison<position); // Fixed97 frame publishes poisoned NPC flags before caster position.
  publication();check(box->drain().empty());
  check(!w.complete_necrotic_traversal(source,plan,1,201)&&draws==1);
  check(w.complete_well_cast(source,*effects::well_cast_plan(92,12),7,300,1248).outcome==world::Instance::WellCastOutcome::created);
  check(w.traversal_corpse_snapshot(near,300)->consumed);
  check(w.necrotic_traversal_available(source,plan,300)==0&&draws==1);
  w.tick(1200);check(w.position(1,1200).first==800);
  check(w.has_condition(foe,combat::conditions::Kind::poison,17199)&&!w.has_condition(foe,combat::conditions::Kind::poison,17200));
 }
 {
  world::Instance w;w.configure_navigation(open);player(w,1,0,1);player(w,2,700,2);
  const auto source=*w.effect_scope(1),corpse=*w.effect_scope(2);w.move(2,1000,0,150);const auto expected=w.position(2,650);
  check(w.kill(corpse,650));const auto first=w.traversal_corpse_snapshot(corpse,650);check(first&&std::abs(first->x-expected.first)<.01);
  const auto used=w.complete_necrotic_traversal(source,plan,1,650);check(used&&used->corpse.corpse.scope==corpse);
  check(w.resurrect(corpse,650)&&w.kill(corpse,650));const auto next=w.traversal_corpse_snapshot(corpse,650);
  check(next&&next->revision>first->revision&&!next->consumed&&!effects::consume_necrotic_traversal_corpse(used->corpse,*next));
  check(w.complete_necrotic_traversal(source,plan,2,650).has_value());
 }
 // Admission does not reserve the body: a competing Well can consume it
 // during activation, and completion must neither relocate nor draw again.
 {
  world::Instance w;w.configure_navigation(open);w.configure_npcs({body(100,600)});player(w,1,0,1);
  const auto source=*w.effect_scope(1);unsigned draws=0;
  w.configure_necrotic_corpse_draws([&](std::size_t){++draws;return 0u;});corpse(w,1,100,150);
  check(w.necrotic_traversal_available(source,plan,200)==1&&draws==0);
  check(w.complete_well_cast(source,*effects::well_cast_plan(92,12),7,250,1248).outcome==world::Instance::WellCastOutcome::created);
  check(w.necrotic_traversal_available(source,plan,300)==0);
  check(!w.complete_necrotic_traversal(source,plan,1,300)&&draws==0&&w.position(1,300).first==0);
 }
 for(int kind:{0,1,2}){
  world::Instance w;w.configure_navigation(kind==0?closed:open);w.configure_npcs({body(100,600,100,kind==1?std::optional<bool>{}:std::optional(kind!=2))});
  player(w,1,0,1);player(w,2,600,1);hold(w,1,110);corpse(w,2,100,150);w.leave(2);
  const auto source=*w.effect_scope(1),corpse_scope=*w.effect_scope(100);std::function<void()> publication;
  const auto availability=w.necrotic_traversal_available(source,plan,200);
  if(kind==1)check(!availability);else check(availability==std::size_t(kind==0?1:0));
  check(!w.complete_necrotic_traversal(source,plan,1,200,&publication)&&!publication&&w.position(1,200).first==0);
  check(!w.traversal_corpse_snapshot(corpse_scope,200)->consumed);
  check(w.held_bundle(source).has_value()&&w.ground_item_count()==0);
 }
 for(auto disposition:{bundles::Disposition::ground,bundles::Disposition::destroy}){
  world::Instance w;w.configure_navigation(open);w.configure_npcs({body(100,800)});player(w,1,0,1);hold(w,1,110,disposition);corpse(w,1,100,150);
  const auto source=*w.effect_scope(1);const auto result=w.complete_necrotic_traversal(source,plan,1,200);
  check(result&&!w.held_bundle(source)&&w.position(1,200).first==800);
  if(disposition==bundles::Disposition::ground){check(result->dropped_bundle&&w.ground_item_count()==1);
   check(result->dropped_bundle->position.x==0&&result->dropped_bundle->position.y==0&&result->dropped_bundle->definition.model==456);
  }else check(!result->dropped_bundle&&w.ground_item_count()==0);
  check(!w.complete_necrotic_traversal(source,plan,1,201));
 }
 // Known Ranger pets have pre-exploited corpses; flesh alone is insufficient.
 {
  world::Instance w;w.configure_navigation(open);w.configure_npcs({body(100,600)});player(w,1,0,1);player(w,2,600,1);
  const auto source=*w.effect_scope(1),pet=*w.effect_scope(100);check(w.bind_pet_owner(*w.effect_scope(2),pet));corpse(w,1,100,150);
  const auto fact=w.traversal_corpse_snapshot(pet,200);check(fact&&fact->consumed);
  w.leave(2);check(w.traversal_corpse_snapshot(pet,200)->consumed);
  check(!w.complete_necrotic_traversal(source,plan,1,200)&&w.position(1,200).first==0&&w.traversal_corpse_snapshot(pet,200)->consumed);
  check(w.complete_well_cast(source,*effects::well_cast_plan(92,12),1,200,1248).outcome!=world::Instance::WellCastOutcome::created);
 }
 unsigned cases=0;
 for(int allowed:{-1,0,1})for(bool shield:{false,true})for(int reversal:{-1,0,20}){
  ++cases;world::Instance w;w.configure_navigation(open);auto ally=body(101,990,500,true,Relation::allied_to_players);
  if(allowed>=0){ally.combat_definition->healing_allowed=bool(allowed);ally.combat_definition->healing_evidence="synthetic Necrotic Seed";}
  w.configure_npcs({body(100,800),ally});auto box=std::make_shared<world::Outbox>();
  player(w,1,0,1,500,box);player(w,2,850,2);player(w,3,860,2,1);player(w,4,1010,2);
  const auto source=*w.effect_scope(1),corpse=*w.effect_scope(100),a=*w.effect_scope(2),b=*w.effect_scope(3),monk=*w.effect_scope(4),npc=*w.effect_scope(101);
  for(auto target:{a,b}){
   check(w.degeneration_hex(source,target,*effects::degeneration_hex(2998,12),100));
   check(w.complete_healing_seed(monk,target,*effects::healing_seed(12),0,100));
   if(shield)check(w.status_mutation(target,1399,800,12,100,6100,true));
  }
  if(reversal>=0)check(w.status_mutation(b,307,900,reversal,100,8100,true));
  ::corpse(w,1,100,150);
  box->drain();std::function<void()> publication;const auto result=w.complete_necrotic_traversal(source,plan,1,200,&publication);
  if(allowed<0){check(!result&&!publication&&box->drain().empty()&&w.position(1,200).first==0);
   check(!w.traversal_corpse_snapshot(corpse,200)->consumed&&!w.has_condition(a,combat::conditions::Kind::poison,200)&&w.resources(2)->health==500&&w.resources(3)->health==1);continue;}
  check(result&&publication&&box->drain().empty());
  const unsigned loss=effects::fragility(2998,12)->damage-(shield?5:0),heal=effects::healing_seed(12)->healing;
  const unsigned converted=reversal<0?0:std::min(loss,effects::reversal_fortune(reversal)->maximum_healing);const bool second=loss>converted;
  check(w.resources(2)->health==500-loss+heal+(second?heal:0));
  check(w.resources(3)->health==1+heal+converted-(loss-converted)+(second?heal:0));
  check(w.resources(4)->health==500+heal+(second?heal:0));check(w.npc_combat_snapshot(npc)->health->current==500+(allowed?heal+(second?heal:0):0));
  publication();const auto packets=box->drain();check(!packets.empty());publication();check(box->drain().empty());
 }
 check(cases==18);std::cout<<"Necrotic World selection/consumption, movement, flesh, blocked step/bundle preservation, bundle ground/destroy, same-time player redeath, pre-exploited pet and18Poison/Fragility/Seed cohort cases pass\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
