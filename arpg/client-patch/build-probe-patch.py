"""m2-affix-loot 4.6 MPQ spike: does the 5.4.8 client (Wow-64.exe) load a custom patch MPQ?

Builds a probe MPQ containing SpellItemEnchantment.dbc = the server's extracted (fully patched) copy + ONE extra row
(id PROBE_ID, text "ARPG patch probe +1 Stamina"), stored under the plain path and the "base\\" and "enUS\\" prefixes that
Cataclysm+ update archives use. --install copies it under every candidate name in TARGETS (one client restart tests all).
In game, ".arpg test probeitem <char> <itemId>" puts the enchantment on a new item: a green probe line = patches load.

    py -I arpg/client-patch/build-probe-patch.py            # build into %TEMP%\\arpg_mpq
    py -I arpg/client-patch/build-probe-patch.py --install  # also copy into the client under all candidate names
    py -I arpg/client-patch/build-probe-patch.py --remove   # delete every candidate from the client again
The server's own DBC is NOT changed (the probe row has no server-side effect).
"""
import ctypes, os, shutil, struct, sys
from ctypes import wintypes

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
DLL = os.path.join(ROOT, 'arpg', 'tools', 'bin', 'StormLib', 'StormLib.dll')   # current StormLib built from source (WDBXEditor's old one can't write patch entries)
SERVER_DBC = r'D:\WoWMoPProject\ServerData\data\dbc\SpellItemEnchantment.dbc'
CLIENT_DATA = r'D:\WoWMoPProject\MOP-5.4.8.18414-enUS-Repack\Data'
NAME = 'arpg-probe.MPQ'
# Findings 2026-11-22 (Wow-64.exe, SPEC 5): the client loads Data\enUS\wow-update-enUS-18299.MPQ (MPQ v4; numbers >= 18300
# are ignored by this exe, Stormforge's exe has that limit removed). A PLAIN file in it crashes the client at startup
# ("SFileReadFile ... Reached the end of the file"): files in update archives are patch entries. So the table is stored as a
# PTCH/COPY patch (whole-file replacement) with the MPQ patch-file flag. Test without logging in: start the client, wait
# ~50 s, look for a new report in the client's Errors\ folder.
# --plain (2026-11-22, with the unlocked exe Wow-64-all.exe from wowmodding.net): a plain DBC in wow-update-enUS-20000.MPQ,
# no patch wrapper/metadata - how 5.4.8 modders do it once the exe's MPQ/signature checks and number limit are removed.
PLAIN = '--plain' in sys.argv
TARGETS = [os.path.join(CLIENT_DATA, 'enUS', 'wow-update-enUS-20000.MPQ' if PLAIN else 'wow-update-enUS-18299.MPQ')]
ALL_TARGETS = [os.path.join(CLIENT_DATA, 'enUS', n) for n in ('wow-update-enUS-18299.MPQ', 'wow-update-enUS-20000.MPQ')]
INNER = [b'DBFilesClient\\SpellItemEnchantment.dbc']
OUT_DIR = os.path.join(os.environ['TEMP'], 'arpg_mpq')
PROBE_ID = 60000                             # reserved ARPG range starts at 60000 (max Blizzard id here: 5267)
PROBE_TEXT = b'ARPG patch probe +1 Stamina'
LOW_PROBE_ID = 5300                          # just above Blizzard's max id: tests whether the client caps ids
RENAME_ID, RENAME_TEXT = 74, b'+1 Agility (ARPG)'


