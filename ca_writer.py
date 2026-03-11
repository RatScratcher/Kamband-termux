import os

with open('src/generate.c', 'r') as f:
    content = f.read()

ca_func = """
/*
 * Cellular Automata (CA) Cave Smoothing Pass.
 * Analyzes wall/floor neighbors and toggles them if they are overwhelmingly
 * one or the other. Softens the "boxiness" of cavern sectors.
 */
static void smooth_caverns(void)
{
	int i, y, x;
	int dir;
	byte temp_feat[DUNGEON_HGT][DUNGEON_WID];

	/* Run for 2 iterations */
	for (i = 0; i < 2; i++)
	{
		/* Copy current state to temp array to allow simultaneous updates */
		for (y = 1; y < DUNGEON_HGT - 1; y++)
		{
			for (x = 1; x < DUNGEON_WID - 1; x++)
			{
				temp_feat[y][x] = cave_feat[y][x];
			}
		}

		for (y = 1; y < DUNGEON_HGT - 1; y++)
		{
			for (x = 1; x < DUNGEON_WID - 1; x++)
			{
				/* Only apply to SECTOR_CAVERN to maintain other sector structures */
				if (cave_sector[y][x] != SECTOR_CAVERN) continue;

				/* Do not overwrite permanent features or stairs/doors */
				if (cave_feat[y][x] >= FEAT_PERM_EXTRA || cave_feat[y][x] == FEAT_LESS || cave_feat[y][x] == FEAT_MORE || (cave_feat[y][x] >= FEAT_DOOR_HEAD && cave_feat[y][x] <= FEAT_DOOR_TAIL)) continue;

				int floor_count = 0;
				int wall_count = 0;

				for (dir = 0; dir < 8; dir++)
				{
					int ny = y + ddy_ddd[dir];
					int nx = x + ddx_ddd[dir];
					if (ny > 0 && ny < DUNGEON_HGT - 1 && nx > 0 && nx < DUNGEON_WID - 1)
					{
						if (temp_feat[ny][nx] == FEAT_FLOOR)
						{
							floor_count++;
						}
						else if (temp_feat[ny][nx] >= FEAT_WALL_EXTRA && temp_feat[ny][nx] <= FEAT_PERM_SOLID)
						{
							wall_count++;
						}
					}
				}

				/* If wall has 5+ floor neighbors, turn to floor */
				if (temp_feat[y][x] >= FEAT_WALL_EXTRA && temp_feat[y][x] <= FEAT_WALL_SOLID && floor_count >= 5)
				{
					cave_feat[y][x] = FEAT_FLOOR;
					cave_info[y][x] |= CAVE_ROOM; /* Treat as room/open area */
				}
				/* If floor has 5+ wall neighbors, turn to wall */
				else if (temp_feat[y][x] == FEAT_FLOOR && wall_count >= 5)
				{
					cave_feat[y][x] = FEAT_WALL_INNER;
					cave_info[y][x] &= ~CAVE_ROOM;
				}
			}
		}
	}
}
"""

# Insert before cave_gen
new_content = content.replace("static void cave_gen(void)", ca_func + "\nstatic void cave_gen(void)")

with open('src/generate.c', 'w') as f:
    f.write(new_content)

print("Added smooth_caverns")
