#!/usr/bin/env python3
"""Per-TU evidence dossiers for recovering X2 function and data names. Leads only, never identity.

X2 has no symbols (decision D1), so a name is an inference. For every in-scope function a dossier lists what
can support one: size, state, callers, callees, strings, float and integer constants, hardware registers, data
tables that hold its address, the row of XENOSAGA2_FUNCTION_NAMES.md, and the most similar X1 functions with
their symbol, annotation and callees. The X1 tree is only read.

    python3 -B tools/tu/name_evidence.py --out .work/x2-names/dossiers [--jobs 16] [--blind blind.json]

Similarity is the Jaccard index of mnemonic 4-grams. Bodies under MIN_WORDS instructions get no X1 candidates,
because short fingerprints gave false names on X1 (memory: xenosaga2-name-recovery-20261104).
`--blind FILE` (a JSON list of "unit:va") removes the name and every direct hint of those functions, so a pilot
can measure how often agents recover a name that is already known.
"""
from __future__ import annotations

import argparse
import collections
import concurrent.futures
import csv
import glob
import json
import multiprocessing
import os
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
X1 = Path('/home/pc/xenosaga1-port/xenosaga-i-decomp')
MIN_WORDS = 16
TOP_X1 = 3
MIN_JACCARD = 0.30
INSTRUCTION = re.compile(r'/\*\s*[0-9A-Fa-f]+\s+([0-9A-Fa-f]{8})\s+[0-9A-Fa-f]{8}\s*\*/\s+(\S+)\s*(.*)$')
SYMBOL = re.compile(r'\b(?:D_[0-9A-Fa-f]{6,8}|[A-Za-z_][A-Za-z0-9_]*)\b')
ELF_FILES = {'main': 'build/main/build/main.elf', 'ov01': 'build/ov01/build/ov01.elf', 'ov02': 'build/ov02/build/ov02.elf'}
BRANCH_OR_REG = re.compile(r'^(?:\$|\.L)')
csv.field_size_limit(10 ** 7)


def parse_asm(path):
    """(mnemonics, instructions as (mnemonic, operands)) of one split function."""
    rows = []
    try:
        text = Path(path).read_text(encoding='utf-8', errors='replace')
    except OSError:
        return rows
    for line in text.splitlines():
        match = INSTRUCTION.search(line)
        if match:
            rows.append((match.group(2), match.group(3).strip()))
    return rows


def shingles(mnemonics):
    return {tuple(mnemonics[i:i + 4]) for i in range(max(0, len(mnemonics) - 3))}


class Image:
    """Allocated bytes of a rebuilt ELF, read by VA."""

    def __init__(self, path):
        self.data = Path(path).read_bytes()
        shoff = struct.unpack_from('<I', self.data, 32)[0]
        shentsize, shnum, shstrndx = struct.unpack_from('<HHH', self.data, 46)
        rows = [struct.unpack_from('<10I', self.data, shoff + i * shentsize) for i in range(shnum)]
        strings = rows[shstrndx][4] if shnum else 0
        self.sections = []
        for row in rows:
            name_end = self.data.index(b'\0', strings + row[0])
            name = self.data[strings + row[0]:name_end].decode('ascii', 'replace')
            if row[3] and row[5] and row[1] != 8 and row[2] & 2:
                self.sections.append((row[3], row[3] + row[5], row[4], name))

    def section_of(self, va):
        for start, end, offset, name in self.sections:
            if start <= va < end:
                return start, end, offset, name
        return None

    def read(self, va, size):
        found = self.section_of(va)
        if not found:
            return None
        start, end, offset, _name = found
        return self.data[offset + va - start:offset + min(va + size, end) - start]


def c_string(image, va):
    raw = image.read(va, 256)
    if not raw or b'\0' not in raw:
        return None
    raw = raw[:raw.index(b'\0')]
    if len(raw) < 3:
        return None
    try:
        text = raw.decode('cp932')
    except UnicodeDecodeError:
        return None
    if any(ord(c) < 32 and c not in '\t\n\r' for c in text):
        return None
    letters = sum(c.isalnum() or ord(c) > 127 for c in text)
    return text if letters * 2 >= len(text) else None


