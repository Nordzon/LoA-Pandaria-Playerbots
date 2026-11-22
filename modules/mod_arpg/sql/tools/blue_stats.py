"""Measures stats on REAL items (item_template) per required-level checkpoint, to calibrate flat affix values
(docs/BALANCE.md §16, BLUE_LINE_PCT_OF_B in gen_affix_sql.py). Read-only.
    py -I modules/mod_arpg/sql/tools/blue_stats.py [quality=3]      # chest-normalised table + % of B
    py -I modules/mod_arpg/sql/tools/blue_stats.py 3 --slots         # raw per-slot medians (what tooltips show)
Skips items with Blizzard random stats and heirlooms. Needs arpg/scripts/mysql.local.cnf and RandPropPoints.dbc.
"""
import os, struct, subprocess, sys
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
MYSQL = r'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe'
CNF = os.path.join(ROOT, 'arpg', 'scripts', 'mysql.local.cnf')
DBC = r'D:\WoWMoPProject\ServerData\data\dbc\RandPropPoints.dbc'
# RandPropPoints: id(ilvl), epic[5], rare[5], good[5]
raw = open(DBC, 'rb').read()
magic, nrec, nfld, rsz, ssz = struct.unpack('<4s4i', raw[:20])
rpp = {}
for i in range(nrec):
    v = struct.unpack('<16I', raw[20 + i*rsz: 20 + i*rsz + 64])
    rpp[v[0]] = (v[1:6], v[6:11], v[11:16])
SLOTCOL = {1:0,4:0,5:0,7:0,15:0,17:0,20:0,25:0,3:1,6:1,8:1,10:1,12:1,2:2,9:2,11:2,14:2,16:2,23:2,13:3,21:3,22:3}
SLOTNAME = {1:'head',5:'chest',20:'chest',7:'legs',3:'shoulder',6:'waist',8:'feet',10:'hands',9:'wrist',2:'neck',11:'finger',16:'back',13:'1h',21:'1h',22:'1h',17:'2h',14:'shield',23:'offhand',15:'ranged',25:'ranged',4:'shirt'}
MAIN = {3:'agi',4:'str',5:'int'}; SEC = {13:'dodge',14:'parry',31:'hit',32:'crit',36:'haste',37:'exp',49:'mastery',6:'spirit'}
q = ("SELECT entry,Quality,ItemLevel,RequiredLevel,InventoryType," + ",".join(f"stat_type{i},stat_value{i}" for i in range(1,11)) +
     " FROM item_template WHERE class IN (2,4) AND Quality IN (2,3,4) AND RequiredLevel BETWEEN 1 AND 90"
     " AND RandomProperty=0 AND RandomSuffix=0 AND ScalingStatDistribution=0 AND stat_value1<>0 AND InventoryType<>12")
out = subprocess.run([MYSQL, '--defaults-extra-file=' + CNF, '-D', 'world', '-B', '-N', '-e', q], capture_output=True, text=True).stdout
items = []
for line in out.splitlines():
    f = list(map(int, line.split('\t')))
    entry, qual, ilvl, req, inv = f[:5]
    if inv not in SLOTCOL or ilvl not in rpp: continue
    col = SLOTCOL[inv]
    sup = rpp[ilvl][1]
    if sup[col] == 0 or sup[0] == 0: continue
    norm = sup[0] / sup[col]          # scale this slot up to chest size
    stats = {}
    for i in range(10):
        t, v = f[5+2*i], f[6+2*i]
        if t and v > 0: stats[t] = stats.get(t, 0) + v
    main = max((stats.get(k, 0) for k in MAIN), default=0)
    sta = stats.get(7, 0)
    secs = sorted((v for k, v in stats.items() if k in SEC), reverse=True)
    items.append(dict(entry=entry, q=qual, ilvl=ilvl, req=req, slot=SLOTNAME[inv], norm=norm, main=main*norm, sta=sta*norm,
                      sec1=(secs[0] if secs else 0)*norm, sec2=(secs[1] if len(secs) > 1 else 0)*norm, nsec=len(secs),
                      raw_main=main, raw_sta=sta, raw_sec1=secs[0] if secs else 0))
def pct(xs, p):
    xs = sorted(xs); 
    return xs[min(len(xs)-1, int(p*(len(xs)-1)+0.5))] if xs else 0
def curve(L):
    pts = [(1,6),(59,64),(60,66),(70,115),(80,213),(85,359),(90,463)]
    for (a,b),(c,d) in zip(pts, pts[1:]):
        if a <= L <= c: return round(b + (d-b)*(L-a)/(c-a))
    return pts[-1][1]
def slot_table():
    order = ['head','chest','legs','shoulder','hands','waist','feet','wrist','neck','back','finger','1h','2h','shield','offhand']
    levels = (10, 20, 30, 40, 50, 60, 70, 80, 85, 90)
    print(f"Quality {qual} RAW medians: main / stamina / best secondary (item count)")
    print(f"{'slot':>9} | " + " | ".join(f"{'lvl ' + str(L):>16}" for L in levels))
    for sl in order:
        row = []
        for L in levels:
            lo, hi = (89, 90) if L == 90 else (L - 2, L + 2)
            b = [it for it in items if it['q'] == qual and it['slot'] == sl and lo <= it['req'] <= hi and it['raw_main'] > 0]
            if not b:
                row.append(f"{'-':>16}"); continue
            m = lambda k: pct([it[k] for it in b if it[k] > 0], .5) if any(it[k] > 0 for it in b) else 0
            row.append(f"{m('raw_main')}/{m('raw_sta')}/{m('raw_sec1')} ({len(b)})".rjust(16))
        print(f"{sl:>9} | " + " | ".join(row))
checkpoints = [5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90]
args = [a for a in sys.argv[1:] if not a.startswith('--')]
qual = int(args[0]) if args else 3
if '--slots' in sys.argv:
    slot_table(); sys.exit(0)
print(f"Quality {qual}. Values normalised to CHEST size (Blizzard slot budget). med [p10-p90]. B = our epic chest budget at that player level today.")
print(f"{'lvl':>3} {'n':>4} {'ilvl':>9} | {'main stat':>17} | {'stamina':>17} | {'best secondary':>17} | {'2nd secondary':>15} | {'B':>5} {'main%B':>6} {'sec%B':>6}")
for L in checkpoints:
    band = [it for it in items if it['q'] == qual and L-2 <= it['req'] <= L+2 and it['main'] > 0]
    if L == 90: band = [it for it in items if it['q'] == qual and it['req'] in (89, 90) and it['main'] > 0]
    if not band: print(f"{L:>3}    0"); continue
    il = [it['ilvl'] for it in band]
    def f(key):
        xs = [it[key] for it in band if it[key] > 0]
        return f"{pct(xs,.5):>5.0f} [{pct(xs,.1):>4.0f}-{pct(xs,.9):>4.0f}]" if xs else f"{'-':>17}"
    ci = curve(L); B = rpp.get(ci, ((0,),))[0][0]
    m = pct([it['main'] for it in band], .5); s1 = pct([it['sec1'] for it in band if it['sec1'] > 0], .5)
    print(f"{L:>3} {len(band):>4} {pct(il,.1):>4}-{pct(il,.9):<4} | {f('main')} | {f('sta')} | {f('sec1')} | {f('sec2')[:15]:>15} | {B:>5} {100*m/B if B else 0:>5.1f}% {100*s1/B if B else 0:>5.1f}%")
