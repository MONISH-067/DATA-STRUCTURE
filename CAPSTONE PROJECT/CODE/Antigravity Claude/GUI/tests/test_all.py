"""
Test Suite — Unit tests for all data structures and algorithms.
Validates correctness of each DSA component independently.
"""

from __future__ import annotations
import os
import sys
import tempfile
import shutil
import unittest

# Add project root to path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from src.hash_table import HashTable
from src.bloom_filter import BloomFilter
from src.heap import BinaryHeap
from src.trie import DirectoryTrie
from src.sorting import merge_sort, quick_sort
from src.hashing import compute_full_hash, compute_quick_hash
from src.walker import DirectoryWalker, ScanConfig, FileInfo
from src.detector import DuplicateDetector


# ====================================================================== #
#  Test Hash Table                                                         #
# ====================================================================== #
class TestHashTable(unittest.TestCase):
    """Tests for the custom Hash Table with separate chaining."""

    def test_put_and_get(self):
        ht = HashTable()
        ht.put("key1", "value1")
        ht.put("key2", "value2")
        self.assertEqual(ht.get("key1"), "value1")
        self.assertEqual(ht.get("key2"), "value2")

    def test_update_existing_key(self):
        ht = HashTable()
        ht.put("key", "old")
        ht.put("key", "new")
        self.assertEqual(ht.get("key"), "new")
        self.assertEqual(len(ht), 1)

    def test_get_missing_key(self):
        ht = HashTable()
        self.assertIsNone(ht.get("missing"))
        self.assertEqual(ht.get("missing", "default"), "default")

    def test_contains(self):
        ht = HashTable()
        ht.put("exists", 42)
        self.assertTrue(ht.contains("exists"))
        self.assertFalse(ht.contains("ghost"))
        self.assertIn("exists", ht)

    def test_delete(self):
        ht = HashTable()
        ht.put("a", 1)
        ht.put("b", 2)
        self.assertTrue(ht.delete("a"))
        self.assertFalse(ht.contains("a"))
        self.assertEqual(len(ht), 1)
        self.assertFalse(ht.delete("nonexistent"))

    def test_setdefault(self):
        ht = HashTable()
        val = ht.setdefault("k", [])
        self.assertEqual(val, [])
        val.append(1)
        self.assertEqual(ht.get("k"), [1])
        val2 = ht.setdefault("k", [99])
        self.assertEqual(val2, [1])  # existing value returned

    def test_iteration(self):
        ht = HashTable()
        expected = {"a": 1, "b": 2, "c": 3}
        for k, v in expected.items():
            ht.put(k, v)
        self.assertEqual(set(ht.keys()), {"a", "b", "c"})
        self.assertEqual(set(ht.values()), {1, 2, 3})
        self.assertEqual(dict(ht.items()), expected)

    def test_resize(self):
        ht = HashTable(capacity=4)
        for i in range(20):
            ht.put(f"key{i}", i)
        self.assertEqual(len(ht), 20)
        for i in range(20):
            self.assertEqual(ht.get(f"key{i}"), i)

    def test_dunder_getitem_setitem(self):
        ht = HashTable()
        ht["x"] = 10
        self.assertEqual(ht["x"], 10)
        with self.assertRaises(KeyError):
            _ = ht["nonexistent"]

    def test_load_factor(self):
        ht = HashTable(capacity=16)
        for i in range(8):
            ht.put(str(i), i)
        self.assertAlmostEqual(ht.load_factor(), 8 / 16)


# ====================================================================== #
#  Test Bloom Filter                                                       #
# ====================================================================== #
class TestBloomFilter(unittest.TestCase):
    """Tests for the Bloom Filter probabilistic data structure."""

    def test_no_false_negatives(self):
        """Items that were added must ALWAYS be reported as present."""
        bf = BloomFilter(expected_items=1000, fp_rate=0.01)
        items = [f"file_{i}.txt" for i in range(500)]
        for item in items:
            bf.add(item)
        for item in items:
            self.assertTrue(bf.might_contain(item),
                            f"False negative for {item}")

    def test_low_false_positive_rate(self):
        """False positive rate should be close to the configured rate."""
        bf = BloomFilter(expected_items=10000, fp_rate=0.01)
        for i in range(10000):
            bf.add(f"inserted_{i}")

        false_positives = 0
        test_count = 10000
        for i in range(test_count):
            if bf.might_contain(f"not_inserted_{i}"):
                false_positives += 1

        fp_rate = false_positives / test_count
        self.assertLess(fp_rate, 0.03,
                        f"FP rate {fp_rate:.2%} too high (expected ~1%)")

    def test_empty_filter(self):
        bf = BloomFilter(expected_items=100)
        self.assertFalse(bf.might_contain("anything"))
        self.assertEqual(bf.count, 0)

    def test_contains_operator(self):
        bf = BloomFilter(expected_items=100)
        bf.add("hello")
        self.assertIn("hello", bf)

    def test_diagnostics(self):
        bf = BloomFilter(expected_items=100, fp_rate=0.05)
        self.assertGreater(bf.size_bits, 0)
        self.assertGreater(bf.num_hash_functions, 0)


