Guild Wars World Logger - experimental contributor build

While recording, /tp <map ID> submits travel in your current district.
The logger does not filter map types or check unlocks; the official server
decides whether the destination is accessible. For example, /tp 55 requests
Lion's Arch. Travel is manual only. Numeric map IDs are supported;
Toolbox aliases, district shortcuts and outpost-name search are not included.
manual_travel_request records identify requested destinations in the log.

FIND CLIENT.cmd identifies the official client without loading or starting
the logger. START LOGGER.cmd identifies it and then attaches the collector.

Optional AUTO TP.cmd requires Python 3. Start recording first, close chat,
then run AUTO TP.cmd and focus Guild Wars during its five-second countdown.
It submits every numeric ID from 13 through 882 in ascending order without skips, with
random 3–5 second pauses. Rejected IDs are expected. Switching windows or
Ctrl+C stops the sweep; recording continues. No automatic focus stealing.
To resume elsewhere: python auto_tp.py --start 248 --end 882
Keyboard input uses scan codes with held Enter presses. If Windows blocks
input because the game is elevated, run AUTO TP.cmd as administrator too.
This short sweep is not a long dwell for collecting later player arrivals.

1. Extract the entire ZIP into a writable folder (not Program Files).
2. Start Guild Wars normally and log in yourself. No credentials are requested.
3. Double-click START LOGGER.cmd. Press Enter for automatic selection if only
   one Gw.exe is running, or enter its PID from Task Manager > Details.
4. Enter maps and stand still briefly after loading. Visit NPCs and merchants.
5. Double-click STOP LOGGER.cmd before gathering the files. It stops recording
   without closing the game. To collect again after stopping, restart the game
   yourself; the DLL stays loaded until the game exits.
6. Send the world-*.jsonl files back to the project. Include any diagnostic.txt
   file when reporting a crash. No automatic uploads occur.

Logs are written directly into THIS extracted folder, beside world_capture.dll.
The command window may be closed after starting. Recording runs inside Guild
Wars; closing the command window does NOT stop it. Use STOP LOGGER.cmd to stop.
Keep this folder in place while the game is running. Do not replace loaded DLLs.
STATUS.cmd reports worker state: 1 is initializing, 4 is ready, 3 is finished.
START LOGGER.cmd waits for readiness and reports initialization failure instead
of treating worker creation as successful recording.
Check that world-*.jsonl is growing after entering an outpost.

This is the legacy standalone package; its binaries still reject clients
outside the tested 32-bit build with PE timestamp 6ada8611. For the updated
fingerprint-driven combined package, use the project-root START CAPTURE.cmd,
which loads bin\OfficialCapture. GWToolbox/GWCA coexistence is not supported:
start with a game process that has no other GWCA DLL attached. If access fails
because the game is elevated, run the launcher at the same privilege level.
With multiple clients, use the exact PID; the selected executable path is shown.
The logger never launches, closes or restarts Guild Wars.

Recorded data:
- NPC-type model IDs, encoded names, decoded names, positions, facing, level and allegiance (includes entities
  such as pets; not a curated permanent-NPC catalog). Names have an explicit
  pending/unavailable/decoded/cache_limit status; null is not an empty name.
  Decoding uses the client's current language. The cache retains up to 4096
  distinct encoded names per session; encoded names continue beyond that limit.
- NPC definitions for observed NPC table IDs: model/skin file IDs, appearance,
  flags, professions, default level, raw visual-adjustment bytes and model-file
  lists. Separate npc_definition_observed records are emitted on change in each
  map instance. These modern-client fields still need validation against 2022;
  an NPC table ID alone is not a renderable model definition. Invalid/unreadable
  definitions are omitted rather than guessed. This capture is not yet live-tested.
- Your initial map-entry position when a loading transition was observed.
- local_map_transition_observed pairs your last sampled position in the old
  instance with the first valid position in the new instance. It includes both
  map IDs, planes, instance times and the departure sample's age. It records
  same-map instance changes too. Travel method remains unknown: map travel,
  reconnects and walked portals must be distinguished during review. These are
  neither confirmed portal triggers nor verified spawn points. Older logs do
  not contain the missing departure positions. Live validation is pending.
- Other players' first appearances AFTER the initial population snapshot,
  without their names. Visibility changes are not confirmed arrivals/spawns.
- Merchant item observations and local equipped items, including model/dye/mods
  and encoded/decoded base item names. Local equipment includes character level
  and primary/secondary profession; this does not prove the gear is starter gear.
  Base item value is NOT a quoted merchant price. Equipment capture in this
  package has been observed in a live session; starter ownership is not inferred.
- local_inventory_observed records carried bags, equipment and installed storage
  panes once per second when changed. Records retain container/slot, session item
  ID, quantity, model/model-file, profession, base value, dye, exact raw modifier
  words and encoded/decoded names. Item IDs are session-local, not catalog IDs.
  Open storage and visit each pane to expose its items; unloaded panes are not
  reported empty. Modifiers retain stat evidence without inventing decoded stats.
  Carried/equipped/bank capture and decoded names were verified during live zoning.
- ground_item_observed captures visible ground loot with position, plane and
  session item/agent IDs. A matching received item definition includes names,
  models, quantity, dye and raw stat modifiers; missing definitions stay null
  and are retried. ground_item_no_longer_observed does not prove pickup.
  Polling can miss items present for less than a second. This does not expose
  hidden loot tables, drop probabilities or items outside client visibility.
- Experimental quest_snapshot_changed records preserve quest IDs, raw state,
  active quest, map references, markers, encoded/decoded names, objectives and
  descriptions. NPC visibility/death changes reference the quest revision.
  These are correlations; they do not prove quest spawn causality. Snapshots
  contain only information received by the client, not every possible quest.
  This quest extension awaits live verification before shared-package promotion.

No passwords, chat messages, player-agent names or account identifiers are requested
or deliberately recorded. NPC-type names may include player-named pets.
The name_decoded event links exact encoded UTF-16 text to its decoded name.
It may arrive after zoning; match it by encoded text within the same capture,
not by the map active when decoding finishes. Queue saturation defers this
event for retry. It cannot recover names absent from older captures.
Map IDs, coordinates, event times, process ID and
possible local module paths in crash diagnostics may be present. Review before
sharing. Logs currently have no rotation; monitor free disk space on long runs.
Map observations do not identify which door/portal was used. No game EXE, DAT,
account capture, existing logs or private-server database is in this package.

This experimental logger loads a DLL into your game and can crash on an
incompatible build. Eight offline tests cover exports and capture helpers; the prior NPC/spawn collector was
validated live. The exact portable package, name decoding and new equipment collector have
not yet had an end-to-end live attachment test.

Third-party GWCA distribution: https://github.com/gwdevhub/GWToolboxpp
Pinned revision and original files: gwca-provenance.json
Upstream license is included as GWToolboxpp-LICENSE.txt.

Chest observations: gadget_observations records visible gadget IDs, positions and map-instance epochs, including empty snapshots. Gadget identity is unverified until matched to an in-game label. Visibility is not full-map coverage; repeated snapshots are not separate spawns. The live map 499 test captured gadget 8141 beside a user-confirmed Locked Chest and a nearby Pyrewood Staff drop. Ground snapshots do not prove the drop source. This build also includes the incoming-chat detour cleanup and full nested loot-name parser.

