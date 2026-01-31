import sys
code = open('src/dungeon.c', 'r').readlines()
depth = 0
for i, line in enumerate(code):
    for c in line:
        if c == '{': depth += 1
        if c == '}': depth -= 1
    if i == 1671: # line 1672 (index 1671)
        print(f"Depth at 1672: {depth}")
        break
