"""Headless smoke test of the Tk UI logic (no mainloop)."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import firmware_generator as fg

app = fg.GeneratorApp()

# Valid entry commits and rebuilds the slider range around the value.
app.entries["brightness"].set("200")
app._commit_entry("brightness")
assert app.vars["brightness"].get() == 200
assert app.scales["brightness"].cget("from") == 100
assert app.scales["brightness"].cget("to") == 255

# Non-numeric entry falls back to the current value without crashing.
app.entries["brightness"].set("abc")
app._commit_entry("brightness")
assert app.vars["brightness"].get() == 200

# Out-of-range entry is clamped and the field shows the clamped value.
app.entries["brightness"].set("9999")
app._commit_entry("brightness")
assert app.vars["brightness"].get() == 255
assert app.entries["brightness"].get() == "255"

# Duration apply with selection present.
app.order_list.selection_set(0)
app.duration.set("7000")
app._apply_duration()
assert app.sequence[0][1].get() == 7000

# Moving a slider updates entry and display immediately (root cause of the
# "no real parameters after slider change" bug).
app.vars["saturation"].set(180.0)
app._on_slider("saturation")
assert app.entries["saturation"].get() == "180"
assert app.displays["saturation"].get() == "180"

# Exact drag callback: the raw string argument must round, not truncate.
app._on_slider("brightness", "129.9")
assert app._read_settings().brightness == 130
assert app.entries["brightness"].get() == "130"

settings = app._read_settings()
assert settings.saturation == 180

app.destroy()
print("UI smoke test OK")
