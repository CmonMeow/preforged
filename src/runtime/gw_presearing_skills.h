#pragma once
#include <array>
#include <string_view>
namespace fw::gw::presearing {
// Local Guild Wars Wiki skill availability; numeric identities from matching GWCA enum.
// Source: List_of_Ascalon_(pre-Searing)_skills.html
// SHA256 c12cb288b257c091691bf0e5ef415dd8db44aa6fae65f9a68aadefea49ec0d25
struct Skill {unsigned id;std::string_view name;};
inline constexpr std::array<Skill,44> skills{{
    {330u,"Cyclone Axe"},
    {336u,"Executioner's Strike"},
    {346u,"Frenzy"},
    {384u,"Gash"},
    {331u,"Hammer Bash"},
    {1u,"Healing Signet"},
    {382u,"Sever Artery"},
    {411u,"Charm Animal"},
    {436u,"Comfort Animal"},
    {396u,"Dual Shot"},
    {431u,"Ignite Arrows"},
    {407u,"Point Blank Shot"},
    {394u,"Power Shot"},
    {432u,"Read the Wind"},
    {446u,"Troll Unguent"},
    {296u,"Bane Signet"},
    {252u,"Banish"},
    {288u,"Healing Breeze"},
    {281u,"Orison of Healing"},
    {248u,"Retribution"},
    {307u,"Reversal of Fortune"},
    {299u,"Shielding Hands"},
    {247u,"Symbol of Wrath"},
    {83u,"Animate Bone Horror"},
    {115u,"Blood Renewal"},
    {105u,"Deathly Swarm"},
    {135u,"Faintheartedness"},
    {109u,"Life Siphon"},
    {100u,"Soul Barbs"},
    {153u,"Vampiric Gaze"},
    {156u,"Vampiric Touch"},
    {28u,"Backfire"},
    {31u,"Conjure Phantasm"},
    {26u,"Empathy"},
    {40u,"Ether Feast"},
    {76u,"Imagined Burden"},
    {27u,"Shatter Delusions"},
    {180u,"Aura of Restoration"},
    {220u,"Blinding Flash"},
    {197u,"Fire Storm"},
    {194u,"Flare"},
    {200u,"Glyph of Lesser Energy"},
    {230u,"Lightning Javelin"},
    {2u,"Resurrection Signet"},
}};
inline constexpr bool obtainable_skill(unsigned id){for(const auto& skill:skills)if(skill.id==id)return true;return false;}
}
