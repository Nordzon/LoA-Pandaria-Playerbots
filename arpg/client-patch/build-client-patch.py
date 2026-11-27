"""Builds the ARPG client patch: Data\\enUS\\wow-update-enUS-20000.MPQ for the unlocked client exe (Wow-64-all.exe, SPEC 5).

Contents (m2-affix-loot task 4.7):
  DBFilesClient\\SpellItemEnchantment.dbc = the server's extracted table + one display row per (stat, value):
      id = Arpg.Display.EnchantBase + statIndex * Arpg.Display.EnchantStride + value      (values 1 .. stride-1)
      text "+$k1 <Stat>" (Blizzard's own wording), type 5 (stat), amount = value
  + one display row per (effect, value in tenths of a percent): id = Arpg.Display.EffectBase + effect * 1000 + tenths,
      text e.g. "1.5% of damage dealt heals you", type 0 (display only)
  The server never loads these rows (stats come from Loot/AffixStats); it only writes their ids into display slots 9-12.
  Id rule and stat order must match modules/mod_arpg/src/Core/DisplayEnchant.{h,cpp}.

    py -I arpg/client-patch/build-client-patch.py            # build into %TEMP%\\arpg_mpq
    py -I arpg/client-patch/build-client-patch.py --install  # also copy into the client (client must be closed)
    py -I arpg/client-patch/build-client-patch.py --remove   # remove it from the client
"""
import ctypes, os, re, shutil, struct, sys
from ctypes import wintypes

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
DLL = os.path.join(ROOT, 'arpg', 'tools', 'bin', 'StormLib', 'StormLib.dll')
SERVER_DBC = r'D:\WoWMoPProject\ServerData\data\dbc\SpellItemEnchantment.dbc'
CLIENT_DATA = r'D:\WoWMoPProject\MOP-5.4.8.18414-enUS-Repack\Data'
TARGET = os.path.join(CLIENT_DATA, 'enUS', 'wow-update-enUS-20000.MPQ')
OUT_DIR = os.path.join(os.environ['TEMP'], 'arpg_mpq')
CONF_FILES = [os.path.join(ROOT, 'build', 'bin', 'RelWithDebInfo', 'arpg.conf'),
              os.path.join(ROOT, 'modules', 'mod_arpg', 'conf', 'arpg.conf.dist')]

# Effect affix rows: id = Arpg.Display.EffectBase + effectIndex * 1000 + tenths of a percent (1..999), text below.
# ORDER = Core::Effect enum in modules/mod_arpg/src/Core/AffixEffects.h (index 1 = first entry). {v} = value in %.
EFFECTS = [
    ('leech_pct', '{v}% of damage dealt heals you'),
    ('heal_on_hit_pct', 'Heals {v}% of maximum health on hit'),
    ('dmg_vs_elite_pct', '+{v}% damage against elites and bosses'),
    ('weapon_dmg_pct', '+{v}% weapon damage'),
    ('spell_power_pct', '+{v}% spell damage'),
    ('crushing_blow_pct', '{v}% chance of Crushing Blow'),
    ('power_on_hit_pct', 'Restores {v}% power on hit'),
    ('power_leech_pct', '{v}% of damage dealt restores mana'),
    ('proc_fire_nova', '{v}% chance on hit to cast Fire Nova'),
    ('proc_chain_light', '{v}% chance on hit to cast Chain Lightning'),
    ('proc_frost_nova', '{v}% chance on hit to cast Frost Nova'),
    ('proc_poison', '{v}% chance on hit to poison the target'),
    ('proc_bleed', '{v}% chance on hit to cause Open Wounds'),
    ('dr_vs_elite_pct', '-{v}% damage taken from elites and bosses'),
    ('thorns_pct', 'Attackers take {v}% of the melee damage they deal'),
    ('armor_pct', '+{v}% armor'),
    ('regen_pct', 'Regenerates {v}% of maximum health every 5 sec'),
    ('phoenix_heal_pct', 'Cheat death: heals {v}% of maximum health (2 min cooldown)'),
    ('combat_absorb_pct', 'Entering combat: absorbs damage equal to {v}% of maximum health'),
    ('proc_struck_chain', '{v}% chance when struck to cast Chain Lightning'),
    ('cc_reduction_pct', '-{v}% stun, fear and root duration'),
    ('snare_reduction_pct', '-{v}% snare duration'),
    ('move_speed_pct', '+{v}% movement speed'),
    ('cast_speed_pct', '+{v}% casting speed'),
    ('cooldown_rate_pct', '-{v}% ability cooldowns'),
    ('overheal_to_dmg_pct', '{v}% of overhealing damages your target'),
    ('heal_to_dmg_pct', '{v}% of healing done also damages your target'),
    ('atonement_pct', '{v}% of damage dealt heals the most injured ally'),
    ('martyr_dmg_pct', '+{v}% damage while below 50% health'),
    ('gold_find_pct', '+{v}% gold from monsters'),
    ('magic_find_pct', '+{v}% chance for better magic items'),
    ('xp_pct', '+{v}% experience'),
    ('skillup_pct', '+{v}% profession skill-up chance'),
    ('pet_range_pct', '+{v}% loot pet range'),
    ('prismatic_socket', 'Socketed'),
]
EFFECT_ROWS_PER = 1000

