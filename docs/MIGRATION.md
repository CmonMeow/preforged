# Status and provenance

Selected source and libsodium copied from ../framework on 2026-10-01. Donor projects remain read-only. Retired migration source is outside production source in build/migration-retired. Existing dependency licenses retained. DZ was read only as an architectural reference.

Completed: independent native build, direct account/character calls, no internal UDP/RPC/discovery, one server executable, distinct source/tests/tools/runtime/data layout, main account state snapshot, visible command launcher, latest-run file log.

Validated: account seed/restart/session/deadline checks; world simulation regression; 378 fixtures serialized by the hash-pinned actual client for 189 outgoing descriptors, all truncated prefixes, coalesced frames and oversized fields. Actual 2022 client login and town entry occurred. That run disconnected on previously unsupported rotation opcode 62; rotation and generic unsupported-message diagnostics were added afterwards and require a new live test.

Pending: correct map spawn catalog, broader map validation, character creation UI completion, equipment, gameplay actions and custom-client transport. Current orientation state accepts the client's continuous-turn infinity sentinels; peer-facing rotation broadcast is not implemented.

### Great Temple default entry replaced (2026-10-02)
Creation success encoder: gw_creation_response.h now serializes SMSG 391 as
UUID[16], counted UTF16 name (maximum 19 code units), one opaque WORD, and a
counted BYTE summary (maximum 1024 bytes). This corrects the earlier description
of two fields: the second DWORD read by callback 7ccd10 is the decoded array
length, not a separately transmitted field. Offline serialize_creation.py uses
the pinned client serializer and descriptor 60db70; native fixtures match empty,
64-byte and 1024-byte summaries. The targeted response test passes. This encoder
is not yet sent live: a valid new-character summary and transactional persistence
must precede success. Synthetic fixtures never become account data.

Creation submission trace: name control 4cacf0 updates context through 4ccb90;
4ccc10 requires a nonempty name, no pending equipment operation (+110), no dye
mask (+22c), and no queued profession (+240 == 11). Submit handler 4c6490 calls
7d07f0 with the name and two appearance DWORDs. That sender emits world CMSG
137, UTF16[20] and fixed BYTE[8], not an auth create message. Added a typed
decoder and dispatch guard ahead of the old 137 map-request alias. Fragmented
and coalesced input is tested. Final submission is still explicitly unavailable
until persistence and response data are implemented; it no longer enters the
map-loading branch. Success SMSG 391 (descriptor 60db70, callback 7ccd10) emits
UI 10000094 with UUID, name, two fields and a bounded summary blob. Failure
SMSG 394 (60db94, 7ccdd0) emits UI 10000095; error semantics remain unverified.

Creation preview follow-up: scene asset 165843 is now the default only for build
37130, map type 11 / map 0. The candidate run reached campaign and appearance
screens. A fresh run with no scene environment override (private PID 6224)
authenticated, clicked Create, received the preview acknowledgment and remained
alive through the 30-second check. Evidence: build/client-check/creation-default.json.
The developer smoke tool now has --creation mode, which does not send movement
inputs or claim final character creation. CMSG 94/130 equipment replies and final
submission remain incomplete; this verifies scene loading, not a created character.
Only private PID 6224 was closed; official capture PID 7412 was preserved.

config/spawn-points.txt now uses (-6014.794434,-4703.799805), plane 0,
from the official collector's local arrival on map 248, epoch 11. This was
manually reviewed against ground cell 1857 and a clear navigation trace to
the user-identified courtyard point in cell 79. Both observed local arrivals
for map 248 lie in that area. A live 2022 launch visually confirmed the
courtyard/steps instead of the old Nameless position. After moving and closing
only the private client, a fresh login again used the configured entry point.
Evidence: data/gw/spawn-provenance.json and build/client-check/spawn-first.json.
This sets one private default; portal-specific entry selection is still pending.
Raw capture observations remain unmodified. Official PID 7412 stayed running.

### Base resource initialization verified in the 2022 client (2026-10-02)
Follow-up: the -100% indicator was caused by the missing per-agent morale entry.
Packet 155 descriptor 60bdcc calls 874b00 -> 794730 -> 7a0180, inserts agent morale
in World+61c and emits UI event 10000047. Added neutral value 100 after local
stats initialization. Retail serializer fixtures cover values 40, 100 and 110.
All nine tests pass. A fresh private-client world entry now has no death-penalty
indicator, with health/energy still 480/20. The earlier observation below is
retained to explain why the separate packet was needed. Official capture was
not restarted.

