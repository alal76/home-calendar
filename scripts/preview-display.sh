#!/usr/bin/env bash
# ────────────────────────────────────────────────────────────────
#  preview-display.sh
#  Fetches the latest calendar events from Home Assistant and
#  renders an HTML preview that mirrors what the Waveshare e-paper
#  is drawing (same 800×480 layout / colour palette).
#  Generates ./preview.html in the repo root and opens it.
# ────────────────────────────────────────────────────────────────
set -euo pipefail

HA_HOST="${HA_HOST:-homeassistant.local}"
HA_PORT="${HA_PORT:-22}"
HA_USER="${HA_USER:-root}"

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/preview.html"
TPL="$ROOT/scripts/preview.template.html"
TMP="$ROOT/.preview-data.json"

[[ -f "$TPL" ]] || { echo "missing template: $TPL" >&2; exit 1; }

echo "→ Fetching sensor.calendar_events_json from HA ($HA_HOST)..."
ssh -p "$HA_PORT" "$HA_USER@$HA_HOST" '
  curl -sS -H "Authorization: Bearer $SUPERVISOR_TOKEN" \
    http://supervisor/core/api/states/sensor.calendar_events_json
' > "$TMP"

python3 - "$TPL" "$OUT" "$TMP" <<'PY'
import json, sys, datetime
tpl_p, out_p, data_p = sys.argv[1], sys.argv[2], sys.argv[3]

raw = json.load(open(data_p))
entity = raw.get("entity_id", "?")
state  = raw.get("state", "?")
attr   = raw.get("attributes", {})
wrap   = attr.get("events", {})

# HA may serialise the attribute as a dict {"events":[...]} (our template)
# or, if someone reverts to a bare list, just a list. Accept both.
if isinstance(wrap, dict):
    events = wrap.get("events", [])
elif isinstance(wrap, list):
    events = wrap
else:
    events = []

print(f"  loaded {len(events)} events (state={state})")

html = open(tpl_p).read()
html = html.replace("__EVENTS_JSON__",   json.dumps(events))
html = html.replace("__GENERATED_AT__",  datetime.datetime.now().isoformat(timespec="seconds"))
html = html.replace("__SOURCE__",        entity)
html = html.replace("__STATE_COUNT__",   str(state))
open(out_p, "w").write(html)
print(f"  wrote {out_p}")
PY

rm -f "$TMP"

# macOS opens in default browser; on Linux fall back to xdg-open
if command -v open >/dev/null 2>&1; then
  open "$OUT"
elif command -v xdg-open >/dev/null 2>&1; then
  xdg-open "$OUT"
else
  echo "Open manually: $OUT"
fi
