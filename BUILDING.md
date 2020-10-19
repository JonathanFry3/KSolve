# Building

Three main programs can be built from these source file.  To build any one of them,
compile and link Game.cpp, KSolveAStar.cpp, and the source file with the same name as
the program you wish to build.  When using g++, you will need the -pthread flag.  Once you
have a program working, I recommend -O3 as well.  It works built for 32-bit or  64-bit operation,
but will solve only small problems as a 32-bit executable.
I have built and tested them with g++ 7.5.0 and 9.3.0 on Ubuntu
Linux, g++ 9.2.0 and MVSC's cl.exe 19.28.29331 on Windows 10.  

The programs are:

* unittests, which is, you guessed it, unit tests of lower-level components plus some small complete problems to solve.
* KSolve, the program described in README.md.  Provides various ways of entering a deal to solve and various output options, all text-based.
* ran, which runs a batch of random deals for statistical purposes.  "ran -?" will display its flags.
