"""
Min-Heap / Max-Heap (Priority Queue)
======================================
Data Structure: Binary Heap (array-based)

Used in the duplicate detector to efficiently retrieve the Top-K largest
duplicate groups by wasted disk space.

Time Complexity:
    - Insert (push)      : O(log n)
    - Extract min/max    : O(log n)
    - Peek               : O(1)
    - Heapify (build)    : O(n)

Space Complexity: O(n)
"""

from __future__ import annotations
from typing import Any, Callable, Optional


class BinaryHeap:
    """
    A binary heap implementation that can act as a min-heap or max-heap.

    Parameters:
        key: A function that extracts a comparison key from each element.
             Defaults to identity (the element itself).
        max_heap: If True, the largest element is at the root.
                  If False (default), the smallest element is at the root.
    """

    def __init__(
        self,
        key: Callable[[Any], Any] = lambda x: x,
        max_heap: bool = False,
    ):
        self._data: list[Any] = []
        self._key = key
        self._max_heap = max_heap

    # ------------------------------------------------------------------ #
    #  Comparison helper                                                   #
    # ------------------------------------------------------------------ #
    def _compare(self, a: Any, b: Any) -> bool:
        """Return True if a should be closer to the root than b."""
        ka, kb = self._key(a), self._key(b)
        if self._max_heap:
            return ka > kb
        return ka < kb

    # ------------------------------------------------------------------ #
    #  Core operations                                                     #
    # ------------------------------------------------------------------ #
    def push(self, item: Any) -> None:
        """Add an item and restore heap property.  O(log n)."""
        self._data.append(item)
        self._sift_up(len(self._data) - 1)

    def pop(self) -> Any:
        """Remove and return the root element.  O(log n)."""
        if not self._data:
            raise IndexError("pop from empty heap")
        root = self._data[0]
        last = self._data.pop()
        if self._data:
            self._data[0] = last
            self._sift_down(0)
        return root

    def peek(self) -> Any:
        """Return the root element without removing it.  O(1)."""
        if not self._data:
            raise IndexError("peek on empty heap")
        return self._data[0]

    def push_pop(self, item: Any) -> Any:
        """Push an item and immediately pop the root.  O(log n)."""
        if self._data and self._compare(self._data[0], item):
            item, self._data[0] = self._data[0], item
            self._sift_down(0)
        return item

    # ------------------------------------------------------------------ #
    #  Heapify (build from iterable)                                       #
    # ------------------------------------------------------------------ #
    @classmethod
    def heapify(
        cls,
        iterable,
        key: Callable[[Any], Any] = lambda x: x,
        max_heap: bool = False,
    ) -> "BinaryHeap":
        """
        Build a heap from an iterable in O(n) time using bottom-up
        sift-down (Floyd's algorithm).
        """
        heap = cls(key=key, max_heap=max_heap)
        heap._data = list(iterable)
        # Start from the last internal node and sift down
        for i in range(len(heap._data) // 2 - 1, -1, -1):
            heap._sift_down(i)
        return heap

    # ------------------------------------------------------------------ #
    #  Top-K extraction                                                    #
    # ------------------------------------------------------------------ #
    def top_k(self, k: int) -> list[Any]:
        """
        Extract the top-k elements (by priority) without modifying the heap.
        Uses a temporary copy.  O(k log n).
        """
        tmp = BinaryHeap(key=self._key, max_heap=self._max_heap)
        tmp._data = self._data.copy()
        result = []
        for _ in range(min(k, len(tmp))):
            result.append(tmp.pop())
        return result

    # ------------------------------------------------------------------ #
    #  Sift operations                                                     #
    # ------------------------------------------------------------------ #
    def _sift_up(self, idx: int) -> None:
        """Bubble an element up to restore heap property."""
        while idx > 0:
            parent = (idx - 1) // 2
            if self._compare(self._data[idx], self._data[parent]):
                self._data[idx], self._data[parent] = (
                    self._data[parent],
                    self._data[idx],
                )
                idx = parent
            else:
                break

    def _sift_down(self, idx: int) -> None:
        """Push an element down to restore heap property."""
        n = len(self._data)
        while True:
            target = idx
            left = 2 * idx + 1
            right = 2 * idx + 2

            if left < n and self._compare(self._data[left], self._data[target]):
                target = left
            if right < n and self._compare(self._data[right], self._data[target]):
                target = right

            if target == idx:
                break
            self._data[idx], self._data[target] = (
                self._data[target],
                self._data[idx],
            )
            idx = target

    # ------------------------------------------------------------------ #
    #  Dunder methods                                                      #
    # ------------------------------------------------------------------ #
    def __len__(self) -> int:
        return len(self._data)

    def __bool__(self) -> bool:
        return bool(self._data)

    def __repr__(self) -> str:
        kind = "Max" if self._max_heap else "Min"
        return f"{kind}Heap(size={len(self._data)})"
