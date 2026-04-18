with open('src/generate.c', 'r') as f:
    content = f.read()

# I messed up and added it twice or it was already added partially?
# Wait, let's remove the duplicated one
import re

# Find all occurrences of "static void ensure_elevation_access"
matches = [m.start() for m in re.finditer(r'static void ensure_elevation_access', content)]
print(matches)
