with open('src/cave.c', 'r') as f:
    c = f.read()

c = c.replace(r"case FEAT_SLOPE:        return '\/\';", "case FEAT_SLOPE:        return '/';")
c = c.replace(r"case FEAT_RAMP:         return '\/\';", "case FEAT_RAMP:         return '/';")
c = c.replace(r"case FEAT_LADDER:       return '\=';", "case FEAT_LADDER:       return '=';")
c = c.replace(r"case FEAT_ROPE:         return '\|';", "case FEAT_ROPE:         return '|';")

with open('src/cave.c', 'w') as f:
    f.write(c)
