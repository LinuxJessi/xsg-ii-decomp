#!/usr/bin/env python3
"""Does the compile contract calibrated on Xenosaga I reproduce Xenosaga II's code?

Phase 3 of docs/x2-bootstrap.md. The annotations mark H1 functions, whose X2 code equals an
X1 function with published C. This compiles that C under candidate contracts and compares
each function with the X2 bytes:

    x2_calibrate.py run [--files N] [--contracts a,b] [--jobs 4]
    x2_calibrate.py report

The comparison masks relocated fields, because X1 and X2 place symbols at different
addresses. So this is calibration evidence and never exactness: only the whole-file gate
decides that. The X1 tree is only read, and all output stays under .work/x2-calibrate/.
"""
import argparse
import collections
import concurrent.futures
import json
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
X1_ROOT = Path('/home/pc/xenosaga1-port/xenosaga-i-decomp')
OUT = ROOT / '.work/x2-calibrate'
CC_TU = ROOT / 'tools/tu/cc_tu.sh'
ANNOTATIONS = (('main', 'SLUS_208.92', 'annotations/slus_208.92_annotations.csv'),
               ('ov01', 'OV01.OVL', 'annotations/overlays/ov01_annotations.csv'),
               ('ov02', 'OV02.OVL', 'annotations/overlays/ov02_annotations.csv'))
AS_SWAP = '.work/toolchains/ee-legacy-binutils-la29-vsqrt-nolabel-laimm-swap-v1/bin/ee-as'
AS_NO_SWAP = '.work/toolchains/ee-legacy-binutils-la29-vsqrt-nolabel-laimm-v1/bin/ee-as'
CC_FSV12 = '.work/toolchains/ee-gcc2.96-realconv-lp7-fsv12-v1/bin'
# name -> (compiler dir, assembler, assembler -g, -G rule). None keeps the X1 TU's own value.
CONTRACTS = {
    'x1': (CC_FSV12, AS_SWAP, None, None),
    'no-g': (CC_FSV12, AS_SWAP, False, None),
    'no-swap': (CC_FSV12, AS_NO_SWAP, False, None),
    'g-flipped': (CC_FSV12, AS_SWAP, None, 'flip'),
    'no-fsv': ('.work/toolchains/ee-gcc2.96-realconv-lp7-v1/bin', AS_SWAP, None, None),
    'no-lp7': ('.work/toolchains/ee-gcc2.96-realconv-v1/bin', AS_SWAP, None, None),
    'no-realconv': ('.work/toolchains/ee-gcc2.96-lp7-v1/bin', AS_SWAP, None, None),
}
# The patch ladder: each earlier compiler, with the rest of the contract unchanged.
for ladder in sorted((ROOT / '.work/toolchains').glob('ee-gcc2.96-realconv-lp7-fsv*')):
    if (ladder / 'bin/ee-gcc').exists() and 'fsv12' not in ladder.name:
        CONTRACTS.setdefault(ladder.name.removeprefix('ee-gcc2.96-realconv-lp7-'),
                             (str(ladder.relative_to(ROOT) / 'bin'), AS_SWAP, None, None))
R_MIPS_MASK = {2: 0xffffffff, 4: 0x03ffffff, 5: 0xffff, 6: 0xffff, 7: 0xffff, 8: 0xffff}


class ElfObject:
    """Function bytes of one relocatable object, with the words that relocations touch."""

    def __init__(self, path):
        data = Path(path).read_bytes()
        shoff = struct.unpack_from('<I', data, 0x20)[0]
        size, count, names = struct.unpack_from('<HHH', data, 0x2e)
        rows = [struct.unpack_from('<10I', data, shoff + i * size) for i in range(count)]
        strings = rows[names][4]
        self.sections = [(data[strings + r[0]:data.index(b'\0', strings + r[0])].decode(), r) for r in rows]
        by_name = {name: row for name, row in self.sections}
        text = by_name['.text']
        self.text = data[text[4]:text[4] + text[5]]
        text_index = next(i for i, (name, _) in enumerate(self.sections) if name == '.text')
        self.masks = {}
        if '.rel.text' in by_name:
            rel = by_name['.rel.text']
            for offset in range(rel[4], rel[4] + rel[5], 8):
                where, info = struct.unpack_from('<II', data, offset)
                self.masks[where] = self.masks.get(where, 0) | R_MIPS_MASK.get(info & 0xff, 0xffffffff)
        self.functions = {}
        symtab = by_name['.symtab']
        names_row = rows[symtab[6]]
        for offset in range(symtab[4], symtab[4] + symtab[5], 16):
            name_at, value, length, info, _, section = struct.unpack_from('<IIIBBH', data, offset)
            if info & 15 == 2 and section == text_index:
                start = names_row[4] + name_at
                self.functions[data[start:data.index(b'\0', start)].decode()] = (value, length)


