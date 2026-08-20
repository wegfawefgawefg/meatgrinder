# Level data

`assets/levels/campaign.json` contains the ten main battle graphs recovered
from version 2.0.0 `map.dat` records 0 through 9. `tools/import_hrr_campaign.py`
rebuilds it from the reverse-engineering project's generated map catalogue.

Imported fields are deliberately narrow: source record/hash, graph-slot
position, node kind, starting owner/garrison, and bidirectional links. Original
faction zero is the player; any other occupied faction is normalized to the
single prototype AI side. Item slots, events, scripts, text, labels, legacy
cell-plane bytes, and route gates are excluded.

The 20x12 background tile layer is generated deterministically by the importer.
It is not decoded or copied HRR terrain. This keeps presentation replaceable
while letting map composition become authored data immediately.

The schema is intentionally readable. A level has `name`, `source`, `tiles`,
`nodes`, and `links`. Node IDs are stable original graph indices, so research
notes can still refer back to recovered records without leaking runtime identity
into array positions.

