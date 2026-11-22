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
    build_mpq({b'DBFilesClient\\SpellItemEnchantment.dbc': dbc, b'DBFilesClient\\ItemRandomSuffix.dbc': suffix}, mpq)
    print(f'built {mpq} ({os.path.getsize(mpq)} bytes): {rows} affix display rows (stats {base + 1}..{base + len(STATS) * stride - 1}, '
          f'effects {effect_base + EFFECT_ROWS_PER}..{effect_base + (len(EFFECTS) + 1) * EFFECT_ROWS_PER - 1})')
    print(f'  + {len(urows)} Unique power rows ({unique_base}+)')
    print(f'  + {len(srows)} random-suffix rows ({srows[0][0]}..{srows[-1][0]}) for 5 lines + item names')
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


if __name__ == '__main__':
    main()
