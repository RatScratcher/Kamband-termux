Ah! `build_sector_cliff` uses `FEAT_CLIMBABLE` `:` and `FEAT_CLIFF_UP` `|`!
Wait, `FEAT_CLIFF_UP` is `|`!
So the user said:
"Currently, elevated areas (represented by green . and ^ boundaries) have multiple access points at the top (the line of |, :, and = symbols). Modify the generation so that an elevated plateau has exactly one designated access point."
Wait.
"represented by green . and ^ boundaries" -> these are `FEAT_FLOOR` (`.`) and maybe `FEAT_SLOPE_UP`?
"the line of |, :, and = symbols" -> this is `FEAT_CLIFF_UP` (`|`), `FEAT_CLIMBABLE` (`:`), and `FEAT_LADDER_UP` (`=`).
If `FEAT_CLIFF_UP` `|` is an access point? NO!
`FEAT_CLIFF_UP` is NOT an access point, it is impassable from below!
But the user says "the line of |, :, and = symbols".
If `FEAT_CLIFF_UP` looks like `|`, the user might think the whole cliff face is an access point because you can walk on it if you fall down, or they don't distinguish them?
Wait, if `diff == 1` for `ELEV_HILL` vs `ELEV_GROUND`, then `elev_allows_move` allows moving up ANY tile, including `FEAT_CLIFF_UP` (`|`)!
YES! If the elevation difference is only 1, the player can step up ONTO `FEAT_CLIFF_UP`!
Because in `elev_allows_move`:
```c
    /* Going up - need assistance or climbable surface */
    int diff = dst_elev - src_elev;

    if (diff >= 2) { ... }

    /* Small elevation change - can usually manage */
    return TRUE;
```
If `diff == 1`, IT JUST RETURNS `TRUE`!
So ANY elevation feature, EVEN `FEAT_CLIFF_UP`, can be walked up if the elevation is only `ELEV_HILL`!
That's why one side of the hill is completely open! I set the boundary to `ELEV_HILL` (which is `1`) and gave it `FEAT_CLIFF_UP` (`|`). Since `diff = 1`, the player can just walk up the `|`!
To make it impassable, the elevation MUST BE `ELEV_HIGH`!
Or I must modify `elev_allows_move` to check for `FEAT_CLIFF_UP` and block it even if `diff == 1`!
Wait! The user specifically said:
"All other boundary tiles of the elevation must be 'impassable' to standard ground movement, forcing a tactical choke point."
If I just use `FEAT_WALL_OUTER`, it is impassable. But `FEAT_WALL_OUTER` blocks line of sight.
If I use `ELEV_HIGH`, the diff is 2, and `elev_allows_move` will block it unless there is an access point!
If I set `next_elev` of the whole hill to `ELEV_HIGH`, then `diff` is 2!
Let's see if setting the entire hill to `ELEV_HIGH` makes it work.
But if I set the whole hill to `ELEV_HIGH`, I can use `FEAT_CLIFF_UP` on the edges, and `FEAT_FLOOR` inside.
Then `diff` = 2, so it's impassable!
Let's rewrite `build_sector_hill` to only use `ELEV_HIGH` for the hill.
