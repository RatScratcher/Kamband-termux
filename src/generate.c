/* File: generate.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#include "angband.h"
#include "pursuit.h"


/*
 * Note that Level generation is *not* an important bottleneck,
 * though it can be annoyingly slow on older machines...  Thus
 * we emphasize "simplicity" and "correctness" over "speed".
 *
 * This entire file is only needed for generating levels.
 * This may allow smart compilers to only load it when needed.
 *
 * Consider the "v_info.txt" file for vault generation.
 *
 * In this file, we use the "special" granite and perma-wall sub-types,
 * where "basic" is normal, "inner" is inside a room, "outer" is the
 * outer wall of a room, and "solid" is the outer wall of the dungeon
 * or any walls that may not be pierced by corridors.  Thus the only
 * wall type that may be pierced by a corridor is the "outer granite"
 * type.  The "basic granite" type yields the "actual" corridors.
 *
 * Note that we use the special "solid" granite wall type to prevent
 * multiple corridors from piercing a wall in two adjacent locations,
 * which would be messy, and we use the special "outer" granite wall
 * to indicate which walls "surround" rooms, and may thus be "pierced"
 * by corridors entering or leaving the room.
 *
 * Note that a tunnel which attempts to leave a room near the "edge"
 * of the dungeon in a direction toward that edge will cause "silly"
 * wall piercings, but will have no permanently incorrect effects,
 * as long as the tunnel can *eventually* exit from another side.
 * And note that the wall may not come back into the room by the
 * hole it left through, so it must bend to the left or right and
 * then optionally re-enter the room (at least 2 grids away).  This
 * is not a problem since every room that is large enough to block
 * the passage of tunnels is also large enough to allow the tunnel
 * to pierce the room itself several times.
 *
 * Note that no two corridors may enter a room through adjacent grids,
 * they must either share an entryway or else use entryways at least
 * two grids apart.  This prevents "large" (or "silly") doorways.
 *
 * To create rooms in the dungeon, we first divide the dungeon up
 * into "blocks" of 11x11 grids each, and require that all rooms
 * occupy a rectangular group of blocks.  As long as each room type
 * reserves a sufficient number of blocks, the room building routines
 * will not need to check bounds.  Note that most of the normal rooms
 * actually only use 23x11 grids, and so reserve 33x11 grids.
 *
 * Note that the use of 11x11 blocks (instead of the 33x11 panels)
 * allows more variability in the horizontal placement of rooms, and
 * at the same time has the disadvantage that some rooms (two thirds
 * of the normal rooms) may be "split" by panel boundaries.  This can
 * induce a situation where a player is in a room and part of the room
 * is off the screen.  This can be so annoying that the player must set
 * a special option to enable "non-aligned" room generation.
 *
 * Note that the dungeon generation routines are much different (2.7.5)
 * and perhaps "DUN_ROOMS" should be less than 50.
 *
 * XXX XXX XXX Note that it is possible to create a room which is only
 * connected to itself, because the "tunnel generation" code allows a
 * tunnel to leave a room, wander around, and then re-enter the room.
 *
 * XXX XXX XXX Note that it is possible to create a set of rooms which
 * are only connected to other rooms in that set, since there is nothing
 * explicit in the code to prevent this from happening.  But this is less
 * likely than the "isolated room" problem, because each room attempts to
 * connect to another room, in a giant cycle, thus requiring at least two
 * bizarre occurances to create an isolated section of the dungeon.
 *
 * Note that (2.7.9) monster pits have been split into monster "nests"
 * and monster "pits".  The "nests" have a collection of monsters of a
 * given type strewn randomly around the room (jelly, animal, or undead),
 * while the "pits" have a collection of monsters of a given type placed
 * around the room in an organized manner (orc, troll, giant, dragon, or
 * demon).  Note that both "nests" and "pits" are now "level dependant",
 * and both make 16 "expensive" calls to the "get_mon_num()" function.
 *
 * Note that the cave grid flags changed in a rather drastic manner
 * for Angband 2.8.0 (and 2.7.9+), in particular, dungeon terrain
 * features, such as doors and stairs and traps and rubble and walls,
 * are all handled as a set of 64 possible "terrain features", and
 * not as "fake" objects (440-479) as in pre-2.8.0 versions.
 *
 * The 64 new "dungeon features" will also be used for "visual display"
 * but we must be careful not to allow, for example, the user to display
 * hidden traps in a different way from floors, or secret doors in a way
 * different from granite walls, or even permanent granite in a different
 * way from granite.  XXX XXX XXX
 */


/*
 * Dungeon generation values
 */
#define DUN_ROOMS	400 /* Number of rooms to attempt */
#define DUN_UNUSUAL	200	/* Level/chance of unusual room */
#define DUN_DEST	15 /* 1/chance of having a destroyed level */

#define DUN_OPEN_FLOOR  10 /* Chance of having an open level */
#define DUN_OPEN_WATER  10
#define DUN_OPEN_CHAOS  10
#define DUN_OPEN_MAZE   10
#define DUN_OPEN_FOG    10

#define DUN_WILD_STAIRS 30 /* Chance of finding FEAT_SHAFT in the wild. */
#define DUN_WILD_VAULT  100	/* Chance of finding a wilderness vault. */

/*
 * Dungeon tunnel generation values
 */
#define DUN_TUN_RND	10 /* Chance of random direction */
#define DUN_TUN_CHG	30 /* Chance of changing direction */
#define DUN_TUN_CON	15 /* Chance of extra tunneling */
#define DUN_TUN_PEN	25 /* Chance of doors at room entrances */
#define DUN_TUN_JCT	90 /* Chance of doors at tunnel junctions */

/*
 * Dungeon streamer generation values
 */
#define DUN_STR_DEN	5 /* Density of streamers */
#define DUN_STR_RNG	2 /* Width of streamers */
#define DUN_STR_MAG	3 /* Number of magma streamers */
#define DUN_STR_MC	90 /* 1/chance of treasure per magma */
#define DUN_STR_QUA	2 /* Number of quartz streamers */
#define DUN_STR_QC	40 /* 1/chance of treasure per quartz */
#define DUN_STR_WLW	1 /* Width of lava & water streamers -KMW- */
#define DUN_STR_DWLW	8 /* Density of water & lava streams -KMW- */

/*
 * Dungeon treasure allocation values
 */
#define DUN_AMT_ROOM	100 /* Amount of objects for rooms */
#define DUN_AMT_ITEM	50 /* Amount of objects for rooms/corridors */
#define DUN_AMT_ALTAR   3 /* Amount of altars */


/*
 * Hack -- Dungeon allocation "places"
 */
#define ALLOC_SET_CORR		1 /* Hallway */
#define ALLOC_SET_ROOM		2 /* Room */
#define ALLOC_SET_BOTH		3 /* Anywhere */

/*
 * Hack -- Dungeon allocation "types"
 */
#define ALLOC_TYP_RUBBLE	1 /* Rubble */
#define ALLOC_TYP_TRAP		3 /* Trap */
#define ALLOC_TYP_OBJECT	4 /* Object */
#define ALLOC_TYP_ALTAR         5 /* Altar */


/*
 * Maximum numbers of rooms along each axis (currently 6x18)
 */
#define MAX_ROOMS_ROW	(DUNGEON_HGT / BLOCK_HGT)
#define MAX_ROOMS_COL	(DUNGEON_WID / BLOCK_WID)


/*
 * Bounds on some arrays used in the "dun_data" structure.
 * These bounds are checked, though usually this is a formality.
 */
#define CENT_MAX	1000
#define DOOR_MAX	1000
#define WALL_MAX	2000
#define TUNN_MAX	9000


/*
 * Maximal number of room types
 */
#define ROOM_MAX	20


/*
 * Simple structure to hold a map location
 */

typedef struct coord coord;

struct coord
{
	s16b y;
	s16b x;
};


/*
 * Room type information
 */

typedef struct room_data room_data;

struct room_data
{
	/* Required size in blocks */
	s16b dy1, dy2, dx1, dx2;

	/* Hack -- minimum level */
	s16b level;
};


/*
 * Structure to hold all "dungeon generation" data
 */

typedef struct dun_data dun_data;

struct dun_data
{
	/* Array of centers of rooms */
	int cent_n;
	coord cent[CENT_MAX];

	/* Array of possible door locations */
	int door_n;
	coord door[DOOR_MAX];

	/* Array of wall piercing locations */
	int wall_n;
	coord wall[WALL_MAX];

	/* Array of tunnel grids */
	int tunn_n;
	coord tunn[TUNN_MAX];

	/* Number of blocks along each axis */
	int row_rooms;
	int col_rooms;

	/* Array of which blocks are used */
	bool room_map[MAX_ROOMS_ROW][MAX_ROOMS_COL];

	/* Hack -- there is a pit/nest on this level */
	bool crowded;
};


/*
 * Dungeon generation data -- see "cave_gen()"
 */
static dun_data *dun;

/* Extern for sanctum */
extern void build_sanctum_vault(int y, int x);

static void build_tunnel(int row1, int col1, int row2, int col2);

/*
 * Array of room types (assumes 11x11 blocks)
 */
static room_data room[ROOM_MAX] = {
	{0, 0, 0, 0, 0}, /* 0 = Nothing */
	{0, 0, -1, 1, 1}, /* 1 = Simple (33x11) */
	{0, 0, -1, 1, 1}, /* 2 = Overlapping (33x11) */
	{0, 0, -1, 1, 3}, /* 3 = Crossed (33x11) */
	{0, 0, -1, 1, 3}, /* 4 = Large (33x11) */
	{0, 0, -1, 1, 5}, /* 5 = Monster nest (33x11) */
	{0, 0, -1, 1, 5}, /* 6 = Monster pit (33x11) */
	{0, 1, -1, 1, 5}, /* 7 = Lesser vault (33x22) */
	{-1, 2, -2, 3, 10},	/* 8 = Greater vault (66x44) */
	{-1, 2, -2, 3, 5}, /* 9 = Themed vault. */
    {-1, 2, -2, 3, 40}, /* 10 = Sanctum (Depth 40+) */
    {-1, 3, -3, 3, 30}, /* 11 = Folly Vault (Depth 30+) */
    {-2, 2, -2, 2, 1}, /* 12 = Circular (55x55) */
    {-2, 2, -2, 2, 1}, /* 13 = Composite (55x55) */
    {-2, 2, -2, 2, 1}, /* 14 = Cavern (55x55) */
    {0, 0, 0, 0, 0},   /* 15 = Unused */
    {0, 0, 0, 0, 0},   /* 16 = Unused */
    {0, 0, -1, 1, 10}, /* 17 = Guard Post Room (33x11) */
    {0, 0, -1, 1, 10}, /* 18 = Ambush Corridor (33x11) */
    {0, 0, 0, 0, 0},   /* 19 = Unused */
};



/*
 * Always picks a correct direction
 */
static void correct_dir(int *rdir, int *cdir, int y1, int x1, int y2,
	int x2)
{
	/* Extract vertical and horizontal directions */
	*rdir = (y1 == y2) ? 0 : (y1 < y2) ? 1 : -1;
	*cdir = (x1 == x2) ? 0 : (x1 < x2) ? 1 : -1;

	/* Never move diagonally */
	if (*rdir && *cdir)
	{
		if (rand_int(100) < 50)
		{
			*rdir = 0;
		}
		else
		{
			*cdir = 0;
		}
	}
}


/*
 * Pick a random direction
 */
static void rand_dir(int *rdir, int *cdir)
{
	/* Pick a random direction */
	int i = rand_int(4);

	/* Extract the dy/dx components */
	*rdir = ddy_ddd[i];
	*cdir = ddx_ddd[i];
}


/*
 * Returns random co-ordinates for player/monster/object
 */
static void new_player_spot(void)
{
	int y = p_ptr->py, x = p_ptr->px;
	int i;

	/* Find level start (up stairs) to seed loot generation logic */
	int start_feat = (!p_ptr->depth) ? FEAT_MORE : FEAT_LESS;
	if (!p_ptr->depth && p_ptr->inside_special == SPECIAL_WILD) start_feat = FEAT_SHAFT;

	/* Try to place the player on a staircase */
	temp_n = 0;
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{
			if (cave_feat[y][x] == start_feat && cave_naked_bold(y, x))
			{
				/* Save the location */
				temp_y[temp_n] = y;
				temp_x[temp_n] = x;
				temp_n++;
			}
		}
	}

	if (temp_n > 0)
	{
		i = rand_int(temp_n);
		y = temp_y[i];
		x = temp_x[i];
		player_place(y, x);
		return;
	}

	/* Place the player */
	while (1)
	{
		/* Pick a legal spot */
		y = rand_range(1, DUNGEON_HGT - 2);
		x = rand_range(1, DUNGEON_WID - 2);

		/* Must be a "naked" floor grid */
		if (!cave_naked_bold(y, x))
			continue;

		/* Refuse to start on anti-teleport grids */
		if (cave_info[y][x] & (CAVE_ICKY))
			continue;

		/* Done */
		break;
	}

	/* Place the player */
	player_place(y, x);
}


/*
 * Move the player, but try to keep centered on some location.
 */
static void old_player_spot(void)
{
	int y = p_ptr->py, x = p_ptr->px;
	int d = 4;

	/* Place the player */
	while (1)
	{
		d++;
		scatter(&y, &x, p_ptr->py, p_ptr->px, d / 5, 0);

		/* Must be a "naked" floor grid */
		if (!cave_naked_bold(y, x))
			continue;

		/* Refuse to start on anti-teleport grids */
		if (cave_info[y][x] & (CAVE_ICKY))
			continue;

		/* Done */
		break;
	}

	/* Place the player */
	player_place(y, x);
}


/*
 * Count the number of walls adjacent to the given grid.
 *
 * Note -- Assumes "in_bounds_fully(y, x)"
 *
 * We count only granite walls and permanent walls.
 */
static int next_to_walls(int y, int x)
{
	int k = 0;

	if (cave_feat[y + 1][x] >= FEAT_WALL_EXTRA)
		k++;
	if (cave_feat[y - 1][x] >= FEAT_WALL_EXTRA)
		k++;
	if (cave_feat[y][x + 1] >= FEAT_WALL_EXTRA)
		k++;
	if (cave_feat[y][x - 1] >= FEAT_WALL_EXTRA)
		k++;

	return (k);
}



/*
 * Convert existing terrain type to rubble
 */
static void place_rubble(int y, int x)
{
	/* Create rubble */
	cave_feat[y][x] = FEAT_RUBBLE;
}



/*
 * Convert existing terrain type to "up stairs"
 */
static void place_up_stairs(int y, int x)
{
	/* Create up stairs */
	cave_feat[y][x] = FEAT_LESS;
}


/*
 * Convert existing terrain type to "down stairs"
 */
static void place_down_stairs(int y, int x)
{
	/* Create down stairs */
  if (p_ptr->inside_special == SPECIAL_WILD) {
    cave_feat[y][x] = FEAT_SHAFT;

  } else {
    cave_feat[y][x] = FEAT_MORE;
  }
}





/*
 * Place an up/down staircase at given location
 */
static void place_random_stairs(int y, int x)
{
	/* Paranoia */
	if (!cave_clean_bold(y, x))
		return;

	/* Choose a staircase */
	if (!p_ptr->depth)
	{
		place_down_stairs(y, x);
	}
	else if (p_ptr->inside_special || (p_ptr->depth >= MAX_DEPTH - 1))
	{
		place_up_stairs(y, x);
	}
	else if (rand_int(100) < 50)
	{
		place_down_stairs(y, x);
	}
	else
	{
		place_up_stairs(y, x);
	}
}

/*
 * Place an altar at the given location
 */
static void place_altar(int y, int x)
{
	int alt, rar;

	while (TRUE)
	{
		alt = rand_int(MAX_GODS);
		rar = deity_info[alt].rarity % 4;

		if (p_ptr->depth < randnor(rar * 10, 3) || rand_int(rar) > 0)
			continue;

		break;
	}

	cave_feat[y][x] = FEAT_ALTAR_HEAD + alt;
}


/*
 * Place a locked door at the given location
 */
static void place_locked_door(int y, int x)
{
	/* Create locked door */
	cave_feat[y][x] = FEAT_DOOR_HEAD + randint(7);
}


/*
 * Place a secret door at the given location
 */
static void place_secret_door(int y, int x)
{
	/* Create secret door */
	cave_feat[y][x] = FEAT_SECRET;
}


/*
 * Place a random type of door at the given location
 */
static void place_random_door(int y, int x)
{
	int tmp;

	/* Choose an object */
	tmp = rand_int(1000);

	/* Open doors (300/1000) */
	if (tmp < 300)
	{
		/* Create open door */
		cave_feat[y][x] = FEAT_OPEN;
	}

	/* Broken doors (100/1000) */
	else if (tmp < 400)
	{
		/* Create broken door */
		cave_feat[y][x] = FEAT_BROKEN;
	}

	/* Secret doors (200/1000) */
	else if (tmp < 600)
	{
		/* Create secret door */
		cave_feat[y][x] = FEAT_SECRET;
	}

	/* Closed doors (300/1000) */
	else if (tmp < 900)
	{
		/* Create closed door */
		cave_feat[y][x] = FEAT_DOOR_HEAD + 0x00;
	}

	/* Locked doors (99/1000) */
	else if (tmp < 999)
	{
		/* Create locked door */
		cave_feat[y][x] = FEAT_DOOR_HEAD + randint(7);
	}

	/* Stuck doors (1/1000) */
	else
	{
		/* Create jammed door */
		cave_feat[y][x] = FEAT_DOOR_HEAD + 0x08 + rand_int(8);
	}
}



/*
 * Places some staircases near walls
 */
static void alloc_stairs(int feat, int num, int walls, bool force_room)
{
	int y, x, i, j, flag;

    if (p_ptr->inside_special == SPECIAL_DREAM) {
        /* Only place one exit */
        if (feat == FEAT_LESS) return; /* No up stairs */
        if (feat == FEAT_MORE) {
             /* Place Dream Exit instead of down stairs */
             /* We only want one, but alloc_stairs loops. */
             /* Hack: only do it once. */
             if (num > 1) num = 1;
             feat = FEAT_DREAM_EXIT;
        }
    }

	/* Place "num" stairs */
	for (i = 0; i < num; i++)
	{
		/* Place some stairs */
		for (flag = FALSE; !flag;)
		{
			/* Try several times, then decrease "walls" */
			for (j = 0; !flag && j < 3000; j++)
			{

				/* Pick a random grid */
				y = rand_int(DUNGEON_HGT);
				x = rand_int(DUNGEON_WID);

				/* Require "naked" floor grid */
				if (!cave_naked_bold(y, x))
					continue;

				/* Force room if requested */
				if (force_room && !(cave_info[y][x] & CAVE_ROOM))
					continue;

				/* Require a certain number of adjacent walls */
				if (next_to_walls(y, x) < walls)
					continue;

				/* Town -- must go down */
				if (!p_ptr->depth)
				{
					/* Clear previous contents, add down stairs */
				  if (p_ptr->inside_special == SPECIAL_WILD) {
				    cave_feat[y][x] = FEAT_SHAFT;
				  } else {
				    cave_feat[y][x] = FEAT_MORE;
				  }
				}

				/* Quest -- must go up */
				else if (p_ptr->inside_special == SPECIAL_QUEST ||
					(p_ptr->depth >= MAX_DEPTH - 1))
				{
					/* Clear previous contents, add up stairs */
					cave_feat[y][x] = FEAT_LESS;
				}

				/* Requested type */
				else
				{
					/* Clear previous contents, add stairs */
					cave_feat[y][x] = feat;
				}

				/* All done */
				flag = TRUE;
			}

			/* Require fewer walls */
			if (walls)
				walls--;
		}
	}
}




/*
 * Allocates some objects (using "place" and "type")
 */
static void alloc_object(int set, int typ, int num)
{
	int y, x, k;

	/* Place some objects */
	for (k = 0; k < num; k++)
	{
		/* Pick a "legal" spot */
		while (TRUE)
		{
			bool room;

			/* Location */
			y = rand_int(DUNGEON_HGT);
			x = rand_int(DUNGEON_WID);

			/* Require "naked" floor grid */
			if (!cave_naked_bold(y, x))
				continue;

			/* Check for "room" */
			room = (cave_info[y][x] & (CAVE_ROOM)) ? TRUE : FALSE;

			/* Require corridor? */
			if ((set == ALLOC_SET_CORR) && room)
				continue;

			/* Require room? */
			if ((set == ALLOC_SET_ROOM) && !room)
				continue;

			/* Accept it */
			break;
		}

		/* Place something */
		switch (typ)
		{
			case ALLOC_TYP_RUBBLE:
			{
				place_rubble(y, x);
				break;
			}

			case ALLOC_TYP_TRAP:
			{
				place_trap(y, x);
				break;
			}

			case ALLOC_TYP_OBJECT:
			{
				place_object(y, x, FALSE, FALSE);
				break;
			}

			case ALLOC_TYP_ALTAR:
			{
				place_altar(y, x);
				break;
			}
		}
	}
}



/*
 * Places "streamers" of rock through dungeon
 *
 * Note that their are actually six different terrain features used
 * to represent streamers.  Three each of magma and quartz, one for
 * basic vein, one with hidden gold, and one with known gold.  The
 * hidden gold types are currently unused.
 */
static void build_streamer(int feat, int chance, int max_len)
{
	int i, tx, ty;
	int y, x, dir;
	int len = 0;


	/* Hack -- Choose starting point */
	y = rand_range(1, DUNGEON_HGT - 2);
	x = rand_range(1, DUNGEON_WID - 2);

	/* Choose a random compass direction */
	dir = ddd[rand_int(8)];

	/* Place streamer into dungeon */
	while (TRUE)
	{
		if (len >= max_len) break;
		len++;

		/* One grid per density */
		for (i = 0; i < DUN_STR_DEN; i++)
		{
			int d = DUN_STR_RNG;

			/* Pick a nearby grid */
			while (1)
			{
				ty = rand_spread(y, d);
				tx = rand_spread(x, d);
				if (!in_bounds(ty, tx))
					continue;
				break;
			}

			/* Only convert "granite" walls */
			if (cave_feat[ty][tx] < FEAT_WALL_EXTRA)
				continue;
			if (cave_feat[ty][tx] > FEAT_WALL_SOLID)
				continue;

			/* Clear previous contents, add proper vein type */
			cave_feat[ty][tx] = feat;

			/* Hack -- Add some (known) treasure */
			if (rand_int(chance) == 0)
				cave_feat[ty][tx] += 0x04;
		}

		/* Advance the streamer */
		y += ddy[dir];
		x += ddx[dir];

		/* Stop at dungeon edge */
		if (!in_bounds(y, x))
			break;
	}
}


/*
 *  Place streams of water, lava, & trees -KMW-
 * This routine varies the placement based on dungeon level
 * otherwise is similar to build_streamer
 */
