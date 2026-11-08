#pragma once
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
namespace fw::gw::development {
inline bool command(std::u16string_view key){
    return key==u".gm"||key==u"allskills"||key==u"s"||key==u"who"||key==u"die"||key==u"disconnect"||key==u"kill"||key==u"resurrect"||key==u"setname";
}
// Account mode belongs to the server session, not a replaceable world socket.
// Every caller must recheck the current administrator allowlist.
class Modes {
    std::mutex mutex_;
    std::set<std::string,std::less<>> enabled_;
public:
    bool enabled(std::string_view account,bool authorized){
        if(!authorized||account.empty())return false;
        std::lock_guard lock(mutex_);return enabled_.contains(account);
    }
    bool toggle(std::string_view account,bool authorized){
        if(!authorized||account.empty())throw std::invalid_argument("development mode requires an authorized account");
        std::lock_guard lock(mutex_);
        const auto found=enabled_.find(account);
        if(found!=enabled_.end()){enabled_.erase(found);return false;}
        if(enabled_.size()>=1024)throw std::runtime_error("development mode account limit reached");
        enabled_.emplace(account);return true;
    }
};
}
