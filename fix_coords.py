import re

with open('src/generate.c', 'r') as f:
    content = f.read()

# Fix smooth_caverns
ca_search = r'static void smooth_caverns\(void\).*?\}\n\}'
old_ca = re.search(ca_search, content, re.DOTALL).group(0)

new_ca = """static void smooth_caverns(void)
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
				int by = y / BLOCK_HGT;
				int bx = x / BLOCK_WID;
				if (by >= MAX_ROOMS_ROW) by = MAX_ROOMS_ROW - 1;
				if (bx >= MAX_ROOMS_COL) bx = MAX_ROOMS_COL - 1;

				/* Only apply to SECTOR_CAVERN to maintain other sector structures */
				if (cave_sector[by][bx] != SECTOR_CAVERN) continue;

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
}"""

content = content.replace(old_ca, new_ca)


# Fix build_streamer2
streamer_search = r'static void build_streamer2\(int feat, int killwall\).*?\}\n\}'
old_streamer = re.search(streamer_search, content, re.DOTALL).group(0)

new_streamer = """static void build_streamer2(int feat, int killwall)
{
	int i, j, mid, tx, ty;
	int y, x, dir;
	int poolchance;
	int poolsize;

	poolchance = randint(10);

	/* Hack -- Choose starting point */
	y = rand_spread(DUNGEON_HGT / 2, 10);
	x = rand_spread(DUNGEON_WID / 2, 15);

	if (poolchance > 2)
	{
		/* Agent-based Drunkard's Walk */
		int life = 50 + randint(150);

		int by = y / BLOCK_HGT;
		int bx = x / BLOCK_WID;
		if (by >= MAX_ROOMS_ROW) by = MAX_ROOMS_ROW - 1;
		if (bx >= MAX_ROOMS_COL) bx = MAX_ROOMS_COL - 1;
		int current_sector = cave_sector[by][bx];

		while (life > 0)
		{
			/* Do not mess up vaults */
			if (in_bounds(y, x) && !(cave_info[y][x] & CAVE_ICKY))
			{
				bool valid = TRUE;
				if (feat == FEAT_ACID)
				{
					/* Acid eats through standard walls, but not permanent walls or stairs/doors */
					if (cave_feat[y][x] >= FEAT_PERM_EXTRA || cave_feat[y][x] == FEAT_LESS || cave_feat[y][x] == FEAT_MORE || (cave_feat[y][x] >= FEAT_DOOR_HEAD && cave_feat[y][x] <= FEAT_DOOR_TAIL))
						valid = FALSE;
				}
				else if (killwall == 0)
				{
					if (cave_feat[y][x] >= FEAT_MAGMA || cave_feat[y][x] == FEAT_LESS || cave_feat[y][x] == FEAT_MORE)
						valid = FALSE;
				}
				else
				{
					if (cave_feat[y][x] >= FEAT_PERM_EXTRA || cave_feat[y][x] == FEAT_LESS || cave_feat[y][x] == FEAT_MORE)
						valid = FALSE;
				}

				if (valid)
				{
					cave_feat[y][x] = feat;
				}
			}

			/* Move to random adjacent tile */
			dir = ddd[rand_int(8)];
			int ny = y + ddy[dir];
			int nx = x + ddx[dir];

			if (in_bounds(ny, nx))
			{
				int nby = ny / BLOCK_HGT;
				int nbx = nx / BLOCK_WID;
				if (nby >= MAX_ROOMS_ROW) nby = MAX_ROOMS_ROW - 1;
				if (nbx >= MAX_ROOMS_COL) nbx = MAX_ROOMS_COL - 1;
				int next_sector = cave_sector[nby][nbx];

				/* Sector overflow check (10% chance to overflow, 90% to stay within sector) */
				if (next_sector != current_sector)
				{
					if (rand_int(100) < 10)
					{
						current_sector = next_sector; /* Overflow allowed, update sector */
						y = ny;
						x = nx;
					}
					else
					{
						/* Try another direction next turn */
					}
				}
				else
				{
					y = ny;
					x = nx;
				}
			}

			life--;
		}
	}
	else if ((feat == FEAT_DEEP_WATER) || (feat == FEAT_DEEP_LAVA) ||
		(feat == FEAT_CHAOS_FOG))
	{ /* create pool */
		poolsize = 5 + randint(10);
		mid = poolsize / 2;
		/* One grid per density */
		for (i = 0; i < poolsize; i++)
		{
			for (j = 0; j < poolsize; j++)
			{
				tx = x + j;
				ty = y + i;

				if (!in_bounds(ty, tx))
					continue;

				if (i < mid)
				{
					if (j < mid)
					{
						if ((i + j + 1) < mid)
							continue;
					}
					else if (j > (mid + i))
						continue;
				}
				else if (j < mid)
				{
					if (i > (mid + j))
						continue;
				}
				else if ((i + j) > ((mid * 3) - 1))
					continue;

				/* Do not mess up vaults */
				if (cave_info[ty][tx] & CAVE_ICKY)
					continue;

				/* Only convert non-permanent features */
				if (cave_feat[ty][tx] >= FEAT_PERM_EXTRA)
					continue;
				if (cave_feat[ty][tx] == FEAT_LESS)
					continue;
				if (cave_feat[ty][tx] == FEAT_MORE)
					continue;
				/* Clear previous contents, add proper vein type */
				cave_feat[ty][tx] = feat;
			}
		}
	}
}"""

content = content.replace(old_streamer, new_streamer)

with open('src/generate.c', 'w') as f:
    f.write(content)

print("Fixed coordinate accesses")
