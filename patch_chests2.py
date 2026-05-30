import re

def update_file():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    # Reset chest_count in generate_cave when resetting the level
    reset_code = """
		/* Reset the object generation level */
		object_level = p_ptr->depth;
"""
    new_reset_code = """
		/* Reset the object generation level */
		object_level = p_ptr->depth;

		/* Reset tracked chests */
		chest_count = 0;
"""
    content = content.replace(reset_code, new_reset_code)

    with open('src/generate.c', 'w') as f:
        f.write(content)

    with open('src/object2.c', 'r') as f:
        content = f.read()

    # We also need to expose chest_count from generate.c or track them when placed.
    # Wait, TV_CHEST is usually placed inside `make_object` / `object_prep`.
    # But wait, generate.c itself places objects directly?
    # Actually, place_object() is in object2.c. We can track it there if we export the variables,
    # or track it inside object2.c and expose the tracking to generate.c?
    # Actually we can just do the tracking inside place_object in object2.c, or use `o_list` in place_traps_near_chests!