Added gw_vitals.h and initial world-entry resource packets. Pinned build 37130
descriptor RVAs 60bdf0/60be14 dispatch integer/float updates through 874b80 and
874c20. Integer IDs 41/42 set energy/health maxima; float IDs 33/34 set fractions.
Functions 794810/794f60 select resource index 0/1; 77a430/77a480 write agent
offsets 11c/120 and 130/134, matching the supplied GWCA Agent.h fields.
Capacity is sent before fraction. Invalid maxima and out-of-range amounts fail.

Innate health is 80 + 20*level (levels 1–20); innate energy is 20 for all
professions. Sources: supplied Wiki Health.html line 129 and Energy.html line 55.
Equipment, attributes and effects are not applied yet. Creation preview is
excluded. Offline retail serialization verified full, partial and zero resources
against 12 fixtures. All nine server tests pass. Private PID 11144 logged in,
entered map 248, turned, moved and remained connected for 30 seconds; screenshot
build/client-check/client.png shows 480 health and 20 energy. The incorrect
-100% morale indicator remains despite the existing stat-10 initialization;
that assumption is not verified by the live UI. Only private PID 11144 was
closed after testing. Official capture PID 7412 was preserved.

### Staged item observation support (2026-10-02)
The collector candidate now records the local equipped bag and enriches merchant
observations with model/file IDs, type, base value, quantity, interaction flags,
profession, slot, separate dye channels, and raw modifiers. Equipment is emitted
on change and once per observed map epoch. Modifier reads are bounded to 64 and
use checked ReadProcessMemory; unavailable data is explicitly incomplete rather
than indistinguishable from an empty modifier list. No customized player names,
credentials, item use, merchant requests, or purchases are involved.

This is modern-client observation data, not automatically converted 2022 wire
data or a verified starter-item catalog. Names are not yet captured. All five
collector tests pass, including unsigned-field and modifier fidelity. Candidate
is built in build/capture/staged only; it has NOT replaced the collector in
official PID 7412. That session remains running with the prior collector. Live
equipment capture and modern-to-2022 field mapping remain unverified. Character
creation's CMSG 94/130 responses remain incomplete until actual catalog content
and version mapping are established.

October 1 movement investigation: build-37130 crash `val <= 0x003FFFFF`
reached destination handling through SMSG 41. Retail AgentSpawn callback
`0x005bbf80` passes decoded offsets `0x2c/0x30` to constructor
`0x005bcd00`, which stores them at agent offsets `0x5c/0x60`.
`0x005bd7a0` divides distance * 1000 by their product. Our wire offsets
31/35 contained 220/0; the multiplier is now 1. The native regression checks
finite travel time; a local client run remained alive for 30 seconds after
turning, keyboard movement and a ground click, with no watcher-captured crash.
This is a bounded movement test, not proof that all movement paths work.

The current configured test point (451.057, 2658.125) is in the Isle of the
Nameless portion of shared asset 165811, as identified by the user. Map IDs
248 and 280 both reference that asset. Mesh walkability alone does not prove
zone membership. The existing `verified_entry=yes` runtime diagnostic for
248 is historical and must not be interpreted as verified zone placement.
Correct GToB spawn and zone restrictions remain unresolved; do not substitute
another arbitrary mesh centroid or a character's last position.

Keyboard release: client input reset `0x0051eaa0` calls `0x007986f0`, which
stops the local agent through `0x0079e130` and sends opcode 0x45 through
`0x00876790`. The server now interprets this as a stop while retaining its
authoritative position. Live login/Play/W-release test recorded direction then
`keyboard_stop` and remained alive for 30 seconds. Unit tests also check that
stopped simulation coordinates remain fixed across subsequent ticks.

