#pragma once
#include <cmath>
#include <stdexcept>
#include <cstdint>
#include <map>
#include <set>
#include <string>
namespace capture::drops {
struct Actor {std::uint32_t model=0,allegiance=0;std::string name;bool visible=false,dead=false,alive_observed=false;};
struct Reducer {
 std::uint64_t epoch=0,kills=0,excluded_initial_dead=0,unidentified=0,visibility_losses=0;
 std::map<std::uint32_t,Actor> actors;
 std::map<std::uint32_t,std::uint64_t> model_kills;
 std::map<std::string,std::set<std::uint32_t>> name_models;
 void scope(std::uint64_t e){if(epoch!=e){epoch=e;actors.clear();}}
 void metadata(std::uint32_t id,std::uint32_t model,std::uint32_t allegiance,std::string name){
  auto& a=actors[id];if(a.model&&a.model!=model)a=Actor{};
  a.model=model;a.allegiance=allegiance;a.name=std::move(name);if(!a.name.empty())name_models[a.name].insert(model);
 }
 void observation(std::uint32_t id,std::uint32_t model,bool visible,bool dead,bool status_change){
  auto& a=actors[id];if(a.model&&a.model!=model)a=Actor{};a.model=model;
  if(!visible){++visibility_losses;a.visible=false;a.alive_observed=false;return;}
  if(!status_change||!a.visible){a.visible=true;a.dead=dead;a.alive_observed=!dead;if(dead)++excluded_initial_dead;return;}
  if(!a.dead&&dead&&a.alive_observed){if(a.allegiance==3){++kills;++model_kills[model];if(a.name.empty())++unidentified;}else ++unidentified;}
  a.dead=dead;if(!dead)a.alive_observed=true;
 }
};
inline std::pair<double,double> wilson(std::uint64_t successes,std::uint64_t trials){
 if(!trials||successes>trials)throw std::invalid_argument("invalid binomial trials");
 constexpr double z=1.959963984540054;double n=double(trials),p=double(successes)/n,d=1+z*z/n;
 double c=(p+z*z/(2*n))/d,r=z*std::sqrt(p*(1-p)/n+z*z/(4*n*n))/d;return {c-r,c+r};
}
}

