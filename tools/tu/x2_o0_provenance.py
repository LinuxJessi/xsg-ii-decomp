#!/usr/bin/env python3
"""Whose code is X2's -O0 zone? Writes reports/x2-o0-provenance.md.

    python3 -B tools/tu/x2_o0_provenance.py          # about 5 minutes; objects are kept and reused

Measures, from the bytes only:

- which strings the zone's code really references (`lui` paired with its first use, no chains);
- `$fp` frames and `$gp`-relative instructions per zone of the image, and `$fp` frames in X1;
- who calls the zone, whom it calls, and which TUs reach game code through their own calls;
- X1's published C compiled at -O0 under X2's contract, compared with every function of the zone.

The X1 tree is only read. The objects go to .work/x2-o0-provenance/. A match here is evidence about
where the code came from and a lead for naming it. It is not exactness: relocated fields are
masked, and only the whole-file gate decides.
"""
import bisect
import json
import re
import struct
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.tu.x2_calibrate import ElfObject  # noqa: E402

X1 = Path('/home/pc/xenosaga1-port/xenosaga-i-decomp')
X1_ELF = X1 / 'game/extracted-iso/SLUS_204.69'
X2_ELF = ROOT / 'game/extracted-iso/SLUS_208.92'
OUT = ROOT / '.work/x2-o0-provenance'
REPORT = ROOT / 'reports/x2-o0-provenance.md'
HOMOLOGS = ROOT / 'config/symbols/x1-o0-homologs.json'
MAP = ROOT / '.work/tu-migration/map-main/tu-map-main.json'
CC = ROOT / '.work/toolchains/ee-gcc2.96-realconv-lp7-fsv12-v1/bin'
AS = ROOT / '.work/toolchains/ee-legacy-binutils-la29-vsqrt-nolabel-laimm-swap-v1/bin/ee-as'
LO, HI = 0x1a75a8, 0x1d5440
TEXT_LO, TEXT_HI = 0x120000, 0x2b3740
TEXT_DELTA = 0x11f000          # .text and the data sections share this VA-to-file delta in main
STRINGS_LO, STRINGS_HI = 0x2b5d80, 0x69e180
ZONES = ((TEXT_LO, 0x138298, 'Sony SDK (low)'), (0x138298, 0x13b7a0, 'KORG client'),
         (0x13b7a0, 0x194638, 'CRI / Sofdec'), (0x194638, LO, 'game'), (LO, HI, '-O0 zone'),
         (HI, 0x29f008, 'game'), (0x29f008, 0x2a1578, 'Sony SDK (high)'), (0x2a1578, TEXT_HI, 'C runtime'))
FP_FRAME = 0x03a0f02d          # daddu $fp, $sp, $zero
LOADS_STORES = {0x20, 0x21, 0x23, 0x24, 0x25, 0x27, 0x37, 0x1e, 0x1f, 0x28, 0x29, 0x2b, 0x3f, 0x31, 0x39}
ADDIU = {8, 9, 0x19}
STRONG_BYTES = 64              # shorter -O0 bodies collide: they are mostly the frame
STRONG_VU = 8                  # shorter VU sequences are common shapes


def zone_of(va):
    return next((name for lo, hi, name in ZONES if lo <= va < hi), None)