def load_symbols(unit):
    table = {}
    path = ROOT / f'config/symbols/{unit}.txt'
    if path.is_file():
        for line in path.read_text(errors='replace').splitlines():
            match = re.match(r'\s*([A-Za-z_][\w.]*)\s*=\s*(0x[0-9A-Fa-f]+)', line)
            if match:
                table[match.group(1)] = int(match.group(2), 16)
    return table


def label_va(label, symbols):
    if label in symbols:
        return symbols[label]
    match = re.fullmatch(r'D_([0-9A-Fa-f]{6,8})', label)
    return int(match.group(1), 16) if match else None


def asm_index(unit):
    base = ROOT / ('build/main/asm/main/nonmatchings' if unit == 'main' else f'build/{unit}/asm/nonmatchings/{unit}')
    return {Path(p).stem: p for p in glob.glob(str(base / '*/*.s'))}


def x1_asm_index():
    index = {}
    for unit in ('main', 'ov01', 'ov02', 'ov10', 'ov11', 'ov12'):
        base = X1 / ('build/main/asm/main/nonmatchings' if unit == 'main' else f'build/{unit}/asm/nonmatchings/{unit}')
        for path in glob.glob(str(base / '*/*.s')):
            index[(unit, Path(path).stem)] = path
    return index


def load_x1():
    """One record per X1 function: unit, name, annotation, mnemonics, callees, source path."""
    rows = {}
    with open(X1 / 'config/index/functions.tsv', encoding='utf-8', errors='replace', newline='') as handle:
        for row in csv.DictReader(handle, delimiter='\t'):
            rows[(row['unit'], row['name'])] = row
    records = []
    for key, path in x1_asm_index().items():
        instructions = parse_asm(path)
        if len(instructions) < MIN_WORDS:
            continue
        row = rows.get(key, {})
        callees = [ops.split()[-1] for mn, ops in instructions if mn == 'jal']
        records.append(dict(unit=key[0], name=key[1], annotation=(row.get('annotation') or '')[:200],
                            path=row.get('tu_path', ''), words=len(instructions),
                            callees=list(dict.fromkeys(callees))[:10],
                            shingles=shingles([mn for mn, _ops in instructions])))
    return records


X1_RECORDS = None
X1_POSTINGS = None


def similar_chunk(chunk):
    out = {}
    for key, grams in chunk:
        votes = collections.Counter()
        for gram in grams:
            for index in X1_POSTINGS.get(gram, ()):
                votes[index] += 1
        scored = []
        for index, _votes in votes.most_common(40):
            record = X1_RECORDS[index]
            common = len(grams & record['shingles'])    # the votes skip common 4-grams, the score must not
            score = common / (len(grams) + len(record['shingles']) - common)
            if score >= MIN_JACCARD:
                scored.append((round(score, 2), index))
        out[key] = sorted(scored, reverse=True)[:TOP_X1]
    return out


def name_map_rows():
    """Lines of XENOSAGA2_FUNCTION_NAMES.md that mention a working name, keyed by (unit or None, va)."""
    rows = collections.defaultdict(list)
    path = ROOT / 'XENOSAGA2_FUNCTION_NAMES.md'
    if not path.is_file():
        return rows
    for line in path.read_text(encoding='utf-8', errors='replace').splitlines():
        if not line.startswith('|'):
            continue
        for match in re.finditer(r'\bfunc_(?:(ov0[12])_)?([0-9A-Fa-f]{8})\b', line):
            rows[(match.group(1) or 'main', int(match.group(2), 16))].append(line.strip()[:260])
    return rows