class Target:
    """Original X2 bytes by address, for one unit."""

    def __init__(self, path, unit):
        self.data = Path(path).read_bytes()
        shoff = struct.unpack_from('<I', self.data, 0x20)[0]
        size, count, names = struct.unpack_from('<HHH', self.data, 0x2e)
        rows = [struct.unpack_from('<10I', self.data, shoff + i * size) for i in range(count)]
        strings = rows[names][4]
        by_name = {self.data[strings + r[0]:self.data.index(b'\0', strings + r[0])].decode('latin1'): r for r in rows}
        section = by_name['.text' if unit == 'main' else unit]
        self.base = section[4] - section[3]

    def word(self, va):
        return struct.unpack_from('<I', self.data, va + self.base)[0]


def load_h1():
    """(unit, va, X1 name, X1 source path) of each H1 annotation that names its X1 source."""
    rows = []
    for unit, _, path in ANNOTATIONS:
        for line in (ROOT / path).read_text().splitlines():
            match = re.match(r'0x([0-9a-fA-F]+),\s*\S+ - \[H1\].*?X1:\s*(\S+) in (\S+\.c)', line)
            if match:
                rows.append((unit, int(match.group(1), 16), match.group(2), match.group(3)))
    return rows


def x1_tus():
    build = json.loads((X1_ROOT / 'config/tu-build.json').read_text())
    return {tu['path']: tu for tu in build['tus'] if tu.get('path')}


def x1_asm_bodies(path):
    """Functions the X1 source does not hold in C: the object would only echo X1's own bytes."""
    text = (X1_ROOT / path).read_text(errors='replace')
    return set(re.findall(r'INCLUDE_ASM\s*\(\s*(?:"[^"]*"\s*,\s*)?"?([A-Za-z_]\w*)"?\s*\)', text))


def compile_tu(contract_name, tu):
    """One X1 TU under one contract, through the project's own cc_tu.sh. Returns (object, error)."""
    compiler, assembler, keep_g, g_rule = CONTRACTS[contract_name]
    flags = tu['contract']['flags']
    g_flag = next(flag for flag in flags if flag.startswith('-G'))
    if g_rule == 'flip':
        g_flag = '-G0' if g_flag == '-G8' else '-G8'
    has_g = '-g' in tu['contract']['assembler_flags']
    as_options = '-g' if (has_g if keep_g is None else keep_g) else ''
    work = OUT / contract_name / tu['unit']
    work.mkdir(parents=True, exist_ok=True)
    out = work / f'{Path(tu["path"]).stem}.o'
    unit_dir = X1_ROOT / 'build' / tu['unit']
    env = dict(os.environ, XENO_EE_AS_OPTIONS=as_options)
    run = subprocess.run(['bash', str(CC_TU), str(X1_ROOT / tu['path']), str(out), str(ROOT / compiler),
                          str(ROOT / assembler), g_flag, str(unit_dir), str(X1_ROOT)],
                         cwd=unit_dir, env=env, capture_output=True, text=True, timeout=120)
    if run.returncode or not out.is_file():
        return None, (run.stdout + run.stderr).strip()[-400:]
    return out, ''


