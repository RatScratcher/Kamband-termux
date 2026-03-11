Wait!
```c
    /* Small elevation change - can usually manage */
    return TRUE;
```
If `diff` < 2 (e.g., `ELEV_HILL` - `ELEV_GROUND` = 1), `elev_allows_move` returns `TRUE`!
So ANY slope is accessible unless `diff >= 2`!
BUT `ELEV_HILL` is just 1. `ELEV_GROUND` is 0. So `diff` is 1!
If `diff` is 1, `elev_allows_move` returns `TRUE` ALWAYS!
So even if I put `FEAT_CLIFF_UP`, wait... `FEAT_CLIFF_UP` is an impassable terrain?
Let's check `cave_floor_bold` or `process_monster` or `elev_allows_move`.
