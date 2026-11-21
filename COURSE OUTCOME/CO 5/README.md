# CSA03-1972260035-Assignment5
Name: MONISH.L | Register number: 1972260035 | Course: Data Structures (CSA0307/CSA03), CO5 Assignment 5 (graphs)
Compiler: gcc 13.3.0 (Ubuntu 24.04), C11.

## Compile and run
    gcc -std=c11 -Wall -Wextra -O2 <file>.c -o <name> [-lm]   # -lm for b3_compare.c and b4_extension.c
    ./<name>
    gcc -std=c11 -Wall -Wextra -g -fsanitize=address,undefined <file>.c -o <name> -lm && ./<name>   # sanitizer run
    (ulimit -s 8192; ./b1_stack rec 200000)     # recursive DFS stack-overflow demo (exit 139)
    ./b1_stack stack 5000000                    # explicit-stack DFS on the same kind of chain

## Results
- TaskA2: DFS `0 1 3 5 4 2 ` and BFS `0 1 2 3 4 5` (predicted before running, matched).
- TaskA3: Kahn gives Lease Power Racks Staff Network Launch / valid schedule; with Launch->Lease: cycle detected.
- TaskA4: Floyd-Warshall matrix matched prediction; d[0][3] 10 -> 9.
- TaskA5: Prim 0-1(2) 1-2(3) 0-3(6) 1-4(5), total 16; Kruskal gives the same total.
- TaskB1: BFS route Koyambedu 3 stops; flood regions 1/2/3/4; matrix vs list BFS at 1,000/10,000/50,000 vertices.
- TaskB2: Dijkstra complete-the-code matches expected; difficult inputs (unreachable, negative edge, overflow, cycle).
- TaskB3: Dijkstra (array, heap) and Floyd-Warshall agree; timings; A* vs Dijkstra on a 500x500 grid.
- TaskB4: Prim cable plan, replanning, 250,000-junction Dijkstra, precomputed table.
Estimates (Floyd at 10,000 vertices etc.) are marked "est." in the outputs and the report. Timings: shared 2-core machine.
