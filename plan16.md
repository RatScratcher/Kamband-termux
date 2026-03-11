If `ELEV_HILL` is 1 and `ELEV_GROUND` is 0, then `diff` is 1. `elev_allows_move` says "Small elevation change - can usually manage" for `diff` < 2.
So `ELEV_HILL` is ALWAYS accessible from `ELEV_GROUND` without ramps, stairs, ladders!
Wait! The original code used `FEAT_WALL_OUTER` (`#`) around the hill!
```c
                if (next_to_floor) {
                    cave_feat[y][x] = FEAT_WALL_OUTER;
                }
```
In the original `build_sector_hill` code, it wrapped the entire hill in `FEAT_WALL_OUTER`. It was literally a block of walls with a few `FEAT_RAMP_UP`s cut into it!
Because it wrapped it in `FEAT_WALL_OUTER`, it was impassable.
The user's complaint: "The hills are still blocks."
If they are blocks, it's because the distance function creates a diamond/block, AND the outer wall wrapper was making it blocky.
If we want a tactical choke point, we should make the boundary impassable.
We can either:
1. Make the elevation `ELEV_HIGH` (which makes `diff >= 2`, blocking movement except at the ramp). Then we can use `FEAT_CLIFF_UP` or `FEAT_SLOPE_UP` and it will physically block movement!
But `FEAT_CLIFF_UP` is visually `|` or green `.`? `FEAT_CLIFF_UP` is `|` by default. `FEAT_SLOPE_UP` is `/`.
The user says: "Currently, elevated areas (represented by green . and ^ boundaries) have multiple access points at the top (the line of |, :, and = symbols). Modify the generation so that an elevated plateau has exactly one designated access point."
Wait! If `FEAT_SLOPE_UP` is the green `.`, then the old code's `ELEV_HILL` ring was the green `.`. But the old code wrapped that in `FEAT_WALL_OUTER` (`#`)!
Wait, if the user sees green `.` and `^` as boundaries, maybe `FEAT_WALL_OUTER` is NOT there in the user's version? Or maybe `FEAT_WALL_OUTER` is what was meant?
Actually, if I just set `next_elev` to `ELEV_HIGH` for the boundary, it will be impassable!
Let's see: if we change the CA rules:
If a floor tile has 5+ `ELEV_HIGH` neighbors, it becomes `ELEV_HIGH`.
And we ONLY use `ELEV_HIGH` (2) for the hill, not `ELEV_HILL` (1).
Then the ENTIRE hill is `ELEV_HIGH`.
Then we set the boundary to `FEAT_CLIFF_UP` or `FEAT_WALL_OUTER` (if they want it to look like a wall).
But they said: "All other boundary tiles of the elevation must be 'impassable' to standard ground movement".
If we just set the boundary to `FEAT_WALL_OUTER`, it is impassable. But if we use `FEAT_CLIFF_UP` with `ELEV_HIGH`, it is ALSO impassable, AND looks like an elevation feature (the line of `|` symbols).
Wait, "the line of |, :, and = symbols" - the user called these "access points"!
Ah! The user thinks `|` is an access point?
Let's check what `FEAT_RAMP_UP`, `FEAT_STAIRS_UP` are visually!
