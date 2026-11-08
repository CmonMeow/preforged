#include "gw_inspired_hex_commit.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Enchantment copy line "+std::to_string(at.line()));}
int main()try{
 for(unsigned skill:{21u,1048u}){
  check(!inspired_hex(skill,0)->allied&&inspired_hex(skill,12)->energy==13&&inspired_hex(skill,20)->energy==19&&inspired_hex(skill,0)->removed_category==Category::enchantment);
  check(casting::Runtime::effect_supported(skill)&&!inspired_hex(skill,21));
 }
 check(classify_effect_copy(180,Category::enchantment).kind==HexCopyKind::player&&classify_hex_copy(180).kind==HexCopyKind::none&&classify_effect_copy(28,Category::enchantment).kind==HexCopyKind::none);
 for(unsigned skill:{21u,1048u})for(unsigned mode=0;mode<8;++mode){
  world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{500,500,50,100},100);}
  const auto owner=*w.effect_scope(1),foe=*w.effect_scope(3);check(w.energy_capacity(1,5,0,100,{80,-2}));
  const unsigned target_id=mode==6?2:mode==7?1:3;const auto target=*w.effect_scope(target_id);
  Slots base{};base[0]={skill,0};HexCopySession session(owner,base);HexCopyIds ids(owner);check(w.configure_adrenaline(owner,base,100));
  Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[skill]=1;
  if(mode!=4)check(w.status_mutation(target,180,1,12,100,60100,true));
  casting::Hooks h;std::vector<Packet> sent;h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto cost,auto at){return w.consume_energy(1,cost,owner,at);};h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.target_valid=[](auto id,float,bool){return id==1;}; // old ally branch would fail for the foe
  h.projectile_target_valid=[&](auto id,float range){return w.projectile_target_valid(owner,id,range,100);};h.before_spell_effect=[](auto,auto,auto){return true;};
  h.inspired_hex=[&](auto scope,const auto& plan,auto source,auto now){const auto result=commit_inspired_hex_copy(w,session,ids,owner,scope,plan,source,now);if(result&&result->changed)sent.push_back(slot_packet_2022(1,result->changed->index,result->changed->after));return bool(result);};
  if(mode==5)h.inspired_hex={};attributes::Ranks ranks{};ranks[3]=12;casting::Runtime cast(1);
  const auto reason=cast.start(100,{skill,0,target_id},base,policy,ranks,true,h);
  check(reason==(mode==5?casting::Reason::unsupported:mode>=6?casting::Reason::target:casting::Reason::accepted));
  if(mode>=5){check(w.resources(1)->energy==50&&session.bar()==base);continue;}
  check(w.resources(1)->energy==40);if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(500,h);
  if(mode==3){w.leave(3);w.join(3,"Replacement",90,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{500,500,50,100},100);}
  cast.update(1100,h);const bool success=mode==0||mode==4;
  check(std::count(sent.begin(),sent.end(),combat::detail::integer(21,1,37))==unsigned(success));
  check(std::count(sent.begin(),sent.end(),*particles::targeted_body_2022(3,1,skill))==unsigned(success));
  if(mode){check(session.bar()==base&&w.resources(1)->energy==40);continue;}
  check(session.bar()[0]==Slot{180,1}&&w.resources(1)->energy==53&&w.status_snapshot(foe,1100)->empty());
  check(w.cooldown_bar_snapshot(owner)->bar==session.bar()&&policy.learned.size()==1&&!policy.learned.contains(180));
  // The copied foreign enchantment is actually cast using a current grant;
  // it never becomes an ordinary learned ability.
  h.admit_session_cast=[&](const Slots& bar,const Policy& p,casting::Request r,auto now)->std::optional<HexCopySession::CastAdmission>{if(bar!=session.bar())return {};return session.admit(owner,p,0,{r.skill,r.copy},now);};
  h.valid_session_cast=[&](const auto& grant,auto now){return session.valid(grant,policy,now);};
  h.status_mutation=[&](auto id,auto effect,auto rank,auto now,auto end,bool add){return w.status_mutation(owner,id,effect,rank,now,end,add);};
  h.heal=[&](auto id,auto amount,float range,bool self){return w.heal_target(1,id,amount,range,self,1900+std::uint64_t(find_skill(180)->activation*1000));};
  h.restore_energy=[&](auto amount){return w.restore_energy(1,amount,owner);};
  check(cast.start(1900,{180,1,1},session.bar(),policy,{},true,h)==casting::Reason::accepted);
  cast.update(1900+std::uint64_t(find_skill(180)->activation*1000),h);const auto own=*w.status_snapshot(owner,3000);check(std::any_of(own.begin(),own.end(),[](const auto& e){return e.skill==180;}));
  const auto changes=expire_hex_copy(w,session,owner,21100);check(changes&&changes->size()==1&&session.bar()==base&&w.cooldown_bar_snapshot(owner)->bar==base);
  check(!session.admit(owner,policy,0,{180,1},21100));
 }
 // World revalidates an offered effect before any bar change/removal/gain.
 {world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id*30.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{100,500,30,50},100);}const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
  Slots base{};base[0]={1048,0};HexCopySession session(a,base);HexCopyIds ids(a);check(w.configure_adrenaline(a,base,100));
  check(w.protective_enchantment(b,b,*regeneration::reviewed(299,12),0,100));const auto plan=*inspired_hex(1048,12);const auto offer=w.prepare_inspired_hex(a,b,plan,100);check(bool(offer));
  check(w.status_mutation(b,180,77,12,100,60100,true));check(!w.commit_inspired_hex(*offer));check(session.bar()==base&&w.resources(1)->energy==30);
  const auto result=commit_inspired_hex_copy(w,session,ids,a,b,plan,{1048,0},100);check(result&&result->changed&&session.bar()[0].skill==180);
 }
 // Copied enchantment removal executes its reviewed end heal before return.
 {world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id*30.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{100,500,30,50},100);}const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
  Slots base{};base[0]={1048,0};HexCopySession session(a,base);HexCopyIds ids(a);check(w.configure_adrenaline(a,base,100));
  check(w.protective_enchantment(b,b,*regeneration::reviewed(299,12),0,100));
  const auto result=commit_inspired_hex_copy(w,session,ids,a,b,*inspired_hex(1048,12),{1048,0},100);
  check(result&&result->changed&&session.bar()[0]==Slot{299,1}&&w.resources(2)->health==141&&w.resources(1)->energy==43);
 }
 // NPC copy uses a real owned214 producer and the same World/session commit.
 {world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{400,500};d.damageable=d.conditions_allowed=true;d.spawn_energy=combat::npcs::Energy{50,50};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Mesmer",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,50,100},100);const auto a=*w.effect_scope(1),b=*w.effect_scope(100);check(w.elemental_armor(a,60,60));check(w.observe_npc_overcast(b,2,1000,"fixture"));
  combat::ai::FlarePolicy ai;ai.source=b;ai.level=20;ai.fire_magic=12;ai.speed=300;ai.acquire=1000;ai.follow=ai.leash=1500;ai.flight={100,32};ai.evidence="fixture";namespace e=combat::ai::elemental;std::vector<e::Reviewed> book{{{214,0},e::Plan{*water_projectile(214,12)},"fixture"}};check(w.configure_npc_elemental_ai(ai,book,4,1000));w.tick(1000);const auto end=w.npc_elemental_ai_snapshot(b)->pending->token.completes;w.tick(end);check(w.npc_combat_snapshot(b)->effects&0x80);
  Slots base{};base[0]={1048,0};HexCopySession session(a,base);HexCopyIds ids(a);check(w.configure_adrenaline(a,base,end));const auto before=w.resources(1)->energy;
  const auto result=commit_inspired_hex_copy(w,session,ids,a,b,*inspired_hex(1048,12),{1048,0},end);check(result&&result->changed&&session.bar()[0]==Slot{214,1}&&w.resources(1)->energy>=before+13);check(!(w.npc_combat_snapshot(b)->effects&0x80));const auto health=w.npc_combat_snapshot(b)->health->current;w.tick(end+500);check(w.npc_combat_snapshot(b)->health->current==health);
 }
 std::cout<<"PASS enemy enchantment copy,conditional Energy,temporary foreign casts/expiry,stale offer/control/target and real NPC214\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
