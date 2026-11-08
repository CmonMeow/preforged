#pragma once
#include "gw_combat.h"
#include "gw_skill_catalog_generated.h"
namespace fw::gw::skills::particles {
inline combat::Packet positional_effect_2022(std::uint32_t actor,unsigned resource,float x,float y,std::uint16_t plane){
    if(!resource||!std::isfinite(x)||!std::isfinite(y))throw std::invalid_argument("invalid positional effect");
    combat::Packet packet{160,0x80};
    combat::detail::put(packet,std::bit_cast<std::uint32_t>(x));
    combat::detail::put(packet,std::bit_cast<std::uint32_t>(y));
    packet.push_back(static_cast<std::uint8_t>(plane));packet.push_back(static_cast<std::uint8_t>(plane>>8));
    combat::detail::put(packet,actor);combat::detail::put(packet,resource);return packet;
}
// Frozen Spirit Rift completions: caster-owned positional resource1103.
inline combat::Packet spirit_rift_ground_2022(std::uint32_t caster,float x,float y,std::uint16_t plane){
    if(!caster||caster>65535)throw std::invalid_argument("invalid Spirit Rift caster");
    return positional_effect_2022(caster,1103,x,y,plane);
}
// Frozen Death's Charge:1170at the old point with caster identity,then1170
// at the new point with actor0. Body951and actual position update are separate.
inline std::vector<combat::Packet> shadow_step_trail_2022(std::uint32_t caster,float from_x,float from_y,std::uint16_t from_plane,
                                                         float to_x,float to_y,std::uint16_t to_plane){
    if(!caster||caster>65535)throw std::invalid_argument("invalid shadow step caster");
    return {positional_effect_2022(caster,1170,from_x,from_y,from_plane),positional_effect_2022(0,1170,to_x,to_y,to_plane)};
}
inline std::vector<combat::Packet> death_charge_trail_2022(std::uint32_t caster,float from_x,float from_y,std::uint16_t from_plane,
                                                         float to_x,float to_y,std::uint16_t to_plane){
    return shadow_step_trail_2022(caster,from_x,from_y,from_plane,to_x,to_y,to_plane);
}
// Captured Breath of Fire is world-positioned: actor0,resource1191. It must
// not attach to the caster or continue following the selected victim.
inline combat::Packet breath_fire_ground_2022(float x,float y,std::uint16_t plane){
    return positional_effect_2022(0,1191,x,y,plane);
}
// Frozen Maelstrom pulse receipts: actor0,world-positioned resource382.
inline combat::Packet meteor_ground_2022(unsigned skill,float x,float y,std::uint16_t plane){
    if(skill!=187&&skill!=192)throw std::invalid_argument("invalid meteor field");
    return positional_effect_2022(0,skill==187?335:341,x,y,plane);
}
inline combat::Packet eruption_ground_2022(std::uint32_t caster,float x,float y,std::uint16_t plane){
    if(!caster)throw std::invalid_argument("invalid Eruption caster");
    return positional_effect_2022(caster,300,x,y,plane);
}
inline combat::Packet maelstrom_ground_2022(float x,float y,std::uint16_t plane){
    return positional_effect_2022(0,382,x,y,plane);
}
inline combat::Packet fire_storm_ground_2022(float x,float y,std::uint16_t plane){
    return positional_effect_2022(0,350,x,y,plane);
}
// Frozen Ward938 completion: caster body1187 is a separate GenericValue;
// ground-owned positional356 remains at the original ward centre.
inline combat::Packet ward_stability_ground_2022(float x,float y,std::uint16_t plane){
    return positional_effect_2022(0,356,x,y,plane);
}
// Frozen Ward239 completion: ground-owned356 then caster-owned900.
// Generic body420 and timer publication are separate consumers.
inline std::vector<combat::Packet> ward_harm_ground_2022(std::uint32_t actor,float x,float y,std::uint16_t plane){
    if(!actor||actor>65535)throw std::invalid_argument("invalid Ward Against Harm caster");
    return {positional_effect_2022(0,356,x,y,plane),positional_effect_2022(actor,900,x,y,plane)};
}
// Frozen Snow Storm2222 completion: caster-owned1956 then ground-owned381,
// same target position/plane, both optional descriptor words zero.
inline std::vector<combat::Packet> snow_storm_ground_2022(std::uint32_t actor,float x,float y,std::uint16_t plane){
    if(!actor||actor>65535)throw std::invalid_argument("invalid Snow Storm caster");
    return {positional_effect_2022(actor,1956,x,y,plane),positional_effect_2022(0,381,x,y,plane)};
}
// Lava Font completion capture: positional resources348/349. Pinned2022
// descriptor60be08 serializes coordinates,16-bit plane,actor and resource;
// both optional data words are zero and omitted (header flag0x8000).
inline std::vector<combat::Packet> lava_font_ground_2022(std::uint32_t actor,float x,float y,std::uint16_t plane){
    if(!actor||!std::isfinite(x)||!std::isfinite(y))throw std::invalid_argument("invalid Lava Font ground effect");
    std::vector<combat::Packet> result;
    for(unsigned resource:{348u,349u}){
        result.push_back(positional_effect_2022(actor,resource,x,y,plane));
    }
    return result;
}
// Frozen successful Well91..94 completions each show a ground-owned primary
// resource, ground-owned174, then caster-owned900 at the same position.
// The first recorded Well94 completion lacked the trio; a later cast has it.
inline std::vector<combat::Packet> well_ground_2022(unsigned skill,std::uint32_t caster,
                                                   float x,float y,std::uint16_t plane){
    if((skill<91||skill>94)||!caster||caster>65535)
        throw std::invalid_argument("unsupported Well ground effect");
    const unsigned primary=skill==91?173:skill==92?175:skill==93?176:177;
    return {positional_effect_2022(0,primary,x,y,plane),
            positional_effect_2022(0,174,x,y,plane),
            positional_effect_2022(caster,900,x,y,plane)};
}
// Official capture 2026-10-14: modern70 carries float16, float0.25 and
// the affected actor. Pinned2022 descriptor60b9dc uses opcode69 instead.
// The client supplies distance attenuation; do not tune it again server-side.
// Plain header:0x8000 is serializer connection state,not a server direction bit.
// Live build37130 disconnects on the flagged form;plain69 accepts the shake.
inline combat::Packet aftershock_screen_shake_2022(std::uint32_t actor){
    if(!actor)throw std::invalid_argument("camera shake actor required");
    combat::Packet packet{69,0};
    combat::detail::put(packet,std::bit_cast<std::uint32_t>(16.f));
    combat::detail::put(packet,std::bit_cast<std::uint32_t>(.25f));
    combat::detail::put(packet,actor);return packet;
}
// Exact pinned native skill-row target-body resources. Kind21 attaches the
// reviewed particle to the victim actor; it is not the skill ID or cast pose.
inline std::optional<combat::Packet> target_body_2022(std::uint32_t actor,unsigned skill){
    const auto* row=find_skill(skill);
    if(!actor||!row||row->target_body_resource>=2077)return {};
    return combat::detail::integer(21,actor,row->target_body_resource);
}
// Original targeted GenericValue routes the particle with both victim and
// source identity. Use only when its captured target-event form is reviewed.
inline std::optional<combat::Packet> targeted_body_2022(std::uint32_t target,std::uint32_t caster,unsigned skill){
    const auto* row=find_skill(skill);
    // Captured target effects override unused row sentinels: Agony260
    // (including PvP), and Teinai's Wind288.
    if(target&&caster&&(skill==145||skill==3059||skill==1081||skill==571)){
        combat::Packet packet{159,0};combat::detail::put(packet,20);combat::detail::put(packet,target);
        combat::detail::put(packet,caster);combat::detail::put(packet,skill==571?1142u:skill==1081?288u:260u);return packet;
    }
    if(!target||!caster||!row||row->target_body_resource>=2077)return {};
    combat::Packet packet{159,0};combat::detail::put(packet,20);
    combat::detail::put(packet,target);combat::detail::put(packet,caster);
    combat::detail::put(packet,row->target_body_resource);return packet;
}
// Pinned 2022 native kind20 consumer accepts resource1019 on each Ray of
// Judgment victim. This differs from the skill-row completion body1018.
inline combat::Packet ray_of_judgment_pulse_2022(std::uint32_t target,std::uint32_t caster){
    if(!target||!caster||target>65535||caster>65535)throw std::invalid_argument("invalid Ray pulse actors");
    combat::Packet packet{159,0};combat::detail::put(packet,20);
    combat::detail::put(packet,target);combat::detail::put(packet,caster);
    combat::detail::put(packet,1019);return packet;
}
// Frozen Arc Lightning completion publishes an additional target event using
// resource989 after its metadata target resource988. Keep it separate from the
// standard row lookup so no other lightning spell inherits that particle.
inline std::optional<combat::Packet> arc_lightning_chain_2022(std::uint32_t target,std::uint32_t caster){
    if(!target||!caster||target>65535||caster>65535)return {};
    combat::Packet packet{159,0};combat::detail::put(packet,20);
    combat::detail::put(packet,target);combat::detail::put(packet,caster);
    combat::detail::put(packet,989);return packet;
}
// Frozen Water hex receipts: Ice Spikes375,Frozen Burst377,Deep Freeze414.
inline combat::Packet water_hex_target_2022(std::uint32_t target,std::uint32_t caster,unsigned skill){
    if(!target||!caster||target>65535||caster>65535||(skill!=211&&skill!=212&&skill!=234))throw std::invalid_argument("invalid Water hex particle");
    combat::Packet packet{159,0};combat::detail::put(packet,20);combat::detail::put(packet,target);combat::detail::put(packet,caster);
    combat::detail::put(packet,skill==211?375u:skill==212?377u:414u);return packet;
}
// Whirlwind163 completion capture: targeted20/resource288 per affected foe.
// Native target-body is2077, so this comes from the reviewed packet receipt.
inline combat::Packet whirlwind_target_2022(std::uint32_t target,std::uint32_t caster){
    if(!target||!caster||target>65535||caster>65535)throw std::invalid_argument("invalid Whirlwind actors");
    combat::Packet packet{159,0};combat::detail::put(packet,20);
    combat::detail::put(packet,target);combat::detail::put(packet,caster);combat::detail::put(packet,288);return packet;
}
// Isolated Orb229 impact: targeted20/resource404; native row is2077.
inline combat::Packet lightning_orb_impact_2022(std::uint32_t target,std::uint32_t caster){
    if(!target||!caster||target>65535||caster>65535)throw std::invalid_argument("invalid Orb impact actors");
    combat::Packet packet{159,0};combat::detail::put(packet,20);
    combat::detail::put(packet,target);combat::detail::put(packet,caster);combat::detail::put(packet,404);return packet;
}
// Lightning Bolt1369 frozen targeted impact resource1628; distinct from Orb.
inline combat::Packet lightning_bolt_impact_2022(std::uint32_t target,std::uint32_t caster){
    if(!target||!caster||target>65535||caster>65535)throw std::invalid_argument("invalid Bolt impact actors");
    combat::Packet packet{159,0};combat::detail::put(packet,20);
    combat::detail::put(packet,target);combat::detail::put(packet,caster);combat::detail::put(packet,1628);return packet;
}
// Shock Arrow1082 frozen targeted20/resource1181; native+0x84 agrees.
inline combat::Packet shock_arrow_impact_2022(std::uint32_t target,std::uint32_t caster){
    if(!target||!caster||target>65535||caster>65535)throw std::invalid_argument("invalid Shock Arrow impact actors");
    combat::Packet packet{159,0};combat::detail::put(packet,20);
    combat::detail::put(packet,target);combat::detail::put(packet,caster);combat::detail::put(packet,1181);return packet;
}
// Three frozen Stone Daggers casts use targeted20/resource306 per hit.
inline combat::Packet stone_dagger_impact_2022(std::uint32_t target,std::uint32_t caster){
    if(!target||!caster||target>65535||caster>65535)throw std::invalid_argument("invalid Stone Daggers actors");
    combat::Packet packet{159,0};combat::detail::put(packet,20);combat::detail::put(packet,target);
    combat::detail::put(packet,caster);combat::detail::put(packet,306);return packet;
}
// Two fixed Dancing Daggers casts show targeted20/resource855 at each of
// three impacts. The pinned 2022 particle consumer accepts this resource.
inline combat::Packet dancing_dagger_impact_2022(std::uint32_t target,std::uint32_t caster){
    if(!target||!caster||target>65535||caster>65535)throw std::invalid_argument("invalid Dancing Daggers actors");
    combat::Packet packet{159,0};combat::detail::put(packet,20);combat::detail::put(packet,target);
    combat::detail::put(packet,caster);combat::detail::put(packet,855);return packet;
}
// AgentProjectileLaunched163,original descriptor60be2c. Orb229 captures
// select model403,flag1,is_attack0. Duration follows the owned flight plan;
// never reuse the short captured duration for a different destination.
inline combat::Packet spell_projectile_launch_2022(std::uint32_t caster,float x,float y,std::uint16_t plane,float seconds,unsigned skill){
    // Modern official opcode164 receipts for Disrupting Dagger select model1141.
    // The pinned 2022 client consumes this older opcode163 descriptor instead.
    const unsigned model=skill==571?1141u:skill==172?306u:skill==858?854u:(skill==229||skill==1369)?403u:skill==1082?1179u:skill==213?378u:skill==214?379u:skill==237?417u:0u;
    if(!caster||caster>65535||!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(seconds)||seconds<=0||seconds>30)
        throw std::invalid_argument("invalid spell projectile launch");
    if(!model)throw std::invalid_argument("unreviewed spell projectile model");
    combat::Packet packet{163,0x80};combat::detail::put(packet,caster);
    combat::detail::put(packet,std::bit_cast<std::uint32_t>(x));combat::detail::put(packet,std::bit_cast<std::uint32_t>(y));
    packet.push_back(static_cast<std::uint8_t>(plane));packet.push_back(static_cast<std::uint8_t>(plane>>8));
    combat::detail::put(packet,std::bit_cast<std::uint32_t>(seconds));combat::detail::put(packet,model);
    combat::detail::put(packet,1);packet.push_back(0);return packet;
}
// Frozen Spear of Light1130 releases use model1266,is_attack0 and varying
// opaque synchronization keys. The caller allocates a live-flight key.
inline combat::Packet spear_light_launch_2022(std::uint32_t caster,float x,float y,std::uint16_t plane,float seconds,std::uint32_t synchronization){
    if(!caster||caster>65535||!synchronization||!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(seconds)||seconds<=0||seconds>30)
        throw std::invalid_argument("invalid Spear of Light projectile launch");
    combat::Packet packet{163,0x80};combat::detail::put(packet,caster);
    combat::detail::put(packet,std::bit_cast<std::uint32_t>(x));combat::detail::put(packet,std::bit_cast<std::uint32_t>(y));
    packet.push_back(static_cast<std::uint8_t>(plane));packet.push_back(static_cast<std::uint8_t>(plane>>8));
    combat::detail::put(packet,std::bit_cast<std::uint32_t>(seconds));combat::detail::put(packet,1266);
    combat::detail::put(packet,synchronization);packet.push_back(0);return packet;
}
// Original descriptor60be2c;model143 and is_attack1 are present in the
// captured six-arrow Barrage volley. synchronization is an opaque trigger key.
inline combat::Packet bow_projectile_launch_2022(std::uint32_t caster,float x,float y,std::uint16_t plane,float seconds,std::uint32_t synchronization){
    if(!caster||caster>65535||!synchronization||!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(seconds)||seconds<=0||seconds>30)
        throw std::invalid_argument("invalid bow projectile launch");
    combat::Packet packet{163,0x80};combat::detail::put(packet,caster);
    combat::detail::put(packet,std::bit_cast<std::uint32_t>(x));combat::detail::put(packet,std::bit_cast<std::uint32_t>(y));
    packet.push_back(static_cast<std::uint8_t>(plane));packet.push_back(static_cast<std::uint8_t>(plane>>8));
    combat::detail::put(packet,std::bit_cast<std::uint32_t>(seconds));combat::detail::put(packet,143);
    combat::detail::put(packet,synchronization);packet.push_back(1);return packet;
}
inline combat::Packet lightning_orb_launch_2022(std::uint32_t caster,float x,float y,std::uint16_t plane,float seconds){
    return spell_projectile_launch_2022(caster,x,y,plane,seconds,229);
}
// Unholy Feast capture: generic kind20/resource210 on each victim;
// caster kind21/resource209 is published by the normal completion path.
inline std::optional<combat::Packet> life_steal_target_2022(std::uint32_t actor,unsigned skill,std::uint32_t caster=0){
    if((skill==153||skill==902||skill==1076||skill==2189)&&caster)return targeted_body_2022(actor,caster,skill);
    if(skill!=110)return target_body_2022(actor,skill);
    if(!actor)return {};
    return combat::detail::integer(20,actor,210);
}
// Shadow of Fear capture: targeted kind20, affected actor first, caster
// second, native resource247. Preserve the source for the target particle.
// Chilblains frozen completion emits targeted resource257 on each damaged foe,
// although the catalog's target body field is the2077 sentinel.
inline combat::Packet chilblains_target_2022(std::uint32_t target,std::uint32_t caster){
    if(!target||target>65535||!caster||caster>65535)throw std::invalid_argument("Chilblains actors required");
    combat::Packet packet{159,0};combat::detail::put(packet,20);
    combat::detail::put(packet,target);combat::detail::put(packet,caster);
    combat::detail::put(packet,257);return packet;
}
inline combat::Packet shadow_of_fear_target_2022(std::uint32_t target,std::uint32_t caster){
    if(!target||!caster)throw std::invalid_argument("Shadow of Fear actors required");
    combat::Packet packet{159,0};combat::detail::put(packet,20);
    combat::detail::put(packet,target);combat::detail::put(packet,caster);
    combat::detail::put(packet,247);return packet;
}
inline combat::Packet health_change_2022(std::uint32_t actor,std::uint32_t source,double amount,unsigned maximum){
    if(!actor||!source||!maximum||!std::isfinite(amount))throw std::invalid_argument("invalid health change");
    combat::Packet packet{162,0};combat::detail::put(packet,55);combat::detail::put(packet,actor);combat::detail::put(packet,source);
    combat::detail::put(packet,std::bit_cast<std::uint32_t>(static_cast<float>(amount/maximum)));return packet;
}
// Modern Summon Spirits2100 applied kind42/resource426 to Signet spirits and
// resource459 to one other owned spirit. Pinned2022 targeted kind20 consumes
// both resources; this is the reviewed form, not the row's caster body1163.
inline combat::Packet summon_spirit_target_2022(std::uint32_t spirit,std::uint32_t caster,unsigned resource){
    if(!spirit||!caster||(resource!=426&&resource!=459))throw std::invalid_argument("invalid Summon Spirits target");
    combat::Packet packet{159,0};combat::detail::put(packet,20);
    combat::detail::put(packet,spirit);combat::detail::put(packet,caster);
    combat::detail::put(packet,resource);return packet;
}
// Infuse Health: targeted kind20/resource517 preserves the healing source.
inline combat::Packet infuse_health_target_2022(std::uint32_t target,std::uint32_t caster){
    if(!target||!caster)throw std::invalid_argument("Infuse Health actors required");
    combat::Packet packet{159,0};combat::detail::put(packet,20);
    combat::detail::put(packet,target);combat::detail::put(packet,caster);
    combat::detail::put(packet,517);return packet;
}
// Captured Midnight kind20 carries affected actor, source actor, resource104.
// Pinned2022 targeted generic event is159 (modern160).
inline combat::Packet midnight_target_2022(std::uint32_t target,std::uint32_t caster){
    if(!target||!caster)throw std::invalid_argument("Midnight actors required");
    combat::Packet packet{159,0};combat::detail::put(packet,20);
    combat::detail::put(packet,target);combat::detail::put(packet,caster);
    combat::detail::put(packet,find_skill(58)->target_body_resource);return packet;
}

}
