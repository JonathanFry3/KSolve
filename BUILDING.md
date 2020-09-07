#Building

Three main programs can be built from these source file.  To build any one of them,
compile and link Game.cpp, KSolve.cpp, and the source file with the same name as
the program you wish to build. I have built and tested them with g++ 7.5.0 and 9.3.0 on Ubuntu
Linux and g++ 9.2.0 on Windows 10.  

The programs are:

* unittests, which is, you guessed it, unit tests of lower-level components with small-problem runs solutions.
* KSolver, the program described in README.md.  Provides various ways of entering a deal to solve and various output options, all text-based.
* ran, which runs a batch of random deals for statistical purposes.

#Threading
After a long time dithering, I elected to drop the single-threaded version and 
provide only the multithreaded version of this program.  It considerably complicates 
the code, but  runs substantially faster.  The algorithm is not a great fit for that 
mode, so it does not benefit from a large number of threads.  Two threads are 
clearly faster than one; three are usually faster than two.  Four threadsare 
likely to be slower than three.  When compiling, set the environment variable 
NTHREADS to the number you want.  If it is not defined, the program will use 
std::thread::hardware_concurrency() in attempt to determine a good setting.