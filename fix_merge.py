def main():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    start_str = "<<<<<<< HEAD\n"
    end_str = ">>>>>>> origin/main\n"

    start_idx = content.find(start_str)
    end_idx = content.find(end_str)

    if start_idx != -1 and end_idx != -1:
        # Basically HEAD deleted all of this, and origin/main added something, or vice versa
        # Since our task was to completely remove the old fractal functions and replace them with populated stamps,
        # we can just delete this whole conflict block.
        content = content[:start_idx] + content[end_idx + len(end_str):]

    with open('src/generate.c', 'w') as f:
        f.write(content)

if __name__ == "__main__":
    main()
