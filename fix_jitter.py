import re

with open('src/generate.c', 'r') as f:
    content = f.read()

# Fix jitter_sector_boundaries
jitter_search = r'static void jitter_sector_boundaries\(void\).*?\}\n\}'
old_jitter = re.search(jitter_search, content, re.DOTALL).group(0)

new_jitter = """static void jitter_sector_boundaries(void)
{
	int y, x;
	int dir;

	for (y = 1; y < DUNGEON_HGT - 1; y++)
	{
		for (x = 1; x < DUNGEON_WID - 1; x++)
		{
			if (cave_feat[y][x] < FEAT_WALL_EXTRA) continue;
			if (cave_feat[y][x] > FEAT_WALL_SOLID) continue;

			bool is_boundary = FALSE;
			int by = y / BLOCK_HGT;
			int bx = x / BLOCK_WID;
			if (by >= MAX_ROOMS_ROW) by = MAX_ROOMS_ROW - 1;
			if (bx >= MAX_ROOMS_COL) bx = MAX_ROOMS_COL - 1;
			int sect = cave_sector[by][bx];

			int by_minus = (y - 1) / BLOCK_HGT;
			int bx_minus = (x - 1) / BLOCK_WID;
			int by_plus = (y + 1) / BLOCK_HGT;
			int bx_plus = (x + 1) / BLOCK_WID;

			if (by_minus >= 0 && cave_sector[by_minus][bx] != sect) is_boundary = TRUE;
			if (by_plus < MAX_ROOMS_ROW && cave_sector[by_plus][bx] != sect) is_boundary = TRUE;
			if (bx_minus >= 0 && cave_sector[by][bx_minus] != sect) is_boundary = TRUE;
			if (bx_plus < MAX_ROOMS_COL && cave_sector[by][bx_plus] != sect) is_boundary = TRUE;

			if (is_boundary)
			{
				int roll = rand_int(100);
				if (roll < 33)
				{
					/* Offset -1: turn wall into floor */
					cave_feat[y][x] = FEAT_FLOOR;
					cave_info[y][x] |= CAVE_ROOM;
				}
				else if (roll < 66)
				{
					/* Offset +1: turn an adjacent floor into a wall, if it's safe */
					for (dir = 0; dir < 8; dir++)
					{
						int ny = y + ddy_ddd[dir];
						int nx = x + ddx_ddd[dir];
						if (in_bounds(ny, nx))
						{
							if (cave_feat[ny][nx] == FEAT_FLOOR)
							{
								/* Check if it's safe to turn into wall (not a corridor) */
								int floor_neighbors = 0;
								int ndir;
								for (ndir = 0; ndir < 8; ndir++)
								{
									int nny = ny + ddy_ddd[ndir];
									int nnx = nx + ddx_ddd[ndir];
									if (in_bounds(nny, nnx) && cave_feat[nny][nnx] == FEAT_FLOOR) floor_neighbors++;
								}
								if (floor_neighbors >= 5)
								{
									cave_feat[ny][nx] = FEAT_WALL_INNER;
									cave_info[ny][nx] &= ~CAVE_ROOM;
									break; /* Only do one */
								}
							}
						}
					}
				}
			}
		}
	}
}"""

content = content.replace(old_jitter, new_jitter)

with open('src/generate.c', 'w') as f:
    f.write(content)

print("Fixed jitter coordinates")
