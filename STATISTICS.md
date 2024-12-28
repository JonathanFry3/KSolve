# Statistics
## How Many Moves are Needed
This table reports the observed proportion of deals that were decided (solved or found to be unsolvable)
using a number of moves in the move tree no greater than the given number.
The sample size was 20,000 deals in each case. Data were generated running _ran_ with a 
seed of 1 and an increment of 1.

Moves Limit|Draw 1 |Draw 3
-----------|-------|------
 10,000,000|93.8%|96.5%
 20,000,000|97.0%|98.5%
 30,000,000|99.9%|99.1%
 40,000,000|98.4%|99.4%
 50,000,000|98.7%|99.6%
 60,000,000|98.9%|99.7%
 70,000,000|99.0%|99.7%
 80,000,000|99.2%|99.8%
 90,000,000|99.3%|99.8%
 100,000,000|99.4%|99.8%
 110,000,000|99.4%|99.9%
 120,000,000|99.4%|99.9%
 130,000,000|99.5%|99.9%
 140,000,000|99.5%|99.9%
 150,000,000|99.6%|99.9%
 160,000,000|99.6%|99.9%
 ## How Much Memory is Needed
 These figures were acquired by running a deal too complex to be decided with any of the 
 move limits tested and observing the peak memory used.  They were run on a Linux machine
 with the _ran_ program compiled with the GNU g++ compiler, although the compiler used 
 will make little difference. The figures would be nearly the same for the _KSolve_ program. The script to generate them is
 in _statistics/MemoryNeeded.sh_.

Move Limit|Draw 1 Peak|Draw 3 Peak
-----------|------|--------
10,000,000|1,821MB|970MB
20,000,000|3,613MB|1,908MB
30,000,000|3,740MB|3,627MB
40,000,000|3,867MB|3,747MB
50,000,000|7,183MB|3,867MB
60,000,000|7,318MB|3,976MB
70,000,000|7,439MB|7,269MB
80,000,000|7,562MB|7,383MB
90,000,000|7,686MB|7,474MB
100,000,000|7,823MB|7,577MB
110,000,000|14,321MB|7,663MB
120,000,000|14,459MB|7,759MB
130,000,000|14,572MB|8,415MB
140,000,000|14,685MB|14,333MB
150,000,000|14,822MB|14,410MB
160,000,000|14,918MB|14,499MB


