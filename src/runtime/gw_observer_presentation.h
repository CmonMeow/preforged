#pragma once
#include "gw_observer_wire.h"
#include "gw_equipment_wire.h"
#include <array>
namespace fw::gw::observer::presentation {
// Attribute30 assigns the native local guild index, not the captured guild
// database ID. Publish guild288 first; native763E40->779E00 refreshes the tag.
inline wire::Packet player_guild(std::uint32_t actor,std::uint16_t local_guild){
 if(!actor||actor>65535||!local_guild)throw std::invalid_argument("invalid observer guild affiliation");
 wire::Packet p{158,0};for(auto value:{30u,actor,std::uint32_t(local_guild)})for(unsigned n=0;n<4;++n)p.push_back(static_cast<std::uint8_t>(value>>(8*n)));return p;
}
// Native37130 descriptorA266F8 [158,404,10,404], callback874B80;
// kind65 ->794810 ->764550 ->77A7F0 sets appearancecache byte5.
// This is distinct from mission428 roster color. Namepalette7786A0 reads
// byte5 when native PvP map flags include40000; colors1/2 are blue/red.
inline wire::Packet team_name_color(std::uint32_t actor,std::uint8_t color){
 if(!actor||actor>65535||!color||color>7)throw std::invalid_argument("invalid observer native name color");
 wire::Packet p{158,0};for(auto value:{65u,actor,std::uint32_t(color)})for(unsigned n=0;n<4;++n)p.push_back(static_cast<std::uint8_t>(value>>(8*n)));return p;
}
// Pinned descriptor60b91c callback8736e0 ->790cf0 ->764720 ->77e750.
// Captured zero interval is absent information: reject it before body actions.
inline wire::Packet attack_timing(std::uint32_t actor,float base_interval,float modifier){if(!actor||actor>65535||!std::isfinite(base_interval)||!std::isfinite(modifier)||base_interval<=0||modifier<=0)throw std::invalid_argument("missing or invalid observed attack timing");wire::Packet p{53,0};for(auto value:{actor,std::bit_cast<std::uint32_t>(base_interval),std::bit_cast<std::uint32_t>(modifier)})for(unsigned n=0;n<4;++n)p.push_back(static_cast<std::uint8_t>(value>>(8*n)));return p;}
struct EquipmentResult {std::vector<wire::Packet> packets;std::vector<std::string> diagnostics;};
inline std::vector<std::string_view> split(std::string_view text,char separator){std::vector<std::string_view> out;std::size_t p=0;for(;;){const auto q=text.find(separator,p);out.push_back(text.substr(p,q==text.npos?q:q-p));if(q==text.npos)return out;p=q+1;}}
inline std::optional<unsigned> unsigned_value(std::string_view text){unsigned v=0;auto r=std::from_chars(text.data(),text.data()+text.size(),v);if(r.ec!=std::errc{}||r.ptr!=text.data()+text.size())return {};return v;}
// Archived snapshot31 counts visual effects; it is not AgentLiving.effects.
// The recorded status booleans reconstruct only documented GWCA masks. Blind
// has no independent verified mask here; its captured conditioned bit remains.
inline std::uint32_t snapshot_effects(const std::vector<std::string>& fields){
 if(fields.size()<22)throw std::invalid_argument("snapshot lacks captured effect booleans");
 auto flag=[&](unsigned index){auto value=unsigned_value(fields[index]);if(!value||*value>1)throw std::invalid_argument("invalid captured effect boolean");return *value!=0;};
 std::uint32_t effects=flag(8)?0x10u:0u;
 constexpr std::array<std::uint32_t,10> masks{2,0x20,1,0xa,0,0x40,0x800,0x400,0x80,0x8000};
 for(unsigned i=0;i<masks.size();++i)if(flag(12+i))effects|=masks[i];return effects;
}
// PA records carry slot:type:composite:model_file:model_files:dyes:tint:composite.
// Model-file lists remain untouched; the pinned client receives only reviewed
// native composite and color fields. Missing armor slots are never filled.
inline EquipmentResult player_equipment(const wire::Event& event){
 EquipmentResult out;if(event.name!="PA"||event.fields.size()!=10){out.diagnostics.push_back("invalid player appearance schema");return out;}
 auto actor=unsigned_value(event.fields[0]);if(!actor||!*actor||*actor>65535){out.diagnostics.push_back("invalid player appearance actor");return out;}
 std::array<bool,5> seen{};
 for(auto piece:split(event.fields[7],'|')){if(piece.empty())continue;const auto f=split(piece,':');if(f.size()!=8){out.diagnostics.push_back("invalid armor piece schema");continue;}
  auto slot=unsigned_value(f[0]),type=unsigned_value(f[1]),composite=unsigned_value(f[2]),tint=unsigned_value(f[6]),repeat=unsigned_value(f[7]);
  if(!slot||!type||!composite||!tint||!repeat){out.diagnostics.push_back("malformed armor identity");continue;}
  unsigned index=5;switch(*slot){case 4:if(*type==16)index=0;break;case 2:if(*type==7)index=1;break;case 6:if(*type==13)index=2;break;case 3:if(*type==19)index=3;break;case 5:if(*type==4)index=4;break;}
  if(index==5||seen[index]){out.diagnostics.push_back("unknown or duplicate armor slot/type");continue;}
  if(!*composite||*composite>=0xedb||*repeat!=*composite||*tint>255){out.diagnostics.push_back("armor composite unavailable or inconsistent for build37130");continue;}
  const auto dyes=split(f[5],',');unsigned color=0;bool valid=dyes.size()==4;for(unsigned n=0;valid&&n<4;++n){auto value=unsigned_value(dyes[n]);if(!value||*value>13)valid=false;else color|=*value<<(4*n);}
  if(!valid){out.diagnostics.push_back("invalid captured armor dye");continue;}
  seen[index]=true;out.packets.push_back(equipment::encode_2022(equipment::Update::composite,*actor,index,*composite|(*tint<<16)));out.packets.push_back(equipment::encode_2022(equipment::Update::color,*actor,index,color));
 }
 return out;
}
}