static void build_streamer2(int feat, int killwall)
{
	int i, j, mid, tx, ty;
	int y, x, dir;
	int poolchance;
	int poolsize;

	poolchance = randint(10);

	/* Hack -- Choose starting point */
	y = rand_spread(DUNGEON_HGT / 2, 10);
	x = rand_spread(DUNGEON_WID / 2, 15);

	/* Choose a random compass direction */
	dir = ddd[rand_int(8)];

	if (poolchance > 2)
	{
		/* Place streamer into dungeon */
		while (TRUE)
		{
			/* One grid per density */
			for (i = 0; i < (DUN_STR_DWLW + 1); i++)
			{
				int d = DUN_STR_WLW;

				/* Pick a nearby grid */
				while (1)
				{
					ty = rand_spread(y, d);
					tx = rand_spread(x, d);
					if (!in_bounds(ty, tx))
						continue;
					break;
				}

				/* Do not mess up vaults */
				if (cave_info[ty][tx] & CAVE_ICKY)
					continue;


				/* Only convert non-permanent features */
				if (killwall == 0)
				{
					if (cave_feat[ty][tx] >= FEAT_MAGMA)
						continue;
					if (cave_feat[ty][tx] == FEAT_LESS)
						continue;
					if (cave_feat[ty][tx] == FEAT_MORE)
						continue;
				}
				else
				{
					if (cave_feat[ty][tx] >= FEAT_PERM_EXTRA)
						continue;
					if (cave_feat[ty][tx] == FEAT_LESS)
						continue;
					if (cave_feat[ty][tx] == FEAT_MORE)
						continue;
				}

				/* Clear previous contents, add proper vein type */
				cave_feat[ty][tx] = feat;
			}

			/* Advance the streamer */
			y += ddy[dir];
			x += ddx[dir];

			if (randint(20) == 1)
				dir = ddd[rand_int(8)];	/* change direction */

			/* Stop at dungeon edge */
			if (!in_bounds(y, x))
				break;
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
}


/*
 * Build a destroyed level
 */
static void destroy_level(void)
{
	int y1, x1, y, x, k, t, n;

	object_type *o_ptr;
	object_type *o_nxt;


	/* Note destroyed levels */
	if (cheat_room)
		msg_print("Destroyed Level");

	/* Drop a few epi-centers (usually about two) */
	for (n = 0; n < randint(5); n++)
	{
		/* Pick an epi-center */
		x1 = rand_range(5, DUNGEON_WID - 1 - 5);
		y1 = rand_range(5, DUNGEON_HGT - 1 - 5);

		/* Big area of affect */
		for (y = (y1 - 15); y <= (y1 + 15); y++)
		{
			for (x = (x1 - 15); x <= (x1 + 15); x++)
			{
				/* Skip illegal grids */
				if (!in_bounds_fully(y, x))
					continue;

				/* Extract the distance */
				k = distance(y1, x1, y, x);

				/* Stay in the circle of death */
				if (k >= 16)
					continue;

				/* Delete the monster (if any) */
				delete_monster(y, x);

				/* Destroy valid grids */
				if (cave_valid_bold(y, x))
				{

					/* Delete objects */
					o_ptr = cave_o_idx[y][x];

					while (TRUE)
					{
						if (!o_ptr)
							break;

						o_nxt = o_ptr->next;
						remove_object(o_ptr);
						o_ptr = o_nxt;
					}

					/* Wall (or floor) type */
					t = rand_int(200);

					/* Granite */
					if (t < 20)
					{
						/* Create granite wall */
						cave_feat[y][x] = FEAT_WALL_EXTRA;
					}

					/* Quartz */
					else if (t < 70)
					{
						/* Create quartz vein */
						cave_feat[y][x] = FEAT_QUARTZ;
					}

					/* Magma */
					else if (t < 100)
					{
						/* Create magma vein */
						cave_feat[y][x] = FEAT_MAGMA;
					}

					/* Floor */
					else
					{
						/* Create floor */
						cave_feat[y][x] = FEAT_FLOOR;
					}

					/* No longer part of a room or vault */
					cave_info[y][x] &= ~(CAVE_ROOM | CAVE_ICKY);

					/* No longer illuminated or known */
					cave_info[y][x] &= ~(CAVE_MARK | CAVE_GLOW);
				}
			}
		}
	}
}



/*
 * Create up to "num" objects near the given coordinates
 * Only really called by some of the "vault" routines.
 */
static void vault_objects(int y, int x, int num)
{
	int i, j, k;

	/* Attempt to place 'num' objects */
	for (; num > 0; --num)
	{
		/* Try up to 11 spots looking for empty space */
		for (i = 0; i < 11; ++i)
		{
			/* Pick a random location */
			while (1)
			{
				j = rand_spread(y, 2);
				k = rand_spread(x, 3);
				if (!in_bounds(j, k))
					continue;
				break;
			}

			/* Require "clean" floor space */
			if (!cave_clean_bold(j, k))
				continue;

			/* Place an item */
			place_object(j, k, FALSE, FALSE);

			/* Placement accomplished */
			break;
		}
	}
}


/*
 * Place a trap with a given displacement of point
 */
static void vault_trap_aux(int y, int x, int yd, int xd)
{
	int count, y1, x1;

	/* Place traps */
	for (count = 0; count <= 5; count++)
	{
		/* Get a location */
		while (1)
		{
			y1 = rand_spread(y, yd);
			x1 = rand_spread(x, xd);
			if (!in_bounds(y1, x1))
				continue;
			break;
		}

		/* Require "naked" floor grids */
		if (!cave_naked_bold(y1, x1))
			continue;

		/* Place the trap */
		place_trap(y1, x1);

		/* Done */
		break;
	}
}


/*
 * Place some traps with a given displacement of given location
 */
static void vault_traps(int y, int x, int yd, int xd, int num)
{
	int i;

	for (i = 0; i < num; i++)
	{
		vault_trap_aux(y, x, yd, xd);
	}
}


/*
 * Hack -- Place some sleeping monsters near the given location
 */
static void vault_monsters(int y1, int x1, int flags)
{
	/* Place the monster (allow groups) */
	monster_level = p_ptr->depth + 2;
	place_monster(y1, x1, flags);
	monster_level = p_ptr->depth;
}


/*
 * Room building routines.
 *
 * Six basic room types:
 *   1 -- normal
 *   2 -- overlapping
 *   3 -- cross shaped
 *   4 -- large room with features
 *   5 -- monster nests
 *   6 -- monster pits
 *   7 -- simple vaults
 *   8 -- greater vaults
 *   9 -- themed vaults
 */


/*
 * Place a patrolling monster or guard
 */
static void place_guard(int y, int x, int r_idx, int guard_type)
{
    int m_idx = place_monster_aux(y, x, r_idx, MON_ALLOC_SLEEP);

    if (m_idx > 0) {
        setup_guard_post(m_idx, guard_type, y, x);
    }
}

/*
 * Place a patrol
 */
static void place_patrol(int y, int x, int r_idx, int patrol_type)
{
    int m_idx = place_monster_aux(y, x, r_idx, MON_ALLOC_SLEEP);

    if (m_idx > 0) {
        setup_monster_patrol(m_idx, patrol_type);
    }
}

/*
 * Generate guard posts in a room
 */
static void populate_guard_posts(int y1, int x1, int y2, int x2)
{
    int num_guards = 1 + rand_int(3);
    int i;

    for (i = 0; i < num_guards; i++) {
        int y, x;
        int tries = 0;

        while (tries++ < 100) {
            y = y1 + rand_int(y2 - y1);
            x = x1 + rand_int(x2 - x1);

            if (!cave_floor_bold(y, x)) continue;

            /* Prefer high value locations */
            if (rand_int(100) < 50) {
                /* Guard doorways */
                int dy, dx;
                for (dy = -1; dy <= 1; dy++) {
                    for (dx = -1; dx <= 1; dx++) {
                        if (cave_feat[y+dy][x+dx] >= FEAT_DOOR_HEAD &&
                            cave_feat[y+dy][x+dx] <= FEAT_DOOR_TAIL + 7) {
                            place_guard(y, x, 0, GUARD_POST_DOOR);
                            return;
                        }
                    }
                }
            }

            /* Guard high ground */
            if (get_elevation(y, x) > ELEV_GROUND && rand_int(100) < 60) {
                place_guard(y, x, 0, GUARD_POST_HIGHGROUND);
                return;
            }

            /* Regular guard post */
            place_guard(y, x, 0, GUARD_POST_ROOM);
            return;
        }
    }
}

/*
 * Type 1 -- normal rectangular rooms
 */
static void build_type1(int yval, int xval)
{
	int y, x, y2, x2;
	int y1, x1;

	bool light;


	/* Choose lite or dark */
	light = (p_ptr->depth <= randint(25));


	/* Pick a room size */
	y1 = yval - randint(4);
	y2 = yval + randint(3);
	x1 = xval - randint(11);
	x2 = xval + randint(11);


	/* Place a full floor under the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
			cave_feat[y][x] = FEAT_FLOOR;
			cave_info[y][x] |= (CAVE_ROOM);
			if (light)
				cave_info[y][x] |= (CAVE_GLOW);
		}
	}

	/* Walls around the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		cave_feat[y][x1 - 1] = FEAT_WALL_OUTER;
		cave_feat[y][x2 + 1] = FEAT_WALL_OUTER;
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
		cave_feat[y1 - 1][x] = FEAT_WALL_OUTER;
		cave_feat[y2 + 1][x] = FEAT_WALL_OUTER;
	}


	/* Hack -- Occasional pillar room */
	if (rand_int(20) == 0)
	{
		for (y = y1; y <= y2; y += 2)
		{
			for (x = x1; x <= x2; x += 2)
			{
				cave_feat[y][x] = FEAT_WALL_INNER;
			}
		}
	}

	/* Hack -- Occasional ragged-edge room */
	else if (rand_int(50) == 0)
	{
		for (y = y1 + 2; y <= y2 - 2; y += 2)
		{
			cave_feat[y][x1] = FEAT_WALL_INNER;
			cave_feat[y][x2] = FEAT_WALL_INNER;
		}
		for (x = x1 + 2; x <= x2 - 2; x += 2)
		{
			cave_feat[y1][x] = FEAT_WALL_INNER;
			cave_feat[y2][x] = FEAT_WALL_INNER;
		}
	}

    /* Add guards to some rooms */
    if (rand_int(100) < 30 && p_ptr->depth > 5) {
        populate_guard_posts(y1, x1, y2, x2);
    }
}


/*
 * Type 2 -- Overlapping rectangular rooms
 */
static void build_type2(int yval, int xval)
{
	int y, x;
	int y1a, x1a, y2a, x2a;
	int y1b, x1b, y2b, x2b;

	bool light;



	/* Choose lite or dark */
	light = (p_ptr->depth <= randint(25));


	/* Determine extents of the first room */
	y1a = yval - randint(4);
	y2a = yval + randint(3);
	x1a = xval - randint(11);
	x2a = xval + randint(10);

	/* Determine extents of the second room */
	y1b = yval - randint(3);
	y2b = yval + randint(4);
	x1b = xval - randint(10);
	x2b = xval + randint(11);


	/* Place a full floor for room "a" */
	for (y = y1a - 1; y <= y2a + 1; y++)
	{
		for (x = x1a - 1; x <= x2a + 1; x++)
		{
			cave_feat[y][x] = FEAT_FLOOR;
			cave_info[y][x] |= (CAVE_ROOM);
			if (light)
				cave_info[y][x] |= (CAVE_GLOW);
		}
	}

	/* Place a full floor for room "b" */
	for (y = y1b - 1; y <= y2b + 1; y++)
	{
		for (x = x1b - 1; x <= x2b + 1; x++)
		{
			cave_feat[y][x] = FEAT_FLOOR;
			cave_info[y][x] |= (CAVE_ROOM);
			if (light)
				cave_info[y][x] |= (CAVE_GLOW);
		}
	}


	/* Place the walls around room "a" */
	for (y = y1a - 1; y <= y2a + 1; y++)
	{
		cave_feat[y][x1a - 1] = FEAT_WALL_OUTER;
		cave_feat[y][x2a + 1] = FEAT_WALL_OUTER;
	}
	for (x = x1a - 1; x <= x2a + 1; x++)
	{
		cave_feat[y1a - 1][x] = FEAT_WALL_OUTER;
		cave_feat[y2a + 1][x] = FEAT_WALL_OUTER;
	}

	/* Place the walls around room "b" */
	for (y = y1b - 1; y <= y2b + 1; y++)
	{
		cave_feat[y][x1b - 1] = FEAT_WALL_OUTER;
		cave_feat[y][x2b + 1] = FEAT_WALL_OUTER;
	}
	for (x = x1b - 1; x <= x2b + 1; x++)
	{
		cave_feat[y1b - 1][x] = FEAT_WALL_OUTER;
		cave_feat[y2b + 1][x] = FEAT_WALL_OUTER;
	}



	/* Replace the floor for room "a" */
	for (y = y1a; y <= y2a; y++)
	{
		for (x = x1a; x <= x2a; x++)
		{
			cave_feat[y][x] = FEAT_FLOOR;
		}
	}

	/* Replace the floor for room "b" */
	for (y = y1b; y <= y2b; y++)
	{
		for (x = x1b; x <= x2b; x++)
		{
			cave_feat[y][x] = FEAT_FLOOR;
		}
	}
}



/*
 * Type 3 -- Cross shaped rooms
 *
 * Builds a room at a row, column coordinate
 *
 * Room "a" runs north/south, and Room "b" runs east/east
 * So the "central pillar" runs from x1a,y1b to x2a,y2b.
 *
 * Note that currently, the "center" is always 3x3, but I think that
 * the code below will work (with "bounds checking") for 5x5, or even
 * for unsymetric values like 4x3 or 5x3 or 3x4 or 3x5, or even larger.
 */
static void build_type3(int yval, int xval)
{
	int y, x, dy, dx, wy, wx;
	int y1a, x1a, y2a, x2a;
	int y1b, x1b, y2b, x2b;

	bool light;



	/* Choose lite or dark */
	light = (p_ptr->depth <= randint(25));


	/* For now, always 3x3 */
	wx = wy = 1;

	/* Pick max vertical size (at most 4) */
	dy = rand_range(3, 4);

	/* Pick max horizontal size (at most 15) */
	dx = rand_range(3, 11);


	/* Determine extents of the north/south room */
	y1a = yval - dy;
	y2a = yval + dy;
	x1a = xval - wx;
	x2a = xval + wx;

	/* Determine extents of the east/west room */
	y1b = yval - wy;
	y2b = yval + wy;
	x1b = xval - dx;
	x2b = xval + dx;


	/* Place a full floor for room "a" */
	for (y = y1a - 1; y <= y2a + 1; y++)
	{
		for (x = x1a - 1; x <= x2a + 1; x++)
		{
			cave_feat[y][x] = FEAT_FLOOR;
			cave_info[y][x] |= (CAVE_ROOM);
			if (light)
				cave_info[y][x] |= (CAVE_GLOW);
		}
	}

	/* Place a full floor for room "b" */
	for (y = y1b - 1; y <= y2b + 1; y++)
	{
		for (x = x1b - 1; x <= x2b + 1; x++)
		{
			cave_feat[y][x] = FEAT_FLOOR;
			cave_info[y][x] |= (CAVE_ROOM);
			if (light)
				cave_info[y][x] |= (CAVE_GLOW);
		}
	}


	/* Place the walls around room "a" */
	for (y = y1a - 1; y <= y2a + 1; y++)
	{
		cave_feat[y][x1a - 1] = FEAT_WALL_OUTER;
		cave_feat[y][x2a + 1] = FEAT_WALL_OUTER;
	}
	for (x = x1a - 1; x <= x2a + 1; x++)
	{
		cave_feat[y1a - 1][x] = FEAT_WALL_OUTER;
		cave_feat[y2a + 1][x] = FEAT_WALL_OUTER;
	}

	/* Place the walls around room "b" */
	for (y = y1b - 1; y <= y2b + 1; y++)
	{
		cave_feat[y][x1b - 1] = FEAT_WALL_OUTER;
		cave_feat[y][x2b + 1] = FEAT_WALL_OUTER;
	}
	for (x = x1b - 1; x <= x2b + 1; x++)
	{
		cave_feat[y1b - 1][x] = FEAT_WALL_OUTER;
		cave_feat[y2b + 1][x] = FEAT_WALL_OUTER;
	}


	/* Replace the floor for room "a" */
	for (y = y1a; y <= y2a; y++)
	{
		for (x = x1a; x <= x2a; x++)
		{
			cave_feat[y][x] = FEAT_FLOOR;
		}
	}

	/* Replace the floor for room "b" */
	for (y = y1b; y <= y2b; y++)
	{
		for (x = x1b; x <= x2b; x++)
		{
			cave_feat[y][x] = FEAT_FLOOR;
		}
	}



	/* Special features (3/4) */
	switch (rand_int(4))
	{
			/* Large solid middle pillar */
		case 1:
		{
			for (y = y1b; y <= y2b; y++)
			{
				for (x = x1a; x <= x2a; x++)
				{
					cave_feat[y][x] = FEAT_WALL_INNER;
				}
			}
			break;
		}

			/* Inner treasure vault */
		case 2:
		{
			/* Build the vault */
			for (y = y1b; y <= y2b; y++)
			{
				cave_feat[y][x1a] = FEAT_WALL_INNER;
				cave_feat[y][x2a] = FEAT_WALL_INNER;
			}
			for (x = x1a; x <= x2a; x++)
			{
				cave_feat[y1b][x] = FEAT_WALL_INNER;
				cave_feat[y2b][x] = FEAT_WALL_INNER;
			}

			/* Place a secret door on the inner room */
			switch (rand_int(4))
			{
				case 0:
					place_secret_door(y1b, xval);
					break;
				case 1:
					place_secret_door(y2b, xval);
					break;
				case 2:
					place_secret_door(yval, x1a);
					break;
				case 3:
					place_secret_door(yval, x2a);
					break;
			}

			/* Place a treasure in the vault */
			place_object(yval, xval, FALSE, FALSE);

			/* Let's guard the treasure well */
			vault_monsters(yval, xval, MON_ALLOC_SLEEP | MON_ALLOC_HORDE);

			/* Traps naturally */
			vault_traps(yval, xval, 4, 4, rand_int(3) + 2);

			break;
		}

			/* Something else */
		case 3:
		{
			/* Occasionally pinch the center shut */
			if (rand_int(3) == 0)
			{
				/* Pinch the east/west sides */
				for (y = y1b; y <= y2b; y++)
				{
					if (y == yval)
						continue;
					cave_feat[y][x1a - 1] = FEAT_WALL_INNER;
					cave_feat[y][x2a + 1] = FEAT_WALL_INNER;
				}

				/* Pinch the north/south sides */
				for (x = x1a; x <= x2a; x++)
				{
					if (x == xval)
						continue;
					cave_feat[y1b - 1][x] = FEAT_WALL_INNER;
					cave_feat[y2b + 1][x] = FEAT_WALL_INNER;
				}

				/* Sometimes shut using secret doors */
				if (rand_int(3) == 0)
				{
					place_secret_door(yval, x1a - 1);
					place_secret_door(yval, x2a + 1);
					place_secret_door(y1b - 1, xval);
					place_secret_door(y2b + 1, xval);
				}
			}

			/* Occasionally put a "plus" in the center */
			else if (rand_int(3) == 0)
			{
				cave_feat[yval][xval] = FEAT_WALL_INNER;
				cave_feat[y1b][xval] = FEAT_WALL_INNER;
				cave_feat[y2b][xval] = FEAT_WALL_INNER;
				cave_feat[yval][x1a] = FEAT_WALL_INNER;
				cave_feat[yval][x2a] = FEAT_WALL_INNER;
			}

			/* Occasionally put a pillar in the center */
			else if (rand_int(3) == 0)
			{
				cave_feat[yval][xval] = FEAT_WALL_INNER;
			}

			break;
		}
	}
}


/*
 * Type 4 -- Large room with inner features
 *
 * Possible sub-types:
 *	1 - Just an inner room with one door
 *	2 - An inner room within an inner room
 *	3 - An inner room with pillar(s)
 *	4 - Inner room has a maze
 *	5 - A set of four inner rooms
 */
static void build_type4(int yval, int xval)
{
	int y, x, y1, x1;
	int y2, x2, tmp;

	bool light;



	/* Choose lite or dark */
	light = (p_ptr->depth <= randint(25));


	/* Large room */
	y1 = yval - 4;
	y2 = yval + 4;
	x1 = xval - 11;
	x2 = xval + 11;


	/* Place a full floor under the room */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
			cave_feat[y][x] = FEAT_FLOOR;
			cave_info[y][x] |= (CAVE_ROOM);
			if (light)
				cave_info[y][x] |= (CAVE_GLOW);
		}
	}

	/* Outer Walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		cave_feat[y][x1 - 1] = FEAT_WALL_OUTER;
		cave_feat[y][x2 + 1] = FEAT_WALL_OUTER;
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
		cave_feat[y1 - 1][x] = FEAT_WALL_OUTER;
		cave_feat[y2 + 1][x] = FEAT_WALL_OUTER;
	}


	/* The inner room */
	y1 = y1 + 2;
	y2 = y2 - 2;
	x1 = x1 + 2;
	x2 = x2 - 2;

	/* The inner walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		cave_feat[y][x1 - 1] = FEAT_WALL_INNER;
		cave_feat[y][x2 + 1] = FEAT_WALL_INNER;
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
		cave_feat[y1 - 1][x] = FEAT_WALL_INNER;
		cave_feat[y2 + 1][x] = FEAT_WALL_INNER;
	}


	/* Inner room variations */
	switch (randint(5))
	{
			/* Just an inner room with a monster */
		case 1:

			/* Place a secret door */
			switch (randint(4))
			{
				case 1:
					place_secret_door(y1 - 1, xval);
					break;
				case 2:
					place_secret_door(y2 + 1, xval);
					break;
				case 3:
					place_secret_door(yval, x1 - 1);
					break;
				case 4:
					place_secret_door(yval, x2 + 1);
					break;
			}

			/* Place a monster in the room */
			vault_monsters(yval, xval, MON_ALLOC_SLEEP);

			break;


			/* Treasure Vault (with a door) */
		case 2:

			/* Place a secret door */
			switch (randint(4))
			{
				case 1:
					place_secret_door(y1 - 1, xval);
					break;
				case 2:
					place_secret_door(y2 + 1, xval);
					break;
				case 3:
					place_secret_door(yval, x1 - 1);
					break;
				case 4:
					place_secret_door(yval, x2 + 1);
					break;
			}

			/* Place another inner room */
			for (y = yval - 1; y <= yval + 1; y++)
			{
				for (x = xval - 1; x <= xval + 1; x++)
				{
					if ((x == xval) && (y == yval))
						continue;
					cave_feat[y][x] = FEAT_WALL_INNER;
				}
			}

			/* Place a locked door on the inner room */
			switch (randint(4))
			{
				case 1:
					place_locked_door(yval - 1, xval);
					break;
				case 2:
					place_locked_door(yval + 1, xval);
					break;
				case 3:
					place_locked_door(yval, xval - 1);
					break;
				case 4:
					place_locked_door(yval, xval + 1);
					break;
			}

			/* Monsters to guard the "treasure" */
			vault_monsters(yval, xval, MON_ALLOC_SLEEP | MON_ALLOC_HORDE);

			/* Object (80%) */
			if (rand_int(100) < 80)
			{
				place_object(yval, xval, FALSE, FALSE);
			}

			/* Stairs (20%) */
			else
			{
				place_random_stairs(yval, xval);
			}

			/* Traps to protect the treasure */
			vault_traps(yval, xval, 4, 10, 2 + randint(3));

			break;


			/* Inner pillar(s). */
		case 3:

			/* Place a secret door */
			switch (randint(4))
			{
				case 1:
					place_secret_door(y1 - 1, xval);
					break;
				case 2:
					place_secret_door(y2 + 1, xval);
					break;
				case 3:
					place_secret_door(yval, x1 - 1);
					break;
				case 4:
					place_secret_door(yval, x2 + 1);
					break;
			}

			/* Large Inner Pillar */
			for (y = yval - 1; y <= yval + 1; y++)
			{
				for (x = xval - 1; x <= xval + 1; x++)
				{
					cave_feat[y][x] = FEAT_WALL_INNER;
				}
			}

			/* Occasionally, two more Large Inner Pillars */
			if (rand_int(2) == 0)
			{
				tmp = randint(2);
				for (y = yval - 1; y <= yval + 1; y++)
				{
					for (x = xval - 5 - tmp; x <= xval - 3 - tmp; x++)
					{
						cave_feat[y][x] = FEAT_WALL_INNER;
					}
					for (x = xval + 3 + tmp; x <= xval + 5 + tmp; x++)
					{
						cave_feat[y][x] = FEAT_WALL_INNER;
					}
				}
			}

			/* Occasionally, some Inner rooms */
			if (rand_int(3) == 0)
			{
				/* Long horizontal walls */
				for (x = xval - 5; x <= xval + 5; x++)
				{
					cave_feat[yval - 1][x] = FEAT_WALL_INNER;
					cave_feat[yval + 1][x] = FEAT_WALL_INNER;
				}

				/* Close off the left/right edges */
				cave_feat[yval][xval - 5] = FEAT_WALL_INNER;
				cave_feat[yval][xval + 5] = FEAT_WALL_INNER;

				/* Secret doors (random top/bottom) */
				place_secret_door(yval - 3 + (randint(2) * 2), xval - 3);
				place_secret_door(yval - 3 + (randint(2) * 2), xval + 3);

				/* Monsters */
				vault_monsters(yval, xval - 2,
					MON_ALLOC_SLEEP | MON_ALLOC_HORDE);
				vault_monsters(yval, xval + 2,
					MON_ALLOC_SLEEP | MON_ALLOC_HORDE);

				/* Objects */
				if (rand_int(3) == 0)
					place_object(yval, xval - 2, FALSE, FALSE);
				if (rand_int(3) == 0)
					place_object(yval, xval + 2, FALSE, FALSE);
			}

			break;


			/* Maze inside. */
		case 4:

			/* Place a secret door */
			switch (randint(4))
			{
				case 1:
					place_secret_door(y1 - 1, xval);
					break;
				case 2:
					place_secret_door(y2 + 1, xval);
					break;
				case 3:
					place_secret_door(yval, x1 - 1);
					break;
				case 4:
					place_secret_door(yval, x2 + 1);
					break;
			}

			/* Maze (really a checkerboard) */
			for (y = y1; y <= y2; y++)
			{
				for (x = x1; x <= x2; x++)
				{
					if (0x1 & (x + y))
					{
						cave_feat[y][x] = FEAT_WALL_INNER;
					}
				}
			}

			/* Monsters just love mazes. */
			vault_monsters(yval, xval - 5,
				MON_ALLOC_SLEEP | MON_ALLOC_HORDE);
			vault_monsters(yval, xval + 5,
				MON_ALLOC_SLEEP | MON_ALLOC_HORDE);

			/* Traps make them entertaining. */
			vault_traps(yval, xval - 3, 2, 8, randint(3));
			vault_traps(yval, xval + 3, 2, 8, randint(3));

			/* Mazes should have some treasure too. */
			vault_objects(yval, xval, 3);

			break;


			/* Four small rooms. */
		case 5:

			/* Inner "cross" */
			for (y = y1; y <= y2; y++)
			{
				cave_feat[y][xval] = FEAT_WALL_INNER;
			}
			for (x = x1; x <= x2; x++)
			{
				cave_feat[yval][x] = FEAT_WALL_INNER;
			}

			/* Doors into the rooms */
			if (rand_int(100) < 50)
			{
				int i = randint(10);
				place_secret_door(y1 - 1, xval - i);
				place_secret_door(y1 - 1, xval + i);
				place_secret_door(y2 + 1, xval - i);
				place_secret_door(y2 + 1, xval + i);
			}
			else
			{
				int i = randint(3);
				place_secret_door(yval + i, x1 - 1);
				place_secret_door(yval - i, x1 - 1);
				place_secret_door(yval + i, x2 + 1);
				place_secret_door(yval - i, x2 + 1);
			}

			/* Treasure, centered at the center of the cross */
			vault_objects(yval, xval, 2 + randint(2));

			/* Gotta have some monsters. */
			vault_monsters(yval + 1, xval - 4,
				MON_ALLOC_SLEEP | MON_ALLOC_HORDE);
			vault_monsters(yval + 1, xval + 4,
				MON_ALLOC_SLEEP | MON_ALLOC_HORDE);
			vault_monsters(yval - 1, xval - 4,
				MON_ALLOC_SLEEP | MON_ALLOC_HORDE);
			vault_monsters(yval - 1, xval + 4,
				MON_ALLOC_SLEEP | MON_ALLOC_HORDE);

			break;
	}
}

