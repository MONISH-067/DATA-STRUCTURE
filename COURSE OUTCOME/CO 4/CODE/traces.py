"""Hand-checkable traces for Tasks 6 and 7, plus capacity and hashing formulas.
Writes ../out/traces.txt and ../out/traces.json"""
import sys, json, math
sys.path.insert(0, __import__("os").path.dirname(__file__))
from idx import *
from math import comb, exp, factorial
OUT = []
def p(s=""): OUT.append(s)

# ---------- render a B+ tree as text ----------
def render_bplus(t):
    lines = []
    def rec(node, depth):
        if isinstance(node, Leaf):
            lines.append("  "*depth + "leaf " + str(node.keys))
        else:
            lines.append("  "*depth + "node sep " + str(node.keys))
            for c in node.children: rec(c, depth+1)
    rec(t.root, 0); return "\n".join(lines)

# ---------- B-tree (keys also in internal nodes), order p=4 (max 3 keys) ----------
class BTree:
    def __init__(self, maxk=3): self.maxk = maxk; self.root = {"k": [], "c": []}; self.log = []
    def insert(self, key):
        r = self.root
        if len(r["k"]) == self.maxk:
            new = {"k": [], "c": [r]}; self._split(new, 0); self.root = new; self.log.append(f"root full: new root, height +1")
        self._ins(self.root, key)
    def _split(self, parent, i):
        child = parent["c"][i]; m = len(child["k"])//2
        mid = child["k"][m]
        right = {"k": child["k"][m+1:], "c": child["c"][m+1:] if child["c"] else []}
        child["k"] = child["k"][:m]; child["c"] = child["c"][:m+1] if child["c"] else []
        parent["k"].insert(i, mid); parent["c"].insert(i+1, right)
        self.log.append(f"split node {child['k'] + [mid] + right['k']} -> {child['k']} | {mid} up | {right['k']}")
    def _ins(self, node, key):
        i = 0
        while i < len(node["k"]) and key > node["k"][i]: i += 1
        if not node["c"]:
            node["k"].insert(i, key); return
        if len(node["c"][i]["k"]) == self.maxk:
            self._split(node, i)
            if key > node["k"][i]: i += 1
        self._ins(node["c"][i], key)
    def show(self):
        out = []
        def rec(n, d):
            out.append("  "*d + str(n["k"]))
            for c in n["c"]: rec(c, d+1)
        rec(self.root, 0); return "\n".join(out)

KEYS = [13, 7, 25, 4, 19, 11, 30, 3, 22, 17, 8, 28, 2, 15, 26]
p("TASK 6 – TRACES (order 4: at most 3 keys per node; 15 keys inserted in the order shown)")
p("keys: " + str(KEYS))
p("\n-- B-tree (keys stored in internal nodes too) --")
bt = BTree(3)
for k in KEYS:
    bt.log = []; bt.insert(k)
    p(f"insert {k:>2}: " + ("; ".join(bt.log) if bt.log else "no split"))
p("final B-tree:"); p(bt.show())

p("\n-- B+ tree (leaf capacity 3, fan-out 4; keys copied up, all records in leaves) --")
bp = BPlusTree(key_bytes=8, leaf_cap=3, fan=4); bp.log = []
for k in KEYS:
    bp.log = []; bp.insert(k, k)
    p(f"insert {k:>2}: " + ("; ".join(bp.log) if bp.log else "no split"))
p("final B+ tree:"); p(render_bplus(bp))
p("leaf chain: " + " -> ".join(str(l.keys) for l in bp.leaf_iter()))
bp.check()

DEL = [25, 4, 19, 3]
p(f"\n-- deletions from the B+ tree: {DEL} (min leaf 2 keys, min children 2) --")
for k in DEL:
    bp.log = []; ok = bp.delete(k, k)
    p(f"delete {k:>2}: " + ("; ".join(bp.log) if bp.log else "no underflow") + f" | height {bp.height}")
    p(render_bplus(bp)); p("")
bp.check()
p("tree after deletions verified: keys in order, separators valid, all leaves at equal depth.")

# ---------- capacity at 69% fill ----------
F = 0.69
leaf_cap = (B-BLOCK_PTR)//(KEY+REC_PTR); fan = (B+KEY)//(KEY+BLOCK_PTR)
l_leaf = int(F*leaf_cap); l_fan = int(F*fan)
bpcap = {}
for L in (1,2,3,4):
    bpcap[L] = l_leaf * (l_fan ** (L-1))
bt_order = (B + 0)//(6+8+8)   # 6p + 16(p-1) <= 4096  -> p = floor((4096+16)/22)
bt_order = (B+16)//22
bt_keys = int(F*(bt_order-1)); bt_fan = int(F*bt_order)
btcap = {}
for L in (1,2,3,4):
    nodes = sum(bt_fan**i for i in range(L)); btcap[L] = nodes*bt_keys
