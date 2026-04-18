import re

with open('src/generate.c', 'r') as f:
    content = f.read()

# Update apply_sector_borders_and_access definition
content = re.sub(
    r'static void apply_sector_borders_and_access\(int y1, int x1, int y2, int x2,\s*\n\s*int elev_check, int edge_feat, int access_feat1, int access_feat2\)',
    r'static void apply_sector_borders_and_access(int y1, int x1, int y2, int x2,\n                                            int elev_check, int edge_feat)',
    content
)

# Replace the inner logic inside apply_sector_borders_and_access
old_logic = r'''        for \(i = 0; i < to_convert; i\+\+\) \{\n            int cy = access_y\[i\];\n            int cx = access_x\[i\];\n            cave_feat\[cy\]\[cx\] = \(rand_int\(100\) < 50\) \? access_feat1 : access_feat2;'''

new_logic = r'''        for (i = 0; i < to_convert; i++) {\n            int cy = access_y[i];\n            int cx = access_x[i];\n\n            /* Unified transit features */\n            int roll = rand_int(100);\n            if (roll < 40) cave_feat[cy][cx] = FEAT_RAMP;\n            else if (roll < 70) cave_feat[cy][cx] = FEAT_LADDER;\n            else cave_feat[cy][cx] = FEAT_STAIRS;'''

content = re.sub(old_logic, new_logic, content)

# Remove the extra arguments from calls to apply_sector_borders_and_access
content = re.sub(r'apply_sector_borders_and_access\(sy1, sx1, sy2, sx2, ELEV_HIGH, FEAT_CLIFF_UP, FEAT_STAIRS_UP, FEAT_LADDER_UP\);',
                 r'apply_sector_borders_and_access(sy1, sx1, sy2, sx2, ELEV_HIGH, FEAT_CLIFF_UP);', content)

content = re.sub(r'apply_sector_borders_and_access\(sy1, sx1, sy2, sx2, ELEV_LOW, FEAT_CLIFF_DOWN, FEAT_ESCAPE_PIT, FEAT_RAMP_UP\);',
                 r'apply_sector_borders_and_access(sy1, sx1, sy2, sx2, ELEV_LOW, FEAT_CLIFF_DOWN);', content)

with open('src/generate.c', 'w') as f:
    f.write(content)