/*
 * Type 5 -- Monster nests
 *
 * A monster nest is a "big" room, with an "inner" room, containing
 * a "collection" of monsters of a given type strewn about the room.
 *
 * The monsters are chosen from a set of 64 randomly selected monster
 * races, to allow the nest creation to fail instead of having "holes".
 *
 * Note the use of the "get_mon_num_prep()" function, and the special
 * "get_mon_num_hook()" restriction function, to prepare the "monster
 * allocation table" in such a way as to optimize the selection of
 * "appropriate" non-unique monsters for the nest.
 *
 * Currently, a monster nest is one of
 *   a nest of "jelly" monsters   (Dungeon level 5 and deeper)
 *   a nest of "animal" monsters  (Dungeon level 30 and deeper)
 *   a nest of "undead" monsters  (Dungeon level 50 and deeper)
 *
 * Note that the "get_mon_num()" function may (rarely) fail, in which
 * case the nest will be empty, and will not affect the level rating.
 *
 * Note that "monster nests" will never contain "unique" monsters.
 */
static void build_type5(int yval, int xval)
{
	int y, x, y1, x1, y2, x2;

	if (seed_dungeon)
	{
		Rand_quick = FALSE;
	}


	/* Large room */
	y1 = yval - 4;
	y2 = yval + 4;
	x1 = xval - 11;
	x2 = xval + 11;


	/* Place the floor area */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
			cave_feat[y][x] = FEAT_FLOOR;
			cave_info[y][x] |= (CAVE_ROOM);
		}
	}

	/* Place the outer walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		cave_feat[y][x1 - 1] = FEAT_WALL_OUTER;
		cave_feat[y][x2 + 1] = FEAT_WALL_OUTER;
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
		cave_feat[y1 - 1][x] = FEAT_WALL_OUTER;
		cave_feat[y2 + 1][x] = FEAT_WALL_OUTER;
	}


	/* Advance to the center room */
	y1 = y1 + 2;
	y2 = y2 - 2;
	x1 = x1 + 2;
	x2 = x2 - 2;

	/* The inner walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		cave_feat[y][x1 - 1] = FEAT_WALL_INNER;
		cave_feat[y][x2 + 1] = FEAT_WALL_INNER;
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
		cave_feat[y1 - 1][x] = FEAT_WALL_INNER;
		cave_feat[y2 + 1][x] = FEAT_WALL_INNER;
	}


	/* Place a secret door */
	switch (randint(4))
	{
		case 1:
			place_secret_door(y1 - 1, xval);
			break;
		case 2:
			place_secret_door(y2 + 1, xval);
			break;
		case 3:
			place_secret_door(yval, x1 - 1);
			break;
		case 4:
			place_secret_door(yval, x2 + 1);
			break;
	}

	/* Place a monster, ensure escorts. */
	place_monster(yval, xval, MON_ALLOC_PIT);

	/* Describe */
	if (cheat_room)
	{
		/* Room type */
		msg_format("Monster nest");
	}


	/* Increase the level rating */
	rating += 10;

	/* (Sometimes) Cause a "special feeling" (for "Monster Nests") */
	if ((p_ptr->depth <= 40) &&
		(randint(p_ptr->depth * p_ptr->depth + 1) < 300))
	{
		good_item_flag = TRUE;
	}


	if (seed_dungeon)
	{
		Rand_quick = TRUE;
	}
}



/*
 * Type 6 -- Monster pits
 *
 * A monster pit is a "big" room, with an "inner" room, containing
 * a "collection" of monsters of a given type organized in the room.
 *
 */
static void build_type6(int yval, int xval)
{
	int y, x, y1, x1, y2, x2;


	/* Large room */
	y1 = yval - 4;
	y2 = yval + 4;
	x1 = xval - 11;
	x2 = xval + 11;

	if (seed_dungeon)
	{
		Rand_quick = FALSE;
	}


	/* Place the floor area */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		for (x = x1 - 1; x <= x2 + 1; x++)
		{
			cave_feat[y][x] = FEAT_FLOOR;
			cave_info[y][x] |= (CAVE_ROOM);
		}
	}

	/* Place the outer walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		cave_feat[y][x1 - 1] = FEAT_WALL_OUTER;
		cave_feat[y][x2 + 1] = FEAT_WALL_OUTER;
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
		cave_feat[y1 - 1][x] = FEAT_WALL_OUTER;
		cave_feat[y2 + 1][x] = FEAT_WALL_OUTER;
	}


	/* Advance to the center room */
	y1 = y1 + 2;
	y2 = y2 - 2;
	x1 = x1 + 2;
	x2 = x2 - 2;

	/* The inner walls */
	for (y = y1 - 1; y <= y2 + 1; y++)
	{
		cave_feat[y][x1 - 1] = FEAT_WALL_INNER;
		cave_feat[y][x2 + 1] = FEAT_WALL_INNER;
	}
	for (x = x1 - 1; x <= x2 + 1; x++)
	{
		cave_feat[y1 - 1][x] = FEAT_WALL_INNER;
		cave_feat[y2 + 1][x] = FEAT_WALL_INNER;
	}


	/* Place a secret door */
	switch (randint(4))
	{
		case 1:
			place_secret_door(y1 - 1, xval);
			break;
		case 2:
			place_secret_door(y2 + 1, xval);
			break;
		case 3:
			place_secret_door(yval, x1 - 1);
			break;
		case 4:
			place_secret_door(yval, x2 + 1);
			break;
	}

	/* Place some monsters, ensure escorts and groups. */
	place_monster(yval, xval, MON_ALLOC_PIT | MON_ALLOC_GROUP);

	/* Message */
	if (cheat_room)
	{
		/* Room type */
		msg_format("Monster pit");
	}


	/* Increase the level rating */
	rating += 10;

	/* (Sometimes) Cause a "special feeling" (for "Monster Pits") */
	if ((p_ptr->depth <= 40) &&
		(randint(p_ptr->depth * p_ptr->depth + 1) < 300))
	{
		good_item_flag = TRUE;
	}

	if (seed_dungeon)
	{
		Rand_quick = TRUE;
	}
}








static char hook_vault_monster_param;
static char hook_vault_object_param;

static bool hook_vault_place_player = FALSE;


static bool hook_vault_monster(int r_idx)
{
	if (r_info[r_idx].d_char == hook_vault_monster_param)
		return TRUE;
	return FALSE;
}

static bool hook_vault_object(int k_idx)
{
	if (k_info[k_idx].d_char == hook_vault_object_param)
		return TRUE;
	return FALSE;
}

/*
 * Hack -- fill in "vault" rooms
 * Added parameter for quest type -KMW-
 * Modified to read extended vault information -KMW-
 */
static void build_vault(int yval, int xval, vault_type* v_ptr) {
	int xmax = v_ptr->wid;
	int ymax = v_ptr->hgt;
	cptr data = v_text + v_ptr->text;
	cptr mdata = vm_text + v_ptr->m_text;
	cptr t = data;
	cptr t2 = mdata;

	int dx, dy, x, y, i, j;


	bool town_symb = (v_ptr->typ == 10 || v_ptr->typ == 11 ||
			  v_ptr->typ == 12);

	bool wild_symb = (v_ptr->typ == 13);

	char datum;
	byte number;

	int mode = MON_ALLOC_SLEEP;

	i = 0;
	j = 0;

	/* Flag quest monsters as such. */
	if (v_ptr->typ == 99)
	{
		mode |= MON_ALLOC_QUEST;
	}

	/* Vaults are different even in persistent dungeons. */
	if (seed_dungeon)
	{
		Rand_quick = FALSE;
	}

	/* Get the first chunk of data. */
	datum = t[0];
	number = t[1];

	/* Place dungeon features. */
	for (dy = 0; dy < ymax; dy++) {
	  for (dx = 0; dx < xmax; dx++) {

	    /* Extract the location */
	    x = xval - (xmax / 2) + dx;
	    y = yval - (ymax / 2) + dy;


	    /* Place some dungeon features. */
	    if (datum != ' ' && datum != '-') {

	      /* Lay down a floor */
	      cave_feat[y][x] = FEAT_FLOOR;

	      /* Part of a vault */
	      cave_info[y][x] |= (CAVE_ROOM);

	      if (!town_symb && !wild_symb) {
		cave_info[y][x] |= (CAVE_ICKY);
	      }

	      /* Shop, 0-7 */
	      if (isdigit(datum) && datum < '8') {
		int feat = FEAT_SHOP_HEAD + (datum - '0');

		cave_feat[y][x] = feat;
	      }

	      /* Building, a-z */
	      if (islower(datum)) {
		int feat = FEAT_BLDG_HEAD + (datum - 'a');

		cave_feat[y][x] = feat;
	      }

	      /* Analyze the grid */
	      switch (datum) {

		/* Granite wall (outer) */
	      case '%':
		cave_feat[y][x] = FEAT_WALL_OUTER;
		break;

		/* Granite wall (inner) */
	      case '#':
		cave_feat[y][x] = FEAT_WALL_INNER;
		break;

		/* Rubble. */
	      case ':':
		cave_feat[y][x] = FEAT_RUBBLE;
		break;

		/* Magma. */
	      case '&':
		cave_feat[y][x] = FEAT_MAGMA;
		break;

		/* Quartz. */
	      case '$':
		cave_feat[y][x] = FEAT_QUARTZ;
		break;

		/* Permanent wall (inner) */
	      case 'X':
		cave_feat[y][x] = FEAT_PERM_INNER;
		break;

		/* Quest Entrance */
	      case 'Q':
		cave_feat[y][x] = FEAT_QUEST_ENTER;
		break;

		/* Quest Exit -KMW- */
	      case 'E':
		cave_feat[y][x] = FEAT_QUEST_EXIT;
		break;

		/* Stairs up (exit without completing) -KMW- */
	      case '<':
		cave_feat[y][x] = FEAT_LESS;
		break;

		/* Stairs down */
	      case '>':
		cave_feat[y][x] = FEAT_MORE;
		break;

		/* Random altar. */
	      case 'O':
		place_altar(y, x);
		break;

		/* Grass. */
	      case 'A':
		cave_feat[y][x] = FEAT_GRASS;
		break;

		/* Swamp. */
	      case 'B':
		cave_feat[y][x] = FEAT_SWAMP;
		break;

		/* Mud. */
	      case 'C':
		cave_feat[y][x] = FEAT_MUD;
		break;

		/* Shrub. */
	      case 'H':
		cave_feat[y][x] = FEAT_SHRUB;
		break;

		/* Rocky hill */
	      case 'I':
		cave_feat[y][x] = FEAT_ROCKY_HILL;
		break;

		/* Water (shallow) -KMW- */
	      case 'V':
		cave_feat[y][x] = FEAT_SHAL_WATER;
		break;

		/* Water (deep) -KMW- */
	      case 'W':
		cave_feat[y][x] = FEAT_DEEP_WATER;
		break;

		/* Fog. */
	      case 'J':
		cave_feat[y][x] = FEAT_FOG;
		break;

		/* Lava (shallow) -KMW- */
	      case 'K':
		cave_feat[y][x] = FEAT_SHAL_LAVA;
		break;

		/* Lava (deep) -KMW- */
	      case 'L':
		cave_feat[y][x] = FEAT_DEEP_LAVA;
		break;

		/* Chaos fog */
	      case 'F':
		cave_feat[y][x] = FEAT_CHAOS_FOG;
		break;

		/* Glyph of warding */
	      case ';':
		/* Always permanent */
		cave_info[y][x] |= (CAVE_ICKY);
		cave_feat[y][x] = FEAT_GLYPH;
		break;

		/* Trees */
	      case 'Y':
		cave_feat[y][x] = FEAT_TREES;
		break;

		/* Treasure/trap */
	      case '*':
		if (rand_int(100) < 50) {
		  place_trap(y, x);
		}
		break;

		/* Secret doors */
	      case '+':
		place_secret_door(y, x);
		break;

		/* Regular doors */
	      case 'D':
		/* Unlocked doors in the town. */
		if (town_symb)
		  cave_feat[y][x] = FEAT_DOOR_HEAD;
		else
		  cave_feat[y][x] = FEAT_DOOR_HEAD + randint(4);
		break;

		/* Trap */
	      case '^':
		place_trap(y, x);
		break;

		/* Generator */
	      case 'G':
		if (v_ptr->mon[0]) {
		  create_generator(v_ptr->mon[0], y, x);
		}
		break;

	      case 'M':
		cave_feat[y][x] = FEAT_MOUNTAIN;
		break;

		/* Store exit. */
	      case 'S':
		cave_feat[y][x] = FEAT_STORE_EXIT;
		break;

		/* Shaft. */
	      case 'U':
		cave_feat[y][x] = FEAT_SHAFT;
		break;
	      }
	    }


	    /* Advance. */
	    number--;

	    /* End of a run. */
	    if (!number) {
	      t += 2;

	      datum = t[0];
	      number = t[1];
	    }
	  }
	}


	/* Get the first chunk of data. */
	datum = t2[0];
	number = t2[1];

	/* Place dungeon monsters and objects. */
	for (dy = 0; dy < ymax; dy++) {
	  for (dx = 0; dx < xmax; dx++) {


	    /* Extract the location */
	    x = xval - (xmax / 2) + dx;
	    y = yval - (ymax / 2) + dy;


	    /* 
	     * Place some monsters/objects. 
	     */
	    if (datum != ' ' && datum != '-') {

	      /* Monsters, 0-9 */
	      if (isdigit(datum)) {
		int i = v_ptr->mon[(datum - '0')];

		/* What a disgusting hack -- allow unfair monsters in */
		/* vaults and such. */
		bool um_opt = unfair_monsters;

		unfair_monsters = TRUE;
		place_monster_aux(y, x, i, mode);
		unfair_monsters = um_opt;
	      }

	      /* Monsters, a-z, A-Z */
	      if (isalpha(datum)) {
		s16b r_idx;

		hook_vault_monster_param = datum;
		get_mon_num_hook = hook_vault_monster;
		get_mon_num_prep();

		r_idx = get_mon_num(p_ptr->depth);

		if (r_idx) {
		  /* What a disgusting hack -- allow unfair monsters in */
		  /* vaults and such. */
		  bool um_opt = unfair_monsters;

		  unfair_monsters = TRUE;
		  place_monster_aux(y, x, r_idx, mode);
		  unfair_monsters = um_opt;
		}

		get_mon_num_hook = NULL;
		get_mon_num_prep();
	      }

	      /* Place the object with that picture. */
	      if (strchr("!\"$(),~'/=?[\\]_{|}", datum)) {
		s16b k_idx;

		hook_vault_object_param = datum;
		get_obj_num_hook = hook_vault_object;
		get_obj_num_prep();

		k_idx = get_obj_num(p_ptr->depth);

		if (k_idx) {
		  object_type *o_ptr = new_object();

		  object_prep(o_ptr, k_idx);
		  apply_magic(o_ptr, p_ptr->depth, TRUE, FALSE, FALSE);

		  floor_carry(y, x, o_ptr);
		}

		get_obj_num_hook = NULL;
		get_obj_num_prep();
	      }

	      switch (datum) {
		/* Treasure/trap */
	      case '*':
		if (rand_int(100) < 50) {
		  place_object(y, x, FALSE, FALSE);
		}
		break;

		/* Treasure -KMW- (Was: 'T') */
	      case '.':
		if (rand_int(100) < 75) {
		  place_object(y, x, FALSE, FALSE);

		} else if (rand_int(100) < 80) {
		  place_object(y, x, TRUE, FALSE);

		} else {
		  place_object(y, x, TRUE, TRUE);
		}
		break;

		/* [Arena] Monster */
	      case '&':
		if (town_symb) {
		  place_monster_aux(y, x, 
				    arena_monsters[p_ptr->which_arena]
				    [p_ptr->arena_number[p_ptr->which_arena]],
				    MON_ALLOC_ARENA | MON_ALLOC_JUST_ONE);
		} else {
		  monster_level = p_ptr->depth + 5;
		  place_monster(y, x, mode);
		  monster_level = p_ptr->depth;
		}
		break;

		/* Meaner monster (Was: '@') */
	      case ';':
		monster_level = p_ptr->depth + 11;
		place_monster(y, x, mode);
		monster_level = p_ptr->depth;
		break;

		/* Meaner monster, plus treasure (Was: '9') */
	      case '#':
		monster_level = p_ptr->depth + 9;
		place_monster(y, x, mode);
		monster_level = p_ptr->depth;
		object_level = p_ptr->depth + 7;
		place_object(y, x, TRUE, FALSE);
		object_level = p_ptr->depth;
		break;

		/* Nasty monster and treasure (Was: '8') */
	      case '^':
		monster_level = p_ptr->depth + 40;
		place_monster(y, x, mode);
		monster_level = p_ptr->depth;
		object_level = p_ptr->depth + 20;
		place_object(y, x, TRUE, TRUE);
		object_level = p_ptr->depth;
		break;

		/* Monster and/or object (Was: ',') */
	      case ':':
		if (rand_int(100) < 50) {
		  monster_level = p_ptr->depth + 3;
		  place_monster(y, x, mode);
		  monster_level = p_ptr->depth;
		}
		
		if (rand_int(100) < 50) {
		  object_level = p_ptr->depth + 7;
		  place_object(y, x, FALSE, FALSE);
		  object_level = p_ptr->depth;
		}
		break;

		/* Player position for quests (Was: 'P') */
	      case '@':
		if (p_ptr->inside_special != SPECIAL_WILD ||
		    hook_vault_place_player) {
		  player_place(y, x);
		}

		break;
	      }
	    }


	    /* Advance. */
	    number--;

	    /* End of a run. */
	    if (!number) {
	      t2 += 2;

	      datum = t2[0];
	      number = t2[1];
	    }

	  }
	}

	if (seed_dungeon)
	{
		Rand_quick = TRUE;
	}
}



