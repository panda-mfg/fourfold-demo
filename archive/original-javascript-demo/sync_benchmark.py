"""Embed the auditable worker source into the existing inline demo."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
DEMO = Path('/home/shanda/.codex/visualizations/2026/09/12/01a0936a-197d-7e63-9722-9c47d7964f7b/four-color-playground.html')
source = (ROOT / 'demo/benchmark-engine.js').read_text()
text = DEMO.read_text()
text, count = re.subn(r'(<script type="text/plain" data-b-worker>).*?(</script>)', lambda m: m[1] + '\n' + source + '\n  ' + m[2], text, flags=re.S)
assert count == 1
DEMO.write_text(text)
print(f'Embedded benchmark worker: {len(source):,} characters.')
