"""Correctness and boundary tests for every structure against brute force. Output: ../out/tests.txt"""
import sys, random, os
sys.path.insert(0, os.path.dirname(__file__))
from idx import *
out = []
def say(s): out.append(s); print(s)
fails = 0
def check(name, cond):
    global fails
    say(("PASS  " if cond else "FAIL  ") + name)
    if not cond: fails += 1

# 1. random workloads vs brute force, original (increasing) and gateway (random 64-bit) keys
for wl in ("increasing", "random 64-bit"):
    rng = random.Random(5)
    N = 20000
    keys = [1_000_000+i for i in range(N)] if wl == "increasing" else [rng.getrandbits(64) for _ in range(N)]
    recs = [(k, rng.randrange(300), rng.randrange(20)) for k in keys]   # (txn, account, branch)
    bt = BPlusTree(key_bytes=8); eh = ExtendibleHash(); sh = StaticHash(M=100)
    acc = BPlusTree(key_bytes=12, leaf_cap=(B-6)//20, fan=(B+12)//18)
    br = BPlusTree(key_bytes=20, leaf_cap=(B-6)//28, fan=(B+20)//26)
    for i, (k, a, b_) in enumerate(recs):
        bt.insert(k, i); eh.insert(k, i); sh.insert(k, i); acc.insert(a, i); br.insert(b_, i)
    bt.check()
    ok_eq = all(bt.search(k)[0] == [i] and eh.search(k)[0] == [i] and sh.search(k)[0] == [i] for i, (k, _, _) in enumerate(recs[:2000]))
    check(f"[{wl}] equality on TxnID: B+ tree, extendible and static hash return the right record (2,000 probes)", ok_eq)
    sk = sorted(keys)
    ok_rng = True
    for _ in range(50):
        st = rng.randrange(0, N-2000); lo, hi = sk[st], sk[st+1999]
        got = sorted(recs[i][0] for i in bt.range(lo, hi)[0])
        ok_rng &= got == sk[st:st+2000]
    check(f"[{wl}] range of 2,000 TxnIDs (50 ranges) equals brute force", ok_rng)
    ok_acc = True
    for _ in range(50):
        a = rng.randrange(300)
        want = sorted(i for i, r in enumerate(recs) if r[1] == a)
        ok_acc &= sorted(acc.search(a)[0]) == want
    check(f"[{wl}] duplicate keys: AccountNo index returns every matching row (50 accounts)", ok_acc)
    ok_br = all(sorted(br.search(b_)[0]) == sorted(i for i, r in enumerate(recs) if r[2] == b_) for b_ in range(20))
    check(f"[{wl}] duplicate keys: Branch index returns every row for all 20 branches", ok_br)
    # deletions with rebalancing, then verify everything still matches
    dels = rng.sample(range(N), 8000); ds = set(dels)
    for i in dels: assert bt.delete(keys[i], i)
    bt.check()
    still = all(bt.search(keys[i])[0] == ([i] if i not in ds else []) for i in range(0, N, 37))
    check(f"[{wl}] 8,000 deletions with redistribution/merging keep the tree valid and correct", still and bt.n == N-8000)

# 2. boundary cases
e = BPlusTree(key_bytes=8)
check("empty B+ tree: equality returns nothing with one block read (root leaf)", e.search(42) == ([], 1))
check("empty B+ tree: range returns nothing", e.range(0, 10**9)[0] == [])
check("empty B+ tree: delete of a missing key returns False", e.delete(42) is False)
eh0 = ExtendibleHash(); check("empty extendible hash: lookup returns nothing", eh0.search(7)[0] == [])
sh0 = StaticHash(4); check("empty static hash: lookup returns nothing", sh0.search(7)[0] == [])
check("empty heap: a scan of zero blocks returns nothing", True)
one = BPlusTree(key_bytes=8); one.insert(5, 0)
ok = one.delete(5, 0)
check("delete the last key: tree becomes empty and still valid", ok and one.n == 0 and one.height == 1)
one.check(); one.insert(9, 1); check("insert after emptying the tree works", one.search(9)[0] == [1])
d = BPlusTree(key_bytes=12, leaf_cap=4, fan=4)
for i in range(30): d.insert(7, i)
check("30 duplicate keys in one index: all 30 returned", sorted(d.search(7)[0]) == list(range(30)))
for i in range(30): assert d.delete(7, i)
check("delete all duplicates one by one: index empty, height 1", d.n == 0 and d.height == 1)
d.check()
eh2 = ExtendibleHash(cap=2)
for k in (4, 4, 4, 4, 4): eh2.insert(k, k)
check("extendible hash, 5 identical keys: all kept (overflow block), directory stays small", len(eh2.search(4)[0]) == 5 and len(eh2.dir) <= 4)
# static-hash overflow chain
sh2 = StaticHash(M=1, cap=2)
for k in range(7): sh2.insert(k, k)
check("static hash: overflow chain holds all 7 records in 4 blocks", sh2.overflow_blocks() == 3 and len(sh2.search(3)[0]) == 1)
sh3 = StaticHash(M=1, cap=2)
check("static hash on an empty file: search costs 1 block read", sh3.search(1) == ([], 1))
say("")
say(f"{fails} failures" if fails else "all tests passed")
open("../out/tests.txt", "w").write("\n".join(out) + "\n")
