import re

with open('src/generate.c', 'r') as f:
    content = f.read()

# We need to move the calls to jitter_sector_boundaries() and smooth_caverns()
# to BEFORE tunnel generation.
# Currently they are right before magma streamers.

# Let's remove them from there.
content = content.replace("\t/* Jitter sector boundaries to reduce boxiness */\n\tjitter_sector_boundaries();\n", "")
content = content.replace("\t/* Cellular Automata cave smoothing pass */\n\tsmooth_caverns();\n", "")

# Now let's find a good place. "Hack -- scramble the room order" or "Start with no tunnel doors"
# Let's put it right before "Hack -- scramble the room order"
insertion_point = "\t/* Hack -- Scramble the room order */"

new_calls = """	/* Jitter sector boundaries to reduce boxiness */
	jitter_sector_boundaries();

	/* Cellular Automata cave smoothing pass */
	smooth_caverns();

"""

content = content.replace(insertion_point, new_calls + insertion_point)

with open('src/generate.c', 'w') as f:
    f.write(content)

print("Moved CA smoothing before tunnel generation")
