# Official-client packet logger

Native C++ x86 logger, separate from the world/spawn collector. It accepts valid
32-bit Guild Wars PE images, then finds the decoded StoC handler table by a code
fingerprint. It requires a unique candidate and validates the table and callbacks
before installing hooks. The November 2026 client fingerprint was found once in
the on-disk executable, but live attachment to that client is not yet verified.
Source: tools/packet_logger; build: build/packet-logger;
package: bin/PacketLogger.

Run START LOGGER.cmd while Guild Wars is open. Enter its PID, or leave the prompt
empty when only one Gw.exe is running. The launcher prints the executable path
and reports readiness. The command window may then close; recording runs inside
the client. STOP LOGGER.cmd stops recording. STATUS.cmd queries readiness.

Files named packets-PID-TICK.jsonl are written next to the DLL and flushed every
half second. `schema` records contain the client's live descriptor words for
each opcode. `packet` records contain an opcode, monotonic Windows tick time,
decoded bytes in hex, byte count and completeness flag. These are decoded client
memory layouts, not encrypted network frames or serialized wire layouts.

This build captures incoming game-server packets (StoC). Fixed layouts are
captured in full, including UTF-16 encoded dialog body/button text and dialog
IDs. The development source additionally recognizes the exact window item-list
schema `[132,4139,6]` (pinned 2022 equivalent `[131,4139,6]`): count DWORD at
offset 4, up to 16 inline item ID DWORDs at offset 8. Its 72-byte decoded capture
zeros unused capacity and reads only the active IDs. Invalid counts or failed
reads retain an incomplete header with `window_items_capture_failed:true`.
This source change has not been deployed into an ongoing capture session.
Other variable arrays/nested layouts record only the four-byte header with
`complete:false`, together with their schema. Outgoing CtoS packets and selected
UI dialog actions are not captured yet. Do not infer a missing outgoing response
from this incoming-only log. Encoded game strings are retained without claiming
they are decoded readable names.

It discovers the native handler table using Toolbox's signature and layout,
validates the complete table, then installs forwarding callbacks. Every callback
calls the original exactly once and returns its result. There are no send,
emulate, block, travel, chat or credential hooks. It does not initialize GWCA.
Live inspection showed native handlers use cdecl; Toolbox's fastcall declaration
belongs to its GWCA callback path and must not be applied to native handlers.

The DLL is pinned while forwarding callbacks exist. Stopping recording does not
unload it; Start resumes the same recording without reinstalling callbacks. Do not replace a
loaded DLL or restart someone's ongoing session automatically. Initial attachment
checks native schemas and live headers; broader zoning/dialog tests remain useful.

Toolbox attribution/license: TOOLBOX-LICENSE.txt. Build with Visual Studio CMake:

    cmake -S tools/packet_logger -B build/packet-logger -A Win32
    cmake --build build/packet-logger --config Release
    ctest --test-dir build/packet-logger -C Release --output-on-failure

Standalone verification covers packet forwarding, return values, invalid-memory
reads, captured raw bytes, and pass-through after recording stops.

Recording lifecycle: Stop pauses packet recording; Start resumes into the same JSONL file with a resumed marker. Callbacks stay installed and continue forwarding while paused. Repeated Start/Stop is safe. Build artifacts now go to build/packet-logger/tools; deploy to bin/PacketLogger only after the official client using that DLL closes. An older loaded DLL cannot gain this fix without a client restart.

NPC interaction files: run the updated bin/GuildWarsLogger collector alongside this packet logger. Server DialogSender129 starts a new interactions/npc-NAME-PID-TICK-SEQUENCE-AGENT.jsonl file; map entry410 or Stop closes it. Matching selected NPC identity/name is shared through a process-local Windows mapping, with a 2s freshness check. Unknown or pending names are explicitly marked unverified. No UI/chat hook added. Full packet log remains authoritative and retains schemas; interaction files duplicate incoming packets until the next interaction/zone. Outgoing dialogue choices and variable-length packet bodies are not yet captured. Quest snapshots and inventory records remain in the world collector log; per-interaction joining still needs implementation and live validation.

Each interaction file now includes the full descriptor schemas, followed by an
interaction_prelude marker and up to 128 incoming packets from the preceding two
seconds. This preserves dialog text arriving before DialogSender. Those packets
are context only and may include unrelated activity; interaction_packets_begin
marks the boundary after which packets were received during this interaction.
Map entry and Stop clear the prelude, so earlier zones or recording sessions
cannot leak into a new interaction. This is not outgoing-choice capture or proof
that every packet in the interaction was caused by its NPC.

Quest correlation (new source build, staged; live validation pending): packet
and interaction-start records include process_id. Quest snapshots and NPC
visibility changes in the world collector now include process_id and tick_ms,
sampled using the same Windows monotonic clock as this recorder. Instance time
and epoch are still preserved separately.

After recording both tools in the same client session, use the native offline
tool built by tools/capture:

    build/capture/tools/quest_interaction_join.exe world.jsonl npc-interaction.jsonl report.jsonl

It retains the preceding quest snapshot as context and quest/NPC observations
between interaction start and the final recorded packet. It requires matching
process/map identity and rejects observed instance changes. It excludes legacy
rows lacking clock/process fields. Output is correlation only, not proof of
which NPC caused a change. Pair files from the same boot/client session: Windows
process IDs can be reused. Sampling gaps, delayed responses after the final
packet, unrelated combat and incomplete quest snapshots remain limitations.
Inputs are preserved; output must use a different filename. Bounded JSON rows
over 256KiB are rejected explicitly rather than silently truncated. Outgoing
dialogue choices and inventory joins remain unfinished.

Staged item-definition capture (not yet in the shipped package)
-------------------------------------------------------------
The development build now recognizes the exact modern352 detail and353/354
general item schemas. Original callbacks847d90/847ba0/847c80 in executable
SHA2568e50edfb... prove inline encoded names and modifier arrays; the final404
descriptor is the array-element type. It captures at most64 active modifiers
and preserves the native expanded scalar fields, including model/file/type,
quantity, dye, interaction and value. Encoded names remain encoded.

Unused name and modifier capacity is zeroed, labeled
normalized_unused_capacity:true. These are normalized decoded-memory records,
not untouched network frames. Invalid counts, incomplete reads and mismatched
headers produce incomplete header records with item_definition_capture_failed.
Other unsupported variable/nested schemas still record only their header.
Capture does not mutate the original packet or alter the original callback's
result. Unit/integration tests and offline executable checks pass; live official
validation and publishing remain pending. Do not infer ownership, installed-bag
links or modern-to-2022 item compatibility from a standalone definition.