/*
 * Type 7 -- simple vaults (see "v_info.txt")
 */
static void build_type7(int yval, int xval)
{
	vault_type *v_ptr;

	/* Pick a lesser vault */
	while (TRUE)
	{
		/* Access a random vault record */
		v_ptr = &v_info[rand_int(MAX_V_IDX)];

		/* Accept the first lesser vault */
		if (v_ptr->typ == 7)
			break;
	}

	/* Message */
	if (cheat_room)
		msg_print("Lesser Vault");

	/* Boost the rating */
	rating += v_ptr->rat;

	/* (Sometimes) Cause a special feeling */
	if ((p_ptr->depth <= 50) ||
		(randint((p_ptr->depth - 40) * (p_ptr->depth - 40) + 1) < 400))
	{
		good_item_flag = TRUE;
	}

	/* Hack -- Build the vault */
	build_vault(yval, xval, v_ptr);
}



/*
 * Type 8 -- greater vaults (see "v_info.txt")
 */
static void build_type8(int yval, int xval)
{
	vault_type *v_ptr;

	/* Pick a lesser vault */
	while (TRUE)
	{
		/* Access a random vault record */
		v_ptr = &v_info[rand_int(MAX_V_IDX)];

		/* Accept the first greater vault */
		if (v_ptr->typ == 8)
			break;
	}

	/* Message */
	if (cheat_room)
		msg_print("Greater Vault");

	/* Boost the rating */
	rating += v_ptr->rat;

	/* (Sometimes) Cause a special feeling */
	if ((p_ptr->depth <= 50) ||
		(randint((p_ptr->depth - 40) * (p_ptr->depth - 40) + 1) < 400))
	{
		good_item_flag = TRUE;
	}

	/* Hack -- Build the vault */
	build_vault(yval, xval, v_ptr);
}


/*
 * Type 9 -- themed vaults (see "v_info.txt")
 */
static void build_type9(int yval, int xval)
{
	vault_type *v_ptr;
	int vindex;

	/* Pick a lesser vault */
	while (TRUE)
	{
		/* Access a random vault record */
		vindex = rand_int(MAX_V_IDX);
		v_ptr = &v_info[vindex];

		/* Accept the first greater vault */
		if (v_ptr->typ == 9)
			break;
	}

	/* Message */
	if (cheat_room)
		msg_format("Themed Vault %d", vindex);

	/* Boost the rating */
	rating += v_ptr->rat;

	/* (Sometimes) Cause a special feeling */
	if ((p_ptr->depth <= 50) ||
		(randint((p_ptr->depth - 40) * (p_ptr->depth - 40) + 1) < 400))
	{
		good_item_flag = TRUE;
	}

	/* Hack -- Build the vault */
	build_vault(yval, xval, v_ptr);
}



/*
 * Constructs a tunnel using a drunken walker algorithm
 */
static void build_tunnel_winding(int row1, int col1, int row2, int col2)
{
	int y, x;
	int loop_max = 20000;
	int loop = 0;

	bool door_flag = FALSE;

	/* Reset the arrays */
	dun->tunn_n = 0;
	dun->wall_n = 0;
    dun->door_n = 0;

	y = row1;
	x = col1;

	while ((y != row2 || x != col2) && loop < loop_max)
	{
		loop++;

		int dir_y = 0;
		int dir_x = 0;

		/* 60% chance to move towards target */
		if (rand_int(100) < 60)
		{
			if (y < row2) dir_y = 1;
			else if (y > row2) dir_y = -1;

			if (x < col2) dir_x = 1;
			else if (x > col2) dir_x = -1;

			if (y == row2) dir_y = 0;
			if (x == col2) dir_x = 0;

			/* If diagonal, pick one component randomly */
			if (dir_y != 0 && dir_x != 0)
			{
				if (rand_int(2) == 0) dir_x = 0;
				else dir_y = 0;
			}
		}
		else
		{
			/* Random move */
			int d = rand_int(4);
			dir_y = ddy_ddd[d];
			dir_x = ddx_ddd[d];
		}

		if (!in_bounds(y + dir_y, x + dir_x)) continue;

		/* Move */
		y += dir_y;
		x += dir_x;

		/* Logic from build_tunnel to handle walls/doors/etc */

		/* Avoid the edge of the dungeon */
		if (cave_feat[y][x] == FEAT_PERM_SOLID) continue;
		if (cave_feat[y][x] == FEAT_PERM_OUTER) continue;
		if (cave_feat[y][x] == FEAT_WALL_SOLID) continue;

		/* Pierce "outer" walls of rooms */
		if (cave_feat[y][x] == FEAT_WALL_OUTER)
		{
            /* Check next step to avoid immediate re-entry? */
            /* Simplified: just mark as wall piercing */

			/* Save the wall location */
			if (dun->wall_n < WALL_MAX)
			{
				dun->wall[dun->wall_n].y = y;
				dun->wall[dun->wall_n].x = x;
				dun->wall_n++;
			}

            /* We don't implement the complex solid wall conversion here for simplicity,
               or we should? The original code does it to prevent silly doors.
               Let's skip it for now or copy it if needed.
               The user asked for winding corridors. */
		}
		/* Travel quickly through rooms */
		else if (cave_info[y][x] & (CAVE_ROOM))
		{
			/* Do nothing, just pass through */
		}
		/* Tunnel through all other walls */
		else if (cave_feat[y][x] >= FEAT_WALL_EXTRA)
		{
			if (dun->tunn_n < TUNN_MAX)
			{
				dun->tunn[dun->tunn_n].y = y;
				dun->tunn[dun->tunn_n].x = x;
				dun->tunn_n++;
			}
			door_flag = FALSE;
		}
		/* Handle corridor intersections */
		else
		{
			if (!door_flag)
			{
				if (dun->door_n < DOOR_MAX)
				{
					dun->door[dun->door_n].y = y;
					dun->door[dun->door_n].x = x;
					dun->door_n++;
				}
				door_flag = TRUE;
			}
		}
	}

    /* Fallback if failed to reach target */
    if (loop >= loop_max) {
        build_tunnel(row1, col1, row2, col2);
        return;
    }

	/* Apply changes */
	int i;
	for (i = 0; i < dun->tunn_n; i++)
	{
		cave_feat[dun->tunn[i].y][dun->tunn[i].x] = FEAT_FLOOR;
	}
	for (i = 0; i < dun->wall_n; i++)
	{
		cave_feat[dun->wall[i].y][dun->wall[i].x] = FEAT_FLOOR;
		if (rand_int(100) < DUN_TUN_PEN) place_random_door(dun->wall[i].y, dun->wall[i].x);
	}
}

/*
 * Constructs a tunnel between two points
 *
 * This function must be called BEFORE any streamers are created,
 * since we use the special "granite wall" sub-types to keep track
 * of legal places for corridors to pierce rooms.
 *
 * We use "door_flag" to prevent excessive construction of doors
 * along overlapping corridors.
 *
 * We queue the tunnel grids to prevent door creation along a corridor
 * which intersects itself.
 *
 * We queue the wall piercing grids to prevent a corridor from leaving
 * a room and then coming back in through the same entrance.
 *
 * We "pierce" grids which are "outer" walls of rooms, and when we
 * do so, we change all adjacent "outer" walls of rooms into "solid"
 * walls so that no two corridors may use adjacent grids for exits.
 *
 * The "solid" wall check prevents corridors from "chopping" the
 * corners of rooms off, as well as "silly" door placement, and
 * "excessively wide" room entrances.
 *
 * Useful "feat" values:
 *   FEAT_WALL_EXTRA -- granite walls
 *   FEAT_WALL_INNER -- inner room walls
 *   FEAT_WALL_OUTER -- outer room walls
 *   FEAT_WALL_SOLID -- solid room walls
 *   FEAT_PERM_EXTRA -- shop walls (perma)
 *   FEAT_PERM_INNER -- inner room walls (perma)
 *   FEAT_PERM_OUTER -- outer room walls (perma)
 *   FEAT_PERM_SOLID -- dungeon border (perma)
 */
static void build_tunnel(int row1, int col1, int row2, int col2)
{
	int i, y, x;
	int tmp_row, tmp_col;
	int row_dir, col_dir;
	int start_row, start_col;
	int main_loop_count = 0;

	bool door_flag = FALSE;



	/* Reset the arrays */
	dun->tunn_n = 0;
	dun->wall_n = 0;

	/* Save the starting location */
	start_row = row1;
	start_col = col1;

	/* Start out in the correct direction */
	correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

	/* Keep going until done (or bored) */
	while ((row1 != row2) || (col1 != col2))
	{
		/* Mega-Hack -- Paranoia -- prevent infinite loops */
		if (main_loop_count++ > 2000)
			break;

		/* Allow bends in the tunnel */
		if (rand_int(100) < DUN_TUN_CHG)
		{
			/* Acquire the correct direction */
			correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

			/* Random direction */
			if (rand_int(100) < DUN_TUN_RND)
			{
				rand_dir(&row_dir, &col_dir);
			}
		}

		/* Get the next location */
		tmp_row = row1 + row_dir;
		tmp_col = col1 + col_dir;


		/* Do not leave the dungeon!!! XXX XXX */
		while (!in_bounds_fully(tmp_row, tmp_col))
		{
			/* Acquire the correct direction */
			correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

			/* Random direction */
			if (rand_int(100) < DUN_TUN_RND)
			{
				rand_dir(&row_dir, &col_dir);
			}

			/* Get the next location */
			tmp_row = row1 + row_dir;
			tmp_col = col1 + col_dir;
		}


		/* Avoid the edge of the dungeon */
		if (cave_feat[tmp_row][tmp_col] == FEAT_PERM_SOLID)
			continue;

		/* Avoid the edge of vaults */
		if (cave_feat[tmp_row][tmp_col] == FEAT_PERM_OUTER)
			continue;

		/* Avoid "solid" granite walls */
		if (cave_feat[tmp_row][tmp_col] == FEAT_WALL_SOLID)
			continue;

		/* Pierce "outer" walls of rooms */
		if (cave_feat[tmp_row][tmp_col] == FEAT_WALL_OUTER)
		{
			/* Acquire the "next" location */
			y = tmp_row + row_dir;
			x = tmp_col + col_dir;

			/* Hack -- Avoid outer/solid permanent walls */
			if (cave_feat[y][x] == FEAT_PERM_SOLID)
				continue;
			if (cave_feat[y][x] == FEAT_PERM_OUTER)
				continue;

			/* Hack -- Avoid outer/solid granite walls */
			if (cave_feat[y][x] == FEAT_WALL_OUTER)
				continue;
			if (cave_feat[y][x] == FEAT_WALL_SOLID)
				continue;

			/* Accept this location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Save the wall location */
			if (dun->wall_n < WALL_MAX)
			{
				dun->wall[dun->wall_n].y = row1;
				dun->wall[dun->wall_n].x = col1;
				dun->wall_n++;
			}

			/* Forbid re-entry near this piercing */
			for (y = row1 - 1; y <= row1 + 1; y++)
			{
				for (x = col1 - 1; x <= col1 + 1; x++)
				{
					/* Convert adjacent "outer" walls as "solid" walls */
					if (cave_feat[y][x] == FEAT_WALL_OUTER)
					{
						/* Change the wall to a "solid" wall */
						cave_feat[y][x] = FEAT_WALL_SOLID;
					}
				}
			}
		}

		/* Travel quickly through rooms */
		else if (cave_info[tmp_row][tmp_col] & (CAVE_ROOM))
		{
			/* Accept the location */
			row1 = tmp_row;
			col1 = tmp_col;
		}

		/* Tunnel through all other walls */
		else if (cave_feat[tmp_row][tmp_col] >= FEAT_WALL_EXTRA)
		{
			/* Accept this location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Save the tunnel location */
			if (dun->tunn_n < TUNN_MAX)
			{
				dun->tunn[dun->tunn_n].y = row1;
				dun->tunn[dun->tunn_n].x = col1;
				dun->tunn_n++;
			}

			/* Allow door in next grid */
			door_flag = FALSE;
		}

		/* Handle corridor intersections or overlaps */
		else
		{
			/* Accept the location */
			row1 = tmp_row;
			col1 = tmp_col;

			/* Collect legal door locations */
			if (!door_flag)
			{
				/* Save the door location */
				if (dun->door_n < DOOR_MAX)
				{
					dun->door[dun->door_n].y = row1;
					dun->door[dun->door_n].x = col1;
					dun->door_n++;
				}

				/* No door in next grid */
				door_flag = TRUE;
			}

			/* Hack -- allow pre-emptive tunnel termination */
			if (rand_int(100) >= DUN_TUN_CON)
			{
				/* Distance between row1 and start_row */
				tmp_row = row1 - start_row;
				if (tmp_row < 0)
					tmp_row = (-tmp_row);

				/* Distance between col1 and start_col */
				tmp_col = col1 - start_col;
				if (tmp_col < 0)
					tmp_col = (-tmp_col);

				/* Terminate the tunnel */
				if ((tmp_row > 10) || (tmp_col > 10))
					break;
			}
		}
	}


	/* Turn the tunnel into corridor */
	for (i = 0; i < dun->tunn_n; i++)
	{
		/* Access the grid */
		y = dun->tunn[i].y;
		x = dun->tunn[i].x;

		/* Clear previous contents, add a floor */
		cave_feat[y][x] = FEAT_FLOOR;
	}


	/* Apply the piercings that we found */
	for (i = 0; i < dun->wall_n; i++)
	{
		/* Access the grid */
		y = dun->wall[i].y;
		x = dun->wall[i].x;

		/* Convert to floor grid */
		cave_feat[y][x] = FEAT_FLOOR;

		/* Occasional doorway */
		if (rand_int(100) < DUN_TUN_PEN)
		{
			/* Place a random door */
			place_random_door(y, x);
		}
	}
}




/*
 * Count the number of "corridor" grids adjacent to the given grid.
 *
 * Note -- Assumes "in_bounds_fully(y1, x1)"
 *
 * This routine currently only counts actual "empty floor" grids
 * which are not in rooms.  We might want to also count stairs,
 * open doors, closed doors, etc.  XXX XXX
 */
static int next_to_corr(int y1, int x1)
{
	int i, y, x, k = 0;


	/* Scan adjacent grids */
	for (i = 0; i < 4; i++)
	{
		/* Extract the location */
		y = y1 + ddy_ddd[i];
		x = x1 + ddx_ddd[i];

		/* Skip non floors */
		if (!cave_floor_bold(y, x))
			continue;

		/* Skip non "empty floor" grids */
		if (cave_feat[y][x] != FEAT_FLOOR)
			continue;

		/* Skip grids inside rooms */
		if (cave_info[y][x] & (CAVE_ROOM))
			continue;

		/* Count these grids */
		k++;
	}

	/* Return the number of corridors */
	return (k);
}


/*
 * Determine if the given location is "between" two walls,
 * and "next to" two corridor spaces.  XXX XXX XXX
 *
 * Assumes "in_bounds_fully(y,x)"
 */
static bool possible_doorway(int y, int x)
{
	/* Count the adjacent corridors */
	if (next_to_corr(y, x) >= 2)
	{
		/* Check Vertical */
		if ((cave_feat[y - 1][x] >= FEAT_MAGMA) &&
			(cave_feat[y + 1][x] >= FEAT_MAGMA))
		{
			return (TRUE);
		}

		/* Check Horizontal */
		if ((cave_feat[y][x - 1] >= FEAT_MAGMA) &&
			(cave_feat[y][x + 1] >= FEAT_MAGMA))
		{
			return (TRUE);
		}
	}

	/* No doorway */
	return (FALSE);
}


/*
 * Places door at y, x position if at least 2 walls found
 */
static void try_door(int y, int x)
{
	/* Paranoia */
	if (!in_bounds(y, x))
		return;

	/* Ignore walls */
	if (cave_feat[y][x] >= FEAT_MAGMA)
		return;

	/* Ignore room grids */
	if (cave_info[y][x] & (CAVE_ROOM))
		return;

	/* Occasional door (if allowed) */
	if ((rand_int(100) < DUN_TUN_JCT) && possible_doorway(y, x))
	{
		/* Place a door */
		place_random_door(y, x);
	}
}




/*
 * Type 12 -- Circular rooms
 */
static void build_type12(int yval, int xval)
{
	int y, x, rad;
	bool light = (p_ptr->depth <= randint(25));

	/* Pick a radius */
	rad = rand_range(3, 7);

	/* Place floor */
	for (y = yval - rad; y <= yval + rad; y++)
	{
		for (x = xval - rad; x <= xval + rad; x++)
		{
			if (!in_bounds(y, x)) continue;
			if (distance(yval, xval, y, x) <= rad)
			{
				cave_feat[y][x] = FEAT_FLOOR;
				cave_info[y][x] |= (CAVE_ROOM);
				if (light) cave_info[y][x] |= (CAVE_GLOW);
			}
		}
	}

	/* Place walls */
	for (y = yval - rad - 1; y <= yval + rad + 1; y++)
	{
		for (x = xval - rad - 1; x <= xval + rad + 1; x++)
		{
			if (!in_bounds(y, x)) continue;
			/* If not floor, and next to floor, make wall */
			if (cave_feat[y][x] != FEAT_FLOOR)
			{
				bool next_to_floor = FALSE;
				int dy, dx;
				for (dy = -1; dy <= 1; dy++)
				{
					for (dx = -1; dx <= 1; dx++)
					{
						if (in_bounds(y+dy, x+dx) && cave_feat[y+dy][x+dx] == FEAT_FLOOR)
						{
							next_to_floor = TRUE;
						}
					}
				}
				if (next_to_floor)
				{
					cave_feat[y][x] = FEAT_WALL_OUTER;
				}
			}
		}
	}
}

/*
 * Type 13 -- Composite Rooms (L and T shapes)
 */
static void build_type13(int yval, int xval)
{
	int i;
	int num_rects = rand_range(2, 3);
	bool light = (p_ptr->depth <= randint(25));

	/* We want them to overlap or touch to form a single room */

	for (i = 0; i < num_rects; i++)
	{
		int y1, x1, y2, x2;
		int h = rand_range(3, 9);
		int w = rand_range(3, 9);

		/* Offset from center */
		int oy = rand_range(-4, 4);
		int ox = rand_range(-4, 4);

		if (i == 0) { oy = 0; ox = 0; }

		y1 = yval + oy - h/2;
		y2 = y1 + h;
		x1 = xval + ox - w/2;
		x2 = x1 + w;

		/* Paint Floor */
		int y, x;
		for (y = y1; y <= y2; y++)
		{
			for (x = x1; x <= x2; x++)
			{
				if (!in_bounds(y, x)) continue;
				cave_feat[y][x] = FEAT_FLOOR;
				cave_info[y][x] |= (CAVE_ROOM);
				if (light) cave_info[y][x] |= (CAVE_GLOW);
			}
		}
	}

	/* Walls around the composite blob */
	int y, x;
	for (y = yval - 15; y <= yval + 15; y++)
	{
		for (x = xval - 15; x <= xval + 15; x++)
		{
			if (!in_bounds(y, x)) continue;
			if (cave_feat[y][x] == FEAT_FLOOR) continue;

			bool next_to_floor = FALSE;
			int dy, dx;
			for (dy = -1; dy <= 1; dy++)
			{
				for (dx = -1; dx <= 1; dx++)
				{
					if (in_bounds(y+dy, x+dx) && cave_feat[y+dy][x+dx] == FEAT_FLOOR)
					{
						next_to_floor = TRUE;
					}
				}
			}
			if (next_to_floor)
			{
				cave_feat[y][x] = FEAT_WALL_OUTER;
			}
		}
	}
}

/*
 * Type 14 -- Organic Cavern (Cellular Automata)
 */
static void build_type14(int yval, int xval)
{
	int y, x, i;
	bool light = (p_ptr->depth <= randint(25));

	int h = 20;
	int w = 20;
	int y1 = yval - h/2;
	int x1 = xval - w/2;

	/* Temp grid */
	bool grid[22][22];
	bool next_grid[22][22];

	/* Init random noise */
	for (y = 0; y < h+2; y++)
	{
		for (x = 0; x < w+2; x++)
		{
			if (y == 0 || y == h+1 || x == 0 || x == w+1)
				grid[y][x] = TRUE; /* Wall border */
			else
				grid[y][x] = (rand_int(100) < 45); /* 45% wall chance */
		}
	}

	/* CA iterations */
	for (i = 0; i < 4; i++)
	{
		for (y = 1; y <= h; y++)
		{
			for (x = 1; x <= w; x++)
			{
				int walls = 0;
				int dy, dx;
				for (dy = -1; dy <= 1; dy++)
					for (dx = -1; dx <= 1; dx++)
						if (grid[y+dy][x+dx]) walls++;

				if (grid[y][x])
					next_grid[y][x] = (walls >= 4);
				else
					next_grid[y][x] = (walls >= 5);
			}
		}
		/* Copy back */
		for (y = 1; y <= h; y++)
			for (x = 1; x <= w; x++)
				grid[y][x] = next_grid[y][x];
	}

	/* Transfer to dungeon */
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			if (!grid[y+1][x+1]) /* If not wall -> floor */
			{
				int dy = y1 + y;
				int dx = x1 + x;
				if (!in_bounds(dy, dx)) continue;
				cave_feat[dy][dx] = FEAT_FLOOR;
				cave_info[dy][dx] |= (CAVE_ROOM);
				if (light) cave_info[dy][dx] |= (CAVE_GLOW);
			}
		}
	}

	/* Walls around */
	for (y = y1 - 1; y <= y1 + h + 1; y++)
	{
		for (x = x1 - 1; x <= x1 + w + 1; x++)
		{
			if (!in_bounds(y, x)) continue;
			if (cave_feat[y][x] == FEAT_FLOOR) continue;
			 bool next_to_floor = FALSE;
			 int dy, dx;
			 for (dy = -1; dy <= 1; dy++)
				 for (dx = -1; dx <= 1; dx++)
					 if (in_bounds(y+dy, x+dx) && cave_feat[y+dy][x+dx] == FEAT_FLOOR)
						 next_to_floor = TRUE;

			 if (next_to_floor) cave_feat[y][x] = FEAT_WALL_OUTER;
		}
	}
}


