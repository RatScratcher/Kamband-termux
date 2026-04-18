import sys

def main():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    # Find where to put ensure_elevation_access
    target = "static void clean_isolated_features"
    idx = content.find(target)

    print(content[idx-100:idx+100])

if __name__ == "__main__":
    main()
