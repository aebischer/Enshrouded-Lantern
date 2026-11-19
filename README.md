# Belt Lantern for Enshrouded — 0.1.28-dev

Craftable belt lanterns for [Enshrouded ModLoader](https://github.com/aebischer/Enshrouded-ModLoader),
with a dedicated equipment slot, a waist-attached model and native lighting
without an exposed flame. Windows x64, SDK ABI 1; currently intended for solo play.

## Installation

Follow [INSTALLATION.txt](INSTALLATION.txt). Install ModLoader first (tested
with 0.6.11), close the game, then copy the compiled files into:

```text
Enshrouded/
  enshrouded.exe
  dinput8.dll                supplied by ModLoader
  Mods/
    lantern/
      lantern.dll
      lantern.dll.mod
```

Enable Lantern in **Edit → MODS** for the selected world, then start that world.
Preserve existing markers when updating older installations for migration.

Supported Windows x64 client build: 1287369. `enshrouded.exe` SHA-256:

```text
5D9EF2F140B8ED49E30DBEE655841B89D08BAEC20F799A1817E171A18A850C77
```

Other executables are rejected before hooks are installed.

## Crafting and equipment

Both recipes are in manual crafting, under **Essentials**, and use the simple
placeable candle:

| Item | Recipe |
|---|---|
| Belt lantern | 2 iron bars + 1 candle |
| Gilded lantern | 2 gold bars + 1 candle |

Both use the same dedicated equipment slot. The gilded version has its own
model, textures, icon and text. Its retained engine lighting parameters are
**radius 187.5** and **intensity 100**. The simple lantern is unchanged.
These engine values are not photometric units and do not imply proportional
changes in perceived brightness.

In the **Character** page (N), click the Lantern slot to open the native equipment
selector, showing the equipped lantern and available inventory instances with
their detail/comparison panels. Selection and confirmation use native behavior,
including double-clicking or the equipment action. The slot is separate from
both rings. Alternatively, right-click a lantern in the bag (B) and choose **Equip**.

Equipping transfers the actual item out of the bag. Replacing a lantern returns
the old one to the slot vacated by the new one, even if the bag was full.
Unequipping requires free bag space. Refused operations must not replace a ring
or remove an item. The model and light follow the actual equipped state.

## Save data

Equipment is saved inside the character's `player_character` KSD section,
with its native checksum and commit. The native bag retains its original page
count. A 96-byte versioned extension records the equipped instance; if a normal
bag cell is free, the native snapshot also carries that same instance there.
With a full bag, the extension carries the instance without adding inventory pages.
Loading with Lantern restores it to the dedicated equipment page.

Old `equipment-markers/*.v1` files are read for migration only. New saves do not
require them. Back up character saves before updates. Loading and saving without
mods can discard unknown mod items, including lanterns and pincers. Unequipping
alone does not make custom items safe for an unmodded save; retain a backup.

Version 0.1.28 fixes the extra-page overflow described in
[the inventory repair notes](docs/INVENTORY_STORAGE.md). Already enlarged saves
require inspection and repair; occupied overflow slots must never be truncated.

## Languages

All custom item names, descriptions, category labels and the equipment-slot title
cover the game's 15 interface languages: English, French, German, Italian,
Spanish, Russian, Polish, Japanese, Korean, Brazilian Portuguese, Thai, Turkish,
Simplified Chinese, Traditional Chinese and Ukrainian. Native selector commands
use the game's own translations. Language selection does not change saved data.

`assets/localization/texts.json` is the source catalogue. The build validates
all 15 languages before generating `src/lantern_texts.h`. Exact coverage is
checked without counting English fallback as a translation. New translations
still need visual and linguistic review in-game.

## Build and test

Install Visual Studio 2022 C++ x64 tools, the Windows SDK and PowerShell.
Clone the repository and run `build.cmd` from its root. Prepared model packs,
icons, SDK headers and MinHook are included. CarryOn and game extracts are not
needed to build the DLL. Output: `dist/Mods/lantern/`.

- `tests/test-localization.cmd`: all 15 languages, native lookup/format adapters,
  equipment title, language changes, fallbacks and unrelated native text.
- `tests/test-equipment.cmd`: production adapters with simulated engine functions;
  transfers, errors, save projection/import and the native selector.
- `tests/test-content.cmd`: recipes, CarryOn coexistence, models/lights, character
  preview, inventory selection and restoration. Requires the sibling CarryOn
  source tree and native fixtures under `../work/`, which are not distributed.

Build uses C++17, `/W4 /WX` and the static runtime. Generated binaries, logs,
saves and personal markers are not tracked. See [validation history](docs/VALIDATION.md).

Offline tests pass, including localization and equipment. User testing confirmed
equipping through the character slot and context menu, and the retained gilded
lighting in 0.1.26. The 0.1.28 storage correction passes 400 simulated save/load cycles;
the user confirmed that the repaired inventory is normal and remains stable
after reloading (2026-11-19). Multiplayer, dedicated servers, tombstone interactions and other game
versions are not validated. The main-menu character preview is intentionally
not loaded before a world enables the mod; the in-world character preview is supported.

## Model sources

The two model source directories are `assets/lantern3d/source/` and
`assets/lantern-gold3d/source/` (Meshy FBX and textures). Regeneration scripts
currently expect a workspace folder named `Lantern` with a sibling `work`
directory. Blender, Python and texconv are needed only for asset regeneration.

Run `prepare-lantern-model.py --gold` through Blender in background mode.
Texture formats: albedo BC7_UNORM_SRGB, normal BC5_UNORM, emissive BC6H_UF16,
and RMA BC1_UNORM. Then run `pack-lantern-model.py --gold`.
`python tests/test-lantern-model.py --gold` uses the independent decoder configured through
`LANTERN_MODEL_REFERENCE`. The gilded pack is embedded as DLL resource 402.
Nominal height: 28 cm; LOD triangle counts: 4,000, 1,200 and 558.
