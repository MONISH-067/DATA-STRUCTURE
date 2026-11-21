"""Experiment runner. Usage: python3 sim.py <N> <workload: orig|gateway>  -> ../out/exp_<workload>_<N>.json
Records: (TxnID, AccountNo, TxnTime, Branch); 20 records per 4096-byte block (bfr = 20).
Strategies for the TxnID path: H heap (unordered, arrival order), S sorted file on TxnID,
BT dense B+ tree on TxnID, EH extendible hash on TxnID, SH static hash (M buckets, 70% load).
Secondary indexes (all strategies): AccountNo and Branch dense B+ trees with record pointers."""
import sys, json, time, random, math, bisect
sys.path.insert(0, __import__("os").path.dirname(__file__))
from idx import *

N = int(sys.argv[1]); WL = sys.argv[2]
rng = random.Random(20260301 + N + (0 if WL == "orig" else 7))
NACC = max(20, N // 50); NBR = min(200, max(5, N // 50))
STEP_MS = 4320 if WL == "orig" else 18            # 20,000/day vs 2,00,000/hour
BASE = 1_700_000_000_000
if WL == "orig":
    txn = [1_000_000 + i for i in range(N)]           # increasing bank numbers
else:
    s = set()
    while len(s) < N: s.add(rng.getrandbits(64))      # random 64-bit gateway TxnIDs
    txn = list(s); rng.shuffle(txn)                   # arrival order is random w.r.t. TxnID
acc_ids = [rng.randrange(10**11, 10**12) for _ in range(NACC)]
recs = [(txn[i], acc_ids[rng.randrange(NACC)], BASE + i*STEP_MS, rng.randrange(NBR)) for i in range(N)]
b = math.ceil(N / BFR)                                # number of blocks in the heap
def blk(pos): return pos // BFR                       # block of heap position pos

res = {"N": N, "workload": WL, "blocks": b, "bfr": BFR, "params": {
    "leaf_cap_txn": (B-BLOCK_PTR)//(KEY+REC_PTR), "fanout_txn": (B+KEY)//(KEY+BLOCK_PTR),
    "leaf_cap_acc": (B-BLOCK_PTR)//(12+REC_PTR), "fanout_acc": (B+12)//(12+BLOCK_PTR),
    "leaf_cap_br": (B-BLOCK_PTR)//(20+REC_PTR), "fanout_br": (B+20)//(20+BLOCK_PTR),
    "eh_bucket_cap": B//(KEY+REC_PTR), "static_M": None, "stepms": STEP_MS, "accounts": NACC, "branches": NBR}}
pos_of = {t: i for i, t in enumerate(txn)}
T0 = time.time()

def timed(fn):
    t = time.time(); r = fn(); return r, (time.time() - t)

# ---------- index builds (counted) ----------
bt_txn = BPlusTree(key_bytes=8); tb = time.time()
for i in range(N): bt_txn.insert(recs[i][0], i)
build_bt = {"seconds": time.time()-tb, "reads": bt_txn.reads, "writes": bt_txn.writes}
bt_txn.reads = bt_txn.writes = 0

bt_acc = BPlusTree(key_bytes=12, leaf_cap=(B-BLOCK_PTR)//(12+REC_PTR), fan=(B+12)//(12+BLOCK_PTR)); tb = time.time()
for i in range(N): bt_acc.insert(recs[i][1], i)
build_acc = {"seconds": time.time()-tb, "reads": bt_acc.reads, "writes": bt_acc.writes}
bt_acc.reads = bt_acc.writes = 0
bt_br = BPlusTree(key_bytes=20, leaf_cap=(B-BLOCK_PTR)//(20+REC_PTR), fan=(B+20)//(20+BLOCK_PTR)); tb = time.time()
for i in range(N): bt_br.insert(recs[i][3], i)
build_br = {"seconds": time.time()-tb, "reads": bt_br.reads, "writes": bt_br.writes}
bt_br.reads = bt_br.writes = 0

# sparse index on the clustered file (one entry per block: first TxnID / first TxnTime)
sp_time = BPlusTree(key_bytes=8, leaf_cap=(B-BLOCK_PTR)//(8+BLOCK_PTR), fan=(B+8)//(8+BLOCK_PTR))
for j in range(b): sp_time.insert(recs[j*BFR][2], j)
sp_time.reads = sp_time.writes = 0

eh = ExtendibleHash(cap=B//(KEY+REC_PTR)); tb = time.time()
for i in range(N): eh.insert(recs[i][0], i)
build_eh = {"seconds": time.time()-tb, "writes": eh.writes, "reads": eh.reads, "splits": eh.splits, "doublings": eh.doublings}
eh.reads = eh.writes = 0

M = math.ceil(N / (0.70 * (B//(KEY+REC_PTR))))
sh = StaticHash(M, cap=B//(KEY+REC_PTR))
for i in range(N): sh.insert(recs[i][0], i)
sh.reads = sh.writes = 0
res["params"]["static_M"] = M
M95 = math.ceil(N / (0.95 * (B//(KEY+REC_PTR))))
sh95 = StaticHash(M95, cap=B//(KEY+REC_PTR))
for i in range(N): sh95.insert(recs[i][0], i)
sh95.reads = sh95.writes = 0
res["params"]["static_M95"] = M95

sorted_order = sorted(range(N), key=lambda i: recs[i][0])
sorted_keys = [recs[i][0] for i in sorted_order]
sorted_rank = {recs[i][0]: r for r, i in enumerate(sorted_order)}

# ---------- helpers ----------
def sorted_search_reads(key):
    """reads = blocks read by binary search over blocks until the block holding key"""
    lo, hi, reads = 0, b-1, 0
    rank = sorted_rank[key]; target = rank // BFR
    while lo <= hi:
        mid = (lo+hi)//2; reads += 1
        if mid == target: return reads
        if target < mid: hi = mid-1
        else: lo = mid+1
    return reads
def heap_reads_for(key):
    """blocks a heap scan reads until the record is found (equality on a unique key)"""
    return blk(pos_of[key]) + 1
def data_blocks(rids):
    return len({blk(r) for r in rids})

rnd = random.Random(99)
Q_EQ = 200; Q_RANGE = 20; RANGE_LEN = 2000; Q_ACC = 100; Q_BR = 10; Q_INS = 100; Q_DEL = 100
sample_idx = [rnd.randrange(N) for _ in range(Q_EQ)]
eq_keys = [recs[i][0] for i in sample_idx]

def run_eq_txn():
    out = {}
    out["H"] = {"reads": sum(heap_reads_for(k) for k in eq_keys)/Q_EQ}
    out["S"] = {"reads": sum(sorted_search_reads(k) for k in eq_keys)/Q_EQ}
    rr = 0; ok = True
    for k in eq_keys:
        r, n = bt_txn.search(k); rr += n + data_blocks(r); ok &= (len(r) == 1)
    out["BT"] = {"reads": rr/Q_EQ, "correct": ok}
    rr = 0; ok = True
    for k in eq_keys:
        r, n = eh.search(k); rr += n + data_blocks(r); ok &= (len(r) == 1)
    out["EH"] = {"reads": rr/Q_EQ, "correct": ok}
    rr = 0; ok = True
    for k in eq_keys:
        r, n = sh.search(k); rr += n + data_blocks(r); ok &= (len(r) == 1)
    out["SH"] = {"reads": rr/Q_EQ, "correct": ok}
    rr = 0; ok = True
    for k in eq_keys:
        r, n = sh95.search(k); rr += n + data_blocks(r); ok &= (len(r) == 1)
    out["SH95"] = {"reads": rr/Q_EQ, "correct": ok}
    return out

def run_range_txn():
    out = {}
    starts = [rnd.randrange(0, max(1, N-RANGE_LEN)) for _ in range(Q_RANGE)]
    h_reads = 0; s_reads = 0; bt_reads = 0; ok = True
    for st in starts:
        lo = sorted_keys[st]; hi = sorted_keys[min(N-1, st+RANGE_LEN-1)]
        want = sorted(sorted_keys[st:st+RANGE_LEN]); 
        h_reads += b                                     # heap: whole file
        s_reads += sorted_search_reads(lo) + (min(N-1, st+RANGE_LEN-1)//BFR - st//BFR)
        got, n = bt_txn.range(lo, hi)
        bt_reads += n + data_blocks(got)
        ok &= sorted(recs[i][0] for i in got) == want
    out["H"] = {"reads": h_reads/Q_RANGE}
    out["S"] = {"reads": s_reads/Q_RANGE}
    out["BT"] = {"reads": bt_reads/Q_RANGE, "correct": ok}
    out["EH"] = {"reads": None, "note": "hash: no range support, falls back to full scan"}
    out["EH"]["reads"] = b  # full bucket scan
    out["SH"] = {"reads": None, "note": "hash: no range support, full scan", }
    out["SH"]["reads"] = sh.overflow_blocks() + M
    return out

def run_time_window():
    """hourly reconciliation by arrival time: 2,000 consecutive transactions (about 1/10 of a block list step)"""
    rr_sparse = 0; rr_heap = 0; ok = True
    for _ in range(Q_RANGE):
        st = rnd.randrange(0, max(1, N-RANGE_LEN))
        lo = recs[st][2]; hi = recs[min(N-1, st+RANGE_LEN-1)][2]
        got, n = sp_time.range(lo, hi)
        first = got[0] if got else 0
        blocks = (min(N-1, st+RANGE_LEN-1)//BFR) - (st//BFR) + 1
        rr_sparse += n + blocks
        ok &= (len(got) >= 1)
        rr_heap += b
    return {"sparse_time_index": {"reads": rr_sparse/Q_RANGE}, "heap_scan": {"reads": rr_heap/Q_RANGE}}

def run_secondary():
    out = {}
    ka = [recs[i][1] for i in rnd.sample(range(N), Q_ACC)]
    acc_count = {}
    for r_ in recs: acc_count[r_[1]] = acc_count.get(r_[1], 0) + 1
    rr = 0; ok = True
    for k in ka:
        r, n = bt_acc.search(k); rr += n + data_blocks(r)
        ok &= len(r) == acc_count[k] and all(recs[i][1]==k for i in r)
    out["acc_index"] = {"reads": rr/Q_ACC, "rows": sum(len(bt_acc.search(k)[0]) for k in ka[:10])/10, "correct": ok}
    out["acc_scan"] = {"reads": b, "note": "no index: whole heap"}
    kb = [recs[i][3] for i in rnd.sample(range(N), Q_BR)]
    br_count = {}
    for r_ in recs: br_count[r_[3]] = br_count.get(r_[3], 0) + 1
    rr = 0; ok = True
    for k in kb:
        r, n = bt_br.search(k); rr += n + data_blocks(r); ok &= len(r) == br_count[k] and all(recs[i][3]==k for i in r)
    out["br_index"] = {"reads": rr/Q_BR, "correct": ok}
    out["br_scan"] = {"reads": b}
    return out

def run_inserts():
    """insert Q_INS new transactions into each strategy (the same 100 new records)"""
    out = {}
    new = []
    for j in range(Q_INS):
        t = rng.getrandbits(64) if WL == "gateway" else 1_000_000 + N + j
        new.append((t, acc_ids[rng.randrange(NACC)], BASE + (N+j)*STEP_MS, rng.randrange(NBR)))
    # heap: append to last block (1 read + 1 write), new block when full
    out["H"] = {"reads": 1*Q_INS, "writes": (Q_INS + Q_INS//BFR)}
    # sorted file: append at end (orig) or shift blocks after insertion point (gateway)
    if WL == "orig":
        out["S"] = {"reads": Q_INS, "writes": Q_INS}
    else:
        rd = wr = 0
        for t, *_ in new:
            p = bisect.bisect_left(sorted_keys, t)
            rd += (b - p//BFR); wr += (b - p//BFR)  # blocks at or after the insertion point are read and rewritten
        out["S"] = {"reads": rd/Q_INS, "writes": wr/Q_INS, "note": "average over 100 inserts (blocks shifted)"}
    # B+ tree: count with a temporary copy so the real index is unchanged
    tmp = bt_txn; tmp.reads = tmp.writes = 0; leaves0 = tmp.leaves
    for j, (t, a, tm, br) in enumerate(new): tmp.insert(t, N+j)
    out["BT"] = {"reads": tmp.reads/Q_INS, "writes": (tmp.writes+Q_INS)/Q_INS, "splits_leaves": tmp.leaves - leaves0}
    tmp2 = eh; tmp2.reads = tmp2.writes = 0; sp0, dp0 = eh.splits, eh.doublings
    for j, (t, a, tm, br) in enumerate(new): tmp2.insert(t, N+j)
    out["EH"] = {"reads": tmp2.reads/Q_INS, "writes": (tmp2.writes+Q_INS)/Q_INS, "splits": tmp2.splits-sp0, "doublings": tmp2.doublings-dp0}
    tmp3 = bt_acc; tmp3.reads = tmp3.writes = 0
    for j, (t, a, tm, br) in enumerate(new): tmp3.insert(a, N+j)
    out["acc_index_insert"] = {"reads": tmp3.reads/Q_INS, "writes": tmp3.writes/Q_INS}
    tmp4 = bt_br; tmp4.reads = tmp4.writes = 0
    for j, (t, a, tm, br) in enumerate(new): tmp4.insert(br, N+j)
    out["br_index_insert"] = {"reads": tmp4.reads/Q_INS, "writes": tmp4.writes/Q_INS}
    return out

def run_deletes():
    out = {}
    dk = [recs[i][0] for i in rnd.sample(range(N), Q_DEL)]
    out["H"] = {"reads": sum(heap_reads_for(k) for k in dk)/Q_DEL, "writes": 1}
    shifts = [b - sorted_rank[k]//BFR for k in dk]
    out["S"] = {"reads": sum(sorted_search_reads(k) + sh_ for k, sh_ in zip(dk, shifts))/Q_DEL, "writes": sum(shifts)/Q_DEL, "note": "locate, then rewrite every later block to close the gap"}
    tmp = bt_txn; tmp.reads = tmp.writes = 0
    ok = True
    for k in dk:
        ok &= tmp.delete(k)
    tmp.check()
    out["BT"] = {"reads": tmp.reads/Q_DEL, "writes": tmp.writes/Q_DEL, "correct": ok, "merges_or_redists_note": "counted in tree"}
    out["EH"] = {"reads": 1, "writes": 1, "note": "bucket locate + write, no merging"}
    out["SH"] = {"reads": 1, "writes": 1, "note": "bucket locate + write"}
    return out

def sizes_util():
    return {
        "heap_bytes": b*B, "sorted_bytes": b*B,
        "bt_txn_bytes": bt_txn.size_bytes(), "bt_txn_height": bt_txn.height, "bt_txn_leaves": bt_txn.leaves, "bt_txn_inner": bt_txn.inners,
        "bt_txn_util": bt_txn.utilisation(),
        "sp_time_bytes": sp_time.size_bytes(), "sp_time_height": sp_time.height,
        "bt_acc_bytes": bt_acc.size_bytes(), "bt_acc_height": bt_acc.height, "bt_acc_util": bt_acc.utilisation(),
        "bt_br_bytes": bt_br.size_bytes(), "bt_br_height": bt_br.height, "bt_br_util": bt_br.utilisation(),
        "eh_bytes": eh.size_bytes(), "eh_dir_entries": len(eh.dir), "eh_global_depth": eh.gd, "eh_util": eh.utilisation(), "eh_buckets": len(eh.buckets()),
        "sh_bytes": (M + sh.overflow_blocks())*B, "sh_overflow_blocks": sh.overflow_blocks(), "sh_buckets": M,
        "sh95_overflow_blocks": sh95.overflow_blocks(), "sh95_buckets": M95, "sh95_bytes": (M95 + sh95.overflow_blocks())*B,
        "sh95_longest_chain": max(len(c) for c in sh95.buckets),
    }

res["build"] = {"bt_txn": build_bt, "bt_acc": build_acc, "bt_br": build_br, "eh": build_eh}
res["sizes"] = sizes_util()
T1=time.perf_counter(); res["eq_txn"] = run_eq_txn(); res["ms_eq_txn"] = (time.perf_counter()-T1)*1000/Q_EQ
T1=time.perf_counter(); res["range_txn"] = run_range_txn(); res["ms_range_txn"] = (time.perf_counter()-T1)*1000/Q_RANGE
res["time_window"] = run_time_window()
T1=time.perf_counter(); res["secondary"] = run_secondary(); res["ms_secondary"] = (time.perf_counter()-T1)*1000/(Q_ACC+Q_BR)
T1=time.perf_counter(); res["insert"] = run_inserts(); res["ms_insert"] = (time.perf_counter()-T1)*1000/Q_INS
T1=time.perf_counter(); res["delete"] = run_deletes(); res["ms_delete"] = (time.perf_counter()-T1)*1000/Q_DEL
res["check_bt_txn"] = bt_txn.check()
res["wall_seconds"] = time.time() - T0
json.dump(res, open(f"../out/exp_{WL}_{N}.json", "w"), indent=1, default=float)
print("done", WL, N, round(res["wall_seconds"],1), "s", res["sizes"]["bt_txn_height"], res["eq_txn"]["BT"])
