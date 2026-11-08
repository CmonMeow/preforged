SKILLS LOGGER - independent native read-only official-client observer

Use bin/skills/START LOGGER.cmd and supply the exact official Gw.exe PID.
The shareable launcher enables --dump-constants automatically, then continues
visible current-character snapshots. Static data is one deterministic companion
per verified client build, reused when its content already matches.
The command window stays visible: WAITING means there is no stable in-world
snapshot yet; CAPTURED/RUNNING means it is working. Keep the window open.
Ctrl+C stops only the observer. Closing the observer never closes Guild Wars.
The tool does not log in, control UI, inject a DLL, hook code, invoke remote
functions, write process memory, or send traffic. It opens the process with
QUERY_LIMITED_INFORMATION, VM_READ and SYNCHRONIZE only. Thread handles use
THREAD_QUERY_INFORMATION only for the already proved external TLS discovery.

Only explicit PIDs are accepted. Before context inspection, both the full
image path and exact build are verified. Allowed locations for this Windows
user are Desktop/Guild Wars/Gw.exe and Desktop/gwlauncher/Gw.exe. Private
project clients are excluded. The supported modern image is 32-bit:
  PE timestamp: 0x6abd61cd
  SizeOfImage: 0x00f4c000
  SHA256: 8e50edfb83515fabfec50bfd9d17968e8635a39b4c3e796e311bbc5cd15e5878
Older/newer builds stop with an explicit unsupported-layout error. The older
copy currently in Desktop/Guild Wars is therefore not supported. Updating
the client requires independent layout verification before changing this pin.

OUTPUT
skills-UTCSESSION-LOGGERPID-TICK.jsonl is written beside skills_logger.exe.
There is no root-bin or project-state output. Every log starts with the exact
client build hash, matching GWCA revision and independent layout evidence.
The logger polls every second, writes only stable changed snapshots or map
epochs, and flushes after every record. WAITING records state why a sample
was rejected; absence of valid data is never represented as an empty capture.
Map epoch checks compare world/account/character/controlled-agent pointers,
map/world token and instance timer. The arrays are read twice with descriptor
and epoch checks before/after. Zoning/reallocation/changes reject the sample.

Snapshot fields:
  character: only the observed player's own name and CHARACTER GUID, for
    accurate roster association. guid uses canonical Windows GUID formatting;
    uuid_words are the four numeric little-endian DWORDs, and uuid_bytes_hex
    preserves the exact 16 bytes in memory. These identify a character rather
    than the account. Name conversion validates UTF-16 then emits UTF-8 JSON.
  account_unlocked: raw DWORD words plus decoded IDs. This is account unlock
    availability, not proof that the current PvE character learned each skill.
  character_learned: raw words plus decoded IDs, copy counts and raw duplicate
    records. A set learned bit gives one copy unless a duplicate-count record
    explicitly gives a different count. Skill ID 0 is reserved; raw words are
    preserved even if its bit is set, but it is not listed as a skill.
  equipped_slots: exactly eight local-player slots, numbered 1..8, with skill
    ID (0 means empty) and zero-based copy_index. No hero/other-player bars are
    output. Copy identity is not the number of learned copies.
  professions: local primary, secondary and raw unlocked-professions mask;
    IDs 0=None, 1=Warrior, 2=Ranger, 3=Monk, 4=Necromancer, 5=Mesmer,
    6=Elementalist, 7=Assassin, 8=Ritualist, 9=Paragon, 10=Dervish.
  attributes: all 51 verified raw local attribute rows, including base/effective level
    and decrement/increment point values. source_index preserves the physical
    row. Empty/reserved rows are kept rather than inferred away. Primary
    attribute identity can be joined to independently known attribute metadata.
No account identifiers, email/password, chat, inventory or other-player data
are output. Only the owner's observed character name/GUID is captured. Logs
therefore contain that character identity; review them before sharing publicly.
Skill names/effects/prices are not inferred from the bitsets.
Change character normally to capture another character's learned skills/bar.
This tool observes the current character; it cannot claim uncaptured characters.
Schema 2 adds attributes; schema 3 adds exact-code-proved own character identity.
Schema 4 corrects a matching GWCA header error: state stride 0x43c does NOT
mean 54 attribute rows. Only the 0x400-byte prefix (agent+51 rows) is read.
Discard old schema 2/3 source_index values 51..53: they interpreted dynamic
state as rows and must never become character attributes. The dynamic tail at
0x400/0x410/0x424 and separate counters0x434/0x438 are no longer read/output.
Character GUID/name are read twice with the skill data and are part of stable
snapshot and epoch detection. Match canonical GUID or exact raw bytes to the
imported roster rather than assuming roster order or inventing a default bar.

