with open('src/cmd5.c', 'r') as f:
    content = f.read()

# We need to make sure that we also do the check in `remove_mutation` because otherwise if a player has an old save and rolls a negative mutation to remove, it will successfully remove it... but wait, if it's already useless, it doesn't matter if they remove it.
# Actually, if we restrict `generate_mutation`, `i` does not increment when we `continue`, which is correct. The `i` counts the number of valid attempts to find a *new* mutation (one the player doesn't already have). So if we roll a negative one, we just skip it entirely and don't count it as an attempt. This is correct.

# But what if `MAX_MUTS` is not much larger than the number of positive mutations, and the player already has all of them?
# MAX_MUTS is 100. There are 25 negative ones (41-65). That leaves 75. Wait, only 68 mutations are defined (0-68). So 43 are positive/neutral.
# If the player has all 43, `i` will eventually reach 25 and exit. This is fine.

pass
