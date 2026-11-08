// Included inside Instance. Movement is sampled at the trusted hit phase,
// before Health/control reactions can stop the recipient. The ledger's terminal
// event consumes this reservation once; cancellation/death never knocks down.
bool prepare_bulls_charge_contact_locked(const combat::weapon::QualifiedHit& hit){
 const auto source=players_.find(hit.attacker.actor);
 if(source==players_.end()||source->second.effect_scope!=hit.attacker||!source->second.effect_registry||
    !skills::effects::active_bulls_charge(*source->second.effect_registry,hit.attacker,hit.at)||
    hit.delivery!=combat::weapon::AttackDelivery::melee)return true;
 if(hit.at>UINT64_MAX-2000)return false;
 const auto player=players_.find(hit.target.actor);
 if(player!=players_.end()){
  auto& p=player->second;if(p.effect_scope!=hit.target||!p.resources||p.precise_health<=0)return false;
  const auto current=healing_position_locked(p,hit.at),future=healing_position_locked(p,hit.at+1);
  if(!current||!future)return false;
  if(std::hypot(future->x-current->x,future->z-current->z)<=.001f||p.knocked_down_until>hit.at)return true;
  if(hit.at<p.health_updated||p.control_epoch==UINT64_MAX)return false;
 }else{
  const auto npc=npc_combat_.find(hit.target.actor);
  if(npc==npc_combat_.end()||npc->second.hex_scope()!=hit.target)return false;
  const auto moving=npc_moving_at_locked(hit.target.actor,hit.at);if(!moving)return false;if(!*moving)return true;
  auto proposed=npc->second;using K=combat::npcs::control::Knockdown;
  const auto control=proposed.knock_down(proposed.lifetime(),hit.at,2000);
  if(control==K::invalid||control==K::unknown)return false;
  if(control!=K::accepted)return true;
 }
 if(bulls_charge_contacts_.size()>=4096)return false;
 return bulls_charge_contacts_.emplace(hit.event,hit).second;
}
void finish_bulls_charge_contact_locked(const combat::weapon::ResolvedAttack& attack){
 const auto pending=bulls_charge_contacts_.find(attack.event);if(pending==bulls_charge_contacts_.end())return;
 const auto hit=pending->second;bulls_charge_contacts_.erase(pending);
 if(attack.outcome!=combat::weapon::AttackOutcome::hit||attack.attacker!=hit.attacker||attack.target!=hit.target||
    attack.at!=hit.at||attack.before_damage!=hit.at||attack.delivery!=combat::weapon::AttackDelivery::melee)return;
 const auto player=players_.find(hit.target.actor);
 if(player!=players_.end()){
  auto& p=player->second;
  if(p.effect_scope==hit.target&&p.resources&&p.precise_health>0&&p.knocked_down_until<=hit.at)
   if(!knockdown_player_locked(p,2000,hit.at))throw std::logic_error("preflighted Bull's Charge player control failed");
 }else{
  const auto npc=npc_combat_.find(hit.target.actor);if(npc==npc_combat_.end()||npc->second.hex_scope()!=hit.target)return;
  const auto life=npc->second.lifetime();const auto actor=npc->second.actor(life);
  if(!actor||!actor->health||actor->health->current<=0||npc->second.knocked_down(life,hit.at))return;
  using K=combat::npcs::control::Knockdown;
  const auto result=knockdown_npc_locked(hit.target,2000,hit.at);
  if(result==K::invalid||result==K::unknown)throw std::logic_error("preflighted Bull's Charge NPC control failed");
 }
}