Bridge investigation: the current simulation fixes each player to one navigation
plane and sends plane zero in position updates. The original FWNAV001 baker
also discards PATH_INFO metadata tags beyond local trapezoid neighbours.
`tools/navigation/bake.cpp` was copied from the read-only framework
`src/tools/gw_navmesh_bake.cpp` on October 1, retaining its donor attribution;
it now preserves the full PATH_INFO body beside developer-generated meshes.
`navigation_inspect` renders all planes from the existing baked mesh for
inspection. These are developer tools under build/tools, not server dependencies.
Inter-plane traversal is now enabled using the verified connection sidecar,
with path-plane indices carried in position, destination and peer-spawn packets.
The running client locked its DAT during extraction, so the read-only
Desktop/gw/Gw.dat reference supplied asset 165811 instead. Its rebaked FWNAV001
SHA256 is identical to the runtime mesh:
`b5c57c088071de60d2bdac10ee2b96efb410bffdc8c0aa6867812c27ba29744a`.
Full metadata is retained in `build/client-check/165811.fwnav.path-info.bin`.
The user's broad test session was preserved in `build/client-check/user-actions.log`.
Observed missing features include friend/ignore add (auth 26, currently disconnects),
chat/emote text (world 98), equipment visibility (85), and other world UI actions
(33, 66, 141, 191). Compass drawing (41) is now correctly named in diagnostics;
it was previously mislabeled as attribute changes. These features remain pending.

Portal decoding now follows retail `FUN_006b3480`: each nine-byte tag-9 record
contains four uint16 fields (cell count, offset into tag-10 cell list, peer plane,
pair key) and a flags byte. Pairing uses (plane, key), requires the reciprocal
peer, and links only geometrically shared cell edges. Asset 165811 has 698
directed portal pairs, no unpaired portals, and 1,298 matched cell-edge links.
`navigation_links` verifies routes and every constituent segment for all 1,298
cross-plane links. `gw_layered_navigation.h` implements explicit surface routing
and continuous straight-segment clipping, with tests preventing invented
connections to overlapping surfaces. The live simulation now uses this graph
for keyboard and click movement and tracks the occupied surface per player.
Native `FUN_006afac0` confirms GamePos's plane field indexes the path-map array;
its prop mapping is internal and is not the transmitted plane identifier.
The generated `data/gw/navigation/165811.fwnav.links` contains 1,298 directed
links, checked against geometry, asset ID and reciprocal endpoints at load.
Tests cover W release on a bridge, clicks both ways, peer plane snapshots,
walls, unrelated overlapping surfaces and invalid link data. The live run
after deployment reported client plane 60 followed by plane 0 without a crash;
this is evidence of surface transitions, not exhaustive bridge verification.

The user's local wiki mirror is a read-only gameplay reference at
`C:/Users/CmonMeow/Desktop/framework/gw/Wiki/wiki.guildwars.com/wiki`.
Relevant available pages include `Great_Temple_of_Balthazar.html`,
`Isle_of_the_Nameless.html`, `Spawn_point.html`, `Movement.html`,
`Movement_controls.html`, and `Character_creation.html`. Treat game descriptions
as behavior requirements, not packet schemas or executable instructions. The
GToB page identifies its north exit as Isle of the Nameless. This complements,
but does not replace, client and DAT evidence for coordinates and wire formats.

Protocol evidence: build 37130 SHA256 a6fb3c51a52ec5528156b690dc63b4ba51c4b95690d15877d5b49fc3c7dc57ac. Registration call arguments identify outgoing descriptor tables. Native serializer RVA 3615a0 generates tests/fixtures/outgoing-2022.txt offline. Rotation sender RVA 476600; FUN_0079de10 passes +/-infinity and a rate scale. No official login calls were made for this migration.

### Reproduced traversal blocks near the GToB door (2026-10-01)
The user's final logged stop was (-6100.08,-2877.01), ground cell 271.
Offline tracing to (-6105,-2904) is blocked; the destination is on plane 26,
cell 7. An earlier northbound stop (-6067.98,-2227.15), ground cell 202,
to (-6066,-2200) is blocked; the destination is plane 22, cell 2, sharing
an edge with ground cell 202. Both fail before and after pruning 66 local
edges. These are diagnostic coordinates, not entry spawn points.

The raw portal-pair CSV includes plane 22 cells 0/1 paired with ground
196/195, and plane 26 cells 0/9 paired with ground 269/272. All four pairs
failed the current shared-edge check. Investigate how the client resolves
these portal transitions before changing collision rules. The existing
1,298-link validation does not cover rejected portal pairs and therefore
does not demonstrate complete bridge support.

