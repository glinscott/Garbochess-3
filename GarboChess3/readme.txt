Killer moves need to be indexed by treeHeight as opposed to depth.  I don't track treeHeight yet though.

- 8.5 million NPS on perft currently - 32 bit
- 16 million NPS on perft 64bit now after psq tables

WAC results:
0 ply - 103/300
1 ply - 164/300
2 ply - 206/300
3 ply - 245/300
4 ply - 261/300
5 ply - 266/300 (w/ null move)
6 ply - 278/300 (w/ null move)

1. Rb7 -> expected Rxb2
29. Qc3+ -> expected Nxd6
37. Qd3 -> expected Rd8+
99. Be3 -> expected b6+
140. Kf1 -> expected Qxf4
162. cxd5 -> expected Qg2+
221. bxa3 -> expected Bf6
228. Rh8 -> expected Rxh4
229. a4 -> expected Rb4
234. Qe4 -> expected Qxf7+
246. Qc5+ -> expected Rxb5
264. Qxe4 -> expected exf6
269. Rxe8+ -> expected Qg4
73671788Passed: 287/300