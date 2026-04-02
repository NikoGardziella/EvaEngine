import os
from pathlib import Path

SWAP = {
    "N": "S",
    "S": "N",
    "E": "W",
    "W": "E",
}

IMAGE_EXTS = {".png", ".jpg", ".jpeg", ".tga", ".bmp", ".dds", ".webp"}

folder = Path(__file__).resolve().parent
self_name = Path(__file__).name

files = []

for p in folder.iterdir():
    if not p.is_file():
        continue
    if p.name == self_name:
        continue
    if p.suffix.lower() not in IMAGE_EXTS:
        continue

    stem = p.stem
    if not stem:
        continue

    last = stem[-1].upper()
    if last not in SWAP:
        continue

    new_last = SWAP[last]
    new_name = stem[:-1] + new_last + p.suffix

    files.append((p, folder / new_name))

if not files:
    print("No matching files found.")
    input("Press Enter to exit...")
    exit()

print("Preview:\n")
for old, new in files:
    print(f"{old.name} -> {new.name}")

confirm = input("\nApply changes? (y/n): ").lower()
if confirm != "y":
    print("Cancelled.")
    exit()

# --- STEP 1: rename everything to temp ---
temp_files = []
for i, (old, new) in enumerate(files):
    tmp = old.with_name(f"__tmp__{i}{old.suffix}")
    old.rename(tmp)
    temp_files.append((tmp, new))

# --- STEP 2: rename temp to final ---
for tmp, final in temp_files:
    tmp.rename(final)

print("Done.")
input("Press Enter to exit...")