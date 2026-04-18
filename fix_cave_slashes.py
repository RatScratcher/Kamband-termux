import re
with open('src/cave.c', 'r') as f:
    c = f.read()
c = c.replace(r"        case FEAT_RAMP:         return '\/\';", r"        case FEAT_RAMP:         return '/';")
c = c.replace(r"        case FEAT_LADDER:       return '\=';", r"        case FEAT_LADDER:       return '=';")
c = c.replace(r"        case FEAT_ROPE:         return '\|';", r"        case FEAT_ROPE:         return '|';")
c = c.replace(r"        case FEAT_SLOPE:        return '\/\';", r"        case FEAT_SLOPE:        return '/';")
with open('src/cave.c', 'w') as f:
    f.write(c)
