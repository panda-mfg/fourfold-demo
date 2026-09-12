"""Rebuild the standalone demo using Python's standard library."""
from pathlib import Path
from html import escape
import argparse
import re
import base64
import subprocess

ROOT = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--check', action='store_true', help='Fail if index.html needs rebuilding.')
parser.add_argument('--rust', action='store_true', help='Compile the Rust/WASM engine before rebuilding.')
args = parser.parse_args()
if args.rust:
    subprocess.run(['cargo', 'build', '--manifest-path', str(ROOT / 'rust/Cargo.toml'), '--target', 'wasm32-unknown-unknown', '--lib', '--release', '--offline'], check=True)
    (ROOT / 'assets/fourfold_engine.wasm').write_bytes((ROOT / 'rust/target/wasm32-unknown-unknown/release/fourfold_engine.wasm').read_bytes())
fragment_path = ROOT / 'src/four-color-playground.html'
fragment = fragment_path.read_text(encoding='utf-8')
worker = '\n'.join((ROOT / ('demo/' + name)).read_text(encoding='utf-8') for name in ['map-generator.js', 'wasm-adapter.js', 'benchmark-engine.js'])
assert worker.count('__FOURFOLD_WASM_BASE64__') == 1
worker = worker.replace('__FOURFOLD_WASM_BASE64__', base64.b64encode((ROOT / 'assets/fourfold_engine.wasm').read_bytes()).decode('ascii'))
fragment, count = re.subn(
    r'(<script type="text/plain" data-b-worker>).*?(</script>)',
    lambda match: match[1] + '\n' + worker + '\n  ' + match[2],
    fragment,
    flags=re.S,
)
assert count == 1, 'Expected exactly one embedded worker.'
pool = (ROOT / 'demo/worker-pool.js').read_text(encoding='utf-8')
fragment, count = re.subn(r'(<script data-b-pool>).*?(</script>)', lambda match: match[1] + '\n' + pool + '\n  ' + match[2], fragment, flags=re.S)
assert count == 1, 'Expected exactly one worker pool script.'
shell = (ROOT / 'src/standalone-shell.html').read_text(encoding='utf-8')
assert shell.count('@@FOURFOLD_FRAGMENT@@') == 1
document = shell.replace('@@FOURFOLD_FRAGMENT@@', escape(fragment))
target = ROOT / 'index.html'
if args.check:
    assert target.read_text(encoding='utf-8') == document, 'Run python3 build.py to update index.html.'
    assert fragment_path.read_text(encoding='utf-8') == fragment, 'Embedded worker needs updating.'
    print('Standalone page and embedded worker are current.')
else:
    fragment_path.write_text(fragment, encoding='utf-8')
    target.write_text(document, encoding='utf-8')
    print('Built index.html.')
