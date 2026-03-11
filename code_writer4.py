import re

with open('src/generate.c', 'r') as f:
    content = f.read()

# I will insert the call just before magma streamers
new_call = """
	/* Jitter sector boundaries to reduce boxiness */
	jitter_sector_boundaries();
"""

new_content = content.replace("\t/* Hack -- Add some magma streamers */", new_call + "\n\t/* Hack -- Add some magma streamers */")

with open('src/generate.c', 'w') as f:
    f.write(new_content)

print("Called jitter_sector_boundaries")
