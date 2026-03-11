import re

with open('src/generate.c', 'r') as f:
    content = f.read()

# We need to insert the CA smoothing pass somewhere. Let's find where cave_gen does "Destroy the level if necessary" or similar, which is after rooms are built.
# Let's search for the end of room building or tunnel building.
