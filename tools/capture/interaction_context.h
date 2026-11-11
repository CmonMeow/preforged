#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
namespace capture {
// Same-process collector/packet-recorder context; never a server protocol.
struct InteractionContext { LONG sequence; std::uint32_t map,agent; std::uint64_t tick; wchar_t name[128]; };
inline std::wstring interaction_mapping(){return L"Local\\GWInteraction-"+std::to_wstring(GetCurrentProcessId());}
}
