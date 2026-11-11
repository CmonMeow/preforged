#pragma once
#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
namespace packet_capture {
struct ItemLayout {std::size_t name,count,modifiers,size;std::uint32_t opcode;};
// Exact schemas and inline offsets verified in the hash-pinned modern client:
// 847ba0/847c80 use name+30, count+b0, modifiers+b4. Detail uses +10/+90/+94.
// The final404 describes ARRAY elements; it is not a trailing scalar field.
inline std::optional<ItemLayout> item_layout(std::span<const std::uint32_t> schema){
 constexpr std::array<std::uint32_t,15> general{353,0x404,0x404,0x104,0x104,0x204,0x204,0x104,0x404,0x404,0x404,0x404,0x4017,0x400c,0x404};
 constexpr std::array<std::uint32_t,7> detail{352,0x404,0x404,0x404,0x4017,0x400c,0x404};
 if(schema.size()==general.size()&&(schema[0]==353||schema[0]==354)){
  for(std::size_t i=1;i<schema.size();++i)if(schema[i]!=general[i])return {};
  return ItemLayout{0x30,0xb0,0xb4,0x1b4,schema[0]};
 }
 if(schema.size()==detail.size()){
  for(std::size_t i=0;i<schema.size();++i)if(schema[i]!=detail[i])return {};
  return ItemLayout{0x10,0x90,0x94,0x194,352};
 }
 return {};
}
// Copy only used inline modifiers. Zero unused capacity/name padding instead
// of exposing stale packet-buffer bytes. No client pointers are dereferenced.
template<class Read> bool copy_item(const ItemLayout& layout,const void* source,
 std::span<unsigned char> destination,Read read){
 if(!source||destination.size()<layout.size||layout.modifiers!=layout.count+4||
    layout.count!=layout.name+128||layout.size!=layout.modifiers+256)return false;
 std::fill(destination.begin(),destination.end(),0);
 if(!read(destination.data(),source,layout.modifiers))return false;
 std::uint32_t opcode=0;std::memcpy(&opcode,destination.data(),4);
 if(opcode!=layout.opcode)return false;
 std::uint32_t count=0;std::memcpy(&count,destination.data()+layout.count,4);
 if(count>64)return false;
 if(count&&!read(destination.data()+layout.modifiers,
    static_cast<const unsigned char*>(source)+layout.modifiers,count*4))return false;
 for(std::size_t i=0;i<64;++i){
  std::uint16_t value=0;std::memcpy(&value,destination.data()+layout.name+i*2,2);
  if(!value){std::fill(destination.begin()+layout.name+i*2,destination.begin()+layout.count,0);break;}
 }
 return true;
}
}