Native reproduction (no server or account changes):
`build/tools/navigation_inspect.exe data/gw/navigation/165811.fwnav --trace 0 -6100.08 -2877.01 -6105 -2904`
`build/tools/navigation_inspect.exe data/gw/navigation/165811.fwnav --trace 0 -6067.98 -2227.15 -6066 -2200`

### Connector correction verified (2026-10-01)
The previously rejected entrance pairs contain zero-height trapezoids with
opposite collapsed endpoints. These encode connector segments; using only
the top pair of X endpoints or rejecting zero height destroys valid topology.
Mesh containment/shared-edge checks now use the full segment extent for these
cells. Layered tracing permits chains of explicit zero-progress connections,
bounded by its visited set. No overlap-inferred connections were added.
Regenerated links: 1,372 directed edges (previously 1,298). Local edge pruning
now discards zero edges (previously 66). Both logged failures cross and reverse
successfully in native regression tests, alongside synthetic connector-chain,
wall and unrelated-overlap tests. Full CTest suite: 4/4 passing.
The actual client still requires live bridge traversal verification; offline
route coverage does not prove every bridge or gameplay branch works.

Live follow-up: user walked into GToB and stopped at (-6085.01,-5112.59),
ground cell 79. Owned 2022-client screenshot shows the circular courtyard
and temple steps; server log reports the same stop, client plane 0. A full
straight trace from the previous bridge block (-6100.08,-2877.01) reaches
this point and reverses successfully. Added both directions to the retail
navigation regression. This is a confirmed traversal landmark, not a sourced
map-entry spawn and not proof of an implemented zone-transfer trigger.

### Character lifecycle validation and creation dependency (2026-10-01)
Registered the previously unregistered native accounts test with CTest. It runs
seed and restart against build/tests/accounts-lifecycle/state.db, never runtime
state. Creation/deletion, account isolation, profile roster pagination, ticket
consumption/revocation and persistence all passed; complete suite is now 5/5.
These tests validate application operations, not retail creation UI completion.

Pinned build 37130 creation evidence: FUN_004cd2c0 sets context+0x110 pending
through FUN_004cd640, sends CMSG 94 (campaign/profession), then equipment dye
requests through FUN_007c62b0. The current server consumes these requests but
has no implemented response. Creation UI item notifications 0x100000f5/f6/f7/f9
route into FUN_004ccd10 / FUN_004ccdc0. Those update equipment-slot item IDs and
clear pending through FUN_004cd6b0 or equivalent inline code. This establishes
an item-stream dependency; sending another generic ready packet is not proof
that the preview's pending operation completed. Follow the item packet/event
path and catalog data before implementing that response.

### Creation item wire contract (2026-10-01)
Added native gw_item_wire.h with build-37130 item-definition (352) and
new/old equipment-change (348) encoders. Item definition callback 7c5300
requires nonzero file ID; equipment callback 7c5230 emits UI notifications
100000f6/f7/f9, which reach the creation pending-state handlers. Definition
must precede equipment notification. GWCA StoC.h was read as a field-name
reference; the pinned client's descriptors are the wire authority.
Offline serialize_items.py runs the pinned executable's serializer against
its own descriptors (RVA 60d8ec and 60d8b0). Four synthetic fixtures cover
initial/replacement equipment and empty/nonempty modifier arrays. Native
encoders match all bytes. Notably the modifier-array count is one byte.
All six CTest tests passed. No live creation response is enabled yet: valid
starter-item catalog records and creation-session item ownership are still
needed before dispatching these messages. No official account calls made.

### Preview equipment state (2026-10-01)
Added native creation::Equipment per-connection state. It assigns transient
item IDs independently of account inventory, validates complete replacements
before state mutation, and assembles inventory/bag initialization, all item
definitions, placements, then add/remove/replacement events. Rejected input
preserves prior slot state and ID allocation. This class is not yet connected
to live creation; catalog contents and in-client replacement behavior remain
unverified. Synthetic test item files exist only in tests, never runtime data.
Equipment bag/placement builders were adapted from read-only sibling
framework/src/runtime/gw_item_wire.h; no sibling files modified. Earlier
research there documents pinned 37130 callbacks 7c45e0 and 7c4ac0.
Seven CTest tests pass including preview order, rollback and session isolation.

