"""
Sorting Algorithms for Duplicate Reports
==========================================
Algorithms: Merge Sort, Quick Sort (with median-of-three pivot)

These are used to sort duplicate groups by various criteria:
    - By wasted space (descending) to prioritise cleanup
    - By file count (descending) to find most-duplicated files
    - By file name (alphabetical) for readable reports

Time Complexity:
    - Merge Sort : O(n log n) — stable, guaranteed
    - Quick Sort : O(n log n) average, O(n²) worst-case
                   (mitigated by median-of-three pivot selection)

Space Complexity:
    - Merge Sort : O(n) auxiliary
    - Quick Sort : O(log n) auxiliary (in-place partitioning)
"""

from __future__ import annotations
from typing import Any, Callable, Optional, Sequence


# ====================================================================== #
#  Merge Sort (Stable)                                                     #
# ====================================================================== #
def merge_sort(
    arr: list[Any],
    key: Callable[[Any], Any] = lambda x: x,
    reverse: bool = False,
) -> list[Any]:
    """
    Stable merge sort.

    Algorithm:
        1. Recursively split the list in half until single elements.
        2. Merge sorted halves by comparing elements in order.

    Parameters:
        arr: List to sort.
        key: Function to extract a comparison key.
        reverse: If True, sort in descending order.

    Returns:
        A new sorted list.
    """
    if len(arr) <= 1:
        return arr[:]

    mid = len(arr) // 2
    left = merge_sort(arr[:mid], key=key, reverse=reverse)
    right = merge_sort(arr[mid:], key=key, reverse=reverse)

    return _merge(left, right, key, reverse)


def _merge(
    left: list[Any],
    right: list[Any],
    key: Callable[[Any], Any],
    reverse: bool,
) -> list[Any]:
    """Merge two sorted lists into one sorted list."""
    result = []
    i = j = 0
    while i < len(left) and j < len(right):
        lk, rk = key(left[i]), key(right[j])
        if (lk >= rk) if reverse else (lk <= rk):
            result.append(left[i])
            i += 1
        else:
            result.append(right[j])
            j += 1
    result.extend(left[i:])
    result.extend(right[j:])
    return result


# ====================================================================== #
#  Quick Sort (In-place, Median-of-Three)                                  #
# ====================================================================== #
def quick_sort(
    arr: list[Any],
    key: Callable[[Any], Any] = lambda x: x,
    reverse: bool = False,
) -> list[Any]:
    """
    Quick sort with median-of-three pivot selection to avoid worst-case
    on already-sorted data.

    Algorithm:
        1. Choose pivot as median of first, middle, last elements.
        2. Partition array around pivot (Lomuto scheme variant).
        3. Recursively sort left and right partitions.

    Returns:
        A new sorted list (the input is not modified).
    """
    data = arr[:]
    _quick_sort_inplace(data, 0, len(data) - 1, key, reverse)
    return data


def _quick_sort_inplace(
    arr: list[Any],
    low: int,
    high: int,
    key: Callable[[Any], Any],
    reverse: bool,
) -> None:
    """Recursive in-place quicksort on arr[low..high]."""
    if low < high:
        # Use insertion sort for small sub-arrays (optimisation)
        if high - low < 16:
            _insertion_sort(arr, low, high, key, reverse)
            return

        pivot_idx = _partition(arr, low, high, key, reverse)
        _quick_sort_inplace(arr, low, pivot_idx - 1, key, reverse)
        _quick_sort_inplace(arr, pivot_idx + 1, high, key, reverse)


def _median_of_three(
    arr: list[Any], low: int, high: int, key: Callable[[Any], Any]
) -> int:
    """Return the index of the median of arr[low], arr[mid], arr[high]."""
    mid = (low + high) // 2
    a, b, c = key(arr[low]), key(arr[mid]), key(arr[high])
    if a <= b <= c or c <= b <= a:
        return mid
    elif b <= a <= c or c <= a <= b:
        return low
    else:
        return high


def _partition(
    arr: list[Any],
    low: int,
    high: int,
    key: Callable[[Any], Any],
    reverse: bool,
) -> int:
    """Lomuto partition with median-of-three pivot."""
    pivot_idx = _median_of_three(arr, low, high, key)
    arr[pivot_idx], arr[high] = arr[high], arr[pivot_idx]
    pivot_val = key(arr[high])

    i = low
    for j in range(low, high):
        cmp = key(arr[j]) >= pivot_val if reverse else key(arr[j]) <= pivot_val
        if cmp:
            arr[i], arr[j] = arr[j], arr[i]
            i += 1
    arr[i], arr[high] = arr[high], arr[i]
    return i


def _insertion_sort(
    arr: list[Any],
    low: int,
    high: int,
    key: Callable[[Any], Any],
    reverse: bool,
) -> None:
    """Insertion sort for small sub-arrays (used as quicksort optimisation)."""
    for i in range(low + 1, high + 1):
        temp = arr[i]
        temp_key = key(temp)
        j = i - 1
        while j >= low:
            cmp = key(arr[j]) < temp_key if reverse else key(arr[j]) > temp_key
            if cmp:
                arr[j + 1] = arr[j]
                j -= 1
            else:
                break
        arr[j + 1] = temp
