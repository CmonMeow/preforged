#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Healing Seed cast line "+std::to_string(at.line()));}
int main()try{
 const auto* row=find_skill(274);const auto canonical=effects::healing_seed(12);
 check(row&&canonical&&row->energy_cost()==10&&row->target_body_resource==489&&
       canonical->activation==2000&&canonical->aftercast==750&&canonical->recharge==25000&&canonical->duration==10000);
 // Contexts: primary player, secondary player, primary NPC, secondary NPC,
 // healing-immune NPC. Modes exercise missing capabilities before/after start,
 // invalid/replaced equipment, target retirement, cancellation and interruption.
 for(unsigned context=0;context<5;++context)for(unsigned mode=0;mode<23;++mode){
  const bool npc=context>=2,secondary=context==1||context==3,immune=context==4;const unsigned target=mode==20?1:mode==21?3:npc?100:2,divine=secondary?0:12;
  if(npc&&(mode==14||mode==15))continue;
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{500,1000};d.healing_allowed=mode==11?std::optional<bool>{}:std::optional(!immune);d.healing_evidence=d.healing_allowed?"authored fixture":"-";n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Caster",0,0,box,0,{world::Packet{165,0,1,0,0,0,static_cast<std::uint8_t>(secondary?6:3),0}}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{500,1000,100,100},100);check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  w.join(3,"Foe",150,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),destination=*w.effect_scope(target);const auto plan=*effects::healing_seed(12);
  
  if(mode==15)check(w.degeneration_hex(*w.effect_scope(3),destination,*effects::degeneration_hex(129,0),100));
  inventory::Profile inventory;inventory.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item weapon;weapon.definition.id=100;weapon.definition.type=26;weapon.definition.quantity=1;weapon.definition.modifiers={0x22b80014};weapon.bag=1;inventory.items.emplace(100,weapon);inventory.weapons.sets[0].weapon=100;
  const auto quote=[&](auto at)->std::optional<effects::EnchantmentDuration>{auto equipment=items::weapon_passives::equipped_enchanting(inventory,1);auto active=w.status_snapshot(source,at);return equipment&&active?effects::enchanting_duration(274,plan.duration,source,*equipment,*active,at):std::nullopt;};
  casting::Hooks h;h.resources=[&](auto id){return w.resources(id);};h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[](auto,float,bool){return false;};
  h.healing_seed_target_valid=[&](auto selected,unsigned df,auto at){return mode!=4&&!(mode==22&&at>=3000)&&w.valid_healing_seed_target(source,selected,df,at);};
  h.consume_energy_at=[&](auto cost,auto at){check(cost==10);return w.consume_energy(1,cost,source,at);};h.events=[&](const auto& packets){box->enqueue(packets);};
  h.before_spell_effect=[&](auto used,auto event,auto at){return mode!=10&&w.before_spell_effect(source,used,event,at);};
  h.enchantment_duration_ready=[&](unsigned skill,auto base,auto at){check(skill==274&&base==plan.duration);return bool(quote(at));};
  unsigned attempts=0;h.healing_seed=[&](auto selected,const auto& p,unsigned df,auto at)->casting::EffectCompletion{++attempts;check(selected==destination&&p==plan&&df==divine);auto duration=quote(at);const bool ok=duration&&w.complete_healing_seed(source,selected,p,df,at,&*duration);return {ok,{},{},{},false,true};};
  if(mode==16)h.before_spell_effect={};if(mode==18)h.target_scope={};
  if(mode==1)h.healing_seed={};if(mode==5)inventory.items.at(100).definition.modifiers={0x22b800ff};if(mode==12)h.healing_seed_target_valid={};
  Policy policy;policy.primary=secondary?6:3;policy.secondary=secondary?3:0;policy.campaigns=255;policy.learned[274]=1;Slots bar{};bar[0]={274,0};attributes::Ranks ranks{};ranks[13]=12;ranks[16]=divine;casting::Runtime cast(1);
  const bool denied=mode==1||mode==4||mode==5||mode==12||mode==16||mode==18||mode==20||mode==21||(npc&&mode==11);
  check(casting::Runtime::effect_supported(274));const auto admission=cast.start(1000,{274,0,target},bar,policy,ranks,true,h);
  check(admission==(mode==1||mode==5||mode==12||mode==16||mode==18?casting::Reason::unsupported:denied?casting::Reason::target:casting::Reason::accepted));
  if(denied){check(!cast.active()&&w.resources(1)->energy==100&&attempts==0);continue;}
  check(w.resources(1)->energy==90);cast.update(2999,h);check(attempts==0);
  if(mode==2)h.healing_seed={};if(mode==3)h.target_scope=[&](auto id)->std::optional<effects::Scope>{auto scope=w.effect_scope(id);if(scope&&id==target)++scope->generation;return scope;};
  if(mode==6)inventory.items.at(100).definition.modifiers={0x22b800ff};if(mode==7)inventory.items.at(100).definition.modifiers={0x22b8000a};if(mode==8)cast.cancel(h);if(mode==9)cast.interrupt(2999,h);if(mode==13)h.healing_seed_target_valid={};
  if(mode==17)h.before_spell_effect={};if(mode==19)h.enchantment_duration_ready={};
  box->drain();cast.update(3000,h);check(!cast.active());const bool applied=mode==0||mode==7||mode==11||mode==14||mode==15;check(attempts==unsigned(applied));
  const auto packets=box->drain();cast.update(3000,h);check(attempts==unsigned(applied));check(box->drain().empty());const auto body=*particles::targeted_body_2022(target,1,274);check(std::count(packets.begin(),packets.end(),body)==unsigned(applied));
  if(!applied){check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))==packets.end());continue;}
  const auto finished=std::find(packets.begin(),packets.end(),casting::terminal(58,1)),visual=std::find(packets.begin(),packets.end(),body);
  const auto added=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==20&&p[0]==65&&(unsigned(p[6])|(unsigned(p[7])<<8))==274;});
  check(finished!=packets.end()&&added!=packets.end()&&finished<visual&&visual<added);check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==1);
  const double health=npc?w.npc_combat_snapshot(destination)->health->current:w.resources(target)->health;check(health==(secondary||immune?500:mode==15?526:538));

  const unsigned duration=mode==7?11000:12000;check(*added==casting::effect_add(target,274,12,unsigned((*added)[12])|(unsigned((*added)[13])<<8)|(unsigned((*added)[14])<<16)|(unsigned((*added)[15])<<24),float(duration)/1000.f));
  check(cast.start(3001,{274,0,target},bar,policy,ranks,true,h)==casting::Reason::busy);check(cast.start(4000,{274,0,target},bar,policy,ranks,true,h)==casting::Reason::recharge);
  if(context==0&&mode==0){
   const auto enemy=*w.effect_scope(3);const auto burn=*effects::energy_drain(42,12);
   const auto before=w.resources(target)->health;
   check(w.before_spell_effect(enemy,42,1,4100));
   check(w.energy_drain(enemy,destination,burn,20,4100,nullptr,1));
   check(w.resources(target)->health==before-burn.amount*burn.damage_per_energy+plan.healing);
  }
  const unsigned effect=unsigned((*added)[12])|(unsigned((*added)[13])<<8)|(unsigned((*added)[14])<<16)|(unsigned((*added)[15])<<24);
  w.tick(3000+duration);const auto ended=box->drain();check(std::count(ended.begin(),ended.end(),casting::effect_remove(target,effect))==1);
 }
 std::cout<<"PASS Healing Seed paid player/NPC/secondary/immune cast, nonself target, capability, cost, duration, publication, replay and expiry guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
