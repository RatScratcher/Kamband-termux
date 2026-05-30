import re

def update_file():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    # Modifying cave_gen signature
    old_sig = "static void cave_gen(void)"
    new_sig = "static void cave_gen(dun_data *dun_body)"
    content = content.replace(old_sig, new_sig)

    # Remove allocation in cave_gen
    old_alloc = """	bool lit_level = FALSE;

	dun_data *dun_body = malloc(sizeof(dun_data));
	if (!dun_body) return; /* safety fallback */

	byte level_bg = FEAT_WALL_EXTRA;"""
    new_alloc = """	bool lit_level = FALSE;

	byte level_bg = FEAT_WALL_EXTRA;"""
    content = content.replace(old_alloc, new_alloc)

    # Remove free in cave_gen
    old_free = """	free(dun_body);
	dun = NULL;"""
    new_free = """	dun = NULL;"""
    content = content.replace(old_free, new_free)

    # Modify generate_cave to allocate dun_body outside the retry loop
    old_generate = """void generate_cave(void)
{
	int num;
	int w, h;
	const char *msg = "Generating level... please wait.";

	/* Clear the screen and alert the player */"""
    new_generate = """void generate_cave(void)
{
	int num;
	int w, h;
	const char *msg = "Generating level... please wait.";
	dun_data *dun_body;

	/* Clear the screen and alert the player */"""
    content = content.replace(old_generate, new_generate)

    # In generate_cave:
    old_seed = """	/* Seed the RNG if appropriate */
	if (seed_dungeon)
	{
		Rand_quick = TRUE;
		Rand_value = seed_dungeon + p_ptr->depth;
	}

	/* Generate */"""
    new_seed = """	/* Seed the RNG if appropriate */
	if (seed_dungeon)
	{
		Rand_quick = TRUE;
		Rand_value = seed_dungeon + p_ptr->depth;
	}

	dun_body = malloc(sizeof(dun_data));
	if (!dun_body) return;

	/* Generate */"""
    content = content.replace(old_seed, new_seed)

    # Now patch the calls inside generate_cave loop
    content = content.replace("cave_gen();", "cave_gen(dun_body);")

    # Finally, free dun_body at the end of generate_cave
    old_free_gen = """	/* Mega-Hack -- no panels yet */
	p_ptr->wy = 0;
	p_ptr->wx = 0;
}"""
    new_free_gen = """	/* Mega-Hack -- no panels yet */
	p_ptr->wy = 0;
	p_ptr->wx = 0;

	if (dun_body) free(dun_body);
}"""
    content = content.replace(old_free_gen, new_free_gen)

    with open('src/generate.c', 'w') as f:
        f.write(content)

update_file()
