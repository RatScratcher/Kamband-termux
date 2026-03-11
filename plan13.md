Let's see the old Choke Point Logic:
```c
	/* Outer walls and slopes */
    for (y = y1 - 1; y <= y2 + 1; y++) {
        for (x = x1 - 1; x <= x2 + 1; x++) {
            if (!in_bounds(y, x)) continue;
            if (cave_feat[y][x] != FEAT_FLOOR &&
                cave_feat[y][x] != FEAT_SLOPE_UP && ...
```
My logic:
```c
        for (y = y1; y <= y2; y++) {
                for (x = x1; x <= x2; x++) {
                        if (get_elevation(y, x) == ELEV_HILL || get_elevation(y, x) == ELEV_HIGH) {
                                bool is_boundary = FALSE;
                                ...
                                                if (in_bounds(ny, nx) && get_elevation(ny, nx) == ELEV_GROUND) {
                                                        is_boundary = TRUE;
                                                }
```
What if `get_elevation(ny, nx)` returns something else?
If the tile is outside `y1..y2`, what is its elevation?
Ah! `cave_elev` is 0 (`ELEV_GROUND`) by default.
BUT, wait... `set_elevation(y, x, elev)` sets it.
What if `y1..y2` is just a rectangle, but the CA causes it to grow or shrink?
Wait, if the CA smooths it, it does:
```c
                                if (current_elev > ELEV_GROUND && elev_neighbors < 3) {
                                        next_elev[y][x] = ELEV_GROUND;
                                } else if (current_elev == ELEV_GROUND && elev_neighbors >= 5) {
                                        next_elev[y][x] = ELEV_HILL;
                                } else {
                                        next_elev[y][x] = current_elev;
                                }
```
If `current_elev == ELEV_GROUND && elev_neighbors >= 5`, it turns into `ELEV_HILL`.
Wait, `next_elev` is only 50x50. The loop runs from `0` to `h`. `h` is 41. `w` is 41.
This is fully contained.

Why is "one side of each hill completely open for access"?
Because `cave_feat[y][x] = FEAT_CLIFF_UP;` is only placed on tiles that are `ELEV_HILL` or `ELEV_HIGH`.
What about `FEAT_SLOPE_UP`? `FEAT_SLOPE_UP` IS the feature for `ELEV_HILL`.
Wait! If `FEAT_CLIFF_UP` is set, `ELEV_HILL` remains. But `FEAT_CLIFF_UP` is impassable from below!
Let's check `elev_allows_move`:
```c
    /* Going down is usually fine (unless too steep) */
    if (dst_elev < src_elev) { ... }
    /* Going up */
    if (dst_elev > src_elev) {
        if (dst_elev - src_elev >= 2) { ... }
        /* Just one level up */
        if (feat != FEAT_RAMP_UP && feat != FEAT_STAIRS_UP &&
            feat != FEAT_LADDER_UP && feat != FEAT_ROPE_UP &&
            feat != FEAT_ESCAPE_PIT && feat != FEAT_CLIMBABLE) {
            return FALSE;
        }
    }
```
Wait! If `dst_elev > src_elev`, it requires an access feature to go up!
Even if `cave_feat[y][x]` is `FEAT_FLOOR` but its elevation is `ELEV_HILL` (which is > `ELEV_GROUND`), the player CANNOT move up unless there is a `FEAT_RAMP_UP`, `FEAT_STAIRS_UP`, etc.!
Wait! Does `FEAT_CLIFF_UP` matter?
Ah! `FEAT_CLIFF_UP` is `162`.
If the player tries to move onto an `ELEV_HILL` tile from `ELEV_GROUND`, `elev_allows_move` returns `FALSE` UNLESS the destination tile is `FEAT_RAMP_UP` or similar!
So ANY boundary between `ELEV_GROUND` and `ELEV_HILL` that is not a ramp is IMPASSABLE.
If I just left the boundary as `FEAT_SLOPE_UP`, wait, `FEAT_SLOPE_UP` is an access feature? No!
Wait! I didn't see `FEAT_SLOPE_UP` in the access feature list!
```c
        /* Need stairs, ramp, or ladder (Bi-directional check) */
        if (feat == FEAT_RAMP_DOWN || feat == FEAT_RAMP_UP ||
            feat == FEAT_STAIRS_DOWN || feat == FEAT_STAIRS_UP ||
            feat == FEAT_LADDER_DOWN || feat == FEAT_LADDER_UP ||
            feat == FEAT_ROPE_DOWN || feat == FEAT_ROPE_UP ||
            feat == FEAT_JUMP_POINT || feat == FEAT_ESCAPE_PIT ||
            feat == FEAT_CLIMBABLE) {
```
`FEAT_SLOPE_UP` is NOT in the list! So `FEAT_SLOPE_UP` is actually impassable from below!
Wait, if `FEAT_SLOPE_UP` is NOT in the list, then it's impassable?
But `FEAT_SLOPE_UP` is "Gradual slope upward". If it's impassable, why does the game call it "slope upward"?
Wait...