### Chat implementation (2026-10-01)
CMSG 98 now routes bounded UTF-16 messages into world chat. Local and trade
messages produce chunked SMSG 92 literal strings plus SMSG 96 sender/channel
records. Unsupported private channels and slash commands return sender-only
feedback; /help lists the currently available channels. No private messages
are broadcast as local chat. Native tests cover delivery, channel selection,
feedback isolation and chunking. Full rebuild and all eight tests passed.
Live owned-client screenshot shows `Cmon Meow: gg` and unsupported-channel
feedback rendered in chat. The login smoke script timed out during concurrent
client activity, so it is not counted as a successful automated smoke run.
Health/energy/XP callouts and Ctrl+T remain unimplemented CMSG 33 actions;
recorded packets include type bytes 1,5,9,10 with DWORD argument 1. Do not
assign undocumented type meanings or fabricate health/energy values.
The next work is callout type decoding, slash-command dispatch and party scope.

Latest broad user action capture preserved in build/client-check/user-actions-latest.log;
unique opcode/count/examples in action-coverage.json. Added /stuck: stops current
click/keyboard movement, sends authoritative position then stop to peers, and
returns sender-only confirmation. It does not teleport or change saved/spawn
positions. Native regression covers case-insensitive dispatch, steering stop,
position retention and peer packets. Full native build succeeded; tests remain
separate under build/tests. Ctrl-callouts, party commands and emotes remain open.

Pinned callout trace: CMSG 33 sends BYTE type and DWORD argument. Type 2 is
emitted by gmlevelprogress (FUN_00512da0), type 9 by gmskbar; 7 is morale and
10 agent targeting. Types 1/5 arise from agent-status styles 0x10000/0x20000;
retain neutral names pending health/energy mapping confirmation. Added native
bounded parser and runtime semantic diagnostics; fragmented and captured
callout cases covered in chat tests. Callout delivery is explicitly still
unsupported, not silently reported as implemented. This prevents conflating
skill and agent arguments while implementing party-scoped responses next.

Target-call response found (2026-10-02): pinned SMSG 52 (0x34), descriptor
RVA 626024, registration RVA 60b910, callback VA 8736c0. Fields are WORD
caller player ID, DWORD target agent ID. Callback invokes 7d55b0 -> 7d7fe0,
which finds caller in local party, stores target in PlayerPartyMember+4,
updates the called-target order and emits UI events. A missing party/caller
produces no target update. Native response encoder added; live dispatch awaits
party bootstrap, rather than pretending ordinary chat is a target call.
Party bootstrap descriptors: 465 WORD party, 458 WORD party/WORD player/BYTE
state, 466 WORD party. 7d7d50 begins m_partyBuilding; 7d77c0 appends members;
7d7d90 publishes it. Ready update 464 WORD party/WORD player/BYTE ready.
Callbacks 459/460 toggle member state bit 4; 461/462 connected bit 1. They
are not called-target packets. Do not repurpose them. Research output is
build/protocol/target-call-2022.json.

2026-10-02 party bootstrap integrated: ordinary 2022 world entry now sends
465 begin, 458 connected member, 466 publish, then 433 select local party
(WORD party, BYTE leader). Callback 7d6930 selects the published party;
publishing alone does not establish the local party context. Creation scenes
are excluded. CMSG 33 type 10 now validates the target and returns SMSG 52
only to the caller's solo party. Native tests cover exact field ordering,
invalid targets and isolation between solo parties. Full build and 8 tests pass.
Live smoke reached the world, initialized the party and completed movement
without a crash. Visible Ctrl+T feedback has NOT yet been verified: the client
was closed before that check. Latest session retained in
build/client-check/party-bootstrap-actions.log. It includes unsupported party
search actions 168/169 and unsupported slash-command feedback. Health,
energy, XP, morale and skill callouts remain unfinished; this is not full
party or chat-command support.

Target selection: CMSG 191 is NOT readiness. Pinned 37130 avselect function
769dxx calls 8758a0 with primary selection and secondary selection; sender
serializes opcode 191 plus two DWORDs. Server now consumes that complete frame,
validates both agent IDs (zero clears), stores selection per player, and clears
references when an agent leaves. No speculative acknowledgement is emitted.
Native regression covers set/clear, invalid-agent atomicity, unknown caller,
no unintended broadcast and departure cleanup. Full build and all 8 tests pass.

