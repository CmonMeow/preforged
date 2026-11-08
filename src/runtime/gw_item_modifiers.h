#pragma once
#include <cstdint>
#include <span>
#include <optional>
#include <vector>
#include <stdexcept>
namespace fw::gw::items::modifiers {
struct Record {
 std::uint32_t raw;
 std::uint16_t identifier()const{return static_cast<std::uint16_t>(raw>>16);}
 std::uint8_t arg1()const{return static_cast<std::uint8_t>(raw>>8);}
 std::uint8_t arg2()const{return static_cast<std::uint8_t>(raw);}
 std::uint16_t arg()const{return static_cast<std::uint16_t>(raw);}
};
// Raw352 modifiers exclude the native-appended sentinel (7c67b0 proof).
// Opaque fields remain unchanged. This is not an arbitrary upgrade decoder.
inline std::vector<Record> decode_wire(std::span<const std::uint32_t> raw){
 if(raw.size()>64)throw std::invalid_argument("modifier bound exceeded");
 std::vector<Record> records;for(auto value:raw){if(value==0xc0000000u)throw std::invalid_argument("native sentinel is not a wire modifier");records.push_back({value});}return records;
}
inline std::vector<std::uint32_t> capture_to_wire(std::span<const std::uint32_t> captured){
 if(captured.empty()||captured.back()!=0xc0000000u)throw std::invalid_argument("capture lacks proven native sentinel");
 captured=captured.first(captured.size()-1);(void)decode_wire(captured);return {captured.begin(),captured.end()};
}
enum class Kit {identification,expert_salvage,basic_salvage,perfect_salvage};
struct KitInfo {Kit kind;unsigned uses_each;};
// Reviewed public Toolbox InventoryItem.cpp commit54333c43 (not a claim that
// use/salvage gameplay or pinned2022 asset identity is proven). Server requires
// its catalog/source eligibility before treating these decoded fields as kits.
inline std::optional<KitInfo> reviewed_kit(std::span<const Record> records){
 std::optional<unsigned> kind,uses;
 for(auto row:records){if(row.identifier()==0x25e8){if(kind)return {};kind=row.arg1();}if(row.identifier()==0x2458){if(uses)return {};uses=row.arg2();}}
 if(!kind||!uses||!*uses)return {};std::optional<Kit> result;
 switch(*kind){case 1:result=Kit::identification;break;case 2:result=Kit::expert_salvage;break;case 3:result=Kit::basic_salvage;break;case 6:result=Kit::perfect_salvage;break;default:return {};}
 return KitInfo{*result,*uses};
}
}
