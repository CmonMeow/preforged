// Included in World::Instance public section; caller supplies trusted lifetimes.
// Health settlement and reachability share one World lock. Account persistence
// remains outside this lock; this method is never an account callback owner.
bool can_interact_npc(skills::effects::Scope owner,skills::effects::Scope target,std::uint64_t now){
 std::lock_guard lock(mutex_);
 const auto player=players_.find(owner.actor);const auto npc=npcs_.find(target.actor);
 if(owner.instance!=instance_scope_||target.instance!=instance_scope_||player==players_.end()||npc==npcs_.end()||
    player->second.effect_scope!=owner||hidden_starter_trainers_.contains(target.actor)||npc->second.captured_name.empty())return false;
 std::uint64_t generation=0;
 if(const auto found=npc_interaction_generations_.find(target.actor);found!=npc_interaction_generations_.end())generation=found->second;
 else if(const auto found=npc_combat_.find(target.actor);found!=npc_combat_.end())generation=found->second.lifetime().actor_generation;
 if(!generation||target.generation!=generation)return false;
 auto& current=player->second;
 if(!current.resources||current.health_updated>now||current.precise_health<=0||!current.resources->health||
    !advance_health_locked(current,now)||current.precise_health<=0||!current.resources->health)return false;
 return npc_in_range_locked(current,npc->second,now);
}
