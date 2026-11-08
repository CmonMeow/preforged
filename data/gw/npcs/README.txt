Pre-Searing NPC population review, 2026-10-12

These files are NOT runtime population catalogs and must not be concatenated
into npcs.txt without reviewing baseline eligibility and quest gating.

presearing-review-candidates.txt contains 23 named stationary-friendly review
candidates absent by name/encoded identity from the current baseline. IDs
30001..30023 are staging assignments only. Existing quest overlays are excluded.
Names, positions, facing, model definitions and encoded names are captured data.
The source allegiance (6 in these selected observations) lives in the evidence
JSON because the NPC catalog format does not encode allegiance.

presearing-review-evidence.json records source SHA256 values and representative
capture/map/epoch/agent/visibility segment, exact observation and definition
line, first observed position, displacement bounds, and quest snapshot context.
Name decoding can arrive after the first observation; decoded names are retained.

Reproduce from repository root:
  python tools/capture/audit_npc_population.py
  python tools/capture/stage_npc_population.py
  python tests/test_npc_population_audit.py

The full generated audit is build/reports/npc-population/audit.json. This run
indexed 14 archived world captures, including four-professions 2026-10-11 and
the side-quests checkpoint-1815 of 2026-10-12: 6,205 visibility segments.
Missing-baseline review groups: 43 stationary friendly, 19 existing quest
overlays, 21 transient miniatures, 115 moving/hostile/unresolved, 11 undecoded.
The explicit shortlist excludes crowds, wildlife and cinematic/scripted actors.

Checks: native npc_catalog_validate passed all 23 candidates on exact navigation
planes with valid definitions, scale, IDs and packet construction. Unit coverage
checks visibility gaps, identity replacement, epoch reuse, motion, late decoding
and quest-overlay separation. No official client was controlled or attached.

Limits: disappearance ends a visibility segment, not a proven biological
lifetime. No spawn point is inferred from a moving observation. Quest snapshots
record context but do not prove activation conditions. Matching a baseline name
does not prove all placements of repeated guards/crowds already exist. Multiple
epochs corroborate observations but are not unioned into a baseline population.
Stationary observations do not prove an actor can never move. No complete-map
coverage or current-live-client equivalence is claimed.

Eligibility review follow-up:
  python tools/capture/review_presearing_population.py
presearing-eligibility-evidence.json reviews all 23 candidates against archived
NPC location pages with source hashes and section lines. 22 map placements have
ordinary-location support. Brother Mhenlo is HOLD: captured in map146 while his
published location and A New Monk Trainer walkthrough specify Ashford Abbey.
Farrah Cappo's map160 placement is ordinary; her map146 placement is conditioned
on Protection Prayers and must remain a separate overlay.

The independent native helper src/runtime/gw_presearing_population.h manages
only six starter trainers and Haversdan in map146. All other names/maps return
unmanaged. Objective-complete retains the trainer for turn-in; committed reward
permits Haversdan and retains the departing trainer during caller-owned walking.
Seven captured transitions cover all six professions, including 7-13 seconds
of simultaneous trainer/guide observations. No fixed despawn timer is inferred.
Fresh entry uses settled state. Party precedence is explicit configuration:
selected member, union, or unresolved by default for multiplayer. Missing/unknown
quest state cannot silently become a completed test. C++20 standalone tests in
tests/presearing_population.cpp pass all six paths and party/boundary cases.
Root owns world/CMake integration; this helper itself does not mutate populations.
