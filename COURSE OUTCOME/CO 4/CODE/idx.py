"""Block-level index structures. Every node/bucket is one 4096-byte block; every
block read or write is counted in .reads / .writes. Sizes follow the assignment:
TXN record 200 B, block 4096 B, block pointer 6 B, record pointer 8 B, key 8 B."""
from bisect import bisect_left, bisect_right
B=4096; BLOCK_PTR=6; REC_PTR=8; KEY=8; REC=200; BFR=B//REC   # bfr = 20
LEAF_CAP_PRIMARY = (B-BLOCK_PTR)//(KEY+REC_PTR)               # 255 entries + next pointer
FANOUT_PRIMARY   = (B+KEY)//(KEY+BLOCK_PTR)                   # 293 children

class Leaf:
    __slots__=("keys","vals","next")
    def __init__(s): s.keys=[]; s.vals=[]; s.next=None
class Inner:
    __slots__=("keys","children")
    def __init__(s): s.keys=[]; s.children=[]

class BPlusTree:
    """Dense B+ tree. Duplicate keys allowed (secondary index). Insert, search,
    range and delete with redistribution and merging, all counted."""
    def __init__(self, key_bytes=KEY, leaf_cap=None, fan=None, min_leaf=None, min_fan=None):
        self.leaf_cap = leaf_cap or (B-BLOCK_PTR)//(key_bytes+REC_PTR)
        self.fan = fan or (B+key_bytes)//(key_bytes+BLOCK_PTR)
        self.min_leaf = min_leaf or (self.leaf_cap+1)//2
        self.min_fan = min_fan or (self.fan+1)//2
        self.root = Leaf(); self.height = 1; self.leaves = 1; self.inners = 0
        self.n = 0; self.reads = 0; self.writes = 0; self.log = None
        self.key_bytes = key_bytes
    def _say(self, msg):
        if self.log is not None: self.log.append(msg)
    def _descend(self, key, path, right=False):
        node = self.root
        while isinstance(node, Inner):
            self.reads += 1
            i = bisect_right(node.keys, key) if right else bisect_left(node.keys, key)
            path.append((node, i)); node = node.children[i]
        self.reads += 1
        return node
    def insert(self, key, rid):
        path = []; leaf = self._descend(key, path, right=True)
        i = bisect_right(leaf.keys, key)
        leaf.keys.insert(i, key); leaf.vals.insert(i, rid); self.writes += 1; self.n += 1
        if len(leaf.keys) <= self.leaf_cap:
            return
        mid = len(leaf.keys)//2
        new = Leaf(); new.keys = leaf.keys[mid:]; new.vals = leaf.vals[mid:]
        del leaf.keys[mid:]; del leaf.vals[mid:]
        new.next = leaf.next; leaf.next = new
        self.writes += 2; self.leaves += 1
        self._say(f"split leaf: {leaf.keys[0]}..{leaf.keys[-1]} | {new.keys[0]}..{new.keys[-1]}; separator {new.keys[0]}")
        self._promote(new.keys[0], new, path)
    def _promote(self, sep, child, path):
        while path:
            parent, i = path.pop()
            parent.keys.insert(i, sep); parent.children.insert(i+1, child); self.writes += 1
            if len(parent.children) <= self.fan:
                return
            mid = len(parent.children)//2
            new = Inner(); new.keys = parent.keys[mid:]; new.children = parent.children[mid:]
            sep = parent.keys[mid-1]
            del parent.keys[mid-1:]; del parent.children[mid:]
            self.writes += 1; self.inners += 1
            self._say(f"split internal node; key {sep} moves up")
            child = new
        new_root = Inner(); new_root.keys = [sep]; new_root.children = [self.root, child]
        self.root = new_root; self.height += 1; self.inners += 1; self.writes += 1
        self._say(f"root split: new root, height now {self.height}")
    def search(self, key):
        """Return all record ids with this key and the number of index blocks read."""
        before = self.reads; path = []; leaf = self._descend(key, path); out = []
        while leaf is not None:
            for k, v in zip(leaf.keys, leaf.vals):
                if k == key: out.append(v)
                elif k > key: return out, self.reads-before
            nxt = leaf.next
            if nxt is not None and nxt.keys and nxt.keys[0] <= key:
                leaf = nxt; self.reads += 1
            else: break
        return out, self.reads-before
    def range(self, lo, hi):
        before = self.reads; path = []; leaf = self._descend(lo, path); out = []
        while leaf is not None:
            for k, v in zip(leaf.keys, leaf.vals):
                if k > hi: return out, self.reads-before
                if k >= lo: out.append(v)
            leaf = leaf.next
            if leaf is not None: self.reads += 1
        return out, self.reads-before
    def delete(self, key, rid=None):
        path = []; leaf = self._descend(key, path)
        found = False; node = leaf
        while node is not None and not found:
            for j,(k,v) in enumerate(zip(node.keys,node.vals)):
                if k == key and (rid is None or v == rid):
                    del node.keys[j]; del node.vals[j]; found = True; break
                if k > key: break
            if not found:
                nxt = node.next
                if nxt is not None and nxt.keys and nxt.keys[0] <= key:
                    node = nxt; self.reads += 1
                else: break
        if not found: return False
        self.writes += 1; self.n -= 1
        path2 = []; self._find_path(self.root, node, key, path2)   # path to the leaf that lost the entry
        self._fix_leaf(node, path2)
        return True
    def _find_path(self, cur, target, key, path):
        if cur is target: return True
        if isinstance(cur, Leaf): return False
        lo = bisect_left(cur.keys, key); hi = bisect_right(cur.keys, key)
        for j in range(lo, hi+1):
            path.append((cur, j))
            if self._find_path(cur.children[j], target, key, path): return True
            path.pop()
        return False
    def _fix_leaf(self, node, path):
        if not path:   # root leaf
            return
        if len(node.keys) >= self.min_leaf:
            return
        parent, i = path[-1]
        left = parent.children[i-1] if i > 0 else None
        right = parent.children[i+1] if i+1 < len(parent.children) else None
        if left is not None: self.reads += 1
        if right is not None: self.reads += 1
        if left is not None and len(left.keys) > self.min_leaf:          # redistribute from left
            node.keys.insert(0, left.keys.pop()); node.vals.insert(0, left.vals.pop())
            parent.keys[i-1] = node.keys[0]; self.writes += 3
            self._say(f"redistribute from left sibling: separator becomes {parent.keys[i-1]}")
        elif right is not None and len(right.keys) > self.min_leaf:      # redistribute from right
            node.keys.append(right.keys.pop(0)); node.vals.append(right.vals.pop(0))
            parent.keys[i] = right.keys[0]; self.writes += 3
            self._say(f"redistribute from right sibling: separator becomes {parent.keys[i]}")
        else:                                                              # merge
            if left is not None:
                left.keys += node.keys; left.vals += node.vals; left.next = node.next
                del parent.keys[i-1]; del parent.children[i]
            else:
                node.keys += right.keys; node.vals += right.vals; node.next = right.next
                del parent.keys[i]; del parent.children[i+1]
            self.writes += 2; self.leaves -= 1
            self._say("merge leaves; separator removed from parent")
            self._fix_inner(parent, path[:-1])
    def _fix_inner(self, node, path):
        if not path:
            if len(node.children) == 1:             # root shrinks
                self.root = node.children[0]; self.height -= 1; self.inners -= 1; self.writes += 1
                self._say(f"root shrinks, height now {self.height}")
            return
        if len(node.children) >= self.min_fan:
            return
        parent, i = path[-1]
        left = parent.children[i-1] if i > 0 else None
        right = parent.children[i+1] if i+1 < len(parent.children) else None
        if left is not None: self.reads += 1
        if right is not None: self.reads += 1
        if left is not None and len(left.children) > self.min_fan:
            node.keys.insert(0, parent.keys[i-1]); parent.keys[i-1] = left.keys.pop()
            node.children.insert(0, left.children.pop()); self.writes += 3
            self._say("internal redistribution from left")
        elif right is not None and len(right.children) > self.min_fan:
            node.keys.append(parent.keys[i]); parent.keys[i] = right.keys.pop(0)
            node.children.append(right.children.pop(0)); self.writes += 3
            self._say("internal redistribution from right")
        else:
            if left is not None:
                left.keys += [parent.keys[i-1]] + node.keys; left.children += node.children
                del parent.keys[i-1]; del parent.children[i]
            else:
                node.keys += [parent.keys[i]] + right.keys; node.children += right.children
                del parent.keys[i]; del parent.children[i+1]
            self.writes += 2; self.inners -= 1
            self._say("internal merge")
            self._fix_inner(parent, path[:-1])
    # statistics
    def leaf_iter(self):
        node = self.root
        while isinstance(node, Inner): node = node.children[0]
        while node is not None: yield node; node = node.next
    def utilisation(self):
        leaves = list(self.leaf_iter())
        return sum(len(l.keys) for l in leaves) / (len(leaves)*self.leaf_cap)
    def size_bytes(self): return (self.leaves + self.inners) * B
    def check(self):
        """Structural check: sorted keys, separator correctness, leaf chain, counts."""
        keys=[k for l in self.leaf_iter() for k in l.keys]
        assert keys == sorted(keys), "leaf chain not sorted"
        assert len(keys) == self.n, (len(keys), self.n)
        def walk(node, lo, hi, depth):
            if isinstance(node, Leaf):
                assert all(lo <= k for k in node.keys) and all(k <= hi for k in node.keys), "leaf key outside separators"
                return depth
            assert len(node.children) == len(node.keys)+1
            ds = {walk(c, lo if j==0 else node.keys[j-1], hi if j==len(node.keys) else node.keys[j], depth+1) for j,c in enumerate(node.children)}
            assert len(ds) == 1, "unbalanced"; return ds.pop()
        d = walk(self.root, -10**30, 10**30, 1); assert d == self.height, (d, self.height)
        return True

