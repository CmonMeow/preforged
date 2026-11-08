// Included inside Instance after pet and resurrection authorities.
public:
    struct ShrinePolicy {
        unsigned dead_pet_health_percent=0;
        shrine::Evidence dead_pet_health_evidence;
        shrine::Evidence selection_policy;
        bool fixture_coordinates=false;
    };
    enum class ShrineIssue {catalog_unavailable,activation_unreviewed,outpost_required,member_state_unavailable,dead_pet_policy_missing,destination_invalid,pet_membership_invalid,pet_lifetime_retired,pet_revival_rejected,pet_ai_unavailable,site_plan_stale,claim_rejected,scheduler_not_ready};
    struct ShrineNotice {shrine::Attempt wipe;ShrineIssue issue;};
    struct ShrineRecovery {shrine::Attempt wipe;unsigned site=0;std::uint64_t at=0,protection_until=0;std::vector<skills::effects::Scope> pets;};
private:
    std::optional<shrine::SiteCatalog> shrine_sites_;
    ShrinePolicy shrine_policy_;
    shrine::SiteActivation shrine_activations_;
    shrine::WipeScheduler shrine_wipes_;
    std::map<skills::effects::Scope,std::set<unsigned>> shrine_visited_;
    std::map<unsigned,shrine::ActiveSites> shrine_active_;
    std::uint64_t shrine_activation_revision_=0;

    struct ShrineGroup {shrine::Party party;std::vector<skills::effects::Scope> members;};
    std::map<unsigned,ShrineGroup> shrine_groups_;
    std::uint64_t shrine_epoch_=0,shrine_clock_=0;
    bool shrine_recovering_=false;
    struct ShrineDeath {skills::effects::Scope owner;std::uint64_t sequence=0,at=0;skills::effects::MindWrackEnergy energy;shrine::Place position;};
    std::map<skills::effects::Scope,ShrineDeath> shrine_deaths_;
    std::set<std::uint64_t> shrine_noticed_;
    std::vector<ShrineNotice> shrine_notices_;
    std::vector<ShrineRecovery> shrine_recoveries_;
    // Dedicated invisible protection marker. Health/hex/condition consumer is
    // a separate reviewed capability; no guessed native status/skill is sent.
    std::map<skills::effects::Scope,std::uint64_t> shrine_protection_;
    bool valid_shrine_place_locked(shrine::Place place)const{
        if(!navigation_||!layered_||!shrine::valid(place)||place.plane>=navigation_->plane_count()||place.cell>=navigation_->trapezoid_count(place.plane))return false;
        const navigation::Point point{float(place.x),float(place.y)};
        const navigation::Surface surface{place.plane,place.cell};
        const auto traced=layered_->trace(surface,point,point);return traced&&*traced==surface;
    }
    std::uint64_t shrine_current_clock_locked(std::uint64_t at=0)const{
        at=std::max(at,shrine_clock_);for(const auto& [id,p]:players_)at=std::max({at,p.health_updated,p.energy_updated});return at;
    }
    shrine::Snapshot shrine_snapshot_locked(const ShrineGroup& group,std::uint64_t now)const{
        shrine::Snapshot result{group.party,{},now,0};
        for(const auto scope:group.members){const auto p=players_.find(scope.actor);std::optional<bool> alive;
            if(p!=players_.end()&&p->second.effect_scope==scope&&p->second.resources&&p->second.npc_publication_ready&&
               now>=p->second.health_updated&&now>=p->second.energy_updated)alive=p->second.precise_health>0;
            result.players.push_back({scope,alive});
            if(const auto session=player_pet_sessions_.find(scope.actor);session!=player_pet_sessions_.end()&&session->second.owner==scope&&session->second.actor&&
               valid_pet_owner_locked(scope,*session->second.actor))++result.living_pets;
        }
        return result;
    }
    void sync_shrine_party_rosters_locked(std::uint64_t now){
        if(!shrine_sites_||shrine_recovering_)return;now=shrine_current_clock_locked(now);shrine_clock_=now;
        std::map<unsigned,std::vector<skills::effects::Scope>> current;
        for(const auto& [id,p]:players_){const auto group=parties_.group(std::uint16_t(id));if(group)current[group].push_back(p.effect_scope);}
        for(auto& [id,members]:current)std::sort(members.begin(),members.end());
        for(auto it=shrine_groups_.begin();it!=shrine_groups_.end();){
            const auto next=current.find(it->first);
            if(next==current.end()||next->second!=it->second.members){shrine_wipes_.retire(it->second.party);shrine_active_.erase(it->first);it=shrine_groups_.erase(it);}else ++it;
        }
        for(const auto& [id,members]:current)if(!shrine_groups_.contains(id)){
            if(shrine_epoch_==UINT64_MAX)throw std::overflow_error("shrine party epoch exhausted");
            const shrine::Party party{instance_scope_,++shrine_epoch_,id};
            shrine_groups_.emplace(id,ShrineGroup{party,members});
            std::set<unsigned> inherited;for(const auto member:members)if(const auto seen=shrine_visited_.find(member);seen!=shrine_visited_.end())inherited.insert(seen->second.begin(),seen->second.end());
            if(shrine_activation_revision_==UINT64_MAX)throw std::overflow_error("shrine activation revision exhausted");
            shrine_active_[id]={party,members,shrine_sites_->stamp(),++shrine_activation_revision_,std::move(inherited),now};
        }
        std::erase_if(shrine_deaths_,[&](const auto& row){const auto p=players_.find(row.first.actor);return p==players_.end()||p->second.effect_scope!=row.first||p->second.precise_health>0;});
        std::erase_if(shrine_visited_,[&](const auto& row){const auto p=players_.find(row.first.actor);return p==players_.end()||p->second.effect_scope!=row.first;});
        std::erase_if(shrine_protection_,[&](const auto& row){const auto p=players_.find(row.first.actor);return p==players_.end()||p->second.effect_scope!=row.first||row.second<=now;});
        for(const auto& [id,group]:shrine_groups_)shrine_wipes_.observe(shrine_snapshot_locked(group,now));
    }
    std::optional<skills::effects::MindWrackEnergy> projected_shrine_death_energy_locked(const Player& p,std::uint64_t now)const{
        if(!p.resources||now<p.energy_updated)return {};
        auto resources=*p.resources;resources.health=1; // Alive through this first death boundary.
        auto regeneration=p.energy_regeneration;auto debt=p.energy_debt;auto tail=p.energy_fractional_tail;auto curve=p.overcast_curve;
        const auto segment=[&](std::uint64_t from,std::uint64_t through,int pips)->bool{
            if(curve){auto next=overcast::settle(*curve,p.effect_scope,resources,regeneration,debt,through,pips,tail);if(!next)return false;
                curve=std::move(next->curve);resources=next->resources;regeneration=next->regeneration;debt=next->debt;tail=next->fractional_tail;
            }else regeneration.advance(resources,through-from,pips,debt,resources.max_energy);
            return true;
        };
        bool well=false;for(const auto& field:well_fields_)if(field.plan.energy_pips&&field.team==p.team_token&&field.plane==p.surface.plane&&field.begins<now&&field.expires>p.energy_updated){well=true;break;}
        if(well&&now>p.energy_updated){const auto exposure=well_player_exposure_locked(p,p.energy_updated,now);if(!exposure)return {};
            auto cursor=p.energy_updated;for(const auto& e:*exposure){const auto from=std::max(p.energy_updated,std::uint64_t(std::ceil(e.begins))),through=std::min(now,std::uint64_t(std::ceil(e.ends)));
                if(through<=from)continue;if(from!=cursor||!segment(from,through,std::clamp(p.energy_pips+e.effect.energy_pips,-10,10)))return {};cursor=through;}
            if(cursor!=now)return {};
        }else if(!segment(p.energy_updated,now,p.energy_pips))return {};
        const auto fraction=resources.energy==resources.max_energy?0u:unsigned(regeneration.remainder()*100)+tail;
        const skills::effects::MindWrackEnergy result{resources.energy,resources.max_energy,debt,fraction};
        return skills::effects::valid_mind_wrack_energy(result)?std::optional(result):std::nullopt;
    }
    std::uint64_t shrine_nav_revision_locked()const{
        if(!northlands_gate_)return 1;const auto revision=northlands_gate_->snapshot()[0].revision;return revision==UINT64_MAX?revision:revision+1;
    }
    bool valid_shrine_player_revival_locked(const Player& p,const ShrineDeath& death,std::uint64_t now)const{
        if(!p.resources||p.precise_health>0||!p.resources->max_health||!resurrection_clock_valid(p,now)||
           !skills::effects::valid_mind_wrack_energy(death.energy))return false;
        if(p.overcast_curve){const auto granted=std::min(p.resources->max_energy,death.energy.current);
            auto proposal=overcast::prepare_resurrection(*p.overcast_curve,p.effect_scope,*p.resources,p.energy_regeneration,p.energy_debt,granted,now,p.energy_fractional_tail);
            if(!proposal)return false;
            const auto desired=(std::int64_t(death.energy.current)-death.energy.debt)*overcast::energy_unit+death.energy.fractional_units;
            const auto ceiling=std::int64_t(p.resources->max_energy)*overcast::energy_unit-std::int64_t(proposal->curve.burden_units())*3;
            return overcast::project_current(*proposal,std::min(desired,ceiling));
        }
        return true;
    }
    void capture_shrine_death_energy_locked(const Player& player,std::uint64_t now){
        if(!shrine_sites_)return;shrine_deaths_.erase(player.effect_scope);
        if(!player.resources||player.death_sequence==UINT64_MAX)return;
        const auto energy=projected_shrine_death_energy_locked(player,now);if(!energy)return;
        const auto position=player.corpse_position?player.corpse_position:shadow_position_locked(player,now);if(!position)return;
        const shrine::Place place{position->point.x,position->point.z,unsigned(position->surface.plane),unsigned(position->surface.cell)};
        if(!valid_shrine_place_locked(place))return;
        shrine_deaths_[player.effect_scope]={player.effect_scope,player.death_sequence+1,now,*energy,place};
    }
    void observe_shrine_player_state_locked(std::uint64_t now){sync_shrine_party_rosters_locked(now);}
    bool activate_shrine_member_locked(skills::effects::Scope scope,std::uint64_t now,std::string_view entry={}){
        if(!shrine_sites_)return false;sync_shrine_party_rosters_locked(now);
        const auto p=players_.find(scope.actor);if(p==players_.end()||p->second.effect_scope!=scope)return false;
        const auto group=shrine_groups_.find(parties_.group(std::uint16_t(scope.actor)));if(group==shrine_groups_.end())return false;
        const auto position=healing_position_locked(p->second,now);if(!position)return false;
        const auto cell=navigation_->locate_cell(p->second.surface.plane,*position);if(!cell)return false;
        if(!shrine_activations_.observe(*shrine_sites_,shrine_snapshot_locked(group->second,now),scope,
            {position->x,position->z,unsigned(p->second.surface.plane),unsigned(*cell)},entry))return false;
        const auto observed=shrine_activations_.snapshot(group->second.party);if(!observed)return false;
        auto& active=shrine_active_.at(group->first);auto combined=active.sites;combined.insert(observed->sites.begin(),observed->sites.end());
        // Current party members retain actual visited-site evidence across an
        // epoch change; no coordinate/activation is inferred by inheritance.
        for(const auto member:group->second.members)shrine_visited_[member].insert(observed->sites.begin(),observed->sites.end());
        if(combined!=active.sites){if(shrine_activation_revision_==UINT64_MAX)throw std::overflow_error("shrine activation revision exhausted");active.sites=std::move(combined);active.revision=++shrine_activation_revision_;}
        active.observed=now;return true;
    }
    void shrine_notice_locked(const shrine::Attempt& wipe,ShrineIssue issue){
        if(shrine_noticed_.insert(wipe.serial).second)shrine_notices_.push_back({wipe,issue});
    }
    // An exact wipe deadline participates in World scheduling. Run the full
    // normal settlement pass at/after it even inside the periodic tick window;
    // recovery itself remains after all timed state and movement settlement.
    bool shrine_recovery_due_locked(std::uint64_t now)const{
        if(!shrine_sites_||shrine_recovering_||now<shrine_clock_)return false;
        for(const auto& [id,group]:shrine_groups_){
            const auto pending=shrine_wipes_.pending(group.party);
            if(pending&&now>=pending->due)return true;
        }
        return false;
    }
    void drive_shrine_recovery_locked(std::uint64_t now){
        if(!shrine_sites_||shrine_recovering_)return;sync_shrine_party_rosters_locked(now);
        for(const auto& [id,p]:players_)if(p.precise_health>0)activate_shrine_member_locked(p.effect_scope,now);
        std::vector<unsigned> groups;for(const auto& [id,g]:shrine_groups_)groups.push_back(id);
        for(const auto id:groups){const auto group=shrine_groups_.find(id);if(group==shrine_groups_.end())continue;
            const auto facts=shrine_snapshot_locked(group->second,now);const auto observed=shrine_wipes_.observe(facts);
            if(observed.code!=shrine::Code::ready||!observed.attempt){
                const auto pending=shrine_wipes_.pending(group->second.party);
                if(pending&&now>=pending->due)shrine_notice_locked(*pending,ShrineIssue::scheduler_not_ready);
                continue;
            }const auto wipe=*observed.attempt;
            const ShrineDeath* last=nullptr;bool eligible=true;
            for(const auto scope:wipe.members){const auto p=players_.find(scope.actor);const auto death=shrine_deaths_.find(scope);
                if(p==players_.end()||p->second.effect_scope!=scope||!p->second.resources||!p->second.resources->max_health||
                   !resurrection_clock_valid(p->second,now)||death==shrine_deaths_.end()||death->second.sequence!=p->second.death_sequence||
                   !valid_shrine_player_revival_locked(p->second,death->second,now)){eligible=false;break;}
                if(!last||death->second.at>last->at||(death->second.at==last->at&&scope.actor<last->owner.actor))last=&death->second;
            }
            if(!eligible||!last){shrine_notice_locked(wipe,ShrineIssue::member_state_unavailable);continue;}
            const auto active=shrine_active_.contains(id)?std::optional(shrine_active_.at(id)):std::nullopt;
            if(!active){shrine_notice_locked(wipe,ShrineIssue::activation_unreviewed);continue;}
            const auto selection=shrine::prepare_site_recovery(*shrine_sites_,*active,wipe,
                {last->owner,last->position,shrine_policy_.selection_policy},shrine_nav_revision_locked());
            if(selection.decision!=shrine::SiteDecision::ready){
                shrine_notice_locked(wipe,selection.decision==shrine::SiteDecision::return_to_last_outpost_required?ShrineIssue::outpost_required:
                    selection.decision==shrine::SiteDecision::catalog_unavailable?ShrineIssue::catalog_unavailable:ShrineIssue::activation_unreviewed);continue;
            }
            const auto plan=*selection.plan;if(!valid_shrine_place_locked(plan.destination)){shrine_notice_locked(wipe,ShrineIssue::destination_invalid);continue;}
            struct PetMove {skills::effects::Scope owner,pet;bool dead=false;combat::npcs::Binding binding;};
            std::vector<PetMove> pet_moves;std::map<unsigned,PetAiController> ai_nodes;
            std::map<unsigned,NpcOrdinaryController> ordinary_nodes;std::map<unsigned,FriendlyOrdinaryBinding> friendly_nodes;
            std::map<skills::effects::Scope,skills::effects::MindWrackEnergy> energy;
            for(const auto scope:wipe.members)energy.emplace(scope,shrine_deaths_.at(scope).energy);
            for(const auto owner:wipe.members){const auto session=player_pet_sessions_.find(owner.actor);
                if(session==player_pet_sessions_.end()||session->second.owner!=owner||!session->second.actor)continue;
                const auto pet=*session->second.actor;auto live=npc_combat_.find(pet.actor);const auto body=npcs_.find(pet.actor);
                if(!valid_pet_owner_locked(owner,pet,false)||live==npc_combat_.end()||body==npcs_.end()||!body->second.combat_definition){shrine_notice_locked(wipe,ShrineIssue::pet_membership_invalid);eligible=false;break;}
                advance_npc_conditions_locked(pet.actor,live->second,now);live=npc_combat_.find(pet.actor);
                if(live==npc_combat_.end()||live->second.hex_scope()!=pet){shrine_notice_locked(wipe,ShrineIssue::pet_lifetime_retired);eligible=false;break;}
                const bool dead=live->second.actor(live->second.lifetime())->health->current<=0;
                if(dead&&(!shrine_policy_.dead_pet_health_percent||!shrine::documented(shrine_policy_.dead_pet_health_evidence))){shrine_notice_locked(wipe,ShrineIssue::dead_pet_policy_missing);eligible=false;break;}
                auto probe=live->second;
                if(dead&&!probe.revive_owned_stalker(probe.lifetime(),body->second.combat_definition->binding,owner,shrine_policy_.dead_pet_health_percent,now)){shrine_notice_locked(wipe,ShrineIssue::pet_revival_rejected);eligible=false;break;}
                auto relocated=body->second;relocated.x=float(plan.destination.x);relocated.y=float(plan.destination.y);relocated.plane=std::uint16_t(plan.destination.plane);
                auto ai=prepare_default_pet_ai_locked(owner,pet,*session->second.durable.pet,relocated,now);if(!ai){shrine_notice_locked(wipe,ShrineIssue::pet_ai_unavailable);eligible=false;break;}
                if(const auto prior=pet_ai_.find(pet.actor);prior!=pet_ai_.end())ai->offensive_completion=prior->second.offensive_completion;
                ai_nodes.emplace(pet.actor,std::move(*ai));ordinary_nodes.emplace(pet.actor,NpcOrdinaryController{pet,combat::ai::ordinary::Ledger(pet,pets::default_stalker_ai(pet,*session->second.durable.pet)->ordinary),{}});
                friendly_nodes.emplace(pet.actor,FriendlyOrdinaryBinding{pet,{}, {},combat::ai::opposition::Side::allied});pet_moves.push_back({owner,pet,dead,body->second.combat_definition->binding});
            }
            if(!eligible)continue;
            if(!shrine::current_site_plan(plan,*shrine_sites_,*active,wipe,shrine_nav_revision_locked())){shrine_notice_locked(wipe,ShrineIssue::site_plan_stale);continue;}
            // Allocate journal/markers before consuming the exact wipe receipt.
            ShrineRecovery recovery{wipe,plan.site,now,now<=UINT64_MAX-5000?now+5000:UINT64_MAX,{}};
            for(const auto& p:pet_moves)recovery.pets.push_back(p.pet);
            shrine_recoveries_.reserve(shrine_recoveries_.size()+1);
            for(const auto member:wipe.members)shrine_protection_.try_emplace(member,0);
            struct Publication {std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;};
            Publication publication;std::set<const Outbox*> seen;
            for(const auto& [id,p]:players_)if(seen.insert(p.outbox.get()).second)publication.recipients.emplace_back(p.outbox,Outbox::DeferredQueue{});
            std::vector<std::unique_ptr<Outbox::Capture>> holds;holds.reserve(publication.recipients.size());
            for(auto& [box,queue]:publication.recipients)holds.push_back(std::make_unique<Outbox::Capture>(*box,queue));
            if(!shrine_wipes_.claim(wipe,shrine_snapshot_locked(group->second,now))){shrine_notice_locked(wipe,ShrineIssue::claim_rejected);continue;}
            shrine_recovering_=true;struct End {bool& flag;~End(){flag=false;}} end{shrine_recovering_};
            // Generic resurrection remains in-place for normal callers. This
            // explicit shrine producer supplies exact death Energy then moves.
            for(const auto member:wipe.members){auto& p=players_.at(member.actor);
                if(!resurrect_locked(p,now,100,0,{},energy.at(member)))throw std::logic_error("prepared shrine player recovery diverged");
                p.x=p.sx=p.tx=float(plan.destination.x);p.y=p.sy=p.ty=float(plan.destination.y);
                p.surface=p.target_surface={plan.destination.plane,plan.destination.cell};
                shrine_protection_.at(member)=recovery.protection_until;
                for(auto& [id,peer]:players_)peer.outbox->enqueue({position_packet(member.actor,p.x,p.y,std::uint16_t(p.surface.plane)),stop_packet(member.actor)});
            }
            for(const auto& moved:pet_moves){auto live=npc_combat_.find(moved.pet.actor);const auto body=npcs_.find(moved.pet.actor);
                if(live==npc_combat_.end()||live->second.hex_scope()!=moved.pet||body==npcs_.end())throw std::logic_error("prepared shrine pet retired");
                cancel_pet_pending_locked(moved.pet,now);stop_pet_motion_locked(moved.pet,now);retire_npc_ordinary_locked(moved.pet,now);
                if(moved.dead&&!live->second.revive_owned_stalker(live->second.lifetime(),moved.binding,moved.owner,shrine_policy_.dead_pet_health_percent,now))throw std::logic_error("prepared shrine pet recovery diverged");
                body->second.x=float(plan.destination.x);body->second.y=float(plan.destination.y);body->second.plane=std::uint16_t(plan.destination.plane);
                pet_ai_.erase(moved.pet.actor);npc_friendly_ordinary_.erase(moved.pet.actor);
                pet_ai_.insert(ai_nodes.extract(moved.pet.actor));npc_ordinary_ai_.insert(ordinary_nodes.extract(moved.pet.actor));npc_friendly_ordinary_.insert(friendly_nodes.extract(moved.pet.actor));
                auto packets=combat::health_packets_2022(*live->second.actor(live->second.lifetime()));
                packets.push_back(emotes::effects_2022(moved.pet.actor,live->second.actor(live->second.lifetime())->effects));
                packets.push_back(position_packet(moved.pet.actor,body->second.x,body->second.y,body->second.plane));packets.push_back(stop_packet(moved.pet.actor));
                packets.push_back(pets::wire::behavior(moved.pet.actor,pets::wire::Behavior::guard));packets.push_back(pets::wire::locked_target(moved.pet.actor,0));
                for(auto& [id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue(packets);
            }
            shrine_recoveries_.push_back(std::move(recovery));refresh_npc_visibility_locked();
            holds.clear();for(auto& [box,queue]:publication.recipients)box->enqueue(std::move(queue.packets));
        }
    }
public:
    bool configure_shrine_sites(unsigned map,unsigned nav_asset,std::span<const shrine::Site> sites,ShrinePolicy policy){
        std::lock_guard lock(mutex_);if(!players_.empty()||!navigation_||navigation_->asset_hash()!=nav_asset||!shrine::documented(policy.selection_policy)||policy.dead_pet_health_percent>100)return false;
        shrine::SiteCatalog catalog(map,nav_asset,policy.fixture_coordinates);
        for(const auto& site:sites)if(catalog.review(site,[&](auto place){return valid_shrine_place_locked(place);})==shrine::Review::rejected)return false;
        shrine_sites_=std::move(catalog);shrine_policy_=std::move(policy);return true;
    }
    bool observe_shrine_entry(skills::effects::Scope member,std::string_view identity,std::uint64_t now){
        std::lock_guard lock(mutex_);return activate_shrine_member_locked(member,now,identity);
    }
    std::vector<ShrineNotice> take_shrine_notices(){std::lock_guard lock(mutex_);auto result=std::move(shrine_notices_);shrine_notices_.clear();return result;}
    std::vector<ShrineRecovery> take_shrine_recoveries(){std::lock_guard lock(mutex_);auto result=std::move(shrine_recoveries_);shrine_recoveries_.clear();return result;}
    std::optional<shrine::Attempt> pending_shrine_wipe(skills::effects::Scope member)const{
        std::lock_guard lock(mutex_);const auto p=players_.find(member.actor);if(p==players_.end()||p->second.effect_scope!=member)return {};
        const auto group=shrine_groups_.find(parties_.group(std::uint16_t(member.actor)));return group==shrine_groups_.end()?std::nullopt:shrine_wipes_.pending(group->second.party);
    }
    // Standard Energy is precise even without an active Overcast curve.
    std::optional<skills::effects::MindWrackEnergy> shrine_current_energy_snapshot(skills::effects::Scope member)const{
        std::lock_guard lock(mutex_);const auto player=players_.find(member.actor);
        if(player==players_.end()||player->second.effect_scope!=member||!player->second.resources)return {};
        return mind_wrack_energy_locked(player->second);
    }
    std::optional<skills::effects::MindWrackEnergy> shrine_death_energy_snapshot(skills::effects::Scope member)const{
        std::lock_guard lock(mutex_);const auto death=shrine_deaths_.find(member);
        return death==shrine_deaths_.end()?std::nullopt:std::optional(death->second.energy);
    }
    bool shrine_protection_active(skills::effects::Scope member,std::uint64_t now)const{
        std::lock_guard lock(mutex_);const auto p=players_.find(member.actor);const auto protection=shrine_protection_.find(member);
        return p!=players_.end()&&p->second.effect_scope==member&&protection!=shrine_protection_.end()&&now<protection->second;
    }