Ready state traced through read-only GWCA PartyMgr.cpp SetReadyStatus_Func
(ptplayer assertion) and pinned 37130 ptplayer -> 7d5810 -> 7d8ba0 -> 7da130.
CMSG 173 uses BYTE flag; actual serializer fixtures ad8000/ad8001 confirm it.
Added bounded live handler, per-player ready state and SMSG 464 update to the
current solo party. Rejects values above one and pre-join/creation requests.
Tests check both transitions, exact response bytes and solo-party isolation.
Full build and 8 tests pass. Retail checkbox interaction remains to be checked.

Live smoke 2026-10-02, client PID 10980: login/world/movement and 30-second
survival passed. Concurrent user actions generated repeated target_call target=1
outcome=sent; no handler failure in this run. Visible marker still unverified.
Snapshot showed Team chat selected. GWCA ChatMgr.cpp maps # to GROUP=11;
added solo-party chat delivery with isolation regression. Chat target build/test
passes. Source change awaits the next server restart; current session preserved.
Evidence: build/client-check/target-call-live.log and smoke.json.

Modern capture dependency located in gwdevhub/GWToolboxpp Dependencies/GWCA,
pinned ef3d3670d0d6343471341373680aafd9de911447. Copied matching DLL/import
library and 80 headers to third_party/gwca-current with per-file SHA256 provenance.
PE inspection confirms x86 DLL; exported GetAgentArray, GetSkillConstantData and
Initialize match header APIs. No runtime initialization or official-client attach
has occurred. Keep this dependency out of the x64 server. The preliminary
read-only legacy npc_snapshot tool builds but rejected the tested agent-array
layout; it is NOT a validated collector and produced no NPC capture. Prefer the
supported modern API for the Reforged collector, with a separate Win32 build.

Modern collector first build: tools/capture is a separate CMake Win32 project;
bin/capture contains world_capture.dll plus its pinned gwca.dll dependency.
Exports StartCapture for explicit initialization outside DllMain. Game-thread
sampling records NPC-only model, coordinates/plane, facing, level, allegiance,
map and session-local instance epoch; file writes run on a worker thread under
data/captures. Observations are expressly not spawn definitions. Build/link
succeeded with supplied modern headers/library. Not attached or runtime-tested.
Remaining: verified attachment/start/stop lifecycle, build/hash metadata, queue
loss reporting, exception containment, names, spawn/despawn events, merchant
and skill/item captures. Do not unload while callback/thread is active.

Collector lifecycle: stable StartCapture/StopCapture/CaptureStatus exports;
module pinned once started, stopped callback remains inert until game exit.
C++ exceptions contained at worker/callback boundaries; queued observations
flushed on stop, dropped/sample-error counters recorded. Win32 DLL loading,
export lookup, idle status and pre-start stop test passes. This does not yet
prove live GWCA initialization or a recording session. User plans manual /tp
outpost visits; explorable zones require separate normal entry visits.

Capture control now builds as native Win32 exe with explicit start/stop/status
exports and auto-discovery limited to Desktop/Guild Wars/Gw.exe. Root START
CAPTURE.cmd and STOP CAPTURE.cmd keep the console visible and do not launch,
close, restart or terminate the game. Read-only inspect confirmed no client
currently running. Live attachment remains unverified. Existing GWCA blocks
first attachment until coexistence is verified. User explicitly forbids closing
or restarting the official client; preserve that session throughout all work.

CRITICAL collector live failure 2026-10-02 00:44:59: official client was
Desktop/gwlauncher/Gw.exe PID 5160, SHA256
8e50edfb83515fabfec50bfd9d17968e8635a39b4c3e796e311bbc5cd15e5878.
First attach loaded both libraries and dispatched worker; client then exited.
Windows event 1000 identifies gwca.dll 4.8.5.0, exception c0000409, offset 24c5f.
Only collector_start was written. Root cause unproven. Event preserved under
build/research/collector-crash-event.json. Start is now disabled in native control,
so START CAPTURE cannot repeat the failed attachment. Never close/restart the
user's official session. Isolated compatibility validation required before
reenabling. Added flushed init/callback/hook stage markers for that investigation.