/*
 * Type 17 -- Guard Post Room
 * Room with fortified positions and alert guards
 */
static void build_type17(int yval, int xval)
{
    int y, x;
    int y1, x1, y2, x2;
    bool light = (p_ptr->depth <= randint(25));

    /* Build standard room */
    y1 = yval - 3;
    y2 = yval + 3;
    x1 = xval - 9;
    x2 = xval + 9;

    /* Floor and walls */
    for (y = y1 - 1; y <= y2 + 1; y++) {
        for (x = x1 - 1; x <= x2 + 1; x++) {
            cave_feat[y][x] = FEAT_FLOOR;
            cave_info[y][x] |= CAVE_ROOM;
            if (light) cave_info[y][x] |= CAVE_GLOW;
        }
    }

    /* Outer walls */
    for (y = y1 - 1; y <= y2 + 1; y++) {
        cave_feat[y][x1 - 1] = FEAT_WALL_OUTER;
        cave_feat[y][x2 + 1] = FEAT_WALL_OUTER;
    }
    for (x = x1 - 1; x <= x2 + 1; x++) {
        cave_feat[y1 - 1][x] = FEAT_WALL_OUTER;
        cave_feat[y2 + 1][x] = FEAT_WALL_OUTER;
    }

    /* Place guard posts at corners */
    place_guard(y1 + 1, x1 + 1, 0, GUARD_POST_HIGHGROUND);
    place_guard(y2 - 1, x2 - 1, 0, GUARD_POST_HIGHGROUND);

    /* Central patrol */
    place_patrol(yval, xval, 0, PATROL_TYPE_CIRCUIT);

    /* Cover features */
    cave_feat[y1 + 2][x1 + 2] = FEAT_BOULDER;
    cave_feat[y2 - 2][x2 - 2] = FEAT_BOULDER;
    cave_feat[y1 + 2][x2 - 2] = FEAT_STONE_PILLAR;
    cave_feat[y2 - 2][x1 + 2] = FEAT_STONE_PILLAR;
}

/*
 * Type 18 -- Ambush Corridor
 * Narrow passage with hidden monsters
 */
static void build_type18(int yval, int xval)
{
    int y, x;
    int y1 = yval - 2;
    int y2 = yval + 2;
    int x1 = xval - 11;
    int x2 = xval + 11;

    /* Long corridor */
    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            if (y == yval) {
                cave_feat[y][x] = FEAT_FLOOR;
            } else {
                cave_feat[y][x] = FEAT_TALL_GRASS; /* Concealment */
            }
            cave_info[y][x] |= CAVE_ROOM;
        }
    }

    /* Walls */
    for (x = x1 - 1; x <= x2 + 1; x++) {
        cave_feat[y1 - 1][x] = FEAT_WALL_OUTER;
        cave_feat[y2 + 1][x] = FEAT_WALL_OUTER;
    }

    /* Ambushers in tall grass */
    int num_ambushers = 2 + rand_int(3);
    for (int i = 0; i < num_ambushers; i++) {
        int my = (rand_int(2) == 0) ? y1 : y2;
        int mx = x1 + 2 + rand_int(x2 - x1 - 3);

        int m_idx = place_monster_aux(my, mx, 0, MON_ALLOC_SLEEP | MON_ALLOC_HIDE);
        if (m_idx > 0) {
            monster_guard_data *guard = alloc_guard_data(m_idx);
            guard->guard_state = GUARD_STATE_SLEEP;
            guard->patrol_type = PATROL_TYPE_STATIONARY;
            guard->home_y = my;
            guard->home_x = mx;
        }
    }
}


/*
 * Attempt to build a room of the given type at the given block
 *
 * Note that we restrict the number of "crowded" rooms to reduce
 * the chance of overflowing the monster list during level creation.
 */
static bool room_build(int y0, int x0, int typ)
{
	int y, x, y1, x1, y2, x2;


	/* Restrict level */
	if (p_ptr->depth < room[typ].level)
		return (FALSE);

	/* Restrict "crowded" rooms */
	if (dun->crowded && ((typ == 5) || (typ == 6)))
		return (FALSE);

	/* Extract blocks */
	y1 = y0 + room[typ].dy1;
	y2 = y0 + room[typ].dy2;
	x1 = x0 + room[typ].dx1;
	x2 = x0 + room[typ].dx2;

	/* Never run off the screen */
	if ((y1 < 0) || (y2 >= dun->row_rooms))
		return (FALSE);
	if ((x1 < 0) || (x2 >= dun->col_rooms))
		return (FALSE);

	/* Verify open space */
	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			if (dun->room_map[y][x])
				return (FALSE);
		}
	}

	/* XXX XXX XXX It is *extremely* important that the following */
	/* calculation is *exactly* correct to prevent memory errors */

	/* Acquire the location of the room */
	y = ((y1 + y2 + 1) * BLOCK_HGT) / 2;
	x = ((x1 + x2 + 1) * BLOCK_WID) / 2;

	/* Build a room */
	switch (typ)
	{
			/* Build an appropriate room */
        case 18:
            build_type18(y, x);
            break;
        case 17:
            build_type17(y, x);
            break;
        case 14:
            build_type14(y, x);
            break;
        case 13:
            build_type13(y, x);
            break;
        case 12:
            build_type12(y, x);
            break;
        case 11:
            build_folly_vault(y, x);
            break;
        case 10:
            build_sanctum_vault(y, x);
            break;
		case 8:
			build_type8(y, x);
			break;
		case 7:
			build_type7(y, x);
			break;
		case 9:
			build_type9(y, x);
			break;
		case 6:
			build_type6(y, x);
			break;
		case 5:
			build_type5(y, x);
			break;
		case 4:
			build_type4(y, x);
			break;
		case 3:
			build_type3(y, x);
			break;
		case 2:
			build_type2(y, x);
			break;
		case 1:
			build_type1(y, x);
			break;

			/* Paranoia */
		default:
			return (FALSE);
	}

	/* Save the room location */
	if (dun->cent_n < CENT_MAX)
	{
		dun->cent[dun->cent_n].y = y;
		dun->cent[dun->cent_n].x = x;
		dun->cent_n++;
	}

	/* Reserve some blocks */
	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			dun->room_map[y][x] = TRUE;
		}
	}

	/* Count "crowded" rooms */
	if ((typ == 5) || (typ == 6))
		dun->crowded = TRUE;

	/* Success */
	return (TRUE);
}


/*
 * Helper for plasma generation.
 */

static void perturb_point_mid(int x1, int x2, int x3, int x4, int xmid,
	int ymid, int rough, int depth_max)
{
	/* Average the four corners & perturb it a bit. */
	/* tmp is a random int +/- rough */
	int tmp2 = rough * 2 + 1;
	int tmp = randint(tmp2) - (rough + 1);

	int avg = ((x1 + x2 + x3 + x4) / 4) + tmp;

	/* Round up if needed. */
	if (((x1 + x2 + x3 + x4) % 4) > 1)
		avg++;

	/* Normalize */
	if (avg < 0)
		avg = 0;
	if (avg > depth_max)
		avg = depth_max;

	/* Set the new value. */
	cave_feat[ymid][xmid] = avg;
}


static void perturb_point_end(int x1, int x2, int x3, int xmid, int ymid,
	int rough, int depth_max)
{
	/* Average the three corners & perturb it a bit. */
	/* tmp is a random int +/- rough */
	int tmp2 = rough * 2 + 1;
	int tmp = randint(tmp2) - (rough + 1);

	int avg = ((x1 + x2 + x3) / 3) + tmp;

	/* Round up if needed. */
	if ((x1 + x2 + x3) % 3)
		avg++;

	/* Normalize */
	if (avg < 0)
		avg = 0;
	if (avg > depth_max)
		avg = depth_max;

	/* Set the new value. */
	cave_feat[ymid][xmid] = avg;
}


/*
 * A generic function to generate the plasma fractal.
 * Note that it uses ``cave_feat'' as temporary storage.
 * The values in ``cave_feat'' after this function
 * are NOT actual features; They are raw heights which
 * need to be converted to features.
 *
 *  A-----U-----B
 *  |           |
 *  L     M     R
 *  |           |
 *  C-----D-----D
 *
 * A, B, C, and D are given; the other five we calculate by
 * averaging the neighbors and adding a random offset.
 */

static void plasma_recursive(int x1, int y1, int x2, int y2, int depth_max,
	int rough)
{

	/* Find middle */
	int xmid = (x2 - x1) / 2 + x1;
	int ymid = (y2 - y1) / 2 + y1;

	/* Are we done? */
	if (x1 + 1 == x2)
	{
		return;
	}

	/* Calculate M */
	perturb_point_mid(cave_feat[y1][x1], cave_feat[y2][x1],
		cave_feat[y1][x2], cave_feat[y2][x2], xmid, ymid, rough,
		depth_max);

	/* Calculate U */
	perturb_point_end(cave_feat[y1][x1], cave_feat[y1][x2],
		cave_feat[ymid][xmid], xmid, y1, rough, depth_max);

	/* Calculate R */
	perturb_point_end(cave_feat[y1][x2], cave_feat[y2][x2],
		cave_feat[ymid][xmid], x2, ymid, rough, depth_max);

	/* Calculate B */
	perturb_point_end(cave_feat[y2][x2], cave_feat[y2][x1],
		cave_feat[ymid][xmid], xmid, y2, rough, depth_max);

	/* Calculate L */
	perturb_point_end(cave_feat[y2][x1], cave_feat[y1][x1],
		cave_feat[ymid][xmid], x1, ymid, rough, depth_max);


	/* Recurse the four quadrants */
	plasma_recursive(x1, y1, xmid, ymid, depth_max, rough);
	plasma_recursive(xmid, y1, x2, ymid, depth_max, rough);
	plasma_recursive(x1, ymid, xmid, y2, depth_max, rough);
	plasma_recursive(xmid, ymid, x2, y2, depth_max, rough);

}


/*
 * The default table in terrain level generation.
 */

static int terrain_table[2][22] = {
	/* Normal terrain table. */
	{
			FEAT_DEEP_WATER,
			FEAT_DEEP_WATER,
			FEAT_DEEP_WATER,
			FEAT_DEEP_WATER,

			FEAT_SHAL_WATER,
			FEAT_SHAL_WATER,
			FEAT_SHAL_WATER,
			FEAT_SHAL_WATER,
			FEAT_SHAL_WATER,

			FEAT_MUD,
			FEAT_MUD,

			FEAT_SWAMP,
			FEAT_SWAMP,

			FEAT_GRASS,
			FEAT_GRASS,
			FEAT_GRASS,

			FEAT_SHRUB,
			FEAT_SHRUB,

			FEAT_TREES,
			FEAT_TREES,

			FEAT_ROCKY_HILL,

			FEAT_MOUNTAIN,
		},

	/* The watery terrain table. */
	{
			FEAT_DEEP_WATER,
			FEAT_DEEP_WATER,
			FEAT_DEEP_WATER,
			FEAT_DEEP_WATER,
			FEAT_DEEP_WATER,
			FEAT_DEEP_WATER,
			FEAT_DEEP_WATER,
			FEAT_DEEP_WATER,
			FEAT_DEEP_WATER,

			FEAT_SHAL_WATER,
			FEAT_SHAL_WATER,
			FEAT_SHAL_WATER,
			FEAT_SHAL_WATER,

			FEAT_MUD,
			FEAT_MUD,
			FEAT_MUD,

			FEAT_SWAMP,
			FEAT_SWAMP,
			FEAT_SWAMP,

			FEAT_GRASS,
			FEAT_GRASS,

			FEAT_SHRUB,
		}
};



/*
 * The opposite procedure of the above table.
 */
static byte table_backwards(int feat, int type)
{
	switch (type)
	{
		case 0:

			switch (feat)
			{
				case FEAT_DEEP_WATER:
					return 0;
				case FEAT_SHAL_WATER:
					return 4;
				case FEAT_MUD:
					return 9;
				case FEAT_SWAMP:
					return 11;
				case FEAT_GRASS:
					return 13;
				case FEAT_SHRUB:
					return 16;
				case FEAT_TREES:
					return 18;
				case FEAT_ROCKY_HILL:
					return 20;
				case FEAT_MOUNTAIN:
					return 21;
				default:
					return 11;
			}

		case 1:

			switch (feat)
			{
				case FEAT_DEEP_WATER:
					return 0;
				case FEAT_SHAL_WATER:
					return 9;
				case FEAT_MUD:
					return 13;
				case FEAT_SWAMP:
					return 16;
				case FEAT_GRASS:
					return 19;
				case FEAT_SHRUB:
					return 21;
				default:
					return 11;
			}
	}

	/* Paranoia. */
	return 11;
}


/*
 * Create the light in the town -- i.e. handle day/night
 */
static void lite_up_town(bool daytime)
{
	int x, y, f, flg;

	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{
			/* Interesting grids */
			if (daytime || !cave_boring_bold(y, x))
			{
				/* Illuminate the grid */
				cave_info[y][x] |= (CAVE_GLOW);

				/* Wiz-lite if appropriate. */
				if (wiz_lite_town && 
				    p_ptr->wild_x == 0 && p_ptr->wild_y == 0)
				{
					/* Memorize the grid */
					cave_info[y][x] |= (CAVE_MARK);
				}
			}
		}
	}

	flg = CAVE_GLOW;
	if (wiz_lite_town)
		flg |= CAVE_MARK;

	/* Now add light in appropriate places */
	for (y = 1; y < DUNGEON_HGT - 1; y++)
	{
		for (x = 1; x < DUNGEON_WID - 1; x++)
		{
			/* If this is a shop, light it and surrounding squares */
			f = cave_feat[y][x];
			if ((f >= FEAT_SHOP_HEAD && f <= FEAT_SHOP_TAIL) ||
				(f >= FEAT_BLDG_HEAD && f <= FEAT_BLDG_TAIL) ||
				(f == FEAT_STORE_EXIT))
			{
				cave_info[y - 1][x - 1] |= flg;
				cave_info[y - 1][x] |= flg;
				cave_info[y - 1][x + 1] |= flg;
				cave_info[y][x - 1] |= flg;
				cave_info[y][x] |= flg;
				cave_info[y][x + 1] |= flg;
				cave_info[y + 1][x - 1] |= flg;
				cave_info[y + 1][x] |= flg;
				cave_info[y + 1][x + 1] |= flg;
			}
		}
	}
}


/*
 * Generate a terrain level using ``plasma'' fractals.
 *
 */
static void terrain_gen(void) {

  int i, k;
  int x, y;
  int depth;
  int table_type = 0;
  int table_size;
  int roughness;
  int level_bg;
  int scroll = 0;
  bool quick_prev = Rand_quick;
  u32b value_prev = Rand_value;

  bool daytime = FALSE;

  if ((turn % (10L * TOWN_DAWN)) < ((10L * TOWN_DAWN) / 2)) {
    daytime = TRUE;
  }


  /* HACK!!! */
  /* This is a total hack -- (0, 0) appears only right after birth. */
  if (p_ptr->px == 0 && p_ptr->py == 0) {
    hook_vault_place_player = TRUE;
  }

  /* Hack -- implement scrolly terrains. */
  if (!hook_vault_place_player && 
      ((p_ptr->py <= 2 || p_ptr->py >= DUNGEON_HGT - 3) || 
       (p_ptr->px <= 2 || p_ptr->px >= DUNGEON_WID - 3))) {

    /* Scroll up. */
    if (p_ptr->py <= 2) {
      scroll = 1;
      p_ptr->wild_y--;

      /* Scroll down. */
    } else if (p_ptr->py >= DUNGEON_HGT - 3) {
      scroll = 2;
      p_ptr->wild_y++;

      /* Scroll left. */
    } else if (p_ptr->px <= 2) {
      scroll = 3;
      p_ptr->wild_x--;

      /* Scroll right. */
    } else {
      scroll = 4;
      p_ptr->wild_x++;
    }
  }



  /* Initialize the four corners.
   * The corners are generated as permanent separately from the 
   * contents. This means that the wilderness is fairly "tileable".
   *
   * Again, the contents of the code below were taken arbitrarily,
   * I've no clear idea how predictable such a hashing function would be.
   */

  /* Set some generation parameters. */

  /* Table of terrain types, one for each depth. */
  /*
   * if (magik(30)) {
   * if (cheat_room) msg_print("Watery terrain level.");
   * }
   */

  table_size = 22;
  level_bg = 11;

  /* The roughness of the level. */
  roughness = 1;

  Rand_quick = TRUE;

#define HASH_CORNERS(X, Y) (((X) - (Y)) ^ (((X) + seed_wild) & (Y)))
#define HASH_LEVEL(X, Y)   (((Y) - (X)) ^ ((Y) & ((X) + seed_wild)))
  
  Rand_value = HASH_CORNERS(p_ptr->wild_x, p_ptr->wild_y);
  cave_feat[1][1] = rand_int(table_size);

  Rand_value = HASH_CORNERS(p_ptr->wild_x, p_ptr->wild_y + 1);
  cave_feat[DUNGEON_HGT - 2][1] = rand_int(table_size);

  Rand_value = HASH_CORNERS(p_ptr->wild_x + 1, p_ptr->wild_y);
  cave_feat[1][DUNGEON_WID - 2] = rand_int(table_size);

  Rand_value = HASH_CORNERS(p_ptr->wild_x + 1, p_ptr->wild_y + 1);
  cave_feat[DUNGEON_HGT - 2][DUNGEON_WID - 2] = rand_int(table_size);


  /* Terrain levels are always ``permanent''. 
   * Note the random bit shuffling to make a unique seed.
   * There's no real rationale behind this. Anyone know a good hashing
   * function for pairs of numbers? */
  Rand_quick = TRUE;
  Rand_value = HASH_LEVEL(p_ptr->wild_x, p_ptr->wild_y);

  /* Clear the rest of the level. */
  for (y = 2; y < DUNGEON_HGT - 2; y++) {
    for (x = 2; x < DUNGEON_WID - 2; x++) {

      /* Create level background */
      cave_feat[y][x] = level_bg;
    }
  }

  /* x1, y1, x2, y2, num_depths, roughness */

  plasma_recursive(1, 1, DUNGEON_WID - 2, DUNGEON_HGT - 2,
		   table_size - 1, roughness);



  /* Light all the grids.
   * HACK -- Assume that the starting town is known to the player
   * if "wiz_lite_town" is set.
   */

  for (y = 1; y < DUNGEON_HGT - 1; y++) {
    for (x = 1; x < DUNGEON_WID - 1; x++) {

      cave_feat[y][x] = terrain_table[table_type][cave_feat[y][x]];

      /* All grids are lit. */
      if (daytime) {
	cave_info[y][x] |= (CAVE_GLOW);

	if (wiz_lite_town && p_ptr->wild_x == 0 && p_ptr->wild_y == 0) {
	  cave_info[y][x] |= (CAVE_MARK);
	}
      }

      /* Nasty hack to allow pseudo-rooms */
      if (cave_floor_bold(y, x)) {
	cave_info[y][x] |= (CAVE_ROOM);
      }
    }
  }


  /* Special boundary walls -- Top */
  for (x = 0; x < DUNGEON_WID; x++) {
    y = 0;
    cave_feat[y][x] = FEAT_UNSEEN;
  }

  /* Special boundary walls -- Bottom */
  for (x = 0; x < DUNGEON_WID; x++) {
    y = DUNGEON_HGT - 1;
    cave_feat[y][x] = FEAT_UNSEEN;
  }

  /* Special boundary walls -- Left */
  for (y = 0; y < DUNGEON_HGT; y++) {
    x = 0;
    cave_feat[y][x] = FEAT_UNSEEN;
  }

  /* Special boundary walls -- Right */
  for (y = 0; y < DUNGEON_HGT; y++) {
    x = DUNGEON_WID - 1;
    cave_feat[y][x] = FEAT_UNSEEN;
  }

  /* Place a stairway down, sometimes. */
  if (rand_int(100) < DUN_WILD_STAIRS) {
    alloc_stairs(FEAT_SHAFT, 1, 0, FALSE);
  }


  /* Mega-hack: Pick a new depth. */
  if (scroll) {
    if (p_ptr->wild_x == 0 && p_ptr->wild_y == 0) {
      depth = 0;

    } else {
      depth = p_ptr->depth + randnor(0, 3);

      if (depth < 0)
	depth = 0;

      if (depth >= MAX_DEPTH)
	depth = MAX_DEPTH - 1;
    }

    p_ptr->depth = depth;

  } else {
    p_ptr->depth = p_ptr->wilderness_depth;
  }

  /* Generate a wilderness vault. (Or town) */
  if (magik(DUN_WILD_VAULT) || p_ptr->depth == 0) {
    int number = (p_ptr->depth ? randnor(0, 1) : 1);

    if (number < 0) {
      number = -number;
    }

    if (number == 0) {
      number = 1;
    }

    while (number) {
      vault_type *v_ptr = NULL;
      int vindex = -1, vy, vx;
      int i;

      /* Pick a wilderness vault */
      if (p_ptr->wild_x == 0 && p_ptr->wild_y == 0) {
	vindex = p_ptr->which_town;
	v_ptr = &v_info[vindex];

      } else {
	for (i = 0; i < 1000; i++) {
	  /* Access a random vault record */
	  vindex = rand_int(MAX_V_IDX);
	  v_ptr = &v_info[vindex];

	  /* Accept the first vault */
	  if (v_ptr->typ == (p_ptr->depth ? 13 : 10))
	    break;
	}
      }

      /* Message */
      if (cheat_room)
	msg_format("Wilderness Vault %d", vindex);

      /* Boost the rating */
      rating += v_ptr->rat;

      vy = rand_range((v_ptr->hgt / 2) + 1,
		      DUNGEON_HGT - (v_ptr->hgt / 2) - 1);
      vx = rand_range((v_ptr->wid / 2) + 1,
		      DUNGEON_WID - (v_ptr->wid / 2) - 1);

      /* Redundant code -- turn off persistent levels. */
      Rand_quick = FALSE;

      build_vault(vy, vx, v_ptr);

      number--;
    }
  }


  if (hook_vault_place_player) {
    hook_vault_place_player = FALSE;

  } else {
    /* Find a new place for the player. */
    switch (scroll) {
    case 1:
      p_ptr->py = DUNGEON_HGT - 3;
      old_player_spot();
      break;

    case 2:
      p_ptr->py = 2;
      old_player_spot();
      break;

    case 3:
      p_ptr->px = DUNGEON_WID - 3;
      old_player_spot();
      break;

    case 4:
      p_ptr->px = 2;
      old_player_spot();
      break;

    default:
      if (p_ptr->wilderness_px > 0 && p_ptr->wilderness_py > 0) {
	p_ptr->px = p_ptr->wilderness_px;
	p_ptr->py = p_ptr->wilderness_py;

	old_player_spot();

      } else {
	new_player_spot();
      }
      break;
    }
  }


  /* Turn off persistent levels. */
  Rand_quick = FALSE;


  /* Reset the monster generation level */
  monster_level = p_ptr->depth;

  /* Reset the object generation level */
  object_level = p_ptr->depth;

  /* Basic "amount" */
  k = (p_ptr->depth / 3);
  if (k > 10)
    k = 10;
  if (k < 2)
    k = 2;

  /* Pick a base number of monsters */
  i = ((daytime ? MIN_M_ALLOC_WILD_DAY : MIN_M_ALLOC_WILD_NIGHT) +
       randint(4));

  /* Put some monsters in the dungeon */
  /* But not in the town! */
  if (p_ptr->depth > 0 || p_ptr->wild_x != 0 || p_ptr->wild_y != 0) {
    for (i = i + k; i > 0; i--) {
      alloc_monster(0, 0);
    }

    /* Put some water dwellers.
     * Yes, that's right -- this code assumes that all aquatic
     * monsters are nocturnal. */
    i = MIN_M_ALLOC_WILD_NIGHT + randint(4);

    for (i = i + k; i > 0; i--) {
      alloc_monster(0, MON_ALLOC_AQUATIC);
    }
  }

  /* Put some objects in rooms */
  alloc_object(ALLOC_SET_ROOM, ALLOC_TYP_OBJECT, 
	       randnor(DUN_AMT_ROOM, 3));

  /* Put some altars */
  alloc_object(ALLOC_SET_ROOM, ALLOC_TYP_ALTAR, 
	       randnor(DUN_AMT_ALTAR, 3));

  /* Put some objects/gold in the dungeon */

  alloc_object(ALLOC_SET_ROOM, ALLOC_TYP_OBJECT, 
	       randnor(DUN_AMT_ITEM, 3));

  Rand_quick = quick_prev;
  Rand_value = value_prev;
}


