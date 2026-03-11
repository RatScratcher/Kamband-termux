import re

with open('src/generate.c', 'r') as f:
    content = f.read()

# I need to see where build_streamer2 is defined
match = re.search(r'static void build_streamer2.+?^}', content, re.MULTILINE | re.DOTALL)
if match:
    print(match.group(0))
