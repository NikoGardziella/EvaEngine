import yaml
import shutil
import os

def parse_value(value: str):
    value = value.strip()

    if value.lower() in ("true", "false"):
        return value.lower() == "true"

    try:
        return int(value)
    except ValueError:
        pass

    try:
        return float(value)
    except ValueError:
        pass

    return value


file_path = input("YAML file to edit: ").strip().strip('"')

if not os.path.exists(file_path):
    print(f"File not found: {file_path}")
    exit(1)

section = input("Section to edit, for example CompactTiles or CompactGroups: ").strip()

line = input("Line to add to each item, for example Floor: 0: ").strip()

if ":" not in line:
    print("Invalid line. Use format like: Floor: 0")
    exit(1)

key, value = line.split(":", 1)
key = key.strip()
value = parse_value(value)

with open(file_path, "r", encoding="utf-8") as f:
    data = yaml.safe_load(f)

if section not in data:
    print(f"Section not found: {section}")
    exit(1)

if not isinstance(data[section], list):
    print(f"{section} is not a list.")
    exit(1)

backup_path = file_path + ".bak"
shutil.copy(file_path, backup_path)
print(f"Backup created: {backup_path}")

count = 0

for item in data[section]:
    if isinstance(item, dict):
        item.setdefault(key, value)  # does not overwrite existing value
        count += 1

with open(file_path, "w", encoding="utf-8") as f:
    yaml.dump(data, f, sort_keys=False)

print(f"Done. Added '{key}: {value}' to {count} items in {section}.")
print(f"Edited file: {file_path}")