def constant_of(mnemonic, operands):
    parts = [p.strip() for p in operands.split(',')]
    if mnemonic in ('addiu', 'ori', 'daddiu') and len(parts) == 3 and parts[1] in ('$0', '$zero'):
        return parts[2]
    if mnemonic == 'lui' and len(parts) == 2 and parts[1].startswith('0x'):
        return f'{parts[1]}0000'
    if mnemonic in ('andi', 'slti', 'sltiu', 'ori') and len(parts) == 3 and parts[2].startswith('0x'):
        return f'{mnemonic}:{parts[2]}'
    return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', required=True)
    parser.add_argument('--jobs', type=int, default=16)
    parser.add_argument('--blind')
    args = parser.parse_args()
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    blind = set(json.loads(Path(args.blind).read_text())) if args.blind else set()

    tus, functions = [], {}
    for unit in ELF_FILES:
        for path in sorted((ROOT / 'config/tu' / unit).glob('*.json')):
            record = json.loads(path.read_text())
            if record.get('id') and 'functions' in record:
                tus.append(record)
                for function in record['functions']:
                    functions[(unit, function['name'])] = (record, function)

    images = {unit: Image(ROOT / path) for unit, path in ELF_FILES.items()}
    symbols = {unit: load_symbols(unit) for unit in ELF_FILES}
    asm_paths = {unit: asm_index(unit) for unit in ELF_FILES}
    parsed = {}
    for (unit, name), _value in functions.items():
        path = asm_paths[unit].get(name)
        parsed[(unit, name)] = parse_asm(path) if path else []

    callers = collections.defaultdict(list)
    by_name = collections.defaultdict(list)
    for key in parsed:
        by_name[key[1]].append(key)
    address = {}
    for (unit, name), (_record, function) in functions.items():
        address[(unit, int(function['va'], 16))] = name
    for key, instructions in parsed.items():
        for mnemonic, operands in instructions:
            if mnemonic == 'jal':
                target = operands.split()[-1]
                for target_key in by_name.get(target, ()):
                    if target_key[0] == key[0] or target_key[0] == 'main':
                        callers[target_key].append(key)

    # Data words that hold a function's address: dispatch tables, callbacks.
    table_refs = collections.defaultdict(list)
    for unit, image in images.items():
        for start, end, offset, name in image.sections:
            if name not in ('.data', '.rodata', '.sdata'):
                continue
            for va in range(start & ~3, end - 3, 4):
                if va < start:
                    continue
                word = struct.unpack_from('<I', image.data, offset + va - start)[0]
                target = address.get((unit, word)) or address.get(('main', word))
                if target:
                    owner = unit if (unit, word) in address else 'main'
                    table_refs[(owner, target)].append((unit, va))

    print('loading X1', file=sys.stderr)
    global X1_RECORDS, X1_POSTINGS
    X1_RECORDS = load_x1()
    postings = collections.defaultdict(list)
    for index, record in enumerate(X1_RECORDS):
        for gram in record['shingles']:
            postings[gram].append(index)
    # A very common 4-gram says nothing and slows the votes down.
    X1_POSTINGS = {gram: items for gram, items in postings.items() if len(items) <= 300}
    queries = [(key, shingles([mn for mn, _ops in rows])) for key, rows in parsed.items()
               if len(rows) >= MIN_WORDS and functions[key][0].get('in_scope', True)]
    chunks = [queries[i:i + 100] for i in range(0, len(queries), 100)]
    similar = {}
    print(f'{len(X1_RECORDS)} X1 functions, {len(queries)} queries', file=sys.stderr)
    with concurrent.futures.ProcessPoolExecutor(args.jobs, mp_context=multiprocessing.get_context('fork')) as pool:
        for result in pool.map(similar_chunk, chunks):
            similar.update(result)

    # The strongest X1 name of a function, to label neighbours.
    best_x1 = {key: X1_RECORDS[items[0][1]]['name'] for key, items in similar.items()
               if items and items[0][0] >= 0.8}
    hints = name_map_rows()

    def describe(key):
        name = key[1]
        shown = name if f'{key[0]}:{functions[key][1]["va"]}' not in blind else 'func_' + functions[key][1]['va'][2:].upper()
        if key in best_x1 and f'{key[0]}:{functions[key][1]["va"]}' not in blind and shown.startswith('func_'):
            return f'{shown} (X1~{best_x1[key]})'
        return shown

    written = 0
    index_rows = []
    for record in tus:
        if not record.get('in_scope', True):
            continue
        unit = record['id'].split('/')[0]
        hidden = all(f'{unit}:{f["va"]}' in blind for f in record['functions'])
        lines = [f'# {record["id"]}', '', f'source: `{"hidden" if hidden else record.get("path")}`; {len(record["functions"])} functions. '
                 'Everything below is a lead. A name is an inference.', '']
        for function in record['functions']:
            key = (unit, function['name'])
            va = int(function['va'], 16)
            is_blind = f'{unit}:{function["va"]}' in blind
            shown = 'func_' + function['va'][2:].upper() if is_blind else function['name']
            instructions = parsed.get(key, [])
            lines += [f'## {shown}', f'- unit {unit}, va {function["va"]}, {function.get("size")} bytes, '
                      f'{len(instructions)} instructions, state {function.get("state")}']
            callee_keys = []
            for mnemonic, operands in instructions:
                if mnemonic == 'jal':
                    target = operands.split()[-1]
                    candidates = [k for k in by_name.get(target, ()) if k[0] in (unit, 'main')]
                    callee_keys.append(describe(candidates[0]) if candidates else target)
            if callee_keys:
                lines.append('- calls, in order: ' + ', '.join(list(dict.fromkeys(callee_keys))[:24]))
            caller_keys = list(dict.fromkeys(callers.get(key, [])))
            if caller_keys:
                lines.append(f'- called by ({len(caller_keys)}): ' + ', '.join(describe(k) for k in caller_keys[:16]))
            tables = table_refs.get(key, [])
            if tables:
                lines.append('- address stored in data: ' + ', '.join(f'{u}:0x{v:08x}' for u, v in tables[:6]))
            strings, floats, data, constants = [], [], [], collections.Counter()
            for mnemonic, operands in instructions:
                value = constant_of(mnemonic, operands)
                if value:
                    constants[value] += 1
                if 'D_' not in operands and '%' not in operands:
                    continue
                for label in re.findall(r'%(?:hi|lo|gp_rel)\(([^)+]+)', operands) or re.findall(r'\bD_[0-9A-Fa-f]{6,8}\b', operands):
                    label = label.strip()
                    target = label_va(label, symbols[unit]) or label_va(label, symbols['main'])
                    if target is None:
                        continue
                    image = images[unit] if images[unit].section_of(target) else images['main']
                    found = image.section_of(target)
                    section = found[3] if found else '?'
                    text = c_string(image, target) if section in ('.rodata', '.data', '.sdata') else None
                    if text:
                        if text not in strings:
                            strings.append(text)
                    elif section == '.lit4':
                        raw = image.read(target, 4)
                        if raw and len(raw) == 4:
                            shown_float = f'{struct.unpack("<f", raw)[0]:g}'
                            if shown_float not in floats:
                                floats.append(shown_float)
                    elif label not in data:
                        data.append(label)
            if strings:
                lines.append('- strings: ' + ' | '.join(json.dumps(s, ensure_ascii=False)[:120] for s in strings[:10]))
            if floats:
                lines.append('- floats: ' + ', '.join(floats[:14]))
            hardware = [c for c in constants if c.startswith(('0x1000', '0x1200', '0x1100')) ]
            interesting = [c for c, _n in constants.most_common(14) if c not in ('0x1', '0x0', '-0x1', '0x2')]
            if interesting:
                lines.append('- integer constants: ' + ', '.join(interesting))
            if hardware:
                lines.append('- hardware addresses: ' + ', '.join(hardware))
            if data:
                lines.append('- data refs: ' + ', '.join(data[:16]))
            if not is_blind:
                for row in hints.get((unit, va), [])[:3]:
                    lines.append(f'- names map row: {row}')
            for score, index in similar.get(key, []):
                x1 = X1_RECORDS[index]
                if is_blind and x1['name'] == function['name']:
                    continue
                note = f'; {x1["annotation"]}' if x1['annotation'] else ''
                calls = f'; calls {", ".join(x1["callees"][:6])}' if x1['callees'] else ''
                lines.append(f'- X1 similar {score}: {x1["unit"]} {x1["name"]} ({x1["words"]} instr, {x1["path"]}){note}{calls}')
            lines.append('')
        name = record['id'].replace('/', '-') + '.md'
        (out / name).write_text('\n'.join(lines) + '\n', encoding='utf-8')
        written += 1
        unnamed = sum(f['name'].startswith('func_') for f in record['functions'])
        index_rows.append(dict(id=record['id'], path=record.get('path'), functions=len(record['functions']),
                               unnamed=unnamed, bytes=sum(f.get('size') or 0 for f in record['functions']),
                               c=sum(f.get('state') == 'c' for f in record['functions'])))
    (out / 'index.json').write_text(json.dumps(index_rows, indent=1) + '\n')
    print(json.dumps(dict(dossiers=written, x1_functions=len(X1_RECORDS),
                          functions_with_x1_candidate=sum(1 for v in similar.values() if v),
                          strong_x1=len(best_x1))))
    return 0


if __name__ == '__main__':
    sys.exit(main())
