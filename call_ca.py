import os

with open('src/generate.c', 'r') as f:
    content = f.read()

new_call = """
	/* Cellular Automata cave smoothing pass */
	smooth_caverns();
"""

new_content = content.replace("\t/* Hack -- Add some magma streamers */", new_call + "\n\t/* Hack -- Add some magma streamers */")

with open('src/generate.c', 'w') as f:
    f.write(new_content)

print("Called smooth_caverns")