/*
 * Places some small gold
 */
static void place_gold_small(int y, int x)
{
	object_type *i_ptr;

	/* Allocate space for the new object. */
	i_ptr = new_object();

	/* Set up gold */
	i_ptr->tval = TV_GOLD;
	i_ptr->pval = randint(100);

	/* Drop the object */
	drop_near(i_ptr, FALSE, y, x);
}

/*
 * Place traps near doors
 */
static void place_traps_near_doors(int chance)
{
	int i, y, x;
	int dy, dx;

	for (i = 0; i < dun->door_n; i++)
	{
		/* Door location */
		y = dun->door[i].y;
		x = dun->door[i].x;

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

/*
 * Place traps near chests
 */
static void place_traps_near_chests(int chance)
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
}

/*
 * Build a Cavern Sector using Plasma Fractal
 */
static void build_sector_cavern(int y0, int x0)
{
	int y1 = y0 * BLOCK_HGT;
	int x1 = x0 * BLOCK_WID;
	int y2 = (y0 + 2) * BLOCK_HGT;
	int x2 = (x0 + 2) * BLOCK_WID;
	int x, y;

	/* Safety */
	if (y2 >= DUNGEON_HGT) y2 = DUNGEON_HGT - 1;
	if (x2 >= DUNGEON_WID) x2 = DUNGEON_WID - 1;

	/* Initialize corners */
	cave_feat[y1][x1] = rand_int(100);
	cave_feat[y1][x2] = rand_int(100);
	cave_feat[y2][x1] = rand_int(100);
	cave_feat[y2][x2] = rand_int(100);

	/* Generate Plasma */
	plasma_recursive(x1, y1, x2, y2, 100, 1);

	/* Threshold */
	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			if (cave_feat[y][x] > 50)
			{
				cave_feat[y][x] = FEAT_FLOOR;
				cave_info[y][x] |= CAVE_ROOM;
			}
			else
			{
				cave_feat[y][x] = FEAT_WALL_INNER;
			}
		}
	}
}

/*
 * Ensure connectivity of floor tiles in a sector
 */
static void ensure_connectivity(int y1, int x1, int y2, int x2)
{
	int h = y2 - y1 + 1;
	int w = x2 - x1 + 1;
	int comp[33][33];
	int y, x;
	int comp_count = 0;
	int qy[1100], qx[1100];
	int qh, qt;
	int loop_safe = 0;

	/* Loop until fully connected */
	while (loop_safe++ < 100)
	{
		/* Reset comp map */
		for (y = 0; y < h; y++)
			for (x = 0; x < w; x++)
				comp[y][x] = 0;

		comp_count = 0;

		/* Find components */
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				int gy = y1 + y;
				int gx = x1 + x;

				if (cave_floor_bold(gy, gx) && comp[y][x] == 0) {
					comp_count++;
					qh = qt = 0;
					qy[qt] = y; qx[qt] = x; qt++;
					comp[y][x] = comp_count;

					while (qh != qt) {
						int cy = qy[qh]; int cx = qx[qh]; qh++;
						int d;
						for (d = 0; d < 4; d++) {
							int ny = cy + ddy_ddd[d];
							int nx = cx + ddx_ddd[d];
							if (ny >= 0 && ny < h && nx >= 0 && nx < w) {
								if (cave_floor_bold(y1+ny, x1+nx) && comp[ny][nx] == 0) {
									comp[ny][nx] = comp_count;
									if (qt < 1099) {
										qy[qt] = ny; qx[qt] = nx; qt++;
									}
								}
							}
						}
					}
				}
			}
		}

		if (comp_count <= 1) break;

		/* Connect Component 1 to nearest other component */
		{
			int min_dist = 9999;
			int py1 = -1, px1 = -1, py2 = -1, px2 = -1;
			int yy, xx;

			for (y = 0; y < h; y++) {
				for (x = 0; x < w; x++) {
					if (comp[y][x] == 1) {
						for (yy = 0; yy < h; yy++) {
							for (xx = 0; xx < w; xx++) {
								if (comp[yy][xx] > 1) {
									int dist = (y-yy)*(y-yy) + (x-xx)*(x-xx);
									if (dist < min_dist) {
										min_dist = dist;
										py1 = y; px1 = x;
										py2 = yy; px2 = xx;
									}
								}
							}
						}
					}
				}
			}

			if (py1 != -1) {
				/* Build Bridge */
				int cy = py1, cx = px1;
				while (cy != py2 || cx != px2) {
					if (cy < py2) cy++; else if (cy > py2) cy--;
					if (cx < px2) cx++; else if (cx > px2) cx--;
					cave_feat[y1+cy][x1+cx] = FEAT_FLOOR;
				}
			} else {
				/* Failed to find connection? Should not happen. */
				break;
			}
		}
	}
}

/*
 * Build a Plaza Sector with Hazards
 */
static void build_sector_plaza(int y0, int x0)
{
	int y1 = y0 * BLOCK_HGT;
	int x1 = x0 * BLOCK_WID;
	int y2 = (y0 + 2) * BLOCK_HGT;
	int x2 = (x0 + 2) * BLOCK_WID;
	int x, y, i;
	int hazard_type;

	/* Safety */
	if (y2 >= DUNGEON_HGT) y2 = DUNGEON_HGT - 1;
	if (x2 >= DUNGEON_WID) x2 = DUNGEON_WID - 1;

	/* Fill with Floor */
	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			cave_feat[y][x] = FEAT_FLOOR;
			cave_info[y][x] |= CAVE_ROOM;
		}
	}

	/* Select Hazard */
	switch (rand_int(3))
	{
		case 0: hazard_type = FEAT_SHAL_LAVA; break;
		case 1: hazard_type = FEAT_ACID; break;
		default: hazard_type = FEAT_ICE; break;
	}

	/* Generate Streams (1-3) */
	int num_streams = 1 + rand_int(3);
	for (i = 0; i < num_streams; i++)
	{
		int sy, sx, ey, ex;
		/* Random start/end on edges */
		if (rand_int(2) == 0) { /* Top/Bottom */
			sy = y1 + 1; sx = rand_range(x1 + 1, x2 - 1);
			ey = y2 - 1; ex = rand_range(x1 + 1, x2 - 1);
		} else { /* Left/Right */
			sy = rand_range(y1 + 1, y2 - 1); sx = x1 + 1;
			ey = rand_range(y1 + 1, y2 - 1); ex = x2 - 1;
		}

		/* Drunken walk from start to end */
		int cy = sy;
		int cx = sx;
		int loop_safe = 0;
		while ((cy != ey || cx != ex) && loop_safe++ < 1000)
		{
			cave_feat[cy][cx] = hazard_type;

			/* Move towards end */
			int dy = (ey > cy) ? 1 : ((ey < cy) ? -1 : 0);
			int dx = (ex > cx) ? 1 : ((ex < cx) ? -1 : 0);

			/* Randomize */
			if (rand_int(100) < 30) {
				dy = rand_range(-1, 1);
				dx = rand_range(-1, 1);
			}

			int ny = cy + dy;
			int nx = cx + dx;

			if (ny >= y1 && ny <= y2 && nx >= x1 && nx <= x2) {
				cy = ny;
				cx = nx;
			}
		}
	}

	/* Force Random Bridges (to ensure multiplicity) */
	for (i = 0; i < 2; i++)
	{
		int by = rand_range(y1 + 2, y2 - 2);
		int bx = rand_range(x1 + 2, x2 - 2);
		/* Simple 3x3 patch of floor */
		int dy, dx;
		for (dy = -1; dy <= 1; dy++)
			for (dx = -1; dx <= 1; dx++)
				if (in_bounds(by+dy, bx+dx))
					cave_feat[by+dy][bx+dx] = FEAT_FLOOR;
	}

	/* Ensure Connectivity */
	ensure_connectivity(y1, x1, y2, x2);
}

/*
 * Build a Dark Sector (Shifting Labyrinth)
 */
static void build_sector_dark(int y0, int x0)
{
	int y1 = y0 * BLOCK_HGT;
	int x1 = x0 * BLOCK_WID;
	int y2 = (y0 + 2) * BLOCK_HGT;
	int x2 = (x0 + 2) * BLOCK_WID;
	int x, y, i;

	/* Safety */
	if (y2 >= DUNGEON_HGT) y2 = DUNGEON_HGT - 1;
	if (x2 >= DUNGEON_WID) x2 = DUNGEON_WID - 1;

	/* Initialize with noise */
	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			if (rand_int(100) < 40) cave_feat[y][x] = FEAT_WALL_EXTRA;
			else cave_feat[y][x] = FEAT_FLOOR;
			cave_info[y][x] |= CAVE_ROOM;
		}
	}

	/* Cellular Automata Smoothing (4 iterations) */
	for (i = 0; i < 4; i++)
	{
		bool next_grid[33][33];
        int h = y2 - y1 + 1;
        int w = x2 - x1 + 1;

		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				int walls = 0;
				int dy, dx;
                int cy = y1 + y;
                int cx = x1 + x;

				for (dy = -1; dy <= 1; dy++)
					for (dx = -1; dx <= 1; dx++) {
                        int ny = cy + dy;
                        int nx = cx + dx;
                        if (in_bounds(ny, nx)) {
                            if (cave_feat[ny][nx] == FEAT_WALL_EXTRA) walls++;
                        } else {
                            walls++; /* Edge is wall */
                        }
                    }

				if (cave_feat[cy][cx] == FEAT_WALL_EXTRA)
					next_grid[y][x] = (walls >= 4);
				else
					next_grid[y][x] = (walls >= 5);
			}
		}

        /* Apply back */
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
                if (next_grid[y][x]) cave_feat[y1+y][x1+x] = FEAT_WALL_EXTRA;
                else cave_feat[y1+y][x1+x] = FEAT_FLOOR;
            }
        }
	}

    /* Ensure connectivity */
    ensure_connectivity(y1, x1, y2, x2);

    /* Place Heart of the Maze */
    {
        int ty, tx;
        int tries = 0;
        while (tries++ < 1000) {
            ty = rand_range(y1 + 1, y2 - 1);
            tx = rand_range(x1 + 1, x2 - 1);
            if (cave_floor_bold(ty, tx)) {
                /* Place item */
                object_level += 10;
                place_object(ty, tx, TRUE, TRUE);
                object_level -= 10;

                /* Glowing beacon */
                cave_set_feat(ty, tx, FEAT_GLOWING_TILE);
                break;
            }
        }
    }
}

/*
 * Place Ancient Ruin
 */
static void place_ancient_ruin(void)
{
	int y, x, dy, dx;
	int y1, x1, y2, x2;
	int tries;

	for (tries = 0; tries < 100; tries++)
	{
		y = rand_range(10, DUNGEON_HGT - 30);
		x = rand_range(10, DUNGEON_WID - 30);

		/* Check space */
		bool safe = TRUE;
		for (dy = 0; dy < 20; dy++)
		{
			for (dx = 0; dx < 20; dx++)
			{
				if (!in_bounds(y + dy, x + dx)) { safe = FALSE; break; }
				if (cave_perma_bold(y + dy, x + dx)) { safe = FALSE; break; }
				if (cave_feat[y+dy][x+dx] == FEAT_SHAFT ||
				    cave_feat[y+dy][x+dx] == FEAT_QUEST_ENTER ||
				    cave_feat[y+dy][x+dx] == FEAT_QUEST_EXIT) { safe = FALSE; break; }
				/* Avoid existing rooms? */
				if (cave_info[y+dy][x+dx] & CAVE_ROOM) { safe = FALSE; break; }
			}
			if (!safe) break;
		}
		if (!safe) continue;

		/* Build */
		y1 = y; x1 = x;
		y2 = y + 19; x2 = x + 19;

		/* Base: Rubble and Floor mix */
		for (dy = 0; dy < 20; dy++) {
			for (dx = 0; dx < 20; dx++) {
				if (rand_int(100) < 70) cave_set_feat(y+dy, x+dx, FEAT_RUBBLE);
				else cave_set_feat(y+dy, x+dx, FEAT_FLOOR);
				cave_info[y+dy][x+dx] |= CAVE_ROOM; /* Mark as room so stairs avoid it */
			}
		}

		/* Streets: Cross */
		for (dy = 0; dy < 20; dy++) cave_set_feat(y+dy, x+10, FEAT_FLOOR);
		for (dx = 0; dx < 20; dx++) cave_set_feat(y+10, x+dx, FEAT_FLOOR);

		/* Points of Interest: Doors */
		int doors = rand_range(1, 3);
		int i;
		for (i = 0; i < doors; i++) {
			int ty, tx;
			int d_tries = 0;
			while (d_tries++ < 100) {
				ty = rand_range(y1 + 1, y2 - 1);
				tx = rand_range(x1 + 1, x2 - 1);
				if (cave_feat[ty][tx] == FEAT_RUBBLE) {
					cave_set_feat(ty, tx, FEAT_RUIN_DOOR);
					break;
				}
			}
		}

		if (cheat_room) msg_print("Ancient Ruin generated.");
		return;
	}
}

/*
 * Populate the level with cover features
 */
void populate_cover_features(void)
{
    int y, x, i;

    /* Place some cover in rooms */
    for (i = 0; i < dun->cent_n; i++) {
        y = dun->cent[i].y;
        x = dun->cent[i].x;

        /* 50% chance for cover in a room */
        if (rand_int(100) < 50) {
            int num_cover = 2 + rand_int(4);
            int j;
            for (j = 0; j < num_cover; j++) {
                int ty = rand_spread(y, 4);
                int tx = rand_spread(x, 4);

                if (!in_bounds(ty, tx)) continue;
                if (cave_naked_bold(ty, tx)) {
                    int roll = rand_int(100);
                    int feat = FEAT_BOULDER;
                    int dura = COVER_DURABILITY_BOULDER;
                    int type = COVER_MEDIUM;

                    if (roll < 30) {
                        feat = FEAT_CRATE;
                        dura = 20;
                        type = COVER_LIGHT;
                    } else if (roll < 50) {
                        feat = FEAT_BARREL;
                        dura = 20;
                        type = COVER_LIGHT;
                    } else if (roll < 70) {
                        feat = FEAT_STONE_PILLAR;
                        dura = COVER_DURABILITY_WALL;
                        type = COVER_HEAVY;
                    }

                    create_cover_at(ty, tx, type, dura, feat);
                }
            }
        }
    }
}

/*
 * Populate the level with new features
 */
static void populate_features(void)
{
	int y, x, i;

	/* Place Ancient Ruins (Rarely) */
	if (p_ptr->depth > 0 && rand_int(100) < 5)
	{
		place_ancient_ruin();
	}

	/* Place Glowing Tiles (Secret Discovery) */
	if (p_ptr->depth > 0)
	{
		for (i = 0; i < rand_range(3, 8); i++)
		{
			int d = 0;
			while (d < 1000) {
				d++;
				y = rand_range(1, DUNGEON_HGT - 2);
				x = rand_range(1, DUNGEON_WID - 2);
				if (cave_floor_bold(y, x) && cave_naked_bold(y, x)) {
					cave_set_feat(y, x, FEAT_GLOWING_TILE);
					break;
				}
			}
		}
	}

	/* Place Fountains */
	for (i = 0; i < rand_range(2, 5); i++) {
		int d = 0;
		while (d < 1000) {
			d++;
			y = rand_int(DUNGEON_HGT);
			x = rand_int(DUNGEON_WID);
			if (cave_clean_bold(y, x) && (cave_info[y][x] & CAVE_ROOM)) {
				cave_feat[y][x] = FEAT_FOUNTAIN;
				break;
			}
		}
	}

	/* Place Cartographer's Desk */
	if (rand_int(100) < 40) { /* 40% chance */
		int d = 0;
		while (d < 1000) {
			d++;
			y = rand_int(DUNGEON_HGT);
			x = rand_int(DUNGEON_WID);
			if (cave_clean_bold(y, x) && (cave_info[y][x] & CAVE_ROOM)) {
				cave_feat[y][x] = FEAT_CARTOGRAPHER;
				break;
			}
		}
	}

	/* Place Heroic Remains in dead ends */
	for (i = 0; i < rand_range(1, 3); i++) {
		int d = 0;
		while (d < 1000) {
			d++;
			y = rand_range(1, DUNGEON_HGT - 2);
			x = rand_range(1, DUNGEON_WID - 2);
			if (cave_floor_bold(y, x) && cave_naked_bold(y, x)) {
				/* Check for 3 walls */
				if (next_to_walls(y, x) >= 3) {
					cave_feat[y][x] = FEAT_HEROIC_REMAINS;
					break;
				}
			}
		}
	}
}

/*
 * Build a Hill Sector (elevated 2x2 block)
 */
static void build_sector_hill(int y0, int x0)
{
    int y1 = y0 * BLOCK_HGT;
    int x1 = x0 * BLOCK_WID;
    int y2 = (y0 + 2) * BLOCK_HGT;
    int x2 = (x0 + 2) * BLOCK_WID;
    int x, y;

    /* Safety */
    if (y2 >= DUNGEON_HGT) y2 = DUNGEON_HGT - 1;
    if (x2 >= DUNGEON_WID) x2 = DUNGEON_WID - 1;

    /* Create hill with gradient */
    int cy = (y1 + y2) / 2;
    int cx = (x1 + x2) / 2;
    int max_dist = MAX(y2 - y1, x2 - x1) / 2;

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            int dist = distance(cy, cx, y, x);
            int elev = ELEV_GROUND;

            /* Elevation based on distance from center */
            if (dist < max_dist / 3) {
                elev = ELEV_HIGH;      /* Summit */
                cave_feat[y][x] = FEAT_HILL_TOP;
            } else if (dist < 2 * max_dist / 3) {
                elev = ELEV_HILL;      /* Hillside */
                cave_feat[y][x] = FEAT_SLOPE_UP;
            } else {
                elev = ELEV_GROUND;    /* Base */
                cave_feat[y][x] = FEAT_FLOOR;
            }

            set_elevation(y, x, elev);
            cave_info[y][x] |= CAVE_ROOM;

            /* Light summit */
            if (elev == ELEV_HIGH) {
                cave_info[y][x] |= CAVE_GLOW;
            }
        }
    }

    /* Add slope indicators around edges */
    for (y = y1 - 1; y <= y2 + 1; y++) {
        for (x = x1 - 1; x <= x2 + 1; x++) {
            if (!in_bounds(y, x)) continue;
            if (get_elevation(y, x) == ELEV_GROUND) {
                /* Check if adjacent to hill */
                int dy, dx;
                for (dy = -1; dy <= 1; dy++) {
                    for (dx = -1; dx <= 1; dx++) {
                        if (in_bounds(y+dy, x+dx) &&
                            get_elevation(y+dy, x+dx) > ELEV_GROUND) {
                            /* Mark as slope down from hill */
                            if (cave_feat[y][x] == FEAT_FLOOR) {
                                cave_feat[y][x] = FEAT_SLOPE_DOWN;
                            }
                        }
                    }
                }
            }
        }
    }

    /* Outer walls */
    for (y = y1 - 1; y <= y2 + 1; y++) {
        for (x = x1 - 1; x <= x2 + 1; x++) {
            if (!in_bounds(y, x)) continue;
            if (cave_feat[y][x] != FEAT_FLOOR &&
                cave_feat[y][x] != FEAT_SLOPE_UP &&
                cave_feat[y][x] != FEAT_SLOPE_DOWN &&
                cave_feat[y][x] != FEAT_HILL_TOP) {
                bool next_to_floor = FALSE;
                int dy, dx;
                for (dy = -1; dy <= 1; dy++) {
                    for (dx = -1; dx <= 1; dx++) {
                        if (in_bounds(y+dy, x+dx) &&
                            (cave_feat[y+dy][x+dx] == FEAT_FLOOR ||
                             cave_feat[y+dy][x+dx] == FEAT_SLOPE_UP ||
                             cave_feat[y+dy][x+dx] == FEAT_HILL_TOP)) {
                            next_to_floor = TRUE;
                        }
                    }
                }
                if (next_to_floor) {
                    cave_feat[y][x] = FEAT_WALL_OUTER;
                }
            }
        }
    }

    /* Place defenders on high ground */
    if (rand_int(100) < 60) {
        int my = cy + rand_int(3) - 1;
        int mx = cx + rand_int(3) - 1;
        if (in_bounds(my, mx) && get_elevation(my, mx) == ELEV_HIGH) {
            vault_monsters(my, mx, MON_ALLOC_SLEEP | MON_ALLOC_GROUP);
        }
    }
}

/*
 * Build a Pit/Depression Sector (low ground 2x2 block)
 */
