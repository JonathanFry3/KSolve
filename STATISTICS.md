# Statistics
These statistics were based on 2000 random deals run once with draw 1 specified 
and once with draw 3 specified.  The commands required to reproduce the sample 
using the _ran_ program from this repository are:

    ran -s 29347 -i 2349 -e 2000 -mv 110000000 > ran-d1-e2000.txt
    ran -s 29347 -i 2349 -e 2000 -mv 110000000 -d 3 > ran-d3-e2000.txt
## Winning Proportion
To compute the proportion of winnable deals, one must decide what to do with deals too complex to
solve using the number of states allowed.  In his statistics report, @shootme concluded that such deals should
be counted as solved.  I tested statistically whether the outcome proportion varied with the size of the move tree required  
for the decided deals and found something interesting.  

For draw 3 deals, there was no evidence of dependency (α = 0.595).  
81.2% of 1748 deals needing a tree of 10 million moves or fewer were solvable compared with 82.8% of 239 deals whose trees needed more than
10 million but fewer than 110 million. 

For draw 1 deals, however, there was clear evidence of dependency 
(α = 0.016).
92.6% of  1580 deals needing trees with 10 million or fewer moves were solvable compared with 88.7 % of 379 deals whose trees needed more than
10 million but fewer than 110 million moves. (Significances were 
computed using Fisher's Exact Test.)

Since the computers available to me would not handle trees larger than
110 million moves, those are excluded from the statistics below.  Be aware,
however, that the true winnable proportion for draw 1 is almost certainly lower than that given here.
The ranges below are Wilson score intervals for the proportion winnable 
with a confidence level of 0.95.

Draw | Winnable
---- | --------
1    | 91.75% ± 1.214%
3    | 81.55% ± 1.702%
## How Many Moves are Needed
This table reports the observed proportion of deals that were decided (solved or found to be unsolvable)
using a number of moves in the move tree no greater than the given number.
The sample size was 2,000 deals in each case.

Moves Limit|Draw 1 |Draw 3
-----------|-------|------
 10,000,000|79.1% |88.6%
 20,000,000|87.6% |94.5%
 30,000,000|91.7% |97.1%
 40,000,000|93.8% |98.0%
 50,000,000|95.0% |98.6%
 60,000,000|96.1% |98.7%
 70,000,000|96.7% |98.8%
 80,000,000|97.1% |99.1%
 90,000,000|97.3% |99.3%
 100,000,000|97.8% |99.4%
 110,000,000|97.9% |99.5%
 ## How Much Memory is Needed
 These figures were acquired by running a deal too complex to be decided with any of the 
 move limits tested and observing the peak memory used.  They were run on a Linux machine
 with the _ran_ program compiled with the GNU g++ compiler, although the compiler used 
 will make little difference. The figures would be nearly the same for the _KSolve_ program.

 Moves Limit|Peak Memory
 -----------|-----------
 10,000,000 |640MB
 20,000,000 |1270MB
 30,000,000 |1380MB
 40,000,000 |2530MB
 50,000,000 |2640MB
 60,000,000 |2740MB 
 70,000,000 |4960MB
 80,000,000 |5060MB
 90,000,000 |5160MB
 100,000,000 |5260MB
 110,000,000 |5370MB


