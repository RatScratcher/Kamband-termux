with open('src/tables.c', 'r') as f:
    lines = f.readlines()

for i, line in enumerate(lines):
    if "Feeble Mind" in line:
        print(f"ERROR: Found 'Feeble Mind' at line {i+1}")
    if "Echolocation Pulse" in line:
        print(f"Found 'Echolocation Pulse' at line {i+1}")