class ExtendibleHash:
    """Extendible hashing on 64-bit keys. Bucket = one block holding up to
    `cap` (key, rid) pairs. Directory kept in memory: 2**global_depth pointers of 8 B."""
    def __init__(self, cap=B//(KEY+REC_PTR)):
        self.cap = cap; self.gd = 0; self.dir = [Bucket(0)]; self.n = 0
        self.reads = 0; self.writes = 0; self.splits = 0; self.doublings = 0; self.log=None
    @staticmethod
    def h(key):
        x = (key + 0x9E3779B97F4A7C15) & (2**64-1)
        x = ((x ^ (x >> 30)) * 0xBF58476D1CE4E5B9) & (2**64-1)
        x = ((x ^ (x >> 27)) * 0x94D049BB133111EB) & (2**64-1)
        return x ^ (x >> 31)
    def _bucket(self, key):
        return self.dir[self.h(key) & ((1 << self.gd)-1)]
    def insert(self, key, rid):
        b = self._bucket(key); self.reads += 1
        b.items.append((key, rid)); self.writes += 1; self.n += 1
        while len(b.items) > self.cap:
            if len({self.h(k) for k, _ in b.items}) == 1:   # identical hashes cannot be separated:
                self.overflow_blocks = getattr(self, "overflow_blocks", 0) + 1; self.writes += 1
                break                                         # keep them in an overflow block
            self._split(b)
            b = self._bucket(key)
    def _split(self, b):
        if b.ld == self.gd:
            self.dir = self.dir + self.dir; self.gd += 1; self.doublings += 1
            self._say(f"directory doubles to {len(self.dir)} entries (global depth {self.gd})")
        nb = Bucket(b.ld+1); b.ld += 1
        self.writes += 2; self.splits += 1
        old = b.items; b.items = []; mask = 1 << (b.ld-1)
        for idx in range(len(self.dir)):
            if self.dir[idx] is b and (idx & mask):
                self.dir[idx] = nb
        for k, r in old:
            tgt = nb if (self.h(k) & mask) else b
            tgt.items.append((k, r))
        self._say(f"split bucket: local depth {b.ld}")
    def _say(self, m):
        if self.log is not None: self.log.append(m)
    def search(self, key):
        b = self._bucket(key); self.reads += 1
        return [r for k, r in b.items if k == key], 1
    def buckets(self):
        seen = {}
        for b in self.dir: seen[id(b)] = b
        return list(seen.values())
    def size_bytes(self): return len(self.buckets())*B + len(self.dir)*KEY
    def utilisation(self):
        bs = self.buckets(); return sum(len(b.items) for b in bs)/(len(bs)*self.cap)

class Bucket:
    __slots__=("items","ld")
    def __init__(s, ld): s.items=[]; s.ld=ld

class StaticHash:
    """Static hashing with M primary buckets and overflow chains."""
    def __init__(self, M, cap=B//(KEY+REC_PTR)):
        self.M=M; self.cap=cap; self.buckets=[[[]] for _ in range(M)]   # chain = list of blocks
        self.reads=0; self.writes=0; self.n=0
    def _idx(self, key): return ExtendibleHash.h(key) % self.M
    def insert(self, key, rid):
        chain = self.buckets[self._idx(key)]; self.reads += 1
        for blk in chain:
            if len(blk) < self.cap:
                blk.append((key,rid)); self.writes += 1; self.n += 1; return
        self.reads += len(chain)-1; chain.append([(key,rid)]); self.writes += 2; self.n += 1
    def search(self, key):
        out=[]; blocks=0
        for blk in self.buckets[self._idx(key)]:
            blocks += 1
            out += [r for k,r in blk if k == key]
        self.reads += blocks
        return out, blocks
    def overflow_blocks(self): return sum(len(c)-1 for c in self.buckets)
