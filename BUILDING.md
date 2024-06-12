# Building

Six main programs can be built from these source files.  
* unittests, which is, you guessed it, unit tests of lower-level components plus some small complete problems to solve.
* KSolve, the program described in README.md.  Provides various ways of entering a deal to solve and various output options, all text-based.
* ran, which runs a batch of random deals for statistical purposes.  "ran -?" will print its flags.
* benchmark, which runs a test problem ten times and prints the smallest of the ten run times.  It provides a quick sanity check for large changes in performance.
* KSolve2Solvitaire, which writes a file of input for the commercial program Solvitaire using the same flags and inputs
that KSolve uses.
*thread-test, which generates data for evaluating the effectiveness of multithreading.

To build any one of them except KSolve2Solvitaire,
compile and link Game.cpp, KSolveAStar.cpp, GameStateMemory.cpp and the .cpp file 
with the same name as
the program you wish to build. 
KSolve2Solvitaire needs only KSolve2Solvitaire.cpp and Game.cpp.
When using g++, you will need the -pthread flag.  Once you
have a program working, I recommend full optimization as well.  It works built for 32-bit or  64-bit operation,
but will solve only very small problems as a 32-bit executable.

This code was developed on Linux and is already configured for VSCode using the g++ compiler.
It is occasionally built and tested using CLang on Linux and MSVC on Windows. It requires the 
2020 level of C++.