LAYOUT EVIDENCE (offline; no injected GWCA dependency)
Headers pinned at GWToolboxpp Dependencies/GWCA revision:
  ef3d3670d0d6343471341373680aafd9de911447
Matching gwca.dll SHA256:
  b33375229bcb06e415b71246ba317966c0ec6c76f71f284bc49f1783685e1975
The executable does not link or load gwca.dll; headers supply compile-time
offset/size checks only. Disassembly of matching DLL independently confirms:
  GetAccountContext RVA b078: GameContext+28
  GetWorldContext RVA b008: GameContext+2c
  GetIsSkillUnlocked RVA 137ff: Account+124 DWORD bitset, ID/32 and ID%32
  GetIsSkillLearnt RVA 1383d: World+710 DWORD bitset, same ID rule
  GetSkillbarArray RVA 1365a: World+6f0
  GetPlayerSkillbar RVA 13680: agent-ID match, bar stride bc
  Slot ID access RVA 12b85: eight rows, stride14, ID at row+c
  GetAgentAttributes RVA fa6c: World+ac, stride43c, local-agent match, rows+4;
    Attribute.h's declared 54 rows is WRONG despite the correct state stride.
    Exact modern 008199e9 compares index with0x33 and branches to00819a02,
    otherwise asserting attrib<arrsize(attribState->attrib) (literal00a9b3c4).
    Setter0081a054 repeats index<0x33; constructor0081a78b clears0x3fc bytes
    from state+4 at0081a79f/0081a7a4, exactly51 rows of0x14 (20) bytes.
    Local record scans use stride43c, but read only agent+51 rows (0x400 bytes).
    Row IDs26..28/45..50 have no named matching GWCA Attribute enum meaning;
    they remain raw/uninterpreted. Named player attribute IDs are0..25/29..44.
Modern exact-image disassembly independently confirms copy identity:
  Handler 00920dc0 passes packet+10/+c/+8/+4 into 00815140.
  00823130 accesses World+6f0. 0082317f stores ID at bar+slot*14+10;
  00823182 stores copy at bar+slot*14+14 (= row+10, GWCA event field).
  Optimistic load 008221a0 zeroes row+10 then 00822224..0082222c increments
  it once per prior identical ID, proving a zero-based occurrence/copy index.
World+700 learnable_character_skills is deliberately NOT used: that field is
the skill-trainer/capture context, rather than the character learned bitset.
Own-character identity has exact modern executable proof (same supported hash):
  0084f610 gets GameContext through 0047f660 and CharacterContext through+44;
    it copies nativepacket+4 UTF-16 to Char+74, bounded to0x14 characters using
    0046bf70. MapLoad00850130 also copies packet+34 to Char+74, length20.
  0085200f/008520ce/00852189 pass Char+64 beside Char+74 to0048ed20.
    0048ed31 captures the UUID pointer;0048ed5e..0048ed80 copies four DWORDs
    to global00c092c8;0048eda8 passes it to GUID formatter0046b250.
    0048edb2 uses literal CharId at00943470, proving CHARACTER identity.
  Formatter0046b250 reads DWORD0,WORD4/6,bytes8..15 using the pattern
    %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X at0093e034.
Only these early whitelisted CharContext fields are read. Later modern fields
may differ from header comments; no full CharContext or email field is read.

FULL IMMUTABLE CONSTANT TABLE (NO CASTS REQUIRED)
skills_logger.exe OFFICIAL_PID --once --dump-constants
--dump-constants also works with continuous snapshots, in either flag order.
START LOGGER.cmd enables it by default; direct executable invocation without
that flag captures only current-state snapshots.
It saves one deterministic companion per supported client build beside the EXE:
  skill-constants-8e50edfb83515fabfec50bfd9d17968e8635a39b4c3e796e311bbc5cd15e5878.json
