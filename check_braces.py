import sys
code = open('src/dungeon.c', 'r').read()
depth = 0
for i, c in enumerate(code):
    if c == '{': depth += 1
    if c == '}': depth -= 1
print(depth)
