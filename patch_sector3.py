with open('src/generate.c', 'r') as f:
    content = f.read()

# I placed ensure_elevation_access after build_sector_populated, so it's undeclared.
# Or rather I placed it replacing ensure_connectivity maybe?
# Let's see where ensure_elevation_access is defined.
import re

start = content.find("static void ensure_elevation_access")
print(start)
