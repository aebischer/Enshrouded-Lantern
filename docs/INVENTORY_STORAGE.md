# Inventory storage and repair (0.1.28)

## Cause

The old exporter temporarily represented the equipment page as a sixth Generic
bag page. A separate marker was required to remove it on import. Losing markers,
or loading without the mod, exposed six pages to native inventory initialization.
The pinned executable subtracts eight slots per existing page from an unsigned
40-slot requirement. Six pages underflow that counter; allocation then reaches
the 16-link InventorySetup limit (12 Generic pages in the observed character).
Exporting an equipped lantern adds the thirteenth page. This is not simply one
extra page on every successful reload.

## Corrected format

The final encoder removes the temporary page before serializing. An unused
native bag slot carries the equipped instance when available. If the bag is
full, that row is removed from the native item vector. In both cases its canonical
56-byte item data is stored in a 96-byte extension after the S2BH block, within
the same KSD section. The KSD section length and checksum cover both parts.

All fields are little-endian:

| Offset | Size | Meaning |
|---|---|---|
| 0 | 8 | `LNTINV02` |
| 8 | 4 | 1: instance also in native bag; 2: extension only |
| 12 | 4 | projected item-row index |
| 16 | 56 | canonical equipped row (slot zero) |
| 72 | 8 | reserved, zero |
| 80 | 8 | FNV-1a 64-bit checksum of bytes 0â€“79 |
| 88 | 8 | `LNTEND02` |

The decoder strips the extension before native decoding. It verifies the matching
native row in mode 1, removes that row, adjusts backpack/reference indices and
restores equipment through native import. Invalid recognized extensions refuse
loading rather than silently dropping the item. Serialization failures refuse
the new save. New saves do not write sidecar markers.

The extension is specific to this pinned game build. Native readers ignore
trailing section bytes, but subsequent unmodded saves do not preserve custom
equipment. Unknown custom items may also be removed by native import. Keep a
backup when disabling mods; this is not an uninstall migration format.

## Existing character repair

`tools/repair_inventory.py` is a narrow offline repair tool, not a generic save
editor. It requires Python, zstandard and xxhash, an explicit input and a NEW
output path. It never overwrites the input. It supports the inspected compressed
character snapshot v5, verifies every KSD section checksum and rejects occupied
overflow slots, duplicate slots, nonempty backpack inventories, mismatched
character UUIDs and ambiguous partial recovery.

`--recover` may supply a previous save of the same character. Only known lantern
and CarryOn item instances absent from the current bag are restored, with their
original metadata. Existing items, equipment, selection references, other
snapshot data and unrelated KSD sections are preserved. The output report lists
recovered instances and input/output SHA-256 hashes. Run only with assertions
enabled (do not use Python `-O`).

This tool is intentionally restricted to the inspected damaged character layout.
It is not suitable for arbitrary saves or characters using other inventory mods.
Always close the game, back up the full save directory, verify the output and
confirm the source hash has not changed before replacing an active save.

## Native stream relay (0.1.31)

Both the span adapter and the underlying native stream decoder preserve the
equipment extension through intermediate character synchronization. The latter
is also called directly by the engine. The transient private row is reconstructed
only for intermediate snapshots; final encoding projects it back to the normal
bag capacity. World import restores the actual equipment slot. Tests cover both
paths, nonzero stream cursors and compressed read-ahead. User testing under Kiln
confirmed that equipment remains selected after reloading.
