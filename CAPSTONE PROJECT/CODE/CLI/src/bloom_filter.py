"""
Bloom Filter — Probabilistic Duplicate Pre-Screen
===================================================
Data Structure: Bloom Filter (bit array + multiple hash functions)

A space-efficient probabilistic data structure used to quickly test whether a
file hash has been "seen before". It can produce false positives (says
"possibly duplicate" when the file is unique) but NEVER false negatives
(if it says "definitely not seen", the file is unique).

Use-case in this project:
    Before computing a full SHA-256 hash (expensive I/O), we check a Bloom
    filter keyed on (file_size, first_4KB_hash). If the Bloom filter says
    "not seen", we skip the file immediately — guaranteed no duplicate.

Time Complexity:
    - Insert : O(k)  — k = number of hash functions
    - Query  : O(k)

Space Complexity: O(m) bits, where m = size of the bit array

False-positive rate ≈ (1 - e^(-kn/m))^k
    where n = number of inserted elements
"""

from __future__ import annotations
import math
import hashlib
from typing import Optional


class BloomFilter:
    """
    A Bloom filter that uses double-hashing to simulate k independent
    hash functions.

    Parameters:
        expected_items: Expected number of items to insert.
        fp_rate: Desired false positive rate (default 1%).
    """

    def __init__(self, expected_items: int = 10_000, fp_rate: float = 0.01):
        if expected_items <= 0:
            raise ValueError("expected_items must be positive")
        if not (0 < fp_rate < 1):
            raise ValueError("fp_rate must be between 0 and 1")

        # Calculate optimal bit-array size  m = -n*ln(p) / (ln2)^2
        self._m = self._optimal_size(expected_items, fp_rate)
        # Calculate optimal number of hash functions  k = (m/n) * ln2
        self._k = self._optimal_hash_count(self._m, expected_items)
        # Bit array stored as a bytearray
        self._bits = bytearray(math.ceil(self._m / 8))
        self._count = 0

        self.expected_items = expected_items
        self.fp_rate = fp_rate

    # ------------------------------------------------------------------ #
    #  Optimal parameter calculations                                      #
    # ------------------------------------------------------------------ #
    @staticmethod
    def _optimal_size(n: int, p: float) -> int:
        """Compute optimal bit-array size for n items and FP rate p."""
        m = -(n * math.log(p)) / (math.log(2) ** 2)
        return int(math.ceil(m))

    @staticmethod
    def _optimal_hash_count(m: int, n: int) -> int:
        """Compute optimal number of hash functions."""
        k = (m / n) * math.log(2)
        return max(1, int(round(k)))

    # ------------------------------------------------------------------ #
    #  Double hashing to generate k positions                              #
    # ------------------------------------------------------------------ #
    def _get_positions(self, item: str) -> list[int]:
        """
        Generate k bit positions using double-hashing:

            position_i = (hash1 + i * hash2) mod m

        hash1 = lower 64 bits of MD5(item)
        hash2 = upper 64 bits of MD5(item)

        This avoids computing k independent hashes.
        """
        digest = hashlib.md5(item.encode("utf-8")).hexdigest()
        h1 = int(digest[:16], 16)  # first 64 bits
        h2 = int(digest[16:], 16)  # last 64 bits

        positions = []
        for i in range(self._k):
            pos = (h1 + i * h2) % self._m
            positions.append(pos)
        return positions

    # ------------------------------------------------------------------ #
    #  Bit manipulation                                                    #
    # ------------------------------------------------------------------ #
    def _set_bit(self, pos: int) -> None:
        byte_idx = pos // 8
        bit_idx = pos % 8
        self._bits[byte_idx] |= (1 << bit_idx)

    def _get_bit(self, pos: int) -> bool:
        byte_idx = pos // 8
        bit_idx = pos % 8
        return bool(self._bits[byte_idx] & (1 << bit_idx))

    # ------------------------------------------------------------------ #
    #  Public API                                                          #
    # ------------------------------------------------------------------ #
    def add(self, item: str) -> None:
        """Add an item to the Bloom filter."""
        for pos in self._get_positions(item):
            self._set_bit(pos)
        self._count += 1

    def might_contain(self, item: str) -> bool:
        """
        Test if an item MIGHT be in the set.

        Returns:
            True  -> item is POSSIBLY in the set (could be false positive)
            False -> item is DEFINITELY NOT in the set
        """
        return all(self._get_bit(pos) for pos in self._get_positions(item))

    # ------------------------------------------------------------------ #
    #  Diagnostics                                                         #
    # ------------------------------------------------------------------ #
    @property
    def count(self) -> int:
        """Number of items added."""
        return self._count

    @property
    def size_bits(self) -> int:
        """Size of the bit array in bits."""
        return self._m

    @property
    def num_hash_functions(self) -> int:
        return self._k

    def estimated_fp_rate(self) -> float:
        """Estimate the current false positive rate given items inserted."""
        if self._count == 0:
            return 0.0
        return (1 - math.exp(-self._k * self._count / self._m)) ** self._k

    def fill_ratio(self) -> float:
        """Fraction of bits set to 1."""
        set_bits = sum(bin(byte).count("1") for byte in self._bits)
        return set_bits / self._m

    def __repr__(self) -> str:
        return (
            f"BloomFilter(items={self._count}, bits={self._m}, "
            f"k={self._k}, est_fp={self.estimated_fp_rate():.4%})"
        )

    def __contains__(self, item: str) -> bool:
        return self.might_contain(item)
