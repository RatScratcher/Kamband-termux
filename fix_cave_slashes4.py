with open('src/cave.c', 'r') as f:
    lines = f.readlines()

with open('src/cave.c', 'w') as f:
    for line in lines:
        if 'case FEAT_SLOPE:' in line:
            f.write("        case FEAT_SLOPE:        return '/';\n")
        elif 'case FEAT_RAMP:' in line and 'return' in line:
            f.write("        case FEAT_RAMP:         return '/';\n")
        elif 'case FEAT_LADDER:' in line and 'return' in line:
            f.write("        case FEAT_LADDER:       return '=';\n")
        elif 'case FEAT_ROPE:' in line and 'return' in line:
            f.write("        case FEAT_ROPE:         return '|';\n")
        else:
            f.write(line)
