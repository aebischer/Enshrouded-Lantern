# Lantern validation history and current status

Current build: 0.1.28-dev, tested with ModLoader 0.6.11. Offline build, content, equipment and production text-adapter tests passed. Catalogue validation requires all 15 game languages for both item names/descriptions, category and equipment title; English fallback does not count as coverage.

The native text test exercises lookup and formatting, changing game-language IDs, regional/unknown fallback and unrelated native text forwarding. Equipment tests cover native slot/context-menu routing, replacement with a full bag, unequip capacity, save projection/import, missing-marker fallback, stale selections and unrelated ring behavior.

User tests previously confirmed equipment through the character slot and inventory context menu, cleaned slot artwork and the retained gilded light values (radius 187.5, intensity 100). The simple lantern is unchanged. New translation appearance and linguistic quality still need manual review. A fresh save/reload cycle was not performed for the localization-only build. Multiplayer, dedicated servers, tombstone interactions and other client versions remain unvalidated.

The matching native resources were tested offline; no automated game test is claimed. Dated diagnostics and historical outcomes, including unsuccessful experimental light values, are preserved in the original notebook linked below.

## Original research record

The complete original French notebook is preserved in [the historical archive](archive/fr/VALIDATION.md). It records earlier development stages and is not the current user/developer guide. Technical measurements and offsets have not been discarded.

## Inventory storage correction — 0.1.28

400 simulated production encoder/decoder cycles cover both lanterns, full and non-full bags, missing external markers, metadata and backpack references, immutable source snapshots, corrupt extensions and write failures. Legacy six-page migration and zero-quantity stale slot IDs are covered. A private-character repair test verifies all current items and equipment, recovery of four missing mod instances, section checksums and idempotence; malformed, foreign-character, duplicate-slot, occupied-overflow and partial-recovery inputs are rejected. User validation on 2026-11-19 confirms that the repaired inventory is normal and remains stable after reloading with 0.1.28. This validates the reported inventory regression; other untested scenarios listed above remain unvalidated.
