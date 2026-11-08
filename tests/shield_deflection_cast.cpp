#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Shield of Deflection cast line "+std::to_string(at.line()));}
int main()try{
 for(unsigned context=0;context<5;++context)for(unsigned mode=0;mode<20;++mode){
  const bool npc=context>=2,secondary=context==3,immune=context==4,self=context==1;const unsigned target=npc?100:self?1:2,divine=secondary?0:12;
  if(npc&&(mode==14||mode==15))continue;
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{500,1000};d.healing_allowed=mode==11?std::optional<bool>{}:std::optional(!immune);d.healing_evidence=d.healing_allowed?"authored fixture":"-";n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Caster",0,0,box,0,{world::Packet{165,0,1,0,0,0,static_cast<std::uint8_t>(secondary?6:3),0}}, {},100,1);w.set_vitals(1,{self?500u:1000u,1000,100,100},100);
  w.join(2,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{500,1000,100,100},100);check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  w.join(3,"Foe",150,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),destination=*w.effect_scope(target);const auto plan=*effects::shield_deflection(12);
  if(mode==14){w.set_vitals(target,{1,1000,100,100},100);check(w.soul_barbs(*w.effect_scope(3),destination,*effects::soul_barbs(12),20,100));}
  if(mode==15)check(w.degeneration_hex(*w.effect_scope(3),destination,*effects::degeneration_hex(129,0),100));
  inventory::Profile inventory;inventory.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item weapon;weapon.definition.id=100;weapon.definition.type=26;weapon.definition.quantity=1;weapon.definition.modifiers={0x22b80014};weapon.bag=1;inventory.items.emplace(100,weapon);inventory.weapons.sets[0].weapon=100;
  const auto quote=[&](auto at)->std::optional<effects::EnchantmentDuration>{auto equipment=items::weapon_passives::equipped_enchanting(inventory,1);auto active=w.status_snapshot(source,at);return equipment&&active?effects::enchanting_duration(259,plan.duration,source,*equipment,*active,at):std::nullopt;};
  casting::Hooks h;h.resources=[&](auto id){return w.resources(id);};h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[](auto,float,bool){return false;};
  h.ally_enchantment_target_valid=[&](auto selected,unsigned df,auto at){return mode!=4&&w.valid_ally_enchantment_target(source,selected,df,at);};
  h.consume_energy_at=[&](auto cost,auto at){check(cost==10);return w.consume_energy(1,cost,source,at);};h.events=[&](const auto& packets){box->enqueue(packets);};
  h.before_spell_effect=[&](auto used,auto event,auto at){return mode!=10&&w.before_spell_effect(source,used,event,at);};
  h.enchantment_duration_ready=[&](unsigned skill,auto base,auto at){check(skill==259&&base==9000);return bool(quote(at));};
  unsigned attempts=0;h.shield_deflection=[&](auto selected,const auto& p,unsigned df,auto at)->casting::EffectCompletion{++attempts;check(selected==destination&&p==plan&&df==divine);auto duration=quote(at);const bool ok=duration&&w.complete_shield_deflection(source,selected,p,df,at,&*duration);return {ok,{},{},{},false,true};};
  if(mode==16)h.before_spell_effect={};if(mode==18)h.target_scope={};
  if(mode==1)h.shield_deflection={};if(mode==5)inventory.items.at(100).definition.modifiers={0x22b800ff};if(mode==12)h.ally_enchantment_target_valid={};
  Policy policy;policy.primary=secondary?6:3;policy.secondary=secondary?3:0;policy.campaigns=255;policy.learned[259]=1;Slots bar{};bar[0]={259,0};attributes::Ranks ranks{};ranks[15]=12;ranks[16]=divine;casting::Runtime cast(1);
  const bool denied=mode==1||mode==4||mode==5||mode==12||mode==16||mode==18||(npc&&!secondary&&mode==11);
  check(casting::Runtime::effect_supported(259));const auto admission=cast.start(1000,{259,0,target},bar,policy,ranks,true,h);
  check(admission==(mode==1||mode==5||mode==12||mode==16||mode==18?casting::Reason::unsupported:denied?casting::Reason::target:casting::Reason::accepted));
  if(denied){check(!cast.active()&&w.resources(1)->energy==100&&attempts==0);continue;}
  check(w.resources(1)->energy==90);cast.update(1249,h);check(attempts==0);
  if(mode==2)h.shield_deflection={};if(mode==3)h.target_scope=[&](auto id)->std::optional<effects::Scope>{auto scope=w.effect_scope(id);if(scope&&id==target)++scope->generation;return scope;};
  if(mode==6)inventory.items.at(100).definition.modifiers={0x22b800ff};if(mode==7)inventory.items.at(100).definition.modifiers={0x22b8000a};if(mode==8)cast.cancel(h);if(mode==9)cast.interrupt(1249,h);if(mode==13)h.ally_enchantment_target_valid={};
  if(mode==17)h.before_spell_effect={};if(mode==19)h.enchantment_duration_ready={};
  box->drain();cast.update(1250,h);check(!cast.active());const bool applied=mode==0||mode==7||mode==11||mode==14||mode==15;check(attempts==unsigned(applied));
  const auto packets=box->drain();const auto body=*(self?particles::target_body_2022(target,259):particles::targeted_body_2022(target,1,259));check(std::count(packets.begin(),packets.end(),body)==unsigned(applied));
  if(!applied){check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))==packets.end());continue;}
  const auto finished=std::find(packets.begin(),packets.end(),casting::terminal(58,1)),visual=std::find(packets.begin(),packets.end(),body);
  const auto added=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==20&&p[0]==65&&p[6]==3&&p[7]==1;});
  check(finished!=packets.end()&&added!=packets.end()&&finished<visual&&visual<added);check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==1);
  const double health=npc?w.npc_combat_snapshot(destination)->health->current:w.resources(target)->health;check(health==(mode==14?0:secondary||immune?500:mode==15?526:538));
  if(mode==14)continue;
  const unsigned duration=mode==7?10000:11000;check(*added==casting::effect_add(target,259,12,unsigned((*added)[12])|(unsigned((*added)[13])<<8)|(unsigned((*added)[14])<<16)|(unsigned((*added)[15])<<24),float(duration)/1000.f));
  check(cast.start(1251,{259,0,target},bar,policy,ranks,true,h)==casting::Reason::busy);check(cast.start(2000,{259,0,target},bar,policy,ranks,true,h)==casting::Reason::recharge);
  const unsigned effect=unsigned((*added)[12])|(unsigned((*added)[13])<<8)|(unsigned((*added)[14])<<16)|(unsigned((*added)[15])<<24);
  w.tick(1250+duration);const auto ended=box->drain();check(std::count(ended.begin(),ended.end(),casting::effect_remove(target,effect))==1);
 }
 std::cout<<"PASS94 Shield of Deflection actual player/self/NPC/secondary/immune casts,scope/capability/cost/duration/publication/expiry guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
