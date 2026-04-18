with open('src/generate.c', 'r') as f:
    content = f.read()

# We need to call ensure_elevation_access after the stamps.
# Wait, when does apply_sector_borders_and_access get called?
# It's called in build_sector_populated inside the stamp loop.
# But we can call ensure_elevation_access there or after the loop.
# It makes sense to call it after the entire stamp loop, so that overlapping pits/hills are connected first.

import re

# let's look at build_sector_populated

start = content.find("static void build_sector_populated")
end = content.find("/* 2.5 Secondary Pass: Monsters, Items, Hazards, and Walls */", start)

print(content[start:end+100])
