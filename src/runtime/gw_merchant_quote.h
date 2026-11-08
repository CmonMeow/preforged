#pragma once
#include "gw_item_wire.h"
#include <bit>
#include <cmath>
#include <limits>
#include <optional>

namespace fw::gw::merchant {
// Construct from server stock policy, never a transaction request's gold or
// UI fields. Quantity/stackability/availability checks remain with the caller.
struct TrustedScale {float value=1.f;};
struct Quote {std::uint32_t item=0,type=0,unit=0,quantity=0,gold=0;};
inline std::uint64_t nearest_shift(std::uint64_t value,unsigned shift){
 if(!shift)return value;
 const auto base=value>>shift,half=std::uint64_t{1}<<(shift-1),tail=value&((std::uint64_t{1}<<shift)-1);
 return base+(tail>half||(tail==half&&(base&1)));
}
inline std::optional<Quote> priced(const items::Definition& current,unsigned type,unsigned quantity,double rounded_value){
 // Original46c320's checked input range ends at4194303. Quantities on75 are
 // BYTE values; per-stock nonstackability and the ordinary250 cap are separate.
 if(!current.id||!quantity||quantity>255||!std::isfinite(rounded_value)||rounded_value<0||rounded_value>4194303)return {};
 const auto floor=static_cast<std::uint32_t>(std::floor(rounded_value));
 const auto fraction=rounded_value-floor;
 const auto unit=floor+static_cast<unsigned>(fraction>.5||(fraction==.5&&(floor&1)));
 if(unit>std::numeric_limits<std::uint32_t>::max()/quantity)return {};
 return Quote{current.id,type,unit,quantity,unit*quantity};
}
inline std::optional<Quote> buy_quote(const items::Definition& current,TrustedScale scale,unsigned quantity){
 const auto bits=std::bit_cast<std::uint32_t>(scale.value);
 const auto exponent=(bits>>23)&255,mantissa=bits&0x7fffff;
 if((bits>>31)||exponent==255||(!exponent&&!mantissa))return {};
 if(!current.price)return priced(current,1,quantity,0); // captured free Starter Shield
 // Original UI first stores unsigned value asfloat32, then value*scale*2 as
 // float32 before46c320. Round significands with integers so host rounding
 // mode cannot alter either intermediate, including halfway cases.
 const unsigned value_bits=static_cast<unsigned>(std::bit_width(current.price));
 const unsigned value_drop=value_bits>24?value_bits-24:0;
 const auto value=nearest_shift(current.price,value_drop);
 const auto product=value*(exponent?(mantissa|0x800000u):mantissa);
 const unsigned product_bits=static_cast<unsigned>(std::bit_width(product));
 const unsigned product_drop=product_bits>24?product_bits-24:0;
 const auto significand=nearest_shift(product,product_drop);
 const int power=(exponent?static_cast<int>(exponent)-149:-148)+static_cast<int>(value_drop+product_drop);
 const double rounded=std::ldexp(static_cast<double>(significand),power);
 return priced(current,1,quantity,rounded);
}
inline std::optional<Quote> sell_quote(const items::Definition& current,unsigned quantity){
 // Every value in the original rounder's accepted range is exactlyfloat32.
 // Larger values cannot round back into that accepted range.
 return priced(current,11,quantity,static_cast<double>(current.price));
}
}