def build_dbc(path_out):
    if '--same-size' in sys.argv:
        return build_dbc_same_size(path_out)
    d = bytearray(open(SERVER_DBC, 'rb').read())
    magic, n, fields, rsz, ssz = struct.unpack('<4s4i', d[:20])
    assert magic == b'WDBC' and fields == 26 and rsz == 104, 'unexpected SpellItemEnchantment layout'
    recs = d[20:20 + n * rsz]
    strings = d[20 + n * rsz:20 + n * rsz + ssz]
    ids = {struct.unpack_from('<I', recs, i * rsz)[0] for i in range(n)}
    new_rows = [(LOW_PROBE_ID, b'ARPG probe 5300 +1 Stamina'), (PROBE_ID, PROBE_TEXT)]
    for rid, _ in new_rows:
        if rid in ids:
            raise SystemExit(f'id {rid} already exists')

    def add_string(text):
        nonlocal strings
        off = len(strings)
        strings += text + b'\0'
        return off

    # Diagnostic: rename an EXISTING Blizzard row (74 = "+1 Agility"); if its tunic shows the new text, the patch is applied.
    for i in range(n):
        if struct.unpack_from('<I', recs, i * rsz)[0] == RENAME_ID:
            struct.pack_into('<I', recs, i * rsz + 11 * 4, add_string(RENAME_TEXT))
    # niiiiiiiiiisiiiiiiiixixfff: id, charges, type[3], amount[3], spellid[3], desc, aura, flags, gem, cond, skill, skillval,
    # reqlevel, maxlevel, unk20, scalinglevel, unk22, scalingpoints[3]
    rows = b''
    for rid, text in new_rows:
        row = struct.pack('<11I', rid, 0, 5, 0, 0, 1, 0, 0, 7, 0, 0)          # type 5 = stat, +1, stat 7 = stamina
        row += struct.pack('<I', add_string(text))
        row += struct.pack('<11i', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        row += struct.pack('<3f', 0, 0, 0)
        assert len(row) == rsz
        rows += row
    out = struct.pack('<4s4i', b'WDBC', n + len(new_rows), fields, rsz, len(strings)) + bytes(recs) + rows + bytes(strings)
    open(path_out, 'wb').write(out)


def build_dbc_same_size(path_out):
    """Round 9 diagnostic: change only the TEXT of row 74 in place ('+$k1 Agility' -> '+$k1 AGILITY', same length).
    The table keeps its exact size and row count. If this works but growing tables crash, the client expects the old size."""
    d = bytearray(open(SERVER_DBC, 'rb').read())
    magic, n, fields, rsz, ssz = struct.unpack('<4s4i', d[:20])
    strings_at = 20 + n * rsz
    for i in range(n):
        if struct.unpack_from('<I', d, 20 + i * rsz)[0] == RENAME_ID:
            off = strings_at + struct.unpack_from('<I', d, 20 + i * rsz + 11 * 4)[0]
            old = bytes(d[off:d.index(b'\0', off)])
            assert old.endswith(b'Agility'), old
            d[off:off + len(old)] = old.replace(b'Agility', b'AGILITY')
    open(path_out, 'wb').write(bytes(d))


def build_ptch_copy(new_path, out_path):
    """Wrap a whole file as a Blizzard PTCH patch that replaces the base file. 'COPY' means "unchanged" to Blizzard's
    loader (it ignores the payload), so this is a BSD0 (Blizzard bsdiff) patch with one control entry that takes every byte
    from the extra block: ctrl (add 0, extra newSize, seek 0), empty data block, extra block = the new file. Uncompressed."""
    import hashlib
    new = open(new_path, 'rb').read()
    base = open(SERVER_DBC, 'rb').read()          # what the client has after Blizzard's own updates
    # Real bsdiff like Blizzard's (round 8): "add" the byte difference against the whole old file, then the appended bytes
    # as "extra". (Round 3-7 used extra-only = 406 KB extra block; the client crashed on it.)
    assert len(new) >= len(base), 'new table must not be shorter than the base'
    diff = bytes((new[i] - base[i]) & 0xFF for i in range(len(base)))
    ctrl = struct.pack('<III', len(base), len(new) - len(base), 0)
    bsdiff = b'BSDIFF40' + struct.pack('<QQQ', len(ctrl), len(diff), len(new)) + ctrl + diff + new[len(base):]
    # Blizzard RLE-compresses BSD0 payloads; the client crashed ("Reached the end of the file") on an uncompressed one.
    # XFRM size counts the compressed bytes; the PTCH header's patch size counts the decompressed ones.
    packed = rle_compress(bsdiff)
    xfrm = b'XFRM' + struct.pack('<I', 12 + len(packed)) + b'BSD0' + packed
    md5 = b'MD5_' + struct.pack('<I', 0x28) + hashlib.md5(base).digest() + hashlib.md5(new).digest()
    body = md5 + xfrm
    size_of_patch_data = 16 + len(md5) + 12 + len(bsdiff)          # decompressed size (StormLib MPQ_PATCH_HEADER + data)
    header = b'PTCH' + struct.pack('<III', size_of_patch_data, len(base), len(new))
    open(out_path, 'wb').write(header + body)


def rle_compress(data):
    """Blizzard patch RLE (inverse of StormLib's Decompress_RLE): DWORD decompressed size, then tokens:
    0x80|(n-1) followed by n literal bytes (n <= 128), or (n-1) for n zero bytes (n <= 128)."""
    out = bytearray(struct.pack('<I', len(data)))
    i, n = 0, len(data)
    while i < n:
        if data[i] == 0:
            j = i
            while j < n and data[j] == 0 and j - i < 128:
                j += 1
            out.append(j - i - 1)
            i = j
        else:
            j = i
            while j < n and data[j] != 0 and j - i < 128:
                j += 1
            out.append(0x80 | (j - i - 1))
            out += data[i:j]
            i = j
    return bytes(out)


def build_mpq(dbc_path, mpq_path):
    s = ctypes.WinDLL(DLL)
    s.SFileCreateArchive.argtypes = [ctypes.c_wchar_p, wintypes.DWORD, wintypes.DWORD, ctypes.POINTER(ctypes.c_void_p)]
    s.SFileCreateArchive.restype = wintypes.BOOL
    s.SFileAddFileEx.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_char_p, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD]
    s.SFileAddFileEx.restype = wintypes.BOOL
    s.SFileCloseArchive.argtypes = [ctypes.c_void_p]
    s.SFileCloseArchive.restype = wintypes.BOOL
    MPQ_CREATE_ARCHIVE_V4, MPQ_CREATE_LISTFILE, MPQ_CREATE_ATTRIBUTES = 0x03000000, 0x00100000, 0x00200000  # v4 like Blizzard update archives
    MPQ_FILE_COMPRESS, MPQ_FILE_REPLACEEXISTING, MPQ_COMPRESSION_ZLIB = 0x00000200, 0x80000000, 0x02
    if os.path.exists(mpq_path):
        os.remove(mpq_path)
    h = ctypes.c_void_p()
    if not s.SFileCreateArchive(mpq_path, MPQ_CREATE_ARCHIVE_V4 | MPQ_CREATE_LISTFILE | MPQ_CREATE_ATTRIBUTES, 16, ctypes.byref(h)):
        raise SystemExit(f'SFileCreateArchive failed ({ctypes.GetLastError()})')
    # (attributes) with exactly Blizzard's fields: CRC32 | MD5 | PATCH_BIT = 0x0D (StormLib's default 0x0F adds file
    # times; a client that assumes Blizzard's layout would read the patch bits at the wrong offset).
    s.SFileSetAttributes.argtypes = [ctypes.c_void_p, wintypes.DWORD]
    s.SFileSetAttributes.restype = wintypes.BOOL
    if not s.SFileSetAttributes(h, 0x01 | 0x04 | 0x08):
        raise SystemExit('SFileSetAttributes failed')
    MPQ_FILE_PATCH_FILE, MPQ_FILE_SINGLE_UNIT = 0x00100000, 0x01000000
    ok, err = True, 0
    if PLAIN:
        for inner in INNER:
            if not s.SFileAddFileEx(h, dbc_path, inner, MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING,
                                    MPQ_COMPRESSION_ZLIB, MPQ_COMPRESSION_ZLIB):
                ok, err = False, ctypes.GetLastError()
        s.SFileCloseArchive(h)
        if not ok:
            raise SystemExit(f'SFileAddFileEx failed ({err})')
        return
    ptch = dbc_path + '.ptch'
    build_ptch_copy(dbc_path, ptch)
    # Archive metadata every Blizzard update archive carries (seen in wow-update-enUS-17645.MPQ). Without (patch_metadata)
    # the client doesn't treat the archive as a patch archive and reads patch entries as plain files ("end of the file").
    meta = {b'(patch_metadata)': b'enUS',
            b'component.wow-enus.txt': b'<componentinfo format="1">\n    <component name="wow-enus" version="18299" />\n</componentinfo>\n'}
    for inner, content in meta.items():
        tmp = os.path.join(OUT_DIR, inner.decode().strip('()').replace('.', '_') + '.bin')
        open(tmp, 'wb').write(content)
        if not s.SFileAddFileEx(h, tmp, inner, MPQ_FILE_COMPRESS | MPQ_FILE_SINGLE_UNIT | MPQ_FILE_REPLACEEXISTING,
                                MPQ_COMPRESSION_ZLIB, MPQ_COMPRESSION_ZLIB):
            ok, err = False, ctypes.GetLastError()
    for inner in INNER:
        # Current StormLib sees PTCH data and sets the patch flag + writes TPatchInfo itself. SINGLE_UNIT like Blizzard's
        # entries (0x81100200): the client crashed ("Reached the end of the file") on a sectored entry without sector CRCs.
        if not s.SFileAddFileEx(h, ptch, inner, MPQ_FILE_COMPRESS | MPQ_FILE_SINGLE_UNIT | MPQ_FILE_REPLACEEXISTING,
                                MPQ_COMPRESSION_ZLIB, MPQ_COMPRESSION_ZLIB):
            ok, err = False, ctypes.GetLastError()
    s.SFileCloseArchive(h)
    if not ok:
        raise SystemExit(f'SFileAddFileEx failed ({err})')


def main():
    if '--remove' in sys.argv:
        for t in ALL_TARGETS:
            if os.path.exists(t):
                os.remove(t)
                print('removed', t)
        return
    os.makedirs(OUT_DIR, exist_ok=True)
    dbc = os.path.join(OUT_DIR, 'SpellItemEnchantment.dbc')
    mpq = os.path.join(OUT_DIR, NAME)
    build_dbc(dbc)
    build_mpq(dbc, mpq)
    print(f'built {mpq} ({os.path.getsize(mpq)} bytes), probe enchant id {PROBE_ID}')
    if '--install' in sys.argv:
        for t in TARGETS:
            if os.path.exists(t) and os.path.getsize(t) != os.path.getsize(mpq):
                print('SKIP (a different file already has this name):', t)
                continue
            shutil.copyfile(mpq, t)
            print('installed', t)
        print('(remove all with --remove)')


if __name__ == '__main__':
    main()
