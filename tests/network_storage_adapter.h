#pragma once
#include "network.h"
// Explicit offline adapter for storage/maintenance contract tests only.
// The runtime NetworkServer exposes no transitional request method.
namespace gwserver {
struct OfflineStorageAdapter {
    NetworkManager& manager;
    explicit OfflineStorageAdapter(NetworkManager& value):manager(value){}
    fw::AccountResult request(gwserver::Operation operation,const std::string& payload,unsigned timeout_ms=3000){return manager.execute(operation,payload,timeout_ms);}
};
}
using TestStorageAdapter = gwserver::OfflineStorageAdapter;
