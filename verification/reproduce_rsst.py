#!/usr/bin/env python3
"""Fetch hash-pinned RSST research sources and replay the original C checks.

The original programs grant use for scholarly research; they are fetched into
ignored build storage, rather than relicensed as this repository's own code.
"""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import subprocess
import urllib.request

ROOT = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--fetch', action='store_true', help='Download missing pinned files.')
parser.add_argument('--check-only', action='store_true', help='Validate hashes without compiling/running.')
parser.add_argument('--timeout', type=float, default=600, help='Seconds per proof checker, default 600.')
args = parser.parse_args()
manifest = json.loads((ROOT / 'rsst/sources.json').read_text())
work = ROOT / 'build/rsst'
work.mkdir(parents=True, exist_ok=True)
for item in manifest['files']:
    path = work / item['name']
    if not path.exists():
        if not args.fetch:
            raise SystemExit(f'Missing {path}; use --fetch for the author-hosted research files.')
        with urllib.request.urlopen(item['url'], timeout=60) as response:
            data = response.read()
        if hashlib.sha256(data).hexdigest() != item['sha256']:
            raise SystemExit(f'Download hash mismatch: {item["name"]}')
        path.write_bytes(data)
    if hashlib.sha256(path.read_bytes()).hexdigest() != item['sha256']:
        raise SystemExit(f'Hash mismatch: {path}')
catalogue = ROOT.parent / 'rust/data/rsst-unavoidable.conf'
assert catalogue.read_bytes() == (work / 'unavoidable.conf').read_bytes()
if args.check_only:
    print('All nine author files and the bundled 633-configuration catalogue match their hashes.')
    raise SystemExit(0)

out = ROOT / 'results/rsst'
out.mkdir(parents=True, exist_ok=True)
flags = ['-O2', '-std=gnu89', '-DPROTOTYPE_MAX']
for name in ['reduce', 'discharge']:
    extra = ['-include', 'string.h'] if name == 'discharge' else []
    subprocess.run(['cc', *flags, *extra, str(work / f'{name}.c'), '-o', str(work / name)], check=True)
jobs = [('reducibility', ['reduce', 'unavoidable.conf'], 'Reducibility of 633 configurations verified')]
jobs += [(f'discharge{i}', ['discharge', f'present{i}'], f'present{i} verified.') for i in range(7, 12)]
checks = []
for name, command, expected in jobs:
    path = out / f'{name}.log'
    with path.open('w') as stream:
        result = subprocess.run([str(work / command[0]), *command[1:]], cwd=work,
                                stdout=stream, stderr=subprocess.STDOUT, timeout=args.timeout)
    lines = path.read_text().splitlines()
    ok = result.returncode == 0 and bool(lines) and lines[-1] == expected
    checks.append({'check': name, 'exit_code': result.returncode, 'log': path.name,
                   'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                   'last_line': lines[-1] if lines else '', 'passed': ok})
    if not ok:
        raise SystemExit(f'{name} did not verify: inspect {path}')
    print(f'{name}: verified')
summary = {'generated_at': datetime.datetime.now(datetime.timezone.utc).isoformat(),
           'compiler': subprocess.check_output(['cc', '--version'], text=True).splitlines()[0],
           'flags': flags, 'discharge_additional_flag': ['-include', 'string.h'],
           'source_manifest': '../../rsst/sources.json',
           'all_633_reducibility_checks_passed': True,
           'all_five_unavoidability_certificates_passed': True,
           'formal_verification_of_rust': False, 'checks': checks}
(out / 'proof-replay.json').write_text(json.dumps(summary, indent=2) + '\n')
