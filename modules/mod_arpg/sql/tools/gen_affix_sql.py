"""Generates modules/mod_arpg/sql/world/2026_11_22_02_arpg_affix.sql from the affix pools in docs/SPEC.md 4.1
(v1 + v2) and docs/BALANCE.md §10 (healer affixes). Edit the POOL table below, then run:
    py -I modules/mod_arpg/sql/tools/gen_affix_sql.py
and apply with arpg/scripts/apply-sql.ps1. Output is idempotent (CREATE IF NOT EXISTS + REPLACE).

Units: 'B'    = percent of B (epic stat budget for the slot at the player's level) -> flat stat value
       'pct'  = hundredths of a percent (250 = 2.5%)
       'flag' = no value (e.g. Socketed adds one prismatic socket)
       'main' / 'sec' / 'sta' = percent of the matching stat line on a REAL blue item of the player's level and slot
                (main stat / best secondary rating / stamina). Converted to 'B' with BLUE_LINE_PCT_OF_B, which is
                measured from item_template by modules/mod_arpg/sql/tools/blue_stats.py (docs/BALANCE.md §16).
T2-T4 ranges are interpolated linearly between the T1 and T5 ranges.
"""
import os

# Slot groups (bit per group). Item slot -> group mapping lives in the game code (m2 task 4.1).
WEAPON, OFFHAND, HEAD, SHOULDER, CHEST, WRIST, HANDS, WAIST, LEGS, FEET, NECK, FINGER, BACK = (1 << i for i in range(13))
ARMOUR = HEAD | SHOULDER | CHEST | WRIST | HANDS | WAIST | LEGS | FEET | BACK
JEWEL = NECK | FINGER
ANY = ARMOUR | JEWEL | WEAPON | OFFHAND
# Armour material restriction (0 = any): 1 cloth, 2 leather, 4 mail, 8 plate
CLOTH, LEATHER, MAIL, PLATE = 1, 2, 4, 8

# Main-stat ids for smart loot (match the core's ITEM_MOD ids): 3 Agility, 4 Strength, 5 Intellect, 7 Stamina
AGI, STR, INT, STA = 3, 4, 5, 7

TIER_UNLOCK = [1, 15, 35, 60, 85]

# Effects the game code does not implement yet (m2 task 4.3): weight 0 until they work. Remove a name here once it does.
NOT_YET_ACTIVE = {'cc_reduction_pct', 'snare_reduction_pct', 'cooldown_rate_pct', 'skillup_pct', 'pet_range_pct',
                  'prismatic_socket'}

# A real blue item's stat line as a percent of B (epic budget, same slot, same player level). Medians over levels 15-90,
# measured 2026-11-22 by blue_stats.py; the ratio is stable across levels (main 36-53%, secondary 27-40%).
BLUE_LINE_PCT_OF_B = {'main': 43, 'sec': 34, 'sta': 50}
# Flat stat affixes: T1 = 30-45% of a blue's line, T5 = 85-100% (a T5 roll equals a full blue stat line).
# Ascended T6-T8 (m5, BALANCE §12) go above that.
STAT_T1 = (30, 45)
STAT_T5 = (85, 100)

