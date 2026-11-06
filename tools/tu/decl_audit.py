#!/usr/bin/env python3
"""Inventory global functions and data declared differently across the published sources. Read-only.

    python3 -B tools/tu/decl_audit.py [--out FILE]

Scans top-level declarations and definitions of src/<unit>/*.c and *.h. A function row groups its signatures, marks which
one is the definition and classifies the disagreement: `return` (void against a value), `arity` (argument count), `type`
(same shape, other type names). An empty `()` says nothing about arguments and only counts for the return type.
Overlay symbols are keyed by unit; a main symbol also collects its overlay declarations. A lead for a person: it parses text.
"""
from __future__ import annotations

import argparse
import collections
import glob
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
UNITS = ('main', 'ov01', 'ov02')
KEYWORDS = set('if for while switch return sizeof else do case goto typedef struct union enum'.split())
NOISE = re.compile(r'\b(extern|static|inline|const|volatile|register)\b')


def strip_code(text):
    text = re.sub(r'/\*.*?\*/|//[^\n]*', ' ', text, flags=re.S)
    text = re.sub(r'"(?:\\.|[^"\\])*"', '""', text)
    return re.sub(r'^[ \t]*#.*(?:\\\n.*)*$', ' ', text, flags=re.M)


def top_level(text):
    """Yield (statement, is_definition) at brace depth 0; a `{` closes a function definition or starts an aggregate."""
    depth, start = 0, 0
    for i, ch in enumerate(text):
        if ch == '{':
            if depth == 0:
                yield text[start:i].strip(), True
            depth += 1
        elif ch == '}':
            depth -= 1
            if depth == 0:
                start = i + 1
        elif ch == ';' and depth == 0:
            yield text[start:i].strip(), False
            start = i + 1


def normal(text):
    return re.sub(r'\s*\*\s*', '*', re.sub(r'\s+', ' ', NOISE.sub(' ', text))).strip()


def parse_param(param):
    param = normal(param)
    if param in ('', 'void', '...'):
        return param
    words = re.match(r'(.*?)([A-Za-z_]\w*)?((?:\[[^\]]*\])*)$', param)
    base, name, array = words.group(1).strip(), words.group(2), words.group(3)
    if '(*' in param:
        return 'fnptr'
    if not base:                      # a type alone
        return name or param
    return base + ('*' if array else '')


def scan():
    functions = collections.defaultdict(list)
    data = collections.defaultdict(list)
    for unit in UNITS:
        for path in sorted([*glob.glob(str(ROOT / f'src/{unit}/*.[ch]')), *glob.glob(str(ROOT / f'include/{unit}/*.h'))]):
            where = f'{unit}/{Path(path).name}' if '/src/' in path else f'include/{unit}/{Path(path).name}'
            for statement, definition in top_level(strip_code(Path(path).read_text(errors='replace'))):
                statement = re.sub(r'\bINCLUDE_ASM\s*\([^)]*\)', '', statement).strip()
                if not statement or statement.startswith(('typedef', 'struct', 'union', 'enum')) and '(' not in statement:
                    continue
                match = re.match(r'(.*?[\s*])([A-Za-z_]\w*)\s*\(((?:[^()]|\([^()]*\))*)\)\s*$', statement, flags=re.S)
                if match and match.group(2) not in KEYWORDS and match.group(1).strip():
                    params = [parse_param(p) for p in re.split(r',(?![^()]*\))', match.group(3))]
                    functions[match.group(2)].append(dict(file=where, ret=normal(match.group(1)), params=params,
                                                          definition=definition))
                    continue
                match = re.match(r'(?:extern\s+)(.*?[\s*])([A-Za-z_]\w*)((?:\[[^\]]*\])*)\s*$', statement, flags=re.S)
                if match and not definition:
                    data[match.group(2)].append(dict(file=where, type=normal(match.group(1)) + ('[]' if match.group(3) else '')))
    return functions, data


def classify(rows):
    kinds = set()
    returns = {'void' if row['ret'] == 'void' else 'value' for row in rows}
    if len(returns) > 1:
        kinds.add('return')
    known = [row['params'] for row in rows if row['params'] != ['']]    # `()` is not a prototype
    counts = {0 if params == ['void'] else len(params) for params in known}
    if len(counts) > 1:
        kinds.add('arity')
    if len({row['ret'] for row in rows}) > 1 or len({tuple(p) for p in known}) > 1:
        kinds.add('type')
    return sorted(kinds)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--out')
    args = parser.parse_args()
    functions, data = scan()
    rows = []
    for name, seen in sorted(functions.items()):
        kinds = classify(seen)
        files = {row['file'] for row in seen}
        if kinds and len(files) > 1:
            rows.append(dict(name=name, kinds=kinds, defined_in=[r['file'] for r in seen if r['definition']], rows=seen))
    data_rows = [dict(name=name, types=sorted({r['type'] for r in seen}), rows=seen)
                 for name, seen in sorted(data.items()) if len({r['type'] for r in seen}) > 1]
    counter = collections.Counter(kind for row in rows for kind in row['kinds'])
    summary = dict(functions_declared=len(functions), functions_in_conflict=len(rows), by_kind=dict(counter),
                   with_c_definition=sum(bool(r['defined_in']) for r in rows), data_in_conflict=len(data_rows),
                   data_declared=len(data))
    print(json.dumps(summary))
    if args.out:
        Path(args.out).write_text(json.dumps(dict(summary=summary, functions=rows, data=data_rows), indent=1) + '\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
