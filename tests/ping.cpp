#include "gw_ping.h"
#include <stdexcept>
void check(bool v){if(!v)throw std::runtime_error("ping challenge regression");}
int main(){fw::gw::PingChallenge ping;
 check(!ping.complete(100));check(ping.begin(100));check(!ping.begin(110));
 check(ping.complete(163)==63);check(!ping.complete(164));
 check(ping.begin(200));check(ping.complete(200)==1);
 check(ping.begin(300));check(!ping.complete(5301));
 check(ping.begin(6000));check(ping.begin(11001));check(ping.complete(11050)==49);
 check(ping.begin(12000));check(!ping.complete(11999));
}
