# Third-party reference data

Two unrelated things live here: the Smithbox id→name aliases the PC-side tools
print with, and a single small mod kept as a fixture for a feature that does not
exist yet. Neither is read by the PS4 app.

---

## Smithbox Bloodborne aliases

`Characters.json` and `MapNames.json` are copied verbatim from
[Smithbox](https://github.com/vawser/Smithbox) by vawser, at
`src/Smithbox.Data/Assets/Aliases/BB/`.

Smithbox is **MIT licensed** (see `Smithbox-LICENSE.txt` alongside this file),
so redistribution here is permitted with the copyright notice retained.

### Why they're here

The game's map files identify every enemy only by a model id like `c2630`.
Nothing in the map data, and nothing in any file this project already parses,
says what that creature actually *is*. Smithbox maintains a community-curated
id → name mapping, which turns our diagnostics from

    c2630_0000  model=c2630  entity=2410015  frozen

into

    c2630_0000  Huntsman (Transformed)  entity=2410015  frozen

This is **reference data only** — nothing in the randomizer's behavior depends
on it, and the PS4 app never reads it. It exists so the PC-side tools in
`tools/` can print names, and so decisions about which enemies to protect can
be made against real identities instead of guesses.

### Format

Both files are a flat JSON array of `{"ID", "Name", "Tags"}`:

```json
{ "ID": "c1050", "Name": "Chime Maiden", "Tags": [""] }
{ "ID": "m24_01_00_01", "Name": "Central Yharnam", "Tags": [""] }
```

`Tags` is used by Smithbox for filtering; the tag `unused` marks content the
retail game never loads, which is how we know `m24_01_00_11` is dead data.

### Accuracy caveat

These names are community-maintained, not extracted from the game's own text.
They are excellent for orientation but should not be treated as authoritative —
if a decision hinges on exactly which creature a model is, confirm it in game.

### Refreshing

    python tools/update_names.py

---

## `mods/` — fixtures for the future mod-merging feature

`mods/no-logo/` is a real 4 KB mod: three `.gfx` files under `dvdroot_ps4/menu/`
that blank the FromSoftware, Japan Studio and SCE boot logos.

It is kept as the **reference example of the layout a mergeable mod has** — a
partial `dvdroot_ps4/` subtree, exactly the same shape as the vanilla source and
the AFR output. That shape is the whole reason merging is cheap: it is an
overlay onto the AFR tree after randomization, using the mirror path `FileIo`
already runs.

**This is a test fixture, not a shipped asset.** Mods are not bundled into the
`.pkg` — at runtime they are expected to live on the console at
`/data/bbrandomizer/Mods/<name>/dvdroot_ps4/`, parallel to `VanillaSource/`, so
adding one is an FTP copy rather than a rebuild. Real (large) mods belong in the
gitignored `../../../data/`, not here; keep this directory to small fixtures.

The feature is not designed or built. See `docs/deferred-ideas.md` §4 — the open
question is conflict handling: a mod shipping `map/mapstudio/*.msb.dcx` or
`param/*` would silently overwrite what the randomizer just wrote. This one is
trivial precisely because it touches nothing the randomizer touches.
