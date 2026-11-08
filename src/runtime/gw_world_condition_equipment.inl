private:
    struct ConditionEquipmentObservation {
        std::optional<equipment::ConditionEquipment> equipment;
        std::uint64_t at=0,revision_watermark=0;
    };
    std::map<skills::effects::Scope,ConditionEquipmentObservation> condition_equipment_;
    std::optional<std::uint64_t> condition_equipment_duration_locked(skills::effects::Scope caster,
            skills::effects::Scope target,combat::conditions::Kind kind,std::uint64_t base,std::uint64_t now)const{
        const auto source=condition_equipment_.find(caster);
        if(source==condition_equipment_.end()||!source->second.equipment||source->second.at>now)return {};
        if(kind!=combat::conditions::Kind::poison&&kind!=combat::conditions::Kind::weakness&&kind!=combat::conditions::Kind::disease)return {};
        const auto index=unsigned(kind);
        const bool extension=source->second.equipment->bonuses().extension[index];
        // Disease has reviewed reduction gear, but no reviewed extension prefix.
        if(extension&&kind==combat::conditions::Kind::disease)return {};
        combat::conditions::EquipmentReductions reductions;
        if(const auto p=players_.find(target.actor);p!=players_.end()){
            if(p->second.effect_scope!=target)return {};
            const auto gear=condition_equipment_.find(target);
            if(gear==condition_equipment_.end()||!gear->second.equipment||gear->second.at>now)return {};
            reductions={gear->second.equipment->bonuses().rune[index],gear->second.equipment->bonuses().inscription[index]};
        }
        // Combined extension/reduction order has no reviewed capture yet.
        if(extension&&(reductions.rune||reductions.inscription))return {};
        const auto duration=extension?combat::conditions::prefix_extended_duration(base):
            combat::conditions::equipment_reduced_duration(base,reductions);
        if(!duration||!*duration||now>UINT64_MAX-*duration)return {};return duration;
    }
    std::optional<std::uint64_t> poison_equipment_duration_locked(skills::effects::Scope caster,
            skills::effects::Scope target,std::uint64_t base,std::uint64_t now)const{
        return condition_equipment_duration_locked(caster,target,combat::conditions::Kind::poison,base,now);
    }
public:
    bool observe_condition_equipment(skills::effects::Scope scope,
            std::optional<equipment::ConditionEquipment> equipment,std::uint64_t now){
        std::lock_guard lock(mutex_);
        const auto p=players_.find(scope.actor);
        if(p==players_.end()||p->second.effect_scope!=scope||now<p->second.health_updated||now<p->second.energy_updated)return false;
        const auto old=condition_equipment_.find(scope);
        const auto watermark=old==condition_equipment_.end()?0:old->second.revision_watermark;
        if(old!=condition_equipment_.end()&&now<old->second.at)return false;
        if(equipment&&equipment->revision()<watermark)return false;
        const auto revision=equipment?equipment->revision():watermark;
        if(!settle_timed_effects_locked(now))return false;
        std::erase_if(condition_equipment_,[&](const auto& entry){const auto found=players_.find(entry.first.actor);
            return found==players_.end()||found->second.effect_scope!=entry.first;});
        // Unknown equipment invalidates bonuses, not the durable revision fence.
        condition_equipment_[scope]={std::move(equipment),now,revision};return true;
    }
