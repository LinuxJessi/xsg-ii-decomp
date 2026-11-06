#!/usr/bin/env python3
"""Check a private copy of a published TU: renamed locals, fields and types must not change what it compiles to.

    python3 -B tools/tu/name_check.py --unit main --file tu083.c [--stage DIR]

Compiles DIR/<unit>/<file> (default .work/x2-names/stage) with the published command of that TU and compares it
with the published object: every allocated section byte for byte, every relocation, every global symbol. A filter
for naming passes; publication still needs the whole-file gate. Local `$L` labels are not compared.
"""
from __future__ import annotations

import argparse
import json
import re
import shlex
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def read_elf(path):
    data = Path(path).read_bytes()
    shoff = struct.unpack_from('<I', data, 32)[0]
    shentsize, shnum, shstrndx = struct.unpack_from('<HHH', data, 46)
    rows = [struct.unpack_from('<10I', data, shoff + i * shentsize) for i in range(shnum)]

    def text(base, offset):
        return data[base + offset:data.index(b'\0', base + offset)].decode('ascii', 'replace')

    names = [text(rows[shstrndx][4], row[0]) for row in rows]
    sections, relocations, symbols = {}, {}, []
    symtab = next((i for i, row in enumerate(rows) if row[1] == 2), None)
    table = []
    if symtab is not None:
        row = rows[symtab]
        for i in range(row[5] // 16):
            name, value, size, info, _other, shndx = struct.unpack_from('<IIIBBH', data, row[4] + i * 16)
            table.append((text(rows[row[6]][4], name), value, size, info, shndx))
        symbols = sorted((n, v, s, names[x] if x < len(names) else x) for n, v, s, info, x in table
                         if info >> 4 == 1 and n)
    for i, row in enumerate(rows):
        if row[2] & 2 and row[1] != 8:
            sections[names[i]] = data[row[4]:row[4] + row[5]]
        elif row[2] & 2:
            sections[names[i]] = row[5]
        if row[1] == 9 and rows[row[7]][2] & 2:
            rel = []
            for k in range(row[5] // 8):
                offset, info = struct.unpack_from('<II', data, row[4] + k * 8)
                sym = table[info >> 8] if info >> 8 < len(table) else ('?', 0, 0, 0, 0)
                label = sym[0] if sym[3] >> 4 == 1 else f'section:{names[sym[4]] if sym[4] < len(names) else sym[4]}'
                rel.append((offset, info & 0xff, label))
            relocations[names[row[7]]] = rel
    return sections, relocations, symbols


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--unit', required=True)
    parser.add_argument('--file', required=True)
    parser.add_argument('--stage', default=str(ROOT / '.work/x2-names/stage'))
    args = parser.parse_args()
    stem = Path(args.file).stem
    unit_dir = ROOT / 'build' / args.unit
    source = Path(args.stage).resolve() / args.unit / args.file
    if not source.is_file():
        sys.exit(f'no stage copy at {source}')
    object_path = f'build/c/{stem}.o' if args.unit == 'main' else f'build/scaffold/src/{args.unit}/{stem}.o'
    raw_path = f'build/c/{stem}.raw.o'
    target = raw_path if args.unit == 'main' and (unit_dir / raw_path).is_file() else object_path
    commands = subprocess.run(['ninja', '-C', str(unit_dir), '-t', 'commands', '-s', target], capture_output=True,
                              text=True, check=True).stdout
    command = next((line for line in commands.splitlines() if 'ee-gcc' in line and f'{stem}.c' in line), None)
    if not command:
        sys.exit(f'no compile command for {target}')
    published_source = f'../../src/{args.unit}/{args.file}' if args.unit == 'main' else f'src/{args.unit}/{args.file}'
    if published_source not in command:
        sys.exit(f'the published command does not compile {published_source}')
    with tempfile.TemporaryDirectory(dir=Path(args.stage).resolve()) as tmp:
        output = str(Path(tmp) / f'{stem}.o')
        rewritten = command.replace(published_source, shlex.quote(str(source)))
        rewritten = rewritten.replace(f'{unit_dir}/{target}', output).replace(target, output)
        rewritten = rewritten.replace(f'build/cinc/{stem}', f'{tmp}/cinc')   # never the published include stage
        sibling = f'-I{ROOT}/src/{args.unit}'
        rewritten = re.sub(r'(ee-gcc\s+-B\S+)', lambda m: f'{m.group(1)} {sibling}', rewritten, count=1)
        done = subprocess.run(rewritten, shell=True, cwd=unit_dir, capture_output=True, text=True, timeout=120)
        if done.returncode:
            errors = [line for line in (done.stdout + done.stderr).splitlines()
                      if 'warning' not in line and 'In function' not in line][:15]
            print(json.dumps(dict(ok=False, stage='compile', errors=errors), ensure_ascii=False, indent=1))
            return 1
        new_sections, new_rel, new_symbols = read_elf(output)
    old_sections, old_rel, old_symbols = read_elf(unit_dir / target)
    problems = []
    for name in sorted(set(old_sections) | set(new_sections)):
        if old_sections.get(name) != new_sections.get(name):
            problems.append(f'section {name} differs')
    for name in sorted(set(old_rel) | set(new_rel)):
        if old_rel.get(name) != new_rel.get(name):
            first = next((pair for pair in zip(old_rel.get(name, []), new_rel.get(name, [])) if pair[0] != pair[1]), None)
            problems.append(f'relocations of {name} differ, first {first}')
    if old_symbols != new_symbols:
        gone = sorted({row[0] for row in old_symbols} - {row[0] for row in new_symbols})[:8]
        added = sorted({row[0] for row in new_symbols} - {row[0] for row in old_symbols})[:8]
        problems.append(f'global symbols differ; removed {gone}, added {added}: leave global names to the proposal')
    print(json.dumps(dict(ok=not problems, problems=problems), ensure_ascii=False, indent=1))
    return 0 if not problems else 1


if __name__ == '__main__':
    sys.exit(main())
