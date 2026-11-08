Items observer is separate from the position and skills loggers.
Build Win32 with this directory's CMakeLists.txt. Run:
  items_logger.exe OFFICIAL_PID
from a normal command prompt while standing still in an outpost. Open the
Xunlai chest normally beforehand if you want its loaded contents included.
It takes two matching read-only snapshots and exits, writing JSONL beside
the executable. It never injects, writes client memory, logs in or restarts.
Only official Desktop/Guild Wars and Desktop/gwlauncher paths and exact
supported SHA256 8e50edfb... are accepted; private clients are excluded.

Captured: exact own character GUID, loaded bags and slots, raw84byte item
records, model/file/type/quantity/value/interaction/profession, complete
modifier arrays, raw encoded UTF16 name and customization, loaded storage
tabs and gold, account material capacity, raw local guild membership rank for
later semantic analysis.
Unloaded bank contents are NOT reported as an empty authoritative account
bank. Modern observations are NOT automatically imported as2022 gameplay
items: unknown wire fields, dye/equipment/stat semantics still need proof.
Output is private account data. No password/email or other players are read.

Implementation currently reuses the skills observer's audited read/image/TLS
utilities by including its translation unit under a renamed unused entry.
The actual skills observer is unchanged and is never called by item capture.

Schema3 capture and offline export
---------------------------------
Schema3 preserves schema2's real item definitions, four weapon-set references/active set,
full native value DWORD, dye tint/packed dyes, formula-derived materials field,
material flag, explicit profession and encoded customization. Private export
inventory ID is intentionally rebound to1; it is not a captured native pointer.
Each installed bag has container_item=actual item ID and container_definition
with the same fields as a normal item. container_ownership is
item_table_exact_installed_bag_backlink_no_slot: the observer verifies the global
item table entry, reverse link to this owned bag, null normal bag pointer, and
Bag type/quantity1. Its raw slot is diagnostic, never an invented placement.
The private codec stores installed containers as referenced bag0/slot0 records;
restoration defines them before bag318 and sends no ordinary location317.

Schema3 adds material_storage_capacity, its source label, and the observed
expansion count (null when the account array has no matching entry). The exact
matching GWCA DLL GetMaterialStorageStackSize at1000cd62 reads account count131
and returns250*(count+1), or250 when absent. The observer double-reads the bounded
array and records only this specific count, not the account's entire count array.
It rejects duplicate counts and values outside the reviewed250..2500 range.
This establishes an observed modern capacity; it does not synthesize a2022
purchase entitlement or infer upgrades from item quantities. No client writes
or remote calls are used.

The root native tool exports a selected ownership domain without installing it:
  items_import CAPTURE_JSONL CHARACTER_GUID ACCOUNT_GUID character|bank OUTPUT
This exports the current FWI5 profile and OUTPUT.ownership.txt. Explicit account
GUID is the operator's ownership binding; the observation authenticates only its
own character GUID. Any later database installation must independently validate
account ownership. The exporter neither opens a client nor changes a database.
Only loaded bank tabs can be exported. Opening a chest does not establish that
unloaded tabs are empty; do not replace an entire existing account bank with an
incomplete observation. Bank export includes ordinary type4 tabs and material
type5/index5/capacity41, retaining physical element slots rather than visual
display rows. Genuine model/modifier identity must match each material slot.
Captured quota is preserved only in the bank domain; character quota remains250.
Schema2 still supports ordinary inventory/bank exports but a schema2 material
pane requires recapture because it lacks capacity evidence. Schema3 capacity
and expansion count must agree with the proved formula and source label.
Missing/unsupported definitions, duplicate IDs, unowned
weapon-set references, contradictory container evidence, old schema1 captures
and unrecognized provenance are rejected rather than replaced with guesses.
Preserved modern2026 fields are not proof of every2022 item gameplay behavior:
upgrade effects, equip constraints and assets still require reviewed semantics.
