// Shared exposure for read-only forecasts and real NPC condition settlement.
private:
    struct NpcExternalHealth {
        std::optional<combat::npcs::LiveActor::ExternalHealth> external;
        bool well_energy=false;
    };
    NpcExternalHealth npc_external_health_locked(unsigned id,const combat::npcs::LiveActor& live,std::uint64_t now)const{
        const auto life=live.lifetime();
        std::optional<combat::npcs::LiveActor::ExternalHealth> external;
        bool well_energy=false;
        const auto placed=npcs_.find(id);
        const auto clock=live.health_clock(live.lifetime());
        if(clock&&placed!=npcs_.end()&&stationary_well_npc_locked(id)){
            std::optional<std::uint32_t> negative_caster;
            bool mixed_casters=false;
            for(const auto& field:well_fields_)if((field.plan.health_pips||field.plan.energy_pips)&&
                well_affects_npc_locked(field,placed->second)&&field.begins<now&&field.expires>*clock){
                if(!external)external.emplace();
                external->boundaries.push_back(field.begins);
                external->boundaries.push_back(field.expires);
                well_energy|=field.plan.energy_pips!=0&&live.precise_energy_units(life).has_value();
                if(field.plan.health_pips<0){
                    if(negative_caster&&*negative_caster!=field.caster.actor)mixed_casters=true;
                    negative_caster=field.caster.actor;
                }
            }
            if(external){
                const auto npc=placed->second;
                external->cause=!mixed_casters&&negative_caster?*negative_caster:0;
                external->pips=[this,npc](std::uint64_t at){
                    int pips=0;
                    for(const auto& field:well_fields_)if(field.begins<=at&&at<field.expires&&
                        well_affects_npc_locked(field,npc))pips+=field.plan.health_pips;
                    return std::clamp(pips,-10,10);
                };
                if(well_energy)external->energy_pips=[this,npc](std::uint64_t at){
                    int pips=0;
                    for(const auto& field:well_fields_)if(field.begins<=at&&at<field.expires&&
                        well_affects_npc_locked(field,npc))pips+=field.plan.energy_pips;
                    return std::clamp(pips,-10,10);
                };
            }
        }
        return {std::move(external),well_energy};
    }
