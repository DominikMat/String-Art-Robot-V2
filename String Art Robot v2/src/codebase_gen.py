import os

def consolidate_code(output_filename="string_robot_code.txt"):
    """
    Consolidates code files into one text file, skipping blacklisted items.
    """
    root_dir = os.getcwd()
    target_extensions = ('.h', '.cpp', '.py')

    # --- BLACKLIST CONFIGURATION ---
    blacklisted_dirs = { } 
    # Specific filenames to skip
    blacklisted_files = {"codebase_gen.py"} 
    # -------------------------------

    with open(output_filename, 'w', encoding='utf-8') as outfile:
        for dirpath, dirnames, filenames in os.walk(root_dir):
            
            # 1. Skip blacklisted directories
            # Modifying dirnames in-place tells os.walk to ignore these paths
            dirnames[:] = [d for d in dirnames if d.lower() not in blacklisted_dirs]

            # 2. Prevent the script from reading its own output
            if output_filename in filenames:
                filenames.remove(output_filename)

            for filename in filenames:
                # 3. Skip blacklisted files or files without target extension
                if filename.lower() in blacklisted_files:
                    continue

                if filename.lower().endswith(target_extensions):
                    filepath = os.path.join(dirpath, filename)
                    relative_path = os.path.relpath(filepath, root_dir)
                    
                    header = f"*** FILE: {relative_path} ***\n"
                    
                    outfile.write("=" * 70 + "\n")
                    outfile.write(header)
                    outfile.write("=" * 70 + "\n\n")

                    print(f"Adding: {relative_path}")
                    
                    try:
                        with open(filepath, 'r', encoding='utf-8') as infile:
                            content = infile.read()
                        
                        outfile.write(content)
                        outfile.write("\n\n") 
                        
                    except UnicodeDecodeError:
                        error_msg = f"\n!!! SKIPPING: Could not read {relative_path} (not UTF-8) !!!\n\n"
                        outfile.write(error_msg)
                        print(f"Skipped {relative_path} (encoding error)")
                    except Exception as e:
                        error_msg = f"\n!!! SKIPPING: Error reading {relative_path}: {e} !!!\n\n"
                        outfile.write(error_msg)
                        print(f"Skipped {relative_path} ({e})")

    print("\n" + "#" * 70)
    print(f"SUCCESS: Consolidated into '{output_filename}'.")
    print("#" * 70)

if __name__ == "__main__":
    consolidate_code()