This reads ONLY the independently proved immutable .rdata table, not the whole
image and not just the eight equipped skills. Every 164-byte row is retained
as 328 hex digits plus matching Skill.h numeric labels: costs/times/durations/
scales, flags and unknown fields, animation references, icon references and
name/concise/description string references. The client must pass the same
explicit PID/path/build gate. Two equal table reads and every ID==ordinal are
required. Invalid input is rejected before output; publication uses a temporary
file followed by an atomic replace, preserving an existing complete dump on
error. Repeated dumps of the same table reuse the identical companion file.

Exact supported executable evidence (SHA256 above): lookup005a9160 compares
index with0xda7 (3495 exclusive), multiplies by0xa4, and adds0098b4b0.
The table RVA is0058b4b0, .rdata, 573180 bytes. Offline verification confirmed
every stored ID0..3494 equals its row index. Matching DLL GetSkillConstantData
RVA12e85 independently multiplies by0xa4; its initializer checks the final ID
againstcount-1. Unlike the pinned2022 table, this modern table has three icon
reference fields and its name/concise/description fields start at98/9c/a0.

These constants include internal/passive/item/monster rows: they are NOT an
unlock list or proof of player usability. Animation/icon/string numbers are
references under GWCA header labels, not resolved resources, animations played,
or implemented server effects. effect1/effect2/condition are retained RAW;
their particle/sound semantics have not been proved. No direct sound-reference
field is exposed by this verified structure. Casting/effect/audio events and
authoritative skill rules require separate runtime implementation/evidence.
Modern rows beyond3432 cannot be assigned to the pinned2022 catalogue blindly.

BUILD / OFFLINE CHECK
cmake -S tools/skills -B build/skills-logger -A Win32
cmake --build build/skills-logger --config Release
bin/skills/skills_logger.exe --self-test
The self-test opens no process and checks bit IDs, copies/slots serialization,
canonical/raw GUID byte order, Unicode names, 51-row attribute prefix, a known
164-byte constants fixture/cost/time, corrupt table ID rejection, descriptor
rejection and private-client path exclusion.
Optional one-shot capture for a reviewed target:
  skills_logger.exe OFFICIAL_PID --once [--dump-constants]

OPTIONAL TITLE/FACTION REFERENCE CAPTURE (2026-10-07)
START PROGRESSION CAPTURE.cmd uses --progression --once with an explicit official
PID. It uses the same exact executable hash/path check as the skills observer.
No supported official client was running during development; live verification
and server import remain pending. Neither the spawn logger nor the private
server's account balances are modified by this tool.

The separate progression_snapshot record carries the same map epoch and own
character GUID as the adjacent skills snapshot. stats_stride8 preserves all15
primary counters in native order; faction_caps_kurzick_luxon_balthazar_imperial
preserves the four caps. title_tiers preserves index, properties, rank number
and exact encoded UTF16. titles_modern44 preserves index, all nine raw DWORD
fields and both encoded text fields. Pointer addresses are not written.
Modern title field4 is an additional field absent from the2022 wire schema;
DO NOT blindly reinterpret or copy all nine fields into a2022 packet.
Title text is bounded at seven units plus terminator, matching native title
packet descriptors. Invalid/unreadable strings reject the complete sample.
All progression fields and text are read twice; epoch, identity and descriptor
changes reject the sample. A zoning race produces WAITING, not guessed data.
Capture does not decode localized names or infer reward rules. It is reference
input, not a completed title catalog, import operation, or H-window repair.
Self-test exercises actual ReadProcessMemory on local synthetic title records,
including modern ninth field, encoded text, null text, stat stride, and bound
rejection; it never opens another process.

2026-10-07 FOLLOW-UP
Use --progression-only to capture titles/factions independently of skillbars.
The dedicated launcher now uses --progression-only --once. It emits no skills
snapshot; the progression record joins directly by own character GUID. The
combined --progression flag remains available for sessions with a valid skills
snapshot. An official live capture has now succeeded; native conversion and
atomic import of the observed Cmon Meow profile succeeded. Refer to
../../docs/progression-integration.txt for exact scope and outstanding UI test.
