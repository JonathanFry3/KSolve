# Building

Three main programs can be built from these source file.  To build any one of them,
compile and link Game.cpp, KSolve.cpp, and the source file with the same name as
the program you wish to build. I have built and tested them with g++ 7.5.0 and 9.3.0 on Ubuntu
Linux and g++ 9.2.0 on Windows 10.  

The programs are:

* unittests, which is, you guessed it, unit tests of lower-level components plus some small complete problems to solve.
* KSolver, the program described in README.md.  Provides various ways of entering a deal to solve and various output options, all text-based.
* ran, which runs a batch of random deals for statistical purposes.
