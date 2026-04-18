with open('src/generate.c', 'r') as f:
    content = f.read()

target = "    /* 3.5 Tidy up single tiles */\n    clean_isolated_features(y1, x1, y2, x2);"
new_code = "    /* 3.5 Tidy up single tiles (now done earlier) */"

content = content.replace(target, new_code)

with open('src/generate.c', 'w') as f:
    f.write(content)
