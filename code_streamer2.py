import re

with open('src/generate.c', 'r') as f:
    content = f.read()

old_func = re.search(r'static void build_streamer2.+?^}', content, re.MULTILINE | re.DOTALL).group(0)

new_func = """static void build_streamer2(int feat, int killwall)
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
		int current_sector = cave_sector[y][x];

		while (life > 0)
		{
			/* Do not mess up vaults */
			if (in_bounds(y, x) && !(cave_info[y][x] & CAVE_ICKY))
			{
				bool valid = TRUE;
				if (killwall == 0)
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
				int next_sector = cave_sector[ny][nx];
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

new_content = content.replace(old_func, new_func)

with open('src/generate.c', 'w') as f:
    f.write(new_content)

print("Modified build_streamer2")
