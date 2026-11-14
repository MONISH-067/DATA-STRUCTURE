# CSA0307 Data Structures – Assignment 3 (CO3): Tree-based storage and retrieval
Name: MONISH.L  Reg No: 1972260035  (SIMATS Engineering)

| Question | Source | Output |
|---|---|---|
| Q1 BST (+ 2 Predict the Output, Complete the Code) | src/q1_bst.c, q1_complete.c, predict1_array_tree.c, predict2_bst_delete.c | output/q1_*.txt, predict*.txt |
| Q2 AVL (+ Complete the Code) | src/q2_avl.c, q2_complete.c | output/q2_*.txt |
| Q3 B-tree | src/q3_btree.c | output/q3_btree.txt |
| Q4 Red-black vs AVL | src/q4_rb_avl.c | output/q4_rb_avl.txt |
| Q5 Splay vs AVL | src/q5_splay.c | output/q5_splay.txt |
| Q6 Trie (+ Complete the Code, ASan) | src/q6_trie.c, q6_complete.c | output/q6_*.txt |
| Q7 Dictionary design | src/q7_dict.c | output/q7_dict.txt (200,000), q7_dict_2M.txt (2,000,000) |

Build and run: `gcc -O2 -Wall -fsanitize=address,undefined src/qX.c -o qX && ./qX` (q7: `./q7_dict 200000`).
Programs write fig_q*.txt (tree data for figures); figures_data/figs.py draws them (Graphviz + matplotlib).
Random numbers use a seeded xorshift generator, so counts are reproducible; times depend on the machine.
Names (Q6) and words (Q7) are generated; the Q5 "given tree" is assumed (see report).
Full report: REPORT/REPORT.pdf
