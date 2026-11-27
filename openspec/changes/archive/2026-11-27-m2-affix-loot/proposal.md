# m2-affix-loot

## Why
Loot is the heart of the ARPG loop: every green, blue and purple should be worth a look, with rare "god rolls" to chase,
and boss drops that come within a few runs. See SPEC 4.1, 4.4 and docs/BALANCE.md.

## What changes
- Affixes roll **when an item is created** (loot, quest reward, crafting) and are stored in the item instance's random-property
  enchant slots (persist automatically). Count by quality: green 1–2, blue 2–3, purple 3–5; ≤ 3 prefixes, ≤ 3 suffixes, no duplicates.
- Pool v1 + v2 (SPEC 4.1) in `arpg_affix` / `arpg_affix_tier`; tiers T1–T5 unlock by the player's level (T6–T8 Ascended later in m5).
- Values follow the player's level (B = epic stat budget for the slot at that level). Smart loot 70% main stat.
- Pity: boss Unique ladder 20% +20%/run, god-roll pity every 3rd kill (`character_arpg_pity`).
- Sell value: `template + affixCount × playerLevel × Arpg.Loot.SellCopperPerAffixLevel` (small `// ARPG:` sell hook).
- Shared test tools (needed by most later tests): `.arpg test pack`, event recorder, `.arpg test equip`, `.arpg pity set|info`,
  `.arpg loot last`.
- Native display first: stat affixes show as the client's own enchant lines where an exact/near SpellItemEnchantment exists;
  other affixes work server-side and get tooltip lines from ArpgUI (m4) or the client patch (m6). MPQ-loading spike.

## Config / SQL added
`Arpg.Loot.Enabled` (1), `Arpg.Loot.SmartLootPct` (70), `Arpg.Loot.AffixBotItems` (0), `Arpg.Loot.SellCopperPerAffixLevel` (200),
`Arpg.Loot.GodRollPityEvery` (3), `Arpg.Loot.UniquePityBase` (20), `Arpg.Loot.UniquePityStep` (20), `Arpg.Test.RecordEvents` (0);
tables `arpg_affix`, `arpg_affix_tier`, `character_arpg_pity`.

## Out of scope
Uniques' signature mechanics (m3, with Superuniques), Ascended tiers and raid tiers (m5), loot pet/filter (m2b), rare names (m6).

## Milestone
2
