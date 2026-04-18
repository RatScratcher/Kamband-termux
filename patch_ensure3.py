with open('src/generate.c', 'r') as f:
    content = f.read()

import re

# We will remove both occurrences and put it nicely where apply_sector_borders_and_access is.

# 1. find the first occurrence and remove it
def remove_func(c, func_name):
    start = c.find("static void " + func_name)
    if start == -1: return c
    # find the matching closing brace
    # assuming standard formatting
    stack = 0
    in_func = False
    for i in range(start, len(c)):
        if c[i] == '{':
            stack += 1
            in_func = True
        elif c[i] == '}':
            stack -= 1
            if in_func and stack == 0:
                end = i + 1
                return c[:start] + c[end:]
    return c

content = remove_func(content, "ensure_elevation_access")
content = remove_func(content, "ensure_elevation_access")
content = remove_func(content, "ensure_elevation_access")

with open('src/generate.c', 'w') as f:
    f.write(content)