p("\nTASK 6 – CAPACITY AT 69% FILL")
p(f"B+ tree: leaf capacity {leaf_cap} entries, fan-out {fan}; at 69%: {l_leaf} records per leaf, {l_fan} children per inner node")
for L in (2,3,4): p(f"  {L} levels: {bpcap[L]:,} records")
p(f"B-tree: order p = floor((4096+16)/22) = {bt_order}; at 69%: {bt_keys} keys per node, {bt_fan} children")
for L in (2,3,4): p(f"  {L} levels: {btcap[L]:,} records (keys in all nodes)")

# ---------- hashing ----------
p("\nTASK 7 – HASHING")
def pois_overflow_log(alpha, cap=256):
    lam = alpha*cap; e = 0.0
    lo = cap+1; hi = int(lam + 15*math.sqrt(lam)) + 2*cap
    for x in range(lo, hi):
        lp = -lam + x*math.log(lam) - math.lgamma(x+1)
        e += math.exp(lp) * math.ceil((x-cap)/cap)
    return e
static = {}
for alpha in (0.5, 0.7, 0.9, 0.95):
    static[alpha] = pois_overflow_log(alpha)
    p(f"static hash, bucket cap 256, load {alpha}: expected overflow blocks per bucket = {static[alpha]:.4f}; expected blocks per successful search ≈ {1+static[alpha]:.3f}")

# extendible hashing trace with 8-bit hash values and bucket capacity 2
class ToyEH(ExtendibleHash):
    @staticmethod
    def h(key): return key & 0xFF
def dump(eh):
    out = [f"  global depth {eh.gd}, directory {len(eh.dir)} entries"]
    seen = {}
    for idx_, b_ in enumerate(eh.dir):
        bid = seen.setdefault(id(b_), len(seen))
        out.append(f"    dir[{idx_:0{eh.gd}b}] -> bucket {bid} (local depth {b_.ld}) {sorted(k for k,_ in b_.items)}")
    return "\n".join(out)
p("\nExtendible hashing trace (keys used as 8-bit hash values, bucket capacity 2)")
eh = ToyEH(cap=2); eh.log = []
for k in [2, 3, 5, 7, 11, 13, 4, 10]:
    eh.log = []; eh.insert(k, k)
    p(f"insert {k:>2} (hash {k:08b}): " + ("; ".join(eh.log) if eh.log else "fits in bucket"))
p(dump(eh))

# linear hashing trace: M0=2 buckets, capacity 2, split when load factor > 0.75
class LinHash:
    def __init__(self, M0=2, cap=2, thr=0.75):
        self.M0=M0; self.cap=cap; self.thr=thr; self.L=0; self.p=0; self.b=[[] for _ in range(M0)]; self.n=0; self.log=[]
    def hL(self, k, lvl): return k % (self.M0 * 2**lvl)
    def addr(self, k):
        a = self.hL(k, self.L)
        if a < self.p: a = self.hL(k, self.L+1)
        return a
    def insert(self, k):
        self.b[self.addr(k)].append(k); self.n += 1
        if self.n / (len(self.b)*self.cap) > self.thr:
            self.split()
    def split(self):
        old = self.b[self.p]; self.b[self.p] = []; self.b.append([])
        newi = len(self.b)-1
        for k in old:
            (self.b[self.p] if self.hL(k, self.L+1) == self.p else self.b[newi]).append(k)
        self.log.append(f"split bucket {self.p} (keys {old}) -> bucket {self.p} {self.b[self.p]} and new bucket {newi} {self.b[newi]}; split pointer {self.p+1 if self.p+1 < self.M0*2**self.L else 0}, level {self.L if self.p+1 < self.M0*2**self.L else self.L+1}")
        self.p += 1
        if self.p == self.M0 * 2**self.L: self.p = 0; self.L += 1
lh = LinHash(2, 2, 0.75)
p("\nLinear hashing trace (M0 = 2 buckets, capacity 2, split when load factor > 0.75)")
for k in [4, 8, 12, 1, 5, 9, 2, 6]:
    lh.log = []; lh.insert(k)
    p(f"insert {k:>2}: " + ("; ".join(lh.log) if lh.log else "no split") + f"  | level {lh.L}, p {lh.p}, buckets {lh.b}")
p("hash function used: h_level(k) = k mod (2^level * M0); addresses below the split pointer use level+1.")
# expected costs
p("\nExpected costs (equality search on TxnID): static 1+overflow; extendible 1 bucket read (+directory in memory); linear 1 to 2 bucket reads; range search: not supported.")

json.dump({"bt_order": bt_order, "bt_keys": bt_keys, "bt_fan": bt_fan, "bp_leaf_cap": leaf_cap, "bp_fan": fan,
           "bp_leaf69": l_leaf, "bp_fan69": l_fan, "bp_capacity": bpcap, "bt_capacity": btcap,
           "static_overflow": {str(k): v for k, v in static.items()}, "bp_height_after_build": bp.height},
          open("../out/traces.json", "w"), indent=1)
open("../out/traces.txt", "w").write("\n".join(OUT) + "\n")
print("\n".join(OUT)[:6000])
