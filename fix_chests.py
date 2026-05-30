import re

def update_file():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    old_place_traps = """static void place_traps_near_chests(int chance)
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

    new_place_traps = """static void place_traps_near_chests(int chance)
{
	int dy, dx, i;
	object_type *o_ptr;

	/* Iterate over all generated objects array to find chests */
	for (i = 1; i < o_max; i++)
	{
		o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

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
    content = content.replace(old_place_traps, new_place_traps)

    with open('src/generate.c', 'w') as f:
        f.write(content)

update_file()