# ====================================================================== #
#  Test Binary Heap                                                        #
# ====================================================================== #
class TestBinaryHeap(unittest.TestCase):
    """Tests for the Binary Heap (Priority Queue)."""

    def test_min_heap(self):
        heap = BinaryHeap()
        for val in [5, 3, 8, 1, 9, 2]:
            heap.push(val)
        result = []
        while heap:
            result.append(heap.pop())
        self.assertEqual(result, [1, 2, 3, 5, 8, 9])

    def test_max_heap(self):
        heap = BinaryHeap(max_heap=True)
        for val in [5, 3, 8, 1, 9, 2]:
            heap.push(val)
        result = []
        while heap:
            result.append(heap.pop())
        self.assertEqual(result, [9, 8, 5, 3, 2, 1])

    def test_custom_key(self):
        data = [("a", 3), ("b", 1), ("c", 2)]
        heap = BinaryHeap(key=lambda x: x[1])
        for item in data:
            heap.push(item)
        self.assertEqual(heap.pop(), ("b", 1))

    def test_heapify(self):
        data = [5, 3, 8, 1, 9, 2]
        heap = BinaryHeap.heapify(data)
        result = []
        while heap:
            result.append(heap.pop())
        self.assertEqual(result, sorted(data))

    def test_peek(self):
        heap = BinaryHeap()
        heap.push(10)
        heap.push(5)
        self.assertEqual(heap.peek(), 5)
        self.assertEqual(len(heap), 2)  # peek doesn't remove

    def test_top_k(self):
        heap = BinaryHeap.heapify(range(100), max_heap=True)
        top5 = heap.top_k(5)
        self.assertEqual(top5, [99, 98, 97, 96, 95])

    def test_empty_heap_raises(self):
        heap = BinaryHeap()
        with self.assertRaises(IndexError):
            heap.pop()
        with self.assertRaises(IndexError):
            heap.peek()


# ====================================================================== #
#  Test Trie                                                               #
# ====================================================================== #
class TestDirectoryTrie(unittest.TestCase):
    """Tests for the Directory Trie (Prefix Tree)."""

    def setUp(self):
        self.trie = DirectoryTrie()
        self.temp_dir = tempfile.mkdtemp()
        # Create test directory structure
        os.makedirs(os.path.join(self.temp_dir, "docs", "reports"), exist_ok=True)
        os.makedirs(os.path.join(self.temp_dir, "docs", "images"), exist_ok=True)
        os.makedirs(os.path.join(self.temp_dir, "code"), exist_ok=True)

        self.files = []
        for subpath in ["docs/a.txt", "docs/b.txt", "docs/reports/r1.pdf",
                         "docs/images/img.png", "code/main.py"]:
            fp = os.path.join(self.temp_dir, subpath)
            with open(fp, "w") as f:
                f.write("test")
            self.files.append(fp)
            self.trie.insert(fp)

    def tearDown(self):
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_total_files(self):
        self.assertEqual(self.trie.total_files(), 5)

    def test_search_prefix(self):
        docs_path = os.path.join(self.temp_dir, "docs")
        results = self.trie.search_prefix(docs_path)
        self.assertEqual(len(results), 4)  # a.txt, b.txt, r1.pdf, img.png

    def test_count_under(self):
        docs_path = os.path.join(self.temp_dir, "docs")
        self.assertEqual(self.trie.count_under(docs_path), 4)

    def test_depth(self):
        self.assertGreater(self.trie.depth(), 0)


