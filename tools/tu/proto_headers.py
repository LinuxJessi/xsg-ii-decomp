#!/usr/bin/env python3
"""Move cross-TU prototypes of matched C functions into include/<unit>/<tu>.h. Only the bytes decide.

    python3 -B tools/tu/proto_headers.py --out DIR --jobs 20 [--limit N]   # trials, writes only DIR
    python3 -B tools/tu/proto_headers.py --out DIR --jobs 20 --write       # under baseline:integration

A function qualifies when one C definition exists, a file other than its own declares it at top level, and its signature
needs only base types and struct tags. Each (file, function) pair is tried on its own: the local prototype becomes the
definition's prototype and tools/tu/name_check.py compares the object with the published one. A function failing in any
file stays local everywhere. The final form of each file (the include instead of the local prototypes) is checked
whole; a file that fails keeps its published text and does not include the header. Afterwards run the ordinary chain and
the gate, which alone proves the result.
"""
from __future__ import annotations

import argparse
import collections
import concurrent.futures
import glob
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
UNITS = ('main', 'ov01', 'ov02')
SPELLING = {'u8': 'unsigned char', 's8': 'signed char', 'u16': 'unsigned short', 's16': 'short', 'u32': 'unsigned int',
            's32': 'int', 'u64': 'unsigned long long', 's64': 'long long', 'f32': 'float'}
BASE_WORDS = set('void int char short long float double unsigned signed const volatile struct'.split()) | set(SPELLING)
KEYWORDS = set('if for while switch return sizeof else do case goto typedef struct union enum'.split())


def blank(text):
    """Comments, strings and preprocessor lines become spaces, so offsets stay valid."""
    def spaces(match):
        return re.sub(r'[^\n]', ' ', match.group())
    text = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\\n])*"', spaces, text, flags=re.S)
    return re.sub(r'^[ \t]*#(?:.*\\\n)*.*$', spaces, text, flags=re.M)


def statements(text):
    """Yield (start, end, statement, is_definition) at brace depth 0, offsets into the raw text."""
    clean = blank(text)
    depth, start = 0, 0
    for offset, char in enumerate(clean):
        if char == '{':
            if depth == 0:
                yield start, offset, clean[start:offset], True
            depth += 1
        elif char == '}':
            depth -= 1
            if depth == 0:
                start = offset + 1
        elif char == ';' and depth == 0:
            yield start, offset + 1, clean[start:offset], False
            start = offset + 1


def skip_lead(text, start):
    """Past whitespace and INCLUDE_ASM(...) lines, which carry no semicolon."""
    clean = blank(text)
    while True:
        match = re.compile(r'\s*(?:INCLUDE_ASM\s*\([^)]*\)\s*)?').match(clean, start)
        if match.end() == start:
            return start
        start = match.end()


FUNCTION = re.compile(r'^\s*(?:INCLUDE_ASM\s*\([^)]*\)\s*)*(.*?[\s*])([A-Za-z_]\w*)\s*\(((?:[^()]|\([^()]*\))*)\)\s*$', re.S)


def parse(statement):
    match = FUNCTION.match(statement)
    if not match or match.group(2) in KEYWORDS or not match.group(1).strip():
        return None
    head = ' '.join(match.group(1).split())
    if head.startswith(('typedef', 'struct') ) and '(' in head:
        return None
    return head, match.group(2), ' '.join(match.group(3).split())


def spell(text):
    return re.sub(r'\b(u8|s8|u16|s16|u32|s32|u64|s64|f32)\b', lambda m: SPELLING[m.group(1)], text)


