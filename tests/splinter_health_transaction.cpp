#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Splinter Health transaction line "+std::to_string(at.line()));
}
combat::npcs::LiveActor actor(unsigned id){
 combat::npcs::Definition d;d.binding.map=161;d.binding.actor=d.binding.definition=id;d.binding.level=20;
 d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{100,100};d.damageable=true;d.conditions_allowed=true;
 return {d,{1,1,1,161,id}};
}
int main()try{
 auto a=actor(100),b=actor(101);const auto al=a.lifetime(),bl=b.lifetime();
 const combat::Damage damage{combat::Kind::armor_ignoring,41,20,792};
 const auto ap=a.prepare_health_damage(al,damage,1000,true),bp=b.prepare_health_damage(bl,damage,1000,true);
 check(ap&&bp&&ap->result().applied==41&&bp->result().applied==41);
 check(a.actor(al)->health->current==100&&b.actor(bl)->health->current==100);
 check(a.can_commit_health_damage(*ap)&&b.can_commit_health_damage(*bp));
 check(a.commit_health_damage(*ap)&&b.commit_health_damage(*bp));
 check(a.actor(al)->health->current==59&&b.actor(bl)->health->current==59&&a.health_loss_revision(al)==1&&b.health_loss_revision(bl)==1);
 check(!a.can_commit_health_damage(*ap)&&!a.commit_health_damage(*ap));
 auto stale=al;++stale.actor_generation;check(!a.prepare_health_damage(stale,damage,1000,true));
 check(!a.prepare_health_damage(al,damage,1000,false));
 auto elemental=damage;elemental.kind=combat::Kind::fire;check(!a.prepare_health_damage(al,elemental,1000,true));
 const auto before=a.prepare_health_damage(al,damage,1000,true);check(bool(before));
 check(a.heal(al,41,1000));check(!a.can_commit_health_damage(*before));
 // Equal restored Health still cannot revive a token after an intervening loss.
 auto c=actor(102);const auto cl=c.lifetime();const auto cp=c.prepare_health_damage(cl,damage,1000,true);check(bool(cp));
 check(c.apply_damage(cl,{combat::Kind::armor_ignoring,1,20,194},1000).applied==1&&c.heal(cl,1,1000));
 check(c.actor(cl)->health->current==100&&!c.can_commit_health_damage(*cp));
 // A future condition clock is an observation barrier, not a rewind permit.
 check(c.apply_condition(cl,combat::conditions::Kind::weakness,1,2000,1000));check(!c.prepare_health_damage(cl,damage,1000,true));
 // Exact lethal Health commit records loss and cannot be replayed.
 auto d=actor(103);const auto dl=d.lifetime();const auto lethal=d.prepare_health_damage(dl,{combat::Kind::armor_ignoring,200,20,792},1000,true);
 check(lethal&&lethal->result().killed&&d.commit_health_damage(*lethal));check(d.actor(dl)->health->current==0&&d.health_loss_revision(dl)==1&&!d.commit_health_damage(*lethal));
 {
  combat::npcs::Definition definition;definition.binding.map=161;definition.binding.actor=definition.binding.definition=104;
  definition.relation=combat::npcs::Relation::hostile_to_players;definition.spawn_health=combat::Health{50,1000};
  definition.damageable=definition.conditions_allowed=true;
  combat::npcs::LiveActor wounded(definition,{1,1,1,161,104});const auto life=wounded.lifetime();
  check(wounded.apply_condition(life,combat::conditions::Kind::deep_wound,1,1000,10000));
  const auto revision=*wounded.health_loss_revision(life);
  const auto zero=wounded.prepare_health_damage(life,{combat::Kind::armor_ignoring,0,20,792},1000,true);
  check(zero&&wounded.commit_health_damage(*zero)&&wounded.actor(life)->health->current==1);
  const auto hit=wounded.prepare_health_damage(life,{combat::Kind::armor_ignoring,.5,20,792},1000,true);
  check(hit&&hit->result().killed&&hit->result().health_settlement);
  check(wounded.actor(life)->health->current==1&&wounded.health_loss_revision(life)==revision);
  check(wounded.commit_health_damage(*hit)&&wounded.actor(life)->health->current==0);
  check(wounded.health_loss_revision(life)==revision+1&&!wounded.commit_health_damage(*hit));
 }
 std::cout<<"Splinter prepared NPC Health commits preserve loss receipts, scopes and atomic preflight\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