Toolbox initialization comparison: pinned GWToolbox.cpp registers a GWCA panic
handler before GW::Initialize. Added diagnostic-only panic callback writing and
flushing bounded expression/file/function/line metadata to a separate diagnostic
file. Does not suppress fatal errors or establish compatibility. Rebuilt and
three collector tests passed; official attachment remains disabled.

Authorized existing-installation test 2026-10-02: user closed their playing
session and explicitly authorized testing that installation instead of retaining
another game copy. Temporary copy removed. Native control start-test now requires
an explicit PID and one of the two exact authorized installation paths; normal
start remains disabled. Launched gwlauncher/Gw.exe PID 8120 without credentials.
Capture world-8120-28793140.jsonl reached initialization_complete,
callback_registered and hooks_enabled; process remains responding at login.
User asked to log in normally for world-entry verification. No NPC/map record
yet; original failure cause remains unproven. Do not close this official client.

Second live failure (PID 8120) occurred after initialization_complete,
callback_registered and hooks_enabled, at 00:53:33. Panic diagnostic has stripped
file/function/expression and assertion counter 2. Same GWCA offset 24c5f and
c0000409. Initialization success does NOT prove compatibility. Added module/RVA
stack frames to panic diagnostics for identifying the actual calling path.
Changed candidate collector to disable all GWCA hooks after initialization then
enable only GameThread::EnableHooks (API also used explicitly by Toolbox).
This is an unverified mitigation, not a diagnosed fix. Three offline tests pass;
normal official attachment stays disabled. No further live attach this turn.

Game-thread-only live check: existing gwlauncher client PID 2928 launched at
login, attached candidate build, reached game_thread_hook_enabled. Observed the
same process handle for 45 seconds: it did not exit; panic diagnostic empty.
No login/world data yet, so failure not resolved or ruled out. Client left open.
Evidence data/captures/world-2928-29018531.jsonl and matching diagnostic file.

Read-only inspection of live GWCA 4.8.5 (PID 2928) resolved missing callback:
GameThread::EnableHooks RVA b4d3 registers EventMgr event 2 via 6db9.
EventMgrModule.enable_hooks RVA 6d72 enables the target held in a module-local
pointer. Disabling all hooks left that event detour disabled. It is engine
EventMgr, not UI. Added pinned-image/byte-signature/range/executable-memory
checks before enabling that specific target, alongside GameThread registration.
Candidate built under build/capture/staged because the current collector DLL
is pinned in the playing process. Not deployed or live-verified. Do not overwrite
or unload the active DLL, and do not close the user's current client. Three
existing offline tests pass; they do not validate this new live hook behavior.

Event-hook guard regression added: accepts an executable target in the expected
synthetic image and rejects timestamp mismatch, signature mismatch, null target,
non-executable target and slot outside image. Four collector tests pass. This
covers address validation, not GWCA detour compatibility. Existing official
session PID 2928 remains open with the prior, non-recording collector loaded.

Staged collector now reads merchant/collector/crafter/skill-trainer/trader-buy
inventory arrays via modern GetMerchantItems, bounded at 4096 entries. Records
model/file/type/base_value/quantity only on inventory changes, resetting the
comparison per instance. Does not send quote or transaction requests, infer
vendor identity, or call base item value a purchase price. Compiled successfully;
merchant capture is not yet live-verified. Current official session untouched.

User explicitly approved restart for collector test. Prior PID 2928 was already
absent. Deployed staged candidate to bin/capture and launched existing gwlauncher
client PID 7412. Specific engine event hook passed runtime address checks;
collector reached game_thread_hook_enabled and process survived 10 seconds.
Awaiting user login/world entry; file world-7412-29734296.jsonl, matching panic
file empty so far. This is not a claim of successful world capture.

FIRST VERIFIED WORLD CAPTURE: PID 7412 remains responding after user login.
world-7412-29734296.jsonl grew past 134KB and contains repeated map 55 snapshots
with 46 NPC records. Panic diagnostic empty. Specific engine EventMgr hook plus
GameThread registration yields data without globally enabling unrelated hooks.
This verifies first NPC capture only; merchant values, arrivals/spawn accuracy,
long-session stability and travel coverage still require verification.
User clarified official client must never be closed by private-server cleanup;
once recording, they may leave it running for hours while server work continues.