# (id, name, kind, effect, slots, material, weight, excl_group, main_stat, native, unit, t1_min, t1_max, t5_min, t5_max)
POOL = [
    # ---- v1 prefixes ----
    (1,  'Deadly',      'prefix', 'crit_rating',       ANY, 0, 100, 0, 0, 1, 'sec', *STAT_T1, *STAT_T5),
    (2,  'Swift',       'prefix', 'haste_rating',      ANY, 0, 100, 0, 0, 1, 'sec', *STAT_T1, *STAT_T5),
    (3,  'Masterful',   'prefix', 'mastery_rating',    ANY, 0, 100, 0, 0, 1, 'sec', *STAT_T1, *STAT_T5),
    (4,  'Precise',     'prefix', 'hit_rating',        ANY, 0, 60, 0, 0, 1, 'sec', *STAT_T1, *STAT_T5),
    (5,  'Keen',        'prefix', 'expertise_rating',  ANY, 0, 60, 0, 0, 1, 'sec', *STAT_T1, *STAT_T5),
    (6,  'Vampiric',    'prefix', 'leech_pct',         WEAPON | FINGER, 0, 20, 0, 0, 0, 'pct', 50, 50, 300, 300),
    (7,  'Thirsty',     'prefix', 'heal_on_hit_pct',   WEAPON | HANDS, 0, 40, 0, 0, 0, 'pct', 20, 20, 100, 100),
    (8,  "Slayer's",    'prefix', 'dmg_vs_elite_pct',  WEAPON | NECK | FINGER, 0, 40, 0, 0, 0, 'pct', 200, 200, 1200, 1200),
    (9,  'Blazing',     'prefix', 'proc_fire_nova',    WEAPON, 0, 20, 1, 0, 0, 'pct', 200, 200, 600, 600),
    (10, 'Shocking',    'prefix', 'proc_chain_light',  WEAPON, 0, 20, 1, 0, 0, 'pct', 200, 200, 600, 600),
    (11, 'Freezing',    'prefix', 'proc_frost_nova',   WEAPON, 0, 20, 1, 0, 0, 'pct', 200, 200, 600, 600),
    # ---- v1 suffixes ----
    (50, 'of the Titan',      'suffix', 'strength',        ANY, 0, 100, 0, STR, 1, 'main', *STAT_T1, *STAT_T5),
    (51, 'of the Wind',       'suffix', 'agility',         ANY, 0, 100, 0, AGI, 1, 'main', *STAT_T1, *STAT_T5),
    (52, 'of the Sage',       'suffix', 'intellect',       ANY, 0, 100, 0, INT, 1, 'main', *STAT_T1, *STAT_T5),
    (53, 'of the Whale',      'suffix', 'stamina',         ANY, 0, 100, 0, STA, 1, 'sta', *STAT_T1, *STAT_T5),
    (54, 'of the Spring',     'suffix', 'spirit',          ARMOUR | JEWEL, CLOTH | LEATHER, 40, 0, 0, 1, 'sec', *STAT_T1, *STAT_T5),
    (55, 'of Evasion',        'suffix', 'dodge_rating',    ARMOUR | JEWEL, 0, 40, 0, 0, 1, 'sec', *STAT_T1, *STAT_T5),
    (56, 'of Deflection',     'suffix', 'parry_rating',    ARMOUR | JEWEL | WEAPON, PLATE, 40, 0, 0, 1, 'sec', *STAT_T1, *STAT_T5),
    (57, 'of Fortitude',      'suffix', 'armor_pct',       ARMOUR | OFFHAND, 0, 60, 0, 0, 0, 'pct', 300, 300, 1500, 1500),
    (58, 'of Thorns',         'suffix', 'thorns_pct',      CHEST | OFFHAND | SHOULDER, 0, 20, 0, 0, 0, 'pct', 200, 200, 1200, 1200),
    (59, 'of Resolve',        'suffix', 'cc_reduction_pct', HEAD | CHEST | NECK, 0, 20, 0, 0, 0, 'pct', 400, 400, 2000, 2000),
    (60, 'of Speed',          'suffix', 'move_speed_pct',  FEET, 0, 40, 0, 0, 0, 'pct', 200, 200, 1000, 1000),
    (61, 'of Greed',          'suffix', 'gold_find_pct',   JEWEL | HEAD | BACK, 0, 40, 0, 0, 0, 'pct', 500, 500, 4000, 4000),
    (62, 'of Fortune',        'suffix', 'magic_find_pct',  JEWEL | HEAD | BACK, 0, 30, 2, 0, 0, 'pct', 300, 300, 2500, 2500),
    (63, 'of Wisdom',         'suffix', 'xp_pct',          HEAD | FINGER, 0, 30, 0, 0, 0, 'pct', 200, 200, 1000, 1000),
    (64, 'of Plenty',         'suffix', 'skillup_pct',     HANDS | BACK, 0, 20, 0, 0, 0, 'pct', 500, 500, 2500, 2500),
    # ---- v2 prefixes (D2-inspired) ----
    (12, 'Cruel',       'prefix', 'weapon_dmg_pct',    WEAPON, 0, 60, 0, 0, 0, 'pct', 500, 500, 2500, 2500),
    (13, 'Socketed',    'prefix', 'prismatic_socket',  WEAPON | CHEST | HEAD | LEGS | WAIST, 0, 15, 0, 0, 1, 'flag', 1, 1, 1, 1),
    (14, 'Venomous',    'prefix', 'proc_poison',       WEAPON, 0, 20, 0, 0, 0, 'pct', 300, 300, 800, 800),
    (15, 'Siphoning',   'prefix', 'power_on_hit_pct',  WEAPON | FINGER, 0, 25, 0, 0, 0, 'pct', 20, 20, 100, 100),
    (16, "Fanatic's",   'prefix', 'cooldown_rate_pct', HANDS | NECK, 0, 15, 0, 0, 0, 'pct', 200, 200, 800, 800),
    (17, 'Arcing',      'prefix', 'proc_struck_chain', CHEST | OFFHAND, 0, 15, 0, 0, 0, 'pct', 300, 300, 800, 800),
    (18, 'Lucky',       'prefix', 'magic_find_pct',    JEWEL | FEET, 0, 20, 2, 0, 0, 'pct', 300, 300, 2000, 2000),
    (19, "Champion's",  'prefix', 'dr_vs_elite_pct',   ARMOUR | OFFHAND, 0, 30, 0, 0, 0, 'pct', 200, 200, 1000, 1000),
    # ---- v2 suffixes ----
    (65, 'of the Bat',        'suffix', 'power_leech_pct',  WEAPON | NECK, 0, 20, 0, 0, 0, 'pct', 50, 50, 200, 200),
    (66, 'of Crushing',       'suffix', 'crushing_blow_pct', WEAPON, 0, 15, 0, 0, 0, 'pct', 200, 200, 600, 600),
    (67, 'of Gore',           'suffix', 'proc_bleed',       WEAPON | HANDS, 0, 20, 0, 0, 0, 'pct', 300, 300, 1000, 1000),
    (68, 'of Regeneration',   'suffix', 'regen_pct',        ARMOUR | JEWEL, 0, 40, 0, 0, 0, 'pct', 20, 20, 100, 100),
    (69, 'of Freedom',        'suffix', 'snare_reduction_pct', FEET | LEGS, 0, 20, 0, 0, 0, 'pct', 500, 500, 2500, 2500),
    (70, 'of the Phoenix',    'suffix', 'phoenix_heal_pct', CHEST | NECK, 0, 10, 0, 0, 0, 'pct', 800, 800, 2000, 2000),
    (71, 'of Warding',        'suffix', 'combat_absorb_pct', OFFHAND | CHEST | BACK, 0, 15, 0, 0, 0, 'pct', 300, 300, 1200, 1200),
    (72, 'of Light',          'suffix', 'pet_range_pct',    HEAD | FINGER, 0, 20, 0, 0, 0, 'pct', 1000, 1000, 4000, 4000),
    (73, 'of the Magus',      'suffix', 'spell_power_pct',  WEAPON | OFFHAND | NECK, 0, 30, 0, 0, 0, 'pct', 200, 200, 1000, 1000),
    (74, 'of Alacrity',       'suffix', 'cast_speed_pct',   ARMOUR | JEWEL, CLOTH, 25, 0, 0, 0, 'pct', 200, 200, 800, 800),
    # ---- healer affixes (BALANCE §10) ----
    (20, 'Radiant',     'prefix', 'overheal_to_dmg_pct', WEAPON | OFFHAND | NECK, 0, 30, 0, 0, 0, 'pct', 2000, 2000, 6000, 6000),
    (21, 'Zealous',     'prefix', 'heal_to_dmg_pct',     WEAPON | OFFHAND, 0, 25, 0, 0, 0, 'pct', 500, 500, 2000, 2000),
    (75, 'of Atonement', 'suffix', 'atonement_pct',      WEAPON | FINGER | NECK, 0, 25, 0, 0, 0, 'pct', 200, 200, 800, 800),
    (76, 'of Martyrdom', 'suffix', 'martyr_dmg_pct',     CHEST | LEGS, 0, 15, 0, 0, 0, 'pct', 1000, 1000, 3000, 3000),
]


