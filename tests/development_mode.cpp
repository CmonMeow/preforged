#include "gw_development_mode.h"
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("development mode lifecycle regression");}
int main(){
    for(const auto key:{u".gm",u"allskills",u"s",u"who",u"die",u"disconnect",u"kill",u"resurrect"})check(fw::gw::development::command(key));
    for(const auto key:{u".dev",u".allskills",u".kill",u".resurrect",u".who",u".die",u".s",u"guild",u"trade"})check(!fw::gw::development::command(key));
    fw::gw::development::Modes server;
    check(!server.enabled("owner",true));
    check(server.toggle("owner",true));
    // Replacing a world connection after death or travel reads the same mode.
    check(server.enabled("owner",true)&&server.enabled("owner",true));
    check(!server.enabled("other",true)&&!server.enabled("owner",false));
    bool denied=false;try{server.toggle("owner",false);}catch(const std::invalid_argument&){denied=true;}
    check(denied&&server.enabled("owner",true));
    check(!server.toggle("owner",true)&&!server.enabled("owner",true));
    fw::gw::development::Modes restarted;
    check(!restarted.enabled("owner",true));
}
