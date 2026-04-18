import re
with open('src/cave.c', 'r') as f:
    c = f.read()
c = c.replace("        case FEAT_RAMP:         return \'\/\';", "        case FEAT_RAMP:         return '/';")
c = c.replace("        case FEAT_LADDER:       return \'\=\';", "        case FEAT_LADDER:       return '=';")
c = c.replace("        case FEAT_ROPE:         return \'\|\';", "        case FEAT_ROPE:         return '|';")
c = c.replace("        case FEAT_SLOPE:        return \'\/\';", "        case FEAT_SLOPE:        return '/';")
with open('src/cave.c', 'w') as f:
    f.write(c)
