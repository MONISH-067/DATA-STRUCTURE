"""
Trie (Prefix Tree) for Directory Path Indexing
================================================
Data Structure: Trie

Stores directory paths split by path separator so that we can:
    1. Quickly find all files under a given prefix directory.
    2. Auto-complete directory paths during interactive search.
    3. Count how many scanned files reside under each directory.

Time Complexity:
    - Insert path of depth d : O(d)
    - Search prefix          : O(d + k) where k = number of results
    - Longest common prefix  : O(d)

Space Complexity: O(N * D) where N = number of paths, D = average depth
"""

from __future__ import annotations
from typing import Optional
import os


class TrieNode:
    """A single node in the directory trie."""

    __slots__ = ("children", "is_end", "file_paths", "count")

    def __init__(self):
        # children maps directory-component -> TrieNode
        self.children: dict[str, TrieNode] = {}
        self.is_end: bool = False  # True if a complete path ends here
        self.file_paths: list[str] = []  # files whose directory is this node
        self.count: int = 0  # number of files in this subtree


class DirectoryTrie:
    """
    A trie that indexes file paths by their directory components.

    Example:
        Insert "/home/user/docs/a.txt"
        Components: ["home", "user", "docs"]
        The file path is stored at the "docs" node.
    """

    def __init__(self):
        self.root = TrieNode()

    def insert(self, file_path: str) -> None:
        """
        Insert a file path into the trie.

        The directory portion is split into components and each component
        becomes a trie node.  The full file_path is stored at the deepest
        directory node.
        """
        directory = os.path.dirname(os.path.abspath(file_path))
        components = self._split_path(directory)

        node = self.root
        node.count += 1
        for comp in components:
            if comp not in node.children:
                node.children[comp] = TrieNode()
            node = node.children[comp]
            node.count += 1

        node.is_end = True
        node.file_paths.append(os.path.abspath(file_path))

    def search_prefix(self, prefix_path: str) -> list[str]:
        """
        Return all file paths stored under a directory prefix.

        Uses DFS to collect all files in the subtree.
        """
        components = self._split_path(os.path.abspath(prefix_path))
        node = self.root
        for comp in components:
            if comp not in node.children:
                return []
            node = node.children[comp]

        # DFS to collect all files under this node
        result: list[str] = []
        self._collect_files(node, result)
        return result

    def count_under(self, prefix_path: str) -> int:
        """Return the number of files indexed under a directory prefix."""
        components = self._split_path(os.path.abspath(prefix_path))
        node = self.root
        for comp in components:
            if comp not in node.children:
                return 0
            node = node.children[comp]
        return node.count

    def autocomplete(self, partial_path: str, max_results: int = 10) -> list[str]:
        """
        Given a partial directory path, return possible completions.

        Useful for an interactive CLI where users type directory prefixes.
        """
        components = self._split_path(os.path.abspath(partial_path))
        node = self.root
        current_path_parts: list[str] = []

        for comp in components:
            if comp not in node.children:
                return []
            node = node.children[comp]
            current_path_parts.append(comp)

        # BFS one level of children as suggestions
        suggestions: list[str] = []
        for child_name in sorted(node.children.keys()):
            if len(suggestions) >= max_results:
                break
            suggestion = os.path.join(*current_path_parts, child_name)
            suggestions.append(suggestion)
        return suggestions

    def longest_common_prefix(self) -> str:
        """
        Find the longest directory prefix shared by ALL indexed files.

        Algorithm: Walk down the trie following the only-child path
        until a node has more than one child or is an endpoint.
        """
        node = self.root
        parts: list[str] = []
        while len(node.children) == 1 and not node.file_paths:
            child_name = next(iter(node.children))
            parts.append(child_name)
            node = node.children[child_name]
        return os.sep.join(parts) if parts else ""

    # ------------------------------------------------------------------ #
    #  Internal helpers                                                    #
    # ------------------------------------------------------------------ #
    def _split_path(self, path: str) -> list[str]:
        """Split an absolute path into its directory components."""
        parts = []
        while True:
            head, tail = os.path.split(path)
            if tail:
                parts.append(tail)
            elif head and head != path:
                path = head
                continue
            else:
                if head:
                    parts.append(head)
                break
            path = head
        parts.reverse()
        return parts

    def _collect_files(self, node: TrieNode, result: list[str]) -> None:
        """DFS traversal to collect all file paths under a trie node."""
        result.extend(node.file_paths)
        for child in node.children.values():
            self._collect_files(child, result)

    # ------------------------------------------------------------------ #
    #  Statistics                                                          #
    # ------------------------------------------------------------------ #
    def depth(self) -> int:
        """Return the maximum depth of the trie."""
        return self._max_depth(self.root, 0)

    def _max_depth(self, node: TrieNode, current: int) -> int:
        if not node.children:
            return current
        return max(
            self._max_depth(child, current + 1)
            for child in node.children.values()
        )

    def total_files(self) -> int:
        return self.root.count
