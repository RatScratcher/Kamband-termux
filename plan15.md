`cave_floor_bold(Y,X)` is true if `!(cave_feat[Y][X] & 0x20)` OR if it's an elevation feature (from `FEAT_SLOPE_UP` to `FEAT_ESCAPE_PIT`).
`FEAT_CLIFF_UP` is 162. Wait, `162 & 0x20` is `162 & 32` = 162 (10100010) & 32 (00100000) = 32 != 0.
But it is between `FEAT_SLOPE_UP` (160) and `FEAT_ESCAPE_PIT` (180).
So `cave_floor_bold` returns TRUE!
And `elev_allows_move` returns TRUE because `ELEV_HILL` - `ELEV_GROUND` = 1.
So the player CAN just walk right over `FEAT_CLIFF_UP` if `diff == 1`!
This means `FEAT_CLIFF_UP` is NOT IMPASSABLE if the elevation difference is only 1!
Is `diff` 1? Yes, `ELEV_HILL` is 1, `ELEV_GROUND` is 0.
Ah! So to make the boundary impassable to standard ground movement, we must make `diff >= 2` or use an ACTUAL wall feature!
But wait, if we use `FEAT_WALL_OUTER`, it is impassable. But `FEAT_WALL_OUTER` is a wall (`#`). It blocks sight and looks like a wall, not an elevated plateau boundary.
Wait! What if we set the boundary elevation to `ELEV_HIGH`?
If the boundary is `ELEV_HIGH` (which is 2), then `ELEV_HIGH` - `ELEV_GROUND` = 2.
If `diff == 2`, `elev_allows_move` returns FALSE unless it's a ramp/stair.
Yes! If we set the whole hill to `ELEV_HIGH`, then `diff` is 2, and climbing requires a ramp!
Let's check `ELEV_HIGH` vs `ELEV_HILL`.
