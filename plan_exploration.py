import re

with open('src/generate.c', 'r') as f:
    content = f.read()

print("Look for CA smoothing logic or where to place it:")
