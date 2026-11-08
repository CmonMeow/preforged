#pragma once
#include "gw_chat_wire.h"
#include "gw_emotes.h"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace fw::gw::chat::commands {
// Native private-server command parsing. Only keywords are folded; names and
// message bodies retain their original UTF-16. Source: local 2021 Commands.html
// SHA256 f902e6fb7b651f9486b0e0c80247cdd6849de48f2bcde8441dd49a7e8347919e.
// /roll has 2..100 sides; authoritative map policy must admit it explicitly.
// Emote.html hash a281bbedb59393681a6588adebfe8dabd9a3cb6b837c2b86480dff55b32e40d6
// describes animations. Only the explicitly traced dance and sitting-state
// branches below have an animation implementation; other names stay explicit.
enum class Kind {
    help, health, experience, age, ip, stuck, invite, kick, leave,
    local, trade, party, whisper, roll, travel, unsupported, unknown, invalid, emote
};
struct External {std::u16string kind,target,message;};
struct Context {
    std::optional<External>* external=nullptr;
    bool roll_allowed=false;
    bool party_changes_allowed=false;
    std::optional<std::uint64_t> map_elapsed_ms;
    std::u16string server_address;
    // Call-owned output, reset on every chat call and never retained by Player.
    // Only an admitted, valid command requests travel. The caller must still
    // run the authoritative map/campaign/navigation/arrival preflight.
    std::optional<std::uint16_t>* travel_destination=nullptr;
    // Same call-owned lifetime; the caller cancels the cast after the world
    // mutex has been released. Never call back into Instance while locked.
    bool* cancel_cast=nullptr;
    std::optional<emotes::Request>* emote_request=nullptr;
};
struct Command {
    Kind kind=Kind::unknown;
    std::u16string keyword,argument,target,message,feedback;
    unsigned number=0;
    std::optional<emotes::Request> emote;
};
inline std::u16string_view trim(std::u16string_view text){
    while(!text.empty()&&text.front()==u' ')text.remove_prefix(1);
    while(!text.empty()&&text.back()==u' ')text.remove_suffix(1);
    return text;
}
inline std::u16string folded(std::u16string_view text){
    std::u16string result(text);
    for(auto& c:result)if(c>=u'A'&&c<=u'Z')c+=u'a'-u'A';
    return result;
}
inline bool same_name(std::u16string_view a,std::u16string_view b){return folded(a)==folded(b);}
inline std::optional<unsigned> number(std::u16string_view text,unsigned maximum){
    if(text.empty()||text.size()>5)return {};
    unsigned value=0;
    for(auto c:text){
        if(c<u'0'||c>u'9')return {};
        const unsigned digit=c-u'0';
        if(value>maximum/10||(value==maximum/10&&digit>maximum%10))return {};
        value=value*10+digit;
    }
    return value;
}
inline Command whisper(std::u16string_view argument){
    Command out;out.keyword=u"whisper";out.argument=trim(argument);
    const auto comma=out.argument.find(u',');
    if(comma!=std::u16string::npos){
        out.target=trim(std::u16string_view(out.argument).substr(0,comma));
        out.message=trim(std::u16string_view(out.argument).substr(comma+1));
    }
    if(!chat::valid_text(argument)||out.target.empty()||out.target.size()>19||!chat::valid_text(out.message)){
        out.kind=Kind::invalid;out.feedback=u"Usage: /whisper <character name>, <message>";
    }else out.kind=Kind::whisper;
    return out;
}
inline Command parse(std::u16string_view body){
    Command out;
    if(!chat::valid_text(body)){out.kind=Kind::invalid;out.feedback=u"Invalid chat command.";return out;}
    body=trim(body);const auto split=body.find(u' ');
    out.keyword=folded(body.substr(0,split));
    if(split!=std::u16string_view::npos)out.argument=trim(body.substr(split+1));
    auto plain=[&](Kind kind,const char16_t* usage){
        if(out.argument.empty())out.kind=kind;
        else {out.kind=Kind::invalid;out.feedback=usage;}
    };
    auto named=[&](Kind kind,const char16_t* usage){
        if(out.argument.empty()||out.argument.size()>19){out.kind=Kind::invalid;out.feedback=usage;}
        else {out.kind=kind;out.target=out.argument;}
    };
    auto message=[&](Kind kind,const char16_t* usage){
        if(!chat::valid_text(out.argument)){out.kind=Kind::invalid;out.feedback=usage;}
        else {out.kind=kind;out.message=out.argument;}
    };
    const auto& key=out.keyword;
    if(key==u"help"||key==u"commands"){
        const auto section=folded(out.argument);
        if(section.empty()||section==u"chat"||section==u"party"||section==u"status")out.kind=Kind::help;
        else {out.kind=Kind::invalid;out.feedback=u"Usage: /commands [chat|party|status]";}
    }else if(key==u"health"||key==u"hp")plain(Kind::health,u"Usage: /health or /hp");
    else if(key==u"xp"||key==u"experience")plain(Kind::experience,u"Usage: /xp");
    else if(key==u"age")plain(Kind::age,u"Usage: /age");
    else if(key==u"ip")plain(Kind::ip,u"Usage: /ip");
    else if(key==u"stuck")plain(Kind::stuck,u"Usage: /stuck");
    else if(key==u"invite")named(Kind::invite,u"Usage: /invite <character name>");
    else if(key==u"kick")named(Kind::kick,u"Usage: /kick <character name>");
    else if(key==u"leave")plain(Kind::leave,u"Usage: /leave");
    else if(key==u"a"||key==u"say")message(Kind::local,u"Usage: /a <message>");
    else if(key==u"trade"||key==u"tr")message(Kind::trade,u"Usage: /trade <message>");
    else if(key==u"p"||key==u"party")message(Kind::party,u"Usage: /p <message>");
    else if(key==u"t"||key==u"tell"||key==u"w"||key==u"whisper"){
        auto parsed=whisper(out.argument);parsed.keyword=out.keyword;out=std::move(parsed);
    }else if(key==u"roll"){
        const auto sides=number(out.argument,100);
        if(!sides||*sides<2){out.kind=Kind::invalid;out.feedback=u"Usage: /roll <sides>, from 2 to 100.";}
        else {out.kind=Kind::roll;out.number=*sides;}
    }else if(key==u"tp"||key==u"travel"){
        const auto map=number(out.argument,65535);
        if(!map||!*map){out.kind=Kind::invalid;out.feedback=u"Usage: /tp <map ID>, from 1 to 65535.";}
        else {out.kind=Kind::travel;out.number=*map;}
    }else if(key==u"dance"||key==u"sit"||key==u"afk"||key==u"stand"){
        const auto action=key==u"dance"?emotes::Action::dance:key==u"stand"?emotes::Action::stand:emotes::Action::sit;
        const bool synchronized=out.argument==u"*";
        if((!out.argument.empty()&&!synchronized)||(synchronized&&action==emotes::Action::stand)){
            out.kind=Kind::invalid;out.feedback=action==emotes::Action::stand?u"Usage: /stand":
                action==emotes::Action::dance?u"Usage: /dance [*]":u"Usage: /sit [*] or /afk [*]";
        }else {out.kind=Kind::emote;out.emote=emotes::Request{action,synchronized};}
    }else {
        out.kind=Kind::unsupported;
        if(key==u"g"||key==u"alliance")out.feedback=u"Guild and alliance chat are not implemented yet.";
        else if(key==u"online"||key==u"away"||key==u"busy"||key==u"offline"||key==u"dnd"||key==u"status")
            out.feedback=u"Friend-list presence is not implemented yet.";
        else if(key==u"deaths")out.feedback=u"Death statistics have not been captured or implemented yet.";
        else if(key==u"favor")out.feedback=u"Favor of the Gods is not simulated yet.";
        else if(key==u"bonus"||key==u"bonusitems"||key==u"preorder"||key==u"cokeplay"||key==u"special")
            out.feedback=u"Bonus item entitlements and item grants are not implemented yet.";
        else if(key==u"resign")out.feedback=u"Mission resign and return-to-outpost are not implemented yet.";
        else if(key==u"namepet"||key==u"petname")out.feedback=u"Pet ownership and naming are not implemented yet.";
        else if(key==u"report")out.feedback=u"Player reports are not implemented yet.";
        else {
            constexpr std::u16string_view emotes[]{u"agree",u"aion",u"wings",u"attention",u"beckon",u"beg",u"boo",u"scare",
                u"bored",u"bow",u"bowhead",u"head",u"catchbreath",u"breath",u"cheer",u"clap",u"congrats",u"cough",
                u"dance",u"dancenew",u"doh",u"doubletake",u"drums",u"drum",u"excited",u"fame",u"rank",u"fistshake",
                u"flex",u"flute",u"goteam",u"encourage",u"guild",u"ladder",u"champion",u"guitar",u"airguitar",u"helpme",
                u"highfive",u"five",u"jump",u"kneel",u"laugh",u"moan",u"no",u"paper",u"pickme",u"point",
                u"ponder",u"pout",u"ready",u"roar",u"rock",u"salute",u"scissors",u"scis",u"scratch",u"shoo",
                u"sigh",u"sit",u"sorry",u"stand",u"taunt",u"violin",u"wave",u"yawn",u"yes"};
            if(std::find(std::begin(emotes),std::end(emotes),key)!=std::end(emotes))
                out.feedback=u"Emote animations are not implemented yet.";
            else {out.kind=Kind::unknown;out.feedback=u"Unknown command. Use /commands for supported commands.";}
        }
    }
    return out;
}
inline std::u16string decimal(std::uint64_t value){const auto text=std::to_string(value);return {text.begin(),text.end()};}
// Player-owned, so disconnect/actor reuse destroys rate/RNG state with the player.
// Eight-message burst; refill one token per250ms. Rejected flood feedback is
// owner-only and no more frequent than once a second. Clock regressions do not
// replenish tokens or throw; messages cannot choose the server's clock.
struct Session {
    unsigned tokens=8;
    std::uint64_t refilled=0,feedback_at=0;
    bool initialized=false,feedback_sent=false;
    std::optional<std::minstd_rand> random;
    bool admit(std::uint64_t now){
        if(!initialized){initialized=true;refilled=now;}
        if(now>=refilled){
            const auto elapsed=now-refilled,earned=elapsed/250;
            if(earned){tokens=std::min(8u,tokens+static_cast<unsigned>(std::min<std::uint64_t>(earned,8)));refilled=now-elapsed%250;}
        }
        if(!tokens)return false;
        --tokens;return true;
    }
    bool flood_feedback(std::uint64_t now){
        if(feedback_sent&&(now<feedback_at||now-feedback_at<1000))return false;
        feedback_sent=true;feedback_at=now;return true;
    }
    unsigned roll(unsigned sides){
        if(sides<2||sides>100)throw std::invalid_argument("invalid dice sides");
        if(!random)random.emplace(std::random_device{}());
        return std::uniform_int_distribution<unsigned>(1,sides)(*random);
    }
};
}
