# CO5 Assignment 5 - predictions written BEFORE compiling or running any program (hand-traced)

## A2 Predict the Output (DFS on 6-vertex matrix from vertex 0)
Neighbours: 0:{1,2} 1:{0,3,4} 2:{0,4} 3:{1,5} 4:{1,2,5} 5:{3,4}
dfs(0) prints 0 -> dfs(1) prints 1 (0 seen) -> dfs(3) prints 3 (1 seen) -> dfs(5) prints 5 (3 seen) -> dfs(4) prints 4 (1 seen) -> dfs(2) prints 2 (0,4 seen); back out, nothing left.
Predicted: "0 1 3 5 4 2 " (trailing space, then newline)
BFS from 0: queue [0]; 0 -> adds 1,2; 1 -> adds 3,4; 2 -> none; 3 -> adds 5; 4 -> none; 5.
Predicted BFS order: 0 1 2 3 4 5

## A4 Predict the Output (Floyd-Warshall, X = 999, printed with %4d)
k=0: column 0 is 999 except d[0][0], so no change (999+x never < current).
k=1: d[0][2] = d[0][1]+d[1][2] = 5+3 = 8 (was 999).  d[0][3]: 5+999 no.
k=2: d[0][3] = d[0][2]+d[2][3] = 8+1 = 9 (was 10); d[1][3] = 3+1 = 4 (was 999).
k=3: row 3 is 999 except diagonal, no change.
Predicted output:
   0   5   8   9
 999   0   3   4
 999 999   0   1
 999 999 999   0
Row 0 column 3 changes 10 -> 9 because the path 0-1-2-3 costs 5+3+1 = 9 < direct 10, found at k=2.

## A3 Complete the Code (Kahn): expected Lease Power Racks Staff Network Launch / valid schedule
indeg: Lease0 Power1 Racks1 Network2 Staff1 Launch2.  Queue order: Lease; then Power,Racks,Staff; Network after Racks; Launch last.
With Launch->Lease added: indeg[Lease]=1, queue starts empty, done=0 -> "cycle detected" and nothing printed before it.

## A5 Complete the Code (Prim): expected 0-1(2) 1-2(3) 0-3(6) 1-4(5) / total cost = 16
Pick 0; key1=2,key3=6. Pick 1(2); key2=3,key4=5 (3 stays 6). Pick 2(3); 4: 7>5 stays. Pick 4(5); 3: 9>6 stays. Pick 3(6). Total 2+3+5+6=16.
Tree route 4 -> 1 -> 2 costs 5+3 = 8, but the direct road 4-2 costs 7: tree path is not cheapest.

## B1 Complete the Code (BFS on metro): expected Airport Guindy Saidapet Vadapalani Central Koyambedu Egmore / Koyambedu is 3 stops away via: Koyambedu Vadapalani Guindy Airport
Queue after each removal: remove Airport [Guindy]; Guindy [Saidapet,Vadapalani]; Saidapet [Vadapalani,Central]; Vadapalani [Central,Koyambedu]; Central [Koyambedu,Egmore]; Koyambedu [Egmore]; Egmore [].

## B2 Complete the Code (Dijkstra): expected 0 7 9 20 20 11 / route to 4: 4 5 2 0
dist: 1=7, 2=9, 5=min(14, 9+2)=11, 3=9+11=20, 4=min(20 via 3? 20+6=26, 11+9=20)=20 via 5.