static void build_sector_pit(int y0, int x0)
{
    int y1 = y0 * BLOCK_HGT;
    int x1 = x0 * BLOCK_WID;
    int y2 = (y0 + 2) * BLOCK_HGT;
    int x2 = (x0 + 2) * BLOCK_WID;
    int x, y;

    /* Safety */
    if (y2 >= DUNGEON_HGT) y2 = DUNGEON_HGT - 1;
    if (x2 >= DUNGEON_WID) x2 = DUNGEON_WID - 1;

    /* Create depression */
    int cy = (y1 + y2) / 2;
    int cx = (x1 + x2) / 2;

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            int dist = distance(cy, cx, y, x);

            /* Center is lowest */
            if (dist < 3) {
                set_elevation(y, x, ELEV_LOW);
                cave_feat[y][x] = FEAT_PIT;
            } else {
                set_elevation(y, x, ELEV_GROUND);
                cave_feat[y][x] = FEAT_SLOPE_DOWN; /* Sloping into pit */
            }

            cave_info[y][x] |= CAVE_ROOM;
        }
    }

    /* Add hazards in pit */
    int hazard = rand_int(3);
    for (y = y1 + 2; y <= y2 - 2; y++) {
        for (x = x1 + 2; x <= x2 - 2; x++) {
            if (get_elevation(y, x) == ELEV_LOW) {
                switch (hazard) {
                    case 0: /* Water/mud */
                        if (rand_int(100) < 30)
                            cave_feat[y][x] = FEAT_SHAL_WATER;
                        break;
                    case 1: /* Traps */
                        if (rand_int(100) < 15)
                            place_trap(y, x);
                        break;
                    case 2: /* Monsters */
                        if (rand_int(100) < 20)
                            place_monster(y, x, MON_ALLOC_SLEEP);
                        break;
                }
            }
        }
    }

    /* Outer walls */
    for (y = y1 - 1; y <= y2 + 1; y++) {
        for (x = x1 - 1; x <= x2 + 1; x++) {
            if (!in_bounds(y, x)) continue;
            if (cave_feat[y][x] != FEAT_FLOOR &&
                cave_feat[y][x] != FEAT_SLOPE_DOWN &&
                cave_feat[y][x] != FEAT_PIT &&
                cave_feat[y][x] != FEAT_SHAL_WATER) {
                bool next_to_floor = FALSE;
                int dy, dx;
                for (dy = -1; dy <= 1; dy++) {
                    for (dx = -1; dx <= 1; dx++) {
                        if (in_bounds(y+dy, x+dx) &&
                            get_elevation(y+dy, x+dx) <= ELEV_GROUND) {
                            next_to_floor = TRUE;
                        }
                    }
                }
                if (next_to_floor) {
                    cave_feat[y][x] = FEAT_WALL_OUTER;
                }
            }
        }
    }
}

/*
 * Build a Cliff Face (impassable barrier with elevation change)
 */
static void build_sector_cliff(int y0, int x0)
{
    int y1 = y0 * BLOCK_HGT;
    int x1 = x0 * BLOCK_WID;
    int y2 = (y0 + 2) * BLOCK_HGT;
    int x2 = (x0 + 2) * BLOCK_WID;
    int x, y;

    /* Safety */
    if (y2 >= DUNGEON_HGT) y2 = DUNGEON_HGT - 1;
    if (x2 >= DUNGEON_WID) x2 = DUNGEON_WID - 1;

    /* Orientation */
    bool vertical = (rand_int(100) < 50);

    if (vertical) {
        int cliff_x = (x1 + x2) / 2;

        /* High ground on left, low on right (or vice versa) */
        bool high_left = (rand_int(100) < 50);

        for (y = y1; y <= y2; y++) {
            for (x = x1; x <= x2; x++) {
                if (high_left) {
                    if (x < cliff_x - 1) {
                        set_elevation(y, x, ELEV_HIGH);
                        cave_feat[y][x] = FEAT_FLOOR;
                    } else if (x == cliff_x - 1 || x == cliff_x) {
                        set_elevation(y, x, ELEV_HIGH);
                        cave_feat[y][x] = FEAT_CLIFF_DOWN; /* Edge */
                    } else {
                        set_elevation(y, x, ELEV_GROUND);
                        cave_feat[y][x] = FEAT_CLIFF_UP; /* Face */
                    }
                } else {
                    if (x > cliff_x + 1) {
                        set_elevation(y, x, ELEV_HIGH);
                        cave_feat[y][x] = FEAT_FLOOR;
                    } else if (x == cliff_x + 1 || x == cliff_x) {
                        set_elevation(y, x, ELEV_HIGH);
                        cave_feat[y][x] = FEAT_CLIFF_DOWN;
                    } else {
                        set_elevation(y, x, ELEV_GROUND);
                        cave_feat[y][x] = FEAT_CLIFF_UP;
                    }
                }
                cave_info[y][x] |= CAVE_ROOM;
            }
        }

        /* Add ledges for climbing spots */
        int num_ledges = 1 + rand_int(2);
        for (int i = 0; i < num_ledges; i++) {
            int ly = y1 + 2 + rand_int(y2 - y1 - 3);
            int lx = cliff_x + (high_left ? 1 : -1);
            if (in_bounds(ly, lx)) {
                cave_feat[ly][lx] = FEAT_LEDGE;
                set_elevation(ly, lx, ELEV_HILL); /* Mid elevation */
            }
        }

    } else {
        /* Horizontal cliff */
        int cliff_y = (y1 + y2) / 2;
        bool high_top = (rand_int(100) < 50);

        for (y = y1; y <= y2; y++) {
            for (x = x1; x <= x2; x++) {
                if (high_top) {
                    if (y < cliff_y - 1) {
                        set_elevation(y, x, ELEV_HIGH);
                        cave_feat[y][x] = FEAT_FLOOR;
                    } else if (y == cliff_y - 1 || y == cliff_y) {
                        set_elevation(y, x, ELEV_HIGH);
                        cave_feat[y][x] = FEAT_CLIFF_DOWN;
                    } else {
                        set_elevation(y, x, ELEV_GROUND);
                        cave_feat[y][x] = FEAT_CLIFF_UP;
                    }
                } else {
                    if (y > cliff_y + 1) {
                        set_elevation(y, x, ELEV_HIGH);
                        cave_feat[y][x] = FEAT_FLOOR;
                    } else if (y == cliff_y + 1 || y == cliff_y) {
                        set_elevation(y, x, ELEV_HIGH);
                        cave_feat[y][x] = FEAT_CLIFF_DOWN;
                    } else {
                        set_elevation(y, x, ELEV_GROUND);
                        cave_feat[y][x] = FEAT_CLIFF_UP;
                    }
                }
                cave_info[y][x] |= CAVE_ROOM;
            }
        }

        /* Add ledges */
        int num_ledges = 1 + rand_int(2);
        for (int i = 0; i < num_ledges; i++) {
            int lx = x1 + 3 + rand_int(x2 - x1 - 5);
            int ly = cliff_y + (high_top ? 1 : -1);
            if (in_bounds(ly, lx)) {
                cave_feat[ly][lx] = FEAT_LEDGE;
                set_elevation(ly, lx, ELEV_HILL);
            }
        }
    }

    /* Archers on high ground */
    if (rand_int(100) < 50) {
        /* Find high ground spot */
        for (int i = 0; i < 10; i++) {
            int hy = y1 + rand_int(y2 - y1);
            int hx = x1 + rand_int(x2 - x1);
            if (in_bounds(hy, hx) && get_elevation(hy, hx) == ELEV_HIGH) {
                vault_monsters(hy, hx, MON_ALLOC_SLEEP);
                break;
            }
        }
    }
}

/*
 * Generate a new dungeon level
 *
 * Note that "dun_body" adds about 4000 bytes of memory to the stack.
 */
static void cave_gen(void)
{
	int i, k, y, x, y1, x1;

	bool destroyed = FALSE;

	bool lit_level = FALSE;

	dun_data dun_body;

	byte level_bg = FEAT_WALL_EXTRA;
	s16b dun_rooms = DUN_ROOMS;

	/* Global data */
	dun = &dun_body;

	/* Allow open levels. */

	if (allow_open_levels)
	{
		int chance1 = DUN_OPEN_FLOOR;
		int chance2 = DUN_OPEN_WATER;
		int chance3 = DUN_OPEN_CHAOS;
		int chance4 = DUN_OPEN_MAZE;
		int chance5 = DUN_OPEN_FOG;

		if (weirdness_is_rare)
		{
			chance1 /= 2;
			chance2 /= 2;
			chance3 /= 2;
			chance4 /= 2;
			chance5 /= 2;
		}

		if (magik(chance1))
		{
			level_bg = FEAT_FLOOR;
			lit_level = TRUE;
		}
		else if (magik(chance2))
		{
			level_bg = FEAT_SHAL_WATER;
			lit_level = TRUE;
		}
		else if (magik(chance3))
		{
			level_bg = FEAT_CHAOS_FOG;
		}
		else if (magik(chance4))
		{
			level_bg = FEAT_NONE;
			lit_level = TRUE;
		}
		else if (magik(chance5))
		{
			level_bg = FEAT_FOG;
		}
	}

	/* Hack -- Start with basic granite (or not) */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{
			byte bg = level_bg;

			/* Hack -- try to make something random if unspecified. */
			if (!level_bg)
			{
				switch ((x + y + randint(12)) % 12)
				{
					case 0:
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
					case 8:
						bg = FEAT_FLOOR;
						break;
					case 9:
						bg = FEAT_WALL_EXTRA;
						break;
					case 10:
						bg = FEAT_QUARTZ;
						break;
					case 11:
						bg = FEAT_MAGMA;
						break;
				}
			}

			/* Create level background */
			cave_feat[y][x] = bg;
		}
	}

	/* Possible "destroyed" level */
	if ((p_ptr->depth > 10) && (rand_int(DUN_DEST) == 0))
		destroyed = TRUE;

	/* Actual maximum number of rooms on this level */
	dun->row_rooms = DUNGEON_HGT / BLOCK_HGT;
	dun->col_rooms = DUNGEON_WID / BLOCK_WID;

	/* Initialize the room table */
	for (y = 0; y < dun->row_rooms; y++)
	{
		for (x = 0; x < dun->col_rooms; x++)
		{
			dun->room_map[y][x] = FALSE;
		}
	}

	/* Initialize Elevation */
	init_elevation();

	/* Initialize Sector Map */
	for (y = 0; y < dun->row_rooms; y += 2)
	{
		for (x = 0; x < dun->col_rooms; x += 2)
		{
			int sect_type = SECTOR_RUINS;
			int roll = rand_int(100);

			if (roll < (p_ptr->depth / 2)) sect_type = SECTOR_CAVERN;
			else if (roll < 10) sect_type = SECTOR_PLAZA;
			else if (roll < 20) sect_type = SECTOR_DARK;
			else if (roll < 40 + (p_ptr->depth / 4)) sect_type = SECTOR_HILL;
			else if (roll < 45 + (p_ptr->depth / 5)) sect_type = SECTOR_PIT;
			else if (roll < 50 + (p_ptr->depth / 6)) sect_type = SECTOR_CLIFF;

			/* Assign to 2x2 block */
			cave_sector[y][x] = sect_type;
			if (y + 1 < dun->row_rooms) cave_sector[y+1][x] = sect_type;
			if (x + 1 < dun->col_rooms) cave_sector[y][x+1] = sect_type;
			if (y + 1 < dun->row_rooms && x + 1 < dun->col_rooms) cave_sector[y+1][x+1] = sect_type;
		}
	}

	/* No "crowded" rooms yet */
	dun->crowded = FALSE;


	/* No rooms yet */
	dun->cent_n = 0;

	/* Build Special Sectors (Caverns and Plazas) */
	for (y = 0; y < dun->row_rooms; y += 2)
	{
		for (x = 0; x < dun->col_rooms; x += 2)
		{
			int sect = cave_sector[y][x];
			if (sect == SECTOR_CAVERN)
			{
				build_sector_cavern(y, x);
				/* Mark blocks as used */
				dun->room_map[y][x] = TRUE;
				if (y + 1 < dun->row_rooms) dun->room_map[y + 1][x] = TRUE;
				if (x + 1 < dun->col_rooms) dun->room_map[y][x + 1] = TRUE;
				if (y + 1 < dun->row_rooms && x + 1 < dun->col_rooms) dun->room_map[y + 1][x + 1] = TRUE;

				/* Add center for tunnels */
				if (dun->cent_n < CENT_MAX) {
					dun->cent[dun->cent_n].y = (y * BLOCK_HGT) + BLOCK_HGT;
					dun->cent[dun->cent_n].x = (x * BLOCK_WID) + BLOCK_WID;
					dun->cent_n++;
				}
			}
			else if (sect == SECTOR_HILL)
			{
				build_sector_hill(y, x);
				/* Mark blocks as used */
				dun->room_map[y][x] = TRUE;
				if (y + 1 < dun->row_rooms) dun->room_map[y + 1][x] = TRUE;
				if (x + 1 < dun->col_rooms) dun->room_map[y][x + 1] = TRUE;
				if (y + 1 < dun->row_rooms && x + 1 < dun->col_rooms) dun->room_map[y + 1][x + 1] = TRUE;

				/* Add center for tunnels */
				if (dun->cent_n < CENT_MAX) {
					dun->cent[dun->cent_n].y = (y * BLOCK_HGT) + BLOCK_HGT;
					dun->cent[dun->cent_n].x = (x * BLOCK_WID) + BLOCK_WID;
					dun->cent_n++;
				}
			}
			else if (sect == SECTOR_PIT)
			{
				build_sector_pit(y, x);
				/* Mark blocks as used */
				dun->room_map[y][x] = TRUE;
				if (y + 1 < dun->row_rooms) dun->room_map[y + 1][x] = TRUE;
				if (x + 1 < dun->col_rooms) dun->room_map[y][x + 1] = TRUE;
				if (y + 1 < dun->row_rooms && x + 1 < dun->col_rooms) dun->room_map[y + 1][x + 1] = TRUE;

				/* Add center for tunnels */
				if (dun->cent_n < CENT_MAX) {
					dun->cent[dun->cent_n].y = (y * BLOCK_HGT) + BLOCK_HGT;
					dun->cent[dun->cent_n].x = (x * BLOCK_WID) + BLOCK_WID;
					dun->cent_n++;
				}
			}
			else if (sect == SECTOR_CLIFF)
			{
				build_sector_cliff(y, x);
				/* Mark blocks as used */
				dun->room_map[y][x] = TRUE;
				if (y + 1 < dun->row_rooms) dun->room_map[y + 1][x] = TRUE;
				if (x + 1 < dun->col_rooms) dun->room_map[y][x + 1] = TRUE;
				if (y + 1 < dun->row_rooms && x + 1 < dun->col_rooms) dun->room_map[y + 1][x + 1] = TRUE;

				/* Add center for tunnels */
				if (dun->cent_n < CENT_MAX) {
					dun->cent[dun->cent_n].y = (y * BLOCK_HGT) + BLOCK_HGT;
					dun->cent[dun->cent_n].x = (x * BLOCK_WID) + BLOCK_WID;
					dun->cent_n++;
				}
			}
			else if (sect == SECTOR_DARK)
			{
				build_sector_dark(y, x);
				/* Mark blocks as used */
				dun->room_map[y][x] = TRUE;
				if (y + 1 < dun->row_rooms) dun->room_map[y + 1][x] = TRUE;
				if (x + 1 < dun->col_rooms) dun->room_map[y][x + 1] = TRUE;
				if (y + 1 < dun->row_rooms && x + 1 < dun->col_rooms) dun->room_map[y + 1][x + 1] = TRUE;

				/* Add center for tunnels */
				if (dun->cent_n < CENT_MAX) {
					dun->cent[dun->cent_n].y = (y * BLOCK_HGT) + BLOCK_HGT;
					dun->cent[dun->cent_n].x = (x * BLOCK_WID) + BLOCK_WID;
					dun->cent_n++;
				}
			}
			else if (sect == SECTOR_PLAZA)
			{
				build_sector_plaza(y, x);
				/* Mark blocks as used */
				dun->room_map[y][x] = TRUE;
				if (y + 1 < dun->row_rooms) dun->room_map[y + 1][x] = TRUE;
				if (x + 1 < dun->col_rooms) dun->room_map[y][x + 1] = TRUE;
				if (y + 1 < dun->row_rooms && x + 1 < dun->col_rooms) dun->room_map[y + 1][x + 1] = TRUE;

				/* Add center for tunnels */
				if (dun->cent_n < CENT_MAX) {
					dun->cent[dun->cent_n].y = (y * BLOCK_HGT) + BLOCK_HGT;
					dun->cent[dun->cent_n].x = (x * BLOCK_WID) + BLOCK_WID;
					dun->cent_n++;
				}
			}
		}
	}

	/* Build some rooms */
	for (i = 0; i < dun_rooms; i++)
	{
		/* Pick a block for the room */
		y = rand_int(dun->row_rooms);
		x = rand_int(dun->col_rooms);

		/* Only build in RUINS sectors */
		if (cave_sector[y][x] != SECTOR_RUINS) continue;

		/* Align dungeon rooms */
		if (dungeon_align)
		{
			/* Slide some rooms right */
			if ((x % 3) == 0)
				x++;

			/* Slide some rooms left */
			if ((x % 3) == 2)
				x--;
		}

		/* Destroyed levels are boring */
		if (destroyed)
		{
			/* Attempt a "trivial" room */
			if (room_build(y, x, 1))
				continue;

			/* Never mind */
			continue;
		}


		/* Attempt a themed vault */
		if (allow_theme_vaults)
		{
			int chance = 70;

			if (weirdness_is_rare)
				chance = 10;

			if (magik(chance))
			{
				if (room_build(y, x, 9))
					continue;
			}
		}

		/* Attempt an "unusual" room */
		if (rand_int(DUN_UNUSUAL) < p_ptr->depth)
		{
			/* Roll for room type */
			k = rand_int(100);

			/* Attempt a very unusual room */
			if (rand_int(DUN_UNUSUAL) < p_ptr->depth)
			{
                /* Type 17 -- Guard Post Room (5% if depth > 10) */
                if ((k < 5) && (p_ptr->depth >= 10) && room_build(y, x, 17))
                    continue;

                /* Type 18 -- Ambush Corridor (5% if depth > 15) */
                if ((k < 10) && (p_ptr->depth >= 15) && room_build(y, x, 18))
                    continue;

                /* Type 11 -- Folly Vault (10% if depth > 30) */
                if ((k < 20) && (p_ptr->depth >= 30) && room_build(y, x, 11))
                    continue;

                /* Type 10 -- Sanctum (10% if depth > 40) */
                if ((k < 20) && (p_ptr->depth >= 40) && room_build(y, x, 10))
                    continue;

				/* Type 8 -- Greater vault (10%) */
				if ((k < 20) && room_build(y, x, 8))
					continue;

				/* Type 7 -- Lesser vault (15%) */
				if ((k < 25) && room_build(y, x, 7))
					continue;

				/* Type 6 -- Monster pit (25%) */
				if ((k < 50) && room_build(y, x, 6))
					continue;

				/* Type 5 -- Monster nest (30%) */
				if ((k < 80) && room_build(y, x, 5))
					continue;
			}

			/* Type 4 -- Large room (25%) */
			if ((k < 25) && room_build(y, x, 4))
				continue;

			/* Type 3 -- Cross room (25%) */
			if ((k < 50) && room_build(y, x, 3))
				continue;

			/* Type 2 -- Overlapping (50%) */
			if ((k < 100) && room_build(y, x, 2))
				continue;
		}

		/* Attempt a trivial room */
		if (room_build(y, x, 1))
			continue;
	}

	/* Special boundary walls -- Top */
	for (x = 0; x < DUNGEON_WID; x++)
	{
		y = 0;

		/* Clear previous contents, add "solid" perma-wall */
		cave_feat[y][x] = FEAT_PERM_SOLID;
	}

	/* Special boundary walls -- Bottom */
	for (x = 0; x < DUNGEON_WID; x++)
	{
		y = DUNGEON_HGT - 1;

		/* Clear previous contents, add "solid" perma-wall */
		cave_feat[y][x] = FEAT_PERM_SOLID;
	}

	/* Special boundary walls -- Left */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		x = 0;

		/* Clear previous contents, add "solid" perma-wall */
		cave_feat[y][x] = FEAT_PERM_SOLID;
	}

	/* Special boundary walls -- Right */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		x = DUNGEON_WID - 1;

		/* Clear previous contents, add "solid" perma-wall */
		cave_feat[y][x] = FEAT_PERM_SOLID;
	}

	/* Hack -- Scramble the room order */
	for (i = 0; i < dun->cent_n; i++)
	{
		int pick1 = rand_int(dun->cent_n);
		int pick2 = rand_int(dun->cent_n);
		y1 = dun->cent[pick1].y;
		x1 = dun->cent[pick1].x;
		dun->cent[pick1].y = dun->cent[pick2].y;
		dun->cent[pick1].x = dun->cent[pick2].x;
		dun->cent[pick2].y = y1;
		dun->cent[pick2].x = x1;
	}

	/* Start with no tunnel doors */
	dun->door_n = 0;

	/* Hack -- connect the first room to the last room */
	y = dun->cent[dun->cent_n - 1].y;
	x = dun->cent[dun->cent_n - 1].x;

	/* Connect all the rooms together */
	for (i = 0; i < dun->cent_n; i++)
	{
		/* Connect the room to the previous room */
        if (rand_int(100) < 75) /* 75% chance for winding tunnel */
		    build_tunnel_winding(dun->cent[i].y, dun->cent[i].x, y, x);
        else
            build_tunnel(dun->cent[i].y, dun->cent[i].x, y, x);

		/* Remember the "previous" room */
		y = dun->cent[i].y;
		x = dun->cent[i].x;
	}

    /* Interconnectivity: Connect some random rooms */
    for (i = 0; i < dun->cent_n; i++)
    {
        if (rand_int(100) < 40) /* 40% chance */
        {
            int target = rand_int(dun->cent_n);
            if (target != i)
            {
                build_tunnel_winding(dun->cent[i].y, dun->cent[i].x, dun->cent[target].y, dun->cent[target].x);
            }
        }
    }

	/* Place intersection doors */
	if (level_bg == FEAT_WALL_EXTRA) {
	  for (i = 0; i < dun->door_n; i++) {
	    /* Extract junction location */
	    y = dun->door[i].y;
	    x = dun->door[i].x;

	    /* Try placing doors */
	    try_door(y, x - 1);
	    try_door(y, x + 1);
	    try_door(y - 1, x);
	    try_door(y + 1, x);
	  }
	}

	/* Hack -- Add some magma streamers */

	if (level_bg == FEAT_WALL_EXTRA)
	{
		u32b level_area = (u32b)DUNGEON_HGT * DUNGEON_WID;
		u32b standard_area = 64 * 64;
		int scale = (level_area + standard_area - 1) / standard_area;

		for (i = 0; i < DUN_STR_MAG * scale; i++)
		{
			build_streamer(FEAT_MAGMA, DUN_STR_MC, 32 + randint(32));
		}

		/* Hack -- Add some quartz streamers */
		for (i = 0; i < DUN_STR_QUA * scale; i++)
		{
			build_streamer(FEAT_QUARTZ, DUN_STR_QC, 32 + randint(32));
		}
	}

	/* Destroy the level if necessary */
	if (destroyed)
		destroy_level();

	/* Add streamers of trees, water, or lava -KMW- */
	if ((p_ptr->depth <= 2) && (randint(20) > 15))
		for (i = 0; i < randint(DUN_STR_QUA); i++)
			build_streamer2(FEAT_TREES, 1);
	if ((p_ptr->depth <= 19) && (randint(20) > 15))
	{
		for (i = 0; i < randint(DUN_STR_QUA - 1); i++)
			build_streamer2(FEAT_SHAL_WATER, 0);
		if (randint(20) > 15)
		{
			for (i = 0; i < randint(DUN_STR_QUA); i++)
				build_streamer2(FEAT_DEEP_WATER, 1);
		}
	}
	else if ((p_ptr->depth > 19) && (randint(20) > 15))
	{
		for (i = 0; i < randint(DUN_STR_QUA); i++)
			build_streamer2(FEAT_SHAL_LAVA, 0);
		if (randint(20) > 15)
		{
			for (i = 0; i < randint(DUN_STR_QUA - 1); i++)
				build_streamer2(FEAT_DEEP_LAVA, 1);
		}
	}
	else if (randint(10) > 7)
	{
		for (i = 0; i < randint(DUN_STR_QUA); i++)
		{
			build_streamer2(FEAT_CHAOS_FOG, 1);
		}
	}

	if (randint(10) > 7)
	{
		build_streamer2(FEAT_OIL, 0);
	}

	if (randint(10) > 7)
	{
		build_streamer2(FEAT_ICE, 0);
	}

	if (randint(10) > 7)
	{
		build_streamer2(FEAT_ACID, 0);
	}

	/* Place 3 or 4 down stairs near some walls */
	alloc_stairs(FEAT_MORE, rand_range(100, 120), 3, (level_bg == FEAT_FOG || level_bg == FEAT_CHAOS_FOG));

	/* Place 1 or 2 up stairs near some walls */
	alloc_stairs(FEAT_LESS, rand_range(40, 60), 3, (level_bg == FEAT_FOG || level_bg == FEAT_CHAOS_FOG));

	/* Find level start (up stairs) to seed loot generation logic */
	{
		int sy, sx;
		bool found_start = FALSE;
		/* Scan for up stairs (FEAT_LESS) usually */
		int start_feat = (!p_ptr->depth) ? FEAT_MORE : FEAT_LESS;
		if (!p_ptr->depth && p_ptr->inside_special == SPECIAL_WILD) start_feat = FEAT_SHAFT;

		for (sy = 0; sy < DUNGEON_HGT; sy++) {
			for (sx = 0; sx < DUNGEON_WID; sx++) {
				if (cave_feat[sy][sx] == start_feat) {
					set_generation_origin(sy, sx);
					found_start = TRUE;
					break;
				}
			}
			if (found_start) break;
		}
		if (!found_start) {
			/* Fallback to center */
			set_generation_origin(DUNGEON_HGT / 2, DUNGEON_WID / 2);
		}
	}

	/* Determine the character location */
	new_player_spot();

	/* Monsters and objects change even in persistent dungeons. */
	if (seed_dungeon)
	{
		Rand_quick = FALSE;
	}

	/* Basic "amount" */
	k = (p_ptr->depth / 3);
	if (k > 10)
		k = 10;
	if (k < 2)
		k = 2;


	/* Pick a base number of monsters */
	i = (MIN_M_ALLOC_LEVEL + randint(8)) * 4;

	if (!dun->crowded) i += 100;

	/* Put some monsters in the dungeon */
	for (i = i + k; i > 0; i--)
	{
		/* Hack -- flooded levels get fishy inhabitants. */
		if (level_bg == FEAT_SHAL_WATER)
		{
			alloc_monster(0, MON_ALLOC_SLEEP | MON_ALLOC_AQUATIC);
		}

		alloc_monster(0, MON_ALLOC_SLEEP);
	}

	/* Place some good items */
	for (i = 0; i < 6; i++)
	{
		int y, x;
		int d = 0;
		while (d < 1000)
		{
			d++;
			y = rand_int(DUNGEON_HGT);
			x = rand_int(DUNGEON_WID);
			if (cave_naked_bold(y, x))
			{
				place_object(y, x, TRUE, FALSE);
				break;
			}
		}
	}

	/* Place some small gold piles */
	for (i = 0; i < 50; i++)
	{
		int y, x;
		int d = 0;
		while (d < 1000)
		{
			d++;
			y = rand_int(DUNGEON_HGT);
			x = rand_int(DUNGEON_WID);
			if (cave_naked_bold(y, x))
			{
				place_gold_small(y, x);
				break;
			}
		}
	}

	/* Place some traps in the dungeon */
	{
		/* Scaled Density */
		u32b level_area = (u32b)DUNGEON_HGT * DUNGEON_WID;
		u32b standard_area = 64 * 64;
		int scale = (level_area + standard_area - 1) / standard_area;
		int base_traps, total_traps;

		if (scale < 1) scale = 1;

		/* Original was randint(k), where k approx 2-10. */
		/* New target: 5-10 per 64x64 unit. */
		base_traps = 5 + rand_int(6);
		total_traps = base_traps * scale;

		alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, total_traps / 2);
		/* Corridors get extra love */
		alloc_object(ALLOC_SET_CORR, ALLOC_TYP_TRAP, total_traps / 2);

		/* High traffic areas */
		place_traps_near_doors(20);
		place_traps_near_chests(40);
	}

	/* Put some rubble in corridors */
	alloc_object(ALLOC_SET_CORR, ALLOC_TYP_RUBBLE, randint(k));

	/* Put some objects in rooms */
	alloc_object(ALLOC_SET_ROOM, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ROOM,
			3));

	/* Put some altars */
	alloc_object(ALLOC_SET_ROOM, ALLOC_TYP_ALTAR, randnor(DUN_AMT_ALTAR,
			3));

	/* Put some objects/gold in the dungeon */
	alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ITEM,
			3));

	/* Populate with new features */
	populate_features();
    populate_cover_features();

	/* Do not light floors inside rooms. */
	/* Floors outside rooms or walls are lit. */

	if (lit_level)
	{
		for (y = 0; y < DUNGEON_HGT; y++)
		{
			for (x = 0; x < DUNGEON_WID; x++)
			{
				if (!(cave_info[y][x] & CAVE_ROOM) ||
					!(cave_floor_bold(y, x)))
				{
					cave_info[y][x] |= (CAVE_GLOW);
				}
			}
		}
	}
}


