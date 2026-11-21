"""
Custom Hash Table Implementation
=================================
Data Structure: Hash Table with Separate Chaining

This module implements a hash table from scratch (without using Python's dict)
to demonstrate the core mechanism behind grouping files by their hash digest.

Time Complexity:
    - Insert : O(1) average, O(n) worst-case (all keys collide)
    - Search : O(1) average, O(n) worst-case
    - Delete : O(1) average, O(n) worst-case

Space Complexity: O(n + m), where n = number of entries, m = number of buckets

Collision Resolution: Separate Chaining using singly-linked lists.
Dynamic Resizing: Table doubles when load factor exceeds 0.75.
"""

from __future__ import annotations
from typing import Any, Generator, Optional


class _Node:
    """A node in the singly-linked chain for separate chaining."""

    __slots__ = ("key", "value", "next")

    def __init__(self, key: str, value: Any, next_node: Optional["_Node"] = None):
        self.key = key
        self.value = value
        self.next = next_node

    def __repr__(self) -> str:
        return f"_Node({self.key!r}, {self.value!r})"


class HashTable:
    """
    A hash table that maps string keys to arbitrary values using separate
    chaining for collision resolution.

    This is used in the duplicate detector to map:
        file_hash  ->  list[file_path]
        file_size  ->  list[file_path]

    Attributes:
        _capacity: Current number of buckets.
        _size: Number of key-value pairs stored.
        _buckets: Array (list) of linked-list heads.
    """

    _INITIAL_CAPACITY = 16
    _LOAD_FACTOR_THRESHOLD = 0.75

    def __init__(self, capacity: int = _INITIAL_CAPACITY):
        if capacity < 1:
            raise ValueError("Capacity must be at least 1")
        self._capacity: int = capacity
        self._size: int = 0
        self._buckets: list[Optional[_Node]] = [None] * self._capacity

    # ------------------------------------------------------------------ #
    #  Core hash function                                                  #
    # ------------------------------------------------------------------ #
    def _hash(self, key: str) -> int:
        """
        DJB2 hash function (Daniel J. Bernstein).

        Algorithm:
            hash = 5381
            for each character c in key:
                hash = hash * 33 + ord(c)
            return hash mod capacity

        This is a well-known non-cryptographic hash with good distribution.
        """
        h = 5381
        for ch in key:
            h = ((h << 5) + h) + ord(ch)  # h * 33 + ord(ch)
        return h % self._capacity

    # ------------------------------------------------------------------ #
    #  Public API                                                          #
    # ------------------------------------------------------------------ #
    def put(self, key: str, value: Any) -> None:
        """Insert or update a key-value pair.  O(1) amortised."""
        idx = self._hash(key)
        node = self._buckets[idx]

        # Walk the chain to check for existing key
        while node is not None:
            if node.key == key:
                node.value = value  # update
                return
            node = node.next

        # Key not found — prepend a new node (O(1))
        new_node = _Node(key, value, self._buckets[idx])
        self._buckets[idx] = new_node
        self._size += 1

        # Resize if load factor exceeded
        if self._size / self._capacity > self._LOAD_FACTOR_THRESHOLD:
            self._resize()

    def get(self, key: str, default: Any = None) -> Any:
        """Retrieve value by key.  O(1) average."""
        idx = self._hash(key)
        node = self._buckets[idx]
        while node is not None:
            if node.key == key:
                return node.value
            node = node.next
        return default

    def contains(self, key: str) -> bool:
        """Check if a key exists.  O(1) average."""
        return self.get(key, _SENTINEL) is not _SENTINEL

    def delete(self, key: str) -> bool:
        """Remove a key.  Returns True if the key was present."""
        idx = self._hash(key)
        node = self._buckets[idx]
        prev = None
        while node is not None:
            if node.key == key:
                if prev is None:
                    self._buckets[idx] = node.next
                else:
                    prev.next = node.next
                self._size -= 1
                return True
            prev = node
            node = node.next
        return False

    def setdefault(self, key: str, default: Any) -> Any:
        """Return value if key exists, otherwise insert default and return it."""
        val = self.get(key, _SENTINEL)
        if val is _SENTINEL:
            self.put(key, default)
            return default
        return val

    # ------------------------------------------------------------------ #
    #  Iteration                                                           #
    # ------------------------------------------------------------------ #
    def keys(self) -> Generator[str, None, None]:
        for bucket in self._buckets:
            node = bucket
            while node is not None:
                yield node.key
                node = node.next

    def values(self) -> Generator[Any, None, None]:
        for bucket in self._buckets:
            node = bucket
            while node is not None:
                yield node.value
                node = node.next

    def items(self) -> Generator[tuple[str, Any], None, None]:
        for bucket in self._buckets:
            node = bucket
            while node is not None:
                yield (node.key, node.value)
                node = node.next

    # ------------------------------------------------------------------ #
    #  Resizing                                                            #
    # ------------------------------------------------------------------ #
    def _resize(self) -> None:
        """Double the capacity and rehash all entries."""
        old_buckets = self._buckets
        self._capacity *= 2
        self._buckets = [None] * self._capacity
        self._size = 0
        for bucket in old_buckets:
            node = bucket
            while node is not None:
                self.put(node.key, node.value)
                node = node.next

    # ------------------------------------------------------------------ #
    #  Diagnostics                                                         #
    # ------------------------------------------------------------------ #
    def load_factor(self) -> float:
        return self._size / self._capacity

    def bucket_distribution(self) -> list[int]:
        """Return the chain length of every bucket (useful for analysis)."""
        dist = []
        for bucket in self._buckets:
            length = 0
            node = bucket
            while node is not None:
                length += 1
                node = node.next
            dist.append(length)
        return dist

    # ------------------------------------------------------------------ #
    #  Dunder methods                                                      #
    # ------------------------------------------------------------------ #
    def __len__(self) -> int:
        return self._size

    def __contains__(self, key: str) -> bool:
        return self.contains(key)

    def __getitem__(self, key: str) -> Any:
        val = self.get(key, _SENTINEL)
        if val is _SENTINEL:
            raise KeyError(key)
        return val

    def __setitem__(self, key: str, value: Any) -> None:
        self.put(key, value)

    def __repr__(self) -> str:
        pairs = ", ".join(f"{k!r}: {v!r}" for k, v in self.items())
        return f"HashTable({{{pairs}}})"


# Sentinel object to distinguish "key not found" from "value is None"
_SENTINEL = object()
