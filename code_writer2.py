import os

with open('src/generate.c', 'r') as f:
    content = f.read()

jitter_func = """
/*
 * Jitter sector boundaries to break up straight walls.
 */
static void jitter_sector_boundaries(void)
{
	int y, x;
	int dir;

	for (y = 1; y < DUNGEON_HGT - 1; y++)
	{
		for (x = 1; x < DUNGEON_WID - 1; x++)
		{
			/* Only operate on walls */
			if (cave_feat[y][x] < FEAT_WALL_EXTRA) continue;
            if (cave_feat[y][x] > FEAT_WALL_SOLID) continue;

			/* Check if it's on a sector boundary (e.g., adjacent to a different sector) */
            bool is_boundary = FALSE;
            int sect = cave_sector[y][x];

            if (cave_sector[y-1][x] != sect || cave_sector[y+1][x] != sect ||
                cave_sector[y][x-1] != sect || cave_sector[y][x+1] != sect) {
                is_boundary = TRUE;
            }

			if (is_boundary)
			{
                /* 50% chance to skip */
                if (rand_int(100) < 50) continue;

				/* Try to find an adjacent floor to swap with */
				int valid_dirs[8];
				int num_valid = 0;
				for (dir = 0; dir < 8; dir++)
				{
					int ny = y + ddy_ddd[dir];
					int nx = x + ddx_ddd[dir];
					if (ny > 0 && ny < DUNGEON_HGT - 1 && nx > 0 && nx < DUNGEON_WID - 1)
					{
						if (cave_feat[ny][nx] == FEAT_FLOOR)
						{
							valid_dirs[num_valid++] = dir;
						}
					}
				}

				if (num_valid > 0)
				{
					/* Pick a random valid direction */
					dir = valid_dirs[rand_int(num_valid)];
					int ny = y + ddy_ddd[dir];
					int nx = x + ddx_ddd[dir];

					/* Swap features */
					int tmp_feat = cave_feat[y][x];
					int tmp_info = cave_info[y][x];

					cave_feat[y][x] = cave_feat[ny][nx];
					cave_info[y][x] = cave_info[ny][nx];

					cave_feat[ny][nx] = tmp_feat;
					cave_info[ny][nx] = tmp_info;
				}
			}
		}
	}
}
"""

# Insert before cave_gen
new_content = content.replace("static void cave_gen(void)", jitter_func + "\nstatic void cave_gen(void)")

with open('src/generate.c', 'w') as f:
    f.write(new_content)

print("Added jitter_sector_boundaries")
