#ifndef FRAMEWORK_MODULE_H
#define FRAMEWORK_MODULE_H
#include <stdint.h>
#ifdef _WIN32
#define FW_CALL __cdecl
#ifdef FW_BUILD_MODULE
#define FW_EXPORT __declspec(dllexport)
#else
#define FW_EXPORT
#endif
#else
#define FW_CALL
#define FW_EXPORT
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define FW_ABI_VERSION 1u
#define FW_MAX_PAYLOAD 768u
enum fw_module_flags { FW_MODULE_SESSION_DRAIN = 1u, FW_MODULE_COUNTER_STORE = 2u, FW_MODULE_ACCOUNT_STORE = 4u,
    FW_MODULE_DRAIN_DEPENDENCY = 8u };
enum fw_status {
    FW_OK = 0,
    FW_INVALID = 1,
    FW_NOT_FOUND = 2,
    FW_OVERLOADED = 3,
    FW_TIMEOUT = 4,
    FW_DRAINING = 5,
    FW_CONFLICT = 6,
    FW_INTERNAL = 7,
    FW_UNAVAILABLE = 8
};
typedef struct fw_bytes {
    const uint8_t *data;
    uint32_t size;
} fw_bytes;
typedef struct fw_request {
    uint32_t struct_size;
    uint64_t handle;
    uint64_t deadline_ms;
    uint64_t peer_incarnation;
    uint16_t peer_port;
    const char *operation;
    fw_bytes payload;
} fw_request;
typedef struct fw_host_api {
    uint32_t abi_version, struct_size;
    void *context;
    uint64_t(FW_CALL *now_ms)(void *);
    uint64_t(FW_CALL *random_id)(void *);
    void(FW_CALL *log)(void *, const char *);
    uint32_t(FW_CALL *reply)(void *, uint64_t handle, uint32_t status, fw_bytes);
    uint32_t(FW_CALL *request)(void *, const char *service, const char *instance, const char *operation,
                               fw_bytes, uint32_t timeout_ms, uint64_t correlation);
    // Scoped durable fixture. Requires the declared capability and configured storage.
    uint32_t(FW_CALL *counter_commit)(void *, const char *operation_id, int64_t delta, int64_t *value);
} fw_host_api;
typedef struct fw_module_api {
    uint32_t abi_version, struct_size;
    const char *service_type;
    uint32_t protocol_version;
    uint32_t flags;
    void *(FW_CALL *create)(const fw_host_api *, const char *settings);
    uint32_t(FW_CALL *start)(void *);
    void(FW_CALL *handle_request)(void *, const fw_request *);
    void(FW_CALL *handle_response)(void *, uint64_t correlation, uint32_t status, fw_bytes);
    void(FW_CALL *tick)(void *);
    void(FW_CALL *begin_drain)(void *);
    void(FW_CALL *stop)(void *);
    void(FW_CALL *destroy)(void *);
    uint32_t(FW_CALL *active_sessions)(void *);
    void(FW_CALL *cancel_request)(void *, uint64_t handle);
} fw_module_api;
typedef uint32_t(FW_CALL *fw_get_api_fn)(uint32_t version, uint32_t size, fw_module_api *out);
FW_EXPORT uint32_t FW_CALL framework_module_get_api(uint32_t version, uint32_t size, fw_module_api *out);
#ifdef __cplusplus
}
#endif
#endif
