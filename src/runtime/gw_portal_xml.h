#pragma once
#include <map>
#include <string>
#include <string_view>
namespace fw::gw::portal {
std::map<std::string, std::string> request_fields(std::string_view body);
}