def compare(obj, name, target, va):
    """'same', or the first differing offset, comparing only bits no relocation touches."""
    value, length = obj.functions[name]
    length -= length % 4
    differing = []
    for offset in range(0, length, 4):
        mask = obj.masks.get(value + offset, 0)
        ours = struct.unpack_from('<I', obj.text, value + offset)[0]
        if (ours & ~mask) != (target.word(va + offset) & ~mask):
            differing.append(offset)
    return dict(status='same' if not differing else 'differs', size=length, differing_words=len(differing),
                first_difference=differing[0] if differing else None)


def run_contract(contract_name, sources, tus, targets):
    results = {}
    errors = {}
    for path, functions in sources.items():
        tu = tus.get(path)
        unusable = 'no_x1_tu' if tu is None else None
        if tu is not None and not tu['contract'].get('assembler_flags'):
            unusable = f'x1_contract_{tu["contract"]["id"]}'
        if unusable:
            for row in functions:
                results[f'{row[0]}:{row[1]:#x}'] = dict(name=row[2], source=path, status=unusable)
            continue
        out, error = compile_tu(contract_name, tu)
        if out is None:
            errors[path] = error
            for row in functions:
                results[f'{row[0]}:{row[1]:#x}'] = dict(name=row[2], source=path, status='compile_failed')
            continue
        obj = ElfObject(out)
        asm_bodies = x1_asm_bodies(path)
        for unit, va, name, _ in functions:
            key = f'{unit}:{va:#x}'
            if name in asm_bodies:
                results[key] = dict(name=name, source=path, status='x1_asm')
            elif name not in obj.functions:
                results[key] = dict(name=name, source=path, status='not_in_object')
            else:
                results[key] = dict(name=name, source=path, **compare(obj, name, targets[unit], va))
    return contract_name, results, errors


def command_run(args):
    rows = load_h1()
    sources = collections.defaultdict(list)
    for row in rows:
        sources[row[3]].append(row)
    chosen = sorted(sources, key=lambda path: -len(sources[path]))
    if args.files:
        chosen = chosen[:args.files]
    subset = {path: sources[path] for path in chosen}
    names = args.contracts.split(',') if args.contracts else list(CONTRACTS)
    missing = [name for name in names if not (ROOT / CONTRACTS[name][0] / 'ee-gcc').exists()]
    if missing:
        raise SystemExit(f'compiler missing for contracts: {missing}')
    tus = x1_tus()
    targets = {unit: Target(ROOT / 'game/extracted-iso' / file_name, unit) for unit, file_name, _ in ANNOTATIONS}
    summary = dict(functions=sum(len(v) for v in subset.values()), files=len(subset), contracts={}, errors={})
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for name, results, errors in pool.map(lambda c: run_contract(c, subset, tus, targets), names):
            summary['contracts'][name] = results
            if errors:
                summary['errors'][name] = errors
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / 'result.json').write_text(json.dumps(summary, indent=1) + '\n')
    print_summary(summary)
    return 0


def print_summary(summary):
    print(f'{summary["functions"]} H1 functions in {summary["files"]} X1 files')
    base = summary['contracts'].get('x1', {})
    for name, results in summary['contracts'].items():
        counts = collections.Counter(row['status'] for row in results.values())
        gained = [key for key, row in results.items() if row['status'] == 'same' and base.get(key, {}).get('status') == 'differs']
        lost = [key for key, row in results.items() if row['status'] == 'differs' and base.get(key, {}).get('status') == 'same']
        print(f'{name:12} {dict(counts)}  fixes against x1: {len(gained)}  breaks against x1: {len(lost)}')
    for name, errors in summary['errors'].items():
        for path, error in list(errors.items())[:3]:
            print(f'  error {name} {path}: {error[:260]}')


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command', required=True)
    run = sub.add_parser('run', help='compile the H1 functions under candidate contracts')
    run.add_argument('--files', type=int, help='only the N X1 files with the most H1 functions')
    run.add_argument('--contracts', help='comma-separated subset of: ' + ', '.join(CONTRACTS))
    run.add_argument('--jobs', type=int, default=4)
    sub.add_parser('report', help='print the last result')
    args = parser.parse_args(argv)
    if args.command == 'run':
        return command_run(args)
    print_summary(json.loads((OUT / 'result.json').read_text()))
    return 0


if __name__ == '__main__':
    sys.exit(main())