# (display name, core ITEM_MOD id) in DisplayStat order (Core/DisplayEnchant.h)
STATS = [('Strength', 4), ('Agility', 3), ('Intellect', 5), ('Spirit', 6), ('Stamina', 7), ('Critical Strike', 32),
         ('Haste', 36), ('Hit', 31), ('Expertise', 37), ('Mastery', 49), ('Dodge', 13), ('Parry', 14)]


def conf_int(key, default):
    for path in CONF_FILES:
        if os.path.exists(path):
            m = re.search(rf'^\s*{re.escape(key)}\s*=\s*"?(\d+)"?', open(path, encoding='utf-8', errors='replace').read(), re.M)
            if m:
                return int(m.group(1))
    return default


def fmt_tenths(t):
    return f'{t // 10}' if t % 10 == 0 else f'{t // 10}.{t % 10}'


def build_dbc(path_out, base, stride, effect_base, unique_rows=()):
    d = open(SERVER_DBC, 'rb').read()
    magic, n, fields, rsz, ssz = struct.unpack('<4s4i', d[:20])
    assert magic == b'WDBC' and fields == 26 and rsz == 104, 'unexpected SpellItemEnchantment layout'
    recs = d[20:20 + n * rsz]
    strings = bytearray(d[20 + n * rsz:20 + n * rsz + ssz])
    max_id = max(struct.unpack_from('<I', recs, i * rsz)[0] for i in range(n))
    assert base > max_id, f'Arpg.Display.EnchantBase {base} must be above the highest Blizzard id {max_id}'
    rows = bytearray()
    count = 0
    for s, (name, item_mod) in enumerate(STATS):
        text_off = len(strings)
        strings += f'+$k1 {name}'.encode() + b'\0'                      # one shared string per stat
        for value in range(1, stride):
            rid = base + s * stride + value
            # niiiiiiiiiisiiiiiiiixixfff: id, charges, type[3], amount[3], spellid[3], desc, aura, flags, gem, cond,
            # skill, skillval, reqlevel, maxlevel, unk20, scalinglevel, unk22, scalingpoints[3]
            rows += struct.pack('<11I', rid, 0, 5, 0, 0, value, 0, 0, item_mod, 0, 0)
            rows += struct.pack('<I', text_off)
            rows += struct.pack('<11i', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
            rows += struct.pack('<3f', 0, 0, 0)
            count += 1
    assert effect_base >= base + len(STATS) * stride, 'Arpg.Display.EffectBase overlaps the stat rows'
    for idx, (name, text) in enumerate(EFFECTS, start=1):
        values = [0] if name == 'prismatic_socket' else range(1, EFFECT_ROWS_PER)
        for t in values:
            rid = effect_base + idx * EFFECT_ROWS_PER + t
            line = text.replace('{v}', fmt_tenths(t))
            # type 0 = display only (the server applies the effect); amount 0
            rows += struct.pack('<11I', rid, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
            rows += struct.pack('<I', len(strings))
            strings += line.encode() + b'\0'
            rows += struct.pack('<11i', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
            rows += struct.pack('<3f', 0, 0, 0)
            count += 1
    for rid, line in unique_rows:
        rows += struct.pack('<11I', rid, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)   # type 0: display only
        rows += struct.pack('<I', len(strings))
        strings += line.encode() + b'\0'
        rows += struct.pack('<11i', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        rows += struct.pack('<3f', 0, 0, 0)
        count += 1
    out = struct.pack('<4s4i', b'WDBC', n + count, fields, rsz, len(strings)) + recs + bytes(rows) + bytes(strings)
    open(path_out, 'wb').write(out)
    return count


SERVER_SUFFIX_DBC = r'D:\WoWMoPProject\ServerData\data\dbc\ItemRandomSuffix.dbc'
# Random-suffix rows (task 4.10): an item whose random-suffix id is -(base + affix id) shows all five enchant slots 8-12
# (confirmed in game 2026-11-22) and gets the suffix in its name. One row per affix; prefix affixes get an "of" form.
# The rows carry no enchants (the item's slots hold our display rows). Base = Arpg.Display.SuffixBase (30000).
PREFIX_TAILS = {
    'Deadly': 'of Deadliness', 'Swift': 'of Swiftness', 'Masterful': 'of Mastery', 'Precise': 'of Precision',
    'Keen': 'of Keenness', 'Vampiric': 'of the Vampire', 'Thirsty': 'of Thirst', "Slayer's": 'of Slaying',
    'Blazing': 'of Flames', 'Shocking': 'of Shock', 'Freezing': 'of Frost', 'Cruel': 'of Cruelty', 'Socketed': 'of Sockets',
    'Venomous': 'of Venom', 'Siphoning': 'of Siphoning', "Fanatic's": 'of Fanaticism', 'Arcing': 'of Arcing', 'Lucky': 'of Luck',
    "Champion's": 'of the Champion', 'Radiant': 'of Radiance', 'Zealous': 'of Zeal',
}
PROBE_SUFFIX = (30001 - 30000, 'of the ARPG Probe')   # kept for .arpg test suffixprobe (affix id 1 = Deadly overrides it)


def db_rows(sql):
    import subprocess
    mysql = r'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe'
    cnf = os.path.join(ROOT, 'arpg', 'scripts', 'mysql.local.cnf')
    out = subprocess.run([mysql, '--defaults-extra-file=' + cnf, '-D', 'world', '-B', '-N', '-e', sql],
                         capture_output=True, text=True, check=True).stdout
    return [line.split('\t') for line in out.splitlines() if line.strip()]


ROMAN = ['', 'I', 'II', 'III', 'IV', 'V', 'VI', 'VII', 'VIII']


def unique_power_rows(unique_base):
    """[(id, text)] one display row per Unique power, tier and roll combination (task m3 3.6).
    id = base + power*1000 + tier*100 + combo, combo = (trigger - lowest trigger) * magnitudeSteps + magnitude step (5%).
    Must match Loot/Uniques.cpp MakeUnique. The best combination ends in " - Perfect"."""
    powers = {int(r[0]): (int(r[1]), r[2]) for r in db_rows('SELECT id, `trigger`, text FROM arpg_unique_power')}
    rows = []
    for r in db_rows('SELECT power_id, tier, trigger_min, trigger_max, magnitude_min, magnitude_max FROM arpg_unique_power_tier'):
        pid, tier, tmin, tmax, mmin, mmax = map(int, r)
        if pid not in powers:
            continue
        trig, text = powers[pid]
        lo, hi = min(tmin, tmax), max(tmin, tmax)
        mags = list(range(mmin, mmax + 1, 5))
        best_t = lo if trig == 3 else hi           # every Nth hit: lower N is better
        for ti, t in enumerate(range(lo, hi + 1)):
            for mi, m in enumerate(mags):
                combo = ti * len(mags) + mi
                assert combo < 100, f'power {pid} tier {tier}: too many roll combinations'
                line = text.replace('{c}', str(t)).replace('{m}', str(m)).replace('{t}', ROMAN[tier])
                if t == best_t and m == mags[-1] and (lo != hi or len(mags) > 1):
                    line += ' - Perfect'
                rows.append((unique_base + pid * 1000 + tier * 100 + combo, line))
    return rows


def suffix_rows(base):
    """[(id, name, internal)] from arpg_affix in the world DB."""
    import subprocess
    mysql = r'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe'
    cnf = os.path.join(ROOT, 'arpg', 'scripts', 'mysql.local.cnf')
    out = subprocess.run([mysql, '--defaults-extra-file=' + cnf, '-D', 'world', '-B', '-N', '-e',
                          'SELECT id, name, kind FROM arpg_affix ORDER BY id'], capture_output=True, text=True, check=True).stdout
    rows = []
    for line in out.splitlines():
        aid, name, kind = line.split('\t')
        tail = name if kind == 'suffix' else PREFIX_TAILS.get(name, 'of ' + name)
        rows.append((base + int(aid), tail, 'ARPG' + re.sub(r'[^A-Za-z]', '', name)))
    # Unique names (until the Item-sparse patch, m6): "<base item> - <Unique name>"
    for r in db_rows('SELECT id, name FROM arpg_unique ORDER BY id'):
        rows.append((base + 1000 + int(r[0]), '- ' + r[1], 'ARPGUnique' + r[0]))
    return rows


def build_suffix_dbc(path_out, suffix_rows_list):
    src = SERVER_SUFFIX_DBC + '.blizzard' if os.path.exists(SERVER_SUFFIX_DBC + '.blizzard') else SERVER_SUFFIX_DBC
    build_suffix_dbc_from(src, path_out, suffix_rows_list)


def build_suffix_dbc_from(src, path_out, suffix_rows_list):
    d = open(src, 'rb').read()
    magic, n, fields, rsz, ssz = struct.unpack('<4s4i', d[:20])
    assert magic == b'WDBC' and fields == 13 and rsz == 52, 'unexpected ItemRandomSuffix layout'
    recs = d[20:20 + n * rsz]
    strings = bytearray(d[20 + n * rsz:20 + n * rsz + ssz])
    rows = bytearray()
    for rid, name, internal in suffix_rows_list:
        name_off = len(strings); strings += name.encode() + b'\0'
        int_off = len(strings); strings += internal.encode() + b'\0'
        rows += struct.pack('<13I', rid, name_off, int_off, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    out = struct.pack('<4s4i', b'WDBC', n + len(suffix_rows_list), fields, rsz, len(strings)) + recs + bytes(rows) + bytes(strings)
    open(path_out, 'wb').write(out)


def build_charbaseinfo(path_out):
    """CharBaseInfo.dbc for the client (m6-any-race-class): one row per race x class pair from charbaseinfo_rows.csv
    (written by modules/mod_arpg/sql/tools/gen_any_race_class.py). Layout as the stock file: uint32 id, uint8 race,
    uint8 class, 2 bytes padding (3 fields, 8 bytes per record, empty string block)."""
    csv = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'charbaseinfo_rows.csv')
    pairs = []
    for line in open(csv, encoding='utf-8').read().splitlines()[1:]:
        if line.strip():
            r, c = line.split(',')
            pairs.append((int(r), int(c)))
    recs = b''.join(struct.pack('<IBBH', i, r, c, 0) for i, (r, c) in enumerate(pairs))
    open(path_out, 'wb').write(struct.pack('<4s4i', b'WDBC', len(pairs), 3, 8, 1) + recs + b'\0')
    return len(pairs)


SERVER_SPELL_DBC = r'D:\WoWMoPProject\ServerData\data\dbc\Spell.dbc'   # Blizzard's, never patched (the server ignores texts)


def potion_descriptions():
    """{spell id: tooltip text} for the ARPG potions (arpg_potion): the item's "Use:" line is the spell description, which
    still said the stock flat amount (owner 2026-11-27)."""
    text = {
        0: 'Restores %d%% of your maximum health. Shares a 10 sec cooldown with other potions.',
        1: 'Restores %d%% of your maximum mana (energy users half of that, rage users rage). Shares a 10 sec cooldown with other potions.',
        2: 'Restores %d%% of your maximum health and mana. Shares a 10 sec cooldown with other potions.',
    }
    out = {}
    for spell, kind, pct in db_rows('SELECT i.spellid_1, p.kind, p.pct FROM arpg_potion p JOIN item_template i ON i.entry = p.item_id '
                                    'WHERE i.spellid_1 > 0'):
        out[int(spell)] = text.get(int(kind), text[0]) % int(pct)
    return out


POTION_ADDON_DATA = os.path.join(ROOT, 'arpg', 'addons', 'ArpgUI', 'PotionData.lua')


def write_potion_addon_data():
    """ArpgUI's PotionData.lua: {item id = "Use:" text} for the ARPG potions, from arpg_potion (generated, don't edit)."""
    text = {
        0: 'Restores %d%% of your maximum health. Shares a 10 sec cooldown with other potions.',
        1: 'Restores %d%% of your maximum mana (energy users half of that, rage users rage). Shares a 10 sec cooldown with other potions.',
        2: 'Restores %d%% of your maximum health and mana. Shares a 10 sec cooldown with other potions.',
    }
    rows = db_rows('SELECT p.item_id, p.kind, p.pct FROM arpg_potion p ORDER BY p.item_id')
    lines = ['-- Generated by arpg/client-patch/build-client-patch.py from world.arpg_potion. Do not edit.', 'ArpgPotionText = {']
    for item, kind, pct in rows:
        lines.append('  [%d] = "Use: %s",' % (int(item), text.get(int(kind), text[0]) % int(pct)))
    lines.append('}')
    open(POTION_ADDON_DATA, 'w', encoding='utf-8', newline='\n').write('\n'.join(lines) + '\n')
    print(f'  + ArpgUI PotionData.lua: {len(rows)} potion tooltips')


AFFIX_ADDON_DATA = os.path.join(ROOT, 'arpg', 'addons', 'ArpgUI', 'AffixText.lua')


def write_affix_addon_data():
    """ArpgUI's AffixText.lua: stat names (DisplayStat order, 0-based) and effect texts (Core::Effect order, 1-based) - the
    same texts as the client patch rows, for the corpse-loot preview lines (LootTooltip.lua)."""
    lines = ['-- Generated by arpg/client-patch/build-client-patch.py (STATS / EFFECTS). Do not edit.', 'ArpgStatNames = {']
    lines += ['  [%d] = "%s",' % (i, name) for i, (name, _) in enumerate(STATS)]
    lines += ['}', 'ArpgEffectTexts = {']
    lines += ['  [%d] = "%s",' % (i, text.replace('"', '\\"')) for i, (_, text) in enumerate(EFFECTS, start=1)]
    lines.append('}')
    open(AFFIX_ADDON_DATA, 'w', encoding='utf-8', newline='\n').write('\n'.join(lines) + '\n')
    print(f'  + ArpgUI AffixText.lua: {len(STATS)} stats, {len(EFFECTS)} effects')


def build_spell_descriptions(path_out):
    """Client Spell.dbc with new description strings (field 3) for some spells: the new strings are appended to the string
    block and the records point at them; every other byte stays Blizzard's."""
    texts = potion_descriptions()
    d = open(SERVER_SPELL_DBC, 'rb').read()
    magic, n, f, sz, ssz = struct.unpack('<4s4I', d[:20])
    recs = bytearray(d[20:20 + n * sz])
    strings = bytearray(d[20 + n * sz:])
    done = 0
    for i in range(n):
        sid = struct.unpack_from('<I', recs, i * sz)[0]
        if sid in texts:
            off = len(strings)
            strings += texts[sid].encode('utf-8') + b'\0'
            struct.pack_into('<I', recs, i * sz + 3 * 4, off)
            done += 1
    open(path_out, 'wb').write(struct.pack('<4s4I', magic, n, f, sz, len(strings)) + bytes(recs) + bytes(strings))
    return done


# m6-any-weapon-skills: class spells take any melee weapon in any hand. SAME RULE as
# modules/mod_arpg/src/Core/WeaponReq.cpp (WidenWeaponRequirement); the server prints its count on startup / .arpg reload
# ("weapon skills: widened=N mainhand=M") and this prints the client's - they must match.
CLASS_FAMILIES = {3, 4, 5, 6, 7, 8, 9, 10, 11, 15, 53}
MELEE_MASK = sum(1 << b for b in (0, 1, 4, 5, 6, 7, 8, 10, 13, 15, 17))
SERVER_DBC_DIR = os.path.dirname(SERVER_DBC)
REQ_OFFHAND = 0x01000000


def widen_weapon_requirement(family, item_class, sub_mask, inv_mask):
    """-> (sub_mask, inv_mask) or None when unchanged."""
    if family not in CLASS_FAMILIES or item_class != 2:
        return None
    any_sub = sub_mask in (0, -1)
    if not any_sub and not (sub_mask & MELEE_MASK):
        return None
    new_sub = sub_mask if any_sub else sub_mask | MELEE_MASK
    if new_sub == sub_mask and inv_mask == 0:
        return None
    return new_sub, 0


def conf_id_list(key, default):
    for path in CONF_FILES:
        if os.path.exists(path):
            m = re.search(rf'^\s*{re.escape(key)}\s*=\s*"?([0-9 ,]*)"?\s*$', open(path, encoding='utf-8', errors='replace').read(), re.M)
            if m:
                return [int(x) for x in re.split(r'[ ,]+', m.group(1).strip()) if x]
    return default


def read_dbc(name):
    d = open(os.path.join(SERVER_DBC_DIR, name), 'rb').read()
    magic, n, f, sz, ssz = struct.unpack('<4s4I', d[:20])
    assert magic == b'WDBC' and sz == f * 4, f'unexpected {name} layout'
    return d, n, f, sz


def build_weapon_skills(eq_out, misc_out):
    """Client SpellEquippedItems.dbc (widened rows) and SpellMisc.dbc (REQ_OFFHAND cleared for MainHandSpells).
    Only those fields change; every other byte stays Blizzard's. Returns (widened spells, main-hand spells)."""
    if conf_int('Arpg.AnyWeapon.Skills', 1) == 0:
        return None
    spell, sn, sf, ssz_ = read_dbc('Spell.dbc')
    eq, en, ef, esz = read_dbc('SpellEquippedItems.dbc')
    co, cn, cf, csz = read_dbc('SpellClassOptions.dbc')
    misc, mn, mf, msz = read_dbc('SpellMisc.dbc')
    rec = lambda d, i, sz, f: struct.unpack_from(f'<{f}i', d, 20 + i * sz)
    eq_index = {rec(eq, i, esz, ef)[0]: i for i in range(en)}
    family = {r[0]: r[6] for r in (rec(co, i, csz, cf) for i in range(cn))}
    misc_index = {rec(misc, i, msz, mf)[0]: i for i in range(mn)}
    eq_new, misc_new = bytearray(eq), bytearray(misc)
    widened, main_hand, row_users = set(), set(), {}
    want_main = set(conf_id_list('Arpg.AnyWeapon.MainHandSpells', [1329, 60103, 100130]))
    for i in range(sn):
        s = rec(spell, i, ssz_, sf)
        sid, co_id, eq_id, misc_id = s[0], s[14], s[16], s[24]
        if eq_id in eq_index:
            row_users.setdefault(eq_id, []).append(sid)
            r = rec(eq, eq_index[eq_id], esz, ef)
            w = widen_weapon_requirement(family.get(co_id, 0), r[3], r[5], r[4])
            if w:
                off = 20 + eq_index[eq_id] * esz
                struct.pack_into('<i', eq_new, off + 4 * 4, w[1])   # inventory type mask (field 4)
                struct.pack_into('<i', eq_new, off + 5 * 4, w[0])   # subclass mask (field 5)
                widened.add(sid)
        if sid in want_main and misc_id in misc_index:
            off = 20 + misc_index[misc_id] * msz + 6 * 4            # AttributesEx3 (field 6)
            a3 = struct.unpack_from('<I', misc_new, off)[0]
            if a3 & REQ_OFFHAND:
                struct.pack_into('<I', misc_new, off, a3 & ~REQ_OFFHAND)
                main_hand.add(sid)
    shared = [(k, v) for k, v in row_users.items() if len(v) > 1 and any(x in widened for x in v) and not all(x in widened for x in v)]
    if shared:
        print(f'  ! {len(shared)} SpellEquippedItems rows shared with non-class spells (those spells widen on the client too): {shared[:5]}')
    open(eq_out, 'wb').write(bytes(eq_new))
    open(misc_out, 'wb').write(bytes(misc_new))
    return len(widened), len(main_hand)


def build_mpq(files, mpq_path):
    """files: {inner name (bytes): local path}. Plain MPQ v4 entries (the unlocked exe loads plain files)."""
    s = ctypes.WinDLL(DLL)
    s.SFileCreateArchive.argtypes = [ctypes.c_wchar_p, wintypes.DWORD, wintypes.DWORD, ctypes.POINTER(ctypes.c_void_p)]
    s.SFileCreateArchive.restype = wintypes.BOOL
    s.SFileAddFileEx.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_char_p, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD]
    s.SFileAddFileEx.restype = wintypes.BOOL
    s.SFileCloseArchive.argtypes = [ctypes.c_void_p]
    V4, LISTFILE, ATTRIBUTES = 0x03000000, 0x00100000, 0x00200000
    COMPRESS, REPLACE, ZLIB = 0x00000200, 0x80000000, 0x02
    if os.path.exists(mpq_path):
        os.remove(mpq_path)
    h = ctypes.c_void_p()
    if not s.SFileCreateArchive(mpq_path, V4 | LISTFILE | ATTRIBUTES, 16, ctypes.byref(h)):
        raise SystemExit('SFileCreateArchive failed')
    try:
        for inner, local in files.items():
            if not s.SFileAddFileEx(h, local, inner, COMPRESS | REPLACE, ZLIB, ZLIB):
                raise SystemExit(f'SFileAddFileEx failed for {inner!r}')
    finally:
        s.SFileCloseArchive(h)


def main():
    if '--remove' in sys.argv:
        if os.path.exists(TARGET):
            os.remove(TARGET)
            print('removed', TARGET)
        return
    base, stride = conf_int('Arpg.Display.EnchantBase', 60000), conf_int('Arpg.Display.EnchantStride', 2000)
    effect_base = conf_int('Arpg.Display.EffectBase', 100000)
    os.makedirs(OUT_DIR, exist_ok=True)
    dbc = os.path.join(OUT_DIR, 'SpellItemEnchantment.client.dbc')
    unique_base = conf_int('Arpg.Display.UniqueBase', 200000)
    urows = unique_power_rows(unique_base)
    rows = build_dbc(dbc, base, stride, effect_base, urows)
    mpq = os.path.join(OUT_DIR, 'arpg-client-patch.MPQ')
    suffix_base = conf_int('Arpg.Display.SuffixBase', 30000)
    srows = suffix_rows(suffix_base)
    suffix = os.path.join(OUT_DIR, 'ItemRandomSuffix.client.dbc')
    build_suffix_dbc(suffix, srows)
    charbase = os.path.join(OUT_DIR, 'CharBaseInfo.client.dbc')
    pairs = build_charbaseinfo(charbase)
    files = {b'DBFilesClient\\SpellItemEnchantment.dbc': dbc, b'DBFilesClient\\ItemRandomSuffix.dbc': suffix,
             b'DBFilesClient\\CharBaseInfo.dbc': charbase}
    eq_dbc = os.path.join(OUT_DIR, 'SpellEquippedItems.client.dbc')
    misc_dbc = os.path.join(OUT_DIR, 'SpellMisc.client.dbc')
    wskills = build_weapon_skills(eq_dbc, misc_dbc)
    if wskills:
        files[b'DBFilesClient\\SpellEquippedItems.dbc'] = eq_dbc
        files[b'DBFilesClient\\SpellMisc.dbc'] = misc_dbc
    spells = 0
    write_potion_addon_data()   # the potion tooltip text now comes from ArpgUI (Potions.lua) instead of Spell.dbc
    write_affix_addon_data()    # corpse-loot preview lines (ArpgUI LootTooltip.lua)
    # Spell.dbc (potion tooltips) only on request: ArpgUI's Potions.lua shows the same text without a 13 MB file in the patch.
    # (2026-11-27: plain items / no new race-class pairs in game were the stock Wow-64.exe, not this file - use Wow-64-all.exe.)
    if '--spell' in sys.argv:
        spell = os.path.join(OUT_DIR, 'Spell.client.dbc')
        spells = build_spell_descriptions(spell)
        files[b'DBFilesClient\\Spell.dbc'] = spell
    build_mpq(files, mpq)
    print(f'built {mpq} ({os.path.getsize(mpq)} bytes): {rows} affix display rows (stats {base + 1}..{base + len(STATS) * stride - 1}, '
          f'effects {effect_base + EFFECT_ROWS_PER}..{effect_base + (len(EFFECTS) + 1) * EFFECT_ROWS_PER - 1})')
    print(f'  + {len(urows)} Unique power rows ({unique_base}+)')
    print(f'  + {len(srows)} random-suffix rows ({srows[0][0]}..{srows[-1][0]}) for 5 lines + item names')
    print(f'  + CharBaseInfo: {pairs} race x class pairs (any race, any class)')
    print(f'  + Spell: {spells} potion tooltips (% heal / mana, shared cooldown)')
    print(f'  + weapon skills: widened={wskills[0]} mainhand={wskills[1]} (server prints the same on startup)' if wskills
          else '  + weapon skills: off (Arpg.AnyWeapon.Skills 0)')
    if '--install' in sys.argv:
        shutil.copyfile(mpq, TARGET)
        print('installed', TARGET)
    if '--server' in sys.argv:
        # The server must know the suffix ids too (chat links validate them). One-time backup of Blizzard's file.
        backup = SERVER_SUFFIX_DBC + '.blizzard'
        if not os.path.exists(backup):
            shutil.copyfile(SERVER_SUFFIX_DBC, backup)
        build_suffix_dbc_from(backup, SERVER_SUFFIX_DBC, srows)
        print('server', SERVER_SUFFIX_DBC, '(original kept as .blizzard; restart the world server to load it)')
        # Same CharBaseInfo on the server (it doesn't read it today; kept identical so both sides agree).
        server_cb = os.path.join(os.path.dirname(SERVER_SUFFIX_DBC), 'CharBaseInfo.dbc')
        if not os.path.exists(server_cb + '.blizzard'):
            shutil.copyfile(server_cb, server_cb + '.blizzard')
        shutil.copyfile(charbase, server_cb)
        print('server', server_cb, '(original kept as .blizzard)')


if __name__ == '__main__':
    main()
