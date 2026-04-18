import re
import glob

files_to_update = ['src/melee2.c', 'src/monster2.c', 'src/melee1.c', 'src/generate.c', 'src/cave.c']

for filepath in files_to_update:
    with open(filepath, 'r') as f:
        c = f.read()

    # Generic replace for _UP and _DOWN
    c = re.sub(r'FEAT_RAMP_UP', 'FEAT_RAMP', c)
    c = re.sub(r'FEAT_RAMP_DOWN', 'FEAT_RAMP', c)
    c = re.sub(r'FEAT_STAIRS_UP', 'FEAT_STAIRS', c)
    c = re.sub(r'FEAT_STAIRS_DOWN', 'FEAT_STAIRS', c)
    c = re.sub(r'FEAT_LADDER_UP', 'FEAT_LADDER', c)
    c = re.sub(r'FEAT_LADDER_DOWN', 'FEAT_LADDER', c)
    c = re.sub(r'FEAT_ROPE_UP', 'FEAT_ROPE', c)
    c = re.sub(r'FEAT_ROPE_DOWN', 'FEAT_ROPE', c)
    c = re.sub(r'FEAT_SLOPE_UP', 'FEAT_SLOPE', c)
    c = re.sub(r'FEAT_SLOPE_DOWN', 'FEAT_SLOPE', c)

    # Simplify redundant checks like feat == FEAT_RAMP || feat == FEAT_RAMP
    # This might need some custom regex, but let's try a simple approach first
    c = re.sub(r'feat == FEAT_RAMP \|\| feat == FEAT_RAMP', 'feat == FEAT_RAMP', c)
    c = re.sub(r'feat != FEAT_RAMP && feat != FEAT_RAMP', 'feat != FEAT_RAMP', c)
    # And similar for others...

    with open(filepath, 'w') as f:
        f.write(c)
