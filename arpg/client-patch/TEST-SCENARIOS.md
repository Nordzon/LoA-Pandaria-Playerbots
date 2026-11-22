# Client patch probe: test sheet

What we test: whether the client loads our own patch file `Data\enUS\wow-update-enUS-20000.MPQ` (plain DBC; build with `--plain`), which changes the
enchantment table (`SpellItemEnchantment.dbc`). If it does, every affix can get its own green tooltip line (and items
D2-style names) later in m6.

## Steps (each round)
1. Start `D:\WoWMoPProject\MOP-5.4.8.18414-enUS-Repack\Wow-64-all.exe` (unlocked exe), log in as **ARPGADMIN**.
2. Select **Nordisha**, click **Enter World**.
   - **Crash?** Click OK and tell Claude "crashed". Claude reads the report in the client's `Errors\` folder and removes the patch.
3. Optional: empty the bags first: `.arpg test clearbags Nordisha` (destroys everything in the bags, not equipped gear).
4. Type the three commands below; hover each new item; report what each tooltip shows (screenshots are perfect).

## The three items

| # | Command | Item you get | Look for | Means |
|---|---|---|---|---|
| A | `.arpg test probeitem Nordisha 1489 74` | **Gloomshroud Armor** (blue) | **"+1 Agility (ARPG)"** | Patch is applied (we renamed Blizzard's own row 74). Plain "+1 Agility" = not applied. |
| B | `.arpg test probeitem Nordisha 2317 5300` | **Dark Leather Tunic** | **"ARPG probe 5300 +1 Stamina"** | A new row with a low id works. |
| C | `.arpg test probeitem Nordisha 82437 60000` | **Spelltwister's Grand Robe** (purple) | **"ARPG patch probe +1 Stamina"** | A new row with a high id works (our planned id range). |

All three put the enchantment in slot 9 (the client never shows slot 8).

## Round 11: real affix lines (the actual feature)
The patch is now the real one (`build-client-patch.py`): 23,988 rows "+N <Stat>" for 12 stats, values 1-1999. New items
show their flat stat affixes as exact tooltip lines (up to 4 per item). The probe rows (A/B/C above) are gone.

| # | Command | What to check |
|---|---|---|
| D | `.arpg test give Nordisha 82437` then `.arpg loot last Nordisha` | Spelltwister's Grand Robe: the chat line lists the affixes (e.g. `of the Titan T2=4 shown=60004`); every affix marked `shown=` appears on the tooltip with **exactly** that value |
| E | (optional, makes Nordisha level 90!) `.arpg test setup Nordisha 90 jadeforest-dawnsblossom`, then D again | Same, with big values (hundreds) |
| F | equip the robe, open the character sheet | the stats go up by template + affix values (server applies them once, no double counting) |

Affixes without `shown=` are effect affixes (leech, magic find…) or a 5th affix: those come with the ArpgUI addon.

## Results so far (newest first)
| Round | Patch version | Result |
|---|---|---|
| 11 | real affix patch (23,988 "+N stat" rows) | **SUCCESS**: robe "+1 Agility/+1 Stamina/+1 Haste" (rolled at low level); level 90 necklace "+147 Parry, +168 Stamina, +302 Critical Strike", measured `match=yes` |
| 10 | **NEW EXE `Wow-64-all.exe`** (unlocked, from wowmodding.net; audited: 18 bytes changed, no new code) + **plain** table in `wow-update-enUS-20000.MPQ` with all three rows (A: "+1 Agility (ARPG)", B: 5300, C: 60000). **Run A, B and C.** | **SUCCESS**: A "+1 Agility (ARPG)", B "ARPG probe 5300 +1 Stamina", C "ARPG patch probe +1 Stamina" |
| 9 | **same-size** table: only row 74's text `+1 Agility` → `+1 AGILITY` (no new rows) **+ `(attributes)` fields exactly Blizzard's (0x0D; ours had 0x0F with file times)**. | crash on Enter World |

| 8 | real diff like Blizzard's (add-against-old + small extra; 27 KB instead of a 406 KB extra block) | crash on Enter World |
| 7 | + archive metadata `(patch_metadata)` = enUS and `component.wow-enus.txt` (as Blizzard's archives have) | crash on Enter World |
| 6 | patch entry RLE-compressed like Blizzard's | crash on Enter World ("Reached the end of the file") |
| 5 | renamed row 74 + rows 5300/60000, single-unit entry | crash on Enter World |
| 4 | single-unit entry (flags = Blizzard's) | no crash once; 74 control showed plain "+1 Agility", 60000 nothing |
| 3 | BSD0 patch entry, sectored | crash on Enter World |
| 2 | COPY patch entry (old StormLib) | no crash, no line (entry unreadable) |
| 1 | plain file in the archive | crash on Enter World |

Controls already confirmed: slot 9 shows Blizzard enchants (row 74 = "+1 Agility"); the client opens our archive by
name (crash reports name it).

## Status (2026-11-22): SOLVED in round 10 (unlocked exe `Wow-64-all.exe` + plain DBC in `wow-update-enUS-20000.MPQ`)

Earlier status (before round 10): PARKED
Our archive now matches Blizzard's update archives in every property we can measure (MPQ v4 header, `(patch_metadata)`,
component file, listfile order, `(attributes)` 0x0D, entry flags 0x81100200, TPatchInfo, PTCH/MD5_/XFRM, RLE-compressed BSD0
real diff), and StormLib applies it through the full patch chain. `Wow-64.exe` still fails the moment it reads from our
archive - even a plain file (round 1). Community notes say custom data on 5.4.8 needs an **unlocked exe** (signature checks
and the update-number limit removed; modders then use plain DBCs in `wow-update-enUS-20000.MPQ`). Next try, when one is
available: an unlocked 5.4.8 x64 exe that also uses the classic login → round 1's plain-file archive at 20000.
Meanwhile: native affix lines for low values work; ArpgUI addon tooltip lines for all affixes.

## Undo
`py -I arpg/client-patch/build-probe-patch.py --remove` (from the repo folder) removes the patch file from the client.
