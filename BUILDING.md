#Building

Three main programs can be built from these source file.  To build any one of them,
compile and link Game.cpp, KSolve.cpp, and the source file with the same name as
the program you wish to build. I have built and tested them only with g++ 7.5.0 on Ubuntu
Linux.  

The programs are:

* unittests, which is, you guessed it, unit tests of lower-level components.
* speeder, which was used to test the effects of various optimizations.  The shortest time from each was saved in the file timetests.txt.  This program may not be of any use to anyone but me.
* KSolver, the program described in README.md.  Provides various ways of entering a deal to solve and various output options, all text-based.