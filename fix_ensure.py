def main():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    # Move ensure_connectivity to before build_sector_populated
    ensure_str = "static void ensure_connectivity(int y1, int x1, int y2, int x2)"
    idx_ensure = content.find(ensure_str)

    if idx_ensure != -1:
        # Find the end of ensure_connectivity
        brace_start = content.find('{', idx_ensure)
        brace_count = 0
        in_function = False
        end_idx = -1
        for i in range(brace_start, len(content)):
            if content[i] == '{':
                if not in_function:
                    in_function = True
                brace_count += 1
            elif content[i] == '}':
                brace_count -= 1
                if in_function and brace_count == 0:
                    end_idx = i + 1
                    break

        if end_idx != -1:
            ensure_func = content[idx_ensure:end_idx]

            # Remove it from its current position
            content = content[:idx_ensure] + content[end_idx:]

            # Place it before build_sector_populated
            idx_populated = content.find("static void build_sector_populated")
            if idx_populated != -1:
                content = content[:idx_populated] + ensure_func + "\n\n" + content[idx_populated:]

    with open('src/generate.c', 'w') as f:
        f.write(content)

if __name__ == "__main__":
    main()
