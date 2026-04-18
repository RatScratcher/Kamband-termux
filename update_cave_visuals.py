import re

with open('src/cave.c', 'r') as f:
    content = f.read()

# Update cave_feat_color
content = re.sub(r'        case FEAT_RAMP_UP:\n        case FEAT_RAMP_DOWN:\s+return TERM_YELLOW;',
                 r'        case FEAT_RAMP:         return TERM_YELLOW;', content)
content = re.sub(r'        case FEAT_LADDER_UP:\n        case FEAT_LADDER_DOWN:\s+return TERM_UMBER;',
                 r'        case FEAT_LADDER:       return TERM_UMBER;', content)
content = re.sub(r'        case FEAT_ROPE_UP:\n        case FEAT_ROPE_DOWN:\s+return TERM_L_UMBER;',
                 r'        case FEAT_ROPE:         return TERM_L_UMBER;', content)
content = re.sub(r'        case FEAT_SLOPE_UP:\n        case FEAT_SLOPE_DOWN:\s+return TERM_WHITE;',
                 r'        case FEAT_SLOPE:        return TERM_WHITE;', content)

# Update cave_feat_char
content = re.sub(r'        case FEAT_SLOPE_UP:\s+return \'\/\';\n        case FEAT_SLOPE_DOWN:\s+return \'\\\\\';',
                 r'        case FEAT_SLOPE:        return \'\/\';', content)
content = re.sub(r'        case FEAT_RAMP_UP:\s+return \'\/\';\n        case FEAT_RAMP_DOWN:\s+return \'\\\\\';',
                 r'        case FEAT_RAMP:         return \'\/\';', content)
content = re.sub(r'        case FEAT_LADDER_UP:\s+return \'=\';\n        case FEAT_LADDER_DOWN:\s+return \'=\';',
                 r'        case FEAT_LADDER:       return \'=\';', content)
content = re.sub(r'        case FEAT_ROPE_UP:\s+return \'\|\';\n        case FEAT_ROPE_DOWN:\s+return \'\|\';',
                 r'        case FEAT_ROPE:         return \'\|\';', content)

with open('src/cave.c', 'w') as f:
    f.write(content)