class Image:
    def __init__(self):
        self.data = X2_ELF.read_bytes()
        count = (TEXT_HI - TEXT_LO) // 4
        self.words = struct.unpack_from(f'<{count}I', self.data, TEXT_LO - TEXT_DELTA)
        tus = json.loads(MAP.read_bytes())['tus']
        self.size, self.tu_of = {}, {}
        for tu in tus:
            for function in tu['functions']:
                va = int(function['va'], 16)
                self.size[va] = function['size']
                self.tu_of[va] = tu['id']
        self.starts = sorted(self.size)
        self.zone_tus = sorted({self.tu_of[va] for va in self.starts if LO <= va < HI})

    def word(self, va):
        return self.words[(va - TEXT_LO) // 4]

    def owner(self, va):
        return self.starts[bisect.bisect_right(self.starts, va) - 1]

    def tu_at(self, va):
        return self.tu_of[self.owner(va)]

    def string_at(self, va):
        """The C string that starts exactly at `va`, or None."""
        if not STRINGS_LO <= va < STRINGS_HI:
            return None
        offset = va - TEXT_DELTA
        if self.data[offset - 1] != 0:
            return None
        end = self.data.find(b'\0', offset, offset + 300)
        text = self.data[offset:end]
        if len(text) < 3 or any((c < 32 or c > 126) and c not in (9, 10, 13) for c in text):
            return None
        return text.decode()


def address_uses(image, lo, hi):
    """(lui address, value) for each `lui` and its first use. Never chains additions."""
    for at in range(lo, hi, 4):
        word = image.word(at)
        if word >> 26 != 0xf:
            continue
        register, high = (word >> 16) & 31, (word & 0xffff) << 16
        for use in range(at + 4, min(at + 56, hi), 4):
            other = image.word(use)
            op, base, target, low = other >> 26, (other >> 21) & 31, (other >> 16) & 31, other & 0xffff
            signed = (low ^ 0x8000) - 0x8000
            if op in ADDIU | LOADS_STORES and base == register:
                yield at, (high + signed) & 0xffffffff
                break
            if op == 0xd and base == register:
                yield at, high | low
                break
            if op == 0xf and target == register:
                break


def zone_statistics(image):
    stats = defaultdict(Counter)
    for va, size in image.size.items():
        row = stats[zone_of(va)]
        row['functions'] += 1
        row['bytes'] += size
        row['not_8_aligned'] += va % 8 != 0
        row['fp_frames'] += FP_FRAME in [image.word(a) for a in range(va, min(va + 48, va + size), 4)]
    for at in range(TEXT_LO, TEXT_HI, 4):
        word = image.word(at)
        if (word >> 21) & 31 == 28 and word >> 26 in ADDIU | LOADS_STORES:
            stats[zone_of(at)]['gp_relative'] += 1
    return stats


def x1_functions():
    """(name, words) of every named X1 function, and how many open a $fp frame."""
    data = X1_ELF.read_bytes()
    shoff, shnum = struct.unpack_from('<I', data, 32)[0], struct.unpack_from('<H', data, 48)[0]
    sections = [struct.unpack_from('<10I', data, shoff + 40 * i) for i in range(shnum)]
    symtab = next(s for s in sections if s[1] == 2)
    strings_row = sections[symtab[6]]
    functions, frames = [], 0
    for k in range(symtab[5] // 16):
        name_at, value, size, info, _, shndx = struct.unpack_from('<IIIBBH', data, symtab[4] + k * 16)
        if info & 15 == 2 and size >= 8 and shndx and shndx < shnum and sections[shndx][1] != 8:
            start = sections[shndx][4] + value - sections[shndx][3]
            words = struct.unpack_from(f'<{size // 4}I', data, start)
            frames += FP_FRAME in words[:12]
            name_start = strings_row[4] + name_at
            functions.append((data[name_start:data.index(b'\0', name_start)].decode(errors='replace'), words))
    return functions, frames


def vu_instructions(words):
    """The COP2 computational instructions: inline asm keeps them, whatever the -O level."""
    return tuple(word for word in words if word >> 26 == 0x12 and (word >> 25) & 1)


def vu_matches(image, x1):
    """{va: X1 names} where a whole function's VU sequence equals an X1 function's."""
    known = defaultdict(set)
    for name, words in x1:
        sequence = vu_instructions(words)
        if len(sequence) >= STRONG_VU:
            known[sequence].add(name)
    found = {}
    for va, size in image.size.items():
        if LO <= va < HI:
            sequence = vu_instructions([image.word(a) for a in range(va, va + size, 4)])
            if sequence in known:
                found[va] = (len(sequence), sorted(known[sequence]))
    return found


def call_graph(image):
    into, out, callees = Counter(), Counter(), Counter()
    tu_calls, game_callers = defaultdict(set), set()
    for at in range(TEXT_LO, TEXT_HI, 4):
        word = image.word(at)
        if word >> 26 != 3:
            continue
        target = (word & 0x3ffffff) << 2
        caller, callee = zone_of(at), zone_of(target)
        if callee == '-O0 zone' and caller != '-O0 zone':
            into[caller] += 1
        elif caller == '-O0 zone':
            if callee != '-O0 zone':
                out[callee] += 1
                callees[target] += 1
                if callee == 'game':
                    game_callers.add(image.tu_at(at))
            elif image.tu_at(at) != image.tu_at(target):
                tu_calls[image.tu_at(at)].add(image.tu_at(target))
    reaches = set(game_callers)
    changed = True
    while changed:
        changed = False
        for tu, others in tu_calls.items():
            if tu not in reaches and others & reaches:
                reaches.add(tu)
                changed = True
    return into, out, callees, game_callers, reaches


def compile_o0(source, g_flag, out):
    """X2's -O0 contract, with cc_tu.sh's argv shapes. Returns an error text or None."""
    if out.is_file():
        return None
    environment = dict(PATH='/usr/bin:/bin', LC_ALL='C')
    work = X1 / 'build/main'
    i_file, s_file = out.with_suffix('.i'), out.with_suffix('.s')
    steps = ([CC / 'ee-gcc', f'-B{CC}/', '-nostdinc', '-fno-builtin', '-E', '-O0', g_flag, '-I', 'include',
              '-I', X1 / 'include', X1 / source, '-o', i_file],
             [CC / 'ee-gcc', f'-B{CC}/', '-nostdinc', '-fno-builtin', '-S', '-O0', g_flag, i_file, '-o', s_file],
             [AS, '-EL', '-m5900', '-mabi=eabi', g_flag, '-g', '-I', '.', '-I', work, '-I', X1, '-o', out, s_file])
    for step in steps:
        run = subprocess.run(['timeout', '-k', '5', '30'] + [str(a) for a in step], cwd=work, env=environment,
                             capture_output=True, text=True)
        if run.returncode:
            out.unlink(missing_ok=True)
            return (run.stdout + run.stderr)[-200:]
    return None


def compare_x1_c(image):
    """{va: [(X1 function, -G flag, X1 file, offset in the object)]} of equal reloc-masked bodies."""
    by_size = defaultdict(list)
    for va, size in image.size.items():
        if LO <= va < HI:
            by_size[size].append(va)
    build = json.loads((X1 / 'config/tu-build.json').read_text())
    sources = [t['path'] for t in build['tus'] if t.get('unit') == 'main' and (t.get('path') or '').endswith('.c')
               and (X1 / t['path']).is_file()]
    OUT.mkdir(parents=True, exist_ok=True)
    matches, failures = defaultdict(list), []
    for source in sources:
        text = (X1 / source).read_text(errors='replace')
        asm_bodies = set(re.findall(r'INCLUDE_ASM\s*\(\s*(?:"[^"]*"\s*,\s*)?"?([A-Za-z_]\w*)"?\s*\)', text))
        for g_flag in ('-G0', '-G8'):
            out = OUT / f'{Path(source).stem}{g_flag}.o'
            error = compile_o0(source, g_flag, out)
            if error:
                failures.append((source, g_flag, error))
                continue
            obj = ElfObject(out)
            for name, (value, length) in obj.functions.items():
                length -= length % 4
                if name in asm_bodies or length < 8:
                    continue
                for va in by_size.get(length, ()):
                    if all((struct.unpack_from('<I', obj.text, value + at)[0] & ~obj.masks.get(value + at, 0)) ==
                           (image.word(va + at) & ~obj.masks.get(value + at, 0)) for at in range(0, length, 4)):
                        matches[va].append((name, g_flag, source, value))
    return matches, failures, len(sources)


def order_kept(rows):
    """How many neighbours, in X2 address order, keep their X1 file order."""
    offsets = [offset for _, offset in sorted(rows)]
    return sum(a < b for a, b in zip(offsets, offsets[1:])), max(len(offsets) - 1, 0)


def render(image, strings, stats, x1_counts, vu, calls, matches, failures, source_count):
    into, out, callees, game_callers, reaches = calls
    zone_functions = {va: size for va, size in image.size.items() if LO <= va < HI}
    strong = {va: rows for va, rows in matches.items() if image.size[va] >= STRONG_BYTES}
    one_name = {va: rows for va, rows in strong.items() if len({row[0] for row in rows}) == 1}
    per_file = defaultdict(list)
    for va, rows in one_name.items():
        per_file[rows[0][2]].append((va, rows[0][3]))
    tu_rows = []
    for tu in image.zone_tus:
        vas = [va for va in zone_functions if image.tu_of[va] == tu]
        names = [one_name[va][0][0] for va in sorted(vas) if va in one_name]
        tu_rows.append(f"| `{tu}` | {len(vas)} | {sum(zone_functions[v] for v in vas):,} | "
                       f"{sum(v in strong for v in vas)} | {'yes' if tu in game_callers else ''} | "
                       f"{'yes' if tu in reaches else ''} | {', '.join(f'`{n}`' for n in names[:3])} |")
    lines = ['# X2 `-O0` zone: whose code is it', '',
             'Generated by `tools/tu/x2_o0_provenance.py`. Do not edit by hand.', '',
             f'Main `{LO:#x}-{HI:#x}`: {len(zone_functions)} functions, {sum(zone_functions.values()):,} bytes, '
             f'{len(image.zone_tus)} TUs.', '',
             '## Answer', '',
             "It is not Sony's. It is the game's own code, the same `xgl` and `nml` source families that X1 "
             'links, built without optimization.', '',
             '## X1 C reproduces it', '',
             f'X1 has {source_count} C files in main. Each was compiled at `-O0` under X2\'s contract, with `-G0` '
             'and `-G8`, and each function was compared with every function of the zone, relocated fields masked.',
             '',
             f'- **{len(strong)} functions of {STRONG_BYTES} bytes or more are equal '
             f'({sum(image.size[v] for v in strong):,} bytes).** {len(one_name)} of them have a single candidate name.',
             f'- All sizes: {len(matches)} of {len(zone_functions)} functions, '
             f'{sum(image.size[v] for v in matches):,} of {sum(zone_functions.values()):,} bytes. Below '
             f'{STRONG_BYTES} bytes a body is mostly its frame and different functions collide, so those do not '
             'count as evidence.',
             f'- Compile failures: {len(failures)}.', '',
             '| VA | Bytes | TU | X1 function | X1 file |', '| --- | ---: | --- | --- | --- |']
    for va in sorted(one_name, key=lambda v: -image.size[v])[:15]:
        name, _, source, _ = one_name[va][0]
        lines.append(f'| `{va:#x}` | {image.size[va]:,} | `{image.tu_of[va]}` | `{name}` | `{source}` |')
    lines += ['', 'X1 file order survives, which a chance match would not do:', '',
              '| X1 file | Matches | Neighbours in X1 order | X2 TUs |', '| --- | ---: | ---: | --- |']
    for source, rows in sorted(per_file.items(), key=lambda kv: -len(kv[1])):
        kept, pairs = order_kept(rows)
        tus = sorted({image.tu_of[va] for va, _ in rows})
        lines.append(f"| `{source}` | {len(rows)} | {kept} of {pairs} | {', '.join(t.split('/')[1] for t in tus)} |")
    lines += ['', f'`{HOMOLOGS.relative_to(ROOT)}` feeds these matches to the TU map, so neighbours from one X1 file stay '
              'in one TU. The file also keeps matches a span cannot use, so one X1 file can still show several TUs here.', '',
              '## VU sequences', '',
              f'{len(vu)} functions have the same whole VU instruction sequence, of {STRONG_VU} or more, as a named '
              'X1 function. Inline assembly survives any `-O` level, so this also works where the C differs.', '',
              '| VA | TU | VU instructions | X1 function |', '| --- | --- | ---: | --- |']
    for va in sorted(vu, key=lambda v: -vu[v][0]):
        lines.append(f"| `{va:#x}` | `{image.tu_of[va]}` | {vu[va][0]} | "
                     f"{', '.join(f'`{n}`' for n in vu[va][1][:3])} |")
    lines += ['', 'X1 keeps no `sceVu0*` symbol at all, so none of these names is a Sony library function.', '',
              '## The bytes do not look like Sony libraries', '',
              '| Zone | Functions | Bytes | Not 8-aligned | `$fp` frames | `$gp`-relative |',
              '| --- | ---: | ---: | ---: | ---: | ---: |']
    for _, _, name in ZONES:
        if name == 'game' and any(line.startswith('| game ') for line in lines):
            continue
        row = stats[name]
        lines.append(f"| {name} | {row['functions']:,} | {row['bytes']:,} | {row['not_8_aligned']:,} | "
                     f"{row['fp_frames']:,} | {row['gp_relative']:,} |")
    lines += ['', f'The Sony and runtime zones have no `$fp` frame and no `$gp`-relative instruction. In X1, '
              f'{x1_counts[1]} of {x1_counts[0]:,} named functions open a `$fp` frame, so Sony libraries were not '
              'built this way there either.', '',
              '## Calls and strings', '',
              f"`jal` into the zone: {', '.join(f'{k} {v:,}' for k, v in into.most_common())}. "
              f"Out of it: {', '.join(f'{k} {v:,}' for k, v in out.most_common())}.", '',
              f'A library does not call its client. {len(game_callers)} TUs call game code directly, and '
              f'{len(reaches)} of {len(image.zone_tus)} reach it through their own calls.', '',
              f'The zone has {len(strings)} exact string references:', '']
    for text, sites in sorted(strings.items(), key=lambda kv: kv[1][0]):
        lines.append(f'- `{text}`, from `{sites[0]:#x}` in `{image.tu_at(sites[0])}`')
    lines += ['', '`pxyFlush` names the `.pxy` format that game code loads from `host0:/home/xeno/work/iwase/`, and '
              '`MOM2_B04` looks like an asset name.', '',
              '## Per TU', '',
              '| TU | Functions | Bytes | X1 C matches (64 B+) | Calls game | Reaches game | Examples |',
              '| --- | ---: | ---: | ---: | --- | --- | --- |'] + tu_rows
    without = [tu for tu in image.zone_tus if tu not in reaches
               and not any(image.tu_of[v] == tu for v in list(strong) + list(vu))]
    lines += ['', '## What this does not prove', '',
              f"TUs with no evidence of their own: {', '.join(f'`{t}`' for t in without) or 'none'}. They are small "
              'leaf code between TUs that do have evidence, and they are not Sony-shaped, but nothing names them.',
              '', 'Nothing here shows why this code was built at `-O0`.', '',
              '## Correction', '',
              'An earlier version of `reports/x2-compiler-calibration.md` listed network strings (`inetctl`, '
              '`SYS_NET.ICO`), `sceGsExecStoreImage`, `libpad` and `mwPly` messages as strings of this zone. That '
              'was wrong: a register tracker added chained offsets and landed inside unrelated strings. Exact '
              'references place the network strings in `0x1a73b0`, an `-O2` function just before the zone, '
              'the `sce` and `libpad` ones in the Sony zone, and `mwPly` in CRI.', '']
    return '\n'.join(lines)


def write_homologs(image, matches):
    """What tools/tu/map_infer.py reads: large bodies with one candidate function in one X1 file."""
    rows = []
    for va, candidates in sorted(matches.items()):
        if image.size[va] < STRONG_BYTES or len({(name, source) for name, _, source, _ in candidates}) != 1:
            continue
        name, _, source, offset = candidates[0]
        rows.append(dict(va=f'{va:#010x}', size=image.size[va], function=name, file=source, x1_offset=offset))
    HOMOLOGS.write_text(json.dumps(dict(
        schema='x2-o0-homologs/1', generated_by='tools/tu/x2_o0_provenance.py',
        note=['Functions of the -O0 zone whose whole body equals X1 C compiled at -O0 under the X2 contract.',
              f'Only bodies of {STRONG_BYTES} bytes or more with one candidate function. Evidence for the TU map, not names.'],
        functions=rows), indent=1) + '\n')


def main():
    image = Image()
    strings = defaultdict(list)
    for at, value in address_uses(image, LO, HI):
        text = image.string_at(value)
        if text:
            strings[text].append(at)
    matches, failures, source_count = compare_x1_c(image)
    x1, x1_frames = x1_functions()
    text = render(image, strings, zone_statistics(image), (len(x1), x1_frames), vu_matches(image, x1),
                  call_graph(image), matches, failures, source_count)
    REPORT.write_text(text)
    (OUT / 'result.json').write_text(json.dumps(
        {f'{va:#x}': rows for va, rows in sorted(matches.items())}, indent=0) + '\n')
    write_homologs(image, matches)
    print(f'{REPORT.relative_to(ROOT)}: {len(matches)} functions match, {len(failures)} compile failures')
    return 0


if __name__ == '__main__':
    sys.exit(main())
