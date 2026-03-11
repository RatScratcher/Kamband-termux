import re

with open('src/generate.c', 'r') as f:
    content = f.read()

# Find cave_gen and its content
match = re.search(r'static void cave_gen\(void\)\s*\{(.+?)\n\}', content, re.DOTALL)
if match:
    lines = match.group(1).split('\n')
    for i, line in enumerate(lines):
        if 'for' in line or 'while' in line or '/* ' in line or 'build_' in line or 'cave_sector' in line:
            print(f"{i}: {line.strip()}")
