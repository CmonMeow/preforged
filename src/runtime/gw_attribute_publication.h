#pragma once
#include "gw_attributes.h"
#include <optional>
namespace fw::gw::attributes {
// A world connection is considered loaded before the final134 spawn exchange.
// Attribute state is a separate native object:55 creates it,182 establishes
// its profession, and only then may58 evaluate ranks. Keep that phase explicit.
class Publication {
 bool native_ready_=false;
 std::optional<Bonuses> published_;
public:
 void initialize(const Bonuses& initial){native_ready_=true;published_=initial;}
 void reset(){native_ready_=false;published_.reset();}
 bool required(bool world_loaded,const Bonuses& current)const{
  return world_loaded&&native_ready_&&(!published_||*published_!=current);
 }
 void published(const Bonuses& current){
  if(!native_ready_)throw std::logic_error("attributes published before native initialization");
  published_=current;
 }
};
}
