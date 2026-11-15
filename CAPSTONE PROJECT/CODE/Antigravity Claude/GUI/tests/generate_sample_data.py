"""
Sample Data Generator — Creates test directories with duplicate files
for demonstration and benchmarking purposes.
"""

import os
import sys
import random
import string
import shutil

# Add project root to path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def generate_random_content(size: int) -> bytes:
    """Generate random bytes of a given size."""
    return bytes(random.getrandbits(8) for _ in range(size))


def create_sample_data(base_dir: str, num_unique: int = 20, max_copies: int = 4):
    """
    Create a sample directory tree with known duplicate files.

    Args:
        base_dir: Directory to create test files in.
        num_unique: Number of unique file contents.
        max_copies: Maximum number of copies per duplicate.
    """
    if os.path.exists(base_dir):
        shutil.rmtree(base_dir)

    # Create subdirectories
    subdirs = [
        os.path.join(base_dir, "documents"),
        os.path.join(base_dir, "documents", "reports"),
        os.path.join(base_dir, "documents", "drafts"),
        os.path.join(base_dir, "photos"),
        os.path.join(base_dir, "photos", "vacation"),
        os.path.join(base_dir, "photos", "work"),
        os.path.join(base_dir, "downloads"),
        os.path.join(base_dir, "backups"),
        os.path.join(base_dir, "backups", "old"),
    ]
    for d in subdirs:
        os.makedirs(d, exist_ok=True)

    extensions = [".txt", ".pdf", ".jpg", ".png", ".docx", ".xlsx", ".py", ".log"]
    sizes = [100, 512, 1024, 4096, 8192, 16384, 32768, 65536]

    total_files = 0
    total_duplicates = 0
    manifest = []  # Track what we created

    for i in range(num_unique):
        size = random.choice(sizes)
        content = generate_random_content(size)
        ext = random.choice(extensions)

        # Decide how many copies (1 = unique, 2+ = duplicates)
        num_copies = random.randint(1, max_copies)
        chosen_dirs = random.sample(subdirs, min(num_copies, len(subdirs)))

        names_used = set()
        for j, target_dir in enumerate(chosen_dirs):
            name = f"file_{i:03d}_copy{j}{ext}"
            if j == 0:
                name = f"file_{i:03d}{ext}"
            filepath = os.path.join(target_dir, name)
            with open(filepath, "wb") as f:
                f.write(content)
            names_used.add(filepath)
            total_files += 1

        if num_copies > 1:
            total_duplicates += num_copies - 1
            manifest.append({
                "content_id": i,
                "size": size,
                "copies": list(names_used),
            })

    # Write manifest
    manifest_path = os.path.join(base_dir, "_manifest.txt")
    with open(manifest_path, "w") as f:
        f.write("SAMPLE DATA MANIFEST\n")
        f.write("=" * 50 + "\n")
        f.write(f"Total files: {total_files}\n")
        f.write(f"Unique contents: {num_unique}\n")
        f.write(f"Duplicate files: {total_duplicates}\n\n")
        for entry in manifest:
            f.write(f"Content {entry['content_id']} ({entry['size']} bytes):\n")
            for p in entry['copies']:
                f.write(f"  - {p}\n")
            f.write("\n")

    print(f"[OK] Created {total_files} files ({total_duplicates} duplicates) in {base_dir}")
    print(f"[INFO] Manifest: {manifest_path}")
    return base_dir


if __name__ == "__main__":
    sample_dir = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "sample_files", "generated"
    )
    create_sample_data(sample_dir, num_unique=25, max_copies=4)
