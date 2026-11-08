#pragma once
#include "gw_effect_registry.h"
#include <functional>
#include <optional>
namespace fw::gw::skills::casting {
// Same-thread Runtime lease. Native batches still pass through World approval;
// only the approved native prefix resolves the reserved outbound position.
struct CompletionPublication {
    std::function<void()> prepare_prefix,publish_prefix,finish;
};
}
