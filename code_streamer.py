import re

with open('src/generate.c', 'r') as f:
    content = f.read()

# I need to modify build_streamer2 completely.
# Let's find where build_streamer2 starts and ends.
