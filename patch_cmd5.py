import re

with open('src/cmd5.c', 'r') as f:
    content = f.read()

# We need to insert a check in `generate_mutation()` after `which_mut = rand_int(MAX_MUTS);`
# The code is:
#		which_mut = rand_int(MAX_MUTS);
#		which_var = which_mut / 32;

# Check to insert:
#		if (which_mut >= MUT_MINUS_INT && which_mut <= MUT_MINUS_FIGHT) continue;

replacement = """		which_mut = rand_int(MAX_MUTS);
		if (which_mut >= MUT_MINUS_INT && which_mut <= MUT_MINUS_FIGHT) continue;
		which_var = which_mut / 32;"""

new_content = content.replace("		which_mut = rand_int(MAX_MUTS);\n		which_var = which_mut / 32;", replacement, 1)

with open('src/cmd5.c', 'w') as f:
    f.write(new_content)
print("Added check.")
