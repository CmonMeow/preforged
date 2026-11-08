#pragma once
namespace fw::gw::combat {
// Effective combat ratings include reviewed base armor AND owned skill/item
// contributions. The raw-data admission bound(500) is not a gameplay cap on
// their sum. Keep a separate generous defensive arithmetic bound,not a claim
// about retail armor limits. Existing negative-rating policy is unchanged.
inline constexpr int maximum_effective_armor=1000000;
inline constexpr bool valid_effective_armor(int rating){return rating>=-100&&rating<=maximum_effective_armor;}
}
