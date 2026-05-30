import re

def update_file():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    # The previous patch introduced chest_track_globals. Let's remove it and use o_list!
    # Because if we use chest_y, chest_x, we'd need to modify `place_object` or similar to populate them.
    # But o_list is a linked list of all objects. We can just iterate it.

    old_chest_track = """/*
 * Cache chest locations during generation to speed up place_traps_near_chests
 */
#define MAX_CHESTS_PER_LEVEL 200
static int chest_y[MAX_CHESTS_PER_LEVEL];
static int chest_x[MAX_CHESTS_PER_LEVEL];
static int chest_count = 0;

static void place_traps_near_chests(int chance)
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

    new_chest_track = """static void place_traps_near_chests(int chance)
{
	int dy, dx;
	object_type *o_ptr;

	/* Iterate over all generated objects to find chests */
	for (o_ptr = o_list; o_ptr; o_ptr = o_ptr->next)
	{
		if (o_ptr->tval == TV_CHEST)
		{
			int y = o_ptr->iy;
			int x = o_ptr->ix;

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

    content = content.replace(old_chest_track, new_chest_track)

    with open('src/generate.c', 'w') as f:
        f.write(content)

update_file()
