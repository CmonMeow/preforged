#pragma once
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace fw::gw::combat::deep_wound {
// Maximum health is integral on the pinned client. Fractional reductions use
// floor here as an explicit server policy; the cap and 20% rule are documented
// in the local pre-Reforged Deep_Wound/Healing/Health wiki references.
struct State {
    unsigned base=0, reduction=0;
    double debt=0;
    bool active=false;
    unsigned maximum()const{return base-reduction;}
    void reset(unsigned capacity){base=capacity;reduction=0;debt=0;active=false;}
    bool capacity(double& health,unsigned capacity,bool wound){
        if(!capacity)throw std::invalid_argument("zero health capacity");
        if(!base)reset(capacity);
        if(base==capacity&&active==wound)return false;
        const auto old=maximum();const auto loss=wound?std::min(100u,capacity/5):0u;
        const auto next=capacity-loss;
        const bool changed=old!=next;
        if(health>0){
            // Equipment changes preserve missing *unwounded* health with the
            // documented nonlethal floor. Only the wound owns lethal debt.
            const auto unwounded=std::clamp(health-debt+reduction+double(capacity)-base,1.,double(capacity));
            const double latent=unwounded-loss;
            health=std::clamp(latent,1.,double(next));debt=health-latent;
        }else debt=0;
        base=capacity;reduction=loss;active=wound;return changed;
    }
    // Only real health gain/loss settles a pending lethal wound; regeneration
    // and capacity changes cannot kill by applying the condition alone.
    void event(double& health,double delta,bool healing=false,double healing_multiplier=1.){
        if(!std::isfinite(healing_multiplier)||healing_multiplier<=0||healing_multiplier>1)throw std::invalid_argument("invalid received healing multiplier");
        if(health<=0||delta==0)return;
        if(healing&&delta>0)delta*=std::max(.6,healing_multiplier*(active?.8:1.));
        health=std::clamp(health-debt+delta,0.,double(maximum()));debt=0;
    }
    void regeneration(double& health,double delta){
        if(health<=0||delta==0)return;
        if(!debt){health=std::clamp(health+delta,0.,double(maximum()));return;}
        if(delta<0&&health+delta<=0){health=0;debt=0;return;}
        const auto latent=health-debt+delta;
        health=std::clamp(latent,1.,double(maximum()));debt=health-latent;
    }
};
}