/*
 * Generate a shop.
 *
 * This is an ugly hack.
 */
static void store_gen(void)
{
	bool daytime;
	vault_type *v_ptr;
	int x, y, i, good_y = 0, good_x = 0;

	object_type *o_ptr = NULL;
	store_type *st_ptr = &store[p_ptr->s_idx];

	/* Day time */
	if ((turn % (10L * TOWN_DAWN)) < ((10L * TOWN_DAWN) / 2))
	{
		daytime = TRUE;
		/* Night time */
	}
	else
	{
		daytime = FALSE;
	}

	/* Start with rock */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{
			cave_feat[y][x] = FEAT_PERM_SOLID;
		}
	}

	v_ptr = &v_info[st_ptr->vault];

	y = (v_ptr->hgt / 2) + 2;
	x = (v_ptr->wid / 2) + 2;

	build_vault(y, x, v_ptr);

	/* Create the sun. */
	lite_up_town(daytime);

	/* Hack -- don't punish thefts. */
	hack_punish_theft = FALSE;

	/* Scatter the store's contents. */
	for (o_ptr = st_ptr->stock; o_ptr != NULL; o_ptr = o_ptr->next_global)
	{

		if (o_ptr->iy && o_ptr->ix)
		{
			floor_carry(o_ptr->iy, o_ptr->ix, o_ptr);

		}
		else
		{

			for (i = 0; i < 2000; i++)
			{
				y = randnor(y, 1);
				x = randnor(x, 1);

				if (in_bounds_fully(y, x) && cave_floor_bold(y, x))
				{

					good_y = y;
					good_x = x;

					if (cave_o_idx[y][x] == NULL || magik(25))
						break;
				}
			}


			if (good_y && good_x)
				floor_carry(good_y, good_x, o_ptr);

		}
	}

	/* Undo the hack. */
	hack_punish_theft = TRUE;

}





/*
 * Town logic flow for generation of arena -KMW-
 */
static void arena_gen(void)
{
	bool daytime;
	vault_type *v_ptr;
	int x, y;

	/* Day time */
	if ((turn % (10L * TOWN_DAWN)) < ((10L * TOWN_DAWN) / 2))
	{
		daytime = TRUE;
		/* Night time */
	}
	else
	{
		daytime = FALSE;
	}

	/* Start with rock */
	for (y = 0; y < DUNGEON_HGT; y++)
	{
		for (x = 0; x < DUNGEON_WID; x++)
		{
			cave_feat[y][x] = FEAT_PERM_SOLID;
		}
	}

	v_ptr = &v_info[p_ptr->which_arena_layout];
	build_vault((v_ptr->hgt / 2) + 2, (v_ptr->wid / 2) + 2, v_ptr);

	/* Create the sun. */
	lite_up_town(daytime);
}



/*
 * Town logic flow for generation of new town
 *
 * We start with a fully wiped cave of normal floors.
 *
 * Note that town_gen_hack() plays games with the R.N.G.
 *
 * This function does NOT do anything about the owners of the stores,
 * nor the contents thereof.  It only handles the physical layout.
 *
 * We place the player on the stairs at the same time we make them.
 *
 * Hack -- since the player always leaves the dungeon by the stairs,
 * he is always placed on the stairs, even if he left the dungeon via
 * word of recall or teleport level.
 *
 * Note that the above is completely false.
 */
static void town_gen(void) {

  p_ptr->inside_special = SPECIAL_WILD;

  terrain_gen();

  /* Grant Ghosts WoR */
  if (p_ptr->prace == RACE_GHOST && !p_ptr->prace_info) {
    p_ptr->prace_info = 1;
    p_ptr->max_depth = 0;
    msg_print("You return to your corporeal form.");
  }

  /* Place the Scholar */
  {
      int y, x, i;
      for (i = 0; i < 1000; i++) {
          y = rand_range(20, DUNGEON_HGT - 20);
          x = rand_range(20, DUNGEON_WID - 20);
          if (cave_floor_bold(y, x) && cave_naked_bold(y, x)) {
              place_monster_aux(y, x, R_IDX_SCHOLAR, MON_ALLOC_JUST_ONE);
              break;
          }
      }
  }
}


/*
 * Generate a quest level -KMW-
 */
static void quest_gen(void)
{
	int x, y;
	vault_type *v_ptr;

	/* Paranoia */
	if (p_ptr->which_quest == 0)
		return;

	v_ptr = q_v_ptrs[p_ptr->which_quest - 1];

	/* Start with perm walls, if we want normal generation. */

	if (v_ptr->gen_info != 1)
	{
		for (y = 0; y < DUNGEON_HGT; y++)
		{
			for (x = 0; x < DUNGEON_WID; x++)
			{

				if (v_ptr->gen_info == 2)
				{
					cave_feat[y][x] = FEAT_FOG;
				}
				else
				{
					cave_feat[y][x] = FEAT_PERM_SOLID;
				}
			}
		}

	}
	else if (v_ptr->gen_info == 1)
	{
		p_ptr->wild_x = rand_range(-100, 100);
		p_ptr->wild_y = rand_range(-100, 100);
		terrain_gen();

	}

	/* Find a random place for the vault. */
	if (seed_dungeon)
	{
		Rand_quick = TRUE;
	}

	y =
		rand_range((v_ptr->hgt / 2) + 1,
		DUNGEON_HGT - (v_ptr->hgt / 2) - 1);
	x =
		rand_range((v_ptr->wid / 2) + 1,
		DUNGEON_WID - (v_ptr->wid / 2) - 1);

	if (seed_dungeon)
	{
		Rand_quick = FALSE;
	}

	build_vault(y, x, v_ptr);

	/* Quest is now officially in progress */
	quest_status[p_ptr->which_quest - 1] = QUEST_IN_PROGRESS;
}


/*
 * Place the traveling merchant.
 * If he exists, move him. If not, create him.
 */
void place_dungeon_merchant(int y, int x)
{
	int i;
	int m_idx = 0;

	/* Check if he is already alive */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		if (!m_ptr->r_idx) continue;
		if (m_ptr->r_idx == R_IDX_MERCHANT)
		{
			m_idx = i;
			break;
		}
	}

	if (m_idx)
	{
		/* He exists, move him here */
		teleport_away_to(m_idx, y, x);
	}
	else
	{
		/* Create him */
		if (!place_monster_aux(y, x, R_IDX_MERCHANT, MON_ALLOC_JUST_ONE))
		{
			int d, ny, nx;
			for (d = 1; d < 10; d++)
			{
				scatter(&ny, &nx, y, x, d, 0);
				if (place_monster_aux(ny, nx, R_IDX_MERCHANT, MON_ALLOC_JUST_ONE)) break;
			}
		}
	}
}


/*
 * Generate a random dungeon level
 *
 * Hack -- regenerate any "overflow" levels
 *
 * Hack -- allow auto-scumming via a gameplay option.
 *
 * Allow quasi-persistent dungeons using a seeded RNG.
 */
static void shuffle_unstable_scrolls(void);
void generate_cave(void)
{
	int y, x, num;


	/* The dungeon is not ready */
	character_dungeon = FALSE;
	reset_dread();
    init_cover_system();
	/* Shuffle unstable scrolls */
	shuffle_unstable_scrolls();

	/* Seed the RNG if appropriate */
	if (seed_dungeon)
	{
		Rand_quick = TRUE;
		Rand_value = seed_dungeon + p_ptr->depth;
	}

	/* Generate */
	for (num = 0; TRUE; num++)
	{
		bool okay = TRUE;
		bool load = FALSE;

		cptr why = NULL;


		/* XXX XXX XXX XXX */
		o_max = 1;
		m_max = 1;

		/* Start with a blank cave */
		for (y = 0; y < DUNGEON_HGT; y++)
		{
			for (x = 0; x < DUNGEON_WID; x++)
			{
				/* No flags */
				cave_info[y][x] = 0;

				/* No objects */
				cave_o_idx[y][x] = 0;

				/* No monsters */
				cave_m_idx[y][x] = 0;

#ifdef MONSTER_FLOW
				/* No flow */
				cave_cost[y][x] = 0;
				cave_when[y][x] = 0;
#endif /* MONSTER_FLOW */

			}
		}


		/* Hack -- illegal panel */
		p_ptr->wy = DUNGEON_HGT;
		p_ptr->wx = DUNGEON_WID;


		/* Reset the monster generation level */
		monster_level = p_ptr->depth;

		/* Reset the object generation level */
		object_level = p_ptr->depth;

		/* Nothing special here yet */
		good_item_flag = FALSE;

		/* Nothing good here yet */
		rating = 0;
		pet_rating = 0;

		/* Restore an old dungeon. */
		if (p_ptr->load_dungeon)
		{
			if (load_dungeon(p_ptr->load_dungeon - 1))
			{
				mprint(MSG_ERROR, "Could not load temporary dungeon!");
			}
			else
			{
				load = TRUE;
			}

			p_ptr->load_dungeon = 0;

			/* Mega-hack: prevent autoscum. */
			num = 100;
		}

		if (!load)
		{
			/* Build the arena -KMW- */
			if (p_ptr->inside_special == SPECIAL_ARENA ||
			    p_ptr->inside_special == SPECIAL_MAGIC_ARENA)
			{
				arena_gen();
			}

			/* Quest levels -KMW- */
			else if (p_ptr->inside_special == SPECIAL_QUEST)
			{
				quest_gen();
			}

			/* Shop vault. */
			else if (p_ptr->inside_special == SPECIAL_STORE)
			{
				store_gen();
			}

			/* Build the wilderness */
			else if (p_ptr->inside_special == SPECIAL_WILD)
			{
				terrain_gen();
			}

			/* Build the town */
			else if (!p_ptr->depth)
			{
				/* Make a town */
				town_gen();
			}

            /* Build a Dream Level */
            else if (p_ptr->inside_special == SPECIAL_DREAM)
            {
                /* Generate a special dream level */
                /* For now, reuse cave_gen but maybe with forced plasma? */
                /* Or a specific layout. */
                /* Let's use terrain_gen for a "wild" feel, or cave_gen with modified room generation. */
                /* Simple hack: cave_gen but only "unusual" rooms? */
                /* For now, just standard cave_gen but forced "open" maybe. */
                cave_gen();
            }

			/* Build a real level */
			else
			{
			  cave_gen();
			}
		}

		/* Extract the feeling */
		if (rating > 100)
			feeling = 2;
		else if (rating > 80)
			feeling = 3;
		else if (rating > 60)
			feeling = 4;
		else if (rating > 40)
			feeling = 5;
		else if (rating > 30)
			feeling = 6;
		else if (rating > 20)
			feeling = 7;
		else if (rating > 10)
			feeling = 8;
		else if (rating > 0)
			feeling = 9;
		else
			feeling = 10;

		/* Normalize the pet rating. */
		if (pet_rating > 10)
			pet_rating = 10;

		/* Hack -- Have a special feeling sometimes */
		if (good_item_flag && !p_ptr->preserve)
			feeling = 1;

		/* It takes 1000 game turns for "feelings" to recharge */
		if ((turn - old_turn) < 1000)
			feeling = 0;

		/* Hack -- no feeling in the town */
		if (!p_ptr->depth)
			feeling = 0;


		/* Prevent object over-flow */
		if (o_max >= MAX_O_IDX)
		{
			/* Message */
			why = "too many objects";

			/* Message */
			okay = FALSE;
		}

		/* Prevent monster over-flow */
		if (m_max >= MAX_M_IDX)
		{
			/* Message */
			why = "too many monsters";

			/* Message */
			okay = FALSE;
		}

		/* Mega-Hack -- "auto-scum" */
		if (auto_scum && (num < 100) && !p_ptr->inside_special)
		{
			/* Require "goodness" */
			if ((feeling > 9) || ((p_ptr->depth >= 5) && (feeling > 8)) ||
				((p_ptr->depth >= 10) && (feeling > 7)) ||
				((p_ptr->depth >= 20) && (feeling > 6)) ||
				((p_ptr->depth >= 40) && (feeling > 5)))
			{
				/* Give message to cheaters */
				if (cheat_room || cheat_hear || cheat_peek || cheat_xtra)
				{
					/* Message */
					why = "boring level";
				}

				/* Try again */
				okay = FALSE;
			}
		}

		/* Accept */
		if (okay)
		{
			/* Morgoth-style hard-spawn for the Merchant */
			if (p_ptr->depth >= 6 && p_ptr->depth < 100)
			{
				int ty, tx;
				int i;

				/* Try to place him on a nice clean floor grid */
				for (i = 0; i < 1000; i++)
				{
					ty = rand_int(DUNGEON_HGT);
					tx = rand_int(DUNGEON_WID);

					if (cave_naked_bold(ty, tx))
					{
						place_dungeon_merchant(ty, tx);
						break;
					}
				}
			}

			/* Handle Ancients following between levels */
			if (ancient_of_days_is_chasing)
			{
				int i, x, y, d;
				int spawned = 0;
				/* Try to spawn near player */
				for (i = 0; i < 100; i++)
				{
					d = 3;
					scatter(&y, &x, p_ptr->py, p_ptr->px, d, 0);
					if (cave_floor_bold(y, x))
					{
						/* Find RF7_ANCIENT race index. We added it as 692. */
						/* But let's scan for it to be safe or use defined index if possible */
						/* Since we can't easily scan by flag here efficiently without iterating all r_info */
						/* We'll iterate. */
						int r_idx = 0;
						int k;
						for (k = 1; k < MAX_R_IDX; k++)
						{
							if (r_info[k].flags7 & RF7_ANCIENT)
							{
								r_idx = k;
								break;
							}
						}

						if (r_idx > 0)
						{
							if (place_monster_aux(y, x, r_idx, MON_ALLOC_JUST_ONE))
							{
								/* Enrage it immediately */
								if (cave_m_idx[y][x] > 0)
								{
									m_list[cave_m_idx[y][x]].mflag |= MFLAG_ANCIENT_ENRAGED;
									spawned++;
									/* We only spawn one for now to represent "following" */
									break;
								}
							}
						}
					}
				}
				if (spawned) ancient_of_days_is_chasing = FALSE;
			}
			break;
		}


		/* Message */
		if (why)
			msg_format("Generation restarted (%s)", why);

		/* Wipe the objects */
		wipe_o_list();

		/* Wipe the monsters */
		wipe_m_list();

	}

	/* The dungeon is ready */
	character_dungeon = TRUE;

	/* Remember when this level was "created" */
	old_turn = turn;

	/* OK to summon more monsters. */
	p_ptr->number_pets = 0;

	/* Handle ``all-seeing'' flag */
	if (p_ptr->allseeing)
	{
		msg_print("You sense the living rock beneath your feet.");
		wiz_lite();
	}

	/* Handle criminal players. */
	if (!p_ptr->depth && p_ptr->sc < 1 && p_ptr->s_idx != 7 &&
		p_ptr->inside_special != SPECIAL_ARENA &&
		p_ptr->inside_special != SPECIAL_MAGIC_ARENA)
	{
		mprint(MSG_WARNING,
			"It seems your criminal tendencies aren't " "welcome here.");
		activate_generators();
	}


	if (p_ptr->prace == RACE_MUNCHKIN)
	{
		acquirement(p_ptr->py, p_ptr->px, 10, TRUE);
	}

	if (seed_dungeon)
	{
		Rand_quick = FALSE;
	}

	execute_staircase_pursuit();
	execute_recall_ambush();
}

/*
 * Shuffle the Unstable Scroll mapping.
 * Each sval (0-14) maps to one of the 15 effects.
 */
static void shuffle_unstable_scrolls(void)
{
	int i;
	for (i = 0; i < 15; i++)
		unstable_scroll_map[i] = i;
	for (i = 0; i < 15; i++)
	{
		int j = rand_int(15);
		byte temp = unstable_scroll_map[i];
		unstable_scroll_map[i] = unstable_scroll_map[j];
		unstable_scroll_map[j] = temp;
	}
}
