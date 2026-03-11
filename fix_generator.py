import re

with open('src/generate.c', 'r') as f:
    content = f.read()

# Fix 1: Jitter should only offset wall tiles outward into floor tiles that are NOT part of a 1-wide corridor,
# OR just don't swap, instead just randomly push the wall into the floor by changing the floor to a wall,
# but that shrinks the room. The prompt said:
# "Iterate through sector-boundary wall tiles and apply a small random offset (-1 to +1 tile). This should break up long, straight horizontal and vertical walls."
# Let's rewrite jitter_sector_boundaries. Instead of swapping, let's just make sure we don't break connectivity.
# Actually, the simplest jitter is: if we have a straight wall, we can randomly add a wall tile adjacent to it (if it doesn't block a path).
# Or we can remove a wall tile (turn it to floor).
# Let's use the CA pass logic for connectivity: if a tile has enough floor neighbors, we can safely turn it into a wall.
# Let's just remove the jitter's swap and replace it with:
# If it's a boundary wall, 50% chance to turn it into floor, or 50% chance to pick an adjacent floor tile and turn IT into a wall.
# BUT wait: turning a floor to a wall might block a corridor.
# To be safe: only turn a boundary wall into a floor (this strictly widens, which never breaks connectivity).
# "apply a small random offset (-1 to +1 tile)" - this means some walls move in, some move out.
# If we move a wall out (into the room), we turn a floor into a wall. If we move a wall in (into the solid rock), we turn a wall into a floor.
# To prevent blocking a 1-tile corridor when moving out: we can check if the floor tile we're turning into a wall has at least 4-5 floor neighbors, or we can just only turn wall->floor.
# Actually, it's simpler: just turn boundary wall to floor. That's offset -1.
# Or if we want offset +1, turn a floor adjacent to boundary wall into a wall, but ONLY if that floor has >= 5 floor neighbors (so it's not a corridor).

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
			int sect = cave_sector[y][x];
			if (cave_sector[y-1][x] != sect || cave_sector[y+1][x] != sect ||
				cave_sector[y][x-1] != sect || cave_sector[y][x+1] != sect) {
				is_boundary = TRUE;
			}

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
						if (ny > 0 && ny < DUNGEON_HGT - 1 && nx > 0 && nx < DUNGEON_WID - 1)
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
									if (cave_feat[nny][nnx] == FEAT_FLOOR) floor_neighbors++;
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

content = re.sub(r'static void jitter_sector_boundaries\(void\).*?\}\n\}', new_jitter, content, flags=re.DOTALL)

# Fix 2: Drunkard's Walk Acid should eat through walls.
# "If the element is "Acid," ensure it avoids destroying critical stairs or artifacts but allows it to "eat" through standard walls."

new_streamer2 = """static void build_streamer2(int feat, int killwall)
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

content = re.sub(r'static void build_streamer2\(int feat, int killwall\).*?\}\n\}', new_streamer2, content, flags=re.DOTALL)

with open('src/generate.c', 'w') as f:
    f.write(content)

print("Applied fixes")
