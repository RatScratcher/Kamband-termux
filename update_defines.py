import re

with open('src/defines.h', 'r') as f:
    content = f.read()

# Replace _UP and _DOWN with single features
content = re.sub(r'#define FEAT_SLOPE_UP\s+160.*\n#define FEAT_SLOPE_DOWN\s+161.*', '#define FEAT_SLOPE          160 /* Gradual slope */', content)
content = re.sub(r'#define FEAT_RAMP_UP\s+170.*\n#define FEAT_RAMP_DOWN\s+171.*', '#define FEAT_RAMP             170 /* Gradual slope, accessible */', content)
content = re.sub(r'#define FEAT_STAIRS_UP\s+172.*\n#define FEAT_STAIRS_DOWN\s+173.*', '#define FEAT_STAIRS           172 /* Stone stairs */', content)
content = re.sub(r'#define FEAT_LADDER_UP\s+174.*\n#define FEAT_LADDER_DOWN\s+175.*', '#define FEAT_LADDER           174 /* Wooden/metal ladder */', content)
content = re.sub(r'#define FEAT_ROPE_UP\s+178.*\n#define FEAT_ROPE_DOWN\s+179.*', '#define FEAT_ROPE             178 /* Rope hanging down */', content)

# update cave_floor_bold
content = re.sub(r'FEAT_SLOPE_UP', 'FEAT_SLOPE', content)

with open('src/defines.h', 'w') as f:
    f.write(content)