# ====================================================================== #
#  Test Sorting Algorithms                                                 #
# ====================================================================== #
class TestSorting(unittest.TestCase):
    """Tests for Merge Sort and Quick Sort."""

    def test_merge_sort_ascending(self):
        data = [5, 2, 8, 1, 9, 3]
        self.assertEqual(merge_sort(data), [1, 2, 3, 5, 8, 9])

    def test_merge_sort_descending(self):
        data = [5, 2, 8, 1, 9, 3]
        self.assertEqual(merge_sort(data, reverse=True), [9, 8, 5, 3, 2, 1])

    def test_merge_sort_custom_key(self):
        data = [("b", 2), ("a", 3), ("c", 1)]
        result = merge_sort(data, key=lambda x: x[1])
        self.assertEqual(result, [("c", 1), ("b", 2), ("a", 3)])

    def test_merge_sort_stability(self):
        """Merge sort is stable: equal elements preserve original order."""
        data = [(1, "a"), (2, "b"), (1, "c"), (2, "d")]
        result = merge_sort(data, key=lambda x: x[0])
        self.assertEqual(result, [(1, "a"), (1, "c"), (2, "b"), (2, "d")])

    def test_merge_sort_empty_and_single(self):
        self.assertEqual(merge_sort([]), [])
        self.assertEqual(merge_sort([42]), [42])

    def test_quick_sort_ascending(self):
        data = [5, 2, 8, 1, 9, 3]
        self.assertEqual(quick_sort(data), [1, 2, 3, 5, 8, 9])

    def test_quick_sort_descending(self):
        data = [5, 2, 8, 1, 9, 3]
        self.assertEqual(quick_sort(data, reverse=True), [9, 8, 5, 3, 2, 1])

    def test_quick_sort_already_sorted(self):
        data = list(range(50))
        self.assertEqual(quick_sort(data), data)

    def test_quick_sort_reverse_sorted(self):
        data = list(range(50, 0, -1))
        self.assertEqual(quick_sort(data), sorted(data))

    def test_quick_sort_duplicates(self):
        data = [3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5]
        self.assertEqual(quick_sort(data), sorted(data))

    def test_quick_sort_large(self):
        import random
        data = random.sample(range(10_000), 1_000)
        self.assertEqual(quick_sort(data), sorted(data))


# ====================================================================== #
#  Test File Hashing                                                       #
# ====================================================================== #
class TestFileHashing(unittest.TestCase):
    """Tests for file hashing functions."""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_identical_files_same_hash(self):
        content = b"Hello, World! This is a test file."
        f1 = os.path.join(self.temp_dir, "file1.txt")
        f2 = os.path.join(self.temp_dir, "file2.txt")
        for f in (f1, f2):
            with open(f, "wb") as fp:
                fp.write(content)
        self.assertEqual(compute_full_hash(f1), compute_full_hash(f2))

    def test_different_files_different_hash(self):
        f1 = os.path.join(self.temp_dir, "a.txt")
        f2 = os.path.join(self.temp_dir, "b.txt")
        with open(f1, "wb") as fp:
            fp.write(b"content A")
        with open(f2, "wb") as fp:
            fp.write(b"content B")
        self.assertNotEqual(compute_full_hash(f1), compute_full_hash(f2))

    def test_quick_hash_same_prefix(self):
        content = b"X" * 4096 + b"different_suffix"
        f1 = os.path.join(self.temp_dir, "f1.bin")
        f2 = os.path.join(self.temp_dir, "f2.bin")
        with open(f1, "wb") as fp:
            fp.write(content)
        with open(f2, "wb") as fp:
            fp.write(b"X" * 4096 + b"another_suffix__")
        # Quick hashes should be the same (same first 4KB)
        self.assertEqual(compute_quick_hash(f1), compute_quick_hash(f2))
        # Full hashes should differ
        self.assertNotEqual(compute_full_hash(f1), compute_full_hash(f2))

    def test_empty_file(self):
        f = os.path.join(self.temp_dir, "empty.txt")
        with open(f, "wb") as fp:
            pass
        # Should not crash
        h = compute_full_hash(f)
        self.assertIsInstance(h, str)
        self.assertGreater(len(h), 0)


