with open('src/generate.c', 'r') as f:
    content = f.read()

# Since ensure_elevation_access is defined BEFORE clean_isolated_features, it might use things or I might have undeclared func if I use clean_isolated_features in build_sector_populated which is defined... wait, build_sector_populated is at line 4908, after clean_isolated_features? No, build_sector_populated is defined before clean_isolated_features?

# Let's check definitions order
def_order = []
for m in ["clean_isolated_features", "ensure_elevation_access", "build_sector_populated"]:
    idx = content.find("static void " + m)
    def_order.append((idx, m))

def_order.sort()
for idx, m in def_order:
    print(f"{m}: {idx}")
