The user said:
"Can you check this code please. It's not placing only one access point to a hill. One side of each hill is completely open for access. Plus the hills are still blocks."

Wait! If one side of each hill is completely open, what does that mean?
It means `FEAT_CLIFF_UP` was not placed on one side of the hill, or `is_boundary` wasn't TRUE for that side.
Why wouldn't `is_boundary` be TRUE?
Ah! `is_boundary` is TRUE if an `ELEV_HILL` or `ELEV_HIGH` tile has an `ELEV_GROUND` neighbor.
But wait! What if the `ELEV_HILL` touches the edge of the 3x3 block area?
```c
    int y1 = y0 * BLOCK_HGT;
    int x1 = x0 * BLOCK_WID;
    int y2 = (y0 + 2) * BLOCK_HGT;
    int x2 = (x0 + 2) * BLOCK_WID;
```
If the distance function makes the hill extend all the way to `y1`, `y2`, `x1`, or `x2`, then the tiles at `y1` have NO tiles outside the bounding box in the loop:
`in_bounds(ny, nx)` handles grid boundaries, but what about the sector itself?
Wait, if it extends past `y1`, the tiles outside `y1` are ALREADY `ELEV_GROUND` because the rest of the map is `ELEV_GROUND`.
BUT we only iterate `y` from `y1` to `y2`. We check `y+dy` and `x+dx`. If `y+dy` is OUTSIDE the box `y1..y2` or `x1..x2`, what is its elevation?
Ah! If the hill sector hasn't modified those tiles, they are `ELEV_GROUND`.
So `is_boundary` SHOULD be true.

Wait, why are hills still blocks?
```c
    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            int dist = distance(cy, cx, y, x);
...
```
The distance function is `d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));`. That generates an octagonal shape, not a block!
Why are they blocks?
Because `max_dist = MAX(y2 - y1, x2 - x1) / 2;`
And `cy` and `cx` are offset by `rand_int(9) - 4`.
Wait! Cellular automata runs on the box `y1` to `y2`.
If it's an octagonal shape, it's already somewhat blocky.
And Cellular Automata is applied.
BUT wait! In CA:
```c
                                                if (in_bounds(ny, nx) && get_elevation(ny, nx) > ELEV_GROUND) {
                                                        elev_neighbors++;
                                                }
```
If `y` is on the edge of the block, `elev_neighbors` might be low.

Wait, look at this:
"One side of each hill is completely open for access."
Why would one side be open?
If `is_boundary` is false? No, if `is_boundary` is true, it turns into `FEAT_CLIFF_UP`.
But wait! `cave_feat[y][x] = FEAT_CLIFF_UP;`
Is `FEAT_CLIFF_UP` impassable? Yes.
But what if the hill edge doesn't touch `ELEV_GROUND`?
What if `ELEV_HILL` drops to `ELEV_GROUND` inside the sector, so `is_boundary` is true.
Why is it completely open?
Wait! In the original code, `build_sector_hill` had this:
```c
	/* Outer walls and slopes */
    for (y = y1 - 1; y <= y2 + 1; y++) {
        for (x = x1 - 1; x <= x2 + 1; x++) {
```
It iterated from `y1 - 1` to `y2 + 1`!
My loops only iterate from `y1` to `y2`.
If the hill reaches the edge of `y1..y2`, then the tiles AT `y1` are `ELEV_HILL`.
Wait, if they are `ELEV_HILL`, and we check `get_elevation(ny, nx) == ELEV_GROUND`, the tiles at `y1 - 1` are indeed `ELEV_GROUND`.
So `is_boundary` will be true, and they become `FEAT_CLIFF_UP`.

Wait! The previous code:
```c
                if (next_to_floor) {
                    cave_feat[y][x] = FEAT_WALL_OUTER;
                }
```
In the original code, `FEAT_WALL_OUTER` was added to the boundary.
But what if `FEAT_SLOPE_UP` was generated?
Ah! In my CA loop, I replace `cave_feat[cy_coord][cx_coord] = FEAT_SLOPE_UP;` for `ELEV_HILL`.
Then in the Choke Point Logic, I change the boundary to `FEAT_CLIFF_UP`.
Wait... why would one side be completely open?
Because `ELEV_HILL` boundary tiles are made `FEAT_CLIFF_UP`.
But what if a tile is `ELEV_HIGH` and touches `ELEV_GROUND`? It also becomes `FEAT_CLIFF_UP`.
Wait! "One side of each hill is completely open for access."
If it's completely open, it means it's `FEAT_SLOPE_UP` or `FEAT_FLOOR`.
Why would it NOT be converted to `FEAT_CLIFF_UP`?
Maybe `get_elevation(ny, nx)` is NOT `ELEV_GROUND`?
What if the adjacent sector is ALSO elevated? (e.g. another hill?) No, this is block-by-block generation. The adjacent sector hasn't been generated yet, or it's `SECTOR_FLOOR` which is `ELEV_GROUND`.

Let's look at `build_sector_hill` again.
Is `y2` and `x2` inclusive? Yes, `(y0 + 2) * BLOCK_HGT`. `y1 = y0 * BLOCK_HGT`. The height is `y2 - y1 + 1`. This is actually `2 * BLOCK_HGT + 1` = 41.
