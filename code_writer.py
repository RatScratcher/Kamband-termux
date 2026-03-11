import re

with open('src/generate.c', 'r') as f:
    content = f.read()

# We need to add jitter_sector_boundaries() and smooth_caverns() before cave_gen.
# Let's see what's just before cave_gen.
