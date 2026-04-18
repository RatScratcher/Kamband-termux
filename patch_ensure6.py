# Clean up isolated features is called again at the end, which is perfectly fine to catch anything left over or just as safety.
# We also want to remove it from the end if it's there twice or leave it. Leaving it is fine.
# But wait! I moved clean_isolated_features before ensure_elevation_access. So it is declared BEFORE ensure_elevation_access?
# Let's check where it's declared.

with open('src/generate.c', 'r') as f:
    content = f.read()

import re

start1 = content.find("static void clean_isolated_features")
start2 = content.find("static void ensure_elevation_access")

print(f"clean is at {start1}, ensure is at {start2}")
