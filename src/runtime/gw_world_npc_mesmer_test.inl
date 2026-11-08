// Compatibility wrappers over the shared reviewed quest descriptor.
public:
    std::size_t arm_mesmer_test_melee(skills::effects::Scope owner,unsigned cycle,std::uint64_t now){
        return arm_reviewed_owned_ordinary(owner,81,cycle,now);
    }
    std::size_t arm_mesmer_test_wander(skills::effects::Scope owner,unsigned cycle,std::uint64_t now){
        return arm_reviewed_owned_ordinary(owner,81,cycle,now);
    }
    std::size_t arm_warrior_test_melee(skills::effects::Scope owner,unsigned cycle,std::uint64_t now){
        return arm_reviewed_owned_ordinary(owner,85,cycle,now);
    }
    std::size_t arm_warrior_test_wander(skills::effects::Scope owner,unsigned cycle,std::uint64_t now){
        return arm_reviewed_owned_ordinary(owner,85,cycle,now);
    }
