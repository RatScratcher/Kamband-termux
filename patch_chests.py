import re

def update_file():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    # Track chest locations globally
    chest_track_globals = """
/*
 * Cache chest locations during generation to speed up place_traps_near_chests
 */
#define MAX_CHESTS_PER_LEVEL 200
static int chest_y[MAX_CHESTS_PER_LEVEL];
static int chest_x[MAX_CHESTS_PER_LEVEL];
static int chest_count = 0;
"""

    # We will inject this before place_traps_near_chests
    content = re.sub(r'static void place_traps_near_chests\(int chance\)',
                     chest_track_globals + '\nstatic void place_traps_near_chests(int chance)',
                     content)

    old_place_traps = """static void place_traps_near_chests(int chance)
{
	int y, x;
	int dy, dx;

	/* Scan dungeon for chests */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{
			object_type *o_ptr;
			bool is_chest = FALSE;

			/* Check for chest */
			for (o_ptr = cave_o_idx[y][x]; o_ptr; o_ptr = o_ptr->next)
			{
				if (o_ptr->tval == TV_CHEST)
				{
					is_chest = TRUE;
					break;
				}
			}

			if (!is_chest) continue;

			/* Try adjacent grids */
			for (dy = -1; dy <= 1; dy++)
			{
				for (dx = -1; dx <= 1; dx++)
				{
					/* Skip self */
					if (!dy && !dx) continue;

					/* Check chance */
					if (rand_int(100) >= chance) continue;

					/* Place trap */
					if (in_bounds(y + dy, x + dx) && cave_naked_bold(y + dy, x + dx))
					{
						place_trap(y + dy, x + dx);
					}
				}
			}
		}
	}
}"""

    new_place_traps = """static void place_traps_near_chests(int chance)
{
	int dy, dx, i;

	/* Iterate over cached chest locations */
	for (i = 0; i < chest_count; i++)
	{
		int y = chest_y[i];
		int x = chest_x[i];

		/* Try adjacent grids */
		for (dy = -1; dy <= 1; dy++)
		{
			for (dx = -1; dx <= 1; dx++)
			{
				/* Skip self */
				if (!dy && !dx) continue;

				/* Check chance */
				if (rand_int(100) >= chance) continue;

				/* Place trap */
				if (in_bounds(y + dy, x + dx) && cave_naked_bold(y + dy, x + dx))
				{
					place_trap(y + dy, x + dx);
				}
			}
		}
	}
}"""
    content = content.replace(old_place_traps, new_place_traps)

    with open('src/generate.c', 'w') as f:
        f.write(content)

update_file()