def lerp(a, b, t):
    return round(a + (b - a) * t, 2)


def main():
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
    out = os.path.join(root, 'modules', 'mod_arpg', 'sql', 'world', '2026_11_22_02_arpg_affix.sql')
    ids = [p[0] for p in POOL]
    assert len(ids) == len(set(ids)), 'duplicate affix id'
    lines = [
        '-- mod_arpg m2: affix pool (SPEC 4.1 v1 + v2, BALANCE §10). GENERATED by modules/mod_arpg/sql/tools/gen_affix_sql.py - edit the',
        '-- generator, not this file. Idempotent.',
        'CREATE TABLE IF NOT EXISTS `arpg_affix` (',
        '  `id` INT UNSIGNED NOT NULL, `name` VARCHAR(48) NOT NULL, `kind` ENUM(\'prefix\',\'suffix\') NOT NULL,',
        '  `effect` VARCHAR(32) NOT NULL, `slot_mask` INT UNSIGNED NOT NULL, `material_mask` TINYINT UNSIGNED NOT NULL DEFAULT 0,',
        '  `weight` INT UNSIGNED NOT NULL, `exclusive_group` INT UNSIGNED NOT NULL DEFAULT 0, `main_stat` TINYINT UNSIGNED NOT NULL DEFAULT 0,',
        '  `native` TINYINT UNSIGNED NOT NULL DEFAULT 0, `unit` ENUM(\'B\',\'pct\',\'flag\') NOT NULL,',
        '  PRIMARY KEY (`id`)',
        ') ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT=\'mod_arpg affixes\';',
        'CREATE TABLE IF NOT EXISTS `arpg_affix_tier` (',
        '  `affix_id` INT UNSIGNED NOT NULL, `tier` TINYINT UNSIGNED NOT NULL, `unlock_level` TINYINT UNSIGNED NOT NULL,',
        '  `min_value` FLOAT NOT NULL, `max_value` FLOAT NOT NULL,',
        '  PRIMARY KEY (`affix_id`, `tier`)',
        ') ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT=\'mod_arpg affix tier ranges (T6-T8 Ascended added in m5)\';',
        '',
        'DELETE FROM `arpg_affix_tier` WHERE `tier` <= 5;',
        'DELETE FROM `arpg_affix`;',
    ]
    for (aid, name, kind, effect, slots, mat, weight, grp, stat, native, unit, a0, a1, b0, b1) in sorted(POOL):
        if effect in NOT_YET_ACTIVE:
            weight = 0                     # rolls only once the effect works (no tooltip lines for nothing)
        if unit in BLUE_LINE_PCT_OF_B:  # percent of a blue line -> percent of B
            k = BLUE_LINE_PCT_OF_B[unit] / 100.0
            a0, a1, b0, b1, unit = a0 * k, a1 * k, b0 * k, b1 * k, 'B'
        n = name.replace("'", "''")
        lines.append(f"INSERT INTO `arpg_affix` VALUES ({aid}, '{n}', '{kind}', '{effect}', {slots}, {mat}, {weight}, {grp}, {stat}, {native}, '{unit}');")
        for t in range(5):
            f = t / 4.0
            lines.append(f"INSERT INTO `arpg_affix_tier` VALUES ({aid}, {t + 1}, {TIER_UNLOCK[t]}, {lerp(a0, b0, f)}, {lerp(a1, b1, f)});")
    with open(out, 'w', encoding='utf-8', newline='\n') as fh:
        fh.write('\n'.join(lines) + '\n')
    print(f'wrote {out}: {len(POOL)} affixes, {len(POOL) * 5} tier rows')


if __name__ == '__main__':
    main()
