#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// All calls on the creating thread. poll drives callbacks; no blocking I/O.
// DLL owns allocations. Events are copied into caller storage.
typedef struct fw_client_event {
    uint32_t kind; // 1 rooms (newline IDs), 2 joined, 3 chat line, 4 left, 5 error, 6 sent,
                   // 7 roster (own-id TAB total LF id TAB name...), 8 room info (id TAB port TAB users)
    uint32_t status;
    char text[769];
} fw_client_event;
typedef struct fw_client_api {
    uint32_t version, size;
    void *(__cdecl *create)(uint16_t discovery_port);
    void (__cdecl *destroy)(void *);
    uint32_t (__cdecl *command)(void *, const char *operation, const char *value);
    uint32_t (__cdecl *poll)(void *, fw_client_event *);
} fw_client_api;
typedef uint32_t (__cdecl *fw_client_get_api_fn)(uint32_t, uint32_t, fw_client_api *);
// Optional binary voice extension. The ordinary client's poll drives networking.
// Calls use the same client handle and creating thread as fw_client_api.
typedef struct fw_voice_packet {
    uint32_t member, sequence, size;
    uint16_t sample_rate, frame_samples;
    uint8_t codec;
    uint8_t data[1275];
} fw_voice_packet;
typedef struct fw_voice_api {
    uint32_t version, size;
    uint32_t (__cdecl *ready)(void *);
    uint32_t (__cdecl *send)(void *, const fw_voice_packet *);
    uint32_t (__cdecl *poll)(void *, fw_voice_packet *);
} fw_voice_api;
typedef uint32_t (__cdecl *fw_voice_get_api_fn)(uint32_t, uint32_t, fw_voice_api *);
// Serialized oi ScreenSignaling events; binary payloads remain opaque to the UI adapter.
typedef struct fw_screen_event {
    uint32_t size;
    uint8_t data[24576];
} fw_screen_event;
typedef struct fw_screen_api {
    uint32_t version, size;
    uint32_t (__cdecl *ready)(void *);
    uint32_t (__cdecl *send)(void *, const fw_screen_event *);
    uint32_t (__cdecl *poll)(void *, fw_screen_event *);
} fw_screen_api;
typedef uint32_t (__cdecl *fw_screen_get_api_fn)(uint32_t, uint32_t, fw_screen_api *);
#ifdef __cplusplus
}
#endif
