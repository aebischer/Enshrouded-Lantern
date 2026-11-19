# Lantern validation history and current status

Current working build: 0.1.31-dev, for ModLoader 0.6.15 standalone or under Kiln. Offline build, content, equipment and production text-adapter tests passed. Catalogue validation requires all 15 game languages for both item names/descriptions, category and equipment title; English fallback does not count as coverage.

## Direct-stream persistence correction â€” 0.1.31

The user confirmed lighting with 0.1.30 but equipment still returned to the bag.
The log recorded extension encoding but no intermediate decode handling. Native
call-site inspection identified the missing path: character synchronization calls
the stream decoder at 0xe0f150 directly (dd30f0 and e2356c), bypassing the span
adapter at 0xe0f470 previously hooked by Lantern. The stream decoder now retains
the same extension before subsequent native encoding. The span adapter still
handles world import, without processing the extension twice.

Regression cycles alternate between both entry points, including a nonzero
stream cursor and compressed-reader read-ahead. Both lanterns, full/non-full
bags and item metadata remain covered. Lighting changes from 0.1.30 are retained.
The user confirmed persistence after reload and functioning light with Kiln on
2026-11-19, using 0.1.31.

## Kiln and persistence correction â€” 0.1.30

Kiln 0.2.0-dev.29.19 logs confirmed that its FxStart guard erased Lantern's
unregistered light GUID before Lantern's CreateInstance hook could register it.
Registration now happens before FxStart is forwarded. Tests reproduce that guard
for both lanterns, catalogue renewal, repeated calls, failed resource creation,
unrelated effects and non-VFX entries. Kiln's guard is not disabled.

Read-only diagnostics confirmed an occupied private equipment page and a
six-page transient export, while the final five-page save retained the item but
lost its equipment extension. The old decoder discarded the extension outside
the character-load scope. The correction preserves it through intermediate
decode/encode passes. All 400 simulated cycles now include that pass, covering
both lanterns, full/non-full bags, metadata, references, corrupt extensions and
write failures. Native row-allocation success/failure is also tested. This does
not increase the on-disk inventory capacity or edit the player's save.

The build uses warnings as errors. All 15 localization tests and content tests
pass. The subsequent 0.1.31 user test confirmed light and equipped restoration with
Kiln. No automated gameplay validation is claimed.

The native text test exercises lookup and formatting, changing game-language IDs, regional/unknown fallback and unrelated native text forwarding. Equipment tests cover native slot/context-menu routing, replacement with a full bag, unequip capacity, save projection/import, missing-marker fallback, stale selections and unrelated ring behavior.

User tests previously confirmed equipment through the character slot and inventory context menu, cleaned slot artwork and the retained gilded light values (radius 187.5, intensity 100). The simple lantern is unchanged. New translation appearance and linguistic quality still need manual review. A fresh save/reload cycle was not performed for the localization-only build. Multiplayer, dedicated servers, tombstone interactions and other client versions remain unvalidated.

The matching native resources were tested offline; no automated game test is claimed. Dated diagnostics and historical outcomes, including unsuccessful experimental light values, are preserved in the original notebook linked below.

## Original research record

The complete original French notebook is preserved in [the historical archive](archive/fr/VALIDATION.md). It records earlier development stages and is not the current user/developer guide. Technical measurements and offsets have not been discarded.

## Inventory storage correction â€” 0.1.28

400 simulated production encoder/decoder cycles cover both lanterns, full and non-full bags, missing external markers, metadata and backpack references, immutable source snapshots, corrupt extensions and write failures. Legacy six-page migration and zero-quantity stale slot IDs are covered. A private-character repair test verifies all current items and equipment, recovery of four missing mod instances, section checksums and idempotence; malformed, foreign-character, duplicate-slot, occupied-overflow and partial-recovery inputs are rejected. User validation on 2026-11-19 confirms that the repaired inventory is normal and remains stable after reloading with 0.1.28. This validates the reported inventory regression; other untested scenarios listed above remain unvalidated.