# ====================================================================== #
#  Test Directory Walker                                                   #
# ====================================================================== #
class TestDirectoryWalker(unittest.TestCase):
    """Tests for BFS and DFS directory traversal."""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        # Create structure
        for sub in ["a", "b", os.path.join("a", "c")]:
            os.makedirs(os.path.join(self.temp_dir, sub), exist_ok=True)
        for fname in ["root.txt", "a/a1.txt", "a/a2.txt",
                       "a/c/deep.txt", "b/b1.txt"]:
            fp = os.path.join(self.temp_dir, fname)
            with open(fp, "w") as f:
                f.write("data" * 10)

    def tearDown(self):
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_dfs_finds_all(self):
        walker = DirectoryWalker()
        files = list(walker.walk_dfs(self.temp_dir))
        self.assertEqual(len(files), 5)

    def test_bfs_finds_all(self):
        walker = DirectoryWalker()
        files = list(walker.walk_bfs(self.temp_dir))
        self.assertEqual(len(files), 5)

    def test_min_size_filter(self):
        # Create a tiny file
        tiny = os.path.join(self.temp_dir, "tiny.txt")
        with open(tiny, "w") as f:
            f.write("x")
        config = ScanConfig(min_size=10)
        walker = DirectoryWalker(config)
        files = list(walker.walk_dfs(self.temp_dir))
        paths = [fi.path for fi in files]
        self.assertNotIn(os.path.abspath(tiny), paths)

    def test_extension_filter(self):
        # Add a .log file
        log = os.path.join(self.temp_dir, "debug.log")
        with open(log, "w") as f:
            f.write("log data" * 5)
        config = ScanConfig(include_extensions={".txt"})
        walker = DirectoryWalker(config)
        files = list(walker.walk_dfs(self.temp_dir))
        for fi in files:
            self.assertEqual(fi.extension, ".txt")

    def test_stats(self):
        walker = DirectoryWalker()
        list(walker.walk_dfs(self.temp_dir))
        stats = walker.get_stats()
        self.assertEqual(stats["files_scanned"], 5)
        self.assertGreaterEqual(stats["dirs_scanned"], 3)


# ====================================================================== #
#  Integration Test — Full Duplicate Detection                             #
# ====================================================================== #
class TestDuplicateDetector(unittest.TestCase):
    """Integration test for the full detection pipeline."""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        # Create some duplicate files
        content_a = b"This is file A content " * 100  # ~2.2 KB
        content_b = b"This is file B content " * 100  # ~2.2 KB
        content_c = b"Unique file C " * 50

        # 3 copies of content_a
        for name in ["a1.txt", "a2.txt", "a3.txt"]:
            with open(os.path.join(self.temp_dir, name), "wb") as f:
                f.write(content_a)

        # 2 copies of content_b
        os.makedirs(os.path.join(self.temp_dir, "sub"), exist_ok=True)
        with open(os.path.join(self.temp_dir, "b1.txt"), "wb") as f:
            f.write(content_b)
        with open(os.path.join(self.temp_dir, "sub", "b2.txt"), "wb") as f:
            f.write(content_b)

        # 1 unique file
        with open(os.path.join(self.temp_dir, "unique.txt"), "wb") as f:
            f.write(content_c)

    def tearDown(self):
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_detects_duplicates(self):
        detector = DuplicateDetector()
        result = detector.scan(self.temp_dir)

        self.assertEqual(len(result.duplicate_groups), 2)
        self.assertEqual(result.total_files_scanned, 6)

        # Check group sizes
        group_counts = sorted([g.count for g in result.duplicate_groups], reverse=True)
        self.assertEqual(group_counts, [3, 2])

    def test_detects_with_bfs(self):
        detector = DuplicateDetector(traversal="bfs")
        result = detector.scan(self.temp_dir)
        self.assertEqual(len(result.duplicate_groups), 2)

    def test_no_duplicates_directory(self):
        unique_dir = tempfile.mkdtemp()
        try:
            for i in range(5):
                with open(os.path.join(unique_dir, f"file_{i}.txt"), "w") as f:
                    f.write(f"unique content {i}" * 100)
            detector = DuplicateDetector()
            result = detector.scan(unique_dir)
            self.assertEqual(len(result.duplicate_groups), 0)
            self.assertEqual(result.total_duplicates, 0)
        finally:
            shutil.rmtree(unique_dir, ignore_errors=True)

    def test_top_k_heap(self):
        detector = DuplicateDetector()
        result = detector.scan(self.temp_dir)
        top = detector.get_top_k_groups(result, k=1)
        self.assertEqual(len(top), 1)
        # The group with 3 copies wastes more space
        self.assertEqual(top[0].count, 3)

    def test_wasted_space_calculation(self):
        detector = DuplicateDetector()
        result = detector.scan(self.temp_dir)
        self.assertGreater(result.total_wasted_bytes, 0)
        for g in result.duplicate_groups:
            self.assertEqual(g.wasted_bytes, (g.count - 1) * g.file_size)


if __name__ == "__main__":
    unittest.main(verbosity=2)
