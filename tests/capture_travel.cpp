#include "../tools/capture/travel_command.h"
#include "../tools/capture/travel_request.h"
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("collector travel command regression");}
int main(){
    std::array<std::uint8_t,32> native={0x8b,0x45,0x0c,0x89,0x45,0xf0,0x8b,0x45,0x10,0x89,0x45,0xf4,0x8b,0x45,0x14,0x89,0x45,0xf8,0x8d,0x45,0xec,0x6a,0x00,0x50,0x89,0x75,0xec,0x68,0x86,0x01,0x00,0x10};
    check(capture::travel_layout_matches(native));
    native[28]=0x83;check(!capture::travel_layout_matches(native));
    check(!capture::travel_layout_matches(std::span(native).first(31)));
    const capture::TravelRequest request{248,0,0,0};
    check(request.map==248&&capture::ordinary_travel_message==0x10000186);
    check(capture::travel_map_id(L"55",883)==55);
    check(capture::travel_map_id(L"882",883)==882);
    for(const auto input:{L"",L"0",L"883",L"9999999999",L"-1",L"55.0",L"55 1",L"la"})check(!capture::travel_map_id(input,883));
}