def header_signature(head, params):
    """The prototype text and struct tags, or None when the signature needs a type other than base types and tags."""
    if re.search(r'\b(static|inline)\b', head) or '(' in params:
        return None
    head = re.sub(r'\bextern\b\s*', '', head)
    types = [head]
    for param in re.split(r',', params):
        param = param.strip()
        if param in ('', 'void', '...'):
            continue
        if '[' in param:
            return None
        types.append(re.sub(r'[A-Za-z_]\w*\s*$', '', param) if re.search(r'[\s*][A-Za-z_]\w*$', param) else param)
    tags = set()
    for item in types:
        for tag in re.findall(r'\bstruct\s+(\w+)', item):
            if '*' not in item:
                return None                                 # a struct by value needs its layout
            tags.add(tag)
        words = set(re.findall(r'[A-Za-z_]\w*', re.sub(r'\bstruct\s+\w+', '', item)))
        if words - BASE_WORDS:
            return None
    if params == '':
        return None
    return spell(head), spell(params), tags


def scan():
    definitions = collections.defaultdict(list)
    declarations = collections.defaultdict(list)
    for unit in UNITS:
        for path in sorted(glob.glob(str(ROOT / f'src/{unit}/*.c'))):
            where = f'{unit}/{Path(path).name}'
            text = Path(path).read_text(encoding='utf-8', errors='surrogateescape')
            for start, end, statement, definition in statements(text):
                parsed = parse(statement)
                if not parsed:
                    continue
                head, name, params = parsed
                (definitions if definition else declarations)[name].append(dict(file=where, head=head, params=params))
    return definitions, declarations


def remove_declarations(text, names, replacement=None):
    """Drop the top-level prototypes of `names`; with `replacement` (name -> text) put the prototype in its place."""
    pieces, last = [], 0
    for start, end, statement, definition in statements(text):
        if definition:
            continue
        parsed = parse(statement)
        if not parsed or parsed[1] not in names:
            continue
        begin = skip_lead(text, start)
        pieces.append(text[last:begin])
        last = end
        if replacement is not None:
            pieces.append(replacement[parsed[1]])
        elif text[end:end + 1] == '\n' and (begin == 0 or text[begin - 1] == '\n'):
            last = end + 1
    pieces.append(text[last:])
    return ''.join(pieces)


def add_includes(text, headers):
    """After the leading block of includes."""
    lines = text.split('\n')
    place = None
    for index, line in enumerate(lines):
        if line.startswith('#include'):
            place = index
        elif place is not None and line.strip() and not line.startswith('#'):
            break
    if place is None:
        return None
    return '\n'.join(lines[:place + 1] + [f'#include "{header}"' for header in sorted(headers)] + lines[place + 1:])


