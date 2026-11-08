#pragma once
#include <cstdint>
namespace gwserver { struct ServerSettings; }
// In-process wire listeners. Lifecycle and configuration belong to NetworkServer.
void fw_gw_auth_prepare();
void fw_gw_auth_stop();
bool fw_gw_auth_ready();
std::uint32_t fw_gw_auth_connections();
int fw_gw_auth_run(const gwserver::ServerSettings& settings);
void fw_gw_portal_prepare();
void fw_gw_portal_stop();
bool fw_gw_portal_ready();
std::uint32_t fw_gw_portal_connections();
int fw_gw_portal_run(const gwserver::ServerSettings& settings);
