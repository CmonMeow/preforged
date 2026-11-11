#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>

namespace packet_capture {
struct WindowItemsLayout {std::uint32_t opcode;static constexpr std::size_t size=72,capacity=16;};
// Exact [opcode, ARRAY16, DWORD-element] schema. Pinned2022 descriptor
// RVA60bcac/callback8746a0 reads count+4 and inline DWORD IDs+8. Modern132
// retains this schema; other arrays remain unsupported. This is decoded
// client memory, not the serialized packet's variable count/element widths.
inline std::optional<WindowItemsLayout> window_items_layout(std::span<const std::uint32_t> schema){
 if(schema.size()!=3||(schema[0]!=131&&schema[0]!=132)||schema[1]!=0x102b||schema[2]!=6)return {};
 return WindowItemsLayout{schema[0]};
}
template<class Read> bool copy_window_items(const WindowItemsLayout& layout,const void* source,
 std::span<unsigned char> destination,Read read){
 if(!source||destination.size()<layout.size||(layout.opcode!=131&&layout.opcode!=132))return false;
 std::fill(destination.begin(),destination.end(),0);
 if(!read(destination.data(),source,8))return false;
 std::uint32_t opcode=0,count=0;
 std::memcpy(&opcode,destination.data(),4);std::memcpy(&count,destination.data()+4,4);
 if(opcode!=layout.opcode||count>layout.capacity)return false;
 return !count||read(destination.data()+8,static_cast<const unsigned char*>(source)+8,count*4);
}
}
