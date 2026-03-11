import re

with open('src/generate.c', 'r') as f:
    content = f.read()

# I need to call it in cave_gen. Let's find "Build Special Sectors" and insert it after.
# Wait, room building happens right after "Build some rooms".
# Where should I put it? After all rooms are connected? Yes, maybe just before CA smoothing, or right after room generation.
# Let's search for "Add some magma streamers" which is after interconnectivity.
