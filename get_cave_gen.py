import re

with open('src/generate.c', 'r') as f:
    content = f.read()

# Find cave_gen and its content
match = re.search(r'static void cave_gen\(void\)\s*\{(.+?)\n\}', content, re.DOTALL)
if match:
    print(match.group(0))
