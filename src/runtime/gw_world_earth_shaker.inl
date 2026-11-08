// Included inside Instance's private section. The parent weapon transaction
// owns this proposal from preflight through one hit; no secondary weapon events.
struct EarthShakerWorldContact {
 double failure_roll=0;bool failed=false;
 std::vector<skills::effects::Scope> recipients;
};
std::optional<EarthShakerWorldContact> prepare_earth_shaker_locked(Player& caster,
 skills::effects::Scope target,const combat::weapon::AttackPlan& attack,std::uint64_t now){
 const auto plan=combat::weapon::earth_shaker(attack.rank);
 if(!plan||attack.skill!=354||!combat::weapon::valid(attack)||!caster.team_token||
    !advance_health_locked(caster,now)||caster.precise_health<=0||caster.knocked_down_until>now)return {};
 const auto origin=healing_position_locked(caster,now);if(!origin)return {};
 navigation::Point center;std::uint16_t plane=0;
 if(const auto it=players_.find(target.actor);it!=players_.end()){
  auto& victim=it->second;const auto point=healing_position_locked(victim,now);
  if(victim.effect_scope!=target||!point||!victim.resources||!victim.team_token||
     victim.team_token==caster.team_token||!advance_health_locked(victim,now)||victim.precise_health<=0)return {};
  center=*point;plane=static_cast<std::uint16_t>(victim.surface.plane);
 }else{
  refresh_npc_movement_locked(target.actor,now);
  const auto body=npcs_.find(target.actor);const auto live=npc_combat_.find(target.actor);
  if(body==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=target||
     !live->second.hostile_to_players(live->second.lifetime()))return {};
  if(now<live->second.health_clock(live->second.lifetime()).value_or(now))return {};
  advance_npc_conditions_locked(target.actor,live->second,now);
  const auto actor=live->second.actor(live->second.lifetime());
  if(!actor||!actor->health||actor->health->current<=0||!actor->damageable)return {};
  center={body->second.x,body->second.y};plane=body->second.plane;
 }
 if(plane!=caster.surface.plane||std::hypot(center.x-origin->x,center.z-origin->z)>128)return {};
 if(navigation_){const auto line=layered_->trace(caster.surface,*origin,center);if(!line||line->plane!=plane)return {};}
 // A separate draw, never the accuracy or block variate. Retain it throughout
 // the transaction, including both damage and area planning. Rank5+ needs no
 // random draw, but receives an explicit successful contact fact.
 EarthShakerWorldContact proposal;proposal.failure_roll=.5;
 if(plan->failure_chance){
  if(earth_shaker_failure_draw_)proposal.failure_roll=earth_shaker_failure_draw_();
  else{
   if(!attack_miss_rng_)attack_miss_rng_=std::make_unique<std::mt19937_64>(attack_miss_seed_);
   proposal.failure_roll=std::uniform_real_distribution<double>(0.,1.)(*attack_miss_rng_);
  }
 }
 const auto failed=combat::weapon::earth_shaker_fails(*plan,proposal.failure_roll);if(!failed)return {};
 proposal.failed=*failed;if(*failed)return proposal;
 std::vector<combat::weapon::EarthShakerRecipient> candidates;
 for(auto&[id,p]:players_){
  if(!p.resources||!p.team_token||p.team_token==caster.team_token||p.surface.plane!=plane||p.precise_health<=0)continue;
  const auto point=healing_position_locked(p,now);if(!point)return {};
  if(std::hypot(point->x-center.x,point->z-center.z)>plan->adjacent)continue;
  if(now<p.health_updated||!advance_health_locked(p,now))return {};
  if(p.precise_health<=0)continue;
  const bool immune=p.knocked_down_until>now||(p.effect_registry&&
   (skills::effects::active_dolyak(*p.effect_registry,p.effect_scope,now)||skills::effects::balanced_stance_active(*p.effect_registry,p.effect_scope,now)));
  if(!immune&&p.control_epoch==UINT64_MAX)return {};
  candidates.push_back({p.effect_scope,point->x,point->z,true});
 }
 using K=combat::npcs::control::Knockdown;
 for(auto&[id,live]:npc_combat_){
  const auto body=npcs_.find(id);const auto life=live.lifetime();
  if(body==npcs_.end()||body->second.plane!=plane||!live.hostile_to_players(life))continue;
  refresh_npc_movement_locked(id,now);
  if(std::hypot(body->second.x-center.x,body->second.y-center.z)>plan->adjacent)continue;
  if(now<live.health_clock(life).value_or(now))return {};
  advance_npc_conditions_locked(id,live,now);const auto actor=live.actor(life);
  if(!actor||!actor->health||actor->health->current<=0||!actor->damageable)continue;
  auto probe=live;const auto result=probe.knock_down(life,now,plan->knockdown_ms);
  if(result==K::invalid||result==K::unknown)return {};
  candidates.push_back({live.hex_scope(),body->second.x,body->second.y,true});
 }
 const auto selected=combat::weapon::earth_shaker_recipients(*plan,caster.effect_scope,target,
  AttackOutcome::hit,false,candidates);
 if(!selected)return {};proposal.recipients=*selected;return proposal;
}
void commit_earth_shaker_locked(const EarthShakerWorldContact& proposal,std::uint64_t now){
 if(proposal.failed)return;
 using K=combat::npcs::control::Knockdown;
 for(const auto scope:proposal.recipients){
  if(const auto found=players_.find(scope.actor);found!=players_.end()){
   auto& victim=found->second;
   if(victim.effect_scope!=scope||victim.precise_health<=0||victim.knocked_down_until>now)continue;
   if(!knockdown_player_locked(victim,2000,now))throw std::logic_error("preflighted Earth Shaker player control failed");
  }else{
   const auto found_npc=npc_combat_.find(scope.actor);if(found_npc==npc_combat_.end())continue;
   auto& live=found_npc->second;const auto life=live.lifetime();const auto actor=live.actor(life);
   if(live.hex_scope()!=scope||!actor||!actor->health||actor->health->current<=0||live.knocked_down(life,now))continue;
   const auto result=knockdown_npc_locked(scope,2000,now);
   if(result==K::invalid||result==K::unknown)throw std::logic_error("preflighted Earth Shaker NPC control failed");
  }
 }
}
