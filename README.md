# Belt Lantern for Enshrouded — 0.1.31-dev

Craftable belt lanterns for [Enshrouded ModLoader](https://github.com/aebischer/Enshrouded-ModLoader),
with a dedicated equipment slot, a waist-attached model and native lighting
without an exposed flame. Windows x64, SDK ABI 1; currently intended for solo play.

## Installation

The repository and compiled downloads are public; no GitHub account is required.

Follow [INSTALLATION.txt](INSTALLATION.txt). Install ModLoader 0.6.15 or newer first, close the game, then copy the compiled files into:

```text
Enshrouded/
  enshrouded.exe
  dinput8.dll                ModLoader standalone, or Kiln when installed
  Mods/EnshroudedModLoader/
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
needed to build the DLL. Output: `dist/Mods/EnshroudedModLoader/lantern/`.

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

## Package layout update

This release requires ModLoader **0.6.15 or newer**. Install the loader first: its
installer migrates recognized old packages into `Mods/EnshroudedModLoader/`,
including package data, while retaining mod IDs and per-world settings. Then
merge this release's `Mods` directory into the game directory. Do not delete
`Mods/EnshroudedModLoader/worlds` or keep duplicate legacy package folders.

Download the [compiled package](https://github.com/aebischer/Enshrouded-Lantern/releases/tag/v0.1.31-dev).

## Kiln lighting and equipment persistence — 0.1.31

The private light is registered before native FX startup, including Kiln's
unknown-effect guard. The guard remains active for other effects, and neither
lantern's light parameters change. Standalone loading uses the same DLL.

Equipment metadata is handled through both native stream and span decoding and re-encoding
between the server snapshot and the final save. Saved inventory capacity remains
unchanged. This fixes a lantern returning to the bag on reload. Lanterns already
returned to the bag must be equipped once again; the mod does not guess which
one the player wants. No save editing is required.

Offline regression tests pass. The user confirmed light rendering with Kiln in
0.1.30; equipped-state persistence still failed there because the stream decoder
bypassed the span adapter. Version 0.1.31 covers that additional entry point;
the user confirmed equipped-state persistence after reloading with Kiln.

## With or without Kiln

Use the same mod download in both cases. **Kiln is optional.**

- **Without Kiln:** install [ModLoader 0.6.15](https://github.com/aebischer/Enshrouded-ModLoader/releases/tag/v0.6.15)
  with `Install.cmd`; it installs its proxy as `<game>/dinput8.dll`.
- **With Kiln:** install [Kiln](https://github.com/frostdev-ops/kiln/releases) first,
  then run the same ModLoader `Install.cmd`. It detects Kiln and installs the
  same loader binary as `kiln/native/enshrouded-modloader/modloader.dll` with
  `native.json`. Kiln keeps ownership of the root `dinput8.dll`.
- In either case, merge this mod's `Mods` folder into the game directory and
  enable it in **Edit world -> MODS**. CarryOn and Lantern are loaded by
  ModLoader, not directly by Kiln: do not copy their DLLs into `kiln/native`,
  `kiln/chain` or the game root.

If your Kiln profile restricts native modules, enable `enshrouded-modloader` or
allow native drop-ins. Do not run two copies of the loader. When adding Kiln to
an existing standalone installation, close the game, install Kiln, then rerun
ModLoader's installer so it can migrate receipt-identified old chain copies.
After removing Kiln, rerun the installer to restore standalone startup.
Preserve `Mods/EnshroudedModLoader/worlds`, package data and installer receipts when updating.

Solo gameplay was tested with ModLoader 0.6.14, CarryOn 0.1.52-dev and Lantern
0.1.31-dev under Kiln **0.2.0-dev.29.19-beta.1**. Lantern lighting and equipped-state
restoration after reload were confirmed in game. This does not certify other
Kiln-hosted frameworks, multiplayer or future game/Kiln builds.

## World profiles migration (ModLoader 0.6.15)

Run the loader Install.cmd before updating or starting the game. It migrates
old ModLoader/worlds files to Mods/EnshroudedModLoader/worlds, with verified
backups, conflict checks and restartable copying. Preserve the new worlds
folder when updating mods. Use 0.6.15 or newer for this directory layout.
