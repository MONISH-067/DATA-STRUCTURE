"""
File Hashing Utilities
=======================
Algorithms: MD5, SHA-256 (cryptographic hashing for file fingerprinting)

Provides functions to compute file hashes with configurable chunk sizes.
Implements a two-phase hashing strategy:

    Phase 1 — Quick Hash : Hash only the first 4 KB of the file.
              Files that differ in the first 4 KB are guaranteed non-duplicates.
    Phase 2 — Full Hash  : Hash the entire file contents.
              Only computed for files whose quick hashes match.

This dramatically reduces I/O for large directories with few actual duplicates.
"""

from __future__ import annotations
import hashlib
import os
from typing import Optional

# Default chunk size for reading files (64 KB)
CHUNK_SIZE = 65_536

# Size of the "quick hash" prefix (4 KB)
QUICK_HASH_SIZE = 4_096


def compute_full_hash(file_path: str, algorithm: str = "sha256") -> str:
    """
    Compute the full cryptographic hash of a file.

    Reads the file in chunks to handle files larger than available RAM.

    Args:
        file_path: Path to the file.
        algorithm: Hash algorithm name (default: sha256).

    Returns:
        Hex digest string.

    Time Complexity: O(n) where n = file size
    Space Complexity: O(1) — only one chunk in memory at a time
    """
    hasher = hashlib.new(algorithm)
    try:
        with open(file_path, "rb") as f:
            while True:
                chunk = f.read(CHUNK_SIZE)
                if not chunk:
                    break
                hasher.update(chunk)
    except (OSError, PermissionError) as e:
        raise FileHashError(f"Cannot hash {file_path}: {e}") from e
    return hasher.hexdigest()


def compute_quick_hash(file_path: str, algorithm: str = "sha256") -> str:
    """
    Compute a hash of only the first QUICK_HASH_SIZE bytes.

    This is a fast pre-filter: if two files differ in their first 4 KB,
    they are definitely not duplicates. This avoids reading the entire
    file for the common case.

    Args:
        file_path: Path to the file.
        algorithm: Hash algorithm name.

    Returns:
        Hex digest of the first 4 KB.
    """
    hasher = hashlib.new(algorithm)
    try:
        with open(file_path, "rb") as f:
            data = f.read(QUICK_HASH_SIZE)
            hasher.update(data)
    except (OSError, PermissionError) as e:
        raise FileHashError(f"Cannot hash {file_path}: {e}") from e
    return hasher.hexdigest()


def compute_combined_key(file_path: str) -> str:
    """
    Create a combined key from file size + quick hash.

    This is used as the Bloom filter key for fast pre-screening.
    """
    try:
        size = os.path.getsize(file_path)
    except OSError:
        size = 0
    quick = compute_quick_hash(file_path)
    return f"{size}:{quick}"


class FileHashError(Exception):
    """Raised when a file cannot be hashed."""
    pass
