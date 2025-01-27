# Statistics
## How Many Moves are Needed
This table reports the observed proportion of deals that were decided (solved or found to be unsolvable)
using a number of moves in the move tree no greater than the given number.
The sample size was 20,000 deals in each case. Data were generated running _ran_ with a 
seed of 1 and an increment of 1.

Moves Limit|Draw 1 |Draw 3
-----------|-------|------
 10,000,000|93.9%|96.6%
 20,000,000|97.3%|98.8%
 30,000,000|98.2%|99.3%
 40,000,000|98.6%|99.5%
 50,000,000|98.9%|99.6%
 60,000,000|99.1%|99.7%
 70,000,000|99.2%|99.8%
 80,000,000|99.3%|99.8%
 90,000,000|99.4%|99.8%
 100,000,000|99.4%|99.8%
 110,000,000|99.5%|99.8%
 120,000,000|99.5%|99.9%
 130,000,000|99.5%|99.9%
 140,000,000|99.6%|99.9%
 150,000,000|99.6%|99.9%
 ## How Much Memory is Needed
 These figures were acquired by running a deal too complex to be decided with any of the 
 move limits tested and observing the peak memory used.  They were run on a Linux machine
 with the _ran_ program compiled with the GNU g++ compiler, although the compiler used 
 will make little difference. The figures would be nearly the same for the _KSolve_ program. The script to generate them is
 in _statistics/MemoryNeeded.sh_.

Move Limit|Draw 1 Peak|Draw 3 Peak
-----------|------|--------
10,000,000|1,981MB|955MB
20,000,000|3,931MB|1,909MB
30,000,000|7,451MB|3,650MB
40,000,000|7,774MB|3,797MB
50,000,000|8,057MB|3,931MB
60,000,000|14,739MB|7,277MB
70,000,000|15,043MB|7,417MB
80,000,000|15,311MB|7,530MB
90,000,000|15,559MB|7,678MB
100,000,000|15,842MB|7,791MB
110,000,000|16,067MB|7,917MB
120,000,000|17,052MB|14,456MB
130,000,000|28,328MB|14,535MB
140,000,000|28,385MB|14,671MB
150,000,000|28,357MB|14,797MB