def check(args):
    stage, where = args
    unit, name = where.split('/')
    done = subprocess.run([sys.executable, '-B', str(ROOT / 'tools/tu/name_check.py'), '--unit', unit, '--file', name,
                           '--stage', str(stage)], capture_output=True, text=True, cwd=ROOT, timeout=300)
    return done.returncode == 0, (done.stdout + done.stderr)[-600:]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', required=True)
    parser.add_argument('--jobs', type=int, default=20)
    parser.add_argument('--limit', type=int)
    parser.add_argument('--write', action='store_true')
    args = parser.parse_args()
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    trial_path = out / 'trials.json'

    definitions, declarations = scan()
    candidates = {}
    skipped = collections.Counter()
    for name, rows in definitions.items():
        if len(rows) != 1:
            skipped['several definitions'] += 1
            continue
        owner = rows[0]['file']
        consumers = sorted({row['file'] for row in declarations.get(name, [])} - {owner})
        if not consumers:
            continue
        signature = header_signature(rows[0]['head'], rows[0]['params'])
        if not signature:
            skipped['signature needs a type'] += 1
            continue
        candidates[name] = dict(owner=owner, consumers=consumers, head=signature[0], params=signature[1], tags=sorted(signature[2]))
    print(json.dumps(dict(candidates=len(candidates), pairs=sum(len(c['consumers']) for c in candidates.values()),
                          skipped=skipped)))

    results = json.loads(trial_path.read_text()) if trial_path.is_file() else {}
    names = sorted(candidates)[:args.limit] if args.limit else sorted(candidates)
    pending = []
    for name in names:
        row = candidates[name]
        for consumer in row['consumers']:
            key = f'{consumer}:{name}'
            if key in results:
                continue
            stage = out / 'trial' / re.sub(r'\W', '_', key)
            (stage / consumer.split('/')[0]).mkdir(parents=True, exist_ok=True)
            text = (ROOT / 'src' / consumer).read_text(encoding='utf-8', errors='surrogateescape')
            tags = ''.join(f'struct {tag}; ' for tag in row['tags'])
            prototype = f'{tags}{row["head"]} {name}({row["params"]});'
            (stage / consumer).write_text(remove_declarations(text, {name}, {name: prototype}), encoding='utf-8',
                                          errors='surrogateescape')
            pending.append((key, stage, consumer))
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        for (key, stage, consumer), (ok, tail) in zip(pending, pool.map(check, [(s, c) for _, s, c in pending])):
            results[key] = dict(ok=ok, tail='' if ok else tail)
            shutil.rmtree(stage, ignore_errors=True)
    trial_path.write_text(json.dumps(results, indent=0, ensure_ascii=False) + '\n', encoding='utf-8')

    failed = {key.split(':')[1] for key, row in results.items() if not row['ok']}
    moved = {name: candidates[name] for name in names if name not in failed}
    print(json.dumps(dict(tried=len(names), failed=len(failed & set(names)), moved=len(moved),
                          pairs_ok=sum(r['ok'] for r in results.values()), pairs=len(results))))
    if not args.write:
        return 0

    # Headers first: a file's final form compiles against them.
    by_owner = collections.defaultdict(list)
    for name, row in moved.items():
        by_owner[row['owner']].append(name)
    for owner, functions in by_owner.items():
        unit, file_name = owner.split('/')
        stem = Path(file_name).stem
        guard = f'INCLUDE_{unit}_{stem}_H'.upper().replace('-', '_')
        tags = sorted({tag for name in functions for tag in moved[name]['tags']})
        lines = [f'#ifndef {guard}', f'#define {guard}', '', f'/* Functions of src/{owner} that other files call. */', '']
        lines += [f'struct {tag};' for tag in tags] + ([''] if tags else [])
        lines += [f'{moved[name]["head"]} {name}({moved[name]["params"]});' for name in sorted(functions)]
        lines += ['', f'#endif /* {guard} */', '']
        path = ROOT / 'include' / unit / f'{stem}.h'
        path.parent.mkdir(exist_ok=True)
        path.write_text('\n'.join(lines), encoding='utf-8')

    headers_by_file = collections.defaultdict(set)
    removed_by_file = collections.defaultdict(set)
    for name, row in moved.items():
        unit, file_name = row['owner'].split('/')
        header = f'{unit}/{Path(file_name).stem}.h'
        headers_by_file[row['owner']].add(header)
        removed_by_file[row['owner']].add(name)     # a forward prototype in the defining file
        for consumer in row['consumers']:
            headers_by_file[consumer].add(header)
            removed_by_file[consumer].add(name)
    final = out / 'final'
    shutil.rmtree(final, ignore_errors=True)
    forms = {}
    for where, headers in headers_by_file.items():
        text = (ROOT / 'src' / where).read_text(encoding='utf-8', errors='surrogateescape')
        new_text = add_includes(remove_declarations(text, removed_by_file[where]), headers)
        if new_text is None:
            continue
        (final / where.split('/')[0]).mkdir(parents=True, exist_ok=True)
        (final / where).write_text(new_text, encoding='utf-8', errors='surrogateescape')
        forms[where] = new_text
    kept = {}
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        wheres = sorted(forms)
        for where, (ok, tail) in zip(wheres, pool.map(check, [(final, w) for w in wheres])):
            if ok:
                (ROOT / 'src' / where).write_text(forms[where], encoding='utf-8', errors='surrogateescape')
            else:
                kept[where] = tail
    (out / 'kept.json').write_text(json.dumps(kept, indent=0, ensure_ascii=False) + '\n', encoding='utf-8')
    print(json.dumps(dict(headers=len(by_owner), files=len(forms), files_rewritten=len(forms) - len(kept), files_kept=len(kept))))
    return 0


if __name__ == '__main__':
    sys.exit(main())
