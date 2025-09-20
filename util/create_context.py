import os
import re

HEADER_DIR = "../"
OUTPUT_FILE = "combined_headers.txt"

# Regex to match struct/union/typedef declarations
# This will match the start of struct/union/typedef struct
struct_union_start = re.compile(r'\b(typedef\s+)?(struct|union)\s+(\w+)?\s*\{', re.MULTILINE)

def remove_struct_union_bodies(content):
    """
    Removes struct/union implementations but keeps forward declarations.
    Handles nested braces.
    """
    result = []
    i = 0
    while i < len(content):
        match = struct_union_start.search(content, i)
        if not match:
            # No more structs/unions, append rest
            result.append(content[i:])
            break

        start = match.start()
        result.append(content[i:start])  # keep text before struct/union

        # Now find the matching closing brace
        brace_count = 0
        j = match.end() - 1
        while j < len(content):
            if content[j] == '{':
                brace_count += 1
            elif content[j] == '}':
                brace_count -= 1
                if brace_count == 0:
                    break
            j += 1

        # Extract name for forward declaration
        name = match.group(3)
        typedef_prefix = "typedef " if match.group(1) else ""
        if name:
            # Keep forward declaration
            result.append(f"{typedef_prefix}{match.group(2)} {name};\n")
        else:
            # Anonymous structs/unions are ignored
            pass

        i = j + 1  # continue after closing brace

    return ''.join(result)

def process_headers(header_dir):
    combined_content = ""
    for root, _, files in os.walk(header_dir):
        for file in files:
            if file.endswith((".h", ".hpp")):
                file_path = os.path.join(root, file)
                with open(file_path, "r", encoding="utf-8") as f:
                    content = f.read()
                    cleaned_content = remove_struct_union_bodies(content)
                    combined_content += f"// {file}\n{cleaned_content}\n\n"
    return combined_content

if __name__ == "__main__":
    combined = process_headers(HEADER_DIR)
    with open(OUTPUT_FILE, "w", encoding="utf-8") as out_file:
        out_file.write(combined)
    print(f"Combined header file created: {OUTPUT_FILE}")
