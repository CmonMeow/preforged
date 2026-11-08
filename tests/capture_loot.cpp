#include "../tools/capture/loot_message.h"
#include <stdexcept>
#include <iostream>
int main(){auto check=[](bool b){if(!b)throw std::runtime_error("loot filter regression");};
 auto r=capture::loot::parse(L"\x7f0\x10a\x123\x1\x10b\x456\x1");check(r&&r->source_atom==L"\x123"&&r->item_atom==L"\x456");
 check(capture::loot::parse(L"\x7f1\x10a\x8123\x2\x1\x10b\x456\x1\x10f\x2").has_value());
 auto actual=capture::loot::parse(L"\x07f1\x9a9d\xe943\x0b33\x010a\x12bf\xf224\xea77\x4e7d\x0001\x010b\x0a40\x010a\x28c1\x0001\x0001\x010c\x0ba9\x0107" L"Cmon Meow\x1\x1\x2\x10a");
 check(actual&&actual->source_atom==L"\x12bf\xf224\xea77\x4e7d"&&actual->item_atom==L"\x0a40\x10a\x28c1\x1");
 check(actual->encoded.find(L"Cmon Meow")==std::wstring::npos&&actual->encoded.find(wchar_t(0x10c))==std::wstring::npos);
 auto actual2=capture::loot::parse(L"\x7f1\x10a\x12c2\x968c\xb2a0\x1035\x1\x10b\x108\x10a\x2939\x1\x1\x10c\xba9\x107" L"Player\x1\x1");
 check(actual2&&actual2->source_atom==L"\x12c2\x968c\xb2a0\x1035"&&actual2->item_atom==L"\x108\x10a\x2939\x1");
 check(!capture::loot::parse(L"\x7f0\x10a\x8123\x10b\x456"));
 check(!capture::loot::parse(L"normal private chat"));check(!capture::loot::parse(L"\x7f2\x10a\x8123\x10b\x8456"));
 check(!capture::loot::parse(L"\x7f0\x10a\xba9\x107" L"Player\x10b\x8456"));
 check(!capture::loot::parse(L"\x7f0\x10b\x8456\x10a\x8123"));check(!capture::loot::parse(L"\x7f0\x10a\x8123"));
 check(!capture::loot::parse(std::wstring(8193,L'x')));std::cout<<"PASS actual loot nested arguments, recipient privacy, player/normal chat exclusion and bounds\n";
}
