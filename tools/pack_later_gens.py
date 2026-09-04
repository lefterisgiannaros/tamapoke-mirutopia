#!/usr/bin/env python3
"""Build Unova / Kalos / Alola sprite paks the public release never shipped.

Unpacks the four local Kanto-Sinnoh bundles so thumbs.bin can stay complete,
fetches the missing species from PMD SpriteCollab in parallel, then writes
web/sprites-{unova,kalos,alola}.pak for the local installer.
"""
import os
import struct
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
os.chdir(HERE)

from pack_pmd import pack, DEX_COUNT
from dex_data import REGIONS as DEX_REGIONS
from pack_bundle import write_pak, dex_of, WEB, MONS

NO_ART = {514, 516, 520, 522, 523, 538, 558, 564, 565, 591, 592, 616, 626,
          668, 732, 735, 741, 756, 765}
LATER = ('unova', 'kalos', 'alola')
WEB_DIR = os.path.normpath(os.path.join(HERE, '..', 'web'))
WORKERS = 10


def unpack_pak(path, dest):
    with open(path, 'rb') as f:
        if f.read(4) != b'TPAK':
            raise SystemExit('not a TPAK: ' + path)
        count = struct.unpack('<H', f.read(2))[0]
        items = []
        for _ in range(count):
            nl = f.read(1)[0]
            name = f.read(nl).decode()
            size = struct.unpack('<I', f.read(4))[0]
            items.append((name, size))
        os.makedirs(dest, exist_ok=True)
        n = 0
        for name, size in items:
            data = f.read(size)
            out = os.path.join(dest, os.path.basename(name))
            if not os.path.exists(out):
                with open(out, 'wb') as o:
                    o.write(data)
                n += 1
        print(f'unpacked {os.path.basename(path)}: {count} files, wrote {n} new')


def one(n, shiny):
    label = f'#{n:03d}{" shiny" if shiny else ""}'
    try:
        pack(n, shiny)
        return label, None
    except Exception as e:
        return label, str(e)


def main():
    os.makedirs(MONS, exist_ok=True)
    for name in ('kanto', 'johto', 'hoenn', 'sinnoh'):
        pak = os.path.join(WEB_DIR, f'sprites-{name}.pak')
        if os.path.exists(pak):
            unpack_pak(pak, MONS)

    span = {name.lower(): (lo, hi) for name, lo, hi, _ in DEX_REGIONS}
    nums = []
    for r in LATER:
        lo, hi = span[r]
        nums.extend(d for d in range(lo, hi + 1) if d not in NO_ART)
    jobs = [(n, sh) for n in nums for sh in (False, True)]
    print(f'packing {len(nums)} species x2 = {len(jobs)} files, {WORKERS} workers')

    fail = []
    done = 0
    with ThreadPoolExecutor(max_workers=WORKERS) as ex:
        futs = [ex.submit(one, n, sh) for n, sh in jobs]
        for fut in as_completed(futs):
            label, err = fut.result()
            done += 1
            if err:
                fail.append((label, err))
                print(f'  FALLO {label}: {err}  ({done}/{len(jobs)})')
            elif done % 20 == 0 or done == len(jobs):
                print(f'  {done}/{len(jobs)}')

    print('FALLOS:', fail if fail else 'none')

    import make_thumbs
    make_thumbs.main()

    import glob
    files = sorted(glob.glob(os.path.join(MONS, '*.bin')))
    shared = [f for f in files if dex_of(f) == 0]
    for name, lo, hi, _ in DEX_REGIONS:
        key = name.lower()
        if key not in LATER:
            continue
        mine = [f for f in files if lo <= dex_of(f) <= hi]
        if not mine:
            print(f'{key}: no sprites, skipped')
            continue
        out = os.path.join(WEB, f'sprites-{key}.pak')
        total = write_pak(out, sorted(mine + shared))
        print(f'{out}: {len(mine) + len(shared)} files, {total / 1048576:.1f} MB')


if __name__ == '__main__':
    main()
