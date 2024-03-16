# Statistics
